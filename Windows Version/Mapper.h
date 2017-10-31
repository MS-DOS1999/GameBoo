#ifndef MAPPER_H
#define MAPPER_H


//NOROM
void Write_NOROM(word, byte);
byte Read_NOROM(word);


//MBC1
void Write_MBC1(word, byte);
byte Read_MBC1(word);


//MBC2
void Write_MBC2(word, byte);
byte Read_MBC2(word);


//MBC3
bool rtc;
bool rtc_enabled;
bool rtc_latched;
byte rtc_latch_1;
byte rtc_latch_2;
byte rtc_reg[5];
byte latch_reg[5];

void grapTime();
void Write_MBC3(word, byte);
byte Read_MBC3(word);



#endif
