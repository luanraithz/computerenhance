#include "arpa/inet.h"
#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include <bits/byteswap.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

static u16 buff[4] = {7521, 124, 4223, 42};
// Big endian - Or Network byte order - Most significant byte first
// Reading u16
// 00011101 01100001 | 00000000 01111100 | 00010000 01111111 | 00000000 00101010
//
// Reading u32
// 00000000 01111100 00011101 01100001 | 00000000 00101010 00010000 01111111
//
//
// Little endian
// 01010100 00000000 | 11111110 00001000 | 0011111 00010000 | 11101000 00000000
//  xxd -b test-bin returns the little endian
//

void assert_file_size(FILE *f, int size) {
    fseek(f, 0l, SEEK_END);
    int s = ftell(f);

    assert(size == s);
}

void assert_byte_as_sequece(FILE *f, u8 bytes[], int count) {
    u8 byte = 0;
    fseek(f, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        assert(fread(&byte, sizeof(u8), 1, f) != 0);
        if (byte != bytes[i]) {
            printf("%i: %i !== %i\n", i, byte, bytes[i]);
        }
    }
}

int main() {
    FILE *f = fopen("test-bin", "w+r");

    fwrite(buff, sizeof(u16), 4, f);

    u8 littleEndian[8] = {97, 29, 124, 0, 127, 16, 42, 0};
    // Bytes are stored (on my computer :P) in little endian
    assert_byte_as_sequece(f, littleEndian, 8);

    fseek(f, 0, SEEK_SET);
    u16 saved[4] = {};
    assert(fread(&saved, sizeof(u16), 4, f) != 0);
    for (int i = 0; i < 4; i++) {
        // When we get them to memory, we turn back into big endian
        assert(saved[i] == buff[i]);
    }

    fseek(f, 0, SEEK_SET);
    u32 saved32[2] = {};
    assert(fread(&saved32, sizeof(u32), 2, f) != 0);
    printf("%i\n", saved32[0]);
    assert(saved32[0] == 8133985);
    assert(saved32[1] == 2756735);
    assert_file_size(f, 8);
}
