#include "memory.h"
#include "gpu.h"
#include "joypad.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

bool MEM_DEBUG;
extern bool SPRITE_DEBUG;
extern bool OPCODE_DEBUG;
extern std::ofstream* log;

class GPU;

using namespace std;
Memory::Memory(Cart* cart)
{
	this->cart = cart;
	
	ram 	= (uint8_t*)new unsigned char[0x1007E];
	vram 	= (uint8_t*)new unsigned char[0x10000];
	oam 	= (uint8_t*)new unsigned char[0xA0];
	io 		= (uint8_t*)new unsigned char[0x100];
	hram 	= (uint8_t*)new unsigned char[0x80];
}

void Memory::Reset()
{
	// Reset IO
	io = (uint8_t*) new unsigned char[0x100]
	{
		0x0F, 0x00, 0x7C, 0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
		0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
		0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
		0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xC1, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,
		0xF8, 0xFF, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
		0x45, 0xEC, 0x52, 0xFA, 0x08, 0xB7, 0x07, 0x5D, 0x01, 0xFD, 0xC0, 0xFF, 0x08, 0xFC, 0x00, 0xE5,
		0x0B, 0xF8, 0xC2, 0xCE, 0xF4, 0xF9, 0x0F, 0x7F, 0x45, 0x6D, 0x3D, 0xFE, 0x46, 0x97, 0x33, 0x5E,
		0x08, 0xEF, 0xF1, 0xFF, 0x86, 0x83, 0x24, 0x74, 0x12, 0xFC, 0x00, 0x9F, 0xB4, 0xB7, 0x06, 0xD5,
		0xD0, 0x7A, 0x00, 0x9E, 0x04, 0x5F, 0x41, 0x2F, 0x1D, 0x77, 0x36, 0x75, 0x81, 0xAA, 0x70, 0x3A,
		0x98, 0xD1, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0x0B, 0x00
	};
	
	// Reset VRAM
	for (int i = 0; i < 0x10000; i++)
		vram[i] = 0x00;
	
	// Reset OAM
	for (int i = 0; i < 0xA0; i++)
		oam[i] = 0x00;
	
	// Reset IE
	for (int i = 0; i < 0x80; i++)
		hram[i] = 0x00;
}

void Memory::RequestInterrupt(uint8_t data)
{
	io[0x0F] |= data;
}

uint8_t Memory::ReadByte(uint16_t address)
{
	if (address <= 0x7FFF)
	{
		return cart->ReadROM(address); // ROM
	}
	else if (address <= 0x9FFF)
	{
		return vram[address - 0x8000]; // VRAM
	}
	else if (address <= 0xBFFF)
	{
		// EXTERNAL RAM (in Cartridge)
		return cart->ReadRAM(address);
	}
	else if (address <= 0xDFFF)
	{
		return ram[address - 0xC000]; // RAM
	}
	else if (address <= 0xFDFF)
	{
		return ram[address - 0xE000]; // RAM SHADOW
	}
	else if (address <= 0xFE9F)
	{
		return oam[address - 0xFE00]; // SPRITE OAM
	}
	else if (address <= 0xFEFF)
	{
		return 0x00;
	}
	else
	{
		return io[address - 0xFF00];
	}
}

uint16_t Memory::ReadShort(uint16_t address)
{
	uint16_t data;
	data = ReadByte(address) + (ReadByte(address + 1) << 8);
	return data;
	//uint16_t* ptr = (uint16_t*) GetBytePointer(address);
	//return (*ptr);
}

void Memory::SetByte(uint16_t address, uint8_t data)
{
	if (address <= 0x7FFF)
	{
		cart->WriteROM(address, data); // ROM
	}
	else if (address <= 0x9FFF)
	{
		vram[address - 0x8000] = data; // VRAM
	}
	else if (address <= 0xBFFF)
	{
		cart->WriteRAM(address, data);
	}
	else if (address <= 0xDFFF)
	{
		ram[address - 0xC000] = data; // RAM
	}
	else if (address <= 0xFDFF)
	{
		ram[address - 0xE000] = data; // RAM SHADOW
	}
	else if (address <= 0xFE9F)
	{
		oam[address - 0xFE00] = data; // SPRITE OAM
	}
	else if (address <= 0xFEFF)
	{
	}
	else
	{
		switch (address)
		{
			case 0xFF00: joypad->OnJOYP(data); return;
			//case 0xFF02: if (data == 0x81) cout << io[0x01]; break;
			case 0xFF04: io[0x04] = 0; return; // Reset DIV
			case 0xFF41: gpu->OnSTAT(data); return;
			case 0xFF44: io[0x44] = 0; return; // Reset LY
			case 0xFF47: gpu->OnBGP(data); return;
			case 0xFF48: gpu->OnOBP0(data); return;
			case 0xFF49: gpu->OnOBP1(data); return;
			case 0xFF68: gpu->OnBGPI(data); return;
			case 0xFF69: gpu->OnBGPD(data); return;
			case 0xFF6A: gpu->OnBGPI(data); return;
			case 0xFF6B: gpu->OnBGPI(data); return;
			case 0xFF46: CopyToOAM(data << 8); return; // COPY TO OAM
		};
		
		// If no return, then write to IO
		io[address - 0xFF00] = data;
	}
}

void Memory::CopyToOAM(uint16_t address)
{
	for (int i = 0; i < 0x9F; i++)
	{
		oam[i] = ReadByte(address + i);
	}
}

void Memory::SetShort(uint16_t address, uint16_t data)
{
	SetByte(address, (uint8_t) data);
	SetByte(address + 1, (uint8_t)(data >> 8));
}

uint8_t* Memory::GetBytePointer(uint16_t address)
{
	if (address <= 0x7FFF)
	{
		// UHH WHAT
		printf("ARE YOU STUPID? 0x%04x\n", address);
		//OPCODE_DEBUG = true;
		return cart->GetROMPtr(address);
	}
	else if (address <= 0x9FFF)
	{
		return &vram[address - 0x8000]; // VRAM
	}
	else if (address <= 0xBFFF)
	{
		// EXTERNAL RAM (in Cartridge)
		printf("WHY ERAM POINTER...");
		return NULL;
	}
	else if (address <= 0xDFFF)
	{
		return &ram[address - 0xC000]; // RAM
	}
	else if (address <= 0xFDFF)
	{
		return &ram[address - 0xE000]; // RAM SHADOW
	}
	else if (address <= 0xFE9F)
	{
		return &oam[address - 0xFE00]; // SPRITE OAM
	}
	else if (address <= 0xFEFF)
	{
		return NULL;
	}
	else
	{
		return &io[address - 0xFF00];
	}
}