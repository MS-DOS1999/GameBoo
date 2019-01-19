#include "sharpLr.hpp"

SharpLr::SharpLr()
{

}

SharpLr& SharpLr::get(void)
{
	static SharpLr instance;
	return instance;
}

void SharpLr::Init()
{
	get().programCounter = 0x100;
	get().registerAF.reg = 0x01B0;
	get().registerBC.reg = 0x0013;
	get().registerDE.reg = 0x00D8;
	get().registerHL.reg = 0x014D;
	get().stackPointer.reg = 0xFFFE;

	get().timerCounter = 0;
	get().timerSpeed = 1024;
	get().interruptMaster = false;
	get().interruptDelay = false;
	get().dividerRegister = 0;
	get().halted = false;
	get().skipInstruction = false;
}

void SharpLr::PushWord(word value)
{
	byte hi = value >> 8;
	byte lo = value & 0xFF;

	get().stackPointer.reg--;
	Emu::WriteMemory(get().stackPointer.reg, hi);
	get().stackPointer.reg--;
	Emu::WriteMemory(get().stackPointer.reg, lo);
}

word SharpLr::PopWord()
{
	word value = Emu::ReadMemory(get().stackPointer.reg+1) << 8;
	value |= Emu::ReadMemory(get().stackPointer.reg);
	get().stackPointer.reg += 2;

	return value;
}

word SharpLr::FetchWord()
{
	word res = Emu::ReadMemory(get().programCounter+1);
	res = res << 8;
	res |= Emu::ReadMemory(get().programCounter);
	return res;
}

byte SharpLr::FetchByte()
{
	byte res = Emu::ReadMemory(get().programCounter);
	return res;
}

void SharpLr::UpdateTimer(int cycles)
{
	get().DoDividerRegister(cycles);

	if(get().IsClockEnabled())
	{
		get().timerCounter += cycles;

		get().SetClockFrequence();

		if(get().timerCounter >= get().timerSpeed)
		{
			Emu::WriteDirectMemory(TIMA, Emu::ReadDirectMemory(TIMA) + 1);
			get().timerCounter -= get().timerSpeed;

			if(Emu::ReadDirectMemory(TIMA) == 0)
			{
				Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) | 0x04);
				Emu::WriteDirectMemory(TIMA, Emu::ReadDirectMemory(TMA));
			}
		}
	}
}

bool SharpLr::IsClockEnabled()
{
	return Emu::ReadMemory(TMC) & 0x4;
}

byte SharpLr::GetClockFrequence()
{
	return Emu::ReadMemory(TMC) & 0x3;
}

void SharpLr::SetClockFrequence()
{
	byte frequence = get().GetClockFrequence();

	switch(frequence)
	{
		case 0: get().timerSpeed = 1024; break;

		case 1: get().timerSpeed = 16;   break;

		case 2: get().timerSpeed = 64;   break;

		case 3: get().timerSpeed = 256;  break;
	}
}

void SharpLr::SetDividerRegister(int value)
{
	get().dividerRegister = value;
}

void SharpLr::DoDividerRegister(int cycles)
{
	get().dividerRegister += cycles;
	if(get().dividerRegister >= 256)
	{
		get().dividerRegister -= 256;
		Emu::WriteDirectMemory(0xFF04, Emu::ReadDirectMemory(0xFF04) + 1);
	}
}

void SharpLr::UpdateInterrupts()
{
	if(get().interruptDelay)
	{
		get().interruptDelay = false;
		get().interruptMaster = true;
	}
	else if(get().interruptMaster)
	{
		//Vblank Interrupt
		if((Emu::ReadDirectMemory(0xFFFF) & 0x1) && (Emu::ReadDirectMemory(0xFF0F) & 0x1))
		{
			get().interruptMaster = false;
			get().halted = false;
			Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) & ~0x01);
			get().PushWord(get().programCounter);
			get().programCounter = 0x40;
		}

		//LCD Status Interrupt
		if((Emu::ReadDirectMemory(0xFFFF) & 0x2) && (Emu::ReadDirectMemory(0xFF0F) & 0x2))
		{
			get().interruptMaster = false;
			get().halted = false;
			Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) & ~0x02);
			get().PushWord(get().programCounter);
			get().programCounter = 0x48;
		}

		//Timer Overflow Interrupt
		if((Emu::ReadDirectMemory(0xFFFF) & 0x4) && (Emu::ReadDirectMemory(0xFF0F) & 0x4))
		{
			get().interruptMaster = false;
			get().halted = false;
			Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) & ~0x04);
			get().PushWord(get().programCounter);
			get().programCounter = 0x50;
		}

		//Serial Interrupt
		if((Emu::ReadDirectMemory(0xFFFF) & 0x8) && (Emu::ReadDirectMemory(0xFF0F) & 0x8))
		{
			get().interruptMaster = false;
			get().halted = false;
			Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) & ~0x08);
			get().PushWord(get().programCounter);
			get().programCounter = 0x58;
		}

		//Joypad Interrupt
		if((Emu::ReadDirectMemory(0xFFFF) & 0x10) && (Emu::ReadDirectMemory(0xFF0F) & 0x10))
		{
			get().interruptMaster = false;
			get().halted = false;
			Emu::WriteDirectMemory(0xFF0F, Emu::ReadDirectMemory(0xFF0F) & ~0x10);
			get().PushWord(get().programCounter);
			get().programCounter = 0x60;
		}
	}
	else if((Emu::ReadDirectMemory(0xFF0F) & Emu::ReadDirectMemory(0xFFFF) & 0x1F) && (!get().skipInstruction))
	{
		get().halted = false;
	}
}

int SharpLr::ExecuteNextOpcode()
{
	byte opcode = Emu::ReadMemory(get().programCounter);
	int OpcodeCycles = 0;
	if(!get().halted)
	{
		get().programCounter++;
		OpcodeCycles = get().ExecuteOpcode(opcode);
	}
	else
	{
		if(get().interruptMaster || !get().skipInstruction)
		{
			OpcodeCycles = 4;
		}
		else if(get().skipInstruction)
		{
			get().halted = false;
			get().skipInstruction = false;

			OpcodeCycles = get().ExecuteOpcode(opcode);
		}
	}

	return OpcodeCycles;
}

