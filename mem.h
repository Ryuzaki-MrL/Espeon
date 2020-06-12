#ifndef MEM_H
#define MEM_H

#include <stdint.h>

extern bool usebootrom;
extern uint8_t* mem;

bool mmu_init(const uint8_t* bootrom = nullptr);
uint8_t mem_get_byte(uint16_t);
inline uint16_t mem_get_word(uint16_t i) { return mem_get_byte(i) | (mem_get_byte(i+1)<<8); }
void mem_write_byte(uint16_t, uint8_t);
inline void mem_write_word(uint16_t d, uint16_t i) { mem_write_byte(d, i&0xFF); mem_write_byte(d+1, i>>8); }

#endif
