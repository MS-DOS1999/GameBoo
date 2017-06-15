#include "z80.h"
#include "BiosDMG.h"
#include "bitUtils.c"

//INIT//
void initZ80(){


	//memcpy(&z80.m_Rom[0x0], &BiosDMG[0], 0x100);

    z80.m_ProgramCounter = 0x100 ;
    z80.m_RegisterAF.reg = 0x01B0 ;
    z80.m_RegisterBC.reg = 0x0013 ;
    z80.m_RegisterDE.reg = 0x00D8 ;
    z80.m_RegisterHL.reg = 0x014D ;
    z80.m_StackPointer.reg = 0xFFFE ;
    z80.m_Rom[0xFF05] = 0x00 ;
    z80.m_Rom[0xFF06] = 0x00 ;
    z80.m_Rom[0xFF07] = 0x00 ;
    z80.m_Rom[0xFF10] = 0x80 ;
    z80.m_Rom[0xFF11] = 0xBF ;
    z80.m_Rom[0xFF12] = 0xF3 ;
    z80.m_Rom[0xFF14] = 0xBF ;
    z80.m_Rom[0xFF16] = 0x3F ;
    z80.m_Rom[0xFF17] = 0x00 ;
    z80.m_Rom[0xFF19] = 0xBF ;
    z80.m_Rom[0xFF1A] = 0x7F ;
    z80.m_Rom[0xFF1B] = 0xFF ;
    z80.m_Rom[0xFF1C] = 0x9F ;
    z80.m_Rom[0xFF1E] = 0xBF ;
    z80.m_Rom[0xFF20] = 0xFF ;
    z80.m_Rom[0xFF21] = 0x00 ;
    z80.m_Rom[0xFF22] = 0x00 ;
    z80.m_Rom[0xFF23] = 0xBF ;
    z80.m_Rom[0xFF24] = 0x77 ;
    z80.m_Rom[0xFF25] = 0xF3 ;
    z80.m_Rom[0xFF26] = 0xF1 ;
    z80.m_Rom[0xFF40] = 0x91 ;
    z80.m_Rom[0xFF42] = 0x00 ;
    z80.m_Rom[0xFF43] = 0x00 ;
    z80.m_Rom[0xFF45] = 0x00 ;
    z80.m_Rom[0xFF47] = 0xFC ;
    z80.m_Rom[0xFF48] = 0xFF ;
    z80.m_Rom[0xFF49] = 0xFF ;
    z80.m_Rom[0xFF4A] = 0x00 ;
    z80.m_Rom[0xFF4B] = 0x00 ;
	z80.m_Rom[0xFF0F] = 0xE1 ;
    z80.m_Rom[0xFFFF] = 0x00 ;

    z80.m_CurrentROMBank = 1;
    z80.m_CurrentRAMBank = 0;
    z80.m_TimerCounter = 0;
	z80.m_TimerSpeed = 1024;
    z80.m_InteruptMaster = false;
    z80.m_DividerRegister = 0;
    z80.m_ScanlineCounter = 0;
	z80.m_InteruptDelay = false;
	z80.m_JoypadState = 0xFF;
	z80.m_Halted = false;
	z80.m_SkipInstruction = false;
}

void DetectMapper(){

    z80.m_MBC1 = false;
    z80.m_MBC2 = false;

    switch (z80.m_CartridgeMemory[0x147]){
		
        case 1 : z80.m_MBC1 = true ; break ;
        case 2 : z80.m_MBC1 = true ; break ;
        case 3 : z80.m_MBC1 = true ; break ;
        case 5 : z80.m_MBC2 = true ; break ;
        case 6 : z80.m_MBC2 = true ; break ;
        default : break ;
    }
	
	int numRamBanks = 0;
	switch (ReadMemory(0x149)){
		case 0: numRamBanks = 0 ;break ;
		case 1: numRamBanks = 1 ;break ;
		case 2: numRamBanks = 1 ;break ;
		case 3: numRamBanks = 4 ;break ;
		case 4: numRamBanks = 16 ;break ;
	}

	CreateRamBanks(numRamBanks);
}

void CreateRamBanks(int numBanks){
	for (int i = 0; i < 17; i++){
		byte ram[0x2000];
		memset(ram, 0, sizeof(ram)) ;
		z80.m_RAMBanks.push_back(ram) ;
	}

	for (int i = 0 ; i < 0x2000; i++){
		z80.m_RAMBanks[0][i] = z80.m_Rom[0xA000+i];
	}
}

//INIT//


//JOYPAD//
void KeyPressed(int key)
{
   bool previouslyUnset = false ; 

   if (TestBit8(z80.m_JoypadState, key)==false){
     previouslyUnset = true ;
   }

   z80.m_JoypadState = BitReset8(z80.m_JoypadState, key);

   bool button = true ;

   if (key > 3){ 
		button = true ;
   }
   else{ 
		button = false ;
   }
   
   byte keyReq = z80.m_Rom[0xFF00] ;
   bool requestInterupt = false ; 

   if (button && !TestBit8(keyReq,5)){
		requestInterupt = true ;
   }
   else if (!button && !TestBit8(keyReq,4)){
		requestInterupt = true ;
   }

   if (requestInterupt && !previouslyUnset){
		z80.m_Rom[0xFF0F] |= 0x10;
   }
}

void KeyReleased(int key){
   z80.m_JoypadState = BitSet8(z80.m_JoypadState,key);
}

byte GetJoypadState(){
   byte res = z80.m_Rom[0xFF00] ;
   
   res ^= 0xFF ;

   if (!TestBit8(res, 4)){
     byte topJoypad = z80.m_JoypadState >> 4 ;
     topJoypad |= 0xF0 ; 
     res &= topJoypad ; 
   }
   else if (!TestBit8(res,5)){
     byte bottomJoypad = z80.m_JoypadState & 0xF ;
     bottomJoypad |= 0xF0 ; 
     res &= bottomJoypad ; 
   }
   return res ;
}
//JOYPAD//


//MEMORY READ/WRITE//


void PushWordOntoStack(word wOrd){

    byte hi = wOrd >> 8 ;

    byte lo = wOrd & 0xFF;

    z80.m_StackPointer.reg--;

    WriteMemory(z80.m_StackPointer.reg, hi);

    z80.m_StackPointer.reg--;

    WriteMemory(z80.m_StackPointer.reg, lo);
}

word PopWordOffStack(){
    word wOrd = ReadMemory(z80.m_StackPointer.reg+1) << 8 ;
    wOrd |= ReadMemory(z80.m_StackPointer.reg) ;
    z80.m_StackPointer.reg+=2 ;

    return wOrd ;
}

void WriteMemory(word address, byte data){
	
	if(address < 0x8000){
        HandleBanking(address, data);
    }
	else if((address>=0x8000) && (address <0xA000)){
		
		z80.m_Rom[address] = data;
	}
    else if((address>=0xA000)&&(address<0xC000)){
        if(z80.m_EnableRAM){
			if(z80.m_MBC1){
				word newAddress = address - 0xA000 ;
				z80.m_RAMBanks.at(z80.m_CurrentRAMBank)[newAddress] = data;
			}
        }
    }
	else if ( (address >= 0xC000) && (address <= 0xDFFF) ){
		z80.m_Rom[address] = data ;
	}
	else if ( (address >= 0xE000) && (address <= 0xFDFF) ){
		z80.m_Rom[address] = data ;
		z80.m_Rom[address-0x2000] = data ; // echo data into ram address
	}
 	else if ((address >= 0xFEA0) && (address <= 0xFEFF)){
 	//restricted area
	}
    else if(0xFF04 == address){
        z80.m_Rom[0xFF04] = 0;
    }
	else if (TMC == address){
		byte currentfreq = GetClockFreq();
		z80.m_CartridgeMemory[TMC] = data;
		byte newfreq = GetClockFreq();

		if (currentfreq != newfreq){
			SetClockFreq();
		}
	}
    else if(0xFF44 == address){
        z80.m_Rom[0xFF44] = 0;
    }
	else if (0xFF45 == address){
		z80.m_Rom[address] = data;
	}
    else if(0xFF46 == address){
        DoDMATransfer(data);
    }
	else if ((address >= 0xFF4C) && (address <= 0xFF7F)){
 	//restricted area
	}
	else{
		z80.m_Rom[address] = data ;
	}
}


byte ReadMemory(word address){
    
	if(address < 0x0100){
		return z80.m_Rom[address];
	}
	else if((address>=0x4000) && (address<=0x7FFF)){

        //on enleve la valeur d'une bank pour récuperer la valeur d'origine
        word newAddress = address - 0x4000;
        //pour recuperer les données de la bonne BANK on multiply le numero de la BANK par 0x4000 ce qui nous positionne sur la bonne BANK et on ajoute la nouvelle adresse pour être sur la bonne donnée
        return z80.m_CartridgeMemory[newAddress + ((z80.m_CurrentROMBank)*0x4000)];
    }
    else if((address>=0xA000) && (address<=0xBFFF)){
        word newAddress = address - 0xA000;
        return z80.m_RAMBanks.at(z80.m_CurrentRAMBank)[newAddress];
    }
	else if (0xFF00 == address){
		return GetJoypadState();
	}
	else{
		return z80.m_Rom[address];
	}

}

void HandleBanking(word address, byte data){

    if (address < 0x2000){
        if (z80.m_MBC1 || z80.m_MBC2){
			
            DoRAMBankEnable(address,data);
        }
		else{
			z80.m_Rom[address] = data;
		}
    }
    else if((address>=0x2000) && (address<0x4000)){
        if(z80.m_MBC1 || z80.m_MBC2){
            DoChangeLoROMBank(data);
        }
		else{
			z80.m_Rom[address] = data;
		}
    }
    else if((address>=0x4000) && (address<0x6000)){
        if(z80.m_MBC1){
            if(z80.m_RomBanking){
				
                DoChangeHiROMBank(data);
            }
            else{
				
                DoRAMBankChange(data);
            }
        }
		else{
			z80.m_Rom[address] = data;
		}
    }
    else if((address>=0x6000) && (address<0x8000)){
        if(z80.m_MBC1){
            DoChangeROMRAMMode(data);
        }
		else{
			z80.m_Rom[address] = data;
		}
    }

}

void DoRAMBankEnable(word address, byte data){
	
		if (z80.m_MBC1){
            if ((data & 0xF) == 0xA){
                z80.m_EnableRAM = true ;
			}
            else if (data == 0x0){
                z80.m_EnableRAM = false ;
			}
	    }
	    else if (z80.m_MBC2){
			if (false == TestBit16(address,8)){
				if ((data & 0xF) == 0xA){
					z80.m_EnableRAM = true ;
				}
				else if (data == 0x0){
					z80.m_EnableRAM = false ;
				}
			}
	    }
}

void DoChangeLoROMBank(byte data){
		if (z80.m_MBC1){
			if (data == 0x00){
				data++;
			}

			data &= 31;

			z80.m_CurrentROMBank &= 224;

			z80.m_CurrentROMBank |= data;

		}
		else if (z80.m_MBC2){
            data &= 0xF ;
            z80.m_CurrentROMBank = data ;
		}
}

void DoChangeHiROMBank(byte data){
    
	z80.m_CurrentRAMBank = 0;
	
	data &= 3;
	data <<=5;
	
	if((z80.m_CurrentROMBank & 31) == 0){
		data++;
	}
	
	z80.m_CurrentROMBank &= 31;
	
	z80.m_CurrentROMBank |= data;

}

void DoRAMBankChange(byte data){
    z80.m_CurrentRAMBank = data & 0x3;
}

void DoChangeROMRAMMode(byte data){
    byte newData = data & 0x1;
    if(newData == 0){
        z80.m_RomBanking = true;
    }
    else{
        z80.m_RomBanking = false;
    }

    if(!z80.m_RomBanking){
        z80.m_CurrentRAMBank = 0;
    }
	
}

