#include <vector>
#include <M5Stack.h>
#include <M5TreeView.h>
#include <MenuItemSD.h>

#include "menu.h"

M5TreeView treeView;

typedef std::vector<MenuItem*> vmi;

static String rompath = "";
static const char* rompath_c = nullptr;
static bool running;

void cb_menu_newrom(MenuItem* sender)
{
	if (rompath != "") {
		rompath_c = rompath.c_str();
		running = false;
	}
}

void cb_menu_loadrom(MenuItem* sender)
{
	MenuItemFS* mi = static_cast<MenuItemFS*>(sender);
	if (!mi) return;

	if (mi->isDir) return;

	int idx = mi->path.lastIndexOf('.') + 1;
	String ext = mi->path.substring(idx);
	if (ext == "gb") {
		rompath = mi->path;
		M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 10, 0);
		M5.Lcd.drawString(rompath, 0, 0);
		cb_menu_newrom(sender);
		//treeView.begin();
	}
}

void cb_menu_lastrom(MenuItem* sender)
{
	rompath_c = nullptr;
	running = false;
}

const char* menu_get_rompath()
{
	return rompath_c;
}

void menu_init()
{
	treeView.clientRect.x = 2;
	treeView.clientRect.y = 16;
	treeView.clientRect.w = 316;
	treeView.clientRect.h = 200;
	treeView.setTextFont(2);
	treeView.itemHeight   = 18;
	treeView.itemWidth    = 220;

	treeView.useFACES       = true;
	treeView.useJoyStick    = true;
	treeView.usePLUSEncoder = true;
	treeView.useFACESEncoder= true;

	treeView.setItems(vmi
		{
			new MenuItem("Last ROM (Flash)", cb_menu_lastrom),
			new MenuItemSD("Load ROM (SD Card)", cb_menu_loadrom),
			//new MenuItem("Start", cb_menu_newrom),
			new MenuItem("Settings (TODO)")
		}
	);

	running = true;
	treeView.begin();
}

void menu_loop()
{
	M5.Lcd.drawString("Espeon v1.0", 0, 0);
	while(running) {
		treeView.update();
	}
}
