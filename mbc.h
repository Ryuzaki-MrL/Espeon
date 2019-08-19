#ifndef MBC_H
#define MBC_H

#include <stdint.h>

typedef uint8_t(*MBCReader)(uint16_t);
typedef void(*MBCWriter)(uint16_t, uint8_t);

extern MBCReader mbc_read_ram;
extern MBCWriter mbc_write_rom;
extern MBCWriter mbc_write_ram;

extern const uint8_t* rombank;
extern uint8_t* rambank;

bool mbc_init();
uint8_t* mbc_get_ram();

void MBC1_write_ROM(uint16_t, uint8_t);
void MBC1_write_RAM(uint16_t, uint8_t);
uint8_t MBC1_read_RAM(uint16_t);

void MBC3_write_ROM(uint16_t, uint8_t);
void MBC3_write_RAM(uint16_t, uint8_t);
uint8_t MBC3_read_RAM(uint16_t);

#endif
