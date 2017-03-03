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

#if defined(WIRE_DEFINE_WIRE1) && (defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__MKL26Z64__))

#include "kinetis.h"
#include <string.h> // for memcpy
#include "core_pins.h"
//#include "HardwareSerial.h"
#include "Wire.h"


TwoWire1::TwoWire1() : TwoWire()
{

#if defined(__MK20DX256__) // T3.2
	sda_pin_num = 30;
	scl_pin_num = 29;
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)  // T3.5/3.6
	sda_pin_num = 38;
	scl_pin_num = 37;
#elif defined(__MKL26Z64__)
	sda_pin_num = 23;
	scl_pin_num = 22;
#endif
}

KINETIS_I2C_t *TwoWire1::kinetisk_i2c (void) {
    return &KINETIS_I2C1;
}


void TwoWire1::begin(void)
{
	//serial_begin(BAUD2DIV(115200));
	//serial_print("\nWire Begin\n");
	//Serial.printf("TwoWire1::Begin SCL: %d SDA: %d\n", scl_pin_num, sda_pin_num);

	// Make sure we have our data structure allocated. 
	checkAndAllocateStruct();

	_pwires->slave_mode = 0;
	SIM_SCGC4 |= SIM_SCGC4_I2C1; // TODO: use bitband

	KINETIS_I2C1.C1 = 0;
	// On Teensy 3.0 external pullup resistors *MUST* be used
	// the PORT_PCR_PE bit is ignored when in I2C mode
	// I2C will not work at all without pullup resistors
	// It might seem like setting PORT_PCR_PE & PORT_PCR_PS
	// would enable pullup resistors.  However, there seems
	// to be a bug in chip while I2C is enabled, where setting
	// those causes the port to be driven strongly high.
    //      Wire1: I2C_PINS_29_30 (3.1/3.2), I2C_PINS_22_23 (LC), I2C_PINS_37_38 (3.5/3.6)
#if defined(__MK20DX256__) // T3.2
	if (scl_pin_num == 29) {
		CORE_PIN29_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
	if (sda_pin_num == 30) {
		CORE_PIN30_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)  // T3.5/3.6
	if (scl_pin_num == 37) {
		CORE_PIN37_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else 	if (scl_pin_num == 59) {
		CORE_PIN59_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
	if (sda_pin_num == 38) {
		CORE_PIN38_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else 	if (sda_pin_num == 58) {
		CORE_PIN58_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
#elif defined(__MKL26Z64__)
	if (scl_pin_num == 22) {
		CORE_PIN22_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
	if (sda_pin_num == 23) {
		CORE_PIN23_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	}
#endif
	setClock(100000);
	KINETIS_I2C1.C2 = I2C_C2_HDRS;
	KINETIS_I2C1.C1 = I2C_C1_IICEN;
	//pinMode(3, OUTPUT);
	//pinMode(4, OUTPUT);
}


void TwoWire1::begin(uint8_t address)
{
	begin();
	KINETIS_I2C1.A1 = address << 1;
	_pwires->slave_mode = 1;
	KINETIS_I2C1.C1 = I2C_C1_IICEN | I2C_C1_IICIE;
	NVIC_ENABLE_IRQ(IRQ_I2C1);
}


void TwoWire1::setSDA(uint8_t pin)
{
	if (pin == sda_pin_num) return;

	if ((SIM_SCGC4 & SIM_SCGC4_I2C1)) {
		// currently only T3.5/T3.6 have multiple pins...
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)  // T3.5/3.6
		if (sda_pin_num == 38) {
			CORE_PIN38_CONFIG = 0;
		} else 	if (sda_pin_num == 58) {
			CORE_PIN58_CONFIG = 0;
		}

		if (pin == 38) {
			CORE_PIN38_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else 	if (pin == 58) {
			CORE_PIN58_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		}
#endif
	}
	sda_pin_num = pin;
}

void TwoWire1::setSCL(uint8_t pin)
{
	if (pin == scl_pin_num) return;
	if ((SIM_SCGC4 & SIM_SCGC4_I2C1)) {
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)  // T3.5/3.6
		if (scl_pin_num == 37) {
			CORE_PIN37_CONFIG = 0;
		} else 	if (scl_pin_num == 59) {
			CORE_PIN59_CONFIG = 0;
		}
		if (pin == 37) {
			CORE_PIN37_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else 	if (pin == 59) {
			CORE_PIN59_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		}
#endif
	}
	scl_pin_num = pin;
}

uint8_t TwoWire1::checkSIM_SCG()
{
	return ( SIM_SCGC4 & SIM_SCGC4_I2C1)? 1 : 0;  //Test to see if we have done a begin...

}


void TwoWire1::end()
{
	if (!(SIM_SCGC4 & SIM_SCGC4_I2C1)) return;
	NVIC_DISABLE_IRQ(IRQ_I2C1);
	KINETIS_I2C1.C1 = 0;
#if defined(__MK20DX256__) // T3.2
	if (scl_pin_num == 29) {
		CORE_PIN29_CONFIG = 0;
	}
	if (sda_pin_num == 30) {
		CORE_PIN30_CONFIG = 0;
	}
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)  // T3.5/3.6
	if (scl_pin_num == 37) {
		CORE_PIN37_CONFIG = 0;
	} else 	if (scl_pin_num == 59) {
		CORE_PIN59_CONFIG = 0;
	}
	if (sda_pin_num == 38) {
		CORE_PIN38_CONFIG = 0;
	} else 	if (sda_pin_num == 58) {
		CORE_PIN58_CONFIG = 0;
	}
#elif defined(__MKL26Z64__)
	if (scl_pin_num == 22) {
		CORE_PIN22_CONFIG = 0;
	}
	if (sda_pin_num == 23) {
		CORE_PIN23_CONFIG = 0;
	}
#endif
	SIM_SCGC4 &= ~SIM_SCGC4_I2C1; // TODO: use bitband
	TwoWire::end();

}

void i2c1_isr(void)
{
	if (Wire1.isr(Wire1._pwires, &KINETIS_I2C1) ) {

		// Hack that mainline function says to set
		attachInterrupt(Wire1.sda_pin_num, TwoWire1::sda_rising_isr, RISING);
	}

}

// Detects the stop condition that terminates a slave receive transfer.
// Sadly, the I2C in Kinetis K series lacks the stop detect interrupt
// This pin change interrupt hack is needed to detect the stop condition
void TwoWire1::sda_rising_isr(void)
{
	//digitalWrite(3, HIGH);
	WIRE_STRUCT *pwires = Wire1._pwires;
	if (!(I2C1_S & I2C_S_BUSY)) {
		detachInterrupt(Wire1.sda_pin_num);
		if (pwires->user_onReceive != NULL) {
			pwires->rxBufferIndex = 0;
			pwires->user_onReceive(pwires->rxBufferLength);
		}
		//delayMicroseconds(100);
	} else {
		if (++pwires->irqcount >= 2 || !pwires->slave_mode) {
			detachInterrupt(Wire1.sda_pin_num);
		}
	}
	//digitalWrite(3, LOW);
}


//TwoWire Wire = TwoWire();
TwoWire1 Wire1;


#endif // __MK20DX256__ || __MK64FX512__ || __MK66FX1M0__ || __MKL26Z64__

