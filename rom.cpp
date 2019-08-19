#include <stdio.h>
#include <string.h>
#include "rom.h"

const uint8_t *bytes;

static s_rominfo rominfo;
static char romtitle[20] = "";

static const uint16_t rombank_count[256] = {
	2, 4, 8, 16, 32, 64, 128, 256, 512,
	/* 0x52 */
	72, 80, 96
};

static const uint8_t rambank_count[256] = {
	0, 1, 1, 4, 16, 8
};

static const uint8_t header[] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

bool rom_init(const uint8_t* rombytes)
{
	if(!rombytes)
		return false;

	/* Check Nintendo logo on ROM header */
	if(memcmp(&rombytes[0x104], header, sizeof(header)) != 0)
		return false;
	
	memcpy(romtitle, &rombytes[0x134], 0x143-0x134);

	uint8_t cart_type  = rombytes[0x147];
	rominfo.rom_banks  = rombytes[0x148]>=0x52 ? rombank_count[rombytes[0x148] - 0x52] : rombank_count[rombytes[0x148]];
	rominfo.ram_banks  = rambank_count[rombytes[0x149]];
	//rominfo.region    = rombytes[0x14A];
	//rominfo.version   = rombytes[0x14C];

	uint8_t checksum = 0;
	for(int i = 0x134; i <= 0x14C; i++)
		checksum = checksum - rombytes[i] - 1;

	if(rombytes[0x14D] != checksum)
		return false;

	bytes = rombytes;

	switch(cart_type)
	{
		case 0x09:
			rominfo.has_battery = true;
		case 0x08:
		case 0x00:
			rominfo.rom_mapper = NROM;
			break;
		case 0x03:
			rominfo.has_battery = true;
		case 0x02:
		case 0x01:
			rominfo.rom_mapper = MBC1;
			break;
		case 0x06:
			rominfo.has_battery = true;
		case 0x05:
			rominfo.rom_mapper = MBC2;
			break;
		case 0x0D:
			rominfo.has_battery = true;
		case 0x0C:
		case 0x0B:
			rominfo.rom_mapper = MMM01;
			break;
		case 0x10:
		case 0x0F:
			rominfo.has_rtc = true;
		case 0x13:
			rominfo.has_battery = true;
		case 0x12:
		case 0x11:
			rominfo.rom_mapper = MBC3;
			break;
		case 0x1E:
		case 0x1B:
			rominfo.has_battery = true;
		case 0x1D:
		case 0x1C:
		case 0x1A:
		case 0x19:
			rominfo.rom_mapper = MBC5;
			break;
	}

	return true;
}

const s_rominfo *rom_get_info(void)
{
	return &rominfo;
}

uint32_t rom_get_ram_size()
{
	if (rominfo.rom_mapper == MBC2)
		return 512;
	return rominfo.ram_banks * 1024 * 8;
}

const char* rom_get_title()
{
	return romtitle;
}

const uint8_t *rom_getbytes(void)
{
	return bytes;
}
