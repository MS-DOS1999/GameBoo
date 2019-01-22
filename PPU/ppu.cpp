#include "ppu.hpp"

Ppu::Ppu()
{
	this->scanlineCounter = 456;

    if(Emu::GetLcdColor() == 0)
    {
        this->pixelColor[0].r = 155;
        this->pixelColor[0].g = 188;
        this->pixelColor[0].b = 15;
        this->pixelColor[0].a = 0xFF;

        this->pixelColor[1].r = 139;
        this->pixelColor[1].g = 172;
        this->pixelColor[1].b = 15;
        this->pixelColor[1].a = 0xFF;

        this->pixelColor[2].r = 48;
        this->pixelColor[2].g = 98;
        this->pixelColor[2].b = 48;
        this->pixelColor[2].a = 0xFF;

        this->pixelColor[3].r = 15;
        this->pixelColor[3].g = 56;
        this->pixelColor[3].b = 15;
        this->pixelColor[3].a = 0xFF;
    }
    else if(Emu::GetLcdColor() == 1)
    {
        this->pixelColor[0].r = 0xFF;
        this->pixelColor[0].g = 0xFF;
        this->pixelColor[0].b = 0xFF;
        this->pixelColor[0].a = 0xFF;

        this->pixelColor[1].r = 0xCC;
        this->pixelColor[1].g = 0xCC;
        this->pixelColor[1].b = 0xCC;
        this->pixelColor[1].a = 0xFF;

        this->pixelColor[2].r = 0x77;
        this->pixelColor[2].g = 0x77;
        this->pixelColor[2].b = 0x77;
        this->pixelColor[2].a = 0xFF;

        this->pixelColor[3].r = 0x00;
        this->pixelColor[3].g = 0x00;
        this->pixelColor[3].b = 0x00;
        this->pixelColor[3].a = 0xFF;
    }
    else if(Emu::GetLcdColor() == 2)
    {
        this->pixelColor[0].r = 0xE0;
        this->pixelColor[0].g = 0xF8;
        this->pixelColor[0].b = 0xD0;
        this->pixelColor[0].a = 0xFF;

        this->pixelColor[1].r = 0x88;
        this->pixelColor[1].g = 0xC0;
        this->pixelColor[1].b = 0x70;
        this->pixelColor[1].a = 0xFF;

        this->pixelColor[2].r = 0x34;
        this->pixelColor[2].g = 0x68;
        this->pixelColor[2].b = 0x56;
        this->pixelColor[2].a = 0xFF;

        this->pixelColor[3].r = 0x08;
        this->pixelColor[3].g = 0x18;
        this->pixelColor[3].b = 0x20;
        this->pixelColor[3].a = 0xFF;
    }
	
    byte x = 0;
    byte y = 0;

    for(x = 0; x < 160; x++)
    {
        for(y = 0; y < 144; y++)
        {
            this->screenData[x][y].position.x = x;
            this->screenData[x][y].position.y = y;
            this->screenData[x][y].color = BLACK;
        }
    }

    for(byte z = 0; z < 160; z++)
    {
        this->tileScanline[z] = 3;
    }
}

Ppu& Ppu::get(void)
{
	static Ppu instance;
	return instance;
}

void Ppu::Update(int cycles)
{
	get().SetLcdStatus();

	if(get().IsLcdEnabled())
	{
		get().scanlineCounter -= cycles;
	}
	else
	{
		return;
	}

	if(get().scanlineCounter <= 0)
	{
        Emu::WriteDirectMemory(0xFF44, Emu::ReadDirectMemory(0xFF44) + 1);

		if(Emu::ReadDirectMemory(0xFF44) > 153)
		{
			Emu::WriteDirectMemory(0xFF44, 0);
        }

        byte currentline = Emu::ReadMemory(0xFF44);

        get().scanlineCounter += 456;

        if(currentline == 144)
        {
            SharpLr::RequestInterrupt(0);
        }

	}
}