int SharpLr::ExecuteOpcode(byte opcode)
{
	int OpcodeCycles = 0;

	switch(opcode)
	{
		case 0x00:
		{
			OpcodeCycles = 4;
			break;
		}
		case 0x01:
		{
			OpcodeCycles = 12;
			get()._16BitLoad(get().registerBC.reg);
			break;
		}
		case 0x02:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerBC.reg, get().registerAF.hi);
			break;
		}
		case 0x03:
		{
			OpcodeCycles = 8;
			get()._16BitInc(get().registerBC.reg);
			break;
		}
		case 0x04:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerBC.hi);
			break;
		}
		case 0x05:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerBC.hi);
			break;
		}
		case 0x06:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerBC.hi);
			break;
		}
		case 0x07:
		{
			OpcodeCycles = 4;
			get()._Rlc(get().registerAF.hi);
			get().registerAF.lo &= ~0x80;
			break;
		}
		case 0x08:
		{
			OpcodeCycles = 20;
			word ldValue = get().FetchWord();
			get().programCounter += 2;
			Emu::WriteMemory(ldValue, get().stackPointer.lo);
			ldValue++;
			Emu::WriteMemory(ldValue, get().stackPointer.hi);
			break;
		}
		case 0x09:
		{
			OpcodeCycles = 8;
			get()._16BitAdd(get().registerHL.reg, get().registerBC.reg);
			break;
		}
		case 0x0A:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, get().registerBC.reg);
			break;
		}
		case 0x0B:
		{
			OpcodeCycles = 8;
			get()._16BitDec(get().registerBC.reg);
			break;
		}
		case 0x0C:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerBC.lo);
			break;
		}
		case 0x0D:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerBC.lo);
			break;
		}
		case 0x0E:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerBC.lo);
			break;
		}
		case 0x0F:
		{
			OpcodeCycles = 8;
			get()._Rrc(get().registerAF.hi);
			get().registerAF.lo &= ~0x80;
			break;
		}
		case 0x10:
		{
			OpcodeCycles = 4;
			get().programCounter++;
			break;
		}
		case 0x11:
		{
			OpcodeCycles = 12;
			get()._16BitLoad(get().registerDE.reg);
			break;
		}
		case 0x12:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerDE.reg, get().registerAF.hi);
			break;
		}
		case 0x13:
		{
			OpcodeCycles = 8;
			get()._16BitInc(get().registerDE.reg);
			break;
		}
		case 0x14:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerDE.hi);
			break;
		}
		case 0x15:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerDE.hi);
			break;
		}
		case 0x16:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerDE.hi);
			break;
		}
		case 0x17:
		{
			OpcodeCycles = 8;
			get()._Rl(get().registerAF.hi);
			get().registerAF.lo &= ~0x80;
			break;
		}
		case 0x18:
		{
			OpcodeCycles = 8;
			get()._JumpImmediate(false, 0, false);
			break;
		}
		case 0x19:
		{
			OpcodeCycles = 8;
			get()._16BitAdd(get().registerHL.reg, get().registerDE.reg);
			break;
		}
		case 0x1A:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, get().registerDE.reg);
			break;
		}
		case 0x1B:
		{
			OpcodeCycles = 8;
			get()._16BitDec(get().registerDE.reg);
			break;
		}
		case 0x1C:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerDE.lo);
			break;
		}
		case 0x1D:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerDE.lo);
			break;
		}
		case 0x1E:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerDE.lo);
			break;
		}
		case 0x1F:
		{
			OpcodeCycles = 8;
			get()._Rr(get().registerAF.hi);
			get().registerAF.lo &= ~0x80;
			break;
		}
		case 0x20:
		{
			OpcodeCycles = 8;
			get()._JumpImmediate(true, Z_Flag, false);
			break;
		}
		case 0x21:
		{
			OpcodeCycles = 12;
			get()._16BitLoad(get().registerHL.reg);
			break;
		}
		case 0x22:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerAF.hi);
			get()._16BitInc(get().registerHL.reg);
			break;
		}
		case 0x23:
		{
			OpcodeCycles = 8;
			get()._16BitInc(get().registerHL.reg);
			break;
		}
		case 0x24:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerHL.hi);
			break;
		}
		case 0x25:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerHL.hi);
			break;
		}
		case 0x26:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerHL.hi);
			break;
		}
		case 0x27:
		{
			OpcodeCycles = 4;
			get()._Daa();
			break;
		}
		case 0x28:
		{
			OpcodeCycles = 8;
			get()._JumpImmediate(true, Z_Flag, true);
			break;
		}
		case 0x29:
		{
			OpcodeCycles = 8;
			get()._16BitAdd(get().registerHL.reg, get().registerHL.reg);
			break;
		}
		case 0x2A:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, get().registerHL.reg);
			get()._16BitInc(get().registerHL.reg);
			break;
		}
		case 0x2B:
		{
			OpcodeCycles = 8;
			get()._16BitDec(get().registerHL.reg);
			break;
		}
		case 0x2C:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerHL.lo);
			break;
		}
		case 0x2D:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerHL.lo);
			break;
		}
		case 0x2E:
		{
			OpcodeCycles = 8;
			get()._8BitLoad(get().registerHL.lo);
			break;
		}
		case 0x2F:
		{
			OpcodeCycles = 4;
			get().registerAF.hi ^= 0xFF;
			get().registerAF.lo = BitSet(get().registerAF.lo, N_Flag);
			get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
			break;
		}
		case 0x30:
		{
			OpcodeCycles = 8;
			get()._JumpImmediate(true, C_Flag, false);
			break;
		}
		case 0x31:
		{
			OpcodeCycles = 12;
			get()._16BitLoad(get().stackPointer.reg);
			break;
		}
		case 0x32:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerAF.hi);
			get()._16BitDec(get().registerHL.reg);
			break;
		}
		case 0x33:
		{
			OpcodeCycles = 8;
			get()._16BitInc(get().stackPointer.reg);
			break;
		}
		case 0x34:
		{
			OpcodeCycles = 12;
			get()._8BitMemoryInc(get().registerHL.reg);
			break;
		}
		case 0x35:
		{
			OpcodeCycles = 12;
			get()._8BitMemoryDec(get().registerHL.reg);
			break;
		}
		case 0x36:
		{
			OpcodeCycles = 12;
			byte ldValue = get().FetchByte();
			get().programCounter++;
			Emu::WriteMemory(get().registerHL.reg, ldValue);
			break;
		}
		case 0x37:
		{
			OpcodeCycles = 4;
			get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
			get().registerAF.lo = BitClear(get().registerAF.lo, H_Flag);
			get().registerAF.lo = BitClear(get().registerAF.lo, N_Flag);
			break;
		}
		case 0x38:
		{
			OpcodeCycles = 8;
			get()._JumpImmediate(true, C_Flag, true);
			break;
		}
		case 0x39:
		{
			OpcodeCycles = 8;
			get()._16BitAdd(get().registerHL.reg, get().stackPointer.reg);
			break;
		}
		case 0x3A:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, get().registerHL.reg);
			get()._16BitDec(get().registerHL.reg);
			break;
		}
		case 0x3B:
		{
			OpcodeCycles = 8;
			get()._16BitDec(get().stackPointer.reg);
			break;
		}
		case 0x3C:
		{
			OpcodeCycles = 4;
			get()._8BitInc(get().registerAF.hi);
			break;
		}
		case 0x3D:
		{
			OpcodeCycles = 4;
			get()._8BitDec(get().registerAF.hi);
			break;
		}
		case 0x3E:
		{
			OpcodeCycles = 8;
			byte ldValue = get().FetchByte();
			get().programCounter++;
			get().registerAF.hi = ldValue;
			break;
		}
		case 0x3F:
		{
			OpcodeCycles = 4;
			if(TestBit(get().registerAF.lo, C_Flag))
			{
				get().registerAF.lo = BitClear(get().registerAF.lo, C_Flag);
			}
			else
			{
				get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
			}

			get().registerAF.lo = BitClear(get().registerAF.lo, H_Flag);
			get().registerAF.lo = BitClear(get().registerAF.lo, N_Flag);
			break;
		}
		case 0x40:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerBC.hi);
			break;
		}
		case 0x41:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerBC.lo);
			break;
		}
		case 0x42:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerDE.hi);
			break;
		}
		case 0x43:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerDE.lo);
			break;
		}
		case 0x44:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerHL.hi);
			break;
		}
		case 0x45:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerHL.lo);
			break;
		}
		case 0x46:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerBC.hi, get().registerHL.reg);
			break;
		}
		case 0x47:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.hi, get().registerAF.hi);
			break;
		}
		case 0x48:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerBC.hi);
			break;
		}
		case 0x49:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerBC.lo);
			break;
		}
		case 0x4A:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerDE.hi);
			break;
		}
		case 0x4B:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerDE.lo);
			break;
		}
		case 0x4C:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerHL.hi);
			break;
		}
		case 0x4D:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerHL.lo);
			break;
		}
		case 0x4E:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerBC.lo, get().registerHL.reg);
			break;
		}
		case 0x4F:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerBC.lo, get().registerAF.hi);
			break;
		}
		case 0x50:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerBC.hi);
			break;
		}
		case 0x51:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerBC.lo);
			break;
		}
		case 0x52:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerDE.hi);
			break;
		}
		case 0x53:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerDE.lo);
			break;
		}
		case 0x54:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerHL.hi);
			break;
		}
		case 0x55:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerHL.lo);
			break;
		}
		case 0x56:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerDE.hi, get().registerHL.reg);
			break;
		}
		case 0x57:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.hi, get().registerAF.hi);
			break;
		}
		case 0x58:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerBC.hi);
			break;
		}
		case 0x59:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerBC.lo);
			break;
		}
		case 0x5A:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerDE.hi);
			break;
		}
		case 0x5B:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerDE.lo);
			break;
		}
		case 0x5C:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerHL.hi);
			break;
		}
		case 0x5D:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerHL.lo);
			break;
		}
		case 0x5E:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerDE.lo, get().registerHL.reg);
			break;
		}
		case 0x5F:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerDE.lo, get().registerAF.hi);
			break;
		}
		case 0x60:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerBC.hi);
			break;
		}
		case 0x61:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerBC.lo);
			break;
		}
		case 0x62:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerDE.hi);
			break;
		}
		case 0x63:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerDE.lo);
			break;
		}
		case 0x64:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerHL.hi);
			break;
		}
		case 0x65:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerHL.lo);
			break;
		}
		case 0x66:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerHL.hi, get().registerHL.reg);
			break;
		}
		case 0x67:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.hi, get().registerAF.hi);
			break;
		}
		case 0x68:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerBC.hi);
			break;
		}
		case 0x69:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerBC.lo);
			break;
		}
		case 0x6A:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerDE.hi);
			break;
		}
		case 0x6B:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerDE.lo);
			break;
		}
		case 0x6C:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerHL.hi);
			break;
		}
		case 0x6D:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerHL.lo);
			break;
		}
		case 0x6E:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerHL.lo, get().registerHL.reg);
			break;
		}
		case 0x6F:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerHL.lo, get().registerAF.hi);
			break;
		}
		case 0x70:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerBC.hi);
			break;
		}
		case 0x71:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerBC.lo);
			break;
		}
		case 0x72:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerDE.hi);
			break;
		}
		case 0x73:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerDE.lo);
			break;
		}
		case 0x74:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerHL.hi);
			break;
		}
		case 0x75:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerHL.lo);
			break;
		}
		case 0x76:
		{
			OpcodeCycles = 4;
			get().halted = true;

			bool tempSkip = false;

			if((Emu::ReadDirectMemory(0xFFFF) & Emu::ReadDirectMemory(0xFF0F) & 0x1F) && (!get().interruptMaster))
			{
				tempSkip = true;
			}
			else
			{
				tempSkip = false;
			}

			get().skipInstruction = tempSkip;
			break;
		}
		case 0x77:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory(get().registerHL.reg, get().registerAF.hi);
			break;
		}
		case 0x78:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerBC.hi);
			break;
		}
		case 0x79:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerBC.lo);
			break;
		}
		case 0x7A:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerDE.hi);
			break;
		}
		case 0x7B:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerDE.lo);
			break;
		}
		case 0x7C:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerHL.hi);
			break;
		}
		case 0x7D:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerHL.lo);
			break;
		}
		case 0x7E:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, get().registerHL.reg);
			break;
		}
		case 0x7F:
		{
			OpcodeCycles = 4;
			get()._RegLoad(get().registerAF.hi, get().registerAF.hi);
			break;
		}
		case 0x80:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerBC.hi, false, false);
			break;
		}
		case 0x81:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerBC.lo, false, false);
			break;
		}
		case 0x82:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerDE.hi, false, false);
			break;
		}
		case 0x83:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerDE.lo, false, false);
			break;
		}
		case 0x84:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerHL.hi, false, false);
			break;
		}
		case 0x85:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerHL.lo, false, false);
			break;
		}
		case 0x86:
		{
			OpcodeCycles = 8;
			get()._8BitAdd(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false, false);
			break;
		}
		case 0x87:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerAF.hi, false, false);
			break;
		}
		case 0x88:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerBC.hi, false, true);
			break;
		}
		case 0x89:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerBC.lo, false, true);
			break;
		}
		case 0x8A:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerDE.hi, false, true);
			break;
		}
		case 0x8B:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerDE.lo, false, true);
			break;
		}
		case 0x8C:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerHL.hi, false, true);
			break;
		}
		case 0x8D:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerHL.lo, false, true);
			break;
		}
		case 0x8E:
		{
			OpcodeCycles = 8;
			get()._8BitAdd(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false, true);
			break;
		}
		case 0x8F:
		{
			OpcodeCycles = 4;
			get()._8BitAdd(get().registerAF.hi, get().registerAF.hi, false, true);
			break;
		}
		case 0x90:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerBC.hi, false, false);
			break;
		}
		case 0x91:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerBC.lo, false, false);
			break;
		}
		case 0x92:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerDE.hi, false, false);
			break;
		}
		case 0x93:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerDE.lo, false, false);
			break;
		}
		case 0x94:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerHL.hi, false, false);
			break;
		}
		case 0x95:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerHL.lo, false, false);
			break;
		}
		case 0x96:
		{
			OpcodeCycles = 8;
			get()._8BitSub(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false, false);
			break;
		}
		case 0x97:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerAF.hi, false, false);
			break;
		}
		case 0x98:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerBC.hi, false, true);
			break;
		}
		case 0x99:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerBC.lo, false, true);
			break;
		}
		case 0x9A:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerDE.hi, false, true);
			break;
		}
		case 0x9B:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerDE.lo, false, true);
			break;
		}
		case 0x9C:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerHL.hi, false, true);
			break;
		}
		case 0x9D:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerHL.lo, false, true);
			break;
		}
		case 0x9E:
		{
			OpcodeCycles = 8;
			get()._8BitSub(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false, true);
			break;
		}
		case 0x9F:
		{
			OpcodeCycles = 4;
			get()._8BitSub(get().registerAF.hi, get().registerAF.hi, false, true);
			break;
		}
		case 0xA0:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerBC.hi, false);
			break;
		}
		case 0xA1:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerBC.lo, false);
			break;
		}
		case 0xA2:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerDE.hi, false);
			break;
		}
		case 0xA3:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerDE.lo, false);
			break;
		}
		case 0xA4:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerHL.hi, false);
			break;
		}
		case 0xA5:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, get().registerHL.lo, false);
			break;
		}
		case 0xA6:
		{
			OpcodeCycles = 8;
			get()._8BitAnd(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false);
			break;
		}
		case 0xA7:
		{
			OpcodeCycles = 4;
			get()._8BitAnd(get().registerAF.hi, registerAF.hi, false);
			break;
		}
		case 0xA8:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerBC.hi, false);
			break;
		}
		case 0xA9:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerBC.lo, false);
			break;
		}
		case 0xAA:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerDE.hi, false);
			break;
		}
		case 0xAB:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerDE.lo, false);
			break;
		}
		case 0xAC:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerHL.hi, false);
			break;
		}
		case 0xAD:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, get().registerHL.lo, false);
			break;
		}
		case 0xAE:
		{
			OpcodeCycles = 8;
			get()._8BitXor(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false);
			break;
		}
		case 0xAF:
		{
			OpcodeCycles = 4;
			get()._8BitXor(get().registerAF.hi, registerAF.hi, false);
			break;
		}
		case 0xB0:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerBC.hi, false);
			break;
		}
		case 0xB1:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerBC.lo, false);
			break;
		}
		case 0xB2:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerDE.hi, false);
			break;
		}
		case 0xB3:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerDE.lo, false);
			break;
		}
		case 0xB4:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerHL.hi, false);
			break;
		}
		case 0xB5:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, get().registerHL.lo, false);
			break;
		}
		case 0xB6:
		{
			OpcodeCycles = 8;
			get()._8BitOr(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false);
			break;
		}
		case 0xB7:
		{
			OpcodeCycles = 4;
			get()._8BitOr(get().registerAF.hi, registerAF.hi, false);
			break;
		}
		case 0xB8:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerBC.hi, false);
			break;
		}
		case 0xB9:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerBC.lo, false);
			break;
		}
		case 0xBA:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerDE.hi, false);
			break;
		}
		case 0xBB:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerDE.lo, false);
			break;
		}
		case 0xBC:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerHL.hi, false);
			break;
		}
		case 0xBD:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, get().registerHL.lo, false);
			break;
		}
		case 0xBE:
		{
			OpcodeCycles = 8;
			get()._8BitCompare(get().registerAF.hi, Emu::ReadMemory(get().registerHL.reg), false);
			break;
		}
		case 0xBF:
		{
			OpcodeCycles = 4;
			get()._8BitCompare(get().registerAF.hi, registerAF.hi, false);
			break;
		}
		case 0xC0:
		{
			OpcodeCycles = 8;
			get()._Return(true, Z_Flag, false);
			break;
		}
		case 0xC1:
		{
			OpcodeCycles = 12;
			get().registerBC.reg = get().PopWord();
			break;
		}
		case 0xC2:
		{
			OpcodeCycles = 12;
			get()._Jump(true, Z_Flag, false);
			break;
		}
		case 0xC3:
		{
			OpcodeCycles = 12;
			get()._Jump(false, 0, false);
			break;
		}
		case 0xC4:
		{
			OpcodeCycles = 12;
			get()._Call(true, Z_Flag, false);
			break;
		}
		case 0xC5:
		{
			OpcodeCycles = 16;
			get().PushWord(get().registerBC.reg);
			break;
		}
		case 0xC6:
		{
			OpcodeCycles = 8;
			get()._8BitAdd(get().registerAF.hi, 0, true, false);
			break;
		}
		case 0xC7:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x00);
			break;
		}
		case 0xC8:
		{
			OpcodeCycles = 8;
			get()._Return(true, Z_Flag, true);
			break;
		}
		case 0xC9:
		{
			OpcodeCycles = 8;
			get()._Return(false, 0, false);
			break;
		}
		case 0xCA:
		{
			OpcodeCycles = 12;
			get()._Jump(true, Z_Flag, true);
			break;
		}
		case 0xCB:
		{
			OpcodeCycles = ExecuteExtendedOpcode();
			OpcodeCycles += 4;
			break;
		}
		case 0xCC:
		{
			OpcodeCycles = 12;
			get()._Call(true, Z_Flag, true);
			break;
		}
		case 0xCD:
		{
			OpcodeCycles = 12;
			get()._Call(false, 0, false);
			break;
		}
		case 0xCE:
		{
			OpcodeCycles = 8;
			get()._8BitAdd(get().registerAF.hi, Emu::ReadMemory(get().programCounter++), true, true);
			break;
		}
		case 0xCF:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x08);
			break;
		}
		case 0xD0:
		{
			OpcodeCycles = 8;
			get()._Return(true, C_Flag, false);
			break;
		}
		case 0xD1:
		{
			OpcodeCycles = 12;
			get().registerDE.reg = get().PopWord();
			break;
		}
		case 0xD2:
		{
			OpcodeCycles = 12;
			get()._Jump(true, C_Flag, false);
			break;
		}
		case 0xD4:
		{
			OpcodeCycles = 12;
			get()._Call(true, C_Flag, false);
			break;
		}
		case 0xD5:
		{
			OpcodeCycles = 16;
			get().PushWord(get().registerDE.reg);
			break;
		}
		case 0xD6:
		{
			OpcodeCycles = 8;
			get()._8BitSub(get().registerAF.hi, 0, true, false);
			break;
		}
		case 0xD7:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x10);
			break;
		}
		case 0xD8:
		{
			OpcodeCycles = 8;
			get()._Return(true, C_Flag, true);
			break;
		}
		case 0xD9:
		{
			OpcodeCycles = 8;
			get().programCounter = get().PopWord();
			get().interruptMaster = true;
			break;
		}
		case 0xDA:
		{
			OpcodeCycles = 12;
			get()._Jump(true, C_Flag, true);
			break;
		}
		case 0xDC:
		{
			OpcodeCycles = 12;
			get()._Call(true, C_Flag, true);
			break;
		}
		case 0xDE:
		{
			OpcodeCycles = 8;
			get()._8BitSub(get().registerAF.hi, Emu::ReadMemory(get().programCounter++), true, true);
			break;
		}
		case 0xDF:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x18);
			break;
		}
		case 0xE0:
		{
			OpcodeCycles = 12;
			byte ldValue = FetchByte();
			get().programCounter++;
			word address = 0xFF00 + ldValue;
			Emu::WriteMemory(address, get().registerAF.hi);
			break;
		}
		case 0xE1:
		{
			OpcodeCycles = 12;
			get().registerHL.reg = get().PopWord();
			break;
		}
		case 0xE2:
		{
			OpcodeCycles = 8;
			Emu::WriteMemory((0xFF00 + get().registerBC.lo), get().registerAF.hi);
			break;
		}
		case 0xE5:
		{
			OpcodeCycles = 16;
			get().PushWord(get().registerHL.reg);
			break;
		}
		case 0xE6:
		{
			OpcodeCycles = 8;
			get()._8BitAnd(get().registerAF.hi, 0, true);
			break;
		}
		case 0xE7:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x20);
			break;
		}
		case 0xE8:
		{
			OpcodeCycles = 16;
			byte ldValue = get().FetchByte();
			get().programCounter++;
			signed_byte s_ldValue = (signed_byte)ldValue;
			get().stackPointer.reg = get()._8BitSignedAdd(get().stackPointer.reg, s_ldValue);
			break;
		}
		case 0xE9:
		{
			OpcodeCycles = 4;
			get().programCounter = get().registerHL.reg;
			break;
		}
		case 0xEA:
		{
			OpcodeCycles = 16;
			word ldValue = get().FetchWord();
			get().programCounter+=2;
			Emu::WriteMemory(ldValue, get().registerAF.hi);
			break;
		}
		case 0xEE:
		{
			OpcodeCycles = 8;
			get()._8BitXor(get().registerAF.hi, 0, true);
			break;
		}
		case 0xEF:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x28);
			break;
		}
		case 0xF0:
		{
			OpcodeCycles = 12;
			byte ldValue = get().FetchByte();
			get().programCounter++;
			word address = 0xFF00 + ldValue;
			get().registerAF.hi = Emu::ReadMemory(address);
			break;
		}
		case 0xF1:
		{
			OpcodeCycles = 12;
			get().registerAF.reg = get().PopWord();
			get().registerAF.lo &= 0xF0;
			break;
		}
		case 0xF2:
		{
			OpcodeCycles = 8;
			get()._RegLoadRom(get().registerAF.hi, (0xFF00 + get().registerBC.lo));
			break;
		}
		case 0xF3:
		{
			OpcodeCycles = 4;
			get().interruptMaster = false;
			break;
		}
		case 0xF5:
		{
			OpcodeCycles = 16;
			get().PushWord(get().registerAF.reg);
			break;
		}
		case 0xF6:
		{
			OpcodeCycles = 8;
			get()._8BitOr(get().registerAF.hi, 0, true);
			break;
		}
		case 0xF7:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x30);
			break;
		}
		case 0xF8:
		{
			OpcodeCycles = 12;
			byte ldValue = get().FetchByte();
			get().programCounter++;
			signed_byte s_ldValue = (signed_byte)ldValue;
			get().registerHL.reg = get()._8BitSignedAdd(get().stackPointer.reg, s_ldValue);
			break;
		}
		case 0xF9:
		{
			OpcodeCycles = 8;
			get().stackPointer.reg = get().registerHL.reg;
			break;
		}
		case 0xFA:
		{
			OpcodeCycles = 16;
			word ldValue = get().FetchWord();
			get().programCounter+=2;
			byte memValue = Emu::ReadMemory(ldValue);
			get().registerAF.hi = memValue;
			break;
		}
		case 0xFB:
		{
			OpcodeCycles = 4;
			get().interruptDelay = true;
			break;
		}
		case 0xFE:
		{
			OpcodeCycles = 8;
			get()._8BitCompare(get().registerAF.hi, 0, true);
			break;
		}
		case 0xFF:
		{
			OpcodeCycles = 32;
			get()._Restarts(0x38);
			break;
		}
		default:
		{
			printf("Unimplemented Opcode : 0x%X\n", opcode);
			OpcodeCycles = 0;
            break;
		}
	}

	return OpcodeCycles;
}

