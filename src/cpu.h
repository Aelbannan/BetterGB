#ifndef __CPU__
#define __CPU__

#include <stdint.h>
#include "memory.h"

class CPU
{
	// CPU Intstruction (function pointer)
	typedef void (CPU::*CPUInstr) (void);
	
	// Instruction struct
	struct Instruction
	{
		const char* label;
		uint8_t operandLength;
		CPUInstr function;
	};
	
	// Registers struct
	struct Registers
	{
		// f is flag register
		union
		{
			struct { uint8_t f, a;};
			uint16_t af;
		};
		union
		{
			struct { uint8_t c, b;};
			uint16_t bc;
		};
		union
		{
			struct { uint8_t e, d;};
			uint16_t de;
		};
		union
		{
			struct { uint8_t l, h;};
			uint16_t hl;
		};
		
		uint16_t sp;
		uint16_t pc;
	};
	
	public:
		// # of cycles last instruction took
		int lastInstructionCycles;
		// if STOP was called
		bool isStopped;
	
		CPU(Memory* memory);
		void Reset();
		void Step();
		
	private:
		// Memory
		Memory* memory;
		// Registers
		Registers registers;
		// If interrupts are enabled
		bool interruptMaster;
		int imeState;
		uint8_t* interruptsEnabled;
		uint8_t* interruptFlags;
		// Instructions (extended after 256)
		const Instruction* instructions;
		// Cycles per instruction
		const int* instructionCycles;		
		// Timer variables
		uint8_t	*div, *tima, *tma, *tac;
		int divCycles;
		int timerCycles;
		// If HALT was called
		bool isHalted;
		
		void Setup();
		
		void ExecuteInterrupts();
		void ExecuteOpcode();
		void UpdateTimer();
		
		// Flags
		void SetFlag(int flag);
		bool GetFlag(int flag);
		void ClearFlag(int flag);
		void ClearFlags();
		
		// Immediate Reads
		uint8_t ReadNextByte();
		uint16_t ReadNextShort();
		
		// Addition
		void Add8(uint8_t data);
		void Adc8(uint8_t data);
		void Sub8(uint8_t data);
		void Sbc8(uint8_t data);
		void And8(uint8_t data);
		void Or8(uint8_t data);
		void Xor8(uint8_t data);
		void Cp8(uint8_t data);
		
		// 16 bit arithmetic
		void Add16(uint16_t data);
		
		// Increment and decrement
		void Inc8(uint8_t* num);
		void Dec8(uint8_t* num);
		void Inc16(uint16_t* num);
		void Dec16(uint16_t* num);
		
		// Stack Functions
		void Push16(uint16_t data);
		uint16_t Pop16();
		
		// Jump
		void Jp(bool condition, uint16_t address);
		void Jr(bool condition, int8_t offset);
		
		// Call
		void Call(bool condition, uint16_t address);
		
		// Restart
		void Rst(uint16_t address);
		
		// Return
		void Ret(bool condition);
		
		// Rotates
		void RLC(uint8_t* data, bool checkZero);
		void RL(uint8_t* data, bool checkZero);
		void RRC(uint8_t* data, bool checkZero);
		void RR(uint8_t* data, bool checkZero);
		
		// Shifts
		void SLA(uint8_t* data);
		void SRA(uint8_t* data);
		void SRL(uint8_t* data);
		
		// Swap
		void Swap(uint8_t* data);
		
		// Bit
		void Bit(uint8_t* data, int bit);
		void Res(uint8_t* data, int bit);
		void Set(uint8_t* data, int bit);
		
		// Opcode Functions
		// 0x00
		void Instr0x00();
		void Instr0x01();
		void Instr0x02();
		void Instr0x03();
		void Instr0x04();
		void Instr0x05();
		void Instr0x06();
		void Instr0x07();
		void Instr0x08();
		void Instr0x09();
		void Instr0x0A();
		void Instr0x0B();
		void Instr0x0C();
		void Instr0x0D();
		void Instr0x0E();
		void Instr0x0F();
		
