#include "basic_cart.h"
#include <stdio.h>

uint8_t BasicCart::ReadROM(uint16_t address)
{
	return rom[address];
}

void BasicCart::WriteROM(uint16_t address, uint8_t data)
{
	// do nothing
}

uint8_t* BasicCart::GetROMPtr(uint16_t address)
{
	return &rom[address];
}

uint8_t BasicCart::ReadRAM(uint16_t address)
{
	printf("NO BASIC CART HAS RAM...");
	return 0;
}

void BasicCart::WriteRAM(uint16_t address, uint8_t data)
{
	printf("NO BASIC CART HAS RAM...");
}
