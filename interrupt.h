#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

extern uint8_t IME;
extern uint8_t IF;
extern uint8_t IE;

void interrupt(uint8_t);
void interrupt_enable(void);
bool interrupt_flush(void);

enum {
	INTR_VBLANK  = 0x01,
	INTR_LCDSTAT = 0x02,
	INTR_TIMER   = 0x04,
	INTR_SERIAL  = 0x08,
	INTR_JOYPAD  = 0x10
};

#endif