int SharpLr::ExecuteExtendedOpcode()
{
	byte opcode;

	opcode = Emu::ReadMemory(get().programCounter);

	get().programCounter++;

	int OpcodeCycles = 0;

	switch(opcode)
	{
		// rotate left through carry
  		case 0x0 : get()._Rlc(get().registerBC.hi); OpcodeCycles = 8; break; 
  		case 0x1 : get()._Rlc(get().registerBC.lo); OpcodeCycles = 8; break; 
  		case 0x2 : get()._Rlc(get().registerDE.hi); OpcodeCycles = 8; break; 
  		case 0x3 : get()._Rlc(get().registerDE.lo); OpcodeCycles = 8; break; 
  		case 0x4 : get()._Rlc(get().registerHL.hi); OpcodeCycles = 8; break; 
  		case 0x5 : get()._Rlc(get().registerHL.lo); OpcodeCycles = 8; break; 
  		case 0x6 : get()._RlcMemory(get().registerHL.reg); OpcodeCycles = 16; break; 
  		case 0x7 : get()._Rlc(get().registerAF.hi); OpcodeCycles = 8; break; 

  		// rotate right through carry
  		case 0x8 : get()._Rrc(get().registerBC.hi); OpcodeCycles = 8; break; 
  		case 0x9 : get()._Rrc(get().registerBC.lo); OpcodeCycles = 8; break; 
  		case 0xA : get()._Rrc(get().registerDE.hi); OpcodeCycles = 8; break; 
  		case 0xB : get()._Rrc(get().registerDE.lo); OpcodeCycles = 8; break; 
  		case 0xC : get()._Rrc(get().registerHL.hi); OpcodeCycles = 8; break; 
  		case 0xD : get()._Rrc(get().registerHL.lo); OpcodeCycles = 8; break; 
  		case 0xE : get()._RrcMemory(get().registerHL.reg); OpcodeCycles = 16; break; 
  		case 0xF : get()._Rrc(get().registerAF.hi); OpcodeCycles = 8; break; 

  		// rotate left
  		case 0x10: get()._Rl(get().registerBC.hi); OpcodeCycles = 8; break;
  		case 0x11: get()._Rl(get().registerBC.lo); OpcodeCycles = 8; break;
	  	case 0x12: get()._Rl(get().registerDE.hi); OpcodeCycles = 8; break;
  		case 0x13: get()._Rl(get().registerDE.lo); OpcodeCycles = 8; break;
  		case 0x14: get()._Rl(get().registerHL.hi); OpcodeCycles = 8; break;
  		case 0x15: get()._Rl(get().registerHL.lo); OpcodeCycles = 8; break;
  		case 0x16: get()._RlMemory(get().registerHL.reg); OpcodeCycles = 16; break;
  		case 0x17: get()._Rl(get().registerAF.hi);OpcodeCycles = 8; break;

  		// rotate right
  		case 0x18: get()._Rr(get().registerBC.hi); OpcodeCycles = 8; break;
  		case 0x19: get()._Rr(get().registerBC.lo); OpcodeCycles = 8; break;
  		case 0x1A: get()._Rr(get().registerDE.hi); OpcodeCycles = 8; break;
  		case 0x1B: get()._Rr(get().registerDE.lo); OpcodeCycles = 8; break;
  		case 0x1C: get()._Rr(get().registerHL.hi); OpcodeCycles = 8; break;
  		case 0x1D: get()._Rr(get().registerHL.lo); OpcodeCycles = 8; break;
  		case 0x1E: get()._RrMemory(get().registerHL.reg); OpcodeCycles = 16; break;
  		case 0x1F: get()._Rr(get().registerAF.hi); OpcodeCycles = 8; break;

  		case 0x20 : get()._Sla(get().registerBC.hi); OpcodeCycles = 8; break; 
  		case 0x21 : get()._Sla(get().registerBC.lo); OpcodeCycles = 8; break; 
  		case 0x22 : get()._Sla(get().registerDE.hi); OpcodeCycles = 8; break; 
	  	case 0x23 : get()._Sla(get().registerDE.lo); OpcodeCycles = 8; break; 
  		case 0x24 : get()._Sla(get().registerHL.hi); OpcodeCycles = 8; break; 
  		case 0x25 : get()._Sla(get().registerHL.lo); OpcodeCycles = 8; break; 
  		case 0x26 : get()._SlaMemory(get().registerHL.reg); OpcodeCycles = 16; break; 
		case 0x27 : get()._Sla(get().registerAF.hi); OpcodeCycles = 8; break; 

  		case 0x28 : get()._Sra(get().registerBC.hi); OpcodeCycles = 8; break; 
  		case 0x29 : get()._Sra(get().registerBC.lo); OpcodeCycles = 8; break; 
  		case 0x2A : get()._Sra(get().registerDE.hi); OpcodeCycles = 8; break; 
  		case 0x2B : get()._Sra(get().registerDE.lo); OpcodeCycles = 8; break; 
  		case 0x2C : get()._Sra(get().registerHL.hi); OpcodeCycles = 8; break; 
  		case 0x2D : get()._Sra(get().registerHL.lo); OpcodeCycles = 8; break; 
  		case 0x2E : get()._SraMemory(get().registerHL.reg); OpcodeCycles = 16; break; 
  		case 0x2F : get()._Sra(get().registerAF.hi); OpcodeCycles = 8; break; 

  		case 0x38 : get()._Srl(get().registerBC.hi); OpcodeCycles = 8; break; 
  		case 0x39 : get()._Srl(get().registerBC.lo); OpcodeCycles = 8; break; 
  		case 0x3A : get()._Srl(get().registerDE.hi); OpcodeCycles = 8; break; 
  		case 0x3B : get()._Srl(get().registerDE.lo); OpcodeCycles = 8; break; 
  		case 0x3C : get()._Srl(get().registerHL.hi); OpcodeCycles = 8; break; 
  		case 0x3D : get()._Srl(get().registerHL.lo); OpcodeCycles = 8; break; 
  		case 0x3E : get()._SrlMemory(get().registerHL.reg); OpcodeCycles = 16; break; 
	  	case 0x3F : get()._Srl(get().registerAF.hi); OpcodeCycles = 8; break; 

		// swap nibbles
		case 0x37 : get()._SwapNibbles(get().registerAF.hi); OpcodeCycles = 8; break; 
		case 0x30 : get()._SwapNibbles(get().registerBC.hi); OpcodeCycles = 8; break; 
		case 0x31 : get()._SwapNibbles(get().registerBC.lo); OpcodeCycles = 8; break; 
		case 0x32 : get()._SwapNibbles(get().registerDE.hi); OpcodeCycles = 8; break; 
		case 0x33 : get()._SwapNibbles(get().registerDE.lo); OpcodeCycles = 8; break; 
		case 0x34 : get()._SwapNibbles(get().registerHL.hi); OpcodeCycles = 8; break; 
		case 0x35 : get()._SwapNibbles(get().registerHL.lo); OpcodeCycles = 8; break; 
		case 0x36 : get()._SwapNibblesMemory(get().registerHL.reg); OpcodeCycles = 16; break; 

		// test bit
		case 0x40 : get()._TestBit(get().registerBC.hi, 0); OpcodeCycles = 8; break; 
		case 0x41 : get()._TestBit(get().registerBC.lo, 0); OpcodeCycles = 8; break; 
		case 0x42 : get()._TestBit(get().registerDE.hi, 0); OpcodeCycles = 8; break; 
		case 0x43 : get()._TestBit(get().registerDE.lo, 0); OpcodeCycles = 8; break; 
		case 0x44 : get()._TestBit(get().registerHL.hi, 0); OpcodeCycles = 8; break; 
		case 0x45 : get()._TestBit(get().registerHL.lo, 0); OpcodeCycles = 8; break; 
		case 0x46 : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 0); OpcodeCycles = 16; break; 
		case 0x47 : get()._TestBit(get().registerAF.hi, 0); OpcodeCycles = 8; break; 
		case 0x48 : get()._TestBit(get().registerBC.hi, 1); OpcodeCycles = 8; break; 
		case 0x49 : get()._TestBit(get().registerBC.lo, 1); OpcodeCycles = 8; break; 
		case 0x4A : get()._TestBit(get().registerDE.hi, 1); OpcodeCycles = 8; break; 
		case 0x4B : get()._TestBit(get().registerDE.lo, 1); OpcodeCycles = 8; break; 
		case 0x4C : get()._TestBit(get().registerHL.hi, 1); OpcodeCycles = 8; break; 
		case 0x4D : get()._TestBit(get().registerHL.lo, 1); OpcodeCycles = 8; break; 
		case 0x4E : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 1); OpcodeCycles = 16; break; 
		case 0x4F : get()._TestBit(get().registerAF.hi, 1); OpcodeCycles = 8; break; 
		case 0x50 : get()._TestBit(get().registerBC.hi, 2); OpcodeCycles = 8; break; 
		case 0x51 : get()._TestBit(get().registerBC.lo, 2); OpcodeCycles = 8; break; 
		case 0x52 : get()._TestBit(get().registerDE.hi, 2); OpcodeCycles = 8; break; 
		case 0x53 : get()._TestBit(get().registerDE.lo, 2); OpcodeCycles = 8; break; 
		case 0x54 : get()._TestBit(get().registerHL.hi, 2); OpcodeCycles = 8; break; 
		case 0x55 : get()._TestBit(get().registerHL.lo, 2); OpcodeCycles = 8; break; 
		case 0x56 : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 2); OpcodeCycles = 16; break; 
		case 0x57 : get()._TestBit(get().registerAF.hi, 2); OpcodeCycles = 8; break; 
		case 0x58 : get()._TestBit(get().registerBC.hi, 3); OpcodeCycles = 8; break; 
		case 0x59 : get()._TestBit(get().registerBC.lo, 3); OpcodeCycles = 8; break; 
		case 0x5A : get()._TestBit(get().registerDE.hi, 3); OpcodeCycles = 8; break; 
		case 0x5B : get()._TestBit(get().registerDE.lo, 3); OpcodeCycles = 8; break; 
		case 0x5C : get()._TestBit(get().registerHL.hi, 3); OpcodeCycles = 8; break; 
		case 0x5D : get()._TestBit(get().registerHL.lo, 3); OpcodeCycles = 8; break; 
		case 0x5E : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 3); OpcodeCycles = 16; break; 
		case 0x5F : get()._TestBit(get().registerAF.hi, 3); OpcodeCycles = 8; break; 
		case 0x60 : get()._TestBit(get().registerBC.hi, 4); OpcodeCycles = 8; break; 
		case 0x61 : get()._TestBit(get().registerBC.lo, 4); OpcodeCycles = 8; break; 
		case 0x62 : get()._TestBit(get().registerDE.hi, 4); OpcodeCycles = 8; break; 
		case 0x63 : get()._TestBit(get().registerDE.lo, 4); OpcodeCycles = 8; break; 
		case 0x64 : get()._TestBit(get().registerHL.hi, 4); OpcodeCycles = 8; break; 
		case 0x65 : get()._TestBit(get().registerHL.lo, 4); OpcodeCycles = 8; break; 
		case 0x66 : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 4); OpcodeCycles = 16; break; 
		case 0x67 : get()._TestBit(get().registerAF.hi, 4); OpcodeCycles = 8; break; 
		case 0x68 : get()._TestBit(get().registerBC.hi, 5); OpcodeCycles = 8; break; 
		case 0x69 : get()._TestBit(get().registerBC.lo, 5); OpcodeCycles = 8; break; 
		case 0x6A : get()._TestBit(get().registerDE.hi, 5); OpcodeCycles = 8; break; 
		case 0x6B : get()._TestBit(get().registerDE.lo, 5); OpcodeCycles = 8; break; 
		case 0x6C : get()._TestBit(get().registerHL.hi, 5); OpcodeCycles = 8; break; 
		case 0x6D : get()._TestBit(get().registerHL.lo, 5); OpcodeCycles = 8; break; 
		case 0x6E : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 5); OpcodeCycles = 16; break; 
		case 0x6F : get()._TestBit(get().registerAF.hi, 5); OpcodeCycles = 8; break; 
		case 0x70 : get()._TestBit(get().registerBC.hi, 6); OpcodeCycles = 8; break; 
		case 0x71 : get()._TestBit(get().registerBC.lo, 6); OpcodeCycles = 8; break; 
		case 0x72 : get()._TestBit(get().registerDE.hi, 6); OpcodeCycles = 8; break; 
		case 0x73 : get()._TestBit(get().registerDE.lo, 6); OpcodeCycles = 8; break; 
		case 0x74 : get()._TestBit(get().registerHL.hi, 6); OpcodeCycles = 8; break; 
		case 0x75 : get()._TestBit(get().registerHL.lo, 6); OpcodeCycles = 8; break; 
		case 0x76 : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 6); OpcodeCycles = 16; break; 
		case 0x77 : get()._TestBit(get().registerAF.hi, 6); OpcodeCycles = 8; break; 
		case 0x78 : get()._TestBit(get().registerBC.hi, 7); OpcodeCycles = 8; break; 
		case 0x79 : get()._TestBit(get().registerBC.lo, 7); OpcodeCycles = 8; break; 
		case 0x7A : get()._TestBit(get().registerDE.hi, 7); OpcodeCycles = 8; break; 
		case 0x7B : get()._TestBit(get().registerDE.lo, 7); OpcodeCycles = 8; break; 
		case 0x7C : get()._TestBit(get().registerHL.hi, 7); OpcodeCycles = 8; break; 
		case 0x7D : get()._TestBit(get().registerHL.lo, 7); OpcodeCycles = 8; break; 
		case 0x7E : get()._TestBit(Emu::ReadMemory(get().registerHL.reg), 7); OpcodeCycles = 16; break; 
		case 0x7F : get()._TestBit(get().registerAF.hi, 7); OpcodeCycles = 8; break; 

		// reset bit
		case 0x80 : get()._ResetBit(get().registerBC.hi, 0); OpcodeCycles = 8; break; 
		case 0x81 : get()._ResetBit(get().registerBC.lo, 0); OpcodeCycles = 8; break; 
		case 0x82 : get()._ResetBit(get().registerDE.hi, 0); OpcodeCycles = 8; break; 
		case 0x83 : get()._ResetBit(get().registerDE.lo, 0); OpcodeCycles = 8; break; 
		case 0x84 : get()._ResetBit(get().registerHL.hi, 0); OpcodeCycles = 8; break; 
		case 0x85 : get()._ResetBit(get().registerHL.lo, 0); OpcodeCycles = 8; break; 
		case 0x86 : get()._ResetBitMemory(get().registerHL.reg, 0); OpcodeCycles = 16; break; 
		case 0x87 : get()._ResetBit(get().registerAF.hi, 0); OpcodeCycles = 8; break; 
		case 0x88 : get()._ResetBit(get().registerBC.hi, 1); OpcodeCycles = 8; break; 
		case 0x89 : get()._ResetBit(get().registerBC.lo, 1); OpcodeCycles = 8; break; 
		case 0x8A : get()._ResetBit(get().registerDE.hi, 1); OpcodeCycles = 8; break; 
		case 0x8B : get()._ResetBit(get().registerDE.lo, 1); OpcodeCycles = 8; break; 
		case 0x8C : get()._ResetBit(get().registerHL.hi, 1); OpcodeCycles = 8; break; 
		case 0x8D : get()._ResetBit(get().registerHL.lo, 1); OpcodeCycles = 8; break; 
		case 0x8E : get()._ResetBitMemory(get().registerHL.reg, 1);  break; 
		case 0x8F : get()._ResetBit(get().registerAF.hi, 1); OpcodeCycles = 8; break; 
		case 0x90 : get()._ResetBit(get().registerBC.hi, 2); OpcodeCycles = 8; break; 
		case 0x91 : get()._ResetBit(get().registerBC.lo, 2); OpcodeCycles = 8; break; 
		case 0x92 : get()._ResetBit(get().registerDE.hi, 2); OpcodeCycles = 8; break; 
		case 0x93 : get()._ResetBit(get().registerDE.lo, 2); OpcodeCycles = 8; break; 
		case 0x94 : get()._ResetBit(get().registerHL.hi, 2); OpcodeCycles = 8; break; 
		case 0x95 : get()._ResetBit(get().registerHL.lo, 2); OpcodeCycles = 8; break; 
		case 0x96 : get()._ResetBitMemory(get().registerHL.reg, 2); OpcodeCycles = 16; break; 
		case 0x97 : get()._ResetBit(get().registerAF.hi, 2); OpcodeCycles = 8; break; 
		case 0x98 : get()._ResetBit(get().registerBC.hi, 3); OpcodeCycles = 8; break; 
		case 0x99 : get()._ResetBit(get().registerBC.lo, 3); OpcodeCycles = 8; break; 
		case 0x9A : get()._ResetBit(get().registerDE.hi, 3); OpcodeCycles = 8; break; 
		case 0x9B : get()._ResetBit(get().registerDE.lo, 3); OpcodeCycles = 8; break; 
		case 0x9C : get()._ResetBit(get().registerHL.hi, 3); OpcodeCycles = 8; break; 
		case 0x9D : get()._ResetBit(get().registerHL.lo, 3); OpcodeCycles = 8; break; 
		case 0x9E : get()._ResetBitMemory(get().registerHL.reg, 3); OpcodeCycles = 16; break; 
		case 0x9F : get()._ResetBit(get().registerAF.hi, 3); OpcodeCycles = 8; break; 
		case 0xA0 : get()._ResetBit(get().registerBC.hi, 4); OpcodeCycles = 8; break; 
		case 0xA1 : get()._ResetBit(get().registerBC.lo, 4); OpcodeCycles = 8; break; 
		case 0xA2 : get()._ResetBit(get().registerDE.hi, 4); OpcodeCycles = 8; break; 
		case 0xA3 : get()._ResetBit(get().registerDE.lo, 4); OpcodeCycles = 8; break; 
		case 0xA4 : get()._ResetBit(get().registerHL.hi, 4); OpcodeCycles = 8; break; 
		case 0xA5 : get()._ResetBit(get().registerHL.lo, 4); OpcodeCycles = 8; break; 
		case 0xA6 : get()._ResetBitMemory(get().registerHL.reg, 4); OpcodeCycles = 16; break; 
		case 0xA7 : get()._ResetBit(get().registerAF.hi, 4); OpcodeCycles = 8; break; 
		case 0xA8 : get()._ResetBit(get().registerBC.hi, 5); OpcodeCycles = 8; break; 
		case 0xA9 : get()._ResetBit(get().registerBC.lo, 5); OpcodeCycles = 8; break; 
		case 0xAA : get()._ResetBit(get().registerDE.hi, 5); OpcodeCycles = 8; break; 
		case 0xAB : get()._ResetBit(get().registerDE.lo, 5); OpcodeCycles = 8; break; 
		case 0xAC : get()._ResetBit(get().registerHL.hi, 5); OpcodeCycles = 8; break; 
		case 0xAD : get()._ResetBit(get().registerHL.lo, 5); OpcodeCycles = 8; break; 
		case 0xAE : get()._ResetBitMemory(get().registerHL.reg, 5); OpcodeCycles = 16; break; 
		case 0xAF : get()._ResetBit(get().registerAF.hi, 5); OpcodeCycles = 8; break; 
		case 0xB0 : get()._ResetBit(get().registerBC.hi, 6); OpcodeCycles = 8; break; 
		case 0xB1 : get()._ResetBit(get().registerBC.lo, 6); OpcodeCycles = 8; break; 
		case 0xB2 : get()._ResetBit(get().registerDE.hi, 6); OpcodeCycles = 8; break; 
		case 0xB3 : get()._ResetBit(get().registerDE.lo, 6); OpcodeCycles = 8; break; 
		case 0xB4 : get()._ResetBit(get().registerHL.hi, 6); OpcodeCycles = 8; break; 
		case 0xB5 : get()._ResetBit(get().registerHL.lo, 6); OpcodeCycles = 8; break; 
		case 0xB6 : get()._ResetBitMemory(get().registerHL.reg, 6); OpcodeCycles = 16; break; 
		case 0xB7 : get()._ResetBit(get().registerAF.hi, 6); OpcodeCycles = 8; break; 
		case 0xB8 : get()._ResetBit(get().registerBC.hi, 7); OpcodeCycles = 8; break; 
		case 0xB9 : get()._ResetBit(get().registerBC.lo, 7); OpcodeCycles = 8; break; 
		case 0xBA : get()._ResetBit(get().registerDE.hi, 7); OpcodeCycles = 8; break; 
		case 0xBB : get()._ResetBit(get().registerDE.lo, 7); OpcodeCycles = 8; break; 
		case 0xBC : get()._ResetBit(get().registerHL.hi, 7); OpcodeCycles = 8; break; 
		case 0xBD : get()._ResetBit(get().registerHL.lo, 7); OpcodeCycles = 8; break; 
		case 0xBE : get()._ResetBitMemory(get().registerHL.reg, 7); OpcodeCycles = 16; break; 
		case 0xBF : get()._ResetBit(get().registerAF.hi, 7); OpcodeCycles = 8; break; 


		// set bit
		case 0xC0 : get()._SetBit(get().registerBC.hi, 0); OpcodeCycles = 8; break; 
		case 0xC1 : get()._SetBit(get().registerBC.lo, 0); OpcodeCycles = 8; break; 
		case 0xC2 : get()._SetBit(get().registerDE.hi, 0); OpcodeCycles = 8; break; 
		case 0xC3 : get()._SetBit(get().registerDE.lo, 0); OpcodeCycles = 8; break; 
		case 0xC4 : get()._SetBit(get().registerHL.hi, 0); OpcodeCycles = 8; break; 
		case 0xC5 : get()._SetBit(get().registerHL.lo, 0); OpcodeCycles = 8; break; 
		case 0xC6 : get()._SetBitMemory(get().registerHL.reg, 0); OpcodeCycles = 16; break; 
		case 0xC7 : get()._SetBit(get().registerAF.hi, 0); OpcodeCycles = 8; break; 
		case 0xC8 : get()._SetBit(get().registerBC.hi, 1); OpcodeCycles = 8; break; 
		case 0xC9 : get()._SetBit(get().registerBC.lo, 1); OpcodeCycles = 8; break; 
		case 0xCA : get()._SetBit(get().registerDE.hi, 1); OpcodeCycles = 8; break; 
		case 0xCB : get()._SetBit(get().registerDE.lo, 1); OpcodeCycles = 8; break; 
		case 0xCC : get()._SetBit(get().registerHL.hi, 1); OpcodeCycles = 8; break; 
		case 0xCD : get()._SetBit(get().registerHL.lo, 1); OpcodeCycles = 8; break; 
		case 0xCE : get()._SetBitMemory(get().registerHL.reg, 1); OpcodeCycles = 16; break; 
		case 0xCF : get()._SetBit(get().registerAF.hi, 1); OpcodeCycles = 8; break; 
		case 0xD0 : get()._SetBit(get().registerBC.hi, 2); OpcodeCycles = 8; break; 
		case 0xD1 : get()._SetBit(get().registerBC.lo, 2); OpcodeCycles = 8; break; 
		case 0xD2 : get()._SetBit(get().registerDE.hi, 2); OpcodeCycles = 8; break; 
		case 0xD3 : get()._SetBit(get().registerDE.lo, 2); OpcodeCycles = 8; break; 
		case 0xD4 : get()._SetBit(get().registerHL.hi, 2); OpcodeCycles = 8; break; 
		case 0xD5 : get()._SetBit(get().registerHL.lo, 2); OpcodeCycles = 8; break; 
		case 0xD6 : get()._SetBitMemory(get().registerHL.reg, 2); OpcodeCycles = 16; break; 
		case 0xD7 : get()._SetBit(get().registerAF.hi, 2); OpcodeCycles = 8; break; 
		case 0xD8 : get()._SetBit(get().registerBC.hi, 3); OpcodeCycles = 8; break; 
		case 0xD9 : get()._SetBit(get().registerBC.lo, 3); OpcodeCycles = 8; break; 
		case 0xDA : get()._SetBit(get().registerDE.hi, 3); OpcodeCycles = 8; break; 
		case 0xDB : get()._SetBit(get().registerDE.lo, 3); OpcodeCycles = 8; break; 
		case 0xDC : get()._SetBit(get().registerHL.hi, 3); OpcodeCycles = 8; break; 
		case 0xDD : get()._SetBit(get().registerHL.lo, 3); OpcodeCycles = 8; break; 
		case 0xDE : get()._SetBitMemory(get().registerHL.reg, 3); OpcodeCycles = 16; break; 
		case 0xDF : get()._SetBit(get().registerAF.hi, 3); OpcodeCycles = 8; break; 
		case 0xE0 : get()._SetBit(get().registerBC.hi, 4); OpcodeCycles = 8; break; 
		case 0xE1 : get()._SetBit(get().registerBC.lo, 4); OpcodeCycles = 8; break; 
		case 0xE2 : get()._SetBit(get().registerDE.hi, 4); OpcodeCycles = 8; break; 
		case 0xE3 : get()._SetBit(get().registerDE.lo, 4); OpcodeCycles = 8; break; 
		case 0xE4 : get()._SetBit(get().registerHL.hi, 4); OpcodeCycles = 8; break; 
		case 0xE5 : get()._SetBit(get().registerHL.lo, 4); OpcodeCycles = 8; break; 
		case 0xE6 : get()._SetBitMemory(get().registerHL.reg, 4); OpcodeCycles = 16; break; 
		case 0xE7 : get()._SetBit(get().registerAF.hi, 4); OpcodeCycles = 8; break; 
		case 0xE8 : get()._SetBit(get().registerBC.hi, 5); OpcodeCycles = 8; break; 
		case 0xE9 : get()._SetBit(get().registerBC.lo, 5); OpcodeCycles = 8; break; 
		case 0xEA : get()._SetBit(get().registerDE.hi, 5); OpcodeCycles = 8; break; 
		case 0xEB : get()._SetBit(get().registerDE.lo, 5); OpcodeCycles = 8; break; 
		case 0xEC : get()._SetBit(get().registerHL.hi, 5); OpcodeCycles = 8; break; 
		case 0xED : get()._SetBit(get().registerHL.lo, 5); OpcodeCycles = 8; break; 
		case 0xEE : get()._SetBitMemory(get().registerHL.reg, 5); OpcodeCycles = 16; break; 
		case 0xEF : get()._SetBit(get().registerAF.hi, 5); OpcodeCycles = 8; break; 
		case 0xF0 : get()._SetBit(get().registerBC.hi, 6); OpcodeCycles = 8; break; 
		case 0xF1 : get()._SetBit(get().registerBC.lo, 6); OpcodeCycles = 8; break; 
		case 0xF2 : get()._SetBit(get().registerDE.hi, 6); OpcodeCycles = 8; break; 
		case 0xF3 : get()._SetBit(get().registerDE.lo, 6); OpcodeCycles = 8; break; 
		case 0xF4 : get()._SetBit(get().registerHL.hi, 6); OpcodeCycles = 8; break; 
		case 0xF5 : get()._SetBit(get().registerHL.lo, 6); OpcodeCycles = 8; break; 
		case 0xF6 : get()._SetBitMemory(get().registerHL.reg, 6); OpcodeCycles = 16; break; 
		case 0xF7 : get()._SetBit(get().registerAF.hi, 6); OpcodeCycles = 8; break; 
		case 0xF8 : get()._SetBit(get().registerBC.hi, 7); OpcodeCycles = 8; break; 
		case 0xF9 : get()._SetBit(get().registerBC.lo, 7); OpcodeCycles = 8; break; 
		case 0xFA : get()._SetBit(get().registerDE.hi, 7); OpcodeCycles = 8; break; 
		case 0xFB : get()._SetBit(get().registerDE.lo, 7); OpcodeCycles = 8; break; 
		case 0xFC : get()._SetBit(get().registerHL.hi, 7); OpcodeCycles = 8; break; 
		case 0xFD : get()._SetBit(get().registerHL.lo, 7); OpcodeCycles = 8; break; 
		case 0xFE : get()._SetBitMemory(get().registerHL.reg, 7); OpcodeCycles = 16; break; 
		case 0xFF : get()._SetBit(get().registerAF.hi, 7); OpcodeCycles = 8; break;
		default:
		{
			printf("Unimplemented Opcode : 0x%X\n", opcode);
			OpcodeCycles = 0;
            break;
		}
	}

	return OpcodeCycles;
}