void Ppu::SetLcdStatus()
{
	byte status = Emu::ReadMemory(0xFF41);

	if(!get().IsLcdEnabled())
	{
		get().scanlineCounter = 456;
		Emu::WriteDirectMemory(0xFF44, 0);
		status &= 0xFC;
		status |= 0x00;
		Emu::WriteMemory(0xFF41, status);
		return;
	}

	byte currentline = Emu::ReadMemory(0xFF44);
	byte currentmode = status & 0x3;

	int mode = 0;
	bool reqInt = false;

	if(currentline >= 144)
	{
        mode = 1;
        status &= 0xFC;
		status |= 0x01;
        reqInt = TestBit(status, 4);
    }
    else
    {
        int mode2bound = 456-80;
        int mode3bound = mode2bound - 172;
        if(get().scanlineCounter >= mode2bound)
        {
            if(currentline != get().lastDrawnScanline)
            {
                get().DrawScanline(lastDrawnScanline);
                lastDrawnScanline = currentline;
            }
            mode = 2;
            status &= 0xFC;
			status |= 0x02;
            reqInt = TestBit(status, 5);
        }
        else if(get().scanlineCounter >= mode3bound)
        {
            mode = 3;
            status &= 0xFC;
			status |= 0x03;
        }
        else
        {
            mode = 0;
            status &= 0xFC;
			status |= 0x00;
            reqInt = TestBit(status, 3);
        }
    }

    if(reqInt && (mode != currentmode))
    {
    	SharpLr::RequestInterrupt(1);
    }

    

    if(Emu::ReadMemory(0xFF44) == Emu::ReadMemory(0xFF45))
    {
		status |= 0x04;

        if(TestBit(status, 6))
        {
            SharpLr::RequestInterrupt(1);
        }
    }
    else
    {
        status &= 0xFB;
    }

    Emu::WriteMemory(0xFF41, status);
}

bool Ppu::IsLcdEnabled()
{
	return TestBit(Emu::ReadMemory(0xFF40), 7);
}

void Ppu::DrawScanline(byte scanline)
{
	byte control = Emu::ReadMemory(0xFF40);

    if(TestBit(control, 0))
    {
        get().RenderTiles(control, scanline);
    }

    if(TestBit(control, 1))
    {
        get().RenderSprites(control, scanline);
    }
}

