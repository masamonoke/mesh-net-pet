#include "crc.h"

#include <stdbool.h>

uint16_t crc16_table[256];

static void init_crc16_table(void) {
    uint16_t crc;
    const uint16_t polynomial = 0xA001;
	uint16_t i;
	uint16_t j;

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crc16_table[i] = crc;
    }
}

uint16_t crc16(const unsigned char *data, size_t length) {
    uint16_t crc;
	crc = 0xFFFF;
	size_t i;
	static bool init = false;

	if (!init) {
		init = true;
		init_crc16_table();
	}

    for (i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }

    return crc;
}