//Opcodes

void SharpLr::_8BitLoad(byte& reg)
{
	byte ldValue = get().FetchByte();
	get().programCounter++;
	reg = ldValue;
}

void SharpLr::_16BitLoad(word& reg)
{
	word ldValue = get().FetchWord();
	programCounter+=2;
	reg = ldValue;
}

void SharpLr::_RegLoad(byte& reg, byte load)
{
	reg = load;
}

void SharpLr::_RegLoadRom(byte& reg, word address)
{
	reg = Emu::ReadMemory(address);
}

void SharpLr::_16BitDec(word& reg)
{
	reg--;
}

void SharpLr::_16BitInc(word& reg)
{
	reg++;
}

word SharpLr::_8BitSignedAdd(word& reg, byte toAdd)
{
	word result = reg + (signed_word)((signed_byte)toAdd);

	get().registerAF.lo = 0;

	if((reg & 0xFF) + toAdd > 0xFF)
	{
		get().registerAF.lo |= 0x10;
	}

	if((reg & 0xF) + (toAdd & 0xF) > 0xF)
	{
		get().registerAF.lo |= 0x20;
	}

	return result;
}

void SharpLr::_8BitAdd(byte& reg, byte toAdd, bool useImmediate, bool addCarry)
{
	if(!addCarry)
	{
		byte before = reg;
		byte adding = 0;

		if(useImmediate)
		{
			byte ldValue = get().FetchByte();
			get().programCounter++;
			adding = ldValue;
		}
		else
		{
			adding = toAdd;
		}

		reg += adding;

		get().registerAF.lo = 0;

		if(reg == 0)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
		}

		word htest = (before & 0xF);
		htest += (adding & 0xF);

		if(htest > 0xF)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
		}

		if((before + adding) > 0xFF)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		}
	}
	else
	{
		byte carryFlag = 0;

		if(get().registerAF.lo & 0x10)
		{
			carryFlag = 1;
		}

		byte carrySet = 0;
		byte halfCarrySet = 0;

		byte temp1, temp2 = 0;

		temp1 = reg + carryFlag;

		if(reg + carryFlag > 0xFF)
		{
			carrySet = 1;
		}

		if((reg & 0xF) + (carryFlag & 0xF) > 0xF)
		{
			halfCarrySet = 1;
		}

		temp2 = temp1 + toAdd;

		if(temp1 + toAdd > 0xFF)
		{
			carrySet = 1;
		}

		if((temp1 & 0xF) + (toAdd & 0xF) > 0xF)
		{
			halfCarrySet = 1;
		}

		get().registerAF.lo = 0;

		if(carrySet)
		{
			get().registerAF.lo |= 0x10;
		}

		if(halfCarrySet)
		{
			get().registerAF.lo |= 0x20;
		}

		if(!temp2)
		{
			get().registerAF.lo |= 0x80;
		}

		get().registerAF.hi = temp2;

	}
}

