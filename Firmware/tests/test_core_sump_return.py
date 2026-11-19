"""Core SUMP/BBIO return output/state checks; no transport or capture simulator."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NO 0
#define HEX 0
#define BP_HIZ 0
#define ASCII_NUL 0
#define CMDLENMSK 255
#define BELL 7
static unsigned cmdstart,cmdend,user_macro;
static char cmdbuf[256],output[256];
static unsigned cleaned,reset_board,sump_calls,bbio_calls,banners,extra_input_reads;
static struct { unsigned cursor_position,history_entry_counter;
 uint8_t binary_mode_counter,escape_state;bool command_present; } menu_state;
static struct { unsigned bus_mode,quiet,display_mode,basic; } bus_pirate_configuration;
static void bp_write_string(const char *s) { assert(strlen(output)+strlen(s)<sizeof(output));strcat(output,s); }
static void user_serial_transmit_character(char c) { char text[]={c,0};bp_write_string(text); }
static void print_version_info(void) { ++banners;bp_write_string("Bus Pirate test banner\r\n"); }
static void cleanup(void) { ++cleaned; }
static const struct { void (*cleanup)(void); } protocol={cleanup};
#define bp_current_protocol() (&protocol)
static void bp_reset_board_state(void) { ++reset_board;bus_pirate_configuration.bus_mode=BP_HIZ; }
static void bp_disable_usb_led(void) {}
static void enter_sump_mode(void) {
 ++sump_calls;bp_write_string("1ALS");
 /* Family-side terminal exit already supplies the banner; no extra CR. */
 bus_pirate_configuration.bus_mode=BP_HIZ;print_version_info();
}
static void enter_binary_bitbang_mode(void) { ++bbio_calls;bp_write_string("BBIO1"); }
#define MSG_CURSOR_LEFT bp_write_string("\x1b[D")
'''

TEST = r'''
static void prepare(unsigned end) {
 memset(cmdbuf,'x',sizeof(cmdbuf));memset(&menu_state,0,sizeof(menu_state));
 output[0]=0;cleaned=reset_board=sump_calls=bbio_calls=banners=extra_input_reads=0;
 cmdend=end;cmdstart=(end+11)&CMDLENMSK;
 menu_state.cursor_position=end;menu_state.history_entry_counter=17;
 menu_state.escape_state=2;user_macro=1;
 bus_pirate_configuration.bus_mode=3;bus_pirate_configuration.quiet=1;
 bus_pirate_configuration.display_mode=3;bus_pirate_configuration.basic=1;
}
static void assert_reset(unsigned end) {
 assert(cleaned==1 && reset_board==1 && bus_pirate_configuration.bus_mode==BP_HIZ);
 assert(cmdstart==end && cmdend==end && menu_state.cursor_position==end && !cmdbuf[end]);
 for(unsigned i=0;i<256;++i) if(i!=end) assert(cmdbuf[i]=='x');
 assert(!user_macro && !menu_state.command_present && !menu_state.binary_mode_counter && !menu_state.escape_state);
 assert(!menu_state.history_entry_counter && !bus_pirate_configuration.quiet && bus_pirate_configuration.display_mode==HEX);
#ifdef BP_ENABLE_BASIC_SUPPORT
 assert(!bus_pirate_configuration.basic);
#endif
 assert(!extra_input_reads);
}
int main(void) {
 for(unsigned end=0;end<256;++end) {
  prepare(end);menu_state.binary_mode_counter=5;
  handle_left_arrow();
#ifdef BP_ENABLE_SUMP_SUPPORT
  assert(sump_calls==1 && banners==1 && !bbio_calls);
  assert(!strcmp(output,"1ALSBus Pirate test banner\r\nHiZ>"));
  assert_reset(end);
#else
  assert(!sump_calls && !banners && !cleaned && !reset_board);
  assert(!strcmp(output,"\x1b[D") && menu_state.cursor_position==((end-1)&CMDLENMSK));
  assert(!menu_state.binary_mode_counter);
#endif
  prepare(end);menu_state.binary_mode_counter=19;
  dispatch_nul();
  assert(bbio_calls==1 && !sump_calls);
#ifdef BUSPIRATEV4
  assert(banners==1 && !strcmp(output,"BBIO1Bus Pirate test banner\r\nHiZ>"));
  assert_reset(end);
#else
  /* V3's existing BBIO branch has no V4 terminal-return epilogue. */
  assert(!banners && !cleaned && !reset_board && !strcmp(output,"BBIO1"));
#endif
 }
 prepare(0);menu_state.binary_mode_counter=4;
 handle_left_arrow();assert(!sump_calls && !strcmp(output,"\x1b[D"));
 prepare(0);menu_state.cursor_position=cmdstart;
 handle_left_arrow();assert(!sump_calls && output[0]==BELL && !output[1]);
 puts("PASS 256 ring positions: SUMP banner+one immediate HiZ prompt, reset state, ordinary left arrow, existing BBIO epilogue");
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    if args.revision:
        menu = subprocess.check_output(["git", "show", args.revision + ":Firmware/core_system/proc_menu.c"],
                                       cwd=ROOT, text=True, encoding="utf-8")
    else:
        menu = (ROOT / "Firmware/core_system/proc_menu.c").read_text(encoding="utf-8")
    body = function(menu, "reset_terminal_state") + "\n" + function(menu, "handle_left_arrow")
    # Compile the actual serviceuser() BBIO switch arm, not a copied epilogue.
    arm = function(menu, "serviceuser").split("      case ASCII_NUL:", 1)[1].split("      default:", 1)[0]
    body += "\nstatic void dispatch_nul(void) { switch(ASCII_NUL) { case ASCII_NUL:" + arm + "}}\n"
    for board in ("V3", "V4"):
        for enabled in (False, True):
            defines = "#define BUSPIRATE" + board + "\n#define BP_ENABLE_BASIC_SUPPORT\n"
            if enabled:
                defines += "#define BP_ENABLE_SUMP_SUPPORT\n"
            with tempfile.TemporaryDirectory(prefix="bp-core-sump-return-", dir=TEMP) as tmp:
                c = Path(tmp) / "return.c"
                exe = c.with_suffix(".exe")
                c.write_text(defines + PREFIX + body + TEST, encoding="ascii")
                subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                                str(c), "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)
            print("PASS", board, "SUMP", "enabled" if enabled else "disabled", flush=True)


if __name__ == "__main__":
    main()