		// 0x10
		void Instr0x10();
		void Instr0x11();
		void Instr0x12();
		void Instr0x13();
		void Instr0x14();
		void Instr0x15();
		void Instr0x16();
		void Instr0x17();
		void Instr0x18();
		void Instr0x19();
		void Instr0x1A();
		void Instr0x1B();
		void Instr0x1C();
		void Instr0x1D();
		void Instr0x1E();
		void Instr0x1F();
		
		// 0x20
		void Instr0x20();
		void Instr0x21();
		void Instr0x22();
		void Instr0x23();
		void Instr0x24();
		void Instr0x25();
		void Instr0x26();
		void Instr0x27();
		void Instr0x28();
		void Instr0x29();
		void Instr0x2A();
		void Instr0x2B();
		void Instr0x2C();
		void Instr0x2D();
		void Instr0x2E();
		void Instr0x2F();
		
		// 0x30
		void Instr0x30();
		void Instr0x31();
		void Instr0x32();
		void Instr0x33();
		void Instr0x34();
		void Instr0x35();
		void Instr0x36();
		void Instr0x37();
		void Instr0x38();
		void Instr0x39();
		void Instr0x3A();
		void Instr0x3B();
		void Instr0x3C();
		void Instr0x3D();
		void Instr0x3E();
		void Instr0x3F();
		
		// 0x40
		void Instr0x40();
		void Instr0x41();
		void Instr0x42();
		void Instr0x43();
		void Instr0x44();
		void Instr0x45();
		void Instr0x46();
		void Instr0x47();
		void Instr0x48();
		void Instr0x49();
		void Instr0x4A();
		void Instr0x4B();
		void Instr0x4C();
		void Instr0x4D();
		void Instr0x4E();
		void Instr0x4F();
		
		// 0x50
		void Instr0x50();
		void Instr0x51();
		void Instr0x52();
		void Instr0x53();
		void Instr0x54();
		void Instr0x55();
		void Instr0x56();
		void Instr0x57();
		void Instr0x58();
		void Instr0x59();
		void Instr0x5A();
		void Instr0x5B();
		void Instr0x5C();
		void Instr0x5D();
		void Instr0x5E();
		void Instr0x5F();
		
		// 0x60
		void Instr0x60();
		void Instr0x61();
		void Instr0x62();
		void Instr0x63();
		void Instr0x64();
		void Instr0x65();
		void Instr0x66();
		void Instr0x67();
		void Instr0x68();
		void Instr0x69();
		void Instr0x6A();
		void Instr0x6B();
		void Instr0x6C();
		void Instr0x6D();
		void Instr0x6E();
		void Instr0x6F();
		
		// 0x70
		void Instr0x70();
		void Instr0x71();
		void Instr0x72();
		void Instr0x73();
		void Instr0x74();
		void Instr0x75();
		void Instr0x76();
		void Instr0x77();
		void Instr0x78();
		void Instr0x79();
		void Instr0x7A();
		void Instr0x7B();
		void Instr0x7C();
		void Instr0x7D();
		void Instr0x7E();
		void Instr0x7F();
		
		// 0x80
		void Instr0x80();
		void Instr0x81();
		void Instr0x82();
		void Instr0x83();
		void Instr0x84();
		void Instr0x85();
		void Instr0x86();
		void Instr0x87();
		void Instr0x88();
		void Instr0x89();
		void Instr0x8A();
		void Instr0x8B();
		void Instr0x8C();
		void Instr0x8D();
		void Instr0x8E();
		void Instr0x8F();
		
		// 0x90
		void Instr0x90();
		void Instr0x91();
		void Instr0x92();
		void Instr0x93();
		void Instr0x94();
		void Instr0x95();
		void Instr0x96();
		void Instr0x97();
		void Instr0x98();
		void Instr0x99();
		void Instr0x9A();
		void Instr0x9B();
		void Instr0x9C();
		void Instr0x9D();
		void Instr0x9E();
		void Instr0x9F();
		
