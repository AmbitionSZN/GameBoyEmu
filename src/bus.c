#include "bus.h"
#include "cart.h"
#include <stdint.h>

uint8_t busRead(uint16_t address, Cartridge *cart) {		
	return cart->RomData[address];
}
