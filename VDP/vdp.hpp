#ifndef VDP_HPP
#define VDP_HPP

#include "../BITS/bitsUtils.hpp"
#include "../GameBoy/emu.hpp"

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

class Vdp
{
public:
	//method
	Vdp();
	static void Update(int cycles);
	static void RenderScreen(sfImage* screenImg, sfTexture* screenTex, sfSprite* screenSpr, sfRenderWindow* window);

private:

	Pixel screenData[160][144];
	sfColor pixelColor[4];

	int scanlineCounter;

	static Vdp& get(void);

	//method
	void SetLcdStatus();
	bool IsLcdEnabled();
	void DrawScanline();
	void RenderTiles(byte control);
	Color GetColor(byte colorNum, word address);
	void RenderSprites(byte control);
};


#endif