		// 0xA0
		void Instr0xA0();
		void Instr0xA1();
		void Instr0xA2();
		void Instr0xA3();
		void Instr0xA4();
		void Instr0xA5();
		void Instr0xA6();
		void Instr0xA7();
		void Instr0xA8();
		void Instr0xA9();
		void Instr0xAA();
		void Instr0xAB();
		void Instr0xAC();
		void Instr0xAD();
		void Instr0xAE();
		void Instr0xAF();
		
		// 0xB0
		void Instr0xB0();
		void Instr0xB1();
		void Instr0xB2();
		void Instr0xB3();
		void Instr0xB4();
		void Instr0xB5();
		void Instr0xB6();
		void Instr0xB7();
		void Instr0xB8();
		void Instr0xB9();
		void Instr0xBA();
		void Instr0xBB();
		void Instr0xBC();
		void Instr0xBD();
		void Instr0xBE();
		void Instr0xBF();
		
		// 0xC0
		void Instr0xC0();
		void Instr0xC1();
		void Instr0xC2();
		void Instr0xC3();
		void Instr0xC4();
		void Instr0xC5();
		void Instr0xC6();
		void Instr0xC7();
		void Instr0xC8();
		void Instr0xC9();
		void Instr0xCA();
		void Instr0xCB();
		void Instr0xCC();
		void Instr0xCD();
		void Instr0xCE();
		void Instr0xCF();
		
		// 0xD0
		void Instr0xD0();
		void Instr0xD1();
		void Instr0xD2();
		void Instr0xD3();
		void Instr0xD4();
		void Instr0xD5();
		void Instr0xD6();
		void Instr0xD7();
		void Instr0xD8();
		void Instr0xD9();
		void Instr0xDA();
		void Instr0xDB();
		void Instr0xDC();
		void Instr0xDD();
		void Instr0xDE();
		void Instr0xDF();
		
		// 0xE0
		void Instr0xE0();
		void Instr0xE1();
		void Instr0xE2();
		void Instr0xE3();
		void Instr0xE4();
		void Instr0xE5();
		void Instr0xE6();
		void Instr0xE7();
		void Instr0xE8();
		void Instr0xE9();
		void Instr0xEA();
		void Instr0xEB();
		void Instr0xEC();
		void Instr0xED();
		void Instr0xEE();
		void Instr0xEF();

		// 0xF0
		void Instr0xF0();
		void Instr0xF1();
		void Instr0xF2();
		void Instr0xF3();
		void Instr0xF4();
		void Instr0xF5();
		void Instr0xF6();
		void Instr0xF7();
		void Instr0xF8();
		void Instr0xF9();
		void Instr0xFA();
		void Instr0xFB();
		void Instr0xFC();
		void Instr0xFD();
		void Instr0xFE();
		void Instr0xFF();
		
		// CB Opcode Functions
		// 0xCB00
		void Instr0xCB00();
		void Instr0xCB01();
		void Instr0xCB02();
		void Instr0xCB03();
		void Instr0xCB04();
		void Instr0xCB05();
		void Instr0xCB06();
		void Instr0xCB07();
		void Instr0xCB08();
		void Instr0xCB09();
		void Instr0xCB0A();
		void Instr0xCB0B();
		void Instr0xCB0C();
		void Instr0xCB0D();
		void Instr0xCB0E();
		void Instr0xCB0F();
		
		// 0xCB10
		void Instr0xCB10();
		void Instr0xCB11();
		void Instr0xCB12();
		void Instr0xCB13();
		void Instr0xCB14();
		void Instr0xCB15();
		void Instr0xCB16();
		void Instr0xCB17();
		void Instr0xCB18();
		void Instr0xCB19();
		void Instr0xCB1A();
		void Instr0xCB1B();
		void Instr0xCB1C();
		void Instr0xCB1D();
		void Instr0xCB1E();
		void Instr0xCB1F();
		
