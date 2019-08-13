#include <stdint.h>
#include "timer.h"
#include "interrupt.h"

static unsigned int ticks;
static unsigned char started;
static unsigned int speed = 1024;
static unsigned char TAC;
static unsigned int TIMA;
static unsigned char TMA;
static uint16_t divider;

void timer_reset_div(void)
{
	divider = 0;
}

unsigned char timer_get_div(void)
{
	return (divider >> 8);
}

void timer_set_counter(unsigned char v)
{
	TIMA = v;
}

unsigned char timer_get_counter(void)
{
	return TIMA;
}

void timer_set_modulo(unsigned char v)
{
	TMA = v;
}

unsigned char timer_get_modulo(void)
{
	return TMA;
}

void timer_set_tac(unsigned char v)
{
	const int speeds[] = {1024, 16, 64, 256};
	TAC = v;
	started = v&4;
	speed = speeds[v&3];
}

unsigned char timer_get_tac(void)
{
	return TAC;
}

void timer_cycle(unsigned int delta)
{
	delta *= 4;
	divider += delta;
	if(started) {
		ticks += delta;
		if(ticks >= speed) {
			//ticks -= speed;
			ticks = 0;
			if(++TIMA >= 0x100) {
				interrupt(INTR_TIMER);
				TIMA = TMA;
			}
		}
	}
}