void SharpLr::_8BitSub(byte& reg, byte toSub, bool useImmediate, bool subCarry)
{
	if(!subCarry)
	{
		byte before = reg;
		byte subtracting = 0;

		if(useImmediate)
		{
			byte ldValue = get().FetchByte();
			get().programCounter++;
			subtracting = ldValue;
		}
		else
		{
			subtracting = toSub;
		}

		reg -= subtracting;

		get().registerAF.lo = 0;

		if(reg == 0)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
		}

		get().registerAF.lo = BitSet(get().registerAF.lo, N_Flag);

		if(before < subtracting)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		}

		signed_word htest = (before & 0xF);
		htest -= (subtracting & 0xF);

		if(htest < 0)
		{
			get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
		}
	}
	else
	{
		byte carryFlag = 0;

		if(get().registerAF.lo & 0x10)
		{
			carryFlag = 1;
		}

		byte carrySet = 0;
		byte halfCarrySet = 0;

		byte temp1, temp2 = 0;

		temp1 = reg - carryFlag;

		if(reg < carryFlag)
		{
			carrySet = 1;
		}

		if((reg & 0xF) < (carryFlag & 0xF))
		{
			halfCarrySet = 1;
		}

		temp2 = temp1 - toSub;

		if(temp1 < toSub)
		{
			carrySet = 1;
		}

		if((temp1 & 0xF) < (toSub & 0xF))
		{
			halfCarrySet = 1;
		}

		get().registerAF.lo = 0;

		if(carrySet)
		{
			get().registerAF.lo |= 0x10;
		}

		if(halfCarrySet)
		{
			get().registerAF.lo |= 0x20;
		}

		get().registerAF.lo |= 0x40;

		if(!temp2)
		{
			get().registerAF.lo |= 0x80;
		}

		get().registerAF.hi = temp2;

	}
}

