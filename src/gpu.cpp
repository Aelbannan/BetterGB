#include "gpu.h"
#include "memory.h"
#include <stdio.h>
#include <iostream>

const int MODE_0_CYCLES = 204;	// HBLANK
const int MODE_1_CYCLES = 4560;	// VBLANK
const int MODE_2_CYCLES = 80;	// READ OAM
const int MODE_3_CYCLES = 172;	// TRANSFER TO LCD
const int LY_CYCLES = 456;

const int FLAG_VBLANK = 0x01;
const int FLAG_LCD_STAT = 0x02;

const int LCDC_MODE = 0x3;
const int FLAG_LY_COINCIDENCE = 0x04;
const int FLAG_HBLANK_INTERRUPT = 0x08;
const int FLAG_VBLANK_INTERRUPT = 0x10;
const int FLAG_OAM_INTERRUPT = 0x20;
const int FLAG_LYC_ENABLE = 0x40;

bool SPRITE_DEBUG = false;
extern bool OPCODE_DEBUG;

class Memory;

using namespace std;
GPU::GPU(CPU* cpu, Memory* memory, Screen* screen)
{
	this->cpu = cpu;
	this->memory = memory;
	this->screen = screen;
	
	memory->gpu = this;
}

void GPU::Reset()
{
	mode = 0;
	cycleCount = 0;
	lyCount = 0;
	
	isCGB = memory->cart->isCGB;
	
	lcdc	= &memory->io[0x40];
	stat	= &memory->io[0x41];
	scy		= &memory->io[0x42];
	scx		= &memory->io[0x43];
	ly		= &memory->io[0x44];
	lyc		= &memory->io[0x45];
	bgp		= &memory->io[0x47];
	obp0	= &memory->io[0x48];
	obp1	= &memory->io[0x49];
	wy		= &memory->io[0x4A];
	wx		= &memory->io[0x4B];
	
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			bgPalette[i][j] = 0;
			objPalette[i][j] = 0;
		}
	}
}	

void GPU::Step()
{
	cycleCount += cpu->lastInstructionCycles;
	lyCount += cpu->lastInstructionCycles;
	
	// Keep incrementing LY (VBLANK will reset it)
	if (lyCount >= LY_CYCLES)
	{
		lyCount -= LY_CYCLES;
		// if not vblank, draw
		if (*ly < 144)
		{
			// Draw backgrounds
			DrawBackground();
			
			if (*lcdc & 0x20)
				DrawWindow();
			
			// If sprites are enabled, draw sprites
			if (*lcdc & 0x02)
				DrawSprites();
		}
		// Increment ly
		(*ly)++;
		
		// LCDC Interrupt (If LY == LYC)
		if ((*stat & FLAG_LYC_ENABLE) && (*ly == *lyc))
		{
			RequestInterrupt();
		}
	}
	
	// FLOW: 2 -> 3 -> 0 (REPEAT x144) -> 1
	switch (mode)
	{
		case 0:
			if (cycleCount >= MODE_0_CYCLES)
			{
				cycleCount -= MODE_0_CYCLES;
				
				// Check if time for VBLANK, otherwise MODE2
				if (*ly == 144)
				{
					mode = 1;
					StartVBlank();
				}
				else
				{
					mode = 2;
					
					// If OAM interrupts are enabled
					if ((*stat & FLAG_OAM_INTERRUPT))
					{
						RequestInterrupt();
					}
				}
			}
			break;
			
		case 1:
			if (cycleCount >= MODE_1_CYCLES)
			{
				cycleCount -= MODE_1_CYCLES;
				// Frame is done, reset LY
				*ly = 0;
				// Go to mode 2
				mode = 2;
				
				// If OAM interrupts are enabled
				if ((*stat & FLAG_OAM_INTERRUPT))
				{
					RequestInterrupt();
				}
			}
			break;
			
		case 2:
			if (cycleCount >= MODE_2_CYCLES)
			{
				cycleCount -= MODE_2_CYCLES;
				// Go to mode 3
				mode = 3;
			}
			break;
			
		case 3:
			if (cycleCount >= MODE_3_CYCLES)
			{
				cycleCount -= MODE_3_CYCLES;
				// 
				mode = 0;
				//
				if (*stat & FLAG_HBLANK_INTERRUPT)
				{
					RequestInterrupt();
				}
			}
			break;
	}
	
	UpdateSTAT();
}

void GPU::StartVBlank()
{
	// regular VBLANK interrupt
	memory->RequestInterrupt(FLAG_VBLANK);
	
	// STAT interrupt (VBLANK) if enabled
	if (*stat & FLAG_VBLANK_INTERRUPT)
	{
		RequestInterrupt();
	}
}

