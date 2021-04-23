#include "printerdisplay.h"

#include "esp_log.h"
#include "stdio.h"

uint8_t displaybuffer[64][128/8];

uint8_t x_pointer = 0;
uint8_t y_pointer = 0;

bool display_updated = false;

//static const char *TAG = "DSP";

void printerDisplayInit()
{
}

void printerDisplaySetAddr(uint8_t vert_addr, uint8_t hor_addr)
{
	x_pointer = (hor_addr % 8) * 2;
	y_pointer = vert_addr + (hor_addr >= 8 ? 32 : 0);
	if (y_pointer > 48) {
		display_updated = true;
	}
	//printf("v: %d h: %d x: %d y: %d\n", vert_addr, hor_addr, x_pointer, y_pointer);
	
}
void printerDisplayWrite(uint8_t data)
{
	displaybuffer[y_pointer][x_pointer] = data;
	x_pointer++;
	if (x_pointer > 16)
		x_pointer = 0;
}

uint8_t* printerDisplayDataStart() {
	return &displaybuffer[0][0];
}

bool printerDisplayIsUpdated() {
	return display_updated;
}
void printerDisplayClearUpdate() {
	display_updated = false;
}
