#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <iostream>
#include <fstream>

bool OPCODE_DEBUG = false;
extern bool MEM_DEBUG;
extern std::ofstream* log;

const int CYCLES_PER_SECOND = 4194304;

// F REGISTER FLAGS
const int FLAG_CARRY = 1 << 4;
const int FLAG_HALF_CARRY = 1 << 5;
const int FLAG_NEG = 1 << 6;
const int FLAG_ZERO = 1 << 7;

// IME STATES
const int FLAG_IME_WAIT = 0x01;
const int IME_ON = 0x04;
const int IME_OFF = 0x02;
const int IME_WAIT_ON = 0x03;
const int IME_WAIT_OFF = 0x01;

// INTERRUPT FLAGS
const int FLAG_VBLANK = 0x01;
const int FLAG_LCD_STAT = 0x02;
const int FLAG_TIMER = 0x04;
const int FLAG_SERIAL = 0x08;
const int FLAG_JOYPAD = 0x10;

// INTERRUPT ADDRESSES
const int VBLANK_ADDR = 0x40;
const int LCD_STAT_ADDR = 0x48;
const int TIMER_ADDR = 0x50;
const int SERIAL_ADDR = 0x58;
const int JOYPAD_ADDR = 0x60;

// TIMER

CPU::CPU(Memory* memory)
{
	this->memory = memory;
	
	Setup();
}

void CPU::Reset()
{
	// Register default values
	registers.af = 0x01B0;
	registers.bc = 0x0013;
	registers.de = 0x00D8;
	registers.hl = 0x014D;
	registers.pc = 0x0100;
	registers.sp = 0xFFFE;
	
	// State
	interruptMaster = true;
	imeState = 0;
	isHalted = false;
	isStopped = false;
	
	// Link to memory
	div 	= &memory->io[0x04];
	tima 	= &memory->io[0x05];
	tma 	= &memory->io[0x06];
	tac 	= &memory->io[0x07];
	interruptFlags 		= &memory->io[0x0F];
	interruptsEnabled 	= &memory->io[0xFF];
}

void CPU::Step()
{
	if (isStopped)
	{
		lastInstructionCycles = 4;
		return;
	}
	else
	{
		lastInstructionCycles = 0;
		
		if (registers.pc == 0x3804)//0x361A)//0x98E)
		{
			//MEM_DEBUG = true;
			//OPCODE_DEBUG = true;
			//printf("Last addr 0x%04x", Pop16());
		}
		
		// If IME is waiting to be changed
		if (imeState)
		{
			if (imeState & FLAG_IME_WAIT)
			{
				// MOVE to NON WAIT state
				imeState++;
			}
			else
			{
				// Set IME
				interruptMaster = (imeState == IME_OFF)? false : true;
				// Reset IME state
				imeState = 0;		
			}
		}
		
		// Handle interrupts
		ExecuteInterrupts();
		
		if (isHalted)
		{
			lastInstructionCycles = 4;
		}
		else
		{
			ExecuteOpcode();
		}
		
		divCycles += lastInstructionCycles;
		timerCycles += lastInstructionCycles;
		
		UpdateTimer();
	}
}

void CPU::ExecuteInterrupts()
{
	if (interruptMaster)
	{
		int waiting = *interruptFlags & *interruptsEnabled;
		
		if (waiting)
		{
			//printf("INTERRUPT 0x%02x", waiting);
			
			// Push current intstruction
			Push16(registers.pc);
			
			// Handle interrupt
			if (waiting & FLAG_VBLANK)
			{
				registers.pc = VBLANK_ADDR;
				*interruptFlags &= ~FLAG_VBLANK;
			}
			else if (waiting & FLAG_LCD_STAT)
			{
				registers.pc = LCD_STAT_ADDR;
				*interruptFlags &= ~FLAG_LCD_STAT;
			}
			else if (waiting & FLAG_TIMER)
			{
				registers.pc = TIMER_ADDR;
				*interruptFlags &= ~FLAG_TIMER;
			}
			else if (waiting & FLAG_SERIAL)
			{
				registers.pc = SERIAL_ADDR;
				*interruptFlags &= ~FLAG_SERIAL;
			}
			else if (waiting & FLAG_JOYPAD)
			{
				registers.pc = JOYPAD_ADDR;
				*interruptFlags &= ~FLAG_JOYPAD;
			}
			
			// Disable interrupts
			interruptMaster = false;
			// Not halted any more
			isHalted = false;
		}
	}
}