void GPU::UpdateSTAT()
{
	// Clear LY coincidence and MODE
	*stat &= ~(FLAG_LY_COINCIDENCE | LCDC_MODE);
	// If LY == LYC, set flag
	if (*ly == *lyc)
		*stat |= FLAG_LY_COINCIDENCE;
	// Set mode
	*stat += (mode & 0x3);
}

void GPU::RequestInterrupt()
{
	// Clear LY coincidence and MODE
	*stat &= ~(FLAG_LY_COINCIDENCE | LCDC_MODE);
	// If LY == LYC, set flag
	if (*ly == *lyc)
		*stat |= FLAG_LY_COINCIDENCE;
	// Set mode
	*stat += (mode & 0x3);
	// Set the interrupt flag
	memory->RequestInterrupt(FLAG_LCD_STAT);
}

uint32_t GetShade(uint8_t num)
{
	switch (num)
	{
		case 0: return 0xFFFFFFFF;
		case 1: return 0xFF808080;
		case 2: return 0xFF404040;
		case 3: return 0xFF000000;
		default: return 0xFFFFFFFF;
	}
}

void GPU::OnSTAT(uint8_t data)
{
	if ((data & 0x08) && (mode == 0))
	{
		RequestInterrupt();
	}
	else if ((data & 0x10) && (mode == 1))
	{
		RequestInterrupt();
	}
	else if ((data & 0x20) && (mode == 2))
	{
		RequestInterrupt();
	}
	else if ((data & 0x04) && (*ly == *lyc))
	{
		RequestInterrupt();
	}
	
	*stat = 0x80 | (data & 0xE8);
}

void GPU::OnBGP(uint8_t data)
{
	bgPalette[0][0] = GetShade(data & 0x03);
	bgPalette[0][1] = GetShade((data & 0x0C) >> 2);
	bgPalette[0][2] = GetShade((data & 0x30) >> 4);
	bgPalette[0][3] = GetShade((data & 0xC0) >> 6);
	
	*bgp = data;
}

void GPU::OnOBP0(uint8_t data)
{
	objPalette[0][1] = GetShade((data & 0x0C) >> 2);
	objPalette[0][2] = GetShade((data & 0x30) >> 4);
	objPalette[0][3] = GetShade((data & 0xC0) >> 6);
	
	*obp0 = data;
}

void GPU::OnOBP1(uint8_t data)
{
	objPalette[1][1] = GetShade((data & 0x0C) >> 2);
	objPalette[1][2] = GetShade((data & 0x30) >> 4);
	objPalette[1][3] = GetShade((data & 0xC0) >> 6);
	
	*obp1 = data;
}

// CGB
void GPU::OnBGPI(uint8_t data)
{
	
}

void GPU::OnBGPD(uint8_t data)
{
	
}

void GPU::OnOBPI(uint8_t data)
{
	
}

void GPU::OnOBPD(uint8_t data)
{

}

void GPU::DrawBackground()
{
	// If the background should be shown, or all white
	bool showBackground = *lcdc & 0x01;
	// Find which tile map to use (depending on LCDC)
	int dataAddr = (*lcdc & 0x10)? 0x0000 : 0x1000;
	int bgMapAddr = (*lcdc & 0x08)? 0x1C00: 0x1800;
	
	// Calculate Y (y + scrollY % 256)
	int wrappedY = (*ly + *scy) & 0xFF;
	// Get Y on map
	int mapY = wrappedY / 8;
	// Get Y in tile (wrappedY % 8)
	int tileY = wrappedY & 7;
	
	// If background is disabled, fill with white
	if (!showBackground)
	{
		for (int x = 0; x < 160; x++)
		{
			screen->SetPixel(x, *ly, 0xFFFFFFFF);
			bg_mask[x][*ly] = 0;
		}
		
		return;
	}
	
	// Otherwise, draw backgrounds
	for (int x = 0; x < 160; x++)
	{
		// Calculate X (x + scrollX % 256)
		uint32_t wrappedX = (x + *scx) & 0xFF;
		// Get X on map
		int mapX = wrappedX / 8;
		// Get tile x (wrappedX % 8)
		int tileX = wrappedX & 7;
		
		// Map Tile = Map Base Addr + (y * 32) + x
		// 32 is map width
		int tileIndex = memory->vram[bgMapAddr + (mapY << 5) + mapX];
		// If using the second Tile Data area, convert to signed char
		if (dataAddr == 0x1000) tileIndex = (int8_t)tileIndex;
		
		// Line = 2 bytes @ DataBaseAddr + (tileIndex * tileSize) + (tileY * lineSize)
		int lsb = memory->vram[dataAddr + tileIndex * 16 + tileY * 2];
		int msb = memory->vram[dataAddr + tileIndex * 16 + tileY * 2 + 1];
		
		// Calculate color index
		int color_index;;
		color_index =  (lsb & (0x80 >> tileX))? 0x1 : 0x0;
		color_index += (msb & (0x80 >> tileX))? 0x2 : 0x0;
		
		// Draw pixel
		//if (!bg_mask[x][*ly])
			screen->SetPixel(x, *ly, bgPalette[0][color_index]);
		
		// If color == 0, then is 'transparent'
		bg_mask[x][*ly] = color_index? 1 : 0;
	}
}