void WriteU16(word address, word data){
	WriteMemory(address, (data & 0xFF));
	WriteMemory((address + 1), (data >> 8));
}
//MEMORY READ/WRITE//


//TIMERS//

void UpdateTimers(int cycles){

	DoDividerRegister(cycles);

    if(IsClockEnabled()){
        z80.m_TimerCounter += cycles;

        SetClockFreq();
		
		if(z80.m_TimerCounter >= z80.m_TimerSpeed){
			z80.m_Rom[0xFF05]++;
			z80.m_TimerCounter -= z80.m_TimerSpeed;
			
			if(z80.m_Rom[0xFF05] == 0){
				z80.m_Rom[0xFF0F] |= 0x04;
				z80.m_Rom[0xFF05] = z80.m_Rom[0xFF06];
			}
		}
    }
	
}

bool IsClockEnabled(){
    return ReadMemory(TMC) & 0x4;
}

byte GetClockFreq(){
    return ReadMemory(TMC) & 0x3;
}

void SetClockFreq(){
    byte freq = GetClockFreq();

    switch(freq){
        case 0:
            z80.m_TimerSpeed = 1024;
            break;
        case 1:
            z80.m_TimerSpeed = 16;
            break;
        case 2:
            z80.m_TimerSpeed = 64;
            break;
        case 3:
            z80.m_TimerSpeed = 256;
            break;
        default:
            printf("frequency has not been changed");
            break;
    }
}

void DoDividerRegister(int cycles){
    z80.m_DividerRegister += cycles;
    if(z80.m_DividerRegister >= 256){
        z80.m_DividerRegister -= 256;
        z80.m_Rom[0xFF04]++;
    }
}
//TIMERS//


//INTERUPTS//

void DoInterupts(){
	if(z80.m_InteruptDelay){
		z80.m_InteruptDelay = false;
		z80.m_InteruptMaster = true;
	}
    else if(z80.m_InteruptMaster){
        //Vblank Interupt
		if((z80.m_Rom[0xFFFF] & 0x1) && (z80.m_Rom[0xFF0F] & 0x1)){
			printf("Vblank int");
			z80.m_InteruptMaster = false;
			z80.m_Halted = false;
			z80.m_Rom[0xFF0F] &= ~0x01;
			z80.m_StackPointer.reg -= 2;
			WriteU16(z80.m_StackPointer.reg, z80.m_ProgramCounter);
			z80.m_ProgramCounter = 0x40;
			loopCounter += 36;
		}
		//LCD Status Interupt
		if((z80.m_Rom[0xFFFF] & 0x02) && (z80.m_Rom[0xFF0F] & 0x02)){
			printf("LCD int");
			z80.m_InteruptMaster = false;
			z80.m_Halted = false;
			z80.m_Rom[0xFF0F] &= ~0x02;
			z80.m_StackPointer.reg -= 2;
			WriteU16(z80.m_StackPointer.reg, z80.m_ProgramCounter);
			z80.m_ProgramCounter = 0x48;
			loopCounter += 36;
		}
		//Timer Overflow Interupt
		if((z80.m_Rom[0xFFFF] & 0x04) && (z80.m_Rom[0xFF0F] & 0x04)){
			printf("Timer int");
			z80.m_InteruptMaster = false;
			z80.m_Halted = false;
			z80.m_Rom[0xFF0F] &= ~0x04;
			z80.m_StackPointer.reg -= 2;
			WriteU16(z80.m_StackPointer.reg, z80.m_ProgramCounter);
			z80.m_ProgramCounter = 0x50;
			loopCounter += 36;
		}
		//Joypad Interupt
		if((z80.m_Rom[0xFFFF] & 0x10) && (z80.m_Rom[0xFF0F] & 0x10)){
			z80.m_InteruptMaster = false;
			z80.m_Halted = false;
			z80.m_Rom[0xFF0F] &= ~0x10;
			z80.m_StackPointer.reg -= 2;
			WriteU16(z80.m_StackPointer.reg, z80.m_ProgramCounter);
			z80.m_ProgramCounter = 0x60;
			loopCounter += 36;
		}
    }
	else if((z80.m_Rom[0xFF0F] & z80.m_Rom[0xFFFF] & 0x1F) && (!z80.m_SkipInstruction)){
		z80.m_Halted = false;
	}
}

//INTERUPTS//


//LCD//

void UpdateGraphics(int cycles){
    SetLCDStatus();

    if(IsLCDEnabled()){
		//printf("lcd enabled");
        z80.m_ScanlineCounter -= cycles;
    }
    else{
        return;
    }

    if(z80.m_ScanlineCounter <= 0){
        z80.m_Rom[0xFF44]++;
        byte currentline = ReadMemory(0xFF44);
        z80.m_ScanlineCounter = 456;
		//printf("current line: %d", currentline);
        if(currentline == 144){
            z80.m_Rom[0xFF0F] |= 0x01;
        }
        else if(currentline > 153){
            z80.m_Rom[0xFF44] = 0;
        }
        else if(currentline < 144){
            DrawScanLine();
        }

    }
}

void SetLCDStatus(){
    byte status = ReadMemory(0xFF41);
    if(IsLCDEnabled() == false){
        z80.m_ScanlineCounter = 456;
        z80.m_Rom[0xFF44] = 0;
        status &= 252;
        status = BitSet8(status, 0);
        WriteMemory(0xFF41, status);
        return;
    }

    byte currentline = ReadMemory(0xFF44);
    byte currentmode = status & 0x3;
    int mode = 0;
    bool reqInt = false;

    if(currentline >= 144){
        mode = 1;
        status = BitSet8(status, 0);
        status = BitReset8(status, 1);
        reqInt = TestBit8(status, 4);
    }
    else{
        int mode2bound = 456-80;
        int mode3bound = mode2bound - 172;
        if(z80.m_ScanlineCounter >= mode2bound){
            mode = 2;
            status = BitSet8(status, 1);
            status = BitReset8(status, 0);
            reqInt = TestBit8(status, 5);
        }
        else if(z80.m_ScanlineCounter >= mode3bound){
            mode = 3;
            status = BitSet8(status, 1);
            status = BitSet8(status, 0);

        }
        else{
            mode = 0;
            status = BitReset8(status, 1);
            status = BitReset8(status, 0);
            reqInt = TestBit8(status, 3);
        }
    }

    if(reqInt && (mode != currentmode)){
        z80.m_Rom[0xFF0F] |= 0x02;
    }

    byte ly = ReadMemory(0xFF44);

    if(ly == ReadMemory(0xFF45)){
        status = BitSet8(status, 2);
        if(TestBit8(status, 6)){
            z80.m_Rom[0xFF0F] |= 0x02;
        }
    }
    else{
        status = BitReset8(status, 2);
    }
    WriteMemory(0xFF41, status);

}

bool IsLCDEnabled(){
    return TestBit8(ReadMemory(0xFF40),7);
}

//LCD//


//DMA//

void DoDMATransfer(byte data){
    word address = data << 8;
    int i;
    for(i = 0; i < 0xA0; i++){
        WriteMemory(0xFE00+i, ReadMemory(address+i));
    }
}

//DMA//

//GRAPHICS//

void DrawScanLine(){
    byte control = ReadMemory(0xFF40);
    if(TestBit8(control, 0)){
        RenderTiles(control);
    }
	else{
		//printf("no background");
	}
    if(TestBit8(control, 1)){
        RenderSprites(control);
    }
	else{
		//printf("no sprite");
	}
	
}

void RenderTiles(byte control){
	
    word tileData = 0;
    word backgroundMemory = 0;
    bool unsig = true;

    byte scrollY = ReadMemory(0xFF42);
    byte scrollX = ReadMemory(0xFF43);

    byte windowY = ReadMemory(0xFF4A);
    byte windowX = ReadMemory(0xFF4B) - 7;

    bool usingWindow = false;

    if(TestBit8(control, 5)){
        if(windowY <= ReadMemory(0xFF44)){
            usingWindow = true;
        }
    }

    if(TestBit8(control, 4)){
        tileData = 0x8000;
    }
    else{
        tileData = 0x8800;
        unsig = false;
    }

    if(usingWindow == false){
        if(TestBit8(control, 3)){
            backgroundMemory = 0x9C00;
        }
        else{
            backgroundMemory = 0x9800;
        }
    }
    else{
        if(TestBit8(control, 6)){
            backgroundMemory = 0x9C00;
        }
        else{
            backgroundMemory = 0x9800;
        }
    }

    byte yPos = 0;

    if(!usingWindow){
        yPos = scrollY + ReadMemory(0xFF44);
    }
    else{
        yPos = ReadMemory(0xFF44) - windowY;
    }

    word tileRow = (((byte)(yPos/8))*32);

    int pixel;

    for(pixel = 0; pixel<160; pixel++){

        byte xPos = pixel+scrollX;

        if(usingWindow){
            if(pixel >= windowX){
                xPos = pixel - windowX;
            }
        }

        word tileCol = (xPos/8);

        signed_word tileNum;

        word tileAddress = backgroundMemory+tileRow+tileCol;

        if(unsig){
            tileNum = (byte)ReadMemory(tileAddress);
        }
        else{
            tileNum = (signed_byte)ReadMemory(tileAddress);
        }

        word tileLocation = tileData;

        if(unsig){
            tileLocation += (tileNum * 16);
        }
        else{
            tileLocation += ((tileNum+128) * 16);
        }

        byte line = yPos % 8;

        line *= 2;

        byte data1 = ReadMemory(tileLocation + line);
        byte data2 = ReadMemory(tileLocation + line + 1);

        int colorBit = xPos % 8;

        colorBit -= 7;

        colorBit *= -1;
		

        int colorNum = BitGetVal8(data2, colorBit);

        colorNum <<= 1;

        colorNum |= BitGetVal8(data1, colorBit);

        COLOR col = GetColor(colorNum, 0xFF47);

        int finaly = ReadMemory(0xFF44);
		//printf("scanline : %d", finaly);
        //test overflow//
        if((finaly<0)||(finaly>143)||(pixel<0)||(pixel>159)){
            printf("pixel overflow");
            continue ;
        }
		

        switch (col){
            case WHITE:
                z80.m_ScreenData[pixel][finaly].color = WHITE;
                break;
            case LIGHT_GREY:
                z80.m_ScreenData[pixel][finaly].color = LIGHT_GREY;
                break;
            case DARK_GREY:
                z80.m_ScreenData[pixel][finaly].color = DARK_GREY;
                break;
            case BLACK:
                z80.m_ScreenData[pixel][finaly].color = BLACK;
                break;
        }
		


    }


}

