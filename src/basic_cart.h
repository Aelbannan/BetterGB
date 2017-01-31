#ifndef __BASIC_CART__
#define __BASIC_CART__

#include "cart.h"

/*
 * The basic GB Cart.
 * CAN read from ROM
 * NO RAM
 */
class BasicCart : public Cart
{
	public:
		BasicCart(int ramSize = 0, bool hasBattery = false) 
			: Cart(ramSize, hasBattery) {};
			
		uint8_t ReadROM(uint16_t address) override;
		void WriteROM(uint16_t address, uint8_t data) override;
		uint8_t* GetROMPtr(uint16_t address) override;
		uint8_t ReadRAM(uint16_t address) override;
		void WriteRAM(uint16_t address, uint8_t data) override;
};

#endif