void CPU::ExecuteOpcode()
{
	uint8_t opcode;
	Instruction instruction;
	
	// Get instruction
	opcode = ReadNextByte();
	
	if (opcode == 0xCB)
	{
		// Get actual opcode
		opcode = ReadNextByte();
		instruction = instructions[opcode + 256];
		lastInstructionCycles += instructionCycles[opcode + 256];
		
		//*log << (uint8_t)0xCB;
		
		if (instruction.function == NULL)
		{
			printf("Unimplemented instruction CB 0x%02X at 0x%04x\n", opcode, registers.pc - 1);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		instruction = instructions[opcode];
		lastInstructionCycles += instructionCycles[opcode];
		
		if (instruction.function == NULL)
		{
			printf("Unimplemented instruction 0x%02X at 0x%04x\n", opcode, registers.pc - 1);
			exit(EXIT_FAILURE);
		}
	}
	
	//*log << opcode;
	
	// DEBUG
	if (OPCODE_DEBUG)
	{
		printf("0x%02X ", opcode);
	
		switch (instruction.operandLength)
		{
			case 0: printf(instruction.label); break;
			case 1: printf(instruction.label, memory->ReadByte(registers.pc)); break;
			case 2: printf(instruction.label, memory->ReadShort(registers.pc)); break;
		}

		printf("\n");
		printf("af: %04X\n", registers.af);
		printf("bc: %04X\n", registers.bc);
		printf("de: %04X\n", registers.de);
		printf("hl: %04X\n", registers.hl);
		printf("sp: %04X\n", registers.sp);
		printf("pc: %04X\n", registers.pc);
		printf("ime: %d\n", interruptMaster);
		printf("ie: 0x%02x\n", *interruptsEnabled);
		printf("if: 0x%02x\n", *interruptFlags);
		printf("stat: 0x%02x", memory->ReadByte(0xFF41));
		
		char c = std::cin.get();
		if (c == 'n')
		{
			OPCODE_DEBUG = false;
			std::cin.get();
		}
	}
	
	// Execute instruction
	(this->*(instruction.function))();
		
	if (OPCODE_DEBUG)
	{
		printf("%i CYCLES\n\n", lastInstructionCycles);
	}
	
}

void CPU::UpdateTimer()
{
	const int TAC_ON = 1 << 2;
	const int TAC_SELECT = 0x03;
	
	if (divCycles > 0xFF)
	{
		*div += (uint8_t)1;
		divCycles -= 0xFF;
	}
	
	// If timer is running
	if (*tac & TAC_ON)
	{
		int freq;
		
		switch (*tac & TAC_SELECT)
		{
			case 0: freq = 1024; break;
			case 1: freq = 16; break;
			case 2: freq = 64; break;
			case 3: freq = 256; break;
		}
		
		if (timerCycles > freq)
		{
			if (*tima == 0xFF)
			{
				memory->RequestInterrupt(FLAG_TIMER);
				*tima = *tma;
			}
			else
			{
				*tima += 1;
			}
			
			timerCycles -= freq;
		}
	}
}

void CPU::Setup()
{
	instructions = new Instruction[512]
	{
		// 0x00
		{"NOP", 0, &CPU::Instr0x00},
		{"LD BC, 0x%04X", 2, &CPU::Instr0x01},
		{"LD (BC), A", 0, &CPU::Instr0x02},
		{"INC BC", 0, &CPU::Instr0x03},
		{"INC B", 0, &CPU::Instr0x04},
		{"DEC B", 0, &CPU::Instr0x05},
		{"LD B, 0x%02X", 1, &CPU::Instr0x06},
		{"RLCA", 0, &CPU::Instr0x07},
		{"LD (0x%04X), SP", 2, &CPU::Instr0x08},
		{"ADD HL, BC", 0, &CPU::Instr0x09},
		{"LD A, (BC)", 0, &CPU::Instr0x0A},
		{"DEC BC", 0, &CPU::Instr0x0B},
		{"INC C", 0, &CPU::Instr0x0C},
		{"DEC C", 0, &CPU::Instr0x0D},
		{"LD C, 0x%02X", 1, &CPU::Instr0x0E},
		{"RRCA", 0, &CPU::Instr0x0F},
		
		// 0x10
		{"STOP", 0, &CPU::Instr0x00},
		{"LD DE, 0x%04X", 2, &CPU::Instr0x11},
		{"LD (DE), A", 0, &CPU::Instr0x12},
		{"INC DE", 0, &CPU::Instr0x13},
		{"INC D", 0, &CPU::Instr0x14},
		{"DEC D", 0, &CPU::Instr0x15},
		{"LD D, 0x%02X", 1, &CPU::Instr0x16},
		{"RLA", 0, &CPU::Instr0x17},
		{"JR %u", 1, &CPU::Instr0x18},
		{"ADD HL, DE", 0, &CPU::Instr0x19},
		{"LD A, (DE)", 0, &CPU::Instr0x1A},
		{"DEC DE", 0, &CPU::Instr0x1B},
		{"INC E", 0, &CPU::Instr0x1C},
		{"DEC E", 0, &CPU::Instr0x1D},
		{"LD E, 0x%02X", 1, &CPU::Instr0x1E},
		{"RRA", 0, &CPU::Instr0x1F},
		
		// 0x20
		{"JR NZ, %u", 1, &CPU::Instr0x20},
		{"LD HL, 0x%04X", 2, &CPU::Instr0x21},
		{"LD (HL+), A", 0, &CPU::Instr0x22},
		{"INC HL", 0, &CPU::Instr0x23},
		{"INC H", 0, &CPU::Instr0x24},
		{"DEC H", 0, &CPU::Instr0x25},
		{"LD H, 0x%02X", 1, &CPU::Instr0x26},
		{"DAA", 0, &CPU::Instr0x27},
		{"JR Z, %u", 1, &CPU::Instr0x28},
		{"ADD HL, HL", 0, &CPU::Instr0x29},
		{"LD A, (HL+)", 0, &CPU::Instr0x2A},
		{"DEC HL", 0, &CPU::Instr0x2B},
		{"INC L", 0, &CPU::Instr0x2C},
		{"DEC L", 0, &CPU::Instr0x2D},
		{"LD L, 0x%02X", 1, &CPU::Instr0x2E},
		{"CPL", 0, &CPU::Instr0x2F},
		
		// 0x30
		{"JR NC, %u", 1, &CPU::Instr0x30},
		{"LD SP, 0x%04X", 2, &CPU::Instr0x31},
		{"LD (HL-), A", 0, &CPU::Instr0x32},
		{"INC SP", 0, &CPU::Instr0x33},
		{"INC (HL)", 0, &CPU::Instr0x34},
		{"DEC (HL)", 0, &CPU::Instr0x35},
		{"LD (HL), 0x%02X", 1, &CPU::Instr0x36},
		{"SCF", 0, &CPU::Instr0x37},
		{"JR C, %u", 1, &CPU::Instr0x38},
		{"ADD HL, SP", 0, &CPU::Instr0x39},
		{"LD A, (HL-)", 0, &CPU::Instr0x3A},
		{"DEC SP", 0, &CPU::Instr0x3B},
		{"INC A", 0, &CPU::Instr0x3C},
		{"DEC A", 0, &CPU::Instr0x3D},
		{"LD A, 0x%02X", 1, &CPU::Instr0x3E},
		{"CCF", 0, &CPU::Instr0x3F},
		
		// 0x40
		{"LD B, B", 0, &CPU::Instr0x40},
		{"LD B, C", 0, &CPU::Instr0x41},
		{"LD B, D", 0, &CPU::Instr0x42},
		{"LD B, E", 0, &CPU::Instr0x43},
		{"LD B, H", 0, &CPU::Instr0x44},
		{"LD B, L", 0, &CPU::Instr0x45},		
		{"LD B, (HL)", 0, &CPU::Instr0x46},
		{"LD B, A", 0, &CPU::Instr0x47},
		{"LD C, B", 0, &CPU::Instr0x48},
		{"LD C, C", 0, &CPU::Instr0x49},
		{"LD C, D", 0, &CPU::Instr0x4A},
		{"LD C, E", 0, &CPU::Instr0x4B},
		{"LD C, H", 0, &CPU::Instr0x4C},
		{"LD C, L", 0, &CPU::Instr0x4D},
		{"LD C, (HL)", 0, &CPU::Instr0x4E},
		{"LD C, A", 0, &CPU::Instr0x4F},
		
		// 0x50
		{"LD D, B", 0, &CPU::Instr0x50},
		{"LD D, C", 0, &CPU::Instr0x51},
		{"LD D, D", 0, &CPU::Instr0x52},
		{"LD D, E", 0, &CPU::Instr0x53},
		{"LD D, H", 0, &CPU::Instr0x54},
		{"LD D, L", 0, &CPU::Instr0x55},		
		{"LD D, (HL)", 0, &CPU::Instr0x56},
		{"LD D, A", 0, &CPU::Instr0x57},
		{"LD E, B", 0, &CPU::Instr0x58},
		{"LD E, C", 0, &CPU::Instr0x59},
		{"LD E, D", 0, &CPU::Instr0x5A},
		{"LD E, E", 0, &CPU::Instr0x5B},
		{"LD E, H", 0, &CPU::Instr0x5C},
		{"LD E, L", 0, &CPU::Instr0x5D},
		{"LD E, (HL)", 0, &CPU::Instr0x5E},
		{"LD E, A", 0, &CPU::Instr0x5F},
		
		// 0x60
		{"LD H, B", 0, &CPU::Instr0x60},
		{"LD H, C", 0, &CPU::Instr0x61},
		{"LD H, D", 0, &CPU::Instr0x62},
		{"LD H, E", 0, &CPU::Instr0x63},
		{"LD H, H", 0, &CPU::Instr0x64},
		{"LD H, L", 0, &CPU::Instr0x65},		
		{"LD H, (HL)", 0, &CPU::Instr0x66},
		{"LD H, A", 0, &CPU::Instr0x67},
		{"LD L, B", 0, &CPU::Instr0x68},
		{"LD L, C", 0, &CPU::Instr0x69},
		{"LD L, D", 0, &CPU::Instr0x6A},
		{"LD L, E", 0, &CPU::Instr0x6B},
		{"LD L, H", 0, &CPU::Instr0x6C},
		{"LD L, L", 0, &CPU::Instr0x6D},
		{"LD L, (HL)", 0, &CPU::Instr0x6E},
		{"LD L, A", 0, &CPU::Instr0x6F},
		
		// 0x70
		{"LD (HL), B", 0, &CPU::Instr0x70},
		{"LD (HL), C", 0, &CPU::Instr0x71},
		{"LD (HL), D", 0, &CPU::Instr0x72},
		{"LD (HL), E", 0, &CPU::Instr0x73},
		{"LD (HL), H", 0, &CPU::Instr0x74},
		{"LD (HL), L", 0, &CPU::Instr0x75},
		{"HALT", 0, &CPU::Instr0x76},
		{"LD (HL), A", 0, &CPU::Instr0x77},
		{"LD A, B", 0, &CPU::Instr0x78},
		{"LD A, C", 0, &CPU::Instr0x79},
		{"LD A, D", 0, &CPU::Instr0x7A},
		{"LD A, E", 0, &CPU::Instr0x7B},
		{"LD A, H", 0, &CPU::Instr0x7C},
		{"LD A, L", 0, &CPU::Instr0x7D},
		{"LD A, (HL)", 0, &CPU::Instr0x7E},
		{"LD A, A", 0, &CPU::Instr0x7F},
		
		// 0x80
		{"ADD A, B", 0, &CPU::Instr0x80},
		{"ADD A, C", 0, &CPU::Instr0x81},
		{"ADD A, D", 0, &CPU::Instr0x82},
		{"ADD A, E", 0, &CPU::Instr0x83},
		{"ADD A, H", 0, &CPU::Instr0x84},
		{"ADD A, L", 0, &CPU::Instr0x85},
		{"ADD A, (0x%04X)", 2, &CPU::Instr0x86},
		{"ADD A, A", 0, &CPU::Instr0x87},
		{"ADC A, B", 0, &CPU::Instr0x88},
		{"ADC A, C", 0, &CPU::Instr0x89},
		{"ADC A, D", 0, &CPU::Instr0x8A},
		{"ADC A, E", 0, &CPU::Instr0x8B},
		{"ADC A, H", 0, &CPU::Instr0x8C},
		{"ADC A, L", 0, &CPU::Instr0x8D},
		{"ADC A, (0x%04X)", 2, &CPU::Instr0x8E},
		{"ADC A, A", 0, &CPU::Instr0x8F},
		
		// 0x90
		{"SUB A, B", 0, &CPU::Instr0x90},
		{"SUB A, C", 0, &CPU::Instr0x91},
		{"SUB A, D", 0, &CPU::Instr0x92},
		{"SUB A, E", 0, &CPU::Instr0x93},
		{"SUB A, H", 0, &CPU::Instr0x94},
		{"SUB A, L", 0, &CPU::Instr0x95},
		{"SUB A, (0x%04X)", 2, &CPU::Instr0x96},
		{"SUB A, A", 0, &CPU::Instr0x97},
		{"SBC A, B", 0, &CPU::Instr0x98},
		{"SBC A, C", 0, &CPU::Instr0x99},
		{"SBC A, D", 0, &CPU::Instr0x9A},
		{"SBC A, E", 0, &CPU::Instr0x9B},
		{"SBC A, H", 0, &CPU::Instr0x9C},
		{"SBC A, L", 0, &CPU::Instr0x9D},
		{"SBC A, (%04X)", 2, &CPU::Instr0x9E},
		{"SBC A, A", 0, &CPU::Instr0x9F},
		
		// 0xA0
		{"AND A, B", 0, &CPU::Instr0xA0},
		{"AND A, C", 0, &CPU::Instr0xA1},
		{"AND A, D", 0, &CPU::Instr0xA2},
		{"AND A, E", 0, &CPU::Instr0xA3},
		{"AND A, H", 0, &CPU::Instr0xA4},
		{"AND A, L", 0, &CPU::Instr0xA5},
		{"AND A, (0x%04X)", 2, &CPU::Instr0xA6},
		{"AND A, A", 0, &CPU::Instr0xA7},
		{"XOR A, B", 0, &CPU::Instr0xA8},
		{"XOR A, C", 0, &CPU::Instr0xA9},
		{"XOR A, D", 0, &CPU::Instr0xAA},
		{"XOR A, E", 0, &CPU::Instr0xAB},
		{"XOR A, H", 0, &CPU::Instr0xAC},
		{"XOR A, L", 0, &CPU::Instr0xAD},
		{"XOR A, (0x%04X)", 2, &CPU::Instr0xAE},
		{"XOR A, A", 0, &CPU::Instr0xAF},
		
		// 0xB0
		{"OR A, B", 0, &CPU::Instr0xB0},
		{"OR A, C", 0, &CPU::Instr0xB1},
		{"OR A, D", 0, &CPU::Instr0xB2},
		{"OR A, E", 0, &CPU::Instr0xB3},
		{"OR A, H", 0, &CPU::Instr0xB4},
		{"OR A, L", 0, &CPU::Instr0xB5},
		{"OR A, (0x%04X)", 2, &CPU::Instr0xB6},
		{"OR A, A", 0, &CPU::Instr0xB7},
		{"CP A, B", 0, &CPU::Instr0xB8},
		{"CP A, C", 0, &CPU::Instr0xB9},
		{"CP A, D", 0, &CPU::Instr0xBA},
		{"CP A, E", 0, &CPU::Instr0xBB},
		{"CP A, H", 0, &CPU::Instr0xBC},
		{"CP A, L", 0, &CPU::Instr0xBD},
		{"CP A, (0x%04X)", 2, &CPU::Instr0xBE},
		{"CP A, A", 0, &CPU::Instr0xBF},
		
		// 0xC0
		{"RET NZ", 0, &CPU::Instr0xC0},
		{"POP BC", 0, &CPU::Instr0xC1},
		{"JP NZ, 0x%04X", 2, &CPU::Instr0xC2},
		{"JP 0x%04X", 2, &CPU::Instr0xC3},
		{"CALL NZ, 0x%04X", 2, &CPU::Instr0xC4},
		{"PUSH BC", 0, &CPU::Instr0xC5},
		{"ADD A, 0x%02X", 1, &CPU::Instr0xC6},
		{"RST 0x00", 0, &CPU::Instr0xC7},
		{"RET Z", 0, &CPU::Instr0xC8},
		{"RET", 0, &CPU::Instr0xC9},
		{"JP Z, 0x%04X", 2, &CPU::Instr0xCA},
		{"0xCB PREFIX", 0, NULL},
		{"CALL Z, 0x%04X", 2, &CPU::Instr0xCC},
		{"CALL 0x%04X", 2, &CPU::Instr0xCD},
		{"ADC A, 0x%02X", 1, &CPU::Instr0xCE},
		{"RST 0x08", 0, &CPU::Instr0xCF},
		
		// 0xD0
		{"RET NC", 0, &CPU::Instr0xD0},
		{"POP DE", 0, &CPU::Instr0xD1},
		{"JP NC, 0x%04X", 2, &CPU::Instr0xD2},
		{"", 0, NULL},
		{"CALL NC, 0x%04X", 2, &CPU::Instr0xD4},
		{"PUSH DE", 0, &CPU::Instr0xD5},
		{"SUB A, 0x%02X", 1, &CPU::Instr0xD6},
		{"RST 0x10", 0, &CPU::Instr0xD7},
		{"RET C", 0, &CPU::Instr0xD8},
		{"RETI", 0, &CPU::Instr0xD9},
		{"JP C, 0x%04X", 2, &CPU::Instr0xDA},
		{"", 0, &CPU::Instr0x00},
		{"CALL C, 0x%04X", 2, &CPU::Instr0xDC},
		{"", 0, &CPU::Instr0x00},
		{"SBC A, 0x%02X", 1, &CPU::Instr0xDE},
		{"RST 0x18", 0, &CPU::Instr0xDF},
		
		// 0xE0
		{"LD (0xFF00 + 0x%02X), A", 1, &CPU::Instr0xE0},
		{"POP HL", 0, &CPU::Instr0xE1},
		{"LD (0xFF00 + C), A", 0, &CPU::Instr0xE2},
		{"", 0, NULL},
		{"", 0, NULL},
		{"PUSH HL", 0, &CPU::Instr0xE5},
		{"AND A, 0x%02X", 1, &CPU::Instr0xE6},
		{"RST 0x20", 0, &CPU::Instr0xE7},
		{"ADD SP, -n", 0, &CPU::Instr0xE8},
		{"JP (HL)", 0, &CPU::Instr0xE9},
		{"LD (0x%04X), A", 2, &CPU::Instr0xEA},
		{"", 0, NULL},
		{"", 0, NULL},
		{"", 0, NULL},
		{"XOR A, 0x%02X", 1, &CPU::Instr0xEE},
		{"RST 0x28", 0, &CPU::Instr0xEF},
		
		// 0xF0
		{"LD A, (0xFF00 + 0x%02X)", 1, &CPU::Instr0xF0},
		{"POP AF", 0, &CPU::Instr0xF1},
		{"LD A, (0xFF00 + C)", 1, &CPU::Instr0xF2},
		{"DI", 0, &CPU::Instr0xF3},
		{"", 0, NULL},
		{"PUSH AF", 0, &CPU::Instr0xF5},
		{"OR A, 0x%02X", 1, &CPU::Instr0xF6},
		{"RST 0x30", 0, &CPU::Instr0xF7},
		{"LD HL, SP + 0x%02X", 1, &CPU::Instr0xF8},
		{"LD SP, HL", 0, &CPU::Instr0xF9},
		{"LD A, (0x%04X)", 2, &CPU::Instr0xFA},
		{"EI", 0, &CPU::Instr0xFB},
		{"", 0, NULL}, 	
		{"", 0, NULL},
		{"CP A, 0x%02X", 1, &CPU::Instr0xFE},
		{"RST 0x38", 0, &CPU::Instr0xFF},
		
		//####CB###############################################
		// 0x00
		{"RLC B", 0, &CPU::Instr0xCB00},
		{"RLC C", 0, &CPU::Instr0xCB01},
		{"RLC D", 0, &CPU::Instr0xCB02},
		{"RLC E", 0, &CPU::Instr0xCB03},
		{"RLC H", 0, &CPU::Instr0xCB04},
		{"RLC L", 0, &CPU::Instr0xCB05},
		{"RLC (HL)", 0, &CPU::Instr0xCB06},
		{"RLC A", 0, &CPU::Instr0xCB07},
		{"RRC B", 0, &CPU::Instr0xCB08},
		{"RRC C", 0, &CPU::Instr0xCB09},
		{"RRC D", 0, &CPU::Instr0xCB0A},
		{"RRC E", 0, &CPU::Instr0xCB0B},
		{"RRC H", 0, &CPU::Instr0xCB0C},
		{"RRC L", 0, &CPU::Instr0xCB0D},
		{"RRC (HL)", 0, &CPU::Instr0xCB0E},
		{"RRC A", 0, &CPU::Instr0xCB0F},
		
		// 0x10
		{"RL B", 0, &CPU::Instr0xCB10},
		{"RL C", 0, &CPU::Instr0xCB11},
		{"RL D", 0, &CPU::Instr0xCB12},
		{"RL E", 0, &CPU::Instr0xCB13},
		{"RL H", 0, &CPU::Instr0xCB14},
		{"RL L", 0, &CPU::Instr0xCB15},
		{"RL (HL)", 0, &CPU::Instr0xCB16},
		{"RL A", 0, &CPU::Instr0xCB17},
		{"RR B", 0, &CPU::Instr0xCB18},
		{"RR C", 0, &CPU::Instr0xCB19},
		{"RR D", 0, &CPU::Instr0xCB1A},
		{"RR E", 0, &CPU::Instr0xCB1B},
		{"RR H", 0, &CPU::Instr0xCB1C},
		{"RR L", 0, &CPU::Instr0xCB1D},
		{"RR (HL)", 0, &CPU::Instr0xCB1E},
		{"RR A", 0, &CPU::Instr0xCB1F},
		
		// 0x20
		{"SLA B", 0, &CPU::Instr0xCB20},
		{"SLA C", 0, &CPU::Instr0xCB21},
		{"SLA D", 0, &CPU::Instr0xCB22},
		{"SLA E", 0, &CPU::Instr0xCB23},
		{"SLA H", 0, &CPU::Instr0xCB24},
		{"SLA L", 0, &CPU::Instr0xCB25},
		{"SLA (HL)", 0, &CPU::Instr0xCB26},
		{"SLA A", 0, &CPU::Instr0xCB27},
		{"SRA B", 0, &CPU::Instr0xCB28},
		{"SRA C", 0, &CPU::Instr0xCB29},
		{"SRA D", 0, &CPU::Instr0xCB2A},
		{"SRA E", 0, &CPU::Instr0xCB2B},
		{"SRA H", 0, &CPU::Instr0xCB2C},
		{"SRA L", 0, &CPU::Instr0xCB2D},
		{"SRA (HL)", 0, &CPU::Instr0xCB2E},
		{"SRA A", 0, &CPU::Instr0xCB2F},
		
		// 0x30
		{"SWAP B", 0, &CPU::Instr0xCB30},
		{"SWAP C", 0, &CPU::Instr0xCB31},
		{"SWAP D", 0, &CPU::Instr0xCB32},
		{"SWAP E", 0, &CPU::Instr0xCB33},
		{"SWAP H", 0, &CPU::Instr0xCB34},
		{"SWAP L", 0, &CPU::Instr0xCB35},
		{"SWAP (HL)", 0, &CPU::Instr0xCB36},
		{"SWAP A", 0, &CPU::Instr0xCB37},
		{"SRL B", 0, &CPU::Instr0xCB38},
		{"SRL C", 0, &CPU::Instr0xCB39},
		{"SRL D", 0, &CPU::Instr0xCB3A},
		{"SRL E", 0, &CPU::Instr0xCB3B},
		{"SRL H", 0, &CPU::Instr0xCB3C},
		{"SRL L", 0, &CPU::Instr0xCB3D},
		{"SRL (HL)", 0, &CPU::Instr0xCB3E},
		{"SRL A", 0, &CPU::Instr0xCB3F},
		
		// 0x40
		{"SET 0,B", 0, &CPU::Instr0xCB40},
		{"SET 0,C", 0, &CPU::Instr0xCB41},
		{"SET 0,D", 0, &CPU::Instr0xCB42},
		{"SET 0,E", 0, &CPU::Instr0xCB43},
		{"SET 0,H", 0, &CPU::Instr0xCB44},
		{"SET 0,L", 0, &CPU::Instr0xCB45},
		{"SET 0,(HL)", 0, &CPU::Instr0xCB46},
		{"SET 0,A", 0, &CPU::Instr0xCB47},
		{"SET 1,B", 0, &CPU::Instr0xCB48},
		{"SET 1,B", 0, &CPU::Instr0xCB49},
		{"SET 1,B", 0, &CPU::Instr0xCB4A},
		{"SET 1,B", 0, &CPU::Instr0xCB4B},
		{"SET 1,B", 0, &CPU::Instr0xCB4C},
		{"SET 1,B", 0, &CPU::Instr0xCB4D},
		{"SET 1,(HL)", 0, &CPU::Instr0xCB4E},
		{"SET 1,B", 0, &CPU::Instr0xCB4F},
		
		// 0x50
		{"SET 2,B", 0, &CPU::Instr0xCB50},
		{"SET 2,C", 0, &CPU::Instr0xCB51},
		{"SET 2,D", 0, &CPU::Instr0xCB52},
		{"SET 2,E", 0, &CPU::Instr0xCB53},
		{"SET 2,H", 0, &CPU::Instr0xCB54},
		{"SET 2,L", 0, &CPU::Instr0xCB55},
		{"SET 2,(HL)", 0, &CPU::Instr0xCB56},
		{"SET 2,A", 0, &CPU::Instr0xCB57},
		{"SET 3,B", 0, &CPU::Instr0xCB58},
		{"SET 3,B", 0, &CPU::Instr0xCB59},
		{"SET 3,B", 0, &CPU::Instr0xCB5A},
		{"SET 3,B", 0, &CPU::Instr0xCB5B},
		{"SET 3,B", 0, &CPU::Instr0xCB5C},
		{"SET 3,B", 0, &CPU::Instr0xCB5D},
		{"SET 3,(HL)", 0, &CPU::Instr0xCB5E},
		{"SET 3,B", 0, &CPU::Instr0xCB5F},
		
		// 0x60
		{"SET 4,B", 0, &CPU::Instr0xCB60},
		{"SET 4,C", 0, &CPU::Instr0xCB61},
		{"SET 4,D", 0, &CPU::Instr0xCB62},
		{"SET 4,E", 0, &CPU::Instr0xCB63},
		{"SET 4,H", 0, &CPU::Instr0xCB64},
		{"SET 4,L", 0, &CPU::Instr0xCB65},
		{"SET 4,(HL)", 0, &CPU::Instr0xCB66},
		{"SET 4,A", 0, &CPU::Instr0xCB67},
		{"SET 5,B", 0, &CPU::Instr0xCB68},
		{"SET 5,B", 0, &CPU::Instr0xCB69},
		{"SET 5,B", 0, &CPU::Instr0xCB6A},
		{"SET 5,B", 0, &CPU::Instr0xCB6B},
		{"SET 5,B", 0, &CPU::Instr0xCB6C},
		{"SET 5,B", 0, &CPU::Instr0xCB6D},
		{"SET 5,(HL)", 0, &CPU::Instr0xCB6E},
		{"SET 5,B", 0, &CPU::Instr0xCB6F},
		
		// 0x70
		{"SET 6,B", 0, &CPU::Instr0xCB70},
		{"SET 6,C", 0, &CPU::Instr0xCB71},
		{"SET 6,D", 0, &CPU::Instr0xCB72},
		{"SET 6,E", 0, &CPU::Instr0xCB73},
		{"SET 6,H", 0, &CPU::Instr0xCB74},
		{"SET 6,L", 0, &CPU::Instr0xCB75},
		{"SET 6,(HL)", 0, &CPU::Instr0xCB76},
		{"SET 6,A", 0, &CPU::Instr0xCB77},
		{"SET 7,B", 0, &CPU::Instr0xCB78},
		{"SET 7,B", 0, &CPU::Instr0xCB79},
		{"SET 7,B", 0, &CPU::Instr0xCB7A},
		{"SET 7,B", 0, &CPU::Instr0xCB7B},
		{"SET 7,B", 0, &CPU::Instr0xCB7C},
		{"SET 7,B", 0, &CPU::Instr0xCB7D},
		{"SET 7,(HL)", 0, &CPU::Instr0xCB7E},
		{"SET 7,B", 0, &CPU::Instr0xCB7F},
		
		// 0x80
		{"RES 0,B", 0, &CPU::Instr0xCB80},
		{"RES 0,C", 0, &CPU::Instr0xCB81},
		{"RES 0,D", 0, &CPU::Instr0xCB82},
		{"RES 0,E", 0, &CPU::Instr0xCB83},
		{"RES 0,H", 0, &CPU::Instr0xCB84},
		{"RES 0,L", 0, &CPU::Instr0xCB85},
		{"RES 0,(HL)", 0, &CPU::Instr0xCB86},
		{"RES 0,A", 0, &CPU::Instr0xCB87},
		{"RES 1,B", 0, &CPU::Instr0xCB88},
		{"RES 1,B", 0, &CPU::Instr0xCB89},
		{"RES 1,B", 0, &CPU::Instr0xCB8A},
		{"RES 1,B", 0, &CPU::Instr0xCB8B},
		{"RES 1,B", 0, &CPU::Instr0xCB8C},
		{"RES 1,B", 0, &CPU::Instr0xCB8D},
		{"RES 1,(HL)", 0, &CPU::Instr0xCB8E},
		{"RES 1,B", 0, &CPU::Instr0xCB8F},
		
		// 0x90
		{"RES 2,B", 0, &CPU::Instr0xCB90},
		{"RES 2,C", 0, &CPU::Instr0xCB91},
		{"RES 2,D", 0, &CPU::Instr0xCB92},
		{"RES 2,E", 0, &CPU::Instr0xCB93},
		{"RES 2,H", 0, &CPU::Instr0xCB94},
		{"RES 2,L", 0, &CPU::Instr0xCB95},
		{"RES 2,(HL)", 0, &CPU::Instr0xCB96},
		{"RES 2,A", 0, &CPU::Instr0xCB97},
		{"RES 3,B", 0, &CPU::Instr0xCB98},
		{"RES 3,B", 0, &CPU::Instr0xCB99},
		{"RES 3,B", 0, &CPU::Instr0xCB9A},
		{"RES 3,B", 0, &CPU::Instr0xCB9B},
		{"RES 3,B", 0, &CPU::Instr0xCB9C},
		{"RES 3,B", 0, &CPU::Instr0xCB9D},
		{"RES 3,(HL)", 0, &CPU::Instr0xCB9E},
		{"RES 3,B", 0, &CPU::Instr0xCB9F},
		
		// 0xA0
		{"RES 4,B", 0, &CPU::Instr0xCBA0},
		{"RES 4,C", 0, &CPU::Instr0xCBA1},
		{"RES 4,D", 0, &CPU::Instr0xCBA2},
		{"RES 4,E", 0, &CPU::Instr0xCBA3},
		{"RES 4,H", 0, &CPU::Instr0xCBA4},
		{"RES 4,L", 0, &CPU::Instr0xCBA5},
		{"RES 4,(HL)", 0, &CPU::Instr0xCBA6},
		{"RES 4,A", 0, &CPU::Instr0xCBA7},
		{"RES 5,B", 0, &CPU::Instr0xCBA8},
		{"RES 5,B", 0, &CPU::Instr0xCBA9},
		{"RES 5,B", 0, &CPU::Instr0xCBAA},
		{"RES 5,B", 0, &CPU::Instr0xCBAB},
		{"RES 5,B", 0, &CPU::Instr0xCBAC},
		{"RES 5,B", 0, &CPU::Instr0xCBAD},
		{"RES 5,(HL)", 0, &CPU::Instr0xCBAE},
		{"RES 5,B", 0, &CPU::Instr0xCBAF},
		
		// 0xB0
		{"RES 6,B", 0, &CPU::Instr0xCBB0},
		{"RES 6,C", 0, &CPU::Instr0xCBB1},
		{"RES 6,D", 0, &CPU::Instr0xCBB2},
		{"RES 6,E", 0, &CPU::Instr0xCBB3},
		{"RES 6,H", 0, &CPU::Instr0xCBB4},
		{"RES 6,L", 0, &CPU::Instr0xCBB5},
		{"RES 6,(HL)", 0, &CPU::Instr0xCBB6},
		{"RES 6,A", 0, &CPU::Instr0xCBB7},
		{"RES 7,B", 0, &CPU::Instr0xCBB8},
		{"RES 7,B", 0, &CPU::Instr0xCBB9},
		{"RES 7,B", 0, &CPU::Instr0xCBBA},
		{"RES 7,B", 0, &CPU::Instr0xCBBB},
		{"RES 7,B", 0, &CPU::Instr0xCBBC},
		{"RES 7,B", 0, &CPU::Instr0xCBBD},
		{"RES 7,(HL)", 0, &CPU::Instr0xCBBE},
		{"RES 7,B", 0, &CPU::Instr0xCBBF},
		
		// 0xC0
		{"SET 0,B", 0, &CPU::Instr0xCBC0},
		{"SET 0,C", 0, &CPU::Instr0xCBC1},
		{"SET 0,D", 0, &CPU::Instr0xCBC2},
		{"SET 0,E", 0, &CPU::Instr0xCBC3},
		{"SET 0,H", 0, &CPU::Instr0xCBC4},
		{"SET 0,L", 0, &CPU::Instr0xCBC5},
		{"SET 0,(HL)", 0, &CPU::Instr0xCBC6},
		{"SET 0,A", 0, &CPU::Instr0xCBC7},
		{"SET 1,B", 0, &CPU::Instr0xCBC8},
		{"SET 1,B", 0, &CPU::Instr0xCBC9},
		{"SET 1,B", 0, &CPU::Instr0xCBCA},
		{"SET 1,B", 0, &CPU::Instr0xCBCB},
		{"SET 1,B", 0, &CPU::Instr0xCBCC},
		{"SET 1,B", 0, &CPU::Instr0xCBCD},
		{"SET 1,(HL)", 0, &CPU::Instr0xCBCE},
		{"SET 1,B", 0, &CPU::Instr0xCBCF},
		
		// 0xD0
		{"SET 2,B", 0, &CPU::Instr0xCBD0},
		{"SET 2,C", 0, &CPU::Instr0xCBD1},
		{"SET 2,D", 0, &CPU::Instr0xCBD2},
		{"SET 2,E", 0, &CPU::Instr0xCBD3},
		{"SET 2,H", 0, &CPU::Instr0xCBD4},
		{"SET 2,L", 0, &CPU::Instr0xCBD5},
		{"SET 2,(HL)", 0, &CPU::Instr0xCBD6},
		{"SET 2,A", 0, &CPU::Instr0xCBD7},
		{"SET 3,B", 0, &CPU::Instr0xCBD8},
		{"SET 3,B", 0, &CPU::Instr0xCBD9},
		{"SET 3,B", 0, &CPU::Instr0xCBDA},
		{"SET 3,B", 0, &CPU::Instr0xCBDB},
		{"SET 3,B", 0, &CPU::Instr0xCBDC},
		{"SET 3,B", 0, &CPU::Instr0xCBDD},
		{"SET 3,(HL)", 0, &CPU::Instr0xCBDE},
		{"SET 3,B", 0, &CPU::Instr0xCBDF},
		
		// 0xE0
		{"SET 4,B", 0, &CPU::Instr0xCBE0},
		{"SET 4,C", 0, &CPU::Instr0xCBE1},
		{"SET 4,D", 0, &CPU::Instr0xCBE2},
		{"SET 4,E", 0, &CPU::Instr0xCBE3},
		{"SET 4,H", 0, &CPU::Instr0xCBE4},
		{"SET 4,L", 0, &CPU::Instr0xCBE5},
		{"SET 4,(HL)", 0, &CPU::Instr0xCBE6},
		{"SET 4,A", 0, &CPU::Instr0xCBE7},
		{"SET 5,B", 0, &CPU::Instr0xCBE8},
		{"SET 5,B", 0, &CPU::Instr0xCBE9},
		{"SET 5,B", 0, &CPU::Instr0xCBEA},
		{"SET 5,B", 0, &CPU::Instr0xCBEB},
		{"SET 5,B", 0, &CPU::Instr0xCBEC},
		{"SET 5,B", 0, &CPU::Instr0xCBED},
		{"SET 5,(HL)", 0, &CPU::Instr0xCBEE},
		{"SET 5,B", 0, &CPU::Instr0xCBEF},
		
		// 0xF0
		{"SET 6,B", 0, &CPU::Instr0xCBF0},
		{"SET 6,C", 0, &CPU::Instr0xCBF1},
		{"SET 6,D", 0, &CPU::Instr0xCBF2},
		{"SET 6,E", 0, &CPU::Instr0xCBF3},
		{"SET 6,H", 0, &CPU::Instr0xCBF4},
		{"SET 6,L", 0, &CPU::Instr0xCBF5},
		{"SET 6,(HL)", 0, &CPU::Instr0xCBF6},
		{"SET 6,A", 0, &CPU::Instr0xCBF7},
		{"SET 7,B", 0, &CPU::Instr0xCBF8},
		{"SET 7,B", 0, &CPU::Instr0xCBF9},
		{"SET 7,B", 0, &CPU::Instr0xCBFA},
		{"SET 7,B", 0, &CPU::Instr0xCBFB},
		{"SET 7,B", 0, &CPU::Instr0xCBFC},
		{"SET 7,B", 0, &CPU::Instr0xCBFD},
		{"SET 7,(HL)", 0, &CPU::Instr0xCBFE},
		{"SET 7,B", 0, &CPU::Instr0xCBFF},
		
	};
	
	instructionCycles = new int[512]
	{
		 4, 12,  8,  8,  4,  4,  8,  8, 20,  8,  8,  8,  4,  4,  8,  8,
		 4, 12,  8,  8,  4,  4,  8,  8,  8,  8,  8,  8,  4,  4,  8,  8,
		 8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,
		 8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
		 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, 
		 8, 12, 12, 12, 12, 16,  8, 16,  8,  4,  8,  4, 12, 12,  8, 16,
		 8, 12, 12, 00, 12, 16,  8, 16,  8, 16,  8, 00, 12, 00,  8, 16,
		12, 12,  8, 00, 00, 16,  8, 16, 16, -4, 16, 00, 00, 00,  8, 16,
		12, 12,  8,  4, 00, 16,  8, 16, 12,  8, 16,  4, 00, 00,  8, 16,
		// CB
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
		8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8  
	};
}

// FLAGS
void CPU::SetFlag(int flag)
{
	registers.f |= flag;
}

bool CPU::GetFlag(int flag)
{
	return (registers.f & (flag));
}

void CPU::ClearFlag(int flag)
{
	registers.f &= ~(flag);
}

void CPU::ClearFlags()
{
	registers.f = 0;
}

// READ IMMEDIATE
uint8_t CPU::ReadNextByte()
{
	return memory->ReadByte(registers.pc++);
}

uint16_t CPU::ReadNextShort()
{
	uint16_t data = memory->ReadShort(registers.pc);	
	registers.pc += 2;
	return data;
}

// NOP
void CPU::Instr0x00() 
{ 
	// Do nothing 
}

// HALT
void CPU::Instr0x76()
{
	// Stop CPU until interrupt occurs
	isHalted = true;
	interruptMaster = true;
}

// STOP
void CPU::Instr0x10()
{
	// Stop CPU until button is pressed
	isStopped = false;
}

// INTERRUPTS
void CPU::Instr0xF3()
{
	// Disable interuppts after the NEXT instruction
	imeState = IME_WAIT_OFF;
}

void CPU::Instr0xFB()
{
	// Enable interuppts after the NEXT instruction
	imeState = IME_WAIT_ON;
}

// ADDITION
void CPU::Add8(uint8_t data)
{
	//
	ClearFlags();
	
	// Check for 4 bit overflow
	if (((registers.a & 0xF) + (data & 0xF)) > 0xF)
		SetFlag(FLAG_HALF_CARRY);
	
	// Check for overflow
	if ((registers.a + data) > 0xFF)
		SetFlag(FLAG_CARRY);
	
	// Add to accumulator
	registers.a += data;
	
	// Check if zero
	if (!registers.a)
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0x80() { Add8(registers.b); }
void CPU::Instr0x81() { Add8(registers.c); }
void CPU::Instr0x82() { Add8(registers.d); }
void CPU::Instr0x83() { Add8(registers.e); }
void CPU::Instr0x84() { Add8(registers.h); }
void CPU::Instr0x85() { Add8(registers.l); }
void CPU::Instr0x86() { Add8(memory->ReadByte(registers.hl)); }
void CPU::Instr0x87() { Add8(registers.a); }
void CPU::Instr0xC6() { Add8(ReadNextByte()); }

void CPU::Add16(uint16_t data)
{
	ClearFlag(FLAG_NEG);
	
	if ((registers.hl + data) > 0xFFFF)
		SetFlag(FLAG_CARRY);
	else
		ClearFlag(FLAG_CARRY);
	
	if ((registers.hl & 0x7FF) + (data & 0x7FF) > 0x7FF)
		SetFlag(FLAG_HALF_CARRY);
	else
		ClearFlag(FLAG_HALF_CARRY);
	
	registers.hl += data;
}

void CPU::Instr0x09() { Add16(registers.bc); }
void CPU::Instr0x19() { Add16(registers.de); }
void CPU::Instr0x29() { Add16(registers.hl); }
void CPU::Instr0x39() { Add16(registers.sp); }

void CPU::Instr0xE8()
{
	int8_t num = (int8_t) ReadNextByte();
	int result = registers.sp + num;
	
	ClearFlags();
	
	if ((registers.sp ^ num ^ (result & 0xFFFF)) & 0x100)
		SetFlag(FLAG_CARRY);
	
	if ((registers.sp ^ num ^ (result & 0xFFFF)) & 0x10)
		SetFlag(FLAG_HALF_CARRY);
	
	registers.sp = result;
}

// ADC
void CPU::Adc8(uint8_t data)
{
	//Add8(data + GetFlag(FLAG_CARRY)? 1 : 0);
	int carry = (GetFlag(FLAG_CARRY)? 1 : 0);
	
	ClearFlag(FLAG_NEG);
	
	if (((registers.a & 0xF) + (data & 0xF) + carry) > 0xF)
		SetFlag(FLAG_HALF_CARRY);
	else
		ClearFlag(FLAG_HALF_CARRY);
	
	if (registers.a + data + carry > 0xFF)
		SetFlag(FLAG_CARRY);
	else
		ClearFlag(FLAG_CARRY);
	
	registers.a += data + (uint8_t) carry;
	
	if (registers.a)
		ClearFlag(FLAG_ZERO);
	else
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0x8F() { Adc8(registers.a); }
void CPU::Instr0x88() { Adc8(registers.b); }
void CPU::Instr0x89() { Adc8(registers.c); }
void CPU::Instr0x8A() { Adc8(registers.d); }
void CPU::Instr0x8B() { Adc8(registers.e); }
void CPU::Instr0x8C() { Adc8(registers.h); }
void CPU::Instr0x8D() { Adc8(registers.l); }
void CPU::Instr0x8E() { Adc8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xCE() { Adc8(ReadNextByte()); }

// SUBTRACTION
void CPU::Sub8(uint8_t data)
{
	ClearFlags();
	SetFlag(FLAG_NEG);
	
	if (registers.a < data)
		SetFlag(FLAG_CARRY);
	
	if ((registers.a & 0xF) < (data & 0xF))
		SetFlag(FLAG_HALF_CARRY);
	
	registers.a -= data;
	
	if (!registers.a)
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0x90() { Sub8(registers.b); }
void CPU::Instr0x91() { Sub8(registers.c); }
void CPU::Instr0x92() { Sub8(registers.d); }
void CPU::Instr0x93() { Sub8(registers.e); }
void CPU::Instr0x94() { Sub8(registers.h); }
void CPU::Instr0x95() { Sub8(registers.l); }
void CPU::Instr0x96() { Sub8(memory->ReadByte(registers.hl)); }
void CPU::Instr0x97() { Sub8(registers.a); }
void CPU::Instr0xD6() { Sub8(ReadNextByte()); }

// SBC
void CPU::Sbc8(uint8_t data)
{
	//Sub8(data + GetFlag(FLAG_CARRY)? 1 : 0);
	int carry = (GetFlag(FLAG_CARRY)? 1 : 0);
	
	SetFlag(FLAG_NEG);
	
	if (((registers.a & 0xF) - (data & 0xF) - carry) < 0x0)
		SetFlag(FLAG_HALF_CARRY);
	else
		ClearFlag(FLAG_HALF_CARRY);
	
	if (registers.a - data - carry < 0x0)
		SetFlag(FLAG_CARRY);
	else
		ClearFlag(FLAG_CARRY);
	
	registers.a -= data + (uint8_t) carry;
	
	if (registers.a)
		ClearFlag(FLAG_ZERO);
	else
		SetFlag(FLAG_ZERO);
}
void CPU::Instr0x9F() { Sbc8(registers.a); }
void CPU::Instr0x98() { Sbc8(registers.b); }
void CPU::Instr0x99() { Sbc8(registers.c); }
void CPU::Instr0x9A() { Sbc8(registers.d); }
void CPU::Instr0x9B() { Sbc8(registers.e); }
void CPU::Instr0x9C() { Sbc8(registers.h); }
void CPU::Instr0x9D() { Sbc8(registers.l); }
void CPU::Instr0x9E() { Sbc8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xDE() { Sbc8(ReadNextByte()); }

// AND
void CPU::And8(uint8_t data)
{
	ClearFlags();
	SetFlag(FLAG_HALF_CARRY);
	
	registers.a &= data;
	
	if (!registers.a)
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xA0() { And8(registers.b); }
void CPU::Instr0xA1() { And8(registers.c); }
void CPU::Instr0xA2() { And8(registers.d); }
void CPU::Instr0xA3() { And8(registers.e); }
void CPU::Instr0xA4() { And8(registers.h); }
void CPU::Instr0xA5() { And8(registers.l); }
void CPU::Instr0xA6() { And8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xA7() { And8(registers.a); }
void CPU::Instr0xE6() { And8(ReadNextByte()); }

// OR
void CPU::Or8(uint8_t data)
{
	ClearFlags();
	
	registers.a |= data;
	
	if (!registers.a)
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xB0() { Or8(registers.b); }
void CPU::Instr0xB1() { Or8(registers.c); }
void CPU::Instr0xB2() { Or8(registers.d); }
void CPU::Instr0xB3() { Or8(registers.e); }
void CPU::Instr0xB4() { Or8(registers.h); }
void CPU::Instr0xB5() { Or8(registers.l); }
void CPU::Instr0xB6() { Or8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xB7() { Or8(registers.a); }
void CPU::Instr0xF6() { Or8(ReadNextByte()); }

// XOR
void CPU::Xor8(uint8_t data)
{
	ClearFlags();
	
	registers.a ^= data;
	
	if (!registers.a)
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xAF() { Xor8(registers.a); }
void CPU::Instr0xA8() { Xor8(registers.b); }
void CPU::Instr0xA9() { Xor8(registers.c); }
void CPU::Instr0xAA() { Xor8(registers.d); }
void CPU::Instr0xAB() { Xor8(registers.e); }
void CPU::Instr0xAC() { Xor8(registers.h); }
void CPU::Instr0xAD() { Xor8(registers.l); }
void CPU::Instr0xAE() { Xor8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xEE() { Xor8(ReadNextByte()); }

// COMPARE
void CPU::Cp8(uint8_t data)
{
	ClearFlags();
	SetFlag(FLAG_NEG);
	
	if (registers.a == data)
		SetFlag(FLAG_ZERO);
	
	// Check for 4 bit overflow
	if ((registers.a & 0xF) < (data & 0xF))
		SetFlag(FLAG_HALF_CARRY);
	
	// Check for overflow
	if (registers.a < data)
		SetFlag(FLAG_CARRY);
}

void CPU::Instr0xBF() { Cp8(registers.a); }
void CPU::Instr0xB8() { Cp8(registers.b); }
void CPU::Instr0xB9() { Cp8(registers.c); }
void CPU::Instr0xBA() { Cp8(registers.d); }
void CPU::Instr0xBB() { Cp8(registers.e); }
void CPU::Instr0xBC() { Cp8(registers.h); }
void CPU::Instr0xBD() { Cp8(registers.l); }
void CPU::Instr0xBE() { Cp8(memory->ReadByte(registers.hl)); }
void CPU::Instr0xFE() { Cp8(ReadNextByte()); }

// INCREMENT
void CPU::Inc8(uint8_t* num)
{	
	// If first four bits will overflow
	if ((*num & 0x0F) == 0x0F)
		SetFlag(FLAG_HALF_CARRY);
	else
		ClearFlag(FLAG_HALF_CARRY);
	
	(*num) += (uint8_t)1;
	
	ClearFlag(FLAG_NEG);
	
	// If zero, set zero flag
	if (!*num) 
		SetFlag(FLAG_ZERO);
	else
		ClearFlag(FLAG_ZERO);
}

void CPU::Instr0x3C() { Inc8(&registers.a); }
void CPU::Instr0x04() { Inc8(&registers.b); }
void CPU::Instr0x0C() { Inc8(&registers.c); }
void CPU::Instr0x14() { Inc8(&registers.d); }
void CPU::Instr0x1C() { Inc8(&registers.e); }
void CPU::Instr0x24() { Inc8(&registers.h); }
void CPU::Instr0x2C() { Inc8(&registers.l); }
void CPU::Instr0x34() 
{ 
	uint8_t data = memory->ReadByte(registers.hl);
	Inc8(&data);
	memory->SetByte(registers.hl, data);
}

// INCREMENT 16
void CPU::Inc16(uint16_t* num)
{
	(*num) += (uint16_t)1;
}

void CPU::Instr0x03() { Inc16(&registers.bc); }
void CPU::Instr0x13() { Inc16(&registers.de); }
void CPU::Instr0x23() { Inc16(&registers.hl); }
void CPU::Instr0x33() { Inc16(&registers.sp); }

// DECREMENT
void CPU::Dec8(uint8_t* num)
{
	// If first four bits will overflow
	if ((*num) & 0x0F)
		ClearFlag(FLAG_HALF_CARRY);
	else
		SetFlag(FLAG_HALF_CARRY);
	
	(*num) -= (uint8_t)1;
	
	SetFlag(FLAG_NEG);
	
	// If zero
	if (*num)
		ClearFlag(FLAG_ZERO);
	else
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0x3D() { Dec8(&registers.a); }
void CPU::Instr0x05() { Dec8(&registers.b); }
void CPU::Instr0x0D() { Dec8(&registers.c); }
void CPU::Instr0x15() { Dec8(&registers.d); }
void CPU::Instr0x1D() { Dec8(&registers.e); }
void CPU::Instr0x25() { Dec8(&registers.h); }
void CPU::Instr0x2D() { Dec8(&registers.l); }
void CPU::Instr0x35() 
{ 
	uint8_t data = memory->ReadByte(registers.hl);
	Dec8(&data);
	memory->SetByte(registers.hl, data);
}

// DECREMENT 16
void CPU::Dec16(uint16_t* num)
{
	(*num) -= (uint16_t)1;
}

void CPU::Instr0x0B() { Dec16(&registers.bc); }
void CPU::Instr0x1B() { Dec16(&registers.de); }
void CPU::Instr0x2B() { Dec16(&registers.hl); }
void CPU::Instr0x3B() { Dec16(&registers.sp); }

// COMPLEMENTS AND ETC
void CPU::Instr0x2F()
{
	// CPL
	SetFlag(FLAG_NEG);
	SetFlag(FLAG_HALF_CARRY);
	
	registers.a = ~registers.a;
}

void CPU::Instr0x3F()
{
	// CCF
	ClearFlag(FLAG_NEG);
	ClearFlag(FLAG_HALF_CARRY);
	
	if (GetFlag(FLAG_CARRY))
		ClearFlag(FLAG_CARRY);
	else
		SetFlag(FLAG_CARRY);
}

void CPU::Instr0x37()
{
	// SCF
	ClearFlag(FLAG_NEG);
	ClearFlag(FLAG_HALF_CARRY);
	
	SetFlag(FLAG_CARRY);
}

// STACK
// Push
void CPU::Push16(uint16_t data)
{
	registers.sp -= 2;
	memory->SetShort(registers.sp, data);
}

void CPU::Instr0xF5() { Push16(registers.af); }
void CPU::Instr0xC5() { Push16(registers.bc); }
void CPU::Instr0xD5() { Push16(registers.de); }
void CPU::Instr0xE5() { Push16(registers.hl); }

// Pop
uint16_t CPU::Pop16()
{
	uint16_t data = memory->ReadShort(registers.sp);
	registers.sp += 2;
	return data;
}

void CPU::Instr0xF1() { registers.af = Pop16() & 0xFFF0; }
void CPU::Instr0xC1() { registers.bc = Pop16(); }
void CPU::Instr0xD1() { registers.de = Pop16(); }
void CPU::Instr0xE1() { registers.hl = Pop16(); }

// LOADS
// 8 BIT IMMEDIATE
void CPU::Instr0x06() { registers.b = ReadNextByte(); }
void CPU::Instr0x0E() { registers.c = ReadNextByte(); }
void CPU::Instr0x16() { registers.d = ReadNextByte(); }
void CPU::Instr0x1E() { registers.e = ReadNextByte(); }
void CPU::Instr0x26() { registers.h = ReadNextByte(); }
void CPU::Instr0x2E() { registers.l = ReadNextByte(); }
void CPU::Instr0x3E() { registers.a = ReadNextByte(); }

// 8 BIT REG TO REG
// DESTINATION A
void CPU::Instr0x78() { registers.a = registers.b; }
void CPU::Instr0x79() { registers.a = registers.c; }
void CPU::Instr0x7A() { registers.a = registers.d; }
void CPU::Instr0x7B() { registers.a = registers.e; }
void CPU::Instr0x7C() { registers.a = registers.h; }
void CPU::Instr0x7D() { registers.a = registers.l; }
void CPU::Instr0x7F() { registers.a = registers.a; }
// DESTINATION B
void CPU::Instr0x40() { registers.b = registers.b; }
void CPU::Instr0x41() { registers.b = registers.c; }
void CPU::Instr0x42() { registers.b = registers.d; }
void CPU::Instr0x43() { registers.b = registers.e; }
void CPU::Instr0x44() { registers.b = registers.h; }
void CPU::Instr0x45() { registers.b = registers.l; }
void CPU::Instr0x47() { registers.b = registers.a; }
// DESTINATION C
void CPU::Instr0x48() { registers.c = registers.b; }
void CPU::Instr0x49() { registers.c = registers.c; }
void CPU::Instr0x4A() { registers.c = registers.d; }
void CPU::Instr0x4B() { registers.c = registers.e; }
void CPU::Instr0x4C() { registers.c = registers.h; }
void CPU::Instr0x4D() { registers.c = registers.l; }
void CPU::Instr0x4F() { registers.c = registers.a; }
// DESTINATION D
void CPU::Instr0x50() { registers.d = registers.b; }
void CPU::Instr0x51() { registers.d = registers.c; }
void CPU::Instr0x52() { registers.d = registers.d; }
void CPU::Instr0x53() { registers.d = registers.e; }
void CPU::Instr0x54() { registers.d = registers.h; }
void CPU::Instr0x55() { registers.d = registers.l; }
void CPU::Instr0x57() { registers.d = registers.a; }
// DESTINATION E
void CPU::Instr0x58() { registers.e = registers.b; }
void CPU::Instr0x59() { registers.e = registers.c; }
void CPU::Instr0x5A() { registers.e = registers.d; }
void CPU::Instr0x5B() { registers.e = registers.e; }
void CPU::Instr0x5C() { registers.e = registers.h; }
void CPU::Instr0x5D() { registers.e = registers.l; }
void CPU::Instr0x5F() { registers.e = registers.a; }
// DESTINATION H
void CPU::Instr0x60() { registers.h = registers.b; }
void CPU::Instr0x61() { registers.h = registers.c; }
void CPU::Instr0x62() { registers.h = registers.d; }
void CPU::Instr0x63() { registers.h = registers.e; }
void CPU::Instr0x64() { registers.h = registers.h; }
void CPU::Instr0x65() { registers.h = registers.l; }
void CPU::Instr0x67() { registers.h = registers.a; }
// DESTINATION L
void CPU::Instr0x68() { registers.l = registers.b; }
void CPU::Instr0x69() { registers.l = registers.c; }
void CPU::Instr0x6A() { registers.l = registers.d; }
void CPU::Instr0x6B() { registers.l = registers.e; }
void CPU::Instr0x6C() { registers.l = registers.h; }
void CPU::Instr0x6D() { registers.l = registers.l; }
void CPU::Instr0x6F() { registers.l = registers.a; }

// 8 BIT FROM MEM
void CPU::Instr0x46() { registers.b = memory->ReadByte(registers.hl); }
void CPU::Instr0x4E() { registers.c = memory->ReadByte(registers.hl); }
void CPU::Instr0x56() { registers.d = memory->ReadByte(registers.hl); }
void CPU::Instr0x5E() { registers.e = memory->ReadByte(registers.hl); }
void CPU::Instr0x66() { registers.h = memory->ReadByte(registers.hl); }
void CPU::Instr0x6E() { registers.l = memory->ReadByte(registers.hl); }
void CPU::Instr0x7E() { registers.a = memory->ReadByte(registers.hl); }

// 8 BIT TO MEM
void CPU::Instr0x70() { memory->SetByte(registers.hl, registers.b); }
void CPU::Instr0x71() { memory->SetByte(registers.hl, registers.c); }
void CPU::Instr0x72() { memory->SetByte(registers.hl, registers.d); }
void CPU::Instr0x73() { memory->SetByte(registers.hl, registers.e); }
void CPU::Instr0x74() { memory->SetByte(registers.hl, registers.h); }
void CPU::Instr0x75() { memory->SetByte(registers.hl, registers.l); }
void CPU::Instr0x77() { memory->SetByte(registers.hl, registers.a); }
void CPU::Instr0x36() { memory->SetByte(registers.hl, ReadNextByte()); }

// REGISTER A SPECIFIC
// FROM MEM
void CPU::Instr0x0A() { registers.a = memory->ReadByte(registers.bc); }
void CPU::Instr0x1A() { registers.a = memory->ReadByte(registers.de); }
void CPU::Instr0xFA() { registers.a = memory->ReadByte(ReadNextShort()); }
void CPU::Instr0x2A() { registers.a = memory->ReadByte(registers.hl++); }
void CPU::Instr0x3A() { registers.a = memory->ReadByte(registers.hl--); }
void CPU::Instr0xF0() { registers.a = memory->ReadByte(0xFF00 + ReadNextByte()); }
void CPU::Instr0xF2() { registers.a = memory->ReadByte(0xFF00 + registers.c); }
// TO MEM
void CPU::Instr0x02() { memory->SetByte(registers.bc, registers.a); }
void CPU::Instr0x12() { memory->SetByte(registers.de, registers.a); }
void CPU::Instr0xEA() { memory->SetByte(ReadNextShort(), registers.a); }
void CPU::Instr0x22() { memory->SetByte(registers.hl++, registers.a); }
void CPU::Instr0x32() { memory->SetByte(registers.hl--, registers.a); }
void CPU::Instr0xE0() { memory->SetByte(0xFF00 + ReadNextByte(), registers.a); }
void CPU::Instr0xE2() { memory->SetByte(0xFF00 + registers.c, registers.a); }

// 16 BIT LOADS
// 16 BIT IMMEDIATE
void CPU::Instr0x01() { registers.bc = ReadNextShort(); }
void CPU::Instr0x11() { registers.de = ReadNextShort(); }
void CPU::Instr0x21() { registers.hl = ReadNextShort(); }
void CPU::Instr0x31() { registers.sp = ReadNextShort(); }

// REG TO REG
void CPU::Instr0xF9() { registers.sp = registers.hl; }
void CPU::Instr0xF8() 
{ 
	int8_t num = (int8_t)ReadNextByte();
	int result = registers.sp + num;
	
	ClearFlags();
	
	if ((registers.sp ^ num ^ result) & 0x100)
		SetFlag(FLAG_CARRY);
	
	if ((registers.sp ^ num ^ result) & 0x10)
		SetFlag(FLAG_HALF_CARRY);
	
	registers.hl = result;
}

// TO MEM
void CPU::Instr0x08() { memory->SetShort(ReadNextShort(), registers.sp); }

// JUMPS
void CPU::Jp(bool condition, uint16_t address)
{
	if (condition)
	{
		registers.pc = address;
		lastInstructionCycles += 4;
	}
}

void CPU::Instr0xC3() { Jp(true, ReadNextShort()); }
void CPU::Instr0xC2() { Jp(!GetFlag(FLAG_ZERO), ReadNextShort()); }
void CPU::Instr0xCA() { Jp(GetFlag(FLAG_ZERO), ReadNextShort()); }
void CPU::Instr0xD2() { Jp(!GetFlag(FLAG_CARRY), ReadNextShort()); }
void CPU::Instr0xDA() { Jp(GetFlag(FLAG_CARRY), ReadNextShort()); }
void CPU::Instr0xE9() { Jp(true, registers.hl); }

// JUMP RELATIVE
void CPU::Jr(bool condition, int8_t offset)
{
	if (condition)
	{
		registers.pc += offset;
		lastInstructionCycles += 4;
	}
}

void CPU::Instr0x18() { Jr(true, ReadNextByte()); }
void CPU::Instr0x20() { Jr(!GetFlag(FLAG_ZERO), (int8_t)ReadNextByte()); }
void CPU::Instr0x28() { Jr(GetFlag(FLAG_ZERO), (int8_t)ReadNextByte()); }
void CPU::Instr0x30() { Jr(!GetFlag(FLAG_CARRY), (int8_t)ReadNextByte()); }
void CPU::Instr0x38() { Jr(GetFlag(FLAG_CARRY), (int8_t)ReadNextByte()); }

// CALL
void CPU::Call(bool condition, uint16_t address)
{
	if (condition)
	{
		Push16(registers.pc);
		registers.pc = address;
		lastInstructionCycles += 12;
	}
}

void CPU::Instr0xCD() { Call(true, ReadNextShort()); }
void CPU::Instr0xC4() { Call(!GetFlag(FLAG_ZERO), ReadNextShort()); }
void CPU::Instr0xCC() { Call(GetFlag(FLAG_ZERO), ReadNextShort()); }
void CPU::Instr0xD4() { Call(!GetFlag(FLAG_CARRY), ReadNextShort()); }
void CPU::Instr0xDC() { Call(GetFlag(FLAG_CARRY), ReadNextShort()); }

// RESTART
void CPU::Rst(uint16_t address)
{
	Push16(registers.pc);
	registers.pc = address;
}

void CPU::Instr0xC7() { Rst(0x00); }
void CPU::Instr0xCF() { Rst(0x08); }
void CPU::Instr0xD7() { Rst(0x10); }
void CPU::Instr0xDF() { Rst(0x18); }
void CPU::Instr0xE7() { Rst(0x20); }
void CPU::Instr0xEF() { Rst(0x28); }
void CPU::Instr0xF7() { Rst(0x30); }
void CPU::Instr0xFF() { Rst(0x38); }

// RETURN
void CPU::Ret(bool condition)
{
	if (condition)
	{
		uint16_t address = Pop16();
		registers.pc = address;
		lastInstructionCycles += 12;
	}
}

void CPU::Instr0xC9() { Ret(true); }
void CPU::Instr0xC0() { Ret(!GetFlag(FLAG_ZERO)); }
void CPU::Instr0xC8() { Ret(GetFlag(FLAG_ZERO)); }
void CPU::Instr0xD0() { Ret(!GetFlag(FLAG_CARRY)); }
void CPU::Instr0xD8() { Ret(GetFlag(FLAG_CARRY)); }
void CPU::Instr0xD9() { Ret(true); interruptMaster = true; }

//CBCBCBCBCBCBCBCBCB
//================================
// ROTATES AND SHIFTS
// RLC
void CPU::RLC(uint8_t* data, bool checkZero = true)
{
	ClearFlags();
	
	if (*data & 0x80)
	{
		SetFlag(FLAG_CARRY);
		*data <<= 1;
		*data |= 1;
	}
	else
	{
		*data <<= 1;
	}
	
	if (checkZero && !*data)
		SetFlag(FLAG_ZERO);		
}

void CPU::Instr0x07() 	{ RLC(&registers.a, false); }
void CPU::Instr0xCB07() { RLC(&registers.a); }
void CPU::Instr0xCB00() { RLC(&registers.b); }
void CPU::Instr0xCB01() { RLC(&registers.c); }
void CPU::Instr0xCB02() { RLC(&registers.d); }
void CPU::Instr0xCB03() { RLC(&registers.e); }
void CPU::Instr0xCB04() { RLC(&registers.h); }
void CPU::Instr0xCB05() { RLC(&registers.l); }
void CPU::Instr0xCB06() { RLC(memory->GetBytePointer(registers.hl)); }

// RL
void CPU::RL(uint8_t* data, bool checkZero = true)
{
	bool msb = *data & 0x80;
	
	*data <<= 1;
	*data ^= GetFlag(FLAG_CARRY)? 1 : 0;
	
	ClearFlags();
	
	if (msb)
		SetFlag(FLAG_CARRY);
	
	if (checkZero && !*data)
		SetFlag(FLAG_ZERO);	
}

void CPU::Instr0x17() 	{ RL(&registers.a, false); }
void CPU::Instr0xCB17() { RL(&registers.a); }
void CPU::Instr0xCB10() { RL(&registers.b); }
void CPU::Instr0xCB11() { RL(&registers.c); }
void CPU::Instr0xCB12() { RL(&registers.d); }
void CPU::Instr0xCB13() { RL(&registers.e); }
void CPU::Instr0xCB14() { RL(&registers.h); }
void CPU::Instr0xCB15() { RL(&registers.l); }
void CPU::Instr0xCB16() { RL(memory->GetBytePointer(registers.hl)); }

// RRC
void CPU::RRC(uint8_t* data, bool checkZero = true)
{
	ClearFlags();
	
	if (*data & 0x01)
	{
		SetFlag(FLAG_CARRY);
		*data >>= 1;
		*data |= 0x80;
	}
	else
	{
		*data >>= 1;
	}
	
	if (checkZero && !*data)
		SetFlag(FLAG_ZERO);	
}

void CPU::Instr0x0F() 	{ RRC(&registers.a, false); }
void CPU::Instr0xCB0F() { RRC(&registers.a); }
void CPU::Instr0xCB08() { RRC(&registers.b); }
void CPU::Instr0xCB09() { RRC(&registers.c); }
void CPU::Instr0xCB0A() { RRC(&registers.d); }
void CPU::Instr0xCB0B() { RRC(&registers.e); }
void CPU::Instr0xCB0C() { RRC(&registers.h); }
void CPU::Instr0xCB0D() { RRC(&registers.l); }
void CPU::Instr0xCB0E() { RRC(memory->GetBytePointer(registers.hl)); }

// RR
void CPU::RR(uint8_t* data, bool checkZero = true)
{
	bool lsb = *data & 0x01;
	
	*data >>= 1;
	*data ^= GetFlag(FLAG_CARRY)? 0x80 : 0;
	
	ClearFlags();
	
	if (lsb)
		SetFlag(FLAG_CARRY);
	
	if (checkZero && !*data)
		SetFlag(FLAG_ZERO);	
}

void CPU::Instr0x1F() 	{ RR(&registers.a, false); }
void CPU::Instr0xCB1F() { RR(&registers.a); }
void CPU::Instr0xCB18() { RR(&registers.b); }
void CPU::Instr0xCB19() { RR(&registers.c); }
void CPU::Instr0xCB1A() { RR(&registers.d); }
void CPU::Instr0xCB1B() { RR(&registers.e); }
void CPU::Instr0xCB1C() { RR(&registers.h); }
void CPU::Instr0xCB1D() { RR(&registers.l); }
void CPU::Instr0xCB1E() { RR(memory->GetBytePointer(registers.hl)); }

// SHIFTS
void CPU::SLA(uint8_t* data)
{
	ClearFlags();
	
	if ((*data) & 0x80)
		SetFlag(FLAG_CARRY);
	
	(*data) = ((*data) << 1);

	if (!(*data))
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xCB27() { SLA(&registers.a); }
void CPU::Instr0xCB20() { SLA(&registers.b); }
void CPU::Instr0xCB21() { SLA(&registers.c); }
void CPU::Instr0xCB22() { SLA(&registers.d); }
void CPU::Instr0xCB23() { SLA(&registers.e); }
void CPU::Instr0xCB24() { SLA(&registers.h); }
void CPU::Instr0xCB25() { SLA(&registers.l); }
void CPU::Instr0xCB26() { SLA(memory->GetBytePointer(registers.hl)); }

void CPU::SRA(uint8_t* data)
{
	ClearFlags();
	
	if ((*data) & 0x1)
		SetFlag(FLAG_CARRY);
	
	uint8_t msb = (*data) & 0x80;
	(*data) = ((*data) >> 1) + msb;

	if (!(*data))
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xCB2F() { SRA(&registers.a); }
void CPU::Instr0xCB28() { SRA(&registers.b); }
void CPU::Instr0xCB29() { SRA(&registers.c); }
void CPU::Instr0xCB2A() { SRA(&registers.d); }
void CPU::Instr0xCB2B() { SRA(&registers.e); }
void CPU::Instr0xCB2C() { SRA(&registers.h); }
void CPU::Instr0xCB2D() { SRA(&registers.l); }
void CPU::Instr0xCB2E() { SRA(memory->GetBytePointer(registers.hl)); }

void CPU::SRL(uint8_t* data)
{
	ClearFlags();
	
	if ((*data) & 0x1)
		SetFlag(FLAG_CARRY);
	
	(*data) = ((*data) >> 1);

	if (!(*data))
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xCB3F() { SRL(&registers.a); }
void CPU::Instr0xCB38() { SRL(&registers.b); }
void CPU::Instr0xCB39() { SRL(&registers.c); }
void CPU::Instr0xCB3A() { SRL(&registers.d); }
void CPU::Instr0xCB3B() { SRL(&registers.e); }
void CPU::Instr0xCB3C() { SRL(&registers.h); }
void CPU::Instr0xCB3D() { SRL(&registers.l); }
void CPU::Instr0xCB3E() { SRL(memory->GetBytePointer(registers.hl)); }

// SWAP
void CPU::Swap(uint8_t* data)
{
	ClearFlags();
	
	uint8_t h = (*data) >> 4;
	(*data) = ((*data) << 4) + h;
	
	if (!(*data))
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xCB37() { Swap(&registers.a); }
void CPU::Instr0xCB30() { Swap(&registers.b); }
void CPU::Instr0xCB31() { Swap(&registers.c); }
void CPU::Instr0xCB32() { Swap(&registers.d); }
void CPU::Instr0xCB33() { Swap(&registers.e); }
void CPU::Instr0xCB34() { Swap(&registers.h); }
void CPU::Instr0xCB35() { Swap(&registers.l); }
void CPU::Instr0xCB36() { Swap(memory->GetBytePointer(registers.hl)); }

// BITS
void CPU::Bit(uint8_t* data, int bit)
{
	ClearFlag(FLAG_NEG);
	SetFlag(FLAG_HALF_CARRY);
	
	if ((*data) & (1 << bit))
		ClearFlag(FLAG_ZERO);
	else
		SetFlag(FLAG_ZERO);
}

void CPU::Instr0xCB47() { Bit(&registers.a, 0); }
void CPU::Instr0xCB40() { Bit(&registers.b, 0); }
void CPU::Instr0xCB41() { Bit(&registers.c, 0); }
void CPU::Instr0xCB42() { Bit(&registers.d, 0); }
void CPU::Instr0xCB43() { Bit(&registers.e, 0); }
void CPU::Instr0xCB44() { Bit(&registers.h, 0); }
void CPU::Instr0xCB45() { Bit(&registers.l, 0); }
void CPU::Instr0xCB46() { Bit(memory->GetBytePointer(registers.hl), 0); }

void CPU::Instr0xCB4F() { Bit(&registers.a, 1); }
void CPU::Instr0xCB48() { Bit(&registers.b, 1); }
void CPU::Instr0xCB49() { Bit(&registers.c, 1); }
void CPU::Instr0xCB4A() { Bit(&registers.d, 1); }
void CPU::Instr0xCB4B() { Bit(&registers.e, 1); }
void CPU::Instr0xCB4C() { Bit(&registers.h, 1); }
void CPU::Instr0xCB4D() { Bit(&registers.l, 1); }
void CPU::Instr0xCB4E() { Bit(memory->GetBytePointer(registers.hl), 1); }

void CPU::Instr0xCB57() { Bit(&registers.a, 2); }
void CPU::Instr0xCB50() { Bit(&registers.b, 2); }
void CPU::Instr0xCB51() { Bit(&registers.c, 2); }
void CPU::Instr0xCB52() { Bit(&registers.d, 2); }
void CPU::Instr0xCB53() { Bit(&registers.e, 2); }
void CPU::Instr0xCB54() { Bit(&registers.h, 2); }
void CPU::Instr0xCB55() { Bit(&registers.l, 2); }
void CPU::Instr0xCB56() { Bit(memory->GetBytePointer(registers.hl), 2); }

void CPU::Instr0xCB5F() { Bit(&registers.a, 3); }
void CPU::Instr0xCB58() { Bit(&registers.b, 3); }
void CPU::Instr0xCB59() { Bit(&registers.c, 3); }
void CPU::Instr0xCB5A() { Bit(&registers.d, 3); }
void CPU::Instr0xCB5B() { Bit(&registers.e, 3); }
void CPU::Instr0xCB5C() { Bit(&registers.h, 3); }
void CPU::Instr0xCB5D() { Bit(&registers.l, 3); }
void CPU::Instr0xCB5E() 
{ 
	uint8_t data = memory->ReadByte(registers.hl);
	Bit(&data, 3); 
}

void CPU::Instr0xCB67() { Bit(&registers.a, 4); }
void CPU::Instr0xCB60() { Bit(&registers.b, 4); }
void CPU::Instr0xCB61() { Bit(&registers.c, 4); }
void CPU::Instr0xCB62() { Bit(&registers.d, 4); }
void CPU::Instr0xCB63() { Bit(&registers.e, 4); }
void CPU::Instr0xCB64() { Bit(&registers.h, 4); }
void CPU::Instr0xCB65() { Bit(&registers.l, 4); }
void CPU::Instr0xCB66() { Bit(memory->GetBytePointer(registers.hl), 4); }

void CPU::Instr0xCB6F() { Bit(&registers.a, 5); }
void CPU::Instr0xCB68() { Bit(&registers.b, 5); }
void CPU::Instr0xCB69() { Bit(&registers.c, 5); }
void CPU::Instr0xCB6A() { Bit(&registers.d, 5); }
void CPU::Instr0xCB6B() { Bit(&registers.e, 5); }
void CPU::Instr0xCB6C() { Bit(&registers.h, 5); }
void CPU::Instr0xCB6D() { Bit(&registers.l, 5); }
void CPU::Instr0xCB6E() { Bit(memory->GetBytePointer(registers.hl), 5); }

void CPU::Instr0xCB77() { Bit(&registers.a, 6); }
void CPU::Instr0xCB70() { Bit(&registers.b, 6); }
void CPU::Instr0xCB71() { Bit(&registers.c, 6); }
void CPU::Instr0xCB72() { Bit(&registers.d, 6); }
void CPU::Instr0xCB73() { Bit(&registers.e, 6); }
void CPU::Instr0xCB74() { Bit(&registers.h, 6); }
void CPU::Instr0xCB75() { Bit(&registers.l, 6); }
void CPU::Instr0xCB76() { Bit(memory->GetBytePointer(registers.hl), 6); }

void CPU::Instr0xCB7F() { Bit(&registers.a, 7); }
void CPU::Instr0xCB78() { Bit(&registers.b, 7); }
void CPU::Instr0xCB79() { Bit(&registers.c, 7); }
void CPU::Instr0xCB7A() { Bit(&registers.d, 7); }
void CPU::Instr0xCB7B() { Bit(&registers.e, 7); }
void CPU::Instr0xCB7C() { Bit(&registers.h, 7); }
void CPU::Instr0xCB7D() { Bit(&registers.l, 7); }
void CPU::Instr0xCB7E() { Bit(memory->GetBytePointer(registers.hl), 7); }

// RESET BIT
void CPU::Res(uint8_t* data, int bit)
{
	(*data) &= ~(1 << bit);
}

void CPU::Instr0xCB87() { Res(&registers.a, 0); }
void CPU::Instr0xCB80() { Res(&registers.b, 0); }
void CPU::Instr0xCB81() { Res(&registers.c, 0); }
void CPU::Instr0xCB82() { Res(&registers.d, 0); }
void CPU::Instr0xCB83() { Res(&registers.e, 0); }
void CPU::Instr0xCB84() { Res(&registers.h, 0); }
void CPU::Instr0xCB85() { Res(&registers.l, 0); }
void CPU::Instr0xCB86() { Res(memory->GetBytePointer(registers.hl), 0); }

void CPU::Instr0xCB8F() { Res(&registers.a, 1); }
void CPU::Instr0xCB88() { Res(&registers.b, 1); }
void CPU::Instr0xCB89() { Res(&registers.c, 1); }
void CPU::Instr0xCB8A() { Res(&registers.d, 1); }
void CPU::Instr0xCB8B() { Res(&registers.e, 1); }
void CPU::Instr0xCB8C() { Res(&registers.h, 1); }
void CPU::Instr0xCB8D() { Res(&registers.l, 1); }
void CPU::Instr0xCB8E() { Res(memory->GetBytePointer(registers.hl), 1); }

void CPU::Instr0xCB97() { Res(&registers.a, 2); }
void CPU::Instr0xCB90() { Res(&registers.b, 2); }
void CPU::Instr0xCB91() { Res(&registers.c, 2); }
void CPU::Instr0xCB92() { Res(&registers.d, 2); }
void CPU::Instr0xCB93() { Res(&registers.e, 2); }
void CPU::Instr0xCB94() { Res(&registers.h, 2); }
void CPU::Instr0xCB95() { Res(&registers.l, 2); }
void CPU::Instr0xCB96() { Res(memory->GetBytePointer(registers.hl), 2); }

void CPU::Instr0xCB9F() { Res(&registers.a, 3); }
void CPU::Instr0xCB98() { Res(&registers.b, 3); }
void CPU::Instr0xCB99() { Res(&registers.c, 3); }
void CPU::Instr0xCB9A() { Res(&registers.d, 3); }
void CPU::Instr0xCB9B() { Res(&registers.e, 3); }
void CPU::Instr0xCB9C() { Res(&registers.h, 3); }
void CPU::Instr0xCB9D() { Res(&registers.l, 3); }
void CPU::Instr0xCB9E() { Res(memory->GetBytePointer(registers.hl), 3); }

void CPU::Instr0xCBA7() { Res(&registers.a, 4); }
void CPU::Instr0xCBA0() { Res(&registers.b, 4); }
void CPU::Instr0xCBA1() { Res(&registers.c, 4); }
void CPU::Instr0xCBA2() { Res(&registers.d, 4); }
void CPU::Instr0xCBA3() { Res(&registers.e, 4); }
void CPU::Instr0xCBA4() { Res(&registers.h, 4); }
void CPU::Instr0xCBA5() { Res(&registers.l, 4); }
void CPU::Instr0xCBA6() { Res(memory->GetBytePointer(registers.hl), 4); }

void CPU::Instr0xCBAF() { Res(&registers.a, 5); }
void CPU::Instr0xCBA8() { Res(&registers.b, 5); }
void CPU::Instr0xCBA9() { Res(&registers.c, 5); }
void CPU::Instr0xCBAA() { Res(&registers.d, 5); }
void CPU::Instr0xCBAB() { Res(&registers.e, 5); }
void CPU::Instr0xCBAC() { Res(&registers.h, 5); }
void CPU::Instr0xCBAD() { Res(&registers.l, 5); }
void CPU::Instr0xCBAE() { Res(memory->GetBytePointer(registers.hl), 5); }

void CPU::Instr0xCBB7() { Res(&registers.a, 6); }
void CPU::Instr0xCBB0() { Res(&registers.b, 6); }
void CPU::Instr0xCBB1() { Res(&registers.c, 6); }
void CPU::Instr0xCBB2() { Res(&registers.d, 6); }
void CPU::Instr0xCBB3() { Res(&registers.e, 6); }
void CPU::Instr0xCBB4() { Res(&registers.h, 6); }
void CPU::Instr0xCBB5() { Res(&registers.l, 6); }
void CPU::Instr0xCBB6() { Res(memory->GetBytePointer(registers.hl), 6); }

void CPU::Instr0xCBBF() { Res(&registers.a, 7); }
void CPU::Instr0xCBB8() { Res(&registers.b, 7); }
void CPU::Instr0xCBB9() { Res(&registers.c, 7); }
void CPU::Instr0xCBBA() { Res(&registers.d, 7); }
void CPU::Instr0xCBBB() { Res(&registers.e, 7); }
void CPU::Instr0xCBBC() { Res(&registers.h, 7); }
void CPU::Instr0xCBBD() { Res(&registers.l, 7); }
void CPU::Instr0xCBBE() { Res(memory->GetBytePointer(registers.hl), 7); }

// SET BIT
void CPU::Set(uint8_t* data, int bit)
{
	(*data) |= (1 << bit);
}

void CPU::Instr0xCBC7() { Set(&registers.a, 0); }
void CPU::Instr0xCBC0() { Set(&registers.b, 0); }
void CPU::Instr0xCBC1() { Set(&registers.c, 0); }
void CPU::Instr0xCBC2() { Set(&registers.d, 0); }
void CPU::Instr0xCBC3() { Set(&registers.e, 0); }
void CPU::Instr0xCBC4() { Set(&registers.h, 0); }
void CPU::Instr0xCBC5() { Set(&registers.l, 0); }
void CPU::Instr0xCBC6() { Set(memory->GetBytePointer(registers.hl), 0); }

void CPU::Instr0xCBCF() { Set(&registers.a, 1); }
void CPU::Instr0xCBC8() { Set(&registers.b, 1); }
void CPU::Instr0xCBC9() { Set(&registers.c, 1); }
void CPU::Instr0xCBCA() { Set(&registers.d, 1); }
void CPU::Instr0xCBCB() { Set(&registers.e, 1); }
void CPU::Instr0xCBCC() { Set(&registers.h, 1); }
void CPU::Instr0xCBCD() { Set(&registers.l, 1); }
void CPU::Instr0xCBCE() { Set(memory->GetBytePointer(registers.hl), 1); }

void CPU::Instr0xCBD7() { Set(&registers.a, 2); }
void CPU::Instr0xCBD0() { Set(&registers.b, 2); }
void CPU::Instr0xCBD1() { Set(&registers.c, 2); }
void CPU::Instr0xCBD2() { Set(&registers.d, 2); }
void CPU::Instr0xCBD3() { Set(&registers.e, 2); }
void CPU::Instr0xCBD4() { Set(&registers.h, 2); }
void CPU::Instr0xCBD5() { Set(&registers.l, 2); }
void CPU::Instr0xCBD6() { Set(memory->GetBytePointer(registers.hl), 2); }

void CPU::Instr0xCBDF() { Set(&registers.a, 3); }
void CPU::Instr0xCBD8() { Set(&registers.b, 3); }
void CPU::Instr0xCBD9() { Set(&registers.c, 3); }
void CPU::Instr0xCBDA() { Set(&registers.d, 3); }
void CPU::Instr0xCBDB() { Set(&registers.e, 3); }
void CPU::Instr0xCBDC() { Set(&registers.h, 3); }
void CPU::Instr0xCBDD() { Set(&registers.l, 3); }
void CPU::Instr0xCBDE() 
{ 
	uint8_t data = memory->ReadByte(registers.hl);
	Set(&data, 3);
	memory->SetByte(registers.hl, data);
}

void CPU::Instr0xCBE7() { Set(&registers.a, 4); }
void CPU::Instr0xCBE0() { Set(&registers.b, 4); }
void CPU::Instr0xCBE1() { Set(&registers.c, 4); }
void CPU::Instr0xCBE2() { Set(&registers.d, 4); }
void CPU::Instr0xCBE3() { Set(&registers.e, 4); }
void CPU::Instr0xCBE4() { Set(&registers.h, 4); }
void CPU::Instr0xCBE5() { Set(&registers.l, 4); }
void CPU::Instr0xCBE6() { Set(memory->GetBytePointer(registers.hl), 4); }

void CPU::Instr0xCBEF() { Set(&registers.a, 5); }
void CPU::Instr0xCBE8() { Set(&registers.b, 5); }
void CPU::Instr0xCBE9() { Set(&registers.c, 5); }
void CPU::Instr0xCBEA() { Set(&registers.d, 5); }
void CPU::Instr0xCBEB() { Set(&registers.e, 5); }
void CPU::Instr0xCBEC() { Set(&registers.h, 5); }
void CPU::Instr0xCBED() { Set(&registers.l, 5); }
void CPU::Instr0xCBEE() { Set(memory->GetBytePointer(registers.hl), 5); }

void CPU::Instr0xCBF7() { Set(&registers.a, 6); }
void CPU::Instr0xCBF0() { Set(&registers.b, 6); }
void CPU::Instr0xCBF1() { Set(&registers.c, 6); }
void CPU::Instr0xCBF2() { Set(&registers.d, 6); }
void CPU::Instr0xCBF3() { Set(&registers.e, 6); }
void CPU::Instr0xCBF4() { Set(&registers.h, 6); }
void CPU::Instr0xCBF5() { Set(&registers.l, 6); }
void CPU::Instr0xCBF6() { Set(memory->GetBytePointer(registers.hl), 6); }

void CPU::Instr0xCBFF() { Set(&registers.a, 7); }
void CPU::Instr0xCBF8() { Set(&registers.b, 7); }
void CPU::Instr0xCBF9() { Set(&registers.c, 7); }
void CPU::Instr0xCBFA() { Set(&registers.d, 7); }
void CPU::Instr0xCBFB() { Set(&registers.e, 7); }
void CPU::Instr0xCBFC() { Set(&registers.h, 7); }
void CPU::Instr0xCBFD() { Set(&registers.l, 7); }
void CPU::Instr0xCBFE() { Set(memory->GetBytePointer(registers.hl), 7); }

// THE FABLED DAA
void CPU::Instr0x27()
{
	uint16_t val = registers.a;
	
	if (!GetFlag(FLAG_NEG))
	{
		if (GetFlag(FLAG_HALF_CARRY) || ((val & 0xF) > 9))
			val += 0x06;
		
		if (GetFlag(FLAG_CARRY) || (val > 0x9F))
			val += 0x60;
	}
	else
	{
		if (GetFlag(FLAG_HALF_CARRY))
			val = (val - 0x6) & 0xFF;
		
		if (GetFlag(FLAG_CARRY))
			val -= 0x60;
	}
	
	ClearFlag(FLAG_HALF_CARRY);
	
	if (val & 0x100)
		SetFlag(FLAG_CARRY);
	
	val &= 0xFF;
	
	if (val)
		ClearFlag(FLAG_ZERO);
	else
		SetFlag(FLAG_ZERO);
	
	registers.a = (uint8_t) val;
}
