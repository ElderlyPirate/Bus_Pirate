/* CC0. Execute the production parser/CDC/EP0 state machines natively. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../v4/usb_stack.c"
#include "../v4/cdc.c"
#include "../v4/bootloader.c"

static unsigned writes, erases, checks;
static uint32_t written_address;
static uint8_t written_row[192], flash_result = 'K';
uint8_t flash_row(uint32_t address, uint8_t *data, uint8_t erase) {
    ++writes;
    erases += erase;
    written_address = address;
    memcpy(written_row, data, 192);
    return flash_result;
}
#define CHECK(x) do { ++checks; assert(x); } while (0)

static void in_done(void) {
    usb_bdt[1].BDSTAT = (usb_bdt[1].BDSTAT & DTS) | 0x24;
    transaction(8);
}
static void request(uint8_t type, uint8_t code, uint16_t value, uint16_t index, uint16_t length) {
    uint8_t s[8] = {type, code, value, value >> 8, index, index >> 8, length, length >> 8};
    memcpy(ep0_out, s, 8);
    usb_bdt[0].BDCNT = 8;
    usb_bdt[0].BDSTAT = 0x34;
    transaction(0);
}
static void out_data(const uint8_t *data, uint8_t length) {
    if (length) memcpy(ep0_out, data, length);
    usb_bdt[0].BDCNT = length;
    usb_bdt[0].BDSTAT = 4;
    transaction(0);
}
static void configure(void) {
    reset();
    request(0, 5, 7, 0, 0);
    CHECK(U1ADDR == 0);
    in_done();
    CHECK(U1ADDR == 7);
    request(0, 9, 1, 0, 0);
    in_done();
    CHECK(configured);
    boot_init();
    boot_poll(); /* Acknowledge the reset/reconfigure generation. */
}
static void bulk_packet(const uint8_t *data, uint8_t length) {
    CHECK(usb_bdt[4].BDSTAT & UOWN);
    if (length) memcpy(rx, data, length);
    usb_bdt[4].BDCNT = length;
    usb_bdt[4].BDSTAT = (usb_bdt[4].BDSTAT & DTS) | 4;
}
static void tx_done(void) { usb_bdt[5].BDSTAT &= ~UOWN; }
static uint8_t command_packet(uint32_t address, uint8_t cmd, unsigned length, int bad) {
    uint8_t packet[260], crc = 0;
    unsigned i, total = 5 + length, start = 0;
    packet[0] = address >> 16;
    packet[1] = address >> 8;
    packet[2] = address;
    packet[3] = cmd;
    packet[4] = length;
    for (i = 5; i < total; ++i) packet[i] = (uint8_t)(i * 13);
    if (length) {
        for (i = 0; i < total - 1; ++i) crc -= packet[i];
        packet[total - 1] = crc ^ !!bad;
    }
    tx_done();
    while (start < total) {
        unsigned n = total - start > 64 ? 64 : total - start;
        bulk_packet(packet + start, n);
        for (i = 0; i < n; ++i) boot_poll();
        start += n;
    }
    CHECK(usb_bdt[5].BDSTAT & UOWN);
    CHECK(usb_bdt[5].BDCNT == 1);
    return tx[0];
}
static void hello(void) {
    uint8_t c = 0xC1;
    tx_done();
    bulk_packet(&c, 1);
    boot_poll();
    CHECK(usb_bdt[5].BDCNT == 4);
    CHECK(tx[0] == DEVICEID && tx[1] == VER_H && tx[2] == VER_L && tx[3] == 'K');
    tx_done();
}