COLOR GetColor(byte colorNum, word address){
    COLOR res = WHITE;
    byte palette = ReadMemory(address);

    int hi = 0;
    int lo = 0;

    switch (colorNum){
        case 0:
            hi = 1;
            lo = 0;
            break;
        case 1:
            hi =3;
            lo =2;
            break;
        case 2:
            hi =5;
            lo =4;
            break;
        case 3:
            hi =7;
            lo =6;
            break;
    }

    int color = 0;
    color = BitGetVal8(palette, hi) << 1;
    color |= BitGetVal8(palette, lo);

    switch(color){
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

void RenderSprites(byte control){
    bool use8x16 = false;
    if(TestBit8(control, 2)){
        use8x16 = true;
    }

    int sprite;

    for(sprite = 0; sprite < 40; sprite++){
        byte index = sprite*4;
        byte yPos = ReadMemory(0xFE00+index) - 16;
        byte xPos = ReadMemory(0xFE00+index+1) - 8;
        byte tileLocation = ReadMemory(0xFE00+index+2);
        byte attributes = ReadMemory(0xFE00+index+3);

        bool yFlip = TestBit8(attributes, 6);
        bool xFlip = TestBit8(attributes, 5);

        int scanline = ReadMemory(0xFF44);

        int ysize = 8;

        if(use8x16){
            ysize = 16;
        }

        if((scanline >= yPos) && (scanline < (yPos+ysize))){
            int line = scanline - yPos;

            if(yFlip){
                line -= ysize;
                line *= -1;
            }

            line *= 2;

            word dataAddress = (0x8000 + (tileLocation * 16)) + line;

            byte data1 = ReadMemory(dataAddress);
            byte data2 = ReadMemory(dataAddress + 1);

            int tilePixel;

            for(tilePixel = 7; tilePixel >= 0; tilePixel--){
                int colorBit = tilePixel;

                if(xFlip){
                    colorBit -= 7;
                    colorBit *= -1;
                }

                int colorNum = BitGetVal8(data2, colorBit);

                colorNum <<= 1;

                colorNum |= BitGetVal8(data1, colorBit);

                word colorAddressValueTemp = 0;

                if(TestBit8(attributes, 4)){
                    colorAddressValueTemp = 0xFF49;
                }
                else{
                    colorAddressValueTemp = 0xFF48;
                }


                word colorAddress = colorAddressValueTemp;

                COLOR col = GetColor(colorNum, colorAddress);

                if(col == WHITE){
                    continue;
                }

                int xPix = 0 - tilePixel;

                xPix += 7;

                int pixel = xPos+xPix;
				
				
                if ((scanline<0)||(scanline>143)||(pixel<0)||(pixel>159)){
                    continue ;
                }


                switch(col){
                    case WHITE:
                        z80.m_ScreenData[pixel][scanline].color = WHITE;
                        break;
                    case LIGHT_GREY:
                        z80.m_ScreenData[pixel][scanline].color = LIGHT_GREY;
                        break;
                    case DARK_GREY:
                        z80.m_ScreenData[pixel][scanline].color = DARK_GREY;
                        break;
                    case BLACK:
                        z80.m_ScreenData[pixel][scanline].color = BLACK;
                        break;

                }
            }
        }
    }
}

//GRAPHICS//

//OPCODE//

void ExecuteNextOpcode(){
    byte opcode = ReadMemory(z80.m_ProgramCounter);
	
	//printf("PC : 0x%08x\n", z80.m_ProgramCounter);
	//rintf("OPCODE : 0x%08x\n", opcode);
	
	if(!z80.m_Halted){
		z80.m_ProgramCounter++;
		ExecuteOpcode(opcode);
	}
	else{
		if(z80.m_InteruptMaster || !z80.m_SkipInstruction){
			loopCounter+=4;
			cyclesTime = 4;
		}
		else if(z80.m_SkipInstruction){
			z80.m_Halted = false;
			z80.m_SkipInstruction = false;
			
			ExecuteOpcode(opcode);
		}
	}
}


void ExecuteOpcode(byte opcode){
	
    switch(opcode){
        case 0x00:{
            cyclesTime = 4;
            loopCounter += 4;break;
		}
        case 0x01:{
            cyclesTime = 12;
            CPU_16BIT_LOAD(z80.m_RegisterBC.reg);break;
		}
        case 0x02:{
            cyclesTime = 8;
            WriteMemory(z80.m_RegisterBC.reg, z80.m_RegisterAF.hi);loopCounter+=8;break;
		}
        case 0x03:{
            cyclesTime = 8;
            CPU_16BIT_INC(z80.m_RegisterBC.reg, 8);break;
		}
        case 0x04:{
            cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterBC.hi, 4);break;
		}
        case 0x05:{
            cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterBC.hi, 4);break;
		}
        case 0x06:{
            cyclesTime = 8;
            CPU_8BIT_LOAD(z80.m_RegisterBC.hi);break;
		}
        case 0x07:{
			cyclesTime = 4;
			loopCounter+=4;
            CPU_RLC(z80.m_RegisterAF.hi); z80.m_RegisterAF.lo &= ~0x80; break;
		}
        case 0x08:{
            cyclesTime = 20;
            word nn = ReadWord();
            z80.m_ProgramCounter+=2;
            WriteMemory(nn, z80.m_StackPointer.lo);
            nn++;
            WriteMemory(nn, z80.m_StackPointer.hi);
            loopCounter += 20;
            break;
		}
        case 0x09:{
            cyclesTime = 8;
            CPU_16BIT_ADD(z80.m_RegisterHL.reg, z80.m_RegisterBC.reg, 8);break;
		}
        case 0x0A:{
            cyclesTime = 8;
            CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi, z80.m_RegisterBC.reg);break;
		}
        case 0x0B:{
            cyclesTime = 8;
            CPU_16BIT_DEC(z80.m_RegisterBC.reg, 8);break;
		}
        case 0x0C:{
            cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterBC.lo,4);break;
		}
        case 0x0D:{
            cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterBC.lo,4);break;
		}
		case 0x0E:{
			cyclesTime = 8;
			CPU_8BIT_LOAD(z80.m_RegisterBC.lo);break;
		}
		case 0x0F:{
			cyclesTime = 8;
			loopCounter += 8;
			CPU_RRC(z80.m_RegisterAF.hi);z80.m_RegisterAF.lo &= ~0x80;break;
		}
		case 0x10:{
			cyclesTime = 4;
			z80.m_ProgramCounter++;
			loopCounter += 4;
			break;
		}
		case 0x11:{
			cyclesTime = 12;
            CPU_16BIT_LOAD(z80.m_RegisterDE.reg);break;
		}
		case 0x12:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterDE.reg, z80.m_RegisterAF.hi);loopCounter+=8;break;
		}
		case 0x13:{
			cyclesTime = 8;
            CPU_16BIT_INC(z80.m_RegisterDE.reg, 8);break;
		}
		case 0x14:{
			cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterDE.hi, 4);break;
		}
		case 0x15:{
			cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterDE.hi, 4);break;
		}
		case 0x16:{
			cyclesTime = 8;
            CPU_8BIT_LOAD(z80.m_RegisterDE.hi);break;
		}
		case 0x17:{
			cyclesTime = 8;
			loopCounter += 8;
			CPU_RL(z80.m_RegisterAF.hi); z80.m_RegisterAF.lo &= ~0x80; break;
		}
		case 0x18:{
			cyclesTime = 8;
			CPU_JUMP_IMMEDIATE(false, 0, false);break;
		}
		case 0x19:{
			cyclesTime = 8;
            CPU_16BIT_ADD(z80.m_RegisterHL.reg, z80.m_RegisterDE.reg, 8);break;
		}
		case 0x1A:{
			cyclesTime = 8;
            CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi, z80.m_RegisterDE.reg);break;
		}
		case 0x1B:{
			cyclesTime = 8;
            CPU_16BIT_DEC(z80.m_RegisterDE.reg, 8);break;
		}
		case 0x1C:{
			cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterDE.lo,4);break;
		}
		case 0x1D:{
			cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterDE.lo,4);break;
		}
		case 0x1E:{
			cyclesTime = 8;
			CPU_8BIT_LOAD(z80.m_RegisterDE.lo);break;
		}
		case 0x1F:{
			cyclesTime = 8;
			loopCounter += 8;
			CPU_RR(z80.m_RegisterAF.hi);z80.m_RegisterAF.lo &= ~0x80;break;
		}
		case 0x20:{
			cyclesTime = 8;
			CPU_JUMP_IMMEDIATE(true, FLAG_Z, false);break;
		}
		case 0x21:{
			cyclesTime = 12;
            CPU_16BIT_LOAD(z80.m_RegisterHL.reg);break;
		}
		case 0x22:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterAF.hi); CPU_16BIT_INC(z80.m_RegisterHL.reg,0) ; loopCounter += 8;break;
        }
		case 0x23:{
			cyclesTime = 8;
            CPU_16BIT_INC(z80.m_RegisterHL.reg, 8);break;
		}
		case 0x24:{
			cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterHL.hi, 4);break;
		}
		case 0x25:{
			cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterHL.hi, 4);break;
		}
		case 0x26:{
			cyclesTime = 8;
            CPU_8BIT_LOAD(z80.m_RegisterHL.hi);break;
		}
		case 0x27:{
			cyclesTime = 4;
			CPU_DAA();break;
		}
		case 0x28:{
			cyclesTime = 8;
			CPU_JUMP_IMMEDIATE(true, FLAG_Z, true);break;
		}
		case 0x29:{
			cyclesTime = 8;
            CPU_16BIT_ADD(z80.m_RegisterHL.reg, z80.m_RegisterHL.reg, 8);break;
		}
		case 0x2A:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi,z80.m_RegisterHL.reg ); CPU_16BIT_INC(z80.m_RegisterHL.reg,0);break;
		}
		case 0x2B:{
			cyclesTime = 8;
            CPU_16BIT_DEC(z80.m_RegisterHL.reg, 8);break;
		}
		case 0x2C:{
			cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterHL.lo, 4);break;
		}
		case 0x2D:{
			cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterHL.lo,4);break;
		}
		case 0x2E:{
			cyclesTime = 8;
			CPU_8BIT_LOAD(z80.m_RegisterHL.lo);break;
		}
		case 0x2F:{
			cyclesTime = 4;
			loopCounter += 4;
			z80.m_RegisterAF.hi ^= 0xFF;
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_N) ;
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
			break;
		}
		case 0x30:{
			cyclesTime = 8;
			CPU_JUMP_IMMEDIATE(true, FLAG_C, false);break;
		}
		case 0x31:{
			cyclesTime = 12;
            CPU_16BIT_LOAD(z80.m_StackPointer.reg);break;
		}
		case 0x32:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterAF.hi); CPU_16BIT_DEC(z80.m_RegisterHL.reg, 0) ; loopCounter += 8;break;
		}
		case 0x33:{
			cyclesTime = 8;
            CPU_16BIT_INC(z80.m_StackPointer.reg, 8);break;
		}
		case 0x34:{
			cyclesTime = 12;
			CPU_8BIT_MEMORY_INC(z80.m_RegisterHL.reg,12);break;
		}
		case 0x35:{
			cyclesTime = 12;
			CPU_8BIT_MEMORY_DEC(z80.m_RegisterHL.reg,12);break;
		}
		case 0x36:{
			cyclesTime = 12;
			loopCounter+=12 ;
			byte n = ReadMemory(z80.m_ProgramCounter) ;
			z80.m_ProgramCounter++;
			WriteMemory(z80.m_RegisterHL.reg, n) ;
			break;
		}
		case 0x37:{
			cyclesTime = 4;
			loopCounter += 4;
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
			z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H);
			z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N);
			break;
		}
		case 0x38:{
			cyclesTime = 8;
			CPU_JUMP_IMMEDIATE(true, FLAG_C, true);break;
		}
		case 0x39:{
			cyclesTime = 8;
            CPU_16BIT_ADD(z80.m_RegisterHL.reg, z80.m_StackPointer.reg, 8);break;
		}
		case 0x3A:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi,z80.m_RegisterHL.reg ); CPU_16BIT_DEC(z80.m_RegisterHL.reg,0);break;
		}
		case 0x3B:{
			cyclesTime = 8;
            CPU_16BIT_DEC(z80.m_StackPointer.reg, 8);break;
		}
		case 0x3C:{
			cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterAF.hi, 4);break;
		}
		case 0x3E:{
			cyclesTime = 8;
			loopCounter+=8;
			byte n = ReadMemory(z80.m_ProgramCounter) ;
			z80.m_ProgramCounter++ ;
			z80.m_RegisterAF.hi = n;
			break;
		}
		case 0x3D:{
			cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterAF.hi,4);break;
		}
		case 0x3F:{
			cyclesTime = 4;
			loopCounter += 4 ;
			if (TestBit8(z80.m_RegisterAF.lo, FLAG_C))
				z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_C) ;
			else
				z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;

			z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
			z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N) ;
			break;
		}
		case 0x40:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x41:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x42:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x43:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x44:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x45:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x46:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterBC.hi, z80.m_RegisterHL.reg);break;
		}
		case 0x47:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.hi, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x48:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x49:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x4A:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x4B:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x4C:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x4D:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x4E:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterBC.lo, z80.m_RegisterHL.reg);break;
		}
		case 0x4F:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterBC.lo, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x50:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x51:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x52:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x53:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x54:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x55:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x56:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterDE.hi, z80.m_RegisterHL.reg);break;
		}
		case 0x57:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.hi, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x58:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x59:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x5A:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x5B:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x5C:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x5D:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x5E:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterDE.lo, z80.m_RegisterHL.reg);break;
		}
		case 0x5F:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterDE.lo, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x60:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x61:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x62:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x63:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x64:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x65:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x66:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterHL.hi, z80.m_RegisterHL.reg);break;
		}
		case 0x67:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.hi, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x68:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x69:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x6A:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x6B:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x6C:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x6D:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x6E:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterHL.lo, z80.m_RegisterHL.reg);break;
		}
		case 0x6F:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterHL.lo, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x70:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterBC.hi); loopCounter+=8; break;
		}
		case 0x71:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterBC.lo); loopCounter+=8; break;
		}
		case 0x72:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterDE.hi); loopCounter+=8; break;
		}
		case 0x73:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterDE.lo); loopCounter+=8; break;
		}
		case 0x74:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterHL.hi); loopCounter+=8; break;
		}
		case 0x75:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterHL.lo); loopCounter+=8; break;
		}
		case 0x76:{
			//CPU éteit, donc on fait rien//
			cyclesTime = 4;
			loopCounter += 4;
			z80.m_Halted = true;
			
			bool tempSkip = false;
			
			if((z80.m_Rom[0xFFFF] & z80.m_Rom[0xFF0F] & 0x1F) && (!z80.m_InteruptMaster)){
				tempSkip = true;
			}
			else{
				tempSkip = false;
			}
			
			z80.m_SkipInstruction = tempSkip; 
			break;
		}
		case 0x77:{
			cyclesTime = 8;
			WriteMemory(z80.m_RegisterHL.reg, z80.m_RegisterAF.hi); loopCounter+=8; break;
		}
		case 0x78:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi, 4);break;
		}
		case 0x79:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo, 4);break;
		}
		case 0x7A:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi, 4);break;
		}
		case 0x7B:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo, 4);break;
		}
		case 0x7C:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi, 4);break;
		}
		case 0x7D:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo, 4);break;
		}
		case 0x7E:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi, z80.m_RegisterHL.reg);break;
		}
		case 0x7F:{
			cyclesTime = 4;
			CPU_REG_LOAD(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi, 4);break;
		}
		case 0x80:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4,false,false); break;
		}
		case 0x81:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4,false,false); break;
		}
		case 0x82:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4,false,false); break;
		}
		case 0x83:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4,false,false); break;
		}
		case 0x84:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4,false,false); break;
		}
		case 0x85:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4,false,false); break;
		}
		case 0x86:{
			cyclesTime = 8;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8,false,false); break;
		}
		case 0x87:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4,false,false); break;
		}
		case 0x88:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4,false,true); break;
		}
		case 0x89:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4,false,true); break;
		}
		case 0x8A:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4,false,true); break;
		}
		case 0x8B:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4,false,true); break;
		}
		case 0x8C:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4,false,true); break;
		}
		case 0x8D:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4,false,true); break;
		}
		case 0x8E:{
			cyclesTime = 8;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8,false,true); break;
		}
		case 0x8F:{
			cyclesTime = 4;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4,false,true); break;
		}
		case 0x90:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4,false,false); break;
		}
		case 0x91:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4,false,false); break;
		}
		case 0x92:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4,false,false); break;
		}
		case 0x93:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4,false,false); break;
		}
		case 0x94:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4,false,false); break;
		}
		case 0x95:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4,false,false); break;
		}
		case 0x96:{
			cyclesTime = 8;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8,false,false); break;
		}
		case 0x97:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4,false,false); break;
		}
		case 0x98:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4,false,true); break;
		}
		case 0x99:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4,false,true); break;
		}
		case 0x9A:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4,false,true); break;
		}
		case 0x9B:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4,false,true); break;
		}
		case 0x9C:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4,false,true); break;
		}
		case 0x9D:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4,false,true); break;
		}
		case 0x9E:{
			cyclesTime = 8;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8,false,true) ; break ;
		}
		case 0x9F:{
			cyclesTime = 4;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4,false,true); break;
		}
		case 0xA0:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4, false); break;
		}
		case 0xA1:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4, false); break;
		}
		case 0xA2:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4, false); break;
		}
		case 0xA3:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4, false); break;
		}
		case 0xA4:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4, false); break;
		}
		case 0xA5:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4, false); break;
		}
		case 0xA6:{
			cyclesTime = 8;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8, false); break;
		}
		case 0xA7:{
			cyclesTime = 4;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4, false); break;
		}
		case 0xA8:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4, false); break;
		}
		case 0xA9:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4, false); break;
		}
		case 0xAA:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4, false); break;
		}
		case 0xAB:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4, false); break;
		}
		case 0xAC:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4, false); break;
		}
		case 0xAD:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4, false); break;
		}
		case 0xAE:{
			cyclesTime = 8;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8, false); break;
		}
		case 0xAF:{
			cyclesTime = 4;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4, false); break;
		}
		case 0xB0:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4, false); break;
		}
		case 0xB1:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4, false); break;
		}
		case 0xB2:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4, false); break;
		}
		case 0xB3:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4, false); break;
		}
		case 0xB4:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4, false); break;
		}
		case 0xB5:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4, false); break;
		}
		case 0xB6:{
			cyclesTime = 8;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8, false); break;
		}
		case 0xB7:{
			cyclesTime = 4;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4, false); break;
		}
		case 0xB8:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterBC.hi,4, false); break;
		}
		case 0xB9:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterBC.lo,4, false); break;
		}
		case 0xBA:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterDE.hi,4, false); break;
		}
		case 0xBB:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterDE.lo,4, false); break;
		}
		case 0xBC:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterHL.hi,4, false); break;
		}
		case 0xBD:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterHL.lo,4, false); break;
		}
		case 0xBE:{
			cyclesTime = 8;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, ReadMemory(z80.m_RegisterHL.reg),8, false); break;
		}
		case 0xBF:{
			cyclesTime = 4;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, z80.m_RegisterAF.hi,4, false); break;
		}
		case 0xC0:{
			cyclesTime = 8;
			CPU_RETURN(true, FLAG_Z, false); break;
		}
		case 0xC1:{
			cyclesTime = 12;
			z80.m_RegisterBC.reg = PopWordOffStack(); loopCounter+=12;break;
		}
		case 0xC2:{
			cyclesTime = 12;
			CPU_JUMP(true, FLAG_Z, false); break;
		}
		case 0xC3:{
			cyclesTime = 12; //maybe 16
			CPU_JUMP(false, 0, false); break;
		}
		case 0xC4:{
			cyclesTime = 12;
			CPU_CALL(true, FLAG_Z, false); break;
		}
		case 0xC5:{
			cyclesTime = 16;
			PushWordOntoStack(z80.m_RegisterBC.reg); loopCounter+=16; break;
		}
		case 0xC6:{
			cyclesTime = 8;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, 0, 8, true, false); break;
		}
		case 0xC7:{
			cyclesTime = 32;
			CPU_RESTARTS(0x00); break;
		}
		case 0xC8:{
			cyclesTime = 8;
			CPU_RETURN(true, FLAG_Z, true); break;
		}
		case 0xC9:{
			cyclesTime = 8;
			CPU_RETURN( false, 0, false ) ; break ;
		}
		case 0xCA:{
			cyclesTime = 12;
			CPU_JUMP(true, FLAG_Z, true); break;
		}
		case 0xCB:{
			ExecuteExtendedOpcode(); cyclesTime+=4; loopCounter+=4; break;
		}
		case 0xCC:{
			cyclesTime = 12;
			CPU_CALL(true, FLAG_Z, true) ;break ;
		}
		case 0xCD:{
			cyclesTime = 12;
			CPU_CALL(false, 0, false); break;
		}
		case 0xCE:{
			cyclesTime = 8;
			CPU_8BIT_ADD(z80.m_RegisterAF.hi, ReadMemory(z80.m_ProgramCounter++),8,true,true); break;
		}
		case 0xCF:{
			cyclesTime = 32;
			CPU_RESTARTS(0x08); break;
		}
		case 0xD0:{
			cyclesTime = 8;
			CPU_RETURN(true, FLAG_C, false); break;
		}
		case 0xD1:{
			loopCounter = 12;
			z80.m_RegisterDE.reg = PopWordOffStack(); loopCounter+=12 ;break;
		}
		case 0xD2:{
			cyclesTime = 12;
			CPU_JUMP(true, FLAG_C, false); break;
		}
		case 0xD4:{
			cyclesTime = 12;
			CPU_CALL(true, FLAG_C, false); break;
		}
		case 0xD5:{
			cyclesTime = 16;
			PushWordOntoStack(z80.m_RegisterDE.reg); loopCounter+=16; break;
		}
		case 0xD6:{
			cyclesTime = 8;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, 0,8,true,false) ; break ;
		}
		case 0xD7:{
			cyclesTime = 32;
			CPU_RESTARTS(0x10); break;
		}
		case 0xD8:{
			cyclesTime = 8;
			CPU_RETURN(true, FLAG_C, true); break;
		}
		case 0xD9:{
			cyclesTime = 8;
			z80.m_ProgramCounter = PopWordOffStack();
			z80.m_InteruptMaster = true;
			loopCounter+=8;
			break;
		}
		case 0xDA:{
			cyclesTime = 12;
			CPU_JUMP(true, FLAG_C, true); break;
		}
		case 0xDC:{
			cyclesTime = 12;
			CPU_CALL(true, FLAG_C, true); break;
		}
		case 0xDE:{
			cyclesTime = 8;
			CPU_8BIT_SUB(z80.m_RegisterAF.hi, ReadMemory(z80.m_ProgramCounter++),8,true,true); break;
		}
		case 0xDF:{
			cyclesTime = 32;
			CPU_RESTARTS(0x18); break;
		}
		case 0xE0:{
			cyclesTime = 12;
			byte n = ReadMemory(z80.m_ProgramCounter) ;
			z80.m_ProgramCounter++;
			word address = 0xFF00 + n;
			WriteMemory(address, z80.m_RegisterAF.hi) ;
			loopCounter += 12 ;
			break;
		}
		case 0xE1:{
			cyclesTime = 12;
			z80.m_RegisterHL.reg = PopWordOffStack(); loopCounter+=12; break;
		}
		case 0xE2:{
			cyclesTime = 8;
			WriteMemory((0xFF00+z80.m_RegisterBC.lo), z80.m_RegisterAF.hi) ; loopCounter+=8; break ;
		}
		case 0xE5:{
			cyclesTime = 16;
			PushWordOntoStack(z80.m_RegisterHL.reg); loopCounter+=16; break;
		}
		case 0xE6:{
			cyclesTime = 8;
			CPU_8BIT_AND(z80.m_RegisterAF.hi, 0,8, true); break;
		}
		case 0xE7:{
			cyclesTime = 32;
			CPU_RESTARTS(0x20); break;
		}
		case 0xE8:{
			cyclesTime = 16;
			byte temp = ReadMemory(z80.m_ProgramCounter++);
			signed_byte s_temp = (signed_byte)temp;
			z80.m_StackPointer.reg = CPU_8BIT_SIGNED_ADD(z80.m_StackPointer.reg, s_temp, 16);
			break;
		}
		case 0xE9:{
			cyclesTime = 4;
			loopCounter+=4; z80.m_ProgramCounter = z80.m_RegisterHL.reg; break;
		}
		case 0xEA:{
			cyclesTime = 16;
			loopCounter += 16;
			word nn = ReadWord();
			z80.m_ProgramCounter+=2;
			WriteMemory(nn, z80.m_RegisterAF.hi);
			break;
		}
		case 0xEE:{
			cyclesTime = 8;
			CPU_8BIT_XOR(z80.m_RegisterAF.hi, 0,8, true); break;
		}
		case 0xEF:{
			cyclesTime = 32;
			CPU_RESTARTS(0x28); break;
		}
		case 0xF0:{
			cyclesTime = 12;
			byte n = ReadMemory(z80.m_ProgramCounter) ;
			z80.m_ProgramCounter++ ;
			word address = 0xFF00 + n ;
			z80.m_RegisterAF.hi = ReadMemory( address ) ;
			loopCounter+=12 ;
			break;
		}
		case 0xF1:{
			cyclesTime = 12;
			z80.m_RegisterAF.reg = PopWordOffStack(); z80.m_RegisterAF.lo &= 0xF0; loopCounter+=12; break;
		}
		case 0xF2:{
			cyclesTime = 8;
			CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi, (0xFF00+z80.m_RegisterBC.lo)); break;
		}
		case 0xF3:{
			cyclesTime = 4;
			z80.m_InteruptMaster = false;
			loopCounter+=4;
			break;
		}
		case 0xF5:{
			cyclesTime = 16;
			PushWordOntoStack(z80.m_RegisterAF.reg); loopCounter+=16; break;
		}
		case 0xF6:{
			cyclesTime = 8;
			CPU_8BIT_OR(z80.m_RegisterAF.hi, 0,8, true); break;
		}
		case 0xF7:{
			cyclesTime = 32;
			CPU_RESTARTS(0x30); break;
		}
		case 0xF8:{
			cyclesTime = 12;
			byte temp = ReadMemory(z80.m_ProgramCounter++);
			signed_byte s_temp = (signed_byte)temp;
			z80.m_RegisterHL.reg = CPU_8BIT_SIGNED_ADD(z80.m_StackPointer.reg, s_temp, 12);
			break;
		}
		case 0xF9:{
			cyclesTime = 8;
			z80.m_StackPointer.reg = z80.m_RegisterHL.reg ; loopCounter+=8; break ;
		}
		case 0xFA:{
			cyclesTime = 16;
			loopCounter+=16;
			word nn = ReadWord();
			z80.m_ProgramCounter+=2;
			byte n = ReadMemory(nn);
			z80.m_RegisterAF.hi = n;
			break;
		}
		case 0xFB:{
			cyclesTime = 4;
			z80.m_InteruptDelay = true;
			loopCounter+=4;
			break;
		}
		case 0xFE:{
			cyclesTime = 8;
			CPU_8BIT_COMPARE(z80.m_RegisterAF.hi, 0,8, true); break;
		}
		case 0xFF:{
			cyclesTime = 32;
			CPU_RESTARTS(0x38); break;
		}
		default:{
			printf("//////////////////////////////////////////////////////////////////////////////////////////////");
			printf("//////////////////////////////////////////////////////////////////////////////////////////////");
			printf("//////////////////////////////////////////////////////////////////////////////////////////////");
			printf("//////////////////////////////////////////////////////////////////////////////////////////////");
			printf("//////////////////////////////////////////////////////////////////////////////////////////////");
			printf("OPCODE non reconnu:0x%08x", opcode);
			cyclesTime = 0;
            break;
		}
    }
	
}

