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

  bool m_MBC1;
  bool m_MBC2;

  byte m_CurrentROMBank;
  byte m_CurrentRAMBank;

  bool m_EnableRAM;
  int m_TimerCounter;
  int m_DividerRegister;
  int m_DividerCounter;
  bool m_InteruptMaster;
  int m_ScanlineCounter;

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

#endif

