"""Build I2CEEPROMWIN with MinGW and exercise its actual main with Win32 I/O mocks."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Firmware/ra28-tests"
OUT.mkdir(parents=True, exist_ok=True)
TOOL = ROOT / "scripts/I2CEEPROMWIN.c"

MOCK = r'''
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned attempts, tries, waits, closes, cleanup, timeouts, reads, acks, nacks, completed;
static unsigned char packet[32];
static unsigned size;
static const char *scenario;
static void report(void) {
    printf("\nMOCK attempts=%u waits=%u closes=%u cleanup=%u timeouts=%u reads=%u acks=%u nacks=%u\n",
           attempts, waits, closes, cleanup, timeouts, reads, acks, nacks);
}
static HANDLE WINAPI fake_open(LPCSTR name, DWORD a, DWORD b, LPSECURITY_ATTRIBUTES c,
                               DWORD d, DWORD e, HANDLE f) {
    (void)name;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
    scenario=getenv("I2C_TEST_SCENARIO"); atexit(report); return (HANDLE)1;
}
static BOOL WINAPI fake_get(HANDLE h, LPDCB d) { (void)h; memset(d,0,sizeof(*d));return TRUE; }
static BOOL WINAPI fake_set(HANDLE h, LPDCB d) { (void)h;(void)d;return TRUE; }
BOOL WINAPI fake_timeout(HANDLE h, LPCOMMTIMEOUTS t) {
    (void)h; ++timeouts;
    if(t->ReadTotalTimeoutConstant!=1000 || t->WriteTotalTimeoutConstant!=1000) abort();
    return strcmp(scenario,"timeout-config")!=0;
}
BOOL WINAPI fake_close(HANDLE h) { (void)h;++closes;return TRUE; }
static void WINAPI fake_sleep(DWORD n) { if(n==1) ++waits; }
static BOOL WINAPI fake_write(HANDLE h, LPCVOID data, DWORD n, LPDWORD done, LPOVERLAPPED o) {
    (void)h;(void)o; size=n;memcpy(packet,data,n);*done=n;
    if(n==3 && !memcmp(data,"\x03\x00\x0F",3)) ++cleanup;
    if(getenv("I2C_TEST_WRITE") && (n==6 || n==7) && packet[0]==2) {
        if(packet[n-1]!=3 || packet[n-3]!=completed ||
           packet[n-2]!=(completed==0 ? 0xAA : 0xBB) || packet[2]!=0xA0 ||
           (n==7 && (packet[1]!=0x13 || packet[3]!=0)) ||
           (n==6 && packet[1]!=0x12)) abort();
        ++attempts; ++tries;
        if(!strcmp(scenario,"write-error")) return FALSE;
        if(!strcmp(scenario,"write-short")) --*done;
    }
    if(n==2 && packet[0]==4) { ++reads; if(packet[1]==6)++acks; if(packet[1]==7)++nacks; }
    return TRUE;
}
static BOOL WINAPI fake_read(HANDLE h, LPVOID data, DWORD n, LPDWORD done, LPOVERLAPPED o) {
    (void)h;(void)o;unsigned char *p=data;memset(p,0,n);*done=n;
    if(size==23) { memcpy(p,"BBIO1I2C1\x01\x01",11);return TRUE; }
    if(getenv("I2C_TEST_WRITE") && (size==6 || size==7) && packet[0]==2) {
        p[0]=p[1]=p[n-1]=1;
        if(!strcmp(scenario,"read-error")) return FALSE;
        if(!strcmp(scenario,"read-short")) { *done=0;return TRUE; }
        unsigned busy=(!strcmp(scenario,"nack") ||
                       (!strcmp(scenario,"transient") && tries<=2) ||
                       (!strcmp(scenario,"last-ack") && tries<=99));
        if(busy) p[atoi(getenv("I2C_TEST_NACK_INDEX"))]=1;
        else { tries=0; ++completed; }
    } else if(size==2 && packet[0]==4) { p[0]=(unsigned char)(0x13+reads);p[1]=1; }
    return TRUE;
}
#undef CreateFile
#define CreateFile fake_open
#define GetCommState fake_get
#define SetCommState fake_set
#define SetCommTimeouts fake_timeout
#define CloseHandle fake_close
#define Sleep fake_sleep
#define WriteFile fake_write
#define ReadFile fake_read
'''


def build(source, name):
    wrapper = OUT / (name + ".c")
    wrapper.write_text(MOCK + '\n#include "' + source.as_posix() + '"\n', encoding="ascii")
    exe = OUT / (name + ".exe")
    subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                    str(wrapper), "-o", str(exe)], check=True)
    return exe


def run(exe, scenario, mode="W", contents="AA BB", index=2, length="2", timeout=3):
    path = OUT / "data.hex"
    path.write_text(contents, encoding="ascii")
    env = dict(os.environ, I2C_TEST_SCENARIO=scenario, I2C_TEST_NACK_INDEX=str(index))
    if mode.startswith("W"):
        env["I2C_TEST_WRITE"] = "1"
    else:
        env.pop("I2C_TEST_WRITE", None)
    args = [str(exe), "MOCK", "0", mode, str(path)]
    if mode.startswith("R"):
        args.append(length)
    return subprocess.run(args,
                          env=env, capture_output=True, text=True, timeout=timeout)


subprocess.run(["gcc", "-Wall", "-Wextra", "-Werror", str(TOOL),
                "-o", str(OUT / "I2CEEPROMWIN.exe")], check=True)
before = OUT / "before.c"
before.write_text(subprocess.check_output(
    ["git", "show", "2d82dc0:scripts/I2CEEPROMWIN.c"], cwd=ROOT, text=True), encoding="ascii")
old = build(before, "before-mock")
try:
    run(old, "nack", timeout=1)
    raise AssertionError("Old NACK loop unexpectedly terminated")
except subprocess.TimeoutExpired:
    print("BEFORE: persistent NACK still running after 1 s; harness terminated it", flush=True)

fixed = build(TOOL, "fixed-mock")
checks = 0
for mode in ("W", "Ws"):
    for index in range(2, 6 if mode == "W" else 5):
        r = run(fixed, "nack", mode, index=index)
        assert r.returncode == 7 and "after 100 attempts (no ACK)" in r.stdout
        assert "attempts=100 waits=99 closes=1 cleanup=1 timeouts=1" in r.stdout
        checks += 1
    for scenario, attempts, retries in (("ack", 2, 0), ("transient", 6, 4), ("last-ack", 200, 198)):
        r = run(fixed, scenario, mode)
        assert r.returncode == 0 and f"2 bytes written with {retries} retries" in r.stdout
        assert f"attempts={attempts} " in r.stdout
        checks += 1
    for scenario in ("write-error", "write-short", "read-error", "read-short"):
        r = run(fixed, scenario, mode)
        assert r.returncode == 7 and "Serial I/O failed" in r.stdout
        assert "attempts=1 waits=0 closes=1 cleanup=1 timeouts=1" in r.stdout
        checks += 1
    for contents in ("1FF", "xyz"):
        r = run(fixed, "ack", mode, contents=contents)
        assert r.returncode == 6 and "attempts=0 " in r.stdout
        checks += 1
    r = run(fixed, "timeout-config", mode)
    assert r.returncode == 7 and "Error setting serial write-mode timeouts" in r.stdout
    checks += 1
for mode in ("R", "Rs"):
    for length in ("0", "-1"):
        r = run(fixed, "ack", mode, length=length)
        assert r.returncode == 4 and "Must specify read length" in r.stdout
        checks += 1
    r = run(fixed, "ack", mode)
    assert r.returncode == 0 and "14 15 " in r.stdout
    assert "timeouts=0 reads=2 acks=1 nacks=1" in r.stdout
    assert (OUT / "data.hex").read_text() == "14 15 "
    checks += 1
r = run(fixed, "nack")
print("AFTER: exit", r.returncode, r.stdout.strip(), flush=True)
print(f"PASS: {checks} mocked Win32 main-path cases; real MinGW tool build clean", flush=True)
