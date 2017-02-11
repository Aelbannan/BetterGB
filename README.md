# BetterGB
Gameboy Color emulator written in C++/SDL2

## Features
- Plays most original GB games (Basic, MBC1, MBC5)
- CPU completely implemented
- Interrupts and timers are now working
- Almost fully working PPU
- Fully functional memory handler
- Theoretically cross platform

## Need to Be Added:
- MBC3 Cart Type
- Add color palletes to GPU
- Make a platform independent game chooser (right now, only windows)
- Check if interrupts are 100% correct
- GBC specific functions 
  * Second VRAM
  * PPU Color palletes
- Audio

## Tested Games
- [x] Kirby's Dreamland
- [x] Link's Awakening
- [x] Tetris
- [x] Pokemon Green/Blue/Red
- [ ] Donkey Kong 
  * Error in the pre-game cinematic (background messed up)
- [ ] Pokemon Yellow
  * No color palletes/GBC specific memory implemented
