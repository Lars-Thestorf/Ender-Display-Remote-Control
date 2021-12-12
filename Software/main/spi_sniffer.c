#include "spi_sniffer.h"
#include "driver/spi_slave.h"

#define SPI_Controller VSPI_HOST
#define SPI_MOSI 33
#define SPI_CS 26
#define SPI_CLK 25

spi_slave_transaction_t trans;

void spi_sniffer_init(uint8_t buffer[], uint16_t maxlengthbytes) {
	spi_bus_config_t bcfg={
		.mosi_io_num = SPI_MOSI,
		.miso_io_num = -1,
		.sclk_io_num = SPI_CLK,
	};
	spi_slave_interface_config_t scfg={
		.spics_io_num = SPI_CS,
		.flags = 0,
		.queue_size = 1,
		.mode = 1,
	};

	esp_err_t spi_state = spi_slave_initialize(SPI_Controller, &bcfg, &scfg, 1);
	switch (spi_state){
		case ESP_ERR_NO_MEM:
		case ESP_ERR_INVALID_STATE:
		case ESP_ERR_INVALID_ARG:
			printf("SPI initialsation failed!\n");
			break;
		case ESP_OK:
			printf("SPI initialsation success!\n");
			break;
	}
	trans = (spi_slave_transaction_t){
		.length=maxlengthbytes*8,
		.rx_buffer=buffer
	};
}
uint16_t spi_sniffer_sniff() {
	esp_err_t spi_state = spi_slave_transmit(SPI_Controller, &trans, portMAX_DELAY);
	switch (spi_state){
		case ESP_ERR_NO_MEM:
		case ESP_ERR_INVALID_STATE:
		case ESP_ERR_INVALID_ARG:
			printf("SPI trans failed!\n");
			break;
		case ESP_OK:
			//printf("SPI trans success!\n");
			break;
	}
	return trans.trans_len;
}
