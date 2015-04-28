#include "structure.h"


// Down Button Element
const struct Element touch_element = {
		.inputPxselRegister = (uint8_t *)&P1SEL,
		.inputPxsel2Register = (uint8_t *)&P1SEL2,
		.inputBits = BIT0,
		.maxResponse = 4000,
		.threshold = 3200
};

// Up Button
const struct Sensor touchpad = {
		.halDefinition = RO_PINOSC_TA0_WDTp,	// Sensing Method
		.numElements = 1,						// # of elements
		.baseOffset = 0,						// First element index
		.arrayPtr[0] = &touch_element,			//
		.measGateSource = GATE_WDT_ACLK,		//
		.accumulationCycles = WDTp_GATE_512
};
