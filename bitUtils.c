#include "processor.h"

bool TestBit16(word data, int position){
    word mask = 1 << position;
    if(data & mask){
        return true;
    }
    else{
        return false;
    }
}

bool TestBit8(byte data, int position){
    byte mask = 1 << position;
    if(data & mask){
        return true;
    }
    else{
        return false;
    }
}

byte BitSet8(byte data, int position){
    byte mask = 1 << position;
    data |= mask;
    return data;
}

byte BitReset8(byte data, int position){
    byte mask = 1 << position;
    data &= ~mask;
    return data;
}

byte BitGetVal8(byte data, int position){
    byte mask = 1 << position;
    if(data & mask){
        return 1;
    }
    else{
        return 0;
    }
}