void ExecuteExtendedOpcode(){
	byte opcode;

	opcode = ReadMemory(z80.m_ProgramCounter);
	
	z80.m_ProgramCounter++;

	//printf("EXTENDED OPCODE: 0x%08x", opcode);
	
	switch(opcode){
		// rotate left through carry
  		case 0x0 : CPU_RLC(z80.m_RegisterBC.hi) ; cyclesTime = 8; loopCounter+=8; break ;
  		case 0x1 : CPU_RLC(z80.m_RegisterBC.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0x2 : CPU_RLC(z80.m_RegisterDE.hi) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0x3 : CPU_RLC(z80.m_RegisterDE.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0x4 : CPU_RLC(z80.m_RegisterHL.hi) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0x5 : CPU_RLC(z80.m_RegisterHL.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0x6 : CPU_RLC_MEMORY(z80.m_RegisterHL.reg) ; break ;
  		case 0x7 : CPU_RLC(z80.m_RegisterAF.hi) ;cyclesTime = 8; loopCounter+=8; break ;

  		// rotate right through carry
  		case 0x8 : CPU_RRC(z80.m_RegisterBC.hi) ; cyclesTime = 8; loopCounter+=8; break ;
  		case 0x9 : CPU_RRC(z80.m_RegisterBC.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0xA : CPU_RRC(z80.m_RegisterDE.hi) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0xB : CPU_RRC(z80.m_RegisterDE.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0xC : CPU_RRC(z80.m_RegisterHL.hi) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0xD : CPU_RRC(z80.m_RegisterHL.lo) ; cyclesTime = 8; loopCounter+=8;break ;
  		case 0xE : CPU_RRC_MEMORY(z80.m_RegisterHL.reg) ; break ;
  		case 0xF : CPU_RRC(z80.m_RegisterAF.hi) ; cyclesTime = 8; loopCounter+=8;break ;

  		// rotate left
  		case 0x10: CPU_RL(z80.m_RegisterBC.hi); cyclesTime = 8; loopCounter+=8; break;
  		case 0x11: CPU_RL(z80.m_RegisterBC.lo); cyclesTime = 8; loopCounter+=8;break;
	  	case 0x12: CPU_RL(z80.m_RegisterDE.hi); cyclesTime = 8; loopCounter+=8;break;
  		case 0x13: CPU_RL(z80.m_RegisterDE.lo); cyclesTime = 8; loopCounter+=8;break;
  		case 0x14: CPU_RL(z80.m_RegisterHL.hi); cyclesTime = 8; loopCounter+=8;break;
  		case 0x15: CPU_RL(z80.m_RegisterHL.lo); cyclesTime = 8; loopCounter+=8;break;
  		case 0x16: CPU_RL_MEMORY(z80.m_RegisterHL.reg); break;
  		case 0x17: CPU_RL(z80.m_RegisterAF.hi);cyclesTime = 8; loopCounter+=8; break;

  		// rotate right
  		case 0x18: CPU_RR(z80.m_RegisterBC.hi); cyclesTime = 8; loopCounter += 8; break;
  		case 0x19: CPU_RR(z80.m_RegisterBC.lo); cyclesTime = 8; loopCounter += 8;break;
  		case 0x1A: CPU_RR(z80.m_RegisterDE.hi); cyclesTime = 8; loopCounter += 8;break;
  		case 0x1B: CPU_RR(z80.m_RegisterDE.lo); cyclesTime = 8; loopCounter += 8;break;
  		case 0x1C: CPU_RR(z80.m_RegisterHL.hi); cyclesTime = 8; loopCounter += 8;break;
  		case 0x1D: CPU_RR(z80.m_RegisterHL.lo); cyclesTime = 8; loopCounter += 8;break;
  		case 0x1E: CPU_RR_MEMORY(z80.m_RegisterHL.reg); break;
  		case 0x1F: CPU_RR(z80.m_RegisterAF.hi); cyclesTime = 8; loopCounter += 8;break;

  		case 0x20 : CPU_SLA( z80.m_RegisterBC.hi ) ;break ;
  		case 0x21 : CPU_SLA( z80.m_RegisterBC.lo ) ;break ;
  		case 0x22 : CPU_SLA( z80.m_RegisterDE.hi ) ;break ;
	  	case 0x23 : CPU_SLA( z80.m_RegisterDE.lo ) ;break ;
  		case 0x24 : CPU_SLA( z80.m_RegisterHL.hi ) ;break ;
  		case 0x25 : CPU_SLA( z80.m_RegisterHL.lo ) ;break ;
  		case 0x26 : CPU_SLA_MEMORY( z80.m_RegisterHL.reg ) ;break ;
		case 0x27 : CPU_SLA( z80.m_RegisterAF.hi ) ;break ;

  		case 0x28 : CPU_SRA( z80.m_RegisterBC.hi ) ; break ;
  		case 0x29 : CPU_SRA( z80.m_RegisterBC.lo ) ; break ;
  		case 0x2A : CPU_SRA( z80.m_RegisterDE.hi ) ; break ;
  		case 0x2B : CPU_SRA( z80.m_RegisterDE.lo ) ; break ;
  		case 0x2C : CPU_SRA( z80.m_RegisterHL.hi ) ; break ;
  		case 0x2D : CPU_SRA( z80.m_RegisterHL.lo ) ; break ;
  		case 0x2E : CPU_SRA_MEMORY( z80.m_RegisterHL.reg ) ; break ;
  		case 0x2F : CPU_SRA( z80.m_RegisterAF.hi ) ; break ;

  		case 0x38 : CPU_SRL( z80.m_RegisterBC.hi ) ; break ;
  		case 0x39 : CPU_SRL( z80.m_RegisterBC.lo ) ; break ;
  		case 0x3A : CPU_SRL( z80.m_RegisterDE.hi ) ; break ;
  		case 0x3B : CPU_SRL( z80.m_RegisterDE.lo ) ; break ;
  		case 0x3C : CPU_SRL( z80.m_RegisterHL.hi ) ; break ;
  		case 0x3D : CPU_SRL( z80.m_RegisterHL.lo ) ; break ;
  		case 0x3E : CPU_SRL_MEMORY( z80.m_RegisterHL.reg ) ; break ;
	  	case 0x3F : CPU_SRL( z80.m_RegisterAF.hi ) ; break ;

			// swap nibbles
		case 0x37 : CPU_SWAP_NIBBLES( z80.m_RegisterAF.hi ) ;break ;
		case 0x30 : CPU_SWAP_NIBBLES( z80.m_RegisterBC.hi ) ;break ;
		case 0x31 : CPU_SWAP_NIBBLES( z80.m_RegisterBC.lo ) ;break ;
		case 0x32 : CPU_SWAP_NIBBLES( z80.m_RegisterDE.hi ) ;break ;
		case 0x33 : CPU_SWAP_NIBBLES( z80.m_RegisterDE.lo ) ;break ;
		case 0x34 : CPU_SWAP_NIBBLES( z80.m_RegisterHL.hi ) ;break ;
		case 0x35 : CPU_SWAP_NIBBLES( z80.m_RegisterHL.lo ) ;break ;
		case 0x36 : CPU_SWAP_NIB_MEM( z80.m_RegisterHL.reg ) ;break ;

		// test bit
		case 0x40 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 0 , 8 ) ; break ;
		case 0x41 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 0 , 8 ) ; break ;
		case 0x42 : CPU_TEST_BIT( z80.m_RegisterDE.hi, 0 , 8 ) ; break ;
		case 0x43 : CPU_TEST_BIT( z80.m_RegisterDE.lo, 0 , 8 ) ; break ;
		case 0x44 : CPU_TEST_BIT( z80.m_RegisterHL.hi, 0 , 8 ) ; break ;
		case 0x45 : CPU_TEST_BIT( z80.m_RegisterHL.lo, 0 , 8 ) ; break ;
		case 0x46 : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 0 , 16 ) ; break ;
		case 0x47 : CPU_TEST_BIT( z80.m_RegisterAF.hi, 0 , 8 ) ; break ;
		case 0x48 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 1 , 8 ) ; break ;
		case 0x49 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 1 , 8 ) ; break ;
		case 0x4A : CPU_TEST_BIT( z80.m_RegisterDE.hi, 1 , 8 ) ; break ;
		case 0x4B : CPU_TEST_BIT( z80.m_RegisterDE.lo, 1 , 8 ) ; break ;
		case 0x4C : CPU_TEST_BIT( z80.m_RegisterHL.hi, 1 , 8 ) ; break ;
		case 0x4D : CPU_TEST_BIT( z80.m_RegisterHL.lo, 1 , 8 ) ; break ;
		case 0x4E : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 1 , 16 ) ; break ;
		case 0x4F : CPU_TEST_BIT( z80.m_RegisterAF.hi, 1 , 8 ) ; break ;
		case 0x50 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 2 , 8 ) ; break ;
		case 0x51 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 2 , 8 ) ; break ;
		case 0x52 : CPU_TEST_BIT( z80.m_RegisterDE.hi, 2 , 8 ) ; break ;
		case 0x53 : CPU_TEST_BIT( z80.m_RegisterDE.lo, 2 , 8 ) ; break ;
		case 0x54 : CPU_TEST_BIT( z80.m_RegisterHL.hi, 2 , 8 ) ; break ;
		case 0x55 : CPU_TEST_BIT( z80.m_RegisterHL.lo, 2 , 8 ) ; break ;
		case 0x56 : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 2 , 16 ) ; break ;
		case 0x57 : CPU_TEST_BIT( z80.m_RegisterAF.hi, 2 , 8 ) ; break ;
		case 0x58 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 3 , 8 ) ; break ;
		case 0x59 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 3 , 8 ) ; break ;
		case 0x5A : CPU_TEST_BIT( z80.m_RegisterDE.hi, 3 , 8 ) ; break ;
		case 0x5B : CPU_TEST_BIT( z80.m_RegisterDE.lo, 3 , 8 ) ; break ;
		case 0x5C : CPU_TEST_BIT( z80.m_RegisterHL.hi, 3 , 8 ) ; break ;
		case 0x5D : CPU_TEST_BIT( z80.m_RegisterHL.lo, 3 , 8 ) ; break ;
		case 0x5E : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 3 , 16 ) ; break ;
		case 0x5F : CPU_TEST_BIT( z80.m_RegisterAF.hi, 3 , 8 ) ; break ;
		case 0x60 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 4 , 8 ) ; break ;
		case 0x61 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 4 , 8 ) ; break ;
		case 0x62 : CPU_TEST_BIT( z80.m_RegisterDE.hi, 4 , 8 ) ; break ;
		case 0x63 : CPU_TEST_BIT( z80.m_RegisterDE.lo, 4 , 8 ) ; break ;
		case 0x64 : CPU_TEST_BIT( z80.m_RegisterHL.hi, 4 , 8 ) ; break ;
		case 0x65 : CPU_TEST_BIT( z80.m_RegisterHL.lo, 4 , 8 ) ; break ;
		case 0x66 : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 4 , 16 ) ; break ;
		case 0x67 : CPU_TEST_BIT( z80.m_RegisterAF.hi, 4 , 8 ) ; break ;
		case 0x68 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 5 , 8 ) ; break ;
		case 0x69 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 5 , 8 ) ; break ;
		case 0x6A : CPU_TEST_BIT( z80.m_RegisterDE.hi, 5 , 8 ) ; break ;
		case 0x6B : CPU_TEST_BIT( z80.m_RegisterDE.lo, 5 , 8 ) ; break ;
		case 0x6C : CPU_TEST_BIT( z80.m_RegisterHL.hi, 5 , 8 ) ; break ;
		case 0x6D : CPU_TEST_BIT( z80.m_RegisterHL.lo, 5 , 8 ) ; break ;
		case 0x6E : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 5 , 16 ) ; break ;
		case 0x6F : CPU_TEST_BIT( z80.m_RegisterAF.hi, 5 , 8 ) ; break ;
		case 0x70 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 6 , 8 ) ; break ;
		case 0x71 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 6 , 8 ) ; break ;
		case 0x72 : CPU_TEST_BIT( z80.m_RegisterDE.hi, 6 , 8 ) ; break ;
		case 0x73 : CPU_TEST_BIT( z80.m_RegisterDE.lo, 6 , 8 ) ; break ;
		case 0x74 : CPU_TEST_BIT( z80.m_RegisterHL.hi, 6 , 8 ) ; break ;
		case 0x75 : CPU_TEST_BIT( z80.m_RegisterHL.lo, 6 , 8 ) ; break ;
		case 0x76 : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 6 , 16 ) ; break ;
		case 0x77 : CPU_TEST_BIT( z80.m_RegisterAF.hi, 6 , 8 ) ; break ;
		case 0x78 : CPU_TEST_BIT( z80.m_RegisterBC.hi, 7 , 8 ) ; break ;
		case 0x79 : CPU_TEST_BIT( z80.m_RegisterBC.lo, 7 , 8 ) ; break ;
		case 0x7A : CPU_TEST_BIT( z80.m_RegisterDE.hi, 7 , 8 ) ; break ;
		case 0x7B : CPU_TEST_BIT( z80.m_RegisterDE.lo, 7 , 8 ) ; break ;
		case 0x7C : CPU_TEST_BIT( z80.m_RegisterHL.hi, 7 , 8 ) ; break ;
		case 0x7D : CPU_TEST_BIT( z80.m_RegisterHL.lo, 7 , 8 ) ; break ;
		case 0x7E : CPU_TEST_BIT( ReadMemory(z80.m_RegisterHL.reg), 7 , 16 ) ; break ;
		case 0x7F : CPU_TEST_BIT( z80.m_RegisterAF.hi, 7 , 8 ) ; break ;

		// reset bit
		case 0x80 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 0 ) ; break ;
		case 0x81 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 0 ) ; break ;
		case 0x82 : CPU_RESET_BIT( z80.m_RegisterDE.hi, 0 ) ; break ;
		case 0x83 : CPU_RESET_BIT( z80.m_RegisterDE.lo, 0 ) ; break ;
		case 0x84 : CPU_RESET_BIT( z80.m_RegisterHL.hi, 0 ) ; break ;
		case 0x85 : CPU_RESET_BIT( z80.m_RegisterHL.lo, 0 ) ; break ;
		case 0x86 : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 0 ) ; break ;
		case 0x87 : CPU_RESET_BIT( z80.m_RegisterAF.hi, 0 ) ; break ;
		case 0x88 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 1  ) ; break ;
		case 0x89 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 1 ) ; break ;
		case 0x8A : CPU_RESET_BIT( z80.m_RegisterDE.hi, 1 ) ; break ;
		case 0x8B : CPU_RESET_BIT( z80.m_RegisterDE.lo, 1 ) ; break ;
		case 0x8C : CPU_RESET_BIT( z80.m_RegisterHL.hi, 1 ) ; break ;
		case 0x8D : CPU_RESET_BIT( z80.m_RegisterHL.lo, 1 ) ; break ;
		case 0x8E : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 1 ) ; break ;
		case 0x8F : CPU_RESET_BIT( z80.m_RegisterAF.hi, 1  ) ; break ;
		case 0x90 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 2  ) ; break ;
		case 0x91 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 2  ) ; break ;
		case 0x92 : CPU_RESET_BIT( z80.m_RegisterDE.hi, 2  ) ; break ;
		case 0x93 : CPU_RESET_BIT( z80.m_RegisterDE.lo, 2  ) ; break ;
		case 0x94 : CPU_RESET_BIT( z80.m_RegisterHL.hi, 2  ) ; break ;
		case 0x95 : CPU_RESET_BIT( z80.m_RegisterHL.lo, 2  ) ; break ;
		case 0x96 : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 2 ) ; break ;
		case 0x97 : CPU_RESET_BIT( z80.m_RegisterAF.hi, 2  ) ; break ;
		case 0x98 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 3  ) ; break ;
		case 0x99 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 3  ) ; break ;
		case 0x9A : CPU_RESET_BIT( z80.m_RegisterDE.hi, 3  ) ; break ;
		case 0x9B : CPU_RESET_BIT( z80.m_RegisterDE.lo, 3  ) ; break ;
		case 0x9C : CPU_RESET_BIT( z80.m_RegisterHL.hi, 3  ) ; break ;
		case 0x9D : CPU_RESET_BIT( z80.m_RegisterHL.lo, 3  ) ; break ;
		case 0x9E : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 3  ) ; break ;
		case 0x9F : CPU_RESET_BIT( z80.m_RegisterAF.hi, 3  ) ; break ;
		case 0xA0 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 4  ) ; break ;
		case 0xA1 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 4  ) ; break ;
		case 0xA2 : CPU_RESET_BIT( z80.m_RegisterDE.hi, 4  ) ; break ;
		case 0xA3 : CPU_RESET_BIT( z80.m_RegisterDE.lo, 4  ) ; break ;
		case 0xA4 : CPU_RESET_BIT( z80.m_RegisterHL.hi, 4  ) ; break ;
		case 0xA5 : CPU_RESET_BIT( z80.m_RegisterHL.lo, 4  ) ; break ;
		case 0xA6 : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 4) ; break ;
		case 0xA7 : CPU_RESET_BIT( z80.m_RegisterAF.hi, 4 ) ; break ;
		case 0xA8 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 5 ) ; break ;
		case 0xA9 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 5 ) ; break ;
		case 0xAA : CPU_RESET_BIT( z80.m_RegisterDE.hi, 5 ) ; break ;
		case 0xAB : CPU_RESET_BIT( z80.m_RegisterDE.lo, 5 ) ; break ;
		case 0xAC : CPU_RESET_BIT( z80.m_RegisterHL.hi, 5 ) ; break ;
		case 0xAD : CPU_RESET_BIT( z80.m_RegisterHL.lo, 5 ) ; break ;
		case 0xAE : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 5 ) ; break ;
		case 0xAF : CPU_RESET_BIT( z80.m_RegisterAF.hi, 5  ) ; break ;
		case 0xB0 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 6  ) ; break ;
		case 0xB1 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 6  ) ; break ;
		case 0xB2 : CPU_RESET_BIT( z80.m_RegisterDE.hi, 6  ) ; break ;
		case 0xB3 : CPU_RESET_BIT( z80.m_RegisterDE.lo, 6  ) ; break ;
		case 0xB4 : CPU_RESET_BIT( z80.m_RegisterHL.hi, 6  ) ; break ;
		case 0xB5 : CPU_RESET_BIT( z80.m_RegisterHL.lo, 6  ) ; break ;
		case 0xB6 : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 6 ) ; break ;
		case 0xB7 : CPU_RESET_BIT( z80.m_RegisterAF.hi, 6  ) ; break ;
		case 0xB8 : CPU_RESET_BIT( z80.m_RegisterBC.hi, 7  ) ; break ;
		case 0xB9 : CPU_RESET_BIT( z80.m_RegisterBC.lo, 7  ) ; break ;
		case 0xBA : CPU_RESET_BIT( z80.m_RegisterDE.hi, 7  ) ; break ;
		case 0xBB : CPU_RESET_BIT( z80.m_RegisterDE.lo, 7  ) ; break ;
		case 0xBC : CPU_RESET_BIT( z80.m_RegisterHL.hi, 7  ) ; break ;
		case 0xBD : CPU_RESET_BIT( z80.m_RegisterHL.lo, 7  ) ; break ;
		case 0xBE : CPU_RESET_BIT_MEMORY( z80.m_RegisterHL.reg, 7 ) ; break ;
		case 0xBF : CPU_RESET_BIT( z80.m_RegisterAF.hi, 7 ) ; break ;


		// set bit
		case 0xC0 : CPU_SET_BIT( z80.m_RegisterBC.hi, 0 ) ; break ;
		case 0xC1 : CPU_SET_BIT( z80.m_RegisterBC.lo, 0 ) ; break ;
		case 0xC2 : CPU_SET_BIT( z80.m_RegisterDE.hi, 0 ) ; break ;
		case 0xC3 : CPU_SET_BIT( z80.m_RegisterDE.lo, 0 ) ; break ;
		case 0xC4 : CPU_SET_BIT( z80.m_RegisterHL.hi, 0 ) ; break ;
		case 0xC5 : CPU_SET_BIT( z80.m_RegisterHL.lo, 0 ) ; break ;
		case 0xC6 : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 0 ) ; break ;
		case 0xC7 : CPU_SET_BIT( z80.m_RegisterAF.hi, 0 ) ; break ;
		case 0xC8 : CPU_SET_BIT( z80.m_RegisterBC.hi, 1  ) ; break ;
		case 0xC9 : CPU_SET_BIT( z80.m_RegisterBC.lo, 1 ) ; break ;
		case 0xCA : CPU_SET_BIT( z80.m_RegisterDE.hi, 1 ) ; break ;
		case 0xCB : CPU_SET_BIT( z80.m_RegisterDE.lo, 1 ) ; break ;
		case 0xCC : CPU_SET_BIT( z80.m_RegisterHL.hi, 1 ) ; break ;
		case 0xCD : CPU_SET_BIT( z80.m_RegisterHL.lo, 1 ) ; break ;
		case 0xCE : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 1 ) ; break ;
		case 0xCF : CPU_SET_BIT( z80.m_RegisterAF.hi, 1  ) ; break ;
		case 0xD0 : CPU_SET_BIT( z80.m_RegisterBC.hi, 2  ) ; break ;
		case 0xD1 : CPU_SET_BIT( z80.m_RegisterBC.lo, 2  ) ; break ;
		case 0xD2 : CPU_SET_BIT( z80.m_RegisterDE.hi, 2  ) ; break ;
		case 0xD3 : CPU_SET_BIT( z80.m_RegisterDE.lo, 2  ) ; break ;
		case 0xD4 : CPU_SET_BIT( z80.m_RegisterHL.hi, 2  ) ; break ;
		case 0xD5 : CPU_SET_BIT( z80.m_RegisterHL.lo, 2  ) ; break ;
		case 0xD6 : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 2 ) ; break ;
		case 0xD7 : CPU_SET_BIT( z80.m_RegisterAF.hi, 2  ) ; break ;
		case 0xD8 : CPU_SET_BIT( z80.m_RegisterBC.hi, 3  ) ; break ;
		case 0xD9 : CPU_SET_BIT( z80.m_RegisterBC.lo, 3  ) ; break ;
		case 0xDA : CPU_SET_BIT( z80.m_RegisterDE.hi, 3  ) ; break ;
		case 0xDB : CPU_SET_BIT( z80.m_RegisterDE.lo, 3  ) ; break ;
		case 0xDC : CPU_SET_BIT( z80.m_RegisterHL.hi, 3  ) ; break ;
		case 0xDD : CPU_SET_BIT( z80.m_RegisterHL.lo, 3  ) ; break ;
		case 0xDE : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 3  ) ; break ;
		case 0xDF : CPU_SET_BIT( z80.m_RegisterAF.hi, 3  ) ; break ;
		case 0xE0 : CPU_SET_BIT( z80.m_RegisterBC.hi, 4  ) ; break ;
		case 0xE1 : CPU_SET_BIT( z80.m_RegisterBC.lo, 4  ) ; break ;
		case 0xE2 : CPU_SET_BIT( z80.m_RegisterDE.hi, 4  ) ; break ;
		case 0xE3 : CPU_SET_BIT( z80.m_RegisterDE.lo, 4  ) ; break ;
		case 0xE4 : CPU_SET_BIT( z80.m_RegisterHL.hi, 4  ) ; break ;
		case 0xE5 : CPU_SET_BIT( z80.m_RegisterHL.lo, 4  ) ; break ;
		case 0xE6 : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 4) ; break ;
		case 0xE7 : CPU_SET_BIT( z80.m_RegisterAF.hi, 4 ) ; break ;
		case 0xE8 : CPU_SET_BIT( z80.m_RegisterBC.hi, 5 ) ; break ;
		case 0xE9 : CPU_SET_BIT( z80.m_RegisterBC.lo, 5 ) ; break ;
		case 0xEA : CPU_SET_BIT( z80.m_RegisterDE.hi, 5 ) ; break ;
		case 0xEB : CPU_SET_BIT( z80.m_RegisterDE.lo, 5 ) ; break ;
		case 0xEC : CPU_SET_BIT( z80.m_RegisterHL.hi, 5 ) ; break ;
		case 0xED : CPU_SET_BIT( z80.m_RegisterHL.lo, 5 ) ; break ;
		case 0xEE : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 5 ) ; break ;
		case 0xEF : CPU_SET_BIT( z80.m_RegisterAF.hi, 5  ) ; break ;
		case 0xF0 : CPU_SET_BIT( z80.m_RegisterBC.hi, 6  ) ; break ;
		case 0xF1 : CPU_SET_BIT( z80.m_RegisterBC.lo, 6  ) ; break ;
		case 0xF2 : CPU_SET_BIT( z80.m_RegisterDE.hi, 6  ) ; break ;
		case 0xF3 : CPU_SET_BIT( z80.m_RegisterDE.lo, 6  ) ; break ;
		case 0xF4 : CPU_SET_BIT( z80.m_RegisterHL.hi, 6  ) ; break ;
		case 0xF5 : CPU_SET_BIT( z80.m_RegisterHL.lo, 6  ) ; break ;
		case 0xF6 : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 6 ) ; break ;
		case 0xF7 : CPU_SET_BIT( z80.m_RegisterAF.hi, 6 ) ; break ;
		case 0xF8 : CPU_SET_BIT( z80.m_RegisterBC.hi, 7  ) ; break ;
		case 0xF9 : CPU_SET_BIT( z80.m_RegisterBC.lo, 7  ) ; break ;
		case 0xFA : CPU_SET_BIT( z80.m_RegisterDE.hi, 7  ) ; break ;
		case 0xFB : CPU_SET_BIT( z80.m_RegisterDE.lo, 7  ) ; break ;
		case 0xFC : CPU_SET_BIT( z80.m_RegisterHL.hi, 7  ) ; break ;
		case 0xFD : CPU_SET_BIT( z80.m_RegisterHL.lo, 7  ) ; break ;
		case 0xFE : CPU_SET_BIT_MEMORY( z80.m_RegisterHL.reg, 7 ) ; break ;
		case 0xFF : CPU_SET_BIT( z80.m_RegisterAF.hi, 7 ) ; break ;
		default: printf("EXTENDED OPCODE non reconnu:0x%08x", opcode); cyclesTime = 0;break;
	}
}