		// 0xCB20
		void Instr0xCB20();
		void Instr0xCB21();
		void Instr0xCB22();
		void Instr0xCB23();
		void Instr0xCB24();
		void Instr0xCB25();
		void Instr0xCB26();
		void Instr0xCB27();
		void Instr0xCB28();
		void Instr0xCB29();
		void Instr0xCB2A();
		void Instr0xCB2B();
		void Instr0xCB2C();
		void Instr0xCB2D();
		void Instr0xCB2E();
		void Instr0xCB2F();
		
		// 0xCB30
		void Instr0xCB30();
		void Instr0xCB31();
		void Instr0xCB32();
		void Instr0xCB33();
		void Instr0xCB34();
		void Instr0xCB35();
		void Instr0xCB36();
		void Instr0xCB37();
		void Instr0xCB38();
		void Instr0xCB39();
		void Instr0xCB3A();
		void Instr0xCB3B();
		void Instr0xCB3C();
		void Instr0xCB3D();
		void Instr0xCB3E();
		void Instr0xCB3F();
		
		// 0xCB40
		void Instr0xCB40();
		void Instr0xCB41();
		void Instr0xCB42();
		void Instr0xCB43();
		void Instr0xCB44();
		void Instr0xCB45();
		void Instr0xCB46();
		void Instr0xCB47();
		void Instr0xCB48();
		void Instr0xCB49();
		void Instr0xCB4A();
		void Instr0xCB4B();
		void Instr0xCB4C();
		void Instr0xCB4D();
		void Instr0xCB4E();
		void Instr0xCB4F();
		
		// 0xCB50
		void Instr0xCB50();
		void Instr0xCB51();
		void Instr0xCB52();
		void Instr0xCB53();
		void Instr0xCB54();
		void Instr0xCB55();
		void Instr0xCB56();
		void Instr0xCB57();
		void Instr0xCB58();
		void Instr0xCB59();
		void Instr0xCB5A();
		void Instr0xCB5B();
		void Instr0xCB5C();
		void Instr0xCB5D();
		void Instr0xCB5E();
		void Instr0xCB5F();
		
		// 0xCB60
		void Instr0xCB60();
		void Instr0xCB61();
		void Instr0xCB62();
		void Instr0xCB63();
		void Instr0xCB64();
		void Instr0xCB65();
		void Instr0xCB66();
		void Instr0xCB67();
		void Instr0xCB68();
		void Instr0xCB69();
		void Instr0xCB6A();
		void Instr0xCB6B();
		void Instr0xCB6C();
		void Instr0xCB6D();
		void Instr0xCB6E();
		void Instr0xCB6F();
		
		// 0xCB70
		void Instr0xCB70();
		void Instr0xCB71();
		void Instr0xCB72();
		void Instr0xCB73();
		void Instr0xCB74();
		void Instr0xCB75();
		void Instr0xCB76();
		void Instr0xCB77();
		void Instr0xCB78();
		void Instr0xCB79();
		void Instr0xCB7A();
		void Instr0xCB7B();
		void Instr0xCB7C();
		void Instr0xCB7D();
		void Instr0xCB7E();
		void Instr0xCB7F();
		
		// 0xCB80
		void Instr0xCB80();
		void Instr0xCB81();
		void Instr0xCB82();
		void Instr0xCB83();
		void Instr0xCB84();
		void Instr0xCB85();
		void Instr0xCB86();
		void Instr0xCB87();
		void Instr0xCB88();
		void Instr0xCB89();
		void Instr0xCB8A();
		void Instr0xCB8B();
		void Instr0xCB8C();
		void Instr0xCB8D();
		void Instr0xCB8E();
		void Instr0xCB8F();
		
