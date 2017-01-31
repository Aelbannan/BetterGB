#ifndef __SCREEN__
#define __SCREEN__

#include <SDL.h>
#include <stdio.h>

class Screen
{
	public:
		SDL_Surface* gameboySurface;
	
		Screen();
		void OnEvent(SDL_Event* e);
		void Draw();
		void SetPixel(int x, int y, uint32_t color);
		void OnDestroy();
	private:
		SDL_Window* window;
		SDL_Surface* windowSurface;	
			
		void CreateWindowSurface();
		void CreateGameBoySurface();
};

#endif