//OPCODE//

//ASM2C//

word ReadWord(){
    word res = ReadMemory(z80.m_ProgramCounter+1);
    res = res << 8;
    res |= ReadMemory(z80.m_ProgramCounter);
    return res;
}

void CPU_8BIT_LOAD(byte& reg){
    loopCounter += 8;
    byte n = ReadMemory(z80.m_ProgramCounter);
    z80.m_ProgramCounter++;
    reg = n;
}

void CPU_16BIT_LOAD(word& reg){
    loopCounter += 12;
    word n = ReadWord();
    z80.m_ProgramCounter+=2;
    reg = n;
}

void CPU_REG_LOAD(byte& reg, byte load, int cycles){
    loopCounter+=cycles;
    reg = load;
}

void CPU_REG_LOAD_ROM(byte& reg, word address){
    loopCounter+=8;
    reg = ReadMemory(address);
}

void CPU_16BIT_DEC(word& wOrd, int cycles){
    loopCounter+=cycles;
    wOrd--;
}

void CPU_16BIT_INC(word& wOrd, int cycles){
    loopCounter+=cycles;
    wOrd++;
}

word CPU_8BIT_SIGNED_ADD(word& reg, byte toAdd, int cycles){
	
	loopCounter+=cycles;
	
	signed_word toAdd_bsx = (signed_word)((signed_byte)toAdd);
	word result = reg + toAdd_bsx;
	
	z80.m_RegisterAF.lo = 0;
	
	//manually carry
	if((reg & 0xFF) + toAdd > 0xFF){
		z80.m_RegisterAF.lo |= 0x10;
	}
	
	//manually halfcarry
	if((reg & 0xF) + (toAdd & 0xF) > 0xF){
		z80.m_RegisterAF.lo |= 0x20;
	}
	
	return result;
}