		// 0xCB90
		void Instr0xCB90();
		void Instr0xCB91();
		void Instr0xCB92();
		void Instr0xCB93();
		void Instr0xCB94();
		void Instr0xCB95();
		void Instr0xCB96();
		void Instr0xCB97();
		void Instr0xCB98();
		void Instr0xCB99();
		void Instr0xCB9A();
		void Instr0xCB9B();
		void Instr0xCB9C();
		void Instr0xCB9D();
		void Instr0xCB9E();
		void Instr0xCB9F();
		
		// 0xCBA0
		void Instr0xCBA0();
		void Instr0xCBA1();
		void Instr0xCBA2();
		void Instr0xCBA3();
		void Instr0xCBA4();
		void Instr0xCBA5();
		void Instr0xCBA6();
		void Instr0xCBA7();
		void Instr0xCBA8();
		void Instr0xCBA9();
		void Instr0xCBAA();
		void Instr0xCBAB();
		void Instr0xCBAC();
		void Instr0xCBAD();
		void Instr0xCBAE();
		void Instr0xCBAF();
		
		// 0xCBB0
		void Instr0xCBB0();
		void Instr0xCBB1();
		void Instr0xCBB2();
		void Instr0xCBB3();
		void Instr0xCBB4();
		void Instr0xCBB5();
		void Instr0xCBB6();
		void Instr0xCBB7();
		void Instr0xCBB8();
		void Instr0xCBB9();
		void Instr0xCBBA();
		void Instr0xCBBB();
		void Instr0xCBBC();
		void Instr0xCBBD();
		void Instr0xCBBE();
		void Instr0xCBBF();
		
		// 0xCBC0
		void Instr0xCBC0();
		void Instr0xCBC1();
		void Instr0xCBC2();
		void Instr0xCBC3();
		void Instr0xCBC4();
		void Instr0xCBC5();
		void Instr0xCBC6();
		void Instr0xCBC7();
		void Instr0xCBC8();
		void Instr0xCBC9();
		void Instr0xCBCA();
		void Instr0xCBCB();
		void Instr0xCBCC();
		void Instr0xCBCD();
		void Instr0xCBCE();
		void Instr0xCBCF();
		
		// 0xCBD0
		void Instr0xCBD0();
		void Instr0xCBD1();
		void Instr0xCBD2();
		void Instr0xCBD3();
		void Instr0xCBD4();
		void Instr0xCBD5();
		void Instr0xCBD6();
		void Instr0xCBD7();
		void Instr0xCBD8();
		void Instr0xCBD9();
		void Instr0xCBDA();
		void Instr0xCBDB();
		void Instr0xCBDC();
		void Instr0xCBDD();
		void Instr0xCBDE();
		void Instr0xCBDF();
		
		// 0xCBE0
		void Instr0xCBE0();
		void Instr0xCBE1();
		void Instr0xCBE2();
		void Instr0xCBE3();
		void Instr0xCBE4();
		void Instr0xCBE5();
		void Instr0xCBE6();
		void Instr0xCBE7();
		void Instr0xCBE8();
		void Instr0xCBE9();
		void Instr0xCBEA();
		void Instr0xCBEB();
		void Instr0xCBEC();
		void Instr0xCBED();
		void Instr0xCBEE();
		void Instr0xCBEF();

		// 0xCBF0
		void Instr0xCBF0();
		void Instr0xCBF1();
		void Instr0xCBF2();
		void Instr0xCBF3();
		void Instr0xCBF4();
		void Instr0xCBF5();
		void Instr0xCBF6();
		void Instr0xCBF7();
		void Instr0xCBF8();
		void Instr0xCBF9();
		void Instr0xCBFA();
		void Instr0xCBFB();
		void Instr0xCBFC();
		void Instr0xCBFD();
		void Instr0xCBFE();
		void Instr0xCBFF();
};



#endif