void SharpLr::_8BitAnd(byte& reg, byte toAnd, bool useImmediate)
{
	byte myAnd = 0;

	if(useImmediate)
	{
		byte ldValue = get().FetchByte();
		programCounter++;
		myAnd = ldValue;
	}
	else
	{
		myAnd = toAnd;
	}

	reg &= myAnd;

	get().registerAF.lo = 0;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
}

void SharpLr::_8BitOr(byte& reg, byte toOr, bool useImmediate)
{
	byte myOr = 0;

	if(useImmediate)
	{
		byte ldValue = get().FetchByte();
		get().programCounter++;
		myOr = ldValue;
	}
	else
	{
		myOr = toOr;
	}

	reg |= myOr;

	get().registerAF.lo = 0;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_8BitXor(byte& reg, byte toXor, bool useImmediate)
{
	byte myXor = 0;

	if(useImmediate)
	{
		byte ldValue = get().FetchByte();
		get().programCounter++;
		myXor = ldValue;
	}
	else
	{
		myXor = toXor;
	}

	reg ^= myXor;

	get().registerAF.lo = 0;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_8BitCompare(byte reg, byte toSub, bool useImmediate)
{
	byte before = reg;
	byte subtracting = 0;

	if(useImmediate)
	{
		byte ldValue = get().FetchByte();
		get().programCounter++;
		subtracting = ldValue;
	}
	else
	{
		subtracting = toSub;
	}

	reg -= subtracting;

	get().registerAF.lo = 0;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitSet(get().registerAF.lo, N_Flag);

	if(before < subtracting)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	signed_word htest = before & 0xF;
	htest -= (subtracting & 0xF);

	if(htest < 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
	}
}

void SharpLr::_8BitInc(byte& reg)
{
	byte before = reg;

	reg++;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitClear(get().registerAF.lo, N_Flag);

	if((before & 0xF) == 0xF)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, H_Flag);
	}
}

