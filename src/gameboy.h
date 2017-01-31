#ifndef __GAMEBOY__
#define __GAMEBOY__

#include "screen.h"
#include "cpu.h"
#include "memory.h"
#include "gpu.h"
#include "joypad.h"
#include <chrono>

class GameBoy
{
	public:
		GameBoy();
		// Resets the gameboy
		void Reset();
	private:
		Screen* screen;
		CPU* cpu;
		Memory* memory;
		GPU* gpu;
		Joypad* joypad;
		
		// Frame Limiter Variables
		uint8_t* ly; // LY (current redraw line)
		std::chrono::high_resolution_clock::time_point frameStart; // Time frame started
		bool shouldSleep; // Should we sleep
		float timeBalance; // Excess/Missing time from previous frames
	
		void Loop();
		bool HandleEvents();
};

#endif