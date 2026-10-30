#include "base.h"
#include "avrisp_mkii.c"
#include "avrisp_v2.c"
#include "avrisp_isp.c"
#include "avrisp_xprog.c"
#include "avrisp_nvm_pdi.c"
#include "avrisp_nvm_tpi.c"

const unsigned char cdc_device_descriptor[] = {0};
const unsigned char cdc_config_descriptor[] = {0};
const unsigned char cdc_str_descs[] = {0};
volatile uint8_t usb_device_state = CONFIGURED_STATE;
static unsigned checks, us_elapsed;
static uint8_t usb_request[1024], usb_reply[1024], wire[8192], spi[8192];
static unsigned out_len, out_pos, out_zlp, in_len, packet_count, packet_size[20];
static unsigned wire_len, spi_len, rx_bit, tx_bit, tx_value;
static uint8_t phy_rx[1024];
static unsigned phy_len, phy_pos;
static bool bad_parity, bad_stop;
static bool blocked_cdc;
static bool mock_require_idle_clock;
static bool mock_zero_packets;
static unsigned mock_run_polls, mock_restart_at;
static uint8_t serial_rx[2048], serial_tx[2048];
static unsigned serial_len, serial_pos, serial_tx_len;
static uint8_t isp_rx[128];
static unsigned isp_rx_len, isp_rx_pos, delay_ms_total;
static bool check_isp_reset;
static unsigned expected_isp_reset;

#define CHECK(x) do { assert(x); checks++; } while (0)