void SharpLr::_8BitMemoryInc(word address)
{
	byte before = Emu::ReadMemory(address);
	Emu::WriteMemory(address, (before+1));
	byte now = before + 1;

	if(now == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitClear(get().registerAF.lo, N_Flag);

	if((before & 0xF) == 0xF)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, H_Flag);
	}
}

void SharpLr::_8BitDec(byte& reg)
{
	byte before = reg;

	reg--;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitSet(get().registerAF.lo, N_Flag);

	if((before & 0xF) == 0)
	{
		get().registerAF.lo = BitSet(registerAF.lo, H_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(registerAF.lo, H_Flag);
	}
}

void SharpLr::_8BitMemoryDec(word address)
{
	byte before = Emu::ReadMemory(address);
	Emu::WriteMemory(address, (before - 1));
	byte now = before - 1;

	if(now == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitSet(get().registerAF.lo, N_Flag);

	if((before & 0xF) == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
	}
	else
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, H_Flag);
	}
}


void SharpLr::_16BitAdd(word& reg, word toAdd)
{
	byte zeroFlag = 0;

	if(get().registerAF.lo & 0x80)
	{
		zeroFlag = 1;
	}

	get().registerAF.lo = 0;

	if(reg + toAdd > 0xFFFF)
	{
		get().registerAF.lo |= 0x10;
	}

	if((reg & 0x0FFF) + (toAdd & 0x0FFF) > 0x0FFF)
	{
		get().registerAF.lo |= 0x20;
	}

	if(zeroFlag)
	{
		get().registerAF.lo |= 0x80;
	}

	reg += toAdd;
}

