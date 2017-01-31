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
		void Reset();
	private:
		Screen* screen;
		CPU* cpu;
		Memory* memory;
		GPU* gpu;
		Joypad* joypad;
		
		// Frame Limiter
		uint8_t* ly;
		std::chrono::high_resolution_clock::time_point frameStart;
		bool shouldSleep;
		float timeBalance;
	
		void Loop();
		bool HandleEvents();
};

#endif