#ifndef __MEMORY__
#define __MEMORY__

#include <stdint.h>
#include "cart.h"

class GPU;
class Joypad;

class Memory
{
	public:
		GPU* gpu;
		Joypad* joypad;
	
		// Cart
		Cart* cart;

		// RAM
		uint8_t* ram; // RAM
		uint8_t* hram; // High RAM
		
		// VRAM
		uint8_t* vram;
		uint8_t* oam;
		
		// IO
		uint8_t* io;
		
		Memory(Cart* cart);
		// RESET
		void Reset();
		// READ
		uint8_t ReadByte(uint16_t address);
		uint16_t ReadShort(uint16_t address);
		// WRITE
		void SetByte(uint16_t address, uint8_t data);
		void SetShort(uint16_t address, uint16_t data);
		// GET POINTER
		uint8_t* GetBytePointer(uint16_t address);
		// INTERRUPTS
		void RequestInterrupt(uint8_t flag);
		
	private:
	
	
		// Loads a ROM
		void LoadRom(const char* filename);
		// Copies to OAM (FF46)
		void CopyToOAM(uint16_t address);
};

#endif