#ifndef SHARPLR_HPP
#define SHARPLR_HPP

#define Z_Flag 7
#define N_Flag 6
#define H_Flag 5
#define C_Flag 4

#define TIMA 0xFF05
#define TMA 0xFF06
#define TMC 0xFF07

#define CLOCKSPEED 4194304

#include "../BITS/bitsUtils.hpp"
#include "../GameBoy/emu.hpp"

typedef union{
	word reg;
	struct
	{
		byte lo;
		byte hi;
	};
}Register;

class SharpLr
{
public:

	SharpLr();
	static void Init();
	static void UpdateTimer(int cycles);
	static void UpdateInterrupts();
	static int ExecuteNextOpcode();

	static void SetDividerRegister(int value);

private:
	Register registerAF;
	Register registerBC;
	Register registerDE;
	Register registerHL;

	Register stackPointer;

	word programCounter;

	int timerCounter;
	int timerSpeed;
	int dividerRegister;

	bool interruptDelay;
	bool interruptMaster;

	bool skipInstruction;

	bool halted;

	static SharpLr& get(void);

	void PushWord(word value);
	word PopWord();
	word FetchWord();
	byte FetchByte();

	bool IsClockEnabled();
	byte GetClockFrequence();
	void SetClockFrequence();
	void DoDividerRegister(int cycles);

	int ExecuteOpcode(byte opcode);
	int ExecuteExtendedOpcode();

	void _8BitLoad(byte& reg);
	void _16BitLoad(word& reg);
	void _RegLoad(byte& reg, byte load);
	void _RegLoadRom(byte& reg, word address);
	void _16BitDec(word& reg);
	void _16BitInc(word& reg);
	word _8BitSignedAdd(word& reg, byte toAdd);
	void _8BitAdd(byte& reg, byte toAdd, bool useImmediate, bool addCarry);
	void _8BitSub(byte& reg, byte toSub, bool useImmediate, bool subCarry);
	void _8BitAnd(byte& reg, byte toAnd, bool useImmediate);
	void _8BitOr(byte& reg, byte toOr, bool useImmediate);
	void _8BitXor(byte& reg, byte toXor, bool useImmediate);
	void _8BitCompare(byte reg, byte toSub, bool useImmediate);
	void _8BitInc(byte& reg);
	void _8BitMemoryInc(word address);
	void _8BitDec(byte& reg);
	void _8BitMemoryDec(word address);
	void _16BitAdd(word& reg, word toAdd);
	void _Jump(bool useCondition, int flag, bool condition);
	void _JumpImmediate(bool useCondition, int flag, bool condition);
	void _Call(bool useCondition, int flag, bool condition);
	void _Return(bool useCondition, int flag, bool condition);
	void _SwapNibbles(byte& reg);
	void _SwapNibblesMemory(word address);
	void _Restarts(byte value);
	void _ShiftLeftCarry(byte& reg);
	void _ShiftLeftCarryMemory(word address);
	void _ResetBit(byte& reg, int bit);
	void _ResetBitMemory(word address, int bit);
	void _TestBit(byte reg, int bit);
	void _SetBit(byte& reg, int bit);
	void _SetBitMemory(word address, int bit);
	void _Daa();
	void _Rr(byte& reg);
	void _RrMemory(word address);
	void _Rlc(byte& reg);
	void _RlcMemory(word address);
	void _Rrc(byte& reg);
	void _RrcMemory(word address);
	void _Sla(byte& reg);
	void _SlaMemory(word address);
	void _Sra(byte& reg);
	void _SraMemory(word address);
	void _Srl(byte& reg);
	void _SrlMemory(word address);
	void _Rl(byte& reg);
	void _RlMemory(word address);

};

#endif