static void ep0_tests(void) {
    unsigned i, n;
    uint8_t original, byte;
    CHECK(sizeof(cdc_config_descriptor) == usb_word(cdc_config_descriptor + 2));
    n = 0;
    for (i = 0; i < USB_NUM_STRINGS; ++i) {
        CHECK(cdc_str_descs[n] >= 2 && !(cdc_str_descs[n] & 1));
        CHECK(cdc_str_descs[n + 1] == 3);
        n += cdc_str_descs[n];
    }
    CHECK(n == sizeof(cdc_str_descs));
    cdc_init();
    reset();
    request(0x80, 6, 0x100, 0, 8);
    CHECK(phase == STATUS_OUT && usb_bdt[1].BDCNT == 8);
    CHECK(!memcmp(ep0_in, cdc_device_descriptor, 8));
    in_done();
    CHECK(!(usb_bdt[1].BDSTAT & UOWN)); /* No surplus ZLP for an exact request. */
    out_data(0, 0);
    CHECK(phase == IDLE);
    request(0x80, 6, 0x100, 0, 255);
    n = 0;
    while (usb_bdt[1].BDSTAT & UOWN) { n += usb_bdt[1].BDCNT; in_done(); }
    CHECK(n == 18 && phase == STATUS_OUT);
    out_data(0, 0);
    /* A truncated descriptor must terminate with ZLP only when required. */
    usb_reply(cdc_str_descs + 4, 16, 255);
    n = 0;
    while (usb_bdt[1].BDSTAT & UOWN) { ++n; in_done(); }
    CHECK(n == 3);
    for (i = 1; i < 256; ++i) {
        request(0x80, 6, 0x200 | i, 0, 255);
        CHECK(phase == STALLED);
    }
    for (i = 4; i < 256; ++i) {
        request(0x80, 6, 0x300 | i, 0x409, 255);
        CHECK(phase == STALLED);
    }
    request(0x80, 6, 0x600, 0, 10);
    CHECK(phase == STALLED);
    request(0x80, 6, 0x300, 0, 255);
    CHECK(phase == STATUS_OUT && usb_bdt[1].BDCNT == 4);
    request(0, 5, 9, 0, 0);
    request(0x80, 6, 0x100, 0, 8); /* Abort deferred address with a new SETUP. */
    in_done();
    CHECK(U1ADDR == 0);
    configure();
    for (i = 0; i < 256; ++i) {
        request(0x82, 0, 0, i, 2);
        if (i == 0 || i == 0x80 || i == 0x81 || i == 2 || i == 0x82)
            CHECK(phase == STATUS_OUT);
        else CHECK(phase == STALLED);
    }
    original = usb_bdt[4].BDSTAT;
    request(0x82, 0, 0, 2, 2);
    CHECK(usb_bdt[4].BDSTAT == original && !ep0_in[0]);
    request(2, 3, 0, 2, 0);
    CHECK(halted[4] && (usb_bdt[4].BDSTAT & BSTALL));
    request(0x82, 0, 0, 2, 2);
    CHECK(ep0_in[0] == 1);
    request(2, 1, 0, 2, 0);
    CHECK(!halted[4] && usb_bdt[4].BDSTAT == (UOWN | DTSEN));
    request(0xA1, 0x21, 0, 0, 9);
    CHECK(usb_bdt[1].BDCNT == 7 && ep0_in[6] == 8);
    request(0x21, 0x20, 0, 0, 6);
    CHECK(phase == STALLED);
    request(0x21, 0x20, 0, 1, 7);
    CHECK(phase == STALLED);
    request(0x21, 0x20, 0, 0, 7);
    CHECK(phase == LINE_OUT);
    { const uint8_t coding[7] = {0, 0, 0, 0, 2, 4, 7};
      out_data(coding, 7); CHECK(phase == STATUS_IN); in_done(); }
    request(0xA1, 0x21, 0, 0, 7);
    CHECK(ep0_in[0] == 0 && ep0_in[4] == 2 && ep0_in[6] == 7);
    request(0x21, 0x20, 0, 0, 7);
    { const uint8_t invalid[7] = {0, 0, 0, 0, 0, 0, 0};
      out_data(invalid, 7); CHECK(phase == STALLED); }
    request(0x21, 0x20, 0, 0, 7);
    request(0x80, 6, 0x100, 0, 8); /* Aborted OUT data must not reach CDC. */
    out_data(0, 0);
    CHECK(phase == IDLE);
    bulk_packet(0, 0);
    CHECK(!cdc_get(&byte) && (usb_bdt[4].BDSTAT & UOWN));
    { uint8_t data[64]; for (i = 0; i < 64; ++i) data[i] = i;
      bulk_packet(data, 64);
      for (i = 0; i < 64; ++i) { CHECK(cdc_get(&byte) && byte == i); }
      CHECK(usb_bdt[4].BDSTAT & UOWN); }
    CHECK(cdc_send((const uint8_t *)"TEST", 4));
    CHECK(!cdc_send((const uint8_t *)"FAIL", 4));
    CHECK(!memcmp(tx, "TEST", 4)); /* Do not overwrite an SIE-owned IN buffer. */
    request(0, 9, 0, 0, 0);
    CHECK(!configured && !U1EP1 && !U1EP2);
    for (i = 2; i < 6; ++i) CHECK(!(usb_bdt[i].BDSTAT & UOWN));
    configure();
    timer_last = 65500; TMR1 = 214; timer_fraction = 0;
    usb_handler(); CHECK(timer_fraction == 0 && milliseconds == 1);
}

