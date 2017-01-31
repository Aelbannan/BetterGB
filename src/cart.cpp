#include "cart.h"
#include "basic_cart.h"
#include "mbc1_cart.h"
#include <iostream>
#include <fstream>

using namespace std;

char* GetBytes(char* filename)
{
	char* bytes;
	
	// LOAD THE CART
	// Create filestream
	ifstream ifs(filename);
	ifs.seekg(0, ios::end);
	// Get size of file
	size_t len = ifs.tellg();
	
	printf("Loaded rom of size %i bytes\n", len);
	
	// Create array for rom
	bytes = new char[len];
	
	// Seek to beginning and read
	ifs.seekg(0, ios::beg);
	ifs.read(bytes, len);
	
	// close file
	ifs.close();
	
	return bytes;
}

int GetRAMSize(uint8_t ramType)
{
	printf("RAM SIZE 0x%02x\n", ramType);
	switch (ramType)
	{
		case 0x00: return 0x0000;
		case 0x01: return 0x0800;
		case 0x02: return 0x2000;
		case 0x03: return 0x8000;
		default: return 0;
	}
}

Cart* CreateSuitableCart(uint8_t type, uint8_t ramType)
{
	printf("CART: 0x%02x\n", type); 
	
	switch (type)
	{
		case 0x00: return new BasicCart();		
		case 0x01: return new MBC1Cart();
		case 0x02: return new MBC1Cart(GetRAMSize(ramType));
		case 0x03: return new MBC1Cart(GetRAMSize(ramType), true);
		default : return NULL;
	}
}

Cart* Cart::Load(char* filename)
{
	Cart* cart;
	uint8_t* rom;
	
	// Get file
	rom = (uint8_t*) GetBytes(filename);
	
	// Initialize based on Header
	cart = CreateSuitableCart(rom[0x147], rom[0x149]);
	cart->isCGB = rom[0x143] & 0x80;
	cart->rom = rom;
	
	return cart;
}

Cart::Cart(int ramSize, bool hasBattery)
{
	if (ramSize)
	{
		ram = (uint8_t*) new char[ramSize];
	}
	
	this->hasBattery = hasBattery;
}