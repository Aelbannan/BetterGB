#include "mbc1_cart.h"
#include <stdio.h>

const int ROM_BASE_ADDR = 0x4000;
const int RAM_BASE_ADDR = 0xA000;
const int ROM_BANK_SIZE = 0x4000;
const int RAM_BANK_SIZE = 0x2000;
const int ROM_BANK_SHIFT = 14;
const int RAM_BANK_SHIFT = 13;

uint8_t MBC1Cart::ReadROM(uint16_t address)
{
	if (address <= 0x3FFF)
	{
		return rom[address];
	}
	else
	{
		int baseAddress;
		// Calculate bank
		baseAddress = (ramSelect)? bankNumber & 0x1F : bankNumber;
		baseAddress <<= ROM_BANK_SHIFT;
		// Calculate bank relative address
		address = address - ROM_BASE_ADDR;
				
		return rom[baseAddress + address];
	}
}

void MBC1Cart::WriteROM(uint16_t address, uint8_t data)
{
	if (address <= 0x1FFF) 
	{
		// RAM Enable (Nothing to do really)
	}
	else if (address <= 0x3FFF)
	{
		// Lower 5 bits of bank number
		// Clear bottom 5 bits
		bankNumber &= ~0x1F;
		// Replace 0 with 1
		if (data == 0x00)
		{
			data = 0x01;
		}
		// Set bank number (only bottom 5 bits)
		bankNumber |= data;
	}
	else if (address <= 0x5FFF)
	{
		// Bit 5-6 of bank number
		// Clear bits 5 & 6
		bankNumber &= ~(0x03 << 5);
		// Set bits 5 & 6
		bankNumber |= (data & 0x03) << 5;
	}
	else
	{
		// Set ROM or RAM select
		ramSelect = ((data & 0x0A) == 0x0A);
	}
}

uint8_t* MBC1Cart::GetROMPtr(uint16_t address)
{
	printf("BANK 0x%02x\n", bankNumber);
	if (address <= 0x3FFF)
	{
		return &rom[address];
	}
	else
	{
		int baseAddress;
		// Calculate bank
		baseAddress = (ramSelect)? bankNumber & 0x1F : bankNumber;
		baseAddress <<= ROM_BANK_SHIFT;
		// Calculate bank relative address
		address = address - ROM_BASE_ADDR;
				
		return &rom[baseAddress + address];
	}
}

uint8_t MBC1Cart::ReadRAM(uint16_t address)
{
	if (ramSelect)
	{
		int baseAddress;
		// Calculate bank
		baseAddress = (bankNumber >> 5) & 0x03;
		baseAddress <<= RAM_BANK_SHIFT;
		// Calculate relative address
		address = address - RAM_BASE_ADDR;
		
		return ram[baseAddress + address];
	}
	else
	{
		return ram[address - RAM_BASE_ADDR];
	}
}

void MBC1Cart::WriteRAM(uint16_t address, uint8_t data)
{
	if (ramSelect)
	{
		int baseAddress;
		// Calculate bank
		baseAddress = (bankNumber >> 5) & 0x03;
		baseAddress <<= RAM_BANK_SHIFT;
		// Calculate relative address
		address = address - RAM_BASE_ADDR;
		
		ram[baseAddress + address] = data;
	}
	else
	{
		ram[address - RAM_BASE_ADDR] = data;
	}
}
