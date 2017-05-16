#include "z80.h"
#include "bitUtils.c"

//INIT//
void initZ80(){


    memset(z80.m_CartridgeMemory,0,sizeof(z80.m_CartridgeMemory));
    memset(&z80.m_RAMBanks,0,sizeof(z80.m_RAMBanks));


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
    z80.m_Rom[0xFFFF] = 0x00 ;

    z80.m_CurrentROMBank = 1;
    z80.m_CurrentRAMBank = 0;
    z80.m_TimerCounter = 1024;
    z80.m_InteruptMaster = false;
    z80.m_DividerRegister = 0;
    z80.m_DividerCounter = 0;
    z80.m_ScanlineCounter = 0;

}

void DetectMapper(){

    z80.m_MBC1 = false;
    z80.m_MBC2 = false;

    switch (z80.m_CartridgeMemory[0x147])
    {
        case 1 : z80.m_MBC1 = true ; break ;
        case 2 : z80.m_MBC1 = true ; break ;
        case 3 : z80.m_MBC1 = true ; break ;
        case 5 : z80.m_MBC2 = true ; break ;
        case 6 : z80.m_MBC2 = true ; break ;
        default : break ;
    }
}

//INIT//


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
    else if((address>=0xA000)&&(address<0xC000)){
        if(z80.m_EnableRAM){
            word newAddress = address - 0xA000 ;
            z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank*0x2000)] = data;
        }
    }
    else if(TMC == address){
        byte currentfreq = GetClockFreq();
        z80.m_CartridgeMemory[TMC]=data;
        byte newFreq = GetClockFreq();
        if(currentfreq != newFreq){
            SetClockFreq();
        }
    }
    else if(0xFF04 == address){
        z80.m_Rom[0xFF04] = 0;
    }
    else if(0xFF44 == address){
        z80.m_Rom[0xFF44] = 0;
    }
    else if(0xFF46 == address){
        DoDMATransfer(data);
    }
}


byte ReadMemory(word address){


    //on read les données depuis le bank switch
    if((address>=0x4000) && (address<=0x7FFF)){

        //on enleve la valeur d'une bank pour récuperer la valeur d'origine
        word newAddress = address - 0x4000;
        //pour recuperer les données de la bonne BANK on multiply le numero de la BANK par 0x4000 ce qui nous positionne sur la bonne BANK et on ajoute la nouvelle adresse pour être sur la bonne donnée
        return z80.m_CartridgeMemory[newAddress + (z80.m_CurrentROMBank*0x4000)];
    }
    else if((address>=0xA000) && (address<=0xBFFF)){
        word newAddress = address - 0x2000;
        return z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank*0x2000)];
    }

    return z80.m_Rom[address];

}

