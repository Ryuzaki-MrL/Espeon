#ifndef INTERRUPT_H
#define INTERRUPT_H

extern unsigned char IME;
extern unsigned char IF;
extern unsigned char IE;

void interrupt(unsigned char);
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
