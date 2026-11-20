/* CC0. Pure host-tool units: never open a serial port. */
#include <assert.h>
#define main pirate_loader_main
#include "../pirate-loader/pirate-loader.c"
#undef main
int main(int argc, char **argv) {
    uint8 data[16] = {0}, pages[PIC_NUM_PAGES] = {0};
    uint8 *image;
    unsigned long i;
    if (argc > 1) {
        int count = readHEX(argv[1], data, sizeof(data), pages);
        if (count < 0) return 2;
        assert(count == 1);
        assert(data[0] == 0x56 && data[1] == 0x12 && data[2] == 0x34);
        assert(pages[0]);
        return 0;
    }
    for (i = 0; i < sizeof(data); ++i) data[i] = (uint8)i;
    data[15] = makeCrc(data, 15);
    { uint8 sum = 0; for (i = 0; i < 16; ++i) sum += data[i]; assert(!sum); }
    assert(bootloaderOk('K') && bootloaderOk(0x13) && !bootloaderOk('P'));
    flashsize = BPV3_FLASHSIZE;
    image = malloc((flashsize / 2) * 3);
    assert(image);
    memset(image, 0xFF, (flashsize / 2) * 3);
    memcpy(image, "\x04\x00\x02\x00\x00\x00", 6);
    fixJumps(image, pages);
    assert(!memcmp(image, "\x04\x00\xA8\x00\x00\x00", 6));
    assert(!memcmp(image + (0xA7FC / 2) * 3, "\x04\x00\x02\x00\x00\x00", 6));
    assert(pages[0xA400 / 1024]);
    pages[0xAC00 / 1024] = 1;
    assert(sendFirmware(-1, image, pages) < 0); /* Must reject before serial I/O. */
    free(image);
    puts("pirate-loader CRC, clone handshake, relocated vector and preflight passed");
    return 0;
}
