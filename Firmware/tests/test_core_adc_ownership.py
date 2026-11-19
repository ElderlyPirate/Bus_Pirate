"""Production core ADC rejection policy with register/command snapshots.

Tests software ordering and observable refusal, not PIC24 timing or regulation.
The SMPS start/stop/ISR bodies are used only as a deterministic owner fixture.
"""
import argparse
from pathlib import Path
import re
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
#define ON 1
#define OFF 0
#define YES 1
#define NO 0
#define BP_HIZ 0
#define BP_ADC_5V0 2
#define BP_ADC_3V3 0
#define BP_ADC_VPU 5
#define BP_ADC_PROBE 12
#define V33L 400
#define V5L 600
#define CMDLENMSK 255
#define OC5_IO 22
#define NULL_IO 0
#define INPUT 1
#define OUTPUT 0
#define LOW 0
#define AUX2 1
#define AUX1 256
#define AUX 32
#define CS 16
#define MISO 8
#define CLK 4
#define MOSI 2
static struct {
 struct { unsigned ADON,ASAM,SAMP,DONE,SSRC; } con1;
 struct { unsigned AD1IE; } ie;
 struct { unsigned AD1IF; } flags;
 uint16_t chs,buf,con2,con3,pwm,period,oc1,oc2,route;
  unsigned pullup,vpu33,vpu50,psu,aux_dir,aux_lat;
} hw;
#define AD1CON1bits hw.con1
#define IEC0bits hw.ie
#define IFS0bits hw.flags
#define AD1CHS hw.chs
#define ADC1BUF0 hw.buf
#define OC5R hw.pwm
#define OC5RS hw.period
#define OC5CON1 hw.oc1
#define OC5CON2 hw.oc2
#define BP_AUX1_RPOUT hw.route
#define BP_AUX1_DIR hw.aux_dir
#define BP_AUX1_LAT hw.aux_lat
static struct { unsigned command_error,high_impedance; } mode_configuration;
static struct { unsigned bus_mode; } bus_pirate_configuration;
static struct { uint16_t voltage_out,voltage_reading;uint8_t pwm_duty_cycle; } smps_state;
static unsigned effects,notes,reads,input_calls,ready_calls,cmdstart,vpu_measurement;
static void bp_aux_pin_reset_state(void) { ++effects; }
static char output[1024];
static void note(void) { ++notes; }
static void bp_write_line(const char *s) { strcat(output,s);strcat(output,"\r\n"); }
static void bp_write_string(const char *s) { (void)s;note(); }
static void bp_write_voltage(uint16_t v) { (void)v;note(); }
static void bp_write_dec_word(uint16_t v) { (void)v;note(); }
static void user_serial_transmit_character(char c) { (void)c;note(); }
static bool user_serial_ready_to_read(void) { ++input_calls;return ++ready_calls>1; }
static char user_serial_read_byte(void) { ++input_calls;return 'x'; }
static void bp_enable_adc(void) { ++effects;hw.con1.ADON=1; }
static void bp_disable_adc(void) { ++effects;hw.con1.ADON=0; }
static uint16_t bp_read_adc(uint16_t channel) {
 ++effects;++reads;hw.chs=channel;
 return channel==BP_ADC_5V0 ? 768 : channel==BP_ADC_VPU ? vpu_measurement : 512;
}
static void bp_disable_3v3_pullup(void) { ++effects;hw.vpu33=hw.vpu50=0; }
static void bp_enable_3v3_pullup(void) { ++effects;hw.vpu33=1;hw.vpu50=0; }
static void bp_enable_5v0_pullup(void) { ++effects;hw.vpu50=1;hw.vpu33=0; }
static void bp_enable_pullup(void) { ++effects;hw.pullup=1; }
static void bp_enable_voltage_regulator(void) { ++effects;hw.psu=1; }
static void bp_disable_voltage_regulator(void) { ++effects;hw.psu=0; }
static void bp_delay_ms(unsigned ms) { (void)ms;++effects; }
static void consumewhitechars(void) { ++input_calls; }
static int getint(void) { ++input_calls;return 2; }
static int getnumber(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d;++input_calls;return 2; }
static int getrepeat(void) { ++input_calls;return 1; }
static void print_pin_direction(uint16_t pin) { (void)pin;note(); }
static void print_pin_state(uint16_t pin) { (void)pin;note(); }
static const struct { void (*print_pins_state)(void);void (*setup_execute)(void); } protocol={note,note};
#define bp_current_protocol() (&protocol)
#define bpBR note()
'''

TEST = r'''
int main(void) {
 void (*operations[])(void)={print_pins_information,bp_adc_probe,bp_adc_continuous_probe,
                           switch_psu_on,command_P,command_d,
#ifdef BUSPIRATEV4
                           set_pullup_voltage,
#endif
 };
 unsigned checks=0;
 for(unsigned ie=0;ie<2;++ie)
 for(unsigned adon=0;adon<2;++adon)
 for(unsigned asam=0;asam<2;++asam)
 for(unsigned i=0;i<sizeof(operations)/sizeof(operations[0]);++i) {
  memset(&hw,0xa5,sizeof(hw));
  hw.ie.AD1IE=ie;hw.con1.ADON=adon;hw.con1.ASAM=asam;
  /* A fault can leave zero duty while feedback still owns the ADC. */
  hw.pwm=(i&1) ? 0 : 45;
  unsigned char before[sizeof(hw)];memcpy(before,&hw,sizeof(hw));
  mode_configuration.command_error=0;mode_configuration.high_impedance=YES;
  bus_pirate_configuration.bus_mode=1;
  effects=notes=reads=input_calls=ready_calls=0;cmdstart=7;output[0]=0;
  vpu_measurement=asam ? 512 : 0;
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
  bool busy=ie;
#else
  bool busy=false;
#endif
  assert(bp_adc_foreground_available()==!busy);
  operations[i]();
  if(busy) {
   assert(mode_configuration.command_error);
   assert(!strcmp(output,"ADC busy: SMPS feedback active; command rejected.\r\n"));
   assert(!effects && !notes && !reads && !input_calls && cmdstart==7);
   assert(!memcmp(before,&hw,sizeof(hw)));
  } else {
   assert(reads && effects && notes && !output[0]);
#ifdef BUSPIRATEV4
   if(operations[i]==set_pullup_voltage && !vpu_measurement) {
    assert(hw.vpu33==1 && !hw.vpu50 && input_calls && cmdstart==8);
   }
#endif
  }
  assert(bus_pirate_configuration.bus_mode==1);
  ++checks;
 }
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
 memset(&hw,0,sizeof(hw));smps_start(768);
 assert(!bp_adc_foreground_available() && hw.con1.ADON && hw.con1.ASAM);
 unsigned duty=hw.pwm;
 hw.buf=smps_state.voltage_out+1;fixture_adc_interrupt();
 assert(!hw.pwm && !bp_adc_foreground_available());
 hw.buf=smps_state.voltage_out-1;fixture_adc_interrupt();
 assert(hw.pwm==duty && !bp_adc_foreground_available());
 smps_stop();assert(bp_adc_foreground_available() && !hw.con1.ADON && !hw.con1.ASAM);
 effects=notes=reads=0;output[0]=0;mode_configuration.command_error=0;
 bp_adc_probe();assert(reads==1 && !output[0] && !mode_configuration.command_error);
 puts("PASS actual SMPS start/feedback zero-duty/recovery/stop ownership and resumed foreground probe");
#endif
 printf("PASS %u core command/owner-history cases: refusal before hardware/input effects, normal idle paths\n",checks);
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    args = p.parse_args()

    def source(path):
        candidates = [path]
        if path == "capability_adc/adc.c":
            candidates.append("core_system/base.c")
        if path == "smps.c":
            candidates.insert(0, "capability_smps/smps.c")
        if args.revision:
            for candidate in candidates:
                result = subprocess.run(["git", "show", args.revision + ":Firmware/" + candidate],
                                        cwd=ROOT, text=True, encoding="utf-8", capture_output=True)
                if result.returncode == 0:
                    return result.stdout
        else:
            for candidate in candidates:
                file = ROOT / "Firmware" / candidate
                if file.exists():
                    return file.read_text(encoding="utf-8")
        raise RuntimeError("Missing source: " + path)

    base = source("core_system/base.c")
    menu = source("core_system/proc_menu.c")
    code = "\n".join(function(base, name) for name in (
        "bp_adc_foreground_available", "bp_adc_check_foreground"))
    adc = source("capability_adc/adc.c")
    code += "\n" + "\n".join(function(adc, name) for name in ("bp_adc_probe", "bp_adc_continuous_probe"))
    code += "\n" + function(menu, "print_pins_information")
    code += "\n#ifdef BUSPIRATEV4\n" + function(menu, "set_pullup_voltage") + "\n#endif\n"
    code += function(menu, "switch_psu_on")
    for command, end in (("P", "#ifdef BUSPIRATEV4\n      case 'e':"), ("d", "      case 'D':")):
        block = menu.split("      case '" + command + "':", 1)[1].split(end, 1)[0]
        code += "\nvoid command_" + command + "(void) { uint16_t repeat=0;(void)repeat;switch('" + command + "') {case '" + command + "':" + block + "}}\n"
    assert "print_pins_information();" in function(menu, "print_status_info")
    assert re.search(r"case 'D':\s*bp_adc_continuous_probe\(\);", menu)
    smps = source("smps.c")
    smps = smps.replace("void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt()", "void fixture_adc_interrupt(void)")
    code += "\n#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)\n" + "\n".join(
        function(smps, name) for name in ("smps_start", "smps_stop", "fixture_adc_interrupt")) + "\n#endif\n"
    messages = "\n".join("#define " + name + " note()" for name in sorted(set(re.findall(r"\b(?:BPMSG\d+|MSG_[A-Z0-9_]+)\b", code))))
    for label, defines in (("v3", "#define BUSPIRATEV3\n"),
                           ("v4", "#define BUSPIRATEV4\n"),
                           ("v3-smps-flag", "#define BUSPIRATEV3\n#define BP_ENABLE_SMPS_SUPPORT\n"),
                           ("v4-smps", "#define BUSPIRATEV4\n#define BP_ENABLE_SMPS_SUPPORT\n")):
        with tempfile.TemporaryDirectory(prefix="bp-core-adc-owner-", dir=TEMP) as tmp:
            c = Path(tmp) / "owner.c"
            exe = c.with_suffix(".exe")
            c.write_text(defines + PREFIX + messages + "\n" + code + TEST, encoding="ascii")
            subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-variable",
                            str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
        print("PASS", label, "foreground ADC ownership policy", flush=True)


if __name__ == "__main__":
    main()
