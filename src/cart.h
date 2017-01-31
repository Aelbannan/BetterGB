#ifndef __CART__
#define __CART__

#include <stdint.h>

class Cart
{
	public:
		bool isCGB;
		bool hasBattery;
	
		static Cart* Load(char* filename);
		
		Cart(int ramSize, bool hasBattery);
		void SetROM(uint8_t* rom);
		virtual uint8_t ReadROM(uint16_t address) = 0;
		virtual void WriteROM(uint16_t address, uint8_t data) = 0;
		virtual uint8_t* GetROMPtr(uint16_t address) = 0;
		virtual uint8_t ReadRAM(uint16_t address) = 0;
		virtual void WriteRAM(uint16_t address, uint8_t data) = 0;
		
	protected:
		uint8_t* rom;
		uint8_t* ram;
};

#endif