#ifndef Z80_H
#define Z80_H


#define MAXLOOP 69905

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4

#define TIMA 0xFF05
#define TMA 0xFF06
#define TMC 0xFF07

#define CLOCKSPEED 4194304

typedef unsigned char byte;
typedef unsigned short word;
typedef signed char signed_byte;
typedef signed short signed_word;

typedef union{
    word reg;
    struct{
        byte lo;
        byte hi;
    };
}Register;

typedef struct{
    SDL_Rect position;
    Uint8 color;
}PIXEL;

typedef struct{

  byte m_CartridgeMemory[0x200000];

  PIXEL m_ScreenData[160][144];

  byte m_Rom[0x10000];
  byte m_RAMBanks[0x8000];
  bool m_RomBanking;

  Register m_RegisterAF;
  Register m_RegisterBC;
  Register m_RegisterDE;
  Register m_RegisterHL;

  word m_ProgramCounter;
  Register m_StackPointer;
  
  byte m_JoypadState;

  bool m_MBC1;
  bool m_MBC2;

  byte m_CurrentROMBank;
  byte m_CurrentRAMBank;

  bool m_EnableRAM;
  int m_TimerCounter;
  int m_TimerSpeed;
  int m_DividerRegister;
  int m_DividerCounter;
  bool m_InteruptMaster;
  int m_ScanlineCounter;
  
  bool m_Halted;

  bool m_InteruptDelay;
  
  bool m_SkipInstruction;
  
}Z80;

Z80 z80;

typedef enum{
    WHITE,
    LIGHT_GREY,
    DARK_GREY,
    BLACK
}COLOR;

void initZ80();
void DetectMapper();
void CreateRamBanks(int);
void KeyPressed(int);
void KeyReleased(int);
byte GetJoypadState();
void WriteMemory(word, byte);
byte ReadMemory(word);
void HandleBanking(word, byte);
void DoRAMBankEnable(word, byte);
void DoChangeLoROMBank(byte);
void DoChangeHiROMBank(byte);
void DoRAMBankChange(byte);
void DoChangeROMRAMMode(byte);
void UpdateTimers(int);
bool IsClockEnabled();
byte GetClockFreq();
void SetClockFreq();
void DoDividerRegister(int);
void RequestInterupts(int);
void DoInterupts();
void ServiceInterupt(int);
void UpdateGraphics(int);
void SetLCDStatus();
bool IsLCDEnabled();
void DoDMATransfer(byte);
void DrawScanLine();
void RenderTiles(byte);
COLOR GetColor(byte, word);
void RenderSprites(byte);
void ExecuteNextOpcode();
void ExecuteOpcode(byte);
void ExecuteExtendedOpcode();
	


//ASM2C//
		word ReadWord();
		void CPU_8BIT_LOAD(byte&);
		void CPU_16BIT_LOAD(word&);
		void CPU_REG_LOAD(byte&, byte, int);
		void CPU_REG_LOAD_ROM(byte&, word);
		word CPU_8BIT_SIGNED_ADD(word&, byte, int);
		void CPU_8BIT_ADD(byte&, byte, int, bool, bool);
		void CPU_8BIT_SUB(byte&, byte, int, bool, bool);
		void CPU_8BIT_AND(byte&, byte, int, bool);
		void CPU_8BIT_OR(byte&, byte, int, bool);
		void CPU_8BIT_XOR(byte&, byte, int, bool);
		void CPU_8BIT_COMPARE(byte, byte, int, bool);
		void CPU_8BIT_INC(byte&, int);
		void CPU_8BIT_DEC(byte&, int);
		void CPU_8BIT_MEMORY_INC(word, int);
		void CPU_8BIT_MEMORY_DEC(word, int);
		void CPU_RESTARTS(byte);

		void CPU_16BIT_DEC(word&, int);
		void CPU_16BIT_INC(word&, int);
		void CPU_16BIT_ADD(word&, word, int);

		void CPU_JUMP(bool, int, bool);
		void CPU_JUMP_IMMEDIATE(bool, int, bool) ;
		void CPU_CALL(bool, int, bool);
		void CPU_RETURN(bool, int, bool);

		void CPU_SWAP_NIBBLES(byte&);
		void CPU_SWAP_NIB_MEM(word);
		void CPU_SHIFT_LEFT_CARRY(byte&);
		void CPU_SHIFT_LEFT_CARRY_MEMORY(word);
		void CPU_SHIFT_RIGHT_CARRY(byte&, bool);
		void CPU_SHIFT_RIGHT_CARRY_MEMORY(word, bool);

		void CPU_RESET_BIT(byte&, int);
		void CPU_RESET_BIT_MEMORY(word, int);
		void CPU_TEST_BIT(byte, int, int);
		void CPU_SET_BIT(byte&, int);
		void CPU_SET_BIT_MEMORY(word, int);

		void CPU_DAA();

		void CPU_RLC(byte&);
		void CPU_RLC_MEMORY(word);
		void CPU_RRC(byte&);
		void CPU_RRC_MEMORY(word);
		void CPU_RL(byte&);
		void CPU_RL_MEMORY(word);
		void CPU_RR(byte&);
		void CPU_RR_MEMORY(word);

		void CPU_SLA(byte&);
		void CPU_SLA_MEMORY(word);
		void CPU_SRA(byte&);
		void CPU_SRA_MEMORY(word);
		void CPU_SRL(byte&);
		void CPU_SRL_MEMORY(word);


#endif

