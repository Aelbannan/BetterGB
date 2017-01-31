#ifndef __MBC1_CART__
#define __MBC1_CART__

#include "cart.h"

class MBC1Cart : public Cart
{
	public:
		MBC1Cart(int ramSize = 0, bool hasBattery = false) 
			: Cart(ramSize, hasBattery) {};
			
		uint8_t ReadROM(uint16_t address) override;
		void WriteROM(uint16_t address, uint8_t data) override;
		uint8_t* GetROMPtr(uint16_t address) override;
		uint8_t ReadRAM(uint16_t address) override;
		void WriteRAM(uint16_t address, uint8_t data) override;
		
	private:
		bool ramSelect = false;
		uint8_t bankNumber = 0x01;
};

#endif