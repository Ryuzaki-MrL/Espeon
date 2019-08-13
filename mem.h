#ifndef MEM_H
#define MEM_H

extern bool usebootrom;

bool mmu_init(const unsigned char* bootrom = nullptr);
unsigned char mem_get_byte(unsigned short);
inline unsigned short mem_get_word(unsigned short i) { return mem_get_byte(i) | (mem_get_byte(i+1)<<8); }
void mem_write_byte(unsigned short, unsigned char);
inline void mem_write_word(unsigned short d, unsigned short i) { mem_write_byte(d, i&0xFF); mem_write_byte(d+1, i>>8); }
unsigned char* mem_get_bytes(void);

#endif
