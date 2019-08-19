#include <stdint.h>
#include "timer.h"
#include "interrupt.h"

static uint32_t ticks;
static uint8_t started;
static uint32_t speed = 1024;
static uint8_t TAC;
static uint32_t TIMA;
static uint8_t TMA;
static uint16_t divider;

void timer_reset_div(void)
{
	divider = 0;
}

uint8_t timer_get_div(void)
{
	return (divider >> 8);
}

void timer_set_counter(uint8_t v)
{
	TIMA = v;
}

uint8_t timer_get_counter(void)
{
	return TIMA;
}

void timer_set_modulo(uint8_t v)
{
	TMA = v;
}

uint8_t timer_get_modulo(void)
{
	return TMA;
}

void timer_set_tac(uint8_t v)
{
	const int speeds[] = {1024, 16, 64, 256};
	TAC = v;
	started = v&4;
	speed = speeds[v&3];
}

uint8_t timer_get_tac(void)
{
	return TAC;
}

void timer_cycle(uint32_t delta)
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