void GPU::DrawSprites()
{
	// LCDC Attributes
	bool is8x16 = *lcdc & 0x04;
	
	// Calculate sprite height
	int sprite_height = (is8x16)? 16 : 8;
	
	for (int sprite = 39; sprite >= 0; sprite--)
	{
		int sprite_index = sprite * 4;
		
		// Get Y
		int sprite_y = memory->oam[sprite_index] - 16;
		
		// If not on line, skip
		if (sprite_y > *ly || (sprite_y + sprite_height) <= *ly)
			continue;
		
		// Get X
		int sprite_x = memory->oam[sprite_index + 1] - 8;
		
		// If not on screen, skip
		if (sprite_x == -8 || sprite_x >= 160)
			continue;
		
		// Get tile index
		int tile_index = memory->oam[sprite_index + 2] & (is8x16 ? 0xFE : 0xFF);
		// Get sprite attributes
		int sprite_attributes = memory->oam[sprite_index + 3];
		
		bool x_flip = sprite_attributes & 0x20;
		bool y_flip = sprite_attributes & 0x40;
		int palette = (sprite_attributes & 0x10) >> 4;//(isCGB)? sprite_attributes & 0x07;
		bool behind_bg = sprite_attributes & 0x80;
		
		int tile_y = *ly - sprite_y;
		
		if (y_flip)
		{
			tile_y = ((is8x16)? 15 : 7) - tile_y;
		}
		
		if (tile_y >= 8)
		{
			tile_index |= 1;
			tile_y -= 8;
		}

		// Get the tile line
		int lsb, msb;
		lsb = memory->vram[tile_index * 16 + tile_y * 2];
		msb = memory->vram[tile_index * 16 + tile_y * 2 + 1];
		
		// Calculate sprite X bounds
		int start = (sprite_x < 0)? 0 - sprite_x : 0;
		int end = (sprite_x + 7 >= 160)? 160 - sprite_x : 8;
		
		for (int tile_x = start; tile_x < end; tile_x++)
		{
			int color_index;
			color_index =  (lsb & (0x01 << (x_flip? tile_x : (7 - tile_x))))? 0x1 : 0x0;
			color_index += (msb & (0x01 << (x_flip? tile_x : (7 - tile_x))))? 0x2 : 0x0;
			
			int bg_mode = bg_mask[sprite_x + tile_x][*ly];
			
			// If not transparent, draw
			if (color_index && (!behind_bg || bg_mode == 0))
			{
				screen->SetPixel(sprite_x + tile_x, *ly, objPalette[palette][color_index]);
			}
		}
	}
}

void GPU::DrawWindow()
{	
	if (*wx > 166 || *wy > 143 || *wy > *ly)
		return;
	
	//printf("X %d Y %d \n", *wx, *wy);
	
	// Find which tile map to use (depending on LCDC)
	int dataAddr = (*lcdc & 0x10)? 0x0000 : 0x1000;
	int bgMapAddr = (*lcdc & 0x40)? 0x1C00: 0x1800;
	
	//printf("%d\n", *wy);
	
	// Calculate Y (y + scrollY % 256)
	int wrappedY = (*ly - *wy);
	// Get Y on map
	int mapY = wrappedY / 8;
	// Get Y in tile (wrappedY % 8)
	int tileY = wrappedY & 7;
	
	// Otherwise, draw backgrounds
	for (int x = *wx - 7; x < 160; x++)
	{
		// Get X on map
		int mapX = x / 8;
		// Get tile x (wrappedX % 8)
		int tileX = x & 7;
		
		// Map Tile = Map Base Addr + (y * 32) + x
		// 32 is map width
		int tileIndex = memory->vram[bgMapAddr + (mapY << 5) + mapX];
		// If using the second Tile Data area, convert to signed char
		if (dataAddr == 0x1000) tileIndex = (int8_t)tileIndex;
		
		// Line = 2 bytes @ DataBaseAddr + (tileIndex * tileSize) + (tileY * lineSize)
		int lsb = memory->vram[dataAddr + tileIndex * 16 + tileY * 2];
		int msb = memory->vram[dataAddr + tileIndex * 16 + tileY * 2 + 1];
		
		// Calculate color index
		int color_index;;
		color_index =  (lsb & (0x80 >> tileX))? 0x1 : 0x0;
		color_index += (msb & (0x80 >> tileX))? 0x2 : 0x0;
		
		bg_mask[x][*ly] = color_index? 1 : 0;
		
		// Draw pixel
		screen->SetPixel(x, *ly, bgPalette[0][color_index]);
	}
}