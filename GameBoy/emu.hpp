#ifndef EMU_HPP
#define EMU_HPP

#define MAX_LOOP 69905

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <windows.h>
#include <SFML/Config.h>
#include <SFML/System.h>
#include <SFML/Window.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>


#include "../BITS/bitsUtils.hpp"
#include "../CPU/sharpLr.hpp"
#include "../PPU/ppu.hpp"
#include "../APU/gb_apu/Gb_Apu.h"
#include "../APU/gb_apu/Multi_Buffer.h"
#include "../APU/Sound_Queue.h"

class Emu
{
public:
	//method
	Emu();
	static unsigned int GetPixelSize();
	static char GetLcdColor();
	static void Intro();
	static void PlayMusic();
	static void GetConfig();
	static void HideCMD();
	static void FileBrowser();
	static void StopMusic();
	static void Execute();
	static void WriteDirectMemory(word address, byte data);
	static byte ReadDirectMemory(word address);
	static void WriteMemory(word address, byte data);
	static byte ReadMemory(word address);

private:

	//config
	unsigned int screenConfig;
	char lcdColor;

	//sfml var
	sfRenderWindow* window;
	sfClock* myClock;
	sfEvent event;
	sfVideoMode mode;

	sfImage* screenImg;
    sfTexture* screenTex;
    sfSprite* screenSpr;
    sfSprite* gameboySpr;
    sfSprite* pUp;
    sfSprite* pDown;
    sfSprite* pRight;
    sfSprite* pLeft;
    sfSprite* pSelect;
    sfSprite* pStart;
    sfSprite* pAButton;
    sfSprite* pBButton;

    sfMusic* kidZan;

    unsigned int pixelSize;

    std::string fullPath;

	//var
    bool bpUp;
    bool bpDown;
    bool bpRight;
    bool bpLeft;
    bool bpSelect;
    bool bpStart;
    bool bpAButton;
    bool bpBButton;

	int loopCounter;
	int cyclesTime;
	bool quit;
	int total;
	int timer;
	int current;
	int counter;
	bool first;
	char romName[200];
	char saveName[200];
	bool romLoaded;

	byte joypadState;

	bool activeRam;
	bool activeBattery;
	bool NOROM;
	bool MBC1;
	bool MBC2;
	bool MBC3;
	bool MBC5;

	byte cartridgeMemory[0x200000];
	byte internalMemory[0x10000];
	byte ramBanks[0x20000];
	bool romBanking;

	byte currentRomBank;
	byte currentRamBank;

	bool enableRam;

	//MBC3 var
	bool rtc;
	bool rtcEnabled;
	bool rtcLatched;
	byte rtcLatch1;
	byte rtcLatch2;
	byte rtcReg[5];
	byte latchReg[5];

	//apu var
	Gb_Apu apu;
	Stereo_Buffer buf;
	blip_sample_t out_buf[4096];
	Sound_Queue sound;

	static Emu& get(void);

	//method
	void InitSFML();
	void LoadGame();
	void GetRomInfo();
	void GetSave();
	void WriteSave();
	void KeyPressed(int key);
	void KeyReleased(int key);
	byte GetJoypadState();
	void HandleInput();
	void FPSChecking();
	void Run();
	void Update();
	void RenderGameboy();

	void WriteMapper(word address, byte data);
	byte ReadMapper(word address);
	void DoDMATransfer(byte data);
	void WriteNOROM(word address, byte data);
	byte ReadNOROM(word address);
	void WriteMBC1(word address, byte data);
	byte ReadMBC1(word address);
	void WriteMBC2(word address, byte data);
	byte ReadMBC2(word address);
	void GrabTime();
	void WriteMBC3(word address, byte data);
	byte ReadMBC3(word address);
	void WriteMBC5(word address, byte data);
	byte ReadMBC5(word address);
};


#endif