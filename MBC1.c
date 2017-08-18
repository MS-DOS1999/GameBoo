

void Write_MBC1(word address, byte data){
	
	if(address < 0x2000){
		if ((data & 0xF) == 0xA){
            z80.m_EnableRAM = true ;
		}
        else if (data == 0x0){
           z80.m_EnableRAM = false ;
		}
	}
	else if((address>=0x2000) && (address<0x4000)){
		if (data == 0x00){
			data++;
		}

		data &= 31;

		z80.m_CurrentROMBank &= 224;

		z80.m_CurrentROMBank |= data;
	}
	else if((address>=0x4000) && (address<0x6000)){
		if(z80.m_RomBanking){
			z80.m_CurrentRAMBank = 0;
	
			data &= 3;
			data <<=5;
	
			if((z80.m_CurrentROMBank & 31) == 0){
				data++;
			}
	
			z80.m_CurrentROMBank &= 31;
	
			z80.m_CurrentROMBank |= data;
		}
		else{
			z80.m_CurrentRAMBank = data & 0x3;
		}
	}
	else if((address>=0x6000) && (address<0x8000)){
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
	else if((address>=0xA000)&&(address<0xC000)){
        if(z80.m_EnableRAM){
			word newAddress = address - 0xA000;
			z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank*0x2000)] = data;
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
	else if(address >= 0xFF10 && address <= 0xFF26){
		apu.write_register( address, data );
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

byte Read_MBC1(word address){
	if((address>=0x4000) && (address<=0x7FFF)){
			word newAddress = address - 0x4000;
			return z80.m_CartridgeMemory[newAddress + ((z80.m_CurrentROMBank)*0x4000)];
    }
    else if((address>=0xA000) && (address<=0xBFFF)){
        word newAddress = address - 0xA000;
        return z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank * 0x2000)];
    }
	else if (0xFF00 == address){
		return GetJoypadState();
	}
	else if(address >= 0xFF10 && address <= 0xFF26){
		return apu.read_register( address );
	}
	else{
		return z80.m_Rom[address];
	}
}



