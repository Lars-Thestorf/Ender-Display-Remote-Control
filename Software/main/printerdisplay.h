#ifndef printerdisplay_h_
#define printerdisplay_h_

#include "stdint.h"
#include "stdbool.h"

void printerDisplayInit();

void printerDisplaySetAddr(uint8_t vert_addr, uint8_t hor_addr);
void printerDisplayWrite(uint8_t data);

bool printerDisplayIsUpdated();
uint8_t* printerDisplayDataStart();

void printerDisplayClearUpdate();

#endif
