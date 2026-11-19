"""Build the actual player mains with deterministic serial/stdio fault fixtures.

No device is opened. This checks host framing/control flow, not JTAG hardware.
Use --revision <commit> --record-before for safe pre-fix evidence.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PLAYERS = ("BP4_XSVFplayer_Win", "BPXSVFPlayer")

MOCK = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "buspirate.h"
static const char *scenario;
static unsigned char queue[1024];
static unsigned head,tail,phase,chunk_length,chunks,bytes,reads,starts,start_pending,scanned,closes;
static FILE *closed_file;
static void push(unsigned v) { queue[tail++]=v; }
static int is(const char *s) { return !strcmp(scenario,s); }
static void report(void) {
 printf("\nTRACE chunks=%u bytes=%u starts=%u start_pending=%u pending=%u reads=%u closes=%u\n",
        chunks,bytes,starts,start_pending,tail-head,reads,closes);
}
int xsvf_player_main(int argc,char **argv);
int main(int argc,char **argv) {
 SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
 scenario=getenv("XSVF_CASE");if(!scenario)scenario="success";
 atexit(report);
 return xsvf_player_main(argc,argv);
}
int serial_open(char *port) { (void)port;return is("open-fail") ? -1 : 7; }
int serial_setup(int fd,speed_t speed) { (void)fd;(void)speed;return is("setup-fail") ? -1 : 0; }
int serial_close(int fd) { (void)fd;return 0; }
int BP_EnableBinary(int fd) { (void)fd;return is("binary-fail") ? ERR : BBIO; }
int BP_EnableMode(int fd,char mode) { (void)fd;(void)mode;return is("mode-fail") ? ERR : JTAG; }
int serial_write(int fd,char *buf,int size) {
 (void)fd;
 if(phase==2) {
  if((unsigned)size!=chunk_length) { puts("BAD_CHUNK_LENGTH");exit(99); }
  ++chunks;bytes+=size;phase=1;
  if(is("payload-fail"))return size-1;
  push(is("early-eof") ? 255 : is("error-after-chunk") ? 2 : 0);
  return size;
 }
 if(size==1 && (unsigned char)buf[0]==1 && is("reset-write-fail"))return 0;
 if(phase==1 && size==2) {
  chunk_length=((unsigned char)buf[0]<<8)|(unsigned char)buf[1];
  if(is("header-fail"))return 1;
  phase=2;return size;
 }
 if(size==1 && (unsigned char)buf[0]==2) {
  if(is("scan-write-fail"))return 0;
  scanned=1;
  if(!is("scan-count-timeout")) {
   unsigned count=is("scan-zero") ? 0 : is("scan-max") ? 252 : 4;
   push(count);
   if(!is("scan-payload-timeout"))for(unsigned i=0;i<count;++i)push(i%4==0 ? 2 : i%4==1 ? 0x40 : i%4==2 ? 0x74 : 0x89);
  }
  return size;
 }
 if(size==1 && (unsigned char)buf[0]==3) {
  ++starts;start_pending=tail-head;phase=1;
  if(is("start-fail"))return 0;
  if(!strncmp(scenario,"result-",7))push(atoi(scenario+7));
  else if(is("early-error"))push(2);
  else if(is("unknown-result"))push(128);
  else if(is("immediate-success"))push(0);
  else if(!is("no-result"))push(255);
 }
 return size;
}
int serial_read(int fd,char *buf,int size) {
 (void)fd;++reads;
 if(head==tail)return is("scan-count-timeout") ? 0 : -1;
 unsigned n=tail-head;
 if(n>(unsigned)size)n=size;
 /* First scan read returns only its count. Later reads may fragment IDs. */
 if(scanned && (head==0 || is("scan-fragmented") || is("scan-max")))n=1;
 memcpy(buf,queue+head,n);head+=n;return n;
}
int xsvf_test_fseek(FILE *file,long offset,int origin) {
 if(!file) { puts("NULL_FILE_SEEK");exit(98); }
 if(is("file-seek-fail"))return -1;
 return fseek(file,offset,origin);
}
int xsvf_test_fclose(FILE *file) {
 ++closes;
 /* Isolate the old Windows double-fclose so unrelated result tests can finish. */
 if(file==closed_file) { puts("DUPLICATE_FILE_CLOSE");return 0; }
 closed_file=file;return fclose(file);
}
long xsvf_test_ftell(FILE *file) { return is("file-tell-fail") ? -1 : ftell(file); }
size_t xsvf_test_fread(void *buf,size_t size,size_t n,FILE *file) {
 return fread(buf,size,is("file-read-fail") ? n/2 : n,file);
}
'''

# case: (exit status, chunk writes, player starts, required output)
CASES = {
    "success": (0, 1, 1, "Success!"),
    "immediate-success": (0, 0, 1, "Success!"),
    "early-error": (1, 0, 1, "XSVF_ERROR_TDOMISMATCH"),
    "error-after-chunk": (1, 1, 1, "XSVF_ERROR_TDOMISMATCH"),
    "unknown-result": (1, 0, 1, "XSVF_ERROR_UNKNOWN"),
    "early-eof": (2, 1, 1, "End of file reached"),
    "no-result": (2, 0, 1, "No reply"),
    "missing-file": (2, 0, 0, "Error opening file"),
    "empty-file": (2, 0, 0, "file"),
    "file-seek-fail": (2, 0, 0, "file"),
    "file-tell-fail": (2, 0, 0, "file"),
    "file-read-fail": (2, 0, 0, "Error reading file"),
    "open-fail": (2, 0, 0, "Error opening serial port"),
    "setup-fail": (2, 0, 0, "serial port"),
    "binary-fail": (2, 0, 0, "Binary mode"),
    "mode-fail": (2, 0, 0, "player mode"),
    "start-fail": (2, 0, 1, "send"),
    "header-fail": (2, 0, 1, "send"),
    "payload-fail": (2, 1, 1, "send"),
    "scan-fragmented": (0, 1, 1, "Chain Scan Result"),
    "scan-zero": (0, 1, 1, "Chain Scan Result"),
    "scan-max": (0, 1, 1, "Chain Scan Result"),
    "scan-count-timeout": (2, 0, 0, "Chain scan"),
    "scan-payload-timeout": (2, 0, 0, "Chain scan"),
    "scan-write-fail": (2, 0, 0, "Chain scan"),
    "scan-only": (0, 0, 0, "Chain Scan Result"),
    "reset-only": (0, 0, 0, "Done"),
    "reset-write-fail": (2, 0, 0, "send"),
    "missing-args": (2, 0, 0, "Usage"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--record-before", action="store_true")
    parser.add_argument("--all-results", action="store_true", help="Exercise every possible result byte")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    import os
    with tempfile.TemporaryDirectory(prefix="bp-xsvf-host-", dir=Path.home() / "AppData/Local/Temp/opencode") as tmp:
        out = args.output.resolve() if args.output else Path(tmp)
        out.mkdir(parents=True, exist_ok=True)
        mock = out / "mock.c"
        mock.write_text(MOCK, encoding="ascii")
        payload = out / "payload.xsvf"
        payload.write_bytes(b"\0" * 4097)
        small = out / "small.xsvf"
        small.write_bytes(b"\0" * 16)
        empty = out / "empty.xsvf"
        empty.write_bytes(b"")
        results = []
        for player in PLAYERS:
            src = ROOT / "scripts" / player
            text = subprocess.check_output(
                ["git", "show", args.revision + ":scripts/" + player + "/main.c"],
                cwd=ROOT, text=True) if args.revision else (src / "main.c").read_text()
            c = out / (player + "-main.c")
            c.write_text(text, encoding="utf-8")
            obj, exe = c.with_suffix(".o"), c.with_suffix(".exe")
            flags = ["gcc", "-std=gnu99", "-O0", "-DWIN32", "-I" + str(src)]
            build = subprocess.run(flags + ["-Dmain=xsvf_player_main", "-Dfseek=xsvf_test_fseek",
                                           "-Dfclose=xsvf_test_fclose", "-Dftell=xsvf_test_ftell",
                                           "-Dfread=xsvf_test_fread", "-c", str(c), "-o", str(obj)],
                                   capture_output=True, text=True)
            (out / (player + "-build.txt")).write_text(build.stdout + build.stderr, encoding="utf-8")
            build.check_returncode()
            subprocess.run(flags + [str(obj), str(mock), "-o", str(exe)], check=True)
            cases = dict(CASES)
            if args.all_results:
                for value in range(256):
                    cases["result-" + str(value)] = (
                        0 if value in (0, 255) else 1, 1 if value == 255 else 0, 1,
                        "Success!" if value in (0, 255) else "XSVF_ERROR_UNKNOWN" if value > 7 else "End of operation reply")
            for case, (status, chunks, starts, message) in cases.items():
                filename = (empty if case == "empty-file" else out / "absent.xsvf" if case == "missing-file"
                            else payload if case == "error-after-chunk" else small)
                command = [str(exe), "-p", "MOCK", "-f", str(filename)]
                if case.startswith("scan-"):
                    command += ["-x"]
                if case in ("scan-only", "reset-only", "reset-write-fail"):
                    command = [str(exe), "-p", "MOCK", "-x" if case == "scan-only" else "-r"]
                if case == "missing-args":
                    command = [str(exe)]
                run = subprocess.run(command, env={**os.environ, "XSVF_CASE": case},
                                     capture_output=True, text=True, timeout=10)
                output = run.stdout + run.stderr
                (out / (player + "-" + case + ".txt")).write_text(output, encoding="utf-8")
                trace = re.search(r"TRACE chunks=(\d+) bytes=(\d+) starts=(\d+) start_pending=(\d+) pending=(\d+)", output)
                actual = tuple(map(int, trace.groups())) if trace else None
                ok = (run.returncode == status and actual is not None and actual[0] == chunks and
                      actual[2] == starts and actual[3] == 0 and actual[4] == 0 and message.lower() in output.lower() and
                      "NULL_FILE_SEEK" not in output and "DUPLICATE_FILE_CLOSE" not in output)
                results.append({"player": player, "case": case, "rc": run.returncode,
                                "trace": actual, "ok": ok, "expected_rc": status})
                if not case.startswith("result-") or not ok:
                    print(player, case, "rc", run.returncode, "trace", actual, "PASS" if ok else "FAIL", flush=True)
        (out / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
        print(sum(r["ok"] for r in results), "/", len(results), "pass", flush=True)
        if not args.record_before:
            assert all(r["ok"] for r in results), "See fixture transcripts: " + str(out)


if __name__ == "__main__":
    main()