static void bp_delay_us(unsigned us) {
  us_elapsed += us;
  while (us_elapsed >= 10000) {
    us_elapsed -= 10000;
    _T3Interrupt();
  }
  if (T2CONbits.TON) IFS0bits.T2IF = 1;
}
static void bp_delay_ms(unsigned ms) {
  delay_ms_total += ms;
  bp_delay_us(ms * 1000);
}
static bool user_serial_ready_to_read(void) { return serial_pos < serial_len; }
static uint8_t user_serial_read_byte(void) { return serial_rx[serial_pos++]; }
static void user_serial_transmit_character(uint8_t b) {
  assert(serial_tx_len < sizeof(serial_tx));
  serial_tx[serial_tx_len++] = b;
}
static unsigned getInReady(void) {
  if (blocked_cdc) bp_delay_ms(1);
  return !blocked_cdc;
}
static void mock_response_append(void) {
  if (mock_require_idle_clock) CHECK(IEC1bits.T4IE);
}
static void mock_usb_configure(void) {
  if (usb_device_state >= CONFIGURED_STATE) return;
  assert(mock_enumerating);
  assert(mock_sof_registrations == 0);
  if (mock_attach_attempts <= mock_fail_configurations) {
    bp_delay_ms(1000);
    usb_device_state = 0;
    return;
  }
  if (++mock_configuration_polls == 3) {
    usb_device_state = CONFIGURED_STATE;
    mock_enumerating = false;
  } else {
    usb_device_state = 0; /* A bus reset clears BDs before configuration. */
  }
}
static void mock_usb_run_poll(void) {
  if (!avrisp_activity_ticks) return;
  CHECK(++mock_run_polls <= AVRISP_IDLE_TICKS + 1);
  _T3Interrupt();
  if (mock_run_polls == mock_restart_at) {
    usb_device_state = DEFAULT_STATE;
    avrisp_ep_out->BDSTAT = 0;
  } else if (mock_zero_packets) {
    avrisp_ep_out->BDCNT = 0;
    avrisp_ep_out->BDSTAT &= ~UOWN;
  }
}
static void mock_usb_in(void) {
  if (avrisp_ep_in->BDSTAT & UOWN) {
    unsigned n = avrisp_ep_in->BDCNT;
    packet_size[packet_count++] = n;
    memcpy(usb_reply + in_len, avrisp_ep_in->BDADDR, n);
    in_len += n;
    avrisp_ep_in->BDSTAT &= ~UOWN;
  }
}
static void mock_usb_out(void) {
  if (!(avrisp_ep_out->BDSTAT & UOWN)) return;
  unsigned n = out_len - out_pos;
  if (n > 64) n = 64;
  assert(n || out_zlp);
  if (!n) out_zlp = 0;
  memcpy(avrisp_ep_out->BDADDR, usb_request + out_pos, n);
  out_pos += n;
  avrisp_ep_out->BDCNT = n;
  avrisp_ep_out->BDSTAT &= ~UOWN;
}
static void mock_spi(void) {
  if (check_isp_reset) {
    CHECK(BP_AUX0_DIR == OUTPUT && AVRISP_AUX_LAT == expected_isp_reset);
  }
  spi[spi_len++] = SPI1BUF;
  SPI1BUF = isp_rx_pos < isp_rx_len ? isp_rx[isp_rx_pos++] : spi_len & 0xff;
  IFS0bits.SPI1IF = 1;
}
static void mock_bit(bool drive, bool bit, bool sample) {
  if (drive) {
    if (!tx_bit && !bit) { tx_bit = 1; tx_value = 0; }
    else if (tx_bit) {
      if (tx_bit <= 8 && bit) tx_value |= 1u << (tx_bit - 1);
      if (tx_bit == 9) assert(bit == avrisp_xprog_parity(tx_value));
      if (tx_bit >= 10) assert(bit);
      if (++tx_bit == 12) {
        assert(wire_len < sizeof(wire));
        wire[wire_len++] = tx_value;
        tx_bit = 0;
      }
    }
  }
  if (sample) {
    uint8_t value = phy_pos < phy_len ? phy_rx[phy_pos] : 2;
    if (!rx_bit) BP_MISO = 0;
    else if (rx_bit <= 8) BP_MISO = (value >> (rx_bit - 1)) & 1;
    else if (rx_bit == 9) BP_MISO = avrisp_xprog_parity(value) ^ bad_parity;
    else BP_MISO = !bad_stop;
    if (++rx_bit == 12) { rx_bit = 0; phy_pos++; }
  }
}
static void reset_test(void) {
  in_len = packet_count = out_pos = wire_len = spi_len = 0;
  phy_len = phy_pos = rx_bit = tx_bit = tx_value = 0;
  bad_parity = bad_stop = false;
  isp_rx_len = isp_rx_pos = 0;
  check_isp_reset = false;
  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;
  avrisp_activity_ticks = AVRISP_IDLE_TICKS;
  avrisp_transport = AVRISP_TRANSPORT_USB;
  usb_device_state = CONFIGURED_STATE;
  avrisp_usb_configure_endpoints();
  avrisp_v2_initialise();
  avrisp_xprog_initialise();
  avrisp_isp_hardware_spi = true;
}
/* Let GCC reject fixture lengths that overread their actual byte arrays. */
static void request(const uint8_t *p, unsigned n)
    __attribute__((access(read_only, 1, 2)));
