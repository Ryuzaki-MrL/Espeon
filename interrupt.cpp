#include "interrupt.h"
#include "cpu.h"

uint8_t IME;
uint8_t IF;
uint8_t IE;

static uint8_t ime_delay;

bool interrupt_flush(void)
{
	if(ime_delay >= 2)
	{
		ime_delay = 1;
		return 0;
	}
	if (ime_delay == 1) {
		IME = 1;
		ime_delay = 0;
	}

	/* Returns true if the cpu should be unhalted */
	if (!IME)
		return !!(IF & IE);

	/* Check interrupts from highest to lowest priority */
	for (int i = 0; i < 5; ++i) {
		if (IE & IF & (1 << i)) {
			IME = 0;
			IF &= ~(1 << i);
			cpu_interrupt(0x40 + i*0x08);
			return true;
		}
	}

	return false;
}

void interrupt_enable(void)
{
	ime_delay = 2;
}

void interrupt(uint8_t n)
{
	/* Add this interrupt to pending queue */
	IF |= n;

	/* Interrupt requested, unhalt CPU if IF and IE have a match */
	if(interrupt_flush())
		halted = 0;
}
