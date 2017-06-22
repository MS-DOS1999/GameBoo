

void Write_NOROM(word address, byte data){
	if(address < 0x8000){
	//restricted area	
	}
	else if((address>=0xA000)&&(address<0xC000)){
        if(z80.m_ActiveRAM){
			word newAddress = address - 0xA000;
			z80.m_RAMBanks[newAddress] = data;
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

byte Read_NOROM(word address){
    if((address>=0xA000) && (address<=0xBFFF)){
        word newAddress = address - 0xA000;
        return z80.m_RAMBanks[newAddress + (z80.m_CurrentRAMBank * 0x2000)];
    }
	else if (0xFF00 == address){
		return GetJoypadState();
	}
	else{
		return z80.m_Rom[address];
	}
}


