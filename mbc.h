#ifndef MBC_H
#define MBC_H

typedef unsigned char(*MBCReader)(unsigned short);
typedef void(*MBCWriter)(unsigned short, unsigned char);

extern MBCReader mbc_read_ram;
extern MBCWriter mbc_write_rom;
extern MBCWriter mbc_write_ram;

extern const unsigned char* rombank;
extern unsigned char* rambank;

bool mbc_init();
unsigned char* mbc_get_ram();

void MBC1_write_ROM(unsigned short, unsigned char);
void MBC1_write_RAM(unsigned short, unsigned char);
unsigned char MBC1_read_RAM(unsigned short);

void MBC3_write_ROM(unsigned short, unsigned char);
void MBC3_write_RAM(unsigned short, unsigned char);
unsigned char MBC3_read_RAM(unsigned short);

#endif
