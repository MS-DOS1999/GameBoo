#ifndef BITSUTILS_HPP
#define BITSUTILS_HPP

typedef unsigned char byte;
typedef unsigned short word;
typedef signed char signed_byte;
typedef signed short signed_word;

template <typename type> 
bool TestBit(type data, int position)
{
	type mask = 1 << position;
	return (data & mask) ? true : false;
}

template <typename type> 
type BitSet(type data, int position)
{
	type mask = 1 << position;
	data |= mask;
	return data;
}

template <typename type> 
type BitClear(type data, int position)
{
	type mask = 1 << position;
	data &= ~mask;
	return data;
}

template <typename type> 
type BitGetVal(type data, int position)
{
	type mask = 1 << position;
	return (data & mask) ? 1 : 0;
}


#endif