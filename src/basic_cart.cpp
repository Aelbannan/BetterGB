#include "basic_cart.h"
#include <stdio.h>
 
uint8_t BasicCart::ReadROM(uint16_t address)
{
	return rom[address];
}

// Do nothing, basic cart can't be written to
void BasicCart::WriteROM(uint16_t address, uint8_t data)
{
}

// Return byte @ address
uint8_t* BasicCart::GetROMPtr(uint16_t address)
{
	return &rom[address];
}

// Error, basic cart has no ram
uint8_t BasicCart::ReadRAM(uint16_t address)
{
	printf("NO BASIC CART HAS RAM...");
	return 0;
}

// Error, basic cart has no ram
void BasicCart::WriteRAM(uint16_t address, uint8_t data)
{
	printf("NO BASIC CART HAS RAM...");
}