static void parser_tests(void) {
    unsigned size, address, before;
    configure(); hello();
    for (size = 0; size < 256; ++size) {
        before = writes;
        CHECK(command_packet(0x2000, 2, size, 0) == (size == 193 ? 'K' : 'U'));
        CHECK(writes == before + (size == 193));
    }
    CHECK(command_packet(0x2000, 2, 193, 1) == 'N');
    CHECK(command_packet(0x2000, 0x82, 193, 0) == 'U');
    for (address = 1; address < 128; ++address)
        CHECK(command_packet(0x2000 + address, 2, 193, 0) == 'P');
    before = writes;
    for (address = 0x400; address < 0x2000; address += 128)
        CHECK(command_packet(address, 2, 193, 0) == 'P');
    CHECK(writes == before);
    CHECK(command_packet(0x2A780, 2, 193, 0) == 'K');
    CHECK(command_packet(0x2A800, 2, 193, 0) == 'P');
    CHECK(command_packet(0x800000, 2, 193, 0) == 'P');
    CHECK(command_packet(0x10000, 2, 193, 0) == 'K');
    CHECK(written_address == 0x10000);
    CHECK(command_packet(0x2000, 1, 1, 0) == 'K');
    before = erases;
    CHECK(command_packet(0x2400, 2, 193, 0) == 'P');
    CHECK(erases == before);
    CHECK(command_packet(0x2000, 1, 1, 0) == 'K');
    CHECK(command_packet(0x2000, 2, 193, 1) == 'N');
    CHECK(command_packet(0x2000, 2, 193, 0) == 'K');
    CHECK(erases == before);
    CHECK(command_packet(0x2000, 1, 1, 0) == 'K');
    CHECK(command_packet(0x2080, 2, 193, 0) == 'K');
    CHECK(erases == before + 1);
    CHECK(command_packet(0, 2, 193, 0) == 'K');
    CHECK(!memcmp(written_row, "\x04\x00\x04\x00\x00\x00", 6));
    flash_result = 'V';
    CHECK(command_packet(0x2000, 2, 193, 0) == 'V');
    flash_result = 'K';
    CHECK(command_packet(0x2000, 2, 193, 0) == 'K');
    tx_done();
    { uint8_t c = 0; bulk_packet(&c, 1); boot_poll(); }
    milliseconds += 1000;
    boot_poll(); CHECK(!active && !erase_pending);
    hello();
    CHECK(command_packet(0x2000, 1, 1, 0) == 'K');
    reset(); configure(); hello();
    before = erases;
    CHECK(command_packet(0x2000, 2, 193, 0) == 'K');
    CHECK(erases == before);
    CHECK(command_packet(0x010203, 0xFF, 1, 0) == 'K');
    CHECK(boot_restart);
}
int main(void) {
    ep0_tests(); parser_tests();
    printf("v4 production state machines: %u checks passed\n", checks);
    return 0;
}