void HandleBanking(word address, byte data){

    if (address < 0x2000){
        if (z80.m_MBC1 || z80.m_MBC2){
            DoRAMBankEnable(address,data);
        }
    }
    else if((address>=0x2000) && (address<0x4000)){
        if(z80.m_MBC1 || z80.m_MBC2){
            DoChangeLoROMBank(data);
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
    }
    else if((address>=0x6000) && (address<0x8000)){
        if(z80.m_MBC1){
            DoChangeROMRAMMode(data);
        }
    }

}

void DoRAMBankEnable(word address, byte data){
    if(z80.m_MBC2){
        if(TestBit16(address, 4)==1){
            return;
        }

        byte testData = data & 0xF;
        if(testData == 0xA){
            z80.m_EnableRAM = true;
        }
        else if(testData == 0x0){
            z80.m_EnableRAM = false;
        }
    }
}

void DoChangeLoROMBank(byte data){
    if(z80.m_MBC2){
        z80.m_CurrentROMBank = data & 0xF;
        if(z80.m_CurrentROMBank == 0){
            z80.m_CurrentROMBank++;
        }
        return;
    }
    byte lower5 = data & 31;
    z80.m_CurrentROMBank &= 224;
    z80.m_CurrentROMBank |= lower5;
    if(z80.m_CurrentROMBank == 0){
        z80.m_CurrentROMBank++;
    }
}

void DoChangeHiROMBank(byte data){
    z80.m_CurrentROMBank &= 31;

    data &= 224;

    z80.m_CurrentROMBank |= data;

    if(z80.m_CurrentROMBank == 0){
        z80.m_CurrentROMBank++;
    }

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

    if(z80.m_RomBanking){
        z80.m_CurrentRAMBank = 0;
    }
}
//MEMORY READ/WRITE//


//TIMERS//

void UpdateTimers(int cycles){
    DoDividerRegister(cycles);

    if(IsClockEnabled()){
        z80.m_TimerCounter -= cycles;

        if(z80.m_TimerCounter <= 0){
            SetClockFreq();

            if(ReadMemory(TIMA) == 255){
                WriteMemory(TIMA, ReadMemory(TMA));
                RequestInterupts(2);
            }
            else{
                WriteMemory(TIMA, ReadMemory(TIMA)+1);
            }
        }
    }
}

bool IsClockEnabled(){
    if(TestBit8(ReadMemory(TMC),2)){
        return true;
    }
    else{
        return false;
    }
}

byte GetClockFreq(){
    return ReadMemory(TMC) & 0x3;
}

void SetClockFreq(){
    byte freq = GetClockFreq();

    switch(freq){
        case 0:
            z80.m_TimerCounter = 1024;
            break;
        case 1:
            z80.m_TimerCounter = 16;
            break;
        case 2:
            z80.m_TimerCounter = 64;
            break;
        case 3:
            z80.m_TimerCounter = 256;
            break;
        default:
            printf("frequency has not been changed");
            break;
    }
}

void DoDividerRegister(int cycles){
    z80.m_DividerRegister += cycles;
    if(z80.m_DividerCounter >= 255){
        z80.m_DividerCounter = 0;
        z80.m_Rom[0xFF04]++;
    }
}
//TIMERS//


//INTERUPTS//

void RequestInterupts(int id){
    byte req = ReadMemory(0xFF0F);
    req = BitSet8(req, id);
    WriteMemory(0xFF0F, id);
}

void DoInterupts(){
    if(z80.m_InteruptMaster == true){
        byte req = ReadMemory(0xFF0F);
        byte Enabled = ReadMemory(0xFFFF);
        if(req > 0){
            int i;
            for(i = 0; i < 5; i++){
                if(TestBit8(req, i) == true){
                    if(TestBit8(Enabled, i) == true){
                        ServiceInterupt(i);
                    }
                }
            }
        }
    }
}

void ServiceInterupt(int interupt){
    z80.m_InteruptMaster = false;
    byte req = ReadMemory(0xFF0F);
    req = BitReset8(req, interupt);
    WriteMemory(0xFF0F, req);

    PushWordOntoStack(z80.m_ProgramCounter);

    switch(interupt){
        case 0:
            z80.m_ProgramCounter = 0x40;
            break;
        case 1:
            z80.m_ProgramCounter = 0x48;
            break;
        case 2:
            z80.m_ProgramCounter = 0x50;
            break;
        case 4:
            z80.m_ProgramCounter = 0x60;
            break;
        default:
            printf("no interupt");
            break;
    }
}

//INTERUPTS//


//LCD//

void UpdateGraphics(int cycles){
    SetLCDStatus();

    if(IsLCDEnabled()){
        z80.m_ScanlineCounter -= cycles;
    }
    else{
        return;
    }

    if(z80.m_ScanlineCounter <= 0){
        z80.m_Rom[0xFF44]++;
        byte currentline = ReadMemory(0xFF44);
        z80.m_ScanlineCounter = 456;

        if(currentline == 144){
            RequestInterupts(0);
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
    byte mode = 0;
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
            status = TestBit8(status, 5);
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
            status = TestBit8(status, 3);
        }
    }

    if(reqInt && (mode != currentmode)){
        RequestInterupts(1);
    }

    byte ly = ReadMemory(0xFF44);

    if(ly == ReadMemory(0xFF45)){
        status = BitSet8(status, 2);
        if(TestBit8(status, 6)){
            RequestInterupts(1);
        }
    }
    else{
        status = BitSet8(status, 2);
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
    if(TestBit8(control, 1)){
        RenderSprites(control);
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
    z80.m_ProgramCounter++;
    ExecuteOpcode(opcode);
}


void ExecuteOpcode(byte opcode){
	/*
    switch(opcode){
        case 0x00:
            cyclesTime = 4;
            loopCounter += 4;break;
        case 0x01:
            cyclesTime = 12;
            CPU_16BIT_LOAD(z80.m_RegisterBC.reg);break;
        case 0x02:
            cyclesTime = 8;
            WriteMemory(z80.m_RegisterBC.reg, z80.m_RegisterAF.hi);loopCounter+=8;break;
        case 0x03:
            cyclesTime = 8;
            CPU_16BIT_INC(z80.m_RegisterBC.reg, 8);break;
        case 0x04:
            cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterBC.hi, 4);break;
        case 0x05:
            cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterBC.hi, 4);break;
        case 0x06:
            cyclesTime = 8;
            CPU_8BIT_LOAD(z80.m_RegisterBC.hi);break;
        case 0x07:
            cyclesTime = 4;
            CPU_RLC(z80.m_RegisterAF.hi);break;
        case 0x08:
            cyclesTime = 20;
            word nn = ReadWord();
            z80.m_programCounter+=2;
            WriteMemory(nn, z80.m_Stackpointer.lo);
            n++;
            WriteMemory(nn, z80.m_StackPointer.hi);
            loopCounter += 20;
            break;
        case 0x09:
            cyclesTime = 8;
            CPU_16BIT_ADD(z80.m_RegisterHL.reg, z80.m_RegisterBC.reg, 8);break;
        case 0x0A:
            cyclesTime = 8;
            CPU_REG_LOAD_ROM(z80.m_RegisterAF.hi, z80.m_RegisterBC.reg);break;
        case 0x0B:
            cyclesTime = 8;
            CPU_16BIT_DEC(z80.m_RegisterBC.reg, 8);break;
        case 0x0C:
            cyclesTime = 4;
            CPU_8BIT_INC(z80.m_RegisterBC.lo);break;
        case 0x0D:
            cyclesTime = 4;
            CPU_8BIT_DEC(z80.m_RegisterBC.lo);break;
        default:
            break;
    }*/
}

//OPCODE//

//ASM2C//
/*
word ReadWord(){
    word res = ReadMemory(z80.m_ProgramCounter+1);
    res = res << 8;
    res |= ReadMemory(z80.m_ProgramCounter);
    return res;
}

void CPU_8BIT_LOAD(byte* reg){
    loopCounter += 8;
    byte n = ReadMemory(z80.m_ProgramCounter);
    z80.m_ProgramCounter++;
    *reg = n;
}

void CPU_16BIT_LOAD(word* reg){
    loopCounter += 12;
    word n = ReadWord();
    z80.m_ProgramCounter+=2;
    *reg = n;
}

void CPU_REG_LOAD(byte* reg, byte load, int cycles){
    loopCounter+=cycles;
    *reg = load;
}

void CPU_REG_LOAD_ROM(byte* reg, word address){
    loopCounter+=8;
	byte temp = ReadMemory(address);
    *reg = temp;
}

void CPU_16BIT_DEC(word* wOrd, int cycles){
    loopCounter+=cycles;
    wOrd--;
}

void CPU_16BIT_INC(word* wOrd, int cycles){
    loopCounter+=cycles;
    wOrd++;
}

void CPU_8BIT_ADD(byte* reg, byte toAdd, int cycles, bool useImmediate, bool addCarry){
    loopCounter+=cycles;
    byte before = *reg;
    byte adding = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        adding = n;
    }
    else{
        adding = toAdd;
    }

    if(addCarry){
        if(TestBit8(z80.m_RegisterAF.lo, FLAG_C)){
            adding++;
        }
    }

    *reg+=adding;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
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

void CPU_8BIT_SUB(byte* reg, byte subtracting, int cycles, bool useImmediate, bool subCarry){
    loopCounter+=cycles;
    byte before = *reg;
    byte toSubtract = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        toSubtract = n;
    }
    else{
        toSubtract = subtracting;
    }

    if(subCarry){
        if(TestBit8(z80.m_RegisterAF.lo, FLAG_C)){
            toSubtract++;
        }
    }

    *reg -= toSubtract;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
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

void CPU_8BIT_AND(byte* reg, byte toAnd, int cycles, bool useImmediate){
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

    *reg &= myand;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }

    z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H);
}

void CPU_8BIT_OR(byte* reg, byte toOr, int cycles, bool useImmediate){
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

    *reg |= myor;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }
}

void CPU_8bit_XOR(byte* reg, byte toXOr, int cycles, bool useImmediate){
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

    *reg ^= myxor;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z);
    }
}

void CPU_8BIT_COMPARE(byte* reg, byte subtracting, int cycles, bool useImmediate){
    loopCounter+=cycles;
    byte before = *reg;
    byte toSubtract = 0;

    if(useImmediate){
        byte n = ReadMemory(z80.m_ProgramCounter);
        z80.m_ProgramCounter++;
        toSubtract = n;
    }
    else{
        toSubtract = subtracting;
    }

    *reg -= toSubtract;

    z80.m_RegisterAF.lo = 0;

    if(*reg == 0){
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

void CPU_8BIT_INC(byte* reg, int cycles){


    loopCounter+= cycles ;

    byte before = *reg ;

    reg++ ;

    if (*reg == 0){
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

void CPU_8BIT_DEC(byte* reg, int cycles){

    loopCounter+=cycles ;
    byte before = *reg ;

    reg-- ;

    if (*reg == 0){
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

void CPU_16BIT_ADD(word* reg, word toAdd, int cycles){
    loopCounter += cycles;
    word before = *reg;

    *reg += toAdd ;

    z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_N) ;

    if ((before + toAdd) > 0xFFFF){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_C) ;
    }

    if (( (before & 0xFF00) & 0xF) + ((toAdd >> 8) & 0xF)){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
    else{
        z80.m_RegisterAF.lo = BitReset8(z80.m_RegisterAF.lo, FLAG_H) ;
    }
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
        z80.m_ProgramCounter = PopWordOffStack( ) ;
        return ;
    }

    if (TestBit8(z80.m_RegisterAF.lo, flag) == condition){
        z80.m_ProgramCounter = PopWordOffStack() ;
    }
}

void CPU_SWAP_NIBBLES(byte* reg)
{
    loopCounter += 8 ;

    z80.m_RegisterAF.lo = 0 ;

    *reg = (((*reg & 0xF0) >> 4) | ((*reg & 0x0F) << 4));

    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SWAP_NIB_MEM(word address){
    loopCounter+= 16 ;

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

void CPU_SHIFT_LEFT_CARRY(byte* reg){

    loopCounter += 8 ;
    z80.m_RegisterAF.lo = 0 ;
    if (TestBit8(*reg,7)){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C);
    }

    *reg = *reg << 1 ;
    if (*reg == 0){
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

void CPU_RESET_BIT(byte* reg, int bit){

    *reg = BitReset8(*reg, bit) ;
    loopCounter += 8 ;
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
}

void CPU_SET_BIT_MEMORY(word address, int bit){

    byte mem = ReadMemory(address) ;
    mem = BitSet8(mem, bit) ;
    WriteMemory(address, mem) ;
    loopCounter += 16 ;
}

void CPU_DAA(){
    loopCounter += 4 ;

    if(TestBit8(z80.m_RegisterAF.lo, FLAG_N)){
        if((z80.m_RegisterAF.hi &0x0F ) >0x09 || z80.m_RegisterAF.lo &0x20 ){
            z80.m_RegisterAF.hi -=0x06;
            if((z80.m_RegisterAF.hi&0xF0)==0xF0){
                z80.m_RegisterAF.lo |= 0x10;
            }
            else{
                z80.m_RegisterAF.lo &= ~0x10;
            }
        }

        if((z80.m_RegisterAF.hi&0xF0)>0x90 || z80.m_RegisterAF.lo&0x10){
            z80.m_RegisterAF.hi-=0x60;
        }
    }
    else{
        if((z80.m_RegisterAF.hi&0x0F)>9 || z80.m_RegisterAF.lo&0x20){
            z80.m_RegisterAF.hi+=0x06;
            if((z80.m_RegisterAF.hi&0xF0)==0){
                z80.m_RegisterAF.lo|=0x10;
            }
            else{
                z80.m_RegisterAF.lo&=~0x10;
            }
        }

        if((z80.m_RegisterAF.hi&0xF0)>0x90 || z80.m_RegisterAF.lo&0x10){
            z80.m_RegisterAF.hi+=0x60;
        }
    }

    if(z80.m_RegisterAF.hi==0){
        z80.m_RegisterAF.lo|=0x80;
    }
    else{
        z80.m_RegisterAF.lo&=~0x80;
    }
}

void CPU_RR(byte* reg){

    loopCounter += 8 ;

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isLSBSet = TestBit8(*reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg >>= 1 ;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        *reg = BitSet8(*reg, 7) ;
    }
    if (reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RR_MEMORY(word address){

    loopCounter += 16 ;

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

void CPU_RLC(byte* reg){

    loopCounter += 8 ;

    bool isMSBSet = TestBit8(*reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg <<= 1;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        *reg = BitSet8(*reg,0) ;
    }

    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RLC_MEMORY(word address){


    loopCounter += 16 ;

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

void CPU_RRC(byte* reg){

    loopCounter += 8 ;

    bool isLSBSet = TestBit8(*reg, 0) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
        *reg = BitSet8(*reg,7) ;
    }

    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RRC_MEMORY(word address)
{

    loopCounter += 16 ;

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

void CPU_SLA(byte* reg){

    loopCounter += 8 ;

    bool isMSBSet = TestBit8(*reg, 7);

    *reg <<= 1;

    z80.m_RegisterAF.lo = 0 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }

    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SLA_MEMORY(word address){

    loopCounter += 16 ;

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

void CPU_SRA(byte* reg){

    loopCounter += 8 ;

    bool isLSBSet = TestBit8(*reg,0) ;
    bool isMSBSet = TestBit8(*reg,7) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg >>= 1;

    if (isMSBSet){
        *reg = BitSet8(*reg,7) ;
    }
    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }

    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SRA_MEMORY(word address){

    loopCounter += 16 ;

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

void CPU_SRL(byte* reg){

    loopCounter += 8 ;

    bool isLSBSet = TestBit8(*reg,0) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg >>= 1;

    if (isLSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_SRL_MEMORY(word address){

    loopCounter += 8 ;

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

void CPU_RL(byte* reg){

    loopCounter += 8 ;

    bool isCarrySet = TestBit8(z80.m_RegisterAF.lo, FLAG_C) ;
    bool isMSBSet = TestBit8(*reg, 7) ;

    z80.m_RegisterAF.lo = 0 ;

    *reg <<= 1 ;

    if (isMSBSet){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_C) ;
    }
    if (isCarrySet){
        *reg = BitSet8(*reg, 0) ;
    }
    if (*reg == 0){
        z80.m_RegisterAF.lo = BitSet8(z80.m_RegisterAF.lo, FLAG_Z) ;
    }
}

void CPU_RL_MEMORY(word address){

    loopCounter += 16 ;
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
}*/
//ASM2C//