void Ppu::RenderTiles(byte control, byte scanline)
{
	word tileData = 0;
    word backgroundMemory = 0;
    bool unsig = true;

    byte scrollY = Emu::ReadMemory(0xFF42);
    byte scrollX = Emu::ReadMemory(0xFF43);

    byte windowY = Emu::ReadMemory(0xFF4A);
    byte windowX = Emu::ReadMemory(0xFF4B) - 7;

    bool usingWindow = false;

    if(TestBit(control, 5))
    {
        if(windowY <= scanline)
        {
            usingWindow = true;
        }
    }

    if(TestBit(control, 4))
    {
        tileData = 0x8000;
    }
    else
    {
        tileData = 0x8800;
        unsig = false;
    }

    if(usingWindow == false)
    {
        if(TestBit(control, 3))
        {
            backgroundMemory = 0x9C00;
        }
        else
        {
            backgroundMemory = 0x9800;
        }
    }
    else
    {
        if(TestBit(control, 6))
        {
            backgroundMemory = 0x9C00;
        }
        else
        {
            backgroundMemory = 0x9800;
        }
    }

    byte yPos = 0;

    if(!usingWindow)
    {
        yPos = scrollY + scanline;
    }
    else
    {
        yPos = scanline - windowY;
    }

    word tileRow = (((byte)(yPos/8))*32);

    int pixel;

    for(pixel = 0; pixel<160; ++pixel)
    {
        byte xPos = pixel+scrollX;

        if(usingWindow)
        {
            if(pixel >= windowX)
            {
                xPos = pixel - windowX;
            }
        }

        word tileCol = (xPos/8);

        signed_word tileNum;

        word tileAddress = backgroundMemory + tileRow + tileCol;

        if(unsig)
        {
            tileNum = (byte)Emu::ReadMemory(tileAddress);
        }
        else
        {
            tileNum = (signed_byte)Emu::ReadMemory(tileAddress);
        }

        word tileLocation = tileData;

        if(unsig)
        {
            tileLocation += (tileNum * 16);
        }
        else
        {
            tileLocation += ((tileNum+128) * 16);
        }

        byte line = yPos % 8;

        line *= 2;

        byte data1 = Emu::ReadMemory(tileLocation + line);
        byte data2 = Emu::ReadMemory(tileLocation + line + 1);

        int colorBit = xPos % 8;

        colorBit -= 7;

        colorBit *= -1;
		

        int colorNum = BitGetVal(data2, colorBit);

        colorNum <<= 1;

        colorNum |= BitGetVal(data1, colorBit);

        get().tileScanline[pixel] = colorNum;

        Color col = get().GetColor(colorNum, 0xFF47);
		
        if((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
        {
            continue;
        }
		
        switch (col)
        {
            case WHITE:
                get().screenData[pixel][scanline].color = WHITE;
                break;
            case LIGHT_GREY:
                get().screenData[pixel][scanline].color = LIGHT_GREY;
                break;
            case DARK_GREY:
                get().screenData[pixel][scanline].color = DARK_GREY;
                break;
            case BLACK:
                get().screenData[pixel][scanline].color = BLACK;
                break;
        }
    }
}

Color Ppu::GetColor(byte colorNum, word address)
{
	Color res = WHITE;
    byte palette = Emu::ReadMemory(address);

    int hi = 0;
    int lo = 0;

    switch (colorNum)
    {
        case 0:
            hi = 1;
            lo = 0;
            break;
        case 1:
            hi = 3;
            lo = 2;
            break;
        case 2:
            hi = 5;
            lo = 4;
            break;
        case 3:
            hi = 7;
            lo = 6;
            break;
    }

    int color = 0;
    color = BitGetVal(palette, hi) << 1;
    color |= BitGetVal(palette, lo);

    switch(color)
    {
        case 0:
            res = WHITE;
            break;
        case 1:
            res = LIGHT_GREY;
            break;
        case 2:
            res = DARK_GREY;
            break;
        case 3:
            res = BLACK;
            break;
    }

    return res;
}

void Ppu::RenderSprites(byte control, byte scanline)
{
    bool use8x16 = false;

    if(TestBit(control, 2))
    {
        use8x16 = true;
    }

    int sprite;

    for(sprite = 0; sprite < 40; ++sprite)
    {
        byte index = sprite*4;
        byte yPos = Emu::ReadMemory(0xFE00+index) - 16;
        byte xPos = Emu::ReadMemory(0xFE00+index+1) - 8;
        byte tileLocation = Emu::ReadMemory(0xFE00+index+2);
        byte attributes = Emu::ReadMemory(0xFE00+index+3);

        bool yFlip = TestBit(attributes, 6);
        bool xFlip = TestBit(attributes, 5);
        bool priority = TestBit(attributes, 7);

        int ysize = 8;

        if(use8x16)
        {
            ysize = 16;
        }

        if((scanline >= yPos) && (scanline < (yPos+ysize)))
        {
            int line = scanline - yPos;

            if(yFlip)
            {
                line = ysize - line - 1;
            }

            line *= 2;

            word dataAddress = (0x8000 + (tileLocation * 16)) + line;

            byte data1 = Emu::ReadMemory(dataAddress);
            byte data2 = Emu::ReadMemory(dataAddress + 1);

            int tilePixel;

            for(tilePixel = 0; tilePixel < 8; ++tilePixel)
            {
                int colorBit = tilePixel;

                if(xFlip)
                {
                    colorBit -= 7;
                    colorBit *= -1;
                }

                int colorNum = BitGetVal(data2, colorBit);

                colorNum <<= 1;

                colorNum |= BitGetVal(data1, colorBit);

				if(colorNum == 0)
				{
                    continue;
                }
				
                word colorAddressValueTemp = 0;

                if(TestBit(attributes, 4))
                {
                    colorAddressValueTemp = 0xFF49;
                }
                else
                {
                    colorAddressValueTemp = 0xFF48;
                }


                word colorAddress = colorAddressValueTemp;

                Color col = get().GetColor(colorNum, colorAddress);

                int xPix = 7 - tilePixel;

                int pixel = xPos+xPix;
				
                if((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
                {
                    continue;
                }

                if(priority)
                {
                    if(get().tileScanline[pixel] != 0)
                    {
                        continue;
                    }
                }

                switch(col)
                {
                    case WHITE:
                        get().screenData[pixel][scanline].color = WHITE;
                        break;
                    case LIGHT_GREY:
                        get().screenData[pixel][scanline].color = LIGHT_GREY;
                        break;
                    case DARK_GREY:
                        get().screenData[pixel][scanline].color = DARK_GREY;
                        break;
                    case BLACK:
                        get().screenData[pixel][scanline].color = BLACK;
                        break;
                }
            }
        }
    }
}

void Ppu::RenderScreen(sfImage* screenImg, sfTexture* screenTex, sfSprite* screenSpr, sfRenderWindow* window)
{   
    int x;
    int y;
    for(y = 0; y < 144; ++y)
    {
        for(x = 0; x < 160; ++x)
        {
            sfColor color;
            color.r = get().pixelColor[get().screenData[x][y].color].r;
            color.g = get().pixelColor[get().screenData[x][y].color].g;
            color.b = get().pixelColor[get().screenData[x][y].color].b;
            color.a = get().pixelColor[get().screenData[x][y].color].a;

            byte ix;
            byte iy;
            unsigned int pixelSize = Emu::GetPixelSize();
            for(ix = 0; ix < pixelSize; ++ix)
            {
                for(iy = 0; iy < pixelSize; ++iy)
                {
                    sfImage_setPixel(screenImg, (get().screenData[x][y].position.x * pixelSize) + ix, (get().screenData[x][y].position.y * pixelSize) + iy, color);
                }
            }
        }
    }

    sfTexture_updateFromImage(screenTex, screenImg, 0, 0);
    sfSprite_setTexture(screenSpr, screenTex, sfTrue);      
}