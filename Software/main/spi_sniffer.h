#ifndef spi_sniffer_h_
#define spi_sniffer_h_

#include <stdint.h>

void spi_sniffer_init(uint8_t buffer[], uint16_t maxlengthbytes);
uint16_t spi_sniffer_sniff();

#endif
