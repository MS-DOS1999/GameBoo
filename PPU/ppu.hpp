#ifndef VDP_HPP
#define VDP_HPP

#include "../BITS/bitsUtils.hpp"
#include "../GameBoy/emu.hpp"
#include "../CPU/sharpLr.hpp"

typedef enum
{
	WHITE,
    LIGHT_GREY,
    DARK_GREY,
    BLACK
}Color;

typedef struct
{
	sfVector2i position;
	byte color;
}Pixel;

class Ppu
{
public:
	//method
	Ppu();
	static void Update(int cycles);
	static void RenderScreen(sfImage* screenImg, sfTexture* screenTex, sfSprite* screenSpr, sfRenderWindow* window);

private:

	Pixel screenData[160][144];
	sfColor pixelColor[4];

	int scanlineCounter;
	byte lastDrawnScanline;
	byte tileScanline[160];


	static Ppu& get(void);

	//method
	void SetLcdStatus();
	bool IsLcdEnabled();
	void DrawScanline(byte scanline);
	void RenderTiles(byte control, byte scanline);
	Color GetColor(byte colorNum, word address);
	void RenderSprites(byte control, byte scanline);
};


#endif