#include "screen.h"

const int DEFAULT_WIDTH = 320;
const int DEFAULT_HEIGHT = 288;

const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;

Screen::Screen()
{
	window = NULL;
	
	// Create window
	window = SDL_CreateWindow( 
		"SDL Tutorial", 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		DEFAULT_WIDTH, 
		DEFAULT_HEIGHT, 
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	
	// Get the window surface
	CreateWindowSurface();
	
	// Create the game boy surface
	CreateGameBoySurface();
}

void Screen::CreateWindowSurface()
{
	windowSurface = SDL_GetWindowSurface(window);
}

void Screen::CreateGameBoySurface()
{
	gameboySurface = SDL_CreateRGBSurface(0, GB_WIDTH, GB_HEIGHT, 32, 0, 0, 0, 0);
}

void Screen::OnEvent(SDL_Event* e)
{
	if (e->type == SDL_WINDOWEVENT)
	{
		switch (e->window.event)
		{
			// If screen is resized, get new window surface
			case SDL_WINDOWEVENT_RESIZED:
				CreateWindowSurface();
				break;
		}
	}
}

void Screen::Draw()
{
	SDL_BlitScaled(gameboySurface, NULL, windowSurface, NULL);
	SDL_UpdateWindowSurface(window);
}

void Screen::SetPixel(int x, int y, Uint32 color)
{
	uint32_t * pixels = (uint32_t *) gameboySurface->pixels;
	pixels[(y * gameboySurface->w) + x] = color;
}

void Screen::OnDestroy()
{
	SDL_DestroyWindow( window );
}