void CPU_8BIT_ADD(byte& reg, byte toAdd, int cycles, bool useImmediate, bool addCarry){
    loopCounter+=cycles;
	
	if(!addCarry){
		byte before = reg;
		byte adding = 0;

		if(useImmediate){
			byte n = ReadMemory(z80.m_ProgramCounter);
			z80.m_ProgramCounter++;
			adding = n;
		}
		else{
			adding = toAdd;
		}

		reg+=adding;

		z80.m_RegisterAF.lo = 0;

		if(reg == 0){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
		}

		word htest = (before & 0xF);
		htest += (adding & 0xF);

		if(htest > 0xF){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H);
		}
		if((before + adding) > 0xFF){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
		}
	}
	else{
		byte carryFlag = 0;
		
		if(z80.m_RegisterAF.lo & 0x10){
			carryFlag = 1;
		}
		else{
			carryFlag = 0;
		}
		
		byte carrySet = 0;
		
		byte halfCarrySet = 0;
		
		byte temp1, temp2 = 0;
		
		
		temp1 = reg + carryFlag;
		
		if(reg + carryFlag > 0xFF){
			carrySet = 1;
		}
		if((reg & 0x0f)+(carryFlag & 0x0F) > 0x0F){
			halfCarrySet = 1;
		}
		
		temp2 = temp1 + toAdd;
		
		if(temp1 + toAdd > 0xFF){
			carrySet = 1;
		}
		if((temp1 & 0x0F) + (toAdd & 0x0F) > 0x0F){
			halfCarrySet = 1;
		}
		
		z80.m_RegisterAF.lo = 0;
		
		if(carrySet){
			z80.m_RegisterAF.lo |= 0x10;
		}
		if(halfCarrySet){
			z80.m_RegisterAF.lo |= 0x20;
		}
		if(!temp2){
			z80.m_RegisterAF.lo |= 0x80;
		}
		
		z80.m_RegisterAF.hi = temp2;
		
	}
}

