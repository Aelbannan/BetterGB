#include "joypad.h"
#include "memory.h"
#include <stdio.h>

const int 
	DIR_RIGHT = 0,
	DIR_LEFT = 1,
	DIR_UP = 2,
	DIR_DOWN = 3,
	BUT_A = 4,
	BUT_B = 5,
	BUT_SELECT = 6,
	BUT_START = 7;
const int FLAG_JOYPAD = 1 << 4;

SDL_Keycode keycodes[8]
{
	SDLK_RIGHT,
	SDLK_LEFT,
	SDLK_UP,
	SDLK_DOWN,
	SDLK_z,
	SDLK_x,
	SDLK_a,
	SDLK_s,
};

extern bool OPCODE_DEBUG;
extern bool FRAMELIMITER_DEBUG;

Joypad::Joypad(Memory* memory, CPU* cpu)
{
	this->memory = memory;
	this->cpu = cpu;
	
	memory->joypad = this;
}

void Joypad::Reset()
{
	joyp = &memory->io[0x00];
	
	for (int i = 0; i < 8; i++)
		keyState[i] = false;
}

void Joypad::OnEvent(SDL_Event* e)
{
	if (e->type == SDL_KEYDOWN)
	{
		ProcessKeyPress(e->key.keysym.sym, true);
	}
	else if (e->type == SDL_KEYUP)
	{
		ProcessKeyPress(e->key.keysym.sym, false);
	}
}

void Joypad::ProcessKeyPress(SDL_Keycode key, bool value)
{
	if (key == SDLK_F1)
	{
		OPCODE_DEBUG = true;
	}
	
	if (key == SDLK_F2)
	{
		FRAMELIMITER_DEBUG = true;
	}
	
	for (int i = 0; i < 8; i++)
	{
		// If right key
		if (keycodes[i] == key)
		{
			// FIX
			/*
			You must set the P1 register ($ff00) in order to do this:
			P1_REG = $20 ; Cause U,D,L,R to joypad interrupt
			P1_REG = $10 ; Cause A,B,SELECT,START to joypad interrupt
			P1_REG = $00 ; Cause any button to joypad interrupt
			*/
			// If high->low, request joypad interrupt
			if (keyState[i] == false && value == true)
			{
				memory->RequestInterrupt(FLAG_JOYPAD);
			}
			
			// Set key state
			keyState[i] = value;
			
			// Update JOYP register
			UpdateInput();
			
			// Reset cpu STOP
			cpu->isStopped = false;
			
			break;
		}
	}
}

void Joypad::OnJOYP(uint8_t data)
{
	*joyp = data;
	UpdateInput();
}

void Joypad::UpdateInput()
{
	// Set top 2 bits, reset bottom 4
	*joyp |= 0xC0;
	*joyp &= 0xF0;
	
	// If buttons
	if (!(*joyp & 0x20))
	{
		if (!keyState[4]) *joyp |= 0x1;
		if (!keyState[5]) *joyp |= 0x2;
		if (!keyState[6]) *joyp |= 0x4;
		if (!keyState[7]) *joyp |= 0x8;
	}
	// Otherwise direction keys
	else
	{
		if (!keyState[0]) *joyp |= 0x1;
		if (!keyState[1]) *joyp |= 0x2;
		if (!keyState[2]) *joyp |= 0x4;
		if (!keyState[3]) *joyp |= 0x8;
	}
}