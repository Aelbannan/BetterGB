#ifndef __JOYPAD__
#define __JOYPAD__

#include "cpu.h"
#include <SDL.h>

class Memory;

class Joypad
{
	public:
		Joypad(Memory* memory, CPU* cpu);
		void Reset();
		void OnEvent(SDL_Event* event);
		void OnJOYP(uint8_t data);
	
	private:
		Memory* memory;
		CPU* cpu;
		
		uint8_t* joyp;	
		bool keyState[8];
		
		void ProcessKeyPress(SDL_Keycode key, bool value);
		void UpdateInput();
};

#endif