static void request(const uint8_t *p, unsigned n) {
  out_len = n; out_pos = in_len = packet_count = 0;
  out_zlp = n && !(n % 64);
  memcpy(usb_request, p, n);
  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;
  CHECK(avrisp_usb_read_request());
  CHECK(out_pos == n && !out_zlp);
  avrisp_v2_process_command();
}
static void reply(const uint8_t *p, unsigned n) {
  CHECK(in_len == n);
  CHECK(!memcmp(usb_reply, p, n));
}
static void frame(const uint8_t *p, unsigned n, uint8_t seq, bool bad) {
  uint8_t c = 0x1b ^ seq ^ (n >> 8) ^ n ^ 0x0e;
  serial_rx[serial_len++] = 0x1b;
  serial_rx[serial_len++] = seq;
  serial_rx[serial_len++] = n >> 8;
  serial_rx[serial_len++] = n;
  serial_rx[serial_len++] = 0x0e;
  for (unsigned i = 0; i < n; i++) { serial_rx[serial_len++] = p[i]; c ^= p[i]; }
  serial_rx[serial_len++] = c ^ bad;
}
static bool contains(const uint8_t *p, unsigned n) {
  for (unsigned i = 0; i + n <= wire_len; i++)
    if (!memcmp(wire + i, p, n)) return true;
  return false;
}
static void test_isp_first_contact(void) {
  /* Distinct delays and poll fields catch wire-order mistakes in the parser. */
  uint8_t enter[] = {CMD_ENTER_PROGMODE_ISP,200,31,17,3,2,0x53,3,0xac,0x53,0,0};
  reset_test();
  request((uint8_t[]){CMD_GET_PARAMETER,PARAM_RESET_POLARITY},2);
  reply((uint8_t[]){CMD_GET_PARAMETER,0,1},3);
  request((uint8_t[]){CMD_SET_PARAMETER,PARAM_SCK_DURATION,6},3);
  reply((uint8_t[]){CMD_SET_PARAMETER,0},2);
  CHECK(avrisp_param_get(PARAM_RESET_POLARITY) == 1);
  memcpy(isp_rx,(uint8_t[]){0xff,0xaa,0,0xcc,0x11,0x22,0x53,0x44},8);
  isp_rx_len = 8;
  check_isp_reset = true;
  expected_isp_reset = LOW;
  unsigned delay_start = delay_ms_total;
  request(enter,sizeof(enter));
  reply((uint8_t[]){CMD_ENTER_PROGMODE_ISP,0},2);
  CHECK(spi_len == 8 && isp_rx_pos == 8);
  CHECK(!memcmp(spi,enter+8,4) && !memcmp(spi+4,enter+8,4));
  CHECK(delay_ms_total-delay_start == 17+31+8*2+2*31);
  request((uint8_t[]){CMD_LEAVE_PROGMODE_ISP,1,1},3);
  reply((uint8_t[]){CMD_LEAVE_PROGMODE_ISP,0},2);
  CHECK(BP_AUX0_DIR == INPUT && AVRISP_AUX_LAT == LOW);

  reset_test();
  memset(isp_rx,0xff,sizeof(isp_rx)); isp_rx_len = sizeof(isp_rx);
  request(enter,sizeof(enter));
  reply((uint8_t[]){CMD_ENTER_PROGMODE_ISP,STATUS_CMD_FAILED},2);
  CHECK(spi_len == 12);
  /* Failed-enable diagnostics query a separate parameter, not a reply byte. */
  request((uint8_t[]){CMD_GET_PARAMETER,PARAM_STATUS_TGT_CONN},2);
  reply((uint8_t[]){CMD_GET_PARAMETER,0,STATUS_ISP_READY},3);

  reset_test();
  enter[7] = 0; /* Host may explicitly disable echo polling. */
  request(enter,sizeof(enter));
  reply((uint8_t[]){CMD_ENTER_PROGMODE_ISP,0},2);
  CHECK(spi_len == 4);

  reset_test();
  request((uint8_t[]){CMD_SET_PARAMETER,PARAM_RESET_POLARITY,0},3);
  reply((uint8_t[]){CMD_SET_PARAMETER,0},2);
  check_isp_reset = true; expected_isp_reset = HIGH;
  request(enter,sizeof(enter));
  reply((uint8_t[]){CMD_ENTER_PROGMODE_ISP,0},2);
  CHECK(spi_len == 4 && AVRISP_AUX_LAT == HIGH);
  avrisp_v2_initialise();
  CHECK(avrisp_param_get(PARAM_RESET_POLARITY) == 1);

  reset_test();
  BP_AUX0_DIR = INPUT;
  request(enter,sizeof(enter)-1);
  reply((uint8_t[]){CMD_ENTER_PROGMODE_ISP,STATUS_CMD_FAILED},2);
  CHECK(spi_len == 0 && BP_AUX0_DIR == INPUT);
  reset_test();
}

static void test_shared_nvm_transactions(void) {
  static const uint8_t key[] = {0xff,0x88,0xd8,0xcd,0x45,0xab,0x89,0x12};
  for (unsigned protocol=0; protocol<2; protocol++) {
    reset_test();
    avrisp_xprog_selected_protocol = protocol ? XPROG_PROTOCOL_TPI : XPROG_PROTOCOL_PDI;
    uint8_t instruction = protocol ? TPI_CMD_SKEY : PDI_CMD_KEY;
    avrisp_xprog_send_nvm_key(instruction);
    CHECK(wire_len == 9 && wire[0] == instruction);
    CHECK(!memcmp(wire+1,key,sizeof(key)));

    reset_test();
    avrisp_xprog_selected_protocol = protocol ? XPROG_PROTOCOL_TPI : XPROG_PROTOCOL_PDI;
    memcpy(phy_rx,(uint8_t[]){0,1,2},3); phy_len=3;
    CHECK(avrisp_xprog_wait_status(0x80,2,2));
    CHECK(phy_pos == 3 && wire_len == 3);
    CHECK(wire[0] == 0x80 && wire[1] == 0x80 && wire[2] == 0x80);

    reset_test();
    memcpy(phy_rx,(uint8_t[]){0x80,0},2); phy_len=2;
    CHECK(avrisp_xprog_wait_status(0x82,0x80,0));
    CHECK(phy_pos == 2 && wire_len == 2);
    avrisp_timeout_ticks=0;
    CHECK(!avrisp_xprog_wait_status(0x80,2,2));
  }
  reset_test();
}
int main(void) {
  uint8_t p[400] = {CMD_SIGN_ON};
  (void)mock_usb_configure;
  (void)CDCFlushOnTimeout;
  (void)usb_register_sof_handler;
  test_isp_first_contact();
  test_shared_nvm_transactions();
  reset_test();
  mock_enumerating=true;
  mock_attach_attempts=mock_fail_configurations=0;
  mock_configuration_polls=mock_sof_registrations=0;
  avrisp_usb_switch_to_cdc();
  CHECK(mock_configuration_polls == 3 && !mock_enumerating);
  CHECK(usb_device_state == CONFIGURED_STATE && !mock_sof_registrations);
  mock_enumerating=true;
  mock_attach_attempts=0; mock_fail_configurations=1;
  mock_configuration_polls=0;
  avrisp_usb_switch_to_cdc();
  CHECK(mock_attach_attempts == 2 && !mock_enumerating);
  CHECK(usb_device_state == CONFIGURED_STATE);
  mock_stuck_se0=true;
  CHECK(!avrisp_usb_attach() && !avrisp_timeout_ticks);
  mock_stuck_se0=false;
  CHECK(avrisp_usb_attach());
  for (unsigned mode=0; mode<3; mode++) {
    reset_test();
    mock_run_polls=0;
    mock_zero_packets=mode == 1;
    mock_restart_at=mode == 2 ? 10 : 0;
    avrisp_usb_run();
  CHECK(mock_run_polls == (mode == 2 ? 10u :
          AVRISP_IDLE_TICKS - AVRISP_USB_DETACH_MS / 10));
    CHECK(!avrisp_usb_personality_active && usb_device_state == CONFIGURED_STATE);
  }
  reset_test();
  static const unsigned primary[] = {64,16,4,1};
  static const unsigned frequencies[] = {8000000,4000000,2000000,1000000,500000,250000,125000};
  for (unsigned i=0; i<7; i++) {
    avrisp_param_set(PARAM_SCK_DURATION,i);
    avrisp_isp_enable_target();
    unsigned divider=primary[SPI1CON1 & 3] * (8-((SPI1CON1 >> 2) & 7));
    CHECK(16000000/divider == frequencies[i]);
    CHECK((SPI1CON1 & ((1u<<6)|(1u<<8)|(1u<<9))) == (1u<<8));
  }
  avrisp_isp_rescue_clock_enable();
  CHECK(PR2 == 3 && OC5RS == 3 && OC5R == 2 && OC5CON2 == 31);
  CHECK(BP_CS_RPOUT == 22);
  CHECK(sizeof(avrisp_soft_sck_half_us)/sizeof(*avrisp_soft_sck_half_us) == 157);
  CHECK(avrisp_soft_sck_half_us[0] == 5 && avrisp_soft_sck_half_us[10] == 9);
  CHECK(avrisp_soft_sck_half_us[57] == 85 && avrisp_soft_sck_half_us[93] == 477);
  CHECK(avrisp_soft_sck_half_us[156] == 9785);
  reset_test();
  request(p, 1);
  reply((uint8_t[]){1,0,10,'A','V','R','I','S','P','_','M','K','2'},13);
  request((uint8_t[]){3,PARAM_VTARGET},2);
  reply((uint8_t[]){3,0,33},3);
  request((uint8_t[]){2,PARAM_VTARGET,50},3);
  reply((uint8_t[]){2,STATUS_CMD_FAILED},2);
  request((uint8_t[]){6,0x80,1,2,3},5);
  CHECK(avrisp_load_extended_address && avrisp_current_address == 0x80010203UL);
  request((uint8_t[]){6,0,0,0,0},5);
  CHECK(avrisp_load_extended_address); /* LUFA preserves a pending boundary reload. */
  for (unsigned n = 1; n < 5; n++) {
    request((uint8_t[]){6,1,2,3,4}, n);
    reply((uint8_t[]){6,STATUS_CMD_FAILED},2);
    CHECK(avrisp_current_address == 0);
  }
  request((uint8_t[]){0x10,0,0,0,1,0,0,5,0xac,0x53,0,0},12);
  reply((uint8_t[]){0x10,STATUS_CMD_FAILED},2);

  /* Response packet ownership and exact-multiple termination. */
  for (unsigned count = 61; count <= 256; count += count == 61 ? 64 : count == 125 ? 1 : 130) {
    request((uint8_t[]){CMD_READ_EEPROM_ISP,count >> 8,count,0xa0},4);
    CHECK(in_len == count + 3);
    CHECK(packet_size[packet_count-1] == (count + 3) % 64);
    for (unsigned i = 0; i < count; i++) CHECK(usb_reply[2+i] == ((i+1)*4 + spi_len-count*4) % 256);
  }

  /* 54 data + 10 header = 64 on the wire; consume exactly one ZLP. */
  memset(p, 0xff, sizeof(p));
  p[0]=CMD_PROGRAM_FLASH_ISP; p[1]=0; p[2]=54; p[3]=1;
  p[4]=0; p[5]=0x40; p[6]=0x4c; p[7]=0x20; p[8]=p[9]=0xff;
  request(p,64);
  reply((uint8_t[]){CMD_PROGRAM_FLASH_ISP,0},2);
  CHECK(avrisp_current_address == 61+125+126+256+27);
  request((uint8_t[]){1},1);
  CHECK(in_len == 13);

  /* A fallback for one word must not disable polling for later words. */
  avrisp_current_address = 0;
  spi_len = 0;
  request((uint8_t[]){CMD_PROGRAM_EEPROM_ISP,0,2,4,0,0xc0,0,0xa0,0xff,0xff,0xff,0x22},12);
  CHECK(spi_len == 12 && spi[8] == 0xa0);
  spi_len = 0;
  request((uint8_t[]){CMD_SPI_MULTI,5,1,0,1,2,3,4,5},9);
  CHECK(spi_len == 5 && spi[4] == 5);
  p[0]=CMD_SPI_MULTI; p[1]=255; p[2]=0; p[3]=0;
  for (unsigned i=0; i<255; i++) p[4+i]=i;
  spi_len=0;
  request(p,259);
  CHECK(spi_len == 255 && spi[254] == 254);
  p[0]=CMD_PROGRAM_FLASH_ISP;
  request(p,300); /* drain all packets of an oversized request */
  reply((uint8_t[]){CMD_PROGRAM_FLASH_ISP,STATUS_CMD_FAILED},2);

  reset_test();
  request((uint8_t[]){6,0x80,0,0xff,0xff},5);
  request((uint8_t[]){CMD_READ_FLASH_ISP,0,4,0x20},4);
  CHECK(spi_len == 24 && spi[0] == 0x4d && spi[2] == 0);
  CHECK(spi[4] == 0x20 && spi[8] == 0x28);
  CHECK(spi[12] == 0x4d && spi[14] == 1);
  CHECK(avrisp_current_address == 0x80010001UL);
  spi_len=0;
  request((uint8_t[]){6,0x80,0,0xff,0xff},5);
  request((uint8_t[]){CMD_PROGRAM_FLASH_ISP,0,4,1,0,0x40,0x4c,0x20,0xff,0xff,1,2,3,4},14);
  CHECK(spi_len == 24 && spi[4] == 0x40 && spi[8] == 0x48);
  CHECK(spi[12] == 0x4d && spi[14] == 1);
  CHECK(avrisp_current_address == 0x80010001UL);
  spi_len=0;
  request((uint8_t[]){CMD_READ_SIGNATURE_ISP,3,0x30,0,0,0},6);
  reply((uint8_t[]){CMD_READ_SIGNATURE_ISP,0,3,0},4);
  request((uint8_t[]){CMD_PROGRAM_FUSE_ISP,0xac,0xa0,0,0xaa},5);
  reply((uint8_t[]){CMD_PROGRAM_FUSE_ISP,0,0},3);

  reset_test();
  /* avrdude emits 256-byte XPROG chunks, including on 512-byte pages. */
  memset(p, 0, sizeof(p));
  p[0]=0x50; p[1]=4; p[2]=1; p[3]=3; p[4]=0; p[5]=0x80;
  p[8]=1; p[9]=0;
  for (unsigned i=0; i<256; i++) p[10+i]=i;
  request(p,266);
  reply((uint8_t[]){0x50,4,0},3);
  CHECK(contains(p+10,256));
  request((uint8_t[]){0x50,7,2,1,0},5);
  CHECK(avrisp_xprog_eeprom_page_size == 256);
  wire_len=0;
  request((uint8_t[]){0x50,3,4,0,0x8c,0,0},7);
  reply((uint8_t[]){0x50,3,0},3);
  uint8_t tagged[259] = {0xa0,0xff,0x64};
  CHECK(contains(tagged,sizeof(tagged))); /* REPEAT 255 + all 256 tags */
  request((uint8_t[]){0x50,7,2,0,32},5);
  wire_len=0;
  request((uint8_t[]){0x50,3,7,0,0x8c,0,0x20},7);
  reply((uint8_t[]){0x50,3,0},3);
  uint8_t page_tags[35] = {0xa0,31,0x64};
  CHECK(contains(page_tags,sizeof(page_tags)));
  CHECK(contains((uint8_t[]){0x4c,0x20,0,0x8c,0,0},6));
  request((uint8_t[]){0x50,7,2,0,0},5);
  reply((uint8_t[]){0x50,7,1},3);
  CHECK(avrisp_xprog_eeprom_page_size == 32);
  request((uint8_t[]){0x51,99},2);
  reply((uint8_t[]){0x51,STATUS_CMD_FAILED},2);
  CHECK(avrisp_xprog_selected_protocol == 0);
  request((uint8_t[]){0x50,0x99},2);
  reply((uint8_t[]){0x50,0x99,1},3);
  request((uint8_t[]){0x50,4,4,0,0,0,0,1,0,0},10);
  reply((uint8_t[]){0x50,4,1},3);

  wire_len=phy_pos=0;
  request((uint8_t[]){0x50,3,2,0,0x80,0x10,0},7);
  reply((uint8_t[]){0x50,3,0},3);
  /* AU Table 33-5 and LUFA specify a PDI-write trigger for APP/BOOT erase. */
  CHECK(contains((uint8_t[]){0x4c,0,0x10,0x80,0,0},6));
  wire_len=phy_pos=0;
  request((uint8_t[]){0x50,5,1,0,0x80,0,0,0,0},9);
  CHECK(wire_len == 0);
  reply((uint8_t[]){0x50,5,0},3);
  phy_pos=0; phy_len=6;
  memcpy(phy_rx,(uint8_t[]){0,2,0,0x11,0x22,0x33},6);
  request((uint8_t[]){0x50,6,3},3);
  reply((uint8_t[]){0x50,6,0,0x33,0x11,0x22},6);

  reset_test();
  request((uint8_t[]){0x50,1},2);
  reply((uint8_t[]){0x50,1,0},3);
  mock_require_idle_clock=true;
  request((uint8_t[]){0x50,5,1,0,0x80,0,0,1,0},9);
  CHECK(in_len == 259 && usb_reply[2] == 0);
  mock_require_idle_clock=false;
  CHECK(IEC1bits.T4IE && PR4 == 639);
  _T4Interrupt();
  CHECK(AVRISP_CLK_LAT == HIGH);
  phy_pos=0; phy_len=2; phy_rx[0]=2; phy_rx[1]=0;
  avrisp_xprog_shutdown();
  CHECK(!IEC1bits.T4IE);
  request((uint8_t[]){0x51,2},2);
  request((uint8_t[]){0x50,4,4,0,0,0,0x3f,0,0,4,0xaa,0xff,0xff,0xff},14);
  reply((uint8_t[]){0x50,4,0},3); /* TPI fuse blocksize != PDI byte write */
  request((uint8_t[]){0x50,5,1,1,0,0,0,0,1},9);
  reply((uint8_t[]){0x50,5,1},3); /* no 32->16-bit address truncation */

  reset_test();
  bad_parity=true;
  CHECK(avrisp_xprog_receive_byte() == 0xff && !avrisp_timeout_ticks);
  bad_parity=false; bad_stop=true;
  avrisp_timeout_ticks=AVRISP_TIMEOUT_TICKS;
  CHECK(avrisp_xprog_receive_byte() == 0xff && !avrisp_timeout_ticks);
  reset_test();
  for (unsigned i=0; i<256; i++) {
    avrisp_xprog_send_byte(i);
    CHECK(BP_MOSI_DIR == OUTPUT);
    phy_pos=0; phy_len=1; phy_rx[0]=i;
    CHECK(avrisp_xprog_receive_byte() == i && BP_MOSI_DIR == INPUT);
  }

  reset_test();
  avrisp_xprog_selected_protocol=XPROG_PROTOCOL_TPI;
  avrisp_xprog_enable_target_tpi();
  wire_len=phy_pos=0;
  phy_len=2; phy_rx[0]=2; phy_rx[1]=0;
  avrisp_xprog_shutdown();
  CHECK(contains((uint8_t[]){TPI_CMD_SSTCS(TPI_REG_STATUS),0},2));
  CHECK(BP_AUX0_DIR == INPUT && !IEC1bits.T4IE);

  /* Every truncation of each fixed header fails before target access. */
  static const uint8_t fixed[][14] = {
    {2,0x98,6}, {3,0x98}, {6,0,0,0,0}, {0x10,0,0,0,1,0,0,0,0,0,0,0},
    {0x11,0,0}, {0x12,0,0,0,0,0,0}, {0x14,0,1,0x20},
    {0x17,0,0,0,0}, {0x18,1,0,0,0,0}, {0x1d,1,0,0,0},
    {0x51,0}, {0x50,1}, {0x50,3,1,0,0,0,0},
    {0x50,4,1,0,0,0,0,0,0,1,0xff}, {0x50,5,1,0,0,0,0,0,1},
    {0x50,6,1}, {0x50,7,1,1,0,1,0xc0}
  };
  static const unsigned lengths[] = {3,2,5,12,3,7,4,5,6,5,2,2,7,11,9,3,7};
  for (unsigned i=0; i<sizeof(lengths)/sizeof(*lengths); i++) {
    for (unsigned n=1; n<lengths[i]; n++) {
      reset_test();
      request(fixed[i],n);
      CHECK(!spi_len && !wire_len);
      CHECK(in_len == (fixed[i][0] == 0x50 ? 3u : 2u));
      CHECK(usb_reply[in_len-1] != 0);
    }
  }

  /* BPv4 CDC framing, identity, seq echo, XOR and compacted data. */
  reset_test();
  avrisp_transport=AVRISP_TRANSPORT_SERIAL;
  serial_len=serial_pos=serial_tx_len=0;
  frame((uint8_t[]){1},1,0x7e,false);
  frame((uint8_t[]){1},1,0x80,true);
  frame((uint8_t[]){CMD_PROGRAM_EEPROM_ISP,0,2,1,0,0xc0,0,0xa0,0xff,0xff,0x12,0x34},12,0x81,false);
  memset(p,0,300);
  frame(p,300,0x82,false); /* oversized frame must not poison next frame */
  frame((uint8_t[]){6,0},2,0x83,false);
  frame((uint8_t[]){1},1,0x84,false);
  avrisp_serial_run();
  CHECK(serial_tx[0] == 0x1b && serial_tx[1] == 0x7e);
  CHECK(serial_tx[4] == 0x0e && serial_tx[7] == 8);
  CHECK(!memcmp(serial_tx+8,"STK500_2",8));
  uint8_t xor=0;
  for (unsigned i=0; i<17; i++) xor ^= serial_tx[i];
  CHECK(xor == 0);
  CHECK(serial_tx[18] == 0x80 && serial_tx[22] == 0xb0 && serial_tx[23] == 0xc1);
  CHECK(spi_len == 8 && spi[3] == 0x12 && spi[7] == 0x34);
  CHECK(serial_tx[serial_tx_len-16] == 0x84);
  blocked_cdc=true;
  avrisp_timeout_ticks=AVRISP_TIMEOUT_TICKS;
  avrisp_activity_ticks=AVRISP_IDLE_TICKS;
  CHECK(!avrisp_serial_write_byte(0) && !avrisp_activity_ticks);
  blocked_cdc=false;
  reset_test();
  avrisp_transport=AVRISP_TRANSPORT_SERIAL;
  serial_len=serial_pos=serial_tx_len=0;
  p[0]=CMD_PROGRAM_EEPROM_ISP; p[1]=1; p[2]=0; p[3]=1;
  p[4]=0; p[5]=0xc0; p[6]=0; p[7]=0xa0; p[8]=p[9]=0xff;
  for (unsigned i=0; i<256; i++) p[10+i]=(uint8_t)i;
  frame(p,266,0xa0,false);
  frame((uint8_t[]){CMD_SIGN_ON},1,0xa1,false);
  avrisp_serial_run();
  CHECK(serial_pos == serial_len && spi_len == 1024);
  CHECK(serial_tx[1] == 0xa0 && serial_tx[2] == 0 && serial_tx[3] == 2);
  CHECK(serial_tx[5] == CMD_PROGRAM_EEPROM_ISP && serial_tx[6] == 0);
  CHECK(serial_tx_len == 25 && serial_tx[9] == 0xa1);
  CHECK(!memcmp(serial_tx+16,"STK500_2",8));
  xor=0;
  for (unsigned i=0; i<8; i++) xor ^= serial_tx[i];
  CHECK(xor == 0);
  printf("AVRISP native regression checks passed: %u\n", checks);
  return 0;
}
