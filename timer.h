#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_set_tac(uint8_t);
void timer_cycle(uint32_t);
uint8_t timer_get_div(void);
uint8_t timer_get_counter(void);
uint8_t timer_get_modulo(void);
uint8_t timer_get_tac(void);
void timer_reset_div(void);
void timer_set_counter(uint8_t);
void timer_set_modulo(uint8_t);

#endif