void CPU_8BIT_SUB(byte& reg, byte subtracting, int cycles, bool useImmediate, bool subCarry){
    loopCounter+=cycles;
	
	if(!subCarry){
		byte before = reg;
		byte toSubtract = 0;

		if(useImmediate){
			byte n = ReadMemory(z80.m_ProgramCounter);
			z80.m_ProgramCounter++;
			toSubtract = n;
		}
		else{
			toSubtract = subtracting;
		}

		reg -= toSubtract;

		z80.m_RegisterAF.lo = 0;

		if(reg == 0){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
		}

		z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_N);

		if(before < toSubtract){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
		}
		signed_word htest = (before & 0xF);
		htest -= (toSubtract & 0xF);

		if(htest < 0){
			z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H);
		}
	}
	else{
		byte carryFlag = 0;
		
		if(z80.m_RegisterAF.lo & 0x10){
			carryFlag = 1;
		}
		else{
			carryFlag = 0;
		}
		
		byte carrySet = 0;
		
		byte halfCarrySet = 0;
		
		byte temp1, temp2 = 0;
		
		temp1 = reg - carryFlag;
		
		if(reg < carryFlag){
			carrySet = 1;
		}
		if((reg & 0x0f)<(carryFlag & 0x0F)){
			halfCarrySet = 1;
		}
		
		temp2 = temp1 - subtracting;
		
		if(temp1 < subtracting){
			carrySet = 1;
		}
		if((temp1 & 0x0F) < (subtracting & 0x0F)){
			halfCarrySet = 1;
		}
		
		z80.m_RegisterAF.lo = 0;
		
		if(carrySet){
			z80.m_RegisterAF.lo |= 0x10;
		}
		if(halfCarrySet){
			z80.m_RegisterAF.lo |= 0x20;
		}
		
		z80.m_RegisterAF.lo |= 0x40;
		
		if(!temp2){
			z80.m_RegisterAF.lo |= 0x80;
		}
		
		z80.m_RegisterAF.hi = temp2;
		
	}
}