void SharpLr::_Jump(bool useCondition, int flag, bool condition)
{
	word ldValue = get().FetchWord();

	get().programCounter += 2;

	if(!useCondition)
	{
		get().programCounter = ldValue;
		return;
	}

	if(TestBit(get().registerAF.lo, flag) == condition)
	{
		get().programCounter = ldValue;
	}
}

void SharpLr::_JumpImmediate(bool useCondition, int flag, bool condition)
{
	signed_byte ldValue = (signed_byte)get().FetchByte();

	get().programCounter++;

	if(!useCondition)
	{
		get().programCounter += ldValue;
	}
	else if(TestBit(get().registerAF.lo, flag) == condition)
	{
		get().programCounter += ldValue;
	}
}

void SharpLr::_Call(bool useCondition, int flag, bool condition)
{
	word ldValue = get().FetchWord();

	get().programCounter += 2;

	if(!useCondition)
	{
		get().PushWord(get().programCounter);
		get().programCounter = ldValue;
		return;
	}

	if(TestBit(get().registerAF.lo, flag) == condition)
	{
		get().PushWord(get().programCounter);
		get().programCounter = ldValue;
	}
}

void SharpLr::_Return(bool useCondition, int flag, bool condition)
{
	if(!useCondition)
	{
		get().programCounter = get().PopWord();
		return;
	}

	if(TestBit(get().registerAF.lo, flag) == condition)
	{
		get().programCounter = get().PopWord();
	}
}

void SharpLr::_SwapNibbles(byte& reg)
{
	get().registerAF.lo = 0;

	reg = (((reg & 0xF0) >> 4) | ((reg & 0xF) << 4));

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_SwapNibblesMemory(word address)
{
	get().registerAF.lo = 0;

	byte memValue = Emu::ReadMemory(address);
	memValue = (((memValue & 0xF0) >> 4) | ((memValue & 0xF) << 4));

	Emu::WriteMemory(address, memValue);

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_Restarts(byte value)
{
	get().PushWord(get().programCounter);
	get().programCounter = value;
}

void SharpLr::_ShiftLeftCarry(byte& reg)
{
	get().registerAF.lo = 0;

	if(TestBit(reg, 7))
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	reg = reg << 1;

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_ShiftLeftCarryMemory(word address)
{
	byte before = Emu::ReadMemory(address);

	get().registerAF.lo = 0;

	if(TestBit(before, 7))
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	before = before << 1;

	if(before == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}

	Emu::WriteMemory(address, before);
}

void SharpLr::_ResetBit(byte& reg, int bit)
{
	reg = BitClear(reg, bit);
}

void SharpLr::_ResetBitMemory(word address, int bit)
{
	byte memValue = Emu::ReadMemory(address);
	memValue = BitClear(memValue, bit);

	Emu::WriteMemory(address, memValue);
}

void SharpLr::_TestBit(byte reg, int bit)
{
	if(TestBit(reg, bit))
	{
		get().registerAF.lo = BitClear(get().registerAF.lo, Z_Flag);
	}
	else
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}

	get().registerAF.lo = BitClear(get().registerAF.lo, N_Flag);
	get().registerAF.lo = BitSet(get().registerAF.lo, H_Flag);
}

void SharpLr::_SetBit(byte& reg, int bit)
{
	reg = BitSet(reg, bit);
}

void SharpLr::_SetBitMemory(word address, int bit)
{
	byte memValue = Emu::ReadMemory(address);
	memValue = BitSet(memValue, bit);

	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Daa()
{
	int regA = get().registerAF.hi;

	if(!(get().registerAF.lo & 0x40))
	{
		if((get().registerAF.lo & 0x20) || ((regA & 0xF) > 0x09))
		{
			regA += 0x06;
		}
		if((get().registerAF.lo & 0x10) || (regA > 0x9F))
		{
			regA += 0x60;
		}
	}
	else
	{
		if(get().registerAF.lo & 0x20)
		{
			regA = (regA - 0x06) & 0xFF;
		}
		if(get().registerAF.lo & 0x10)
		{
			regA -= 0x60;
		}
	}

	if(regA & 0x100)
	{
		get().registerAF.lo |= 0x10;
	}

	regA &= 0xFF;

	get().registerAF.lo &= ~0x20;

	if(regA == 0)
	{
		get().registerAF.lo |= 0x80;
	}
	else
	{
		get().registerAF.lo &= ~0x80;
	}

	get().registerAF.hi = (byte)regA;
}

void SharpLr::_Rr(byte& reg)
{
	bool isCarrySet = TestBit(get().registerAF.lo, C_Flag);
	bool isLSBSet = TestBit(reg, 0);

	get().registerAF.lo = 0;

	reg >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}
	if(isCarrySet)
	{
		reg = BitSet(reg, 7);
	}
	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_RrMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isCarrySet = TestBit(get().registerAF.lo, C_Flag);
	bool isLSBSet = TestBit(memValue, 0);

	get().registerAF.lo = 0;

	memValue >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}
	if(isCarrySet)
	{
		memValue = BitSet(memValue, 7);
	}
	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Rlc(byte& reg)
{
	bool isMSBSet = TestBit(reg, 7);

	get().registerAF.lo = 0;

	reg <<= 1;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		reg = BitSet(reg, 0);
	}
	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_RlcMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isMSBSet = TestBit(memValue, 7);

	get().registerAF.lo = 0;

	memValue <<= 1;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		memValue = BitSet(memValue, 0);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Rrc(byte& reg)
{
	bool isLSBSet = TestBit(reg, 0);

	get().registerAF.lo = 0;

	reg >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		reg = BitSet(reg, 7);
	}

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_RrcMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isLSBSet = TestBit(memValue, 0);

	get().registerAF.lo = 0;

	memValue >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
		memValue = BitSet(memValue, 7);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Sla(byte& reg)
{
	bool isMSBSet = TestBit(reg, 7);

	reg <<= 1;

	get().registerAF.lo = 0;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_SlaMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isMSBSet = TestBit(memValue, 7);

	memValue <<= 1;

	get().registerAF.lo = 0;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Sra(byte& reg)
{
	bool isLSBSet = TestBit(reg, 0);
	bool isMSBSet = TestBit(reg, 7);

	get().registerAF.lo = 0;

	reg >>= 1;

	if(isMSBSet)
	{
		reg = BitSet(reg, 7);
	}

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_SraMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isLSBSet = TestBit(memValue, 0);
	bool isMSBSet = TestBit(memValue, 7);

	get().registerAF.lo = 0;

	memValue >>= 1;

	if(isMSBSet)
	{
		memValue = BitSet(memValue, 7);
	}

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Srl(byte& reg)
{
	bool isLSBSet = TestBit(reg, 0);

	get().registerAF.lo = 0;

	reg >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_SrlMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isLSBSet = TestBit(memValue, 0);

	get().registerAF.lo = 0;

	memValue >>= 1;

	if(isLSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}

void SharpLr::_Rl(byte& reg)
{
	bool isCarrySet = TestBit(get().registerAF.lo, C_Flag);
	bool isMSBSet = TestBit(reg, 7);

	get().registerAF.lo = 0;

	reg <<= 1;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(isCarrySet)
	{
		reg = BitSet(reg, 0);
	}

	if(reg == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
}

void SharpLr::_RlMemory(word address)
{
	byte memValue = Emu::ReadMemory(address);

	bool isCarrySet = TestBit(get().registerAF.lo, C_Flag);
	bool isMSBSet = TestBit(memValue, 7);

	get().registerAF.lo = 0;

	memValue <<= 1;

	if(isMSBSet)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, C_Flag);
	}

	if(isCarrySet)
	{
		memValue = BitSet(memValue, 0);
	}

	if(memValue == 0)
	{
		get().registerAF.lo = BitSet(get().registerAF.lo, Z_Flag);
	}
	Emu::WriteMemory(address, memValue);
}