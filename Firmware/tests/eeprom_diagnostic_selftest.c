/* Test-image replacement for the board selftest, never a production source.
 * Exercises the real EEPROM API without driving the attached external rig. */
#include "base.h"
#include "onboard_eeprom.h"
#include "selftest.h"

uint8_t perform_selftest(bool show_progress, bool jumper_test) {
  bool saved_wp = LATDbits.LATD11;
  bool entry_wp = !jumper_test;
  uint8_t errors = 0;
  (void)show_progress;
  LATDbits.LATD11 = entry_wp;
  if (!eeprom_test()) {
    errors |= 1;
  }
  if (LATDbits.LATD11 != entry_wp) {
    errors |= 2;
  }
  if (I2C1CON & _I2C1CON_I2CEN_MASK) {
    errors |= 4;
  }
  if (BP_EE_SDA_DIR != INPUT || BP_EE_SCL_DIR != INPUT) {
    errors |= 8;
  }
  LATDbits.LATD11 = saved_wp;
  return errors;
}