void CPU_8BIT_AND(byte& reg, byte toAnd, int cycles, bool useImmediate){
    loopCounter+=cycles;
    byte myand = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        myand = n;
    }
    else{
        myand = toAnd;
    }

    reg &= myand;

    z80.m_RegisterAF.lo = 0;

    if(reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }

    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H);
}

void CPU_8BIT_OR(byte& reg, byte toOr, int cycles, bool useImmediate){
    loopCounter+=cycles;
    byte myor = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        myor = n;
    }
    else{
        myor = toOr;
    }

    reg |= myor;

    z80.m_RegisterAF.lo = 0;

    if(reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }
}

void CPU_8BIT_XOR(byte& reg, byte toXOr, int cycles, bool useImmediate){
    loopCounter+=cycles;
    byte myxor = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        myxor = n;
    }
    else{
        myxor = toXOr;
    }

    reg ^= myxor;

    z80.m_RegisterAF.lo = 0;

    if(reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }
}

void CPU_8BIT_COMPARE(byte reg, byte subtracting, int cycles, bool useImmediate){
    loopCounter+=cycles;
    byte before = reg;
    byte toSubtract = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        toSubtract = n;
    }
    else{
        toSubtract = subtracting;
    }

    reg -= toSubtract;

    z80.m_RegisterAF.lo = 0;

    if(reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }

    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_N);

    if(before < toSubtract){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
    }

    signed_word htest = before & 0xF;
    htest -= (toSubtract & 0xF);

    if(htest < 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H);
    }
}

void CPU_8BIT_INC(byte& reg, int cycles){


    loopCounter+= cycles ;

    byte before = reg ;

    reg++ ;

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }

    z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N) ;

    if ((before & 0xF) == 0xF){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
}

void CPU_8BIT_MEMORY_INC(word address, int cycles){


    loopCounter+= cycles ;

    byte before = ReadMemory( address ) ;
    WriteMemory(address, (before+1)) ;
    byte now =  before+1 ;

    if (now == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }

    z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N) ;

    if ((before & 0xF) == 0xF){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
}

void CPU_8BIT_DEC(byte& reg, int cycles){

    loopCounter+=cycles ;
    byte before = reg ;

    reg-- ;

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_N) ;

    if ((before & 0x0F) == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
}

void CPU_8BIT_MEMORY_DEC(word address, int cycles){

    loopCounter+=cycles ;
    byte before = ReadMemory(address) ;
    WriteMemory(address, (before-1)) ;
    byte now = before-1 ;

    if (now == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_N) ;

    if ((before & 0x0F) == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
}

void CPU_16BIT_ADD(word& reg, word toAdd, int cycles){
    loopCounter += cycles;
    
	byte zero_flag = 0;
	
	if(z80.m_RegisterAF.lo & 0x80){
		zero_flag = 1;
	}
	else{
		zero_flag = 0;
	}
	
	z80.m_RegisterAF.lo = 0;
	
	if(reg + toAdd > 0xFFFF){
		z80.m_RegisterAF.lo |= 0x10;
	}
	
	if((reg & 0x0FFF) + (toAdd & 0x0FFF) > 0x0FFF){
		z80.m_RegisterAF.lo |= 0x20;
	}
	
	if(zero_flag){
		z80.m_RegisterAF.lo |= 0x80;
	}
	
    reg += toAdd;

}

void CPU_JUMP(bool useCondition, int flag, bool condition){
    loopCounter += 12 ;

    word nn = ReadWord() ;
    z80.m_ProgramCounter += 2 ;

    if (!useCondition){
        z80.m_ProgramCounter = nn ;
        return ;
    }

    if (TestBit8(z80.m_RegisterAF.lo, flag) == condition){
        z80.m_ProgramCounter = nn ;
    }

}

void CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition){
    loopCounter += 8 ;

    signed_byte n = (signed_byte)ReadMemory(z80.m_ProgramCounter) ;

    if (!useCondition){
        z80.m_ProgramCounter += n;
    }
    else if (TestBit8(z80.m_RegisterAF.lo, flag) == condition){
		//printf("loop");
        z80.m_ProgramCounter += n ;
    }
	
    z80.m_ProgramCounter++ ;
}

void CPU_CALL(bool useCondition, int flag, bool condition){
    loopCounter+=12 ;
    word nn = ReadWord() ;
    z80.m_ProgramCounter += 2;

    if (!useCondition){
        PushWordOntoStack(z80.m_ProgramCounter) ;
        z80.m_ProgramCounter = nn ;
        return ;
    }

    if (TestBit8(z80.m_RegisterAF.lo, flag)==condition){
        PushWordOntoStack(z80.m_ProgramCounter) ;
        z80.m_ProgramCounter = nn ;
    }
}

void CPU_RETURN(bool useCondition, int flag, bool condition){
    loopCounter += 8 ;
    if (!useCondition){
        z80.m_ProgramCounter = PopWordOffStack();
        return ;
    }

    if (TestBit8(z80.m_RegisterAF.lo, flag) == condition){
        z80.m_ProgramCounter = PopWordOffStack();
    }
}

void CPU_SWAP_NIBBLES(byte& reg){
    loopCounter += 8 ;
	cyclesTime = 8;

    z80.m_RegisterAF.lo = 0 ;

    reg = (((reg & 0xF0) >> 4) | ((reg & 0x0F) << 4));

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SWAP_NIB_MEM(word address){
    loopCounter+= 16 ;
	cyclesTime = 16;

    z80.m_RegisterAF.lo = 0 ;

    byte mem = ReadMemory(address) ;
    mem = (((mem & 0xF0) >> 4) | ((mem & 0x0F) << 4));

    WriteMemory(address,mem) ;

    if (mem == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }

}

void CPU_RESTARTS(byte n){
    PushWordOntoStack(z80.m_ProgramCounter) ;
    loopCounter += 32 ;
    z80.m_ProgramCounter = n ;
}

void CPU_SHIFT_LEFT_CARRY(byte& reg){

    loopCounter += 8 ;
    z80.m_RegisterAF.lo = 0 ;
    if (TestBit8(reg,7)){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
    }

    reg = reg << 1 ;
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SHIFT_LEFT_CARRY_MEMORY(word address){

    loopCounter += 16 ;
    byte before = ReadMemory(address) ;

    z80.m_RegisterAF.lo = 0 ;
    if (TestBit8(before,7)){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    before = before << 1 ;
    if (before == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, before) ;
}

void CPU_RESET_BIT(byte& reg, int bit){

    reg = BitReset8(reg, bit) ;
    loopCounter += 8 ;
	cyclesTime = 8;
}

void CPU_RESET_BIT_MEMORY(word address, int bit){
	byte mem = ReadMemory(address) ;
	mem = BitReset8(mem, bit) ;
	WriteMemory(address, mem) ;
	loopCounter += 16 ;
	cyclesTime = 16;
}

void CPU_TEST_BIT(byte reg, int bit, int cycles){
    if (TestBit8(reg, bit)){
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    else{
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N) ;
    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;

    loopCounter += cycles ;
	cyclesTime = cycles;
}

void CPU_SET_BIT(byte& reg, int bit){
	reg = BitSet8(reg, bit) ;
	loopCounter += 8 ;
	cyclesTime = 8;
}

void CPU_SET_BIT_MEMORY(word address, int bit){

    byte mem = ReadMemory(address) ;
    mem = BitSet8(mem, bit) ;
    WriteMemory(address, mem) ;
    loopCounter += 16 ;
	cyclesTime = 16;
}

void CPU_DAA(){
    loopCounter += 4 ;

    int regA = z80.m_RegisterAF.hi;
	
	if(!(z80.m_RegisterAF.lo & 0x40)){
		if((z80.m_RegisterAF.lo & 0x20) || ((regA & 0xF) > 0x09)){
			regA += 0x06;
		}
		if((z80.m_RegisterAF.lo & 0x10) || (regA > 0x9F)){
			regA +=0x60;
		}
	}
	else{
		if(z80.m_RegisterAF.lo & 0x20){
			regA = (regA - 0x06) & 0xFF;
		}
		if(z80.m_RegisterAF.lo & 0x10){
			regA -= 0x60;
		}
	}
	
	if(regA & 0x100){
		z80.m_RegisterAF.lo |= 0x10;
	}
	regA &= 0xFF;
	
	z80.m_RegisterAF.lo &= ~0x20;
	
	if(regA == 0){
		z80.m_RegisterAF.lo |= 0x80;
	}
	else{
		z80.m_RegisterAF.lo &= ~0x80;
	}
	
	z80.m_RegisterAF.hi = (byte)regA;
}

void CPU_RR(byte& reg){

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isLSBSet = TestBit8(reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1 ;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        reg = BitSet8(reg, 7) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RR_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isLSBSet = TestBit8(reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1 ;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        reg = BitSet8(reg, 7) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}

void CPU_RLC(byte& reg){


    bool isMSBSet = TestBit8(reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg <<= 1;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        reg = BitSet8(reg,0) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RLC_MEMORY(word address){


    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isMSBSet = TestBit8(reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg <<= 1;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        reg = BitSet8(reg,0) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg);

}

void CPU_RRC(byte& reg){


    bool isLSBSet = TestBit8(reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        reg = BitSet8(reg,7) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RRC_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isLSBSet = TestBit8(reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        reg = BitSet8(reg,7) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}

void CPU_SLA(byte& reg){

    loopCounter += 8 ;
	cyclesTime = 8;

    bool isMSBSet = TestBit8(reg, 7);

    reg <<= 1;

    z80.m_RegisterAF.lo = 0 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SLA_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isMSBSet = TestBit8(reg, 7);

    reg <<= 1;

    z80.m_RegisterAF.lo = 0 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}

void CPU_SRA(byte& reg){

    loopCounter += 8 ;
	cyclesTime = 8;

    bool isLSBSet = TestBit8(reg,0) ;
    bool isMSBSet = TestBit8(reg,7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isMSBSet){
        reg = BitSet8(reg,7) ;
    }
    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }

    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SRA_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isLSBSet = TestBit8(reg,0) ;
    bool isMSBSet = TestBit8(reg,7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isMSBSet){
        reg = BitSet8(reg,7) ;
    }
    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}

void CPU_SRL(byte& reg){

    loopCounter += 8 ;
	cyclesTime = 8;

    bool isLSBSet = TestBit8(reg,0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SRL_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;

    byte reg = ReadMemory(address) ;

    bool isLSBSet = TestBit8(reg,0) ;

    z80.m_RegisterAF.lo = 0 ;

    reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}

void CPU_RL(byte& reg){

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isMSBSet = TestBit8(reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg <<= 1 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        reg = BitSet8(reg, 0) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RL_MEMORY(word address){

    loopCounter += 16 ;
	cyclesTime = 16;
    byte reg = ReadMemory(address) ;

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isMSBSet = TestBit8(reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    reg <<= 1 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        reg = BitSet8(reg, 0) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
    WriteMemory(address, reg) ;
}
//ASM2C//


