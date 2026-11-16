"""Actual AUX/LCD consumers plus the core parser, with native hardware mocks."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

import test_binary_capabilities as capabilities
from test_core_system import function


ROOT = Path(__file__).resolve().parents[2]
PARSER = r'''
static struct { bool command_error; } mode_configuration;
#ifdef BUSPIRATEV3
#define BP_COMMAND_BUFFER_SIZE 128
#else
#define BP_COMMAND_BUFFER_SIZE 256
#endif
#define CMDLENMSK (BP_COMMAND_BUFFER_SIZE - 1)
static char cmdbuf[BP_COMMAND_BUFFER_SIZE];
static unsigned cmdstart, prompts;
static bool setup_defaults_mode;
static int getnumber(int def, int minimum, int maximum, int exit_allowed) {
  (void)minimum; ++prompts;
  if(exit_allowed) { assert(def == -1 && maximum == 180); return -1; }
  if(maximum == 4000) return 1000;
  if(maximum == 99) return 50;
  if(maximum == 180) return 90;
  assert(maximum == 2); return 2;
}
static void line(const char *text, unsigned start) {
  memset(cmdbuf, 0, sizeof cmdbuf);
  for(unsigned i = 0; text[i]; ++i) cmdbuf[(start + i) & CMDLENMSK] = text[i];
  cmdstart = start; prompts = 0; mode_configuration.command_error = false;
  setup_defaults_mode = false;
}
#define bpBR ((void)0)
'''

PWM_TEST = r'''
static void prepare(void) {
  available = true; state.mode = AUX_MODE_IO; AUXPIN_DIR = INPUT;
  TMR2 = 123; OC5CON2 = 31;
}
int main(void) {
  const char *valid[] = {"g1000 50", "g 1000   50", "g   0x3e8 0x32"};
  unsigned checks = 0;
  for(unsigned offset = 0; offset < sizeof cmdbuf; ++offset)
    for(unsigned i = 0; i < sizeof valid / sizeof *valid; ++i) {
      line(valid[i], offset); prepare(); bp_pwm_setup();
      assert(!prompts && !mode_configuration.command_error);
      assert(cmdstart == ((offset + strlen(valid[i])) & CMDLENMSK));
      assert(PR2 == 15 && OC5R == expected_pwm_compare(16, 50) && OC5RS == OC5R);
      assert(T2CONbits.TON && !TMR2 && AUXPIN_DIR == OUTPUT && state.mode == AUX_MODE_PWM);
      ++checks;
    }
  const char *invalid[] = {"g", "g xyz", "g 1000", "g 65536 50", "g 4001 50", "g 1000 100"};
  for(unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
    line(invalid[i], CMDLENMSK - 2); prepare(); bp_pwm_setup();
    assert(prompts == 2 && !mode_configuration.command_error);
    assert(PR2 == 15 && OC5R == expected_pwm_compare(16, 50)); ++checks;
  }
  line("g 4000 50", 0); prepare(); bp_pwm_setup();
  assert(!prompts && PR2 == 3 && OC5R == expected_pwm_compare(4, 50)); ++checks;
  line("g 1000 0", 0); prepare(); bp_pwm_setup(); assert(!prompts && OC5R == 0); ++checks;
  line("g   1000 50", CMDLENMSK); prepare(); setup_defaults_mode = true;
  char saved[sizeof cmdbuf]; memcpy(saved, cmdbuf, sizeof saved); bp_pwm_setup();
  assert(prompts == 2 && cmdstart == 0 && !memcmp(saved, cmdbuf, sizeof saved)); ++checks;
  line("g 1000 50", 0); prepare(); available = false; bp_pwm_setup();
  assert(cmdstart == 0 && !prompts && TMR2 == 123); ++checks;
  printf("PASS %u actual PWM parser/consumer cases, whitespace, wrap, fallback and defaults\n", checks);
}
'''

SERVO_TEST = r'''
int main(void) {
  (void)expected_pwm_compare;
  (void)OC5CON2;
  unsigned checks = 0;
  for(unsigned offset = 0; offset < sizeof cmdbuf; ++offset)
    for(unsigned position = 0; position <= 180; position += 90) {
      char text[32]; snprintf(text, sizeof text, "S   %u", position); line(text, offset);
      available = true; state.mode = AUX_MODE_IO; AUXPIN_DIR = INPUT; TMR2 = 123;
      bp_servo_setup();
      assert(!prompts && !mode_configuration.command_error);
      assert(cmdstart == ((offset + strlen(text)) & CMDLENMSK));
      assert(PR2 == 1250 && OC5R == position * 5 / 14 + 62 && OC5RS == OC5R);
      assert(AUXPIN_DIR == OUTPUT && !TMR2 && T2CONbits.TON && state.mode == AUX_MODE_PWM);
      ++checks;
    }
  const char *invalid[] = {"S", "S bad", "S 181", "S 65536", "S 0xffff"};
  for(unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
    line(invalid[i], CMDLENMSK - 1); available = true; state.mode = AUX_MODE_IO;
    bp_servo_setup(); assert(prompts == 2 && OC5R == 94 && PR2 == 1250); ++checks;
  }
  line("S   90", CMDLENMSK); available = true; state.mode = AUX_MODE_IO; setup_defaults_mode = true;
  char saved[sizeof cmdbuf]; memcpy(saved, cmdbuf, sizeof saved); bp_servo_setup();
  assert(!prompts && OC5R == 62 && cmdstart == 0 && !memcmp(saved, cmdbuf, sizeof saved)); ++checks;
  line("S 90", 0); available = false; TMR2 = 123; bp_servo_setup();
  assert(cmdstart == 0 && !prompts && TMR2 == 123); ++checks;
  printf("PASS %u actual servo parser/consumer cases, positions, wrap, fallback and defaults\n", checks);
}
'''

LCD = r'''
#define CMD_CLEARDISPLAY 1
#define CMD_SETDDRAMADDR 128
#define HD44780_COMMAND 0
#define HD44780_DATA 1
#define DISPLAYLINES1 0
#define DISPLAYLINES2 8
#define MSG_UNKNOWN_MACRO_ERROR ((void)0)
static unsigned resets, inits, lines, writes, last_reg, last_value, serial_bytes;
static void HD44780_Reset(void) { ++resets; }
static void HD44780_Init(unsigned char value) { ++inits; lines = value; }
static void HD44780_WriteByte(unsigned char reg, unsigned char value) { ++writes; last_reg = reg; last_value = value; }
static void bp_delay_ms(unsigned value) { (void)value; }
static void user_serial_transmit_character(unsigned char value) { (void)value; ++serial_bytes; }
static void reset_counts(void) { resets = inits = lines = writes = last_reg = last_value = serial_bytes = 0; }
'''

LCD_TEST = r'''
int main(void) {
  unsigned checks = 0;
  for(unsigned offset = 0; offset < sizeof cmdbuf; ++offset)
    for(unsigned count = 1; count <= 2; ++count) {
      char text[16]; snprintf(text, sizeof text, "m   %u", count); line(text, offset); reset_counts(); LCDmacro(2);
      assert(resets == 1 && inits == 1 && !prompts && lines == (count == 1 ? 0 : 8));
      assert(cmdstart == ((offset + strlen(text)) & CMDLENMSK) && !mode_configuration.command_error); ++checks;
    }
  const char *invalid[] = {"m", "m bad", "m 0", "m 3", "m 65536"};
  for(unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
    line(invalid[i], CMDLENMSK); reset_counts(); LCDmacro(2);
    assert(resets == 1 && inits == 1 && prompts == 1 && lines == 8 && !mode_configuration.command_error); ++checks;
  }
  line("m 0x27", CMDLENMSK - 2); reset_counts(); LCDmacro(4);
  assert(writes == 1 && last_reg == 0 && last_value == 0xa7); ++checks;
  line("m 2", 0); reset_counts(); LCDmacro(6); assert(writes == 3 && serial_bytes == 2); ++checks;
  line("m 3", 0); reset_counts(); LCDmacro(7); assert(writes == 4 && serial_bytes == 3); ++checks;
  line("m   1", CMDLENMSK); reset_counts(); setup_defaults_mode = true;
  char saved[sizeof cmdbuf]; memcpy(saved, cmdbuf, sizeof saved); LCDmacro(2);
  assert(cmdstart == 0 && prompts == 1 && lines == 8 && !memcmp(saved, cmdbuf, sizeof saved)); ++checks;
  printf("PASS %u actual LCD macro parser cases, inline lines, wrap, fallback, cursor and writes\n", checks);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    core = capabilities.source("core_system/proc_menu.c", args.revision)
    aux = capabilities.source("capability_aux/aux_pin.c", args.revision)
    lcd = capabilities.source("capability_hd44780/hd44780.c", args.revision)
    parsing = "\n".join(function(core, name) for name in ("getint", "consumewhitechars", "getint_with_spaces"))
    macros = "\n".join("#define " + name + " 0" for name in sorted(set(re.findall(r"\b_OC5CON(?:1)?_\w+_POSITION\b", aux))))
    messages = "\n".join("#define " + name + " ((void)0)" for name in sorted(set(re.findall(r"\bBPMSG\d+\b", aux + lcd))))
    code = {
        "pwm": capabilities.COMMON + capabilities.PWM + PARSER + parsing + "\n" + macros + "\n" + messages + "\n" +
               function(aux, "pwm_duty_register") + function(aux, "setup_prescaler_divisor") + function(aux, "bp_pwm_setup") + PWM_TEST,
        "servo": capabilities.COMMON + capabilities.PWM + PARSER + parsing + "\n" + macros + "\n" + messages + "\n" +
                 function(aux, "bp_servo_setup") + SERVO_TEST,
        "lcd": capabilities.COMMON + PARSER + parsing + LCD + messages + "\n" + function(lcd, "LCDmacro") + LCD_TEST,
    }
    for board in ("V4", "V3"):
        for name, text in code.items():
            if board == "V3":
                text = text.replace("#define BUSPIRATEV4", "#define BUSPIRATEV3")
            out = ROOT / "Firmware/build/test-capability-inline-parse" / board
            out.mkdir(parents=True, exist_ok=True)
            source = out / (name + ".c")
            exe = source.with_suffix(".exe")
            source.write_text(text, encoding="ascii")
            linker = ["-fuse-ld=bfd"] if shutil.which("ld.bfd") else []
            subprocess.run(["gcc", *linker, "-std=gnu99", "-Wall", "-Wextra", "-Werror",
                            str(source), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == "__main__":
    main()
