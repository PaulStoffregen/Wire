/*
  TwoWire.cpp - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
*/

#include "Wire.h"

// Only T3.6
#if defined(WIRE_DEFINE_WIRE1) && defined(__MK66FX1M0__)

#include "kinetis.h"
#include <string.h> // for memcpy
#include "core_pins.h"
//#include "HardwareSerial.h"
#include "Wire.h"

TwoWire3::TwoWire3() : TwoWire()
{
	sda_pin_num = 56;
	scl_pin_num = 57;
}

KINETIS_I2C_t *TwoWire3::kinetisk_i2c (void) {
    return &KINETIS_I2C3;
}

void TwoWire3::begin(void)
{
	// Make sure we have our data structure allocated. 
	checkAndAllocateStruct();

	//serial_begin(BAUD2DIV(115200));
	//serial_print("\nWire Begin\n");

	_pwires->slave_mode = 0;
	SIM_SCGC1 |= SIM_SCGC1_I2C3; // TODO: use bitband
	KINETIS_I2C3.C1 = 0;

	// Currently only pin each
	// SCL
	CORE_PIN56_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE; 
	// SDA
	CORE_PIN57_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	setClock(100000);
	KINETIS_I2C3.C2 = I2C_C2_HDRS;
	KINETIS_I2C3.C1 = I2C_C1_IICEN;
}

void TwoWire3::begin(uint8_t address)
{
	begin();
	KINETIS_I2C3.A1 = address << 1;
	_pwires->slave_mode = 1;
	KINETIS_I2C3.C1 = I2C_C1_IICEN | I2C_C1_IICIE;
	NVIC_ENABLE_IRQ(IRQ_I2C3);
}


void TwoWire3::setSDA(uint8_t pin)
{
}

void TwoWire3::setSCL(uint8_t pin)
{
}

uint8_t TwoWire3::checkSIM_SCG()
{
	return ( SIM_SCGC1 & SIM_SCGC1_I2C3)? 1 : 0;  //Test to see if we have done a begin...

}


void TwoWire3::end()
{
	if (!(SIM_SCGC1 & SIM_SCGC1_I2C3)) return;
	NVIC_DISABLE_IRQ(IRQ_I2C3);
	KINETIS_I2C3.C1 = 0;

	CORE_PIN56_CONFIG = 0;
	CORE_PIN57_CONFIG = 0;

	SIM_SCGC1 &= ~SIM_SCGC1_I2C3; // TODO: use bitband
	TwoWire::end();
}

void i2c3_isr(void)
{
	if (Wire3.isr(Wire3._pwires, &KINETIS_I2C3) ) {
		// Hack that mainline function says to set
		attachInterrupt(Wire3.sda_pin_num, TwoWire3::sda_rising_isr, RISING);
	}

}

// Detects the stop condition that terminates a slave receive transfer.
// Sadly, the I2C in Kinetis K series lacks the stop detect interrupt
// This pin change interrupt hack is needed to detect the stop condition
void TwoWire3::sda_rising_isr(void)
{
	//digitalWrite(3, HIGH);
	WIRE_STRUCT *pwires = Wire3._pwires;
	if (!(I2C3_S & I2C_S_BUSY)) {
		detachInterrupt(Wire3.sda_pin_num);
		if (pwires->user_onReceive != NULL) {
			pwires->rxBufferIndex = 0;
			pwires->user_onReceive(pwires->rxBufferLength);
		}
		//delayMicroseconds(100);
	} else {
		if (++pwires->irqcount >= 2 || !pwires->slave_mode) {
			detachInterrupt(Wire3.sda_pin_num);
		}
	}
	//digitalWrite(3, LOW);
}


//TwoWire Wire = TwoWire();
TwoWire3 Wire3;


#endif //  __MK66FX1M0__ 

