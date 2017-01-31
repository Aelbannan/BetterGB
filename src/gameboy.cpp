#include "gameboy.h"
#include <time.h>
#include <thread>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

bool FRAMELIMITER_DEBUG = false;

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;

const float DESIRED_FRAME_TIME = 1/59.73;

std::ofstream* log;

using namespace std::chrono;

GameBoy::GameBoy()
{
	if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL_error: %s\n", SDL_GetError() );
		return;
	}
	else
	{
		char filename[MAX_PATH] = {0};
		
		#if _WIN32		
		OPENFILENAME ofn;

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.lpstrFilter = "Gameboy/Gameboy Color Games\0*.gb;*.gbc\0\0";
		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = "Browse";
		ofn.Flags = OFN_FILEMUSTEXIST;
		
		GetOpenFileName(&ofn);
		#endif
		
		// Create the screen class
		memory = new Memory(Cart::Load(filename));
		screen = new Screen();
		cpu = new CPU(memory);
		gpu = new GPU(cpu, memory, screen);
		joypad = new Joypad(memory, cpu);
	
		// Reset frame limiter vars
		frameStart = high_resolution_clock::now();
		timeBalance = 0;
		shouldSleep = false;
	
		#ifdef _WIN32
		timeBeginPeriod(1);
		#endif
		
		log = new std::ofstream("log.txt");
	
		// Reset all devices
		Reset();
		// Enter game loop
		Loop();
	}
}

void GameBoy::Reset()
{
	memory->Reset();
	cpu->Reset();
	gpu->Reset();
	joypad->Reset();
	
	ly = &memory->io[0x44];
}

void Wait()
{
	#ifdef _WIN32
		//timeBeginPeriod(1);
		Sleep(1);
	#else
		printf("UH NO");
	#endif
}

void GameBoy::Loop()
{
	bool quit = false;
	clock_t cpuTime, gpuTime, eventsTime, screenTime;
	high_resolution_clock::time_point frameTime;
	float frameTimeInSecs;
	
	while (!quit)
	{
		// Step events are called every cycle
		cpuTime = clock();
		cpu->Step();
		cpuTime = clock() - cpuTime;
		
		gpuTime = clock();
		gpu->Step();
		gpuTime = clock() - gpuTime;
				
		// If a frame has just finished
		if (*ly == 0)
		{
			// If should sleep
			if (shouldSleep)
			{
				// Called once per frame
				// Handle SDL Events
				eventsTime = clock();
				quit = HandleEvents();
				eventsTime = clock() - eventsTime;

				// Draw screen
				screenTime = clock();
				screen->Draw();
				screenTime = clock() - screenTime;
				
				// Calculate frame time
				frameTime = high_resolution_clock::now();
				frameTimeInSecs = duration_cast<microseconds>(frameTime - frameStart).count() / 1000000.0;
				
				// While we have not reached the desired frame time
				while (frameTimeInSecs < (DESIRED_FRAME_TIME - timeBalance))
				{
					// wait for 1ms
					SDL_Delay(1);
					
					// Calculate frame time
					frameTime = high_resolution_clock::now();
					frameTimeInSecs = duration_cast<microseconds>(frameTime - frameStart).count() / 1000000.0;
				}
				
				if (FRAMELIMITER_DEBUG)
				{
					printf("CPU   : %f ms\n", ((float)cpuTime) / CLOCKS_PER_SEC);
					printf("GPU   : %f ms\n", ((float)gpuTime) / CLOCKS_PER_SEC);
					printf("EVENTS: %f ms\n", ((float)eventsTime) / CLOCKS_PER_SEC);
					printf("SCREEN: %f ms\n", ((float)screenTime) / CLOCKS_PER_SEC);
					
					printf("TOTAL : %f ms\n", ((float)(cpuTime + gpuTime + eventsTime + screenTime)) / CLOCKS_PER_SEC);
					printf("DESIRED: %f ms, GOT: %f ms\n\n", DESIRED_FRAME_TIME - timeBalance, frameTimeInSecs);
				}
				
				// Calculate new time balance (time vs actual time)
				timeBalance = frameTimeInSecs - (DESIRED_FRAME_TIME - timeBalance);
				
				// Record new frame start time
				frameStart = high_resolution_clock::now();;
				
				// Don't sleep again this frame
				shouldSleep = false;
			}
		}
		else
		{
			// Reset for next frame
			shouldSleep = true;
		}
	}
	
	#ifdef _WIN32
		timeEndPeriod(1);
	#endif
	
	log->flush();
	log->close();
	
	SDL_Quit();		
}

bool GameBoy::HandleEvents()
{
	SDL_Event e;
	
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			return true;
		}
		else
		{
			screen->OnEvent(&e);
			joypad->OnEvent(&e);
		}
	}
	
	return false;
}