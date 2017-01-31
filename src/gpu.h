#ifndef __GPU__
#define __GPU__

#include "cpu.h"
#include "screen.h"

class Memory;

class GPU
{
	public:
		GPU(CPU* cpu, Memory* memory, Screen* screen);
		
		void Reset();
		void Step();
		
		//
		void OnSTAT(uint8_t data);
		
		// GB
		void OnBGP(uint8_t data);
		void OnOBP0(uint8_t data);
		void OnOBP1(uint8_t data);
		
		// CGB
		void OnBGPI(uint8_t data);
		void OnBGPD(uint8_t data);
		void OnOBPI(uint8_t data);
		void OnOBPD(uint8_t data);
		
	private:
		CPU* cpu;
		Memory* memory;
		Screen* screen;
		
		// If the screen is enabled
		bool enabled;
		// The mode the GPU is currently in
		int mode;
		// Is in color mode
		bool isCGB;
		
		// Cycles passed
		int cycleCount;
		int lyCount;
		
		// Memory references
		uint8_t *lcdc, *stat;
		uint8_t *ly, *lyc;
		uint8_t *scy, *scx, *wy, *wx;
		uint8_t *bgp, *obp0, *obp1;
		
		// Palettes
		uint32_t objPalette[8][4];
		uint32_t bgPalette[8][4];
		
		// Is Clear
		int bg_mask[160][144];
		
		void StartVBlank();
		void UpdateSTAT();
		void RequestInterrupt();
		
		void DrawBackground();
		void DrawSprites();
		void DrawWindow();
};

#endif