#ifndef CPU_H
#define CPU_H

#include <stdint.h>

extern bool halted;

void cpu_init(void);
uint32_t cpu_cycle(void);
uint32_t cpu_get_cycles(void);
void cpu_interrupt(uint16_t);

#endif
