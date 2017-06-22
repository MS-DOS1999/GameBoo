#include <ctime>


void grabTime(){
	time_t system_time = time(0);
	tm* current_time = localtime(&system_time);
	
	rtc_reg[0] = current_time->tm_sec;
	
	if(rtc_reg[0] > 59){
		rtc_reg[0] = 59;
	}
	
	rtc_reg[0] = (rtc_reg[0] % 60);
	
	rtc_reg[1] = current_time->tm_min;
	rtc_reg[1] = (rtc_reg[1] % 60);
	
	rtc_reg[2] = current_time->tm_hour;
	rtc_reg[2] = (rtc_reg[2] % 24);
	
	word temp_day = current_time->tm_yday;
	temp_day = (temp_day % 366);
	
	rtc_reg[3] = temp_day & 0xFF;
	temp_day >>= 8;
	
	if(temp_day == 1){
		rtc_reg[4] |= 0x1;
	}
	else{
		rtc_reg[4] &= ~0x1;
	}
	
	for(int x = 0; x < 5; x++) {
		latch_reg[x] = rtc_reg[x];
	}
	
}


void Write_MBC3(word address, byte data){
	
	if(address < 0x2000){
		if ((data & 0xF) == 0xA){
            if(z80.m_ActiveRAM){
				z80.m_EnableRAM = true;
			}
			if(rtc){
				rtc_enabled = true;
			}
		}
		else{
			z80.m_EnableRAM = false;
			rtc_enabled = false;
		}
        
	}
	else if((address>=0x2000) && (address<0x4000)){
		if((data & 0x7F) == 0x00){
			z80.m_CurrentROMBank = 0x01;
		}
		else{
			z80.m_CurrentROMBank = (data & 0x7F);
		}
	}
	else if((address>=0x4000) && (address<0x6000)){
		if((data >= 0x08) && (data <= 0x0C)){
			rtc = true;
			z80.m_CurrentRAMBank = data;
		}
		if(data <= 0x03){
			rtc = false;
			z80.m_CurrentRAMBank = data;
		}
	}
	else if((address>=0x6000) && (address<0x8000)){
		if(rtc_enabled){
			if((rtc_latch_1 == 0xFF) && (data == 0)){
				rtc_latch_1 = 0;
			}
			else if((rtc_latch_2 == 0xFF) && (data == 1)){
				
				grabTime();
				
				rtc_latch_1 = rtc_latch_2 = 0xFF;
				
			}
		}
	}
	else if((address>=0xA000)&&(address<0xC000)){
        if((z80.m_EnableRAM) && (z80.m_CurrentRAMBank <= 3)){
			word newAddress = address - 0xA000;
			z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank*0x2000)] = data;
        }
		else if((rtc_enabled) && (z80.m_CurrentRAMBank >= 8) && (z80.m_CurrentRAMBank <= 12)){
			rtc_reg[z80.m_CurrentRAMBank - 8] = data;
		}
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
		z80.m_DividerRegister = 0;
    }
	else if(0xFF07 == address){
		data |= 0xF8;
		z80.m_Rom[address] = data;
	}
	else if(0xFF0F == address){
		data |= 0xE0;
		z80.m_Rom[address] = data;
	}
    else if(0xFF44 == address){
        z80.m_Rom[0xFF44] = 0;
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

byte Read_MBC3(word address){
	if((address>=0x4000) && (address<=0x7FFF)){
			word newAddress = address - 0x4000;
			return z80.m_CartridgeMemory[newAddress + ((z80.m_CurrentROMBank)*0x4000)];
    }
    else if((address>=0xA000) && (address<=0xBFFF)){
		if((z80.m_EnableRAM) && (z80.m_CurrentRAMBank <= 3)){
			word newAddress = address - 0xA000;
			return z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank * 0x2000)];
		}
		else if((rtc_enabled) && (z80.m_CurrentRAMBank >= 8) && (z80.m_CurrentRAMBank <= 12)){
			return rtc_reg[z80.m_CurrentRAMBank - 8];
		}
		else{
			return 0x00;
		}
    }
	else if (0xFF00 == address){
		return GetJoypadState();
	}
	else{
		return z80.m_Rom[address];
	}
}