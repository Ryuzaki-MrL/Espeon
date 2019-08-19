#include <M5Stack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_partition.h>

#include "espeon.h"
#include "interrupt.h"
#include "mbc.h"
#include "rom.h"
#include "gbborder.h"

#define PARTITION_ROM (esp_partition_subtype_t(0x40))
#define MAX_ROM_SIZE (2*1024*1024)

#define JOYPAD_INPUT 5
#define JOYPAD_ADDR  0x88

#define GETBIT(x, b) (((x)>>(b)) & 0x01)

#define GAMEBOY_WIDTH 160
#define GAMEBOY_HEIGHT 144

#define CENTER_X ((320 - GAMEBOY_WIDTH)  >> 1)
#define CENTER_Y ((240 - GAMEBOY_HEIGHT) >> 1)

static fbuffer_t* pixels;

volatile int spi_lock = 0;
volatile bool sram_modified = false;

uint16_t palette[] = { 0xFFFF, 0xAAAA, 0x5555, 0x2222 };

// QueueHandle_t fbqueue;

// static void videoTask(void *arg) {
	// fbuffer_t* fb = NULL;
	// int x = (320 - GAMEBOY_WIDTH)  >> 1;
	// int y = (240 - GAMEBOY_HEIGHT) >> 1;
	// while(true) {
		// xQueueReceive(fbqueue, &fb, portMAX_DELAY);
		// M5.Lcd.drawBitmap(x, y, GAMEBOY_WIDTH, GAMEBOY_HEIGHT, fb);
	// }
// }

void espeon_render_border()
{
	M5.Lcd.drawBitmap(0, 0, 320, 240, (const uint16_t*)gbborder);
}

static void espeon_request_sd_write()
{
	spi_lock = 1;
}

void espeon_init(void)
{
	/* LCDEnable, SDEnable, SerialEnable, I2CEnable */
	M5.begin(true, true, false, true);
	M5.Lcd.setBrightness(0x7F); // 50%
	
	/* Stops the speaker from having a damn stroke */
	ledcDetachPin(SPEAKER_PIN);
	
	pinMode(JOYPAD_INPUT, INPUT_PULLUP);
	pinMode(BUTTON_A_PIN, INPUT_PULLUP);
	attachInterrupt(BUTTON_A_PIN, espeon_request_sd_write, FALLING);
	
	pixels = (fbuffer_t*)calloc(GAMEBOY_HEIGHT * GAMEBOY_WIDTH, sizeof(fbuffer_t));
	
	const uint32_t pal[] = {0xEFFFDE, 0xADD794, 0x525F73, 0x183442}; // Default greenscale palette
	espeon_set_palette(pal);
	
	// fbqueue = xQueueCreate(1, sizeof(fbuffer_t*));
	// xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 0);
}

void espeon_update(void)
{
	if(!((GPIO.in >> JOYPAD_INPUT) & 0x1)) {
		Wire.requestFrom(JOYPAD_ADDR, 1);
		if (Wire.available()) {
			uint8_t btns = Wire.read();
			btn_faces = (btns >> 4);
			btn_directions = (GETBIT(btns, 1) << 3) | (GETBIT(btns, 0) << 2) | (GETBIT(btns, 2) << 1) | (GETBIT(btns, 3));
			if (!btn_faces || !btn_directions)
				interrupt(INTR_JOYPAD);
		}
	}
}

void espeon_faint(const char* msg)
{
	M5.Lcd.clear();
	M5.Lcd.setCursor(2, 2);
	M5.Lcd.printf("Espeon fainted!\n%s", msg);
	while(true);
}

fbuffer_t* espeon_get_framebuffer(void)
{
	return pixels;
}

void espeon_clear_framebuffer(fbuffer_t col)
{
	memset(pixels, col, sizeof(pixels));
}

void espeon_clear_screen(uint16_t col)
{
	M5.Lcd.fillScreen(col);
}

void espeon_set_palette(const uint32_t* col)
{
	/* RGB888 -> RGB565 */
	for (int i = 0; i < 4; ++i) {
		palette[i] = ((col[i]&0xFF)>>3)+((((col[i]>>8)&0xFF)>>2)<<5)+((((col[i]>>16)&0xFF)>>3)<<11);
	}
}

void espeon_end_frame(void)
{
	if (spi_lock) {
		const s_rominfo* rominfo = rom_get_info();
		if (rominfo->has_battery && rom_get_ram_size())
			espeon_save_sram(mbc_get_ram(), rom_get_ram_size());
		spi_lock = 0;
	}
	//xQueueSend(fbqueue, &pixels, 0);
	M5.Lcd.drawBitmap(CENTER_X, CENTER_Y, GAMEBOY_WIDTH, GAMEBOY_HEIGHT, pixels);
}

void espeon_save_sram(uint8_t* ram, uint32_t size)
{
	if (!ram) return;
	
	static char path[20];
	sprintf(path, "/%.8s.bin", rom_get_title());
	File sram = SD.open(path, FILE_WRITE);
	if (sram) {
		sram.seek(0);
		sram.write(ram, size);
		sram.close();
	}
}

void espeon_load_sram(uint8_t* ram, uint32_t size)
{
	if (!ram) return;
	
	static char path[20];
	sprintf(path, "/%.8s.bin", rom_get_title());
	File sram = SD.open(path, FILE_READ);
	if (sram) {
		sram.seek(0);
		sram.read(ram, size);
		sram.close();
	}
}

static inline const uint8_t* espeon_get_last_rom(const esp_partition_t* part)
{
	spi_flash_mmap_handle_t hrom;
	const uint8_t* romdata;
	esp_err_t err;
	err = esp_partition_mmap(part, 0, MAX_ROM_SIZE, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err != ESP_OK)
		return nullptr;
	return romdata;
}

const uint8_t* espeon_load_rom(const char* path)
{
	const esp_partition_t* part;
	part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, PARTITION_ROM, NULL);
	if (!part)
		return nullptr;
	
	if (!path)
		return espeon_get_last_rom(part);
	
	File romfile = SD.open(path, FILE_READ);
	if (!romfile)
		return nullptr;
	
	esp_err_t err;
	err = esp_partition_erase_range(part, 0, MAX_ROM_SIZE);
	if (err != ESP_OK)
		return nullptr;
	
	const size_t bufsize = 32 * 1024;
	size_t romsize = romfile.size();
	if (romsize > MAX_ROM_SIZE)
		return nullptr;
	
	uint8_t* rombuf = (uint8_t*)calloc(bufsize, 1);
	if (!rombuf)
		return nullptr;
	
	M5.Lcd.clear();
	M5.Lcd.setTextColor(TFT_WHITE);
	M5.Lcd.drawString("Flashing ROM...", 0, 0);
	size_t offset = 0;
	while(romfile.available()) {
		romfile.read(rombuf, bufsize);
		esp_partition_write(part, offset, (const void*)rombuf, bufsize);
		offset += bufsize;
		M5.Lcd.progressBar(50, 100, 200, 40, (offset*100)/romsize);
	}
	M5.Lcd.clear();
	free(rombuf);
	romfile.close();
	
	return espeon_get_last_rom(part);
}
