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
#if defined (WIRE_DEFINE_WIRE0)
#if defined(__arm__) && defined(CORE_TEENSY)

#include "kinetis.h"
#include <string.h> // for memcpy
#include "core_pins.h"
//#include "HardwareSerial.h"
#include "Wire.h"

TwoWire0::TwoWire0() : TwoWire()
{
	sda_pin_num = A4;
	scl_pin_num = A5;
}

KINETIS_I2C_t *TwoWire0::kinetisk_i2c (void) {
    return &KINETIS_I2C0;
}


void TwoWire0::begin(void) 
{
	// Make sure we have our data structure allocated. 
	checkAndAllocateStruct();
	//serial_begin(BAUD2DIV(115200));
	//serial_print("\nWire Begin\n");

	_pwires->slave_mode = 0;
	SIM_SCGC4 |= SIM_SCGC4_I2C0; // TODO: use bitband
	I2C0_C1 = 0;
	// On Teensy 3.0 external pullup resistors *MUST* be used
	// the PORT_PCR_PE bit is ignored when in I2C mode
	// I2C will not work at all without pullup resistors
	// It might seem like setting PORT_PCR_PE & PORT_PCR_PS
	// would enable pullup resistors.  However, there seems
	// to be a bug in chip while I2C is enabled, where setting
	// those causes the port to be driven strongly high.
	if (sda_pin_num == 18) {
		CORE_PIN18_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (sda_pin_num == 17) {
		CORE_PIN17_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	} else if (sda_pin_num == 34) {
		CORE_PIN34_CONFIG = PORT_PCR_MUX(5)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (sda_pin_num == 8) {
		CORE_PIN8_CONFIG = PORT_PCR_MUX(7)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (sda_pin_num == 48) {
		CORE_PIN48_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#endif	
	}
	if (scl_pin_num == 19) {
		CORE_PIN19_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (scl_pin_num == 16) {
		CORE_PIN16_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	} else if (scl_pin_num == 33) {
		CORE_PIN33_CONFIG = PORT_PCR_MUX(5)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (scl_pin_num == 7) {
		CORE_PIN7_CONFIG = PORT_PCR_MUX(7)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
	} else if (scl_pin_num == 47) {
		CORE_PIN47_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#endif	
	}
	setClock(100000);
	I2C0_C2 = I2C_C2_HDRS;
	I2C0_C1 = I2C_C1_IICEN;
	//pinMode(3, OUTPUT);
	//pinMode(4, OUTPUT);
}


void TwoWire0::setSDA(uint8_t pin)
{
	if (pin == sda_pin_num) return;
	if ((SIM_SCGC4 & SIM_SCGC4_I2C0)) {
		if (sda_pin_num == 18) {
			CORE_PIN18_CONFIG = 0;
		} else if (sda_pin_num == 17) {
			CORE_PIN17_CONFIG = 0;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		} else if (sda_pin_num == 34) {
			CORE_PIN34_CONFIG = 0;
		} else if (sda_pin_num == 8) {
			CORE_PIN8_CONFIG = 0;
		} else if (sda_pin_num == 48) {
			CORE_PIN48_CONFIG = 0;
#endif	
		}

		if (pin == 18) {
			CORE_PIN18_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 17) {
			CORE_PIN17_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		} else if (pin == 34) {
			CORE_PIN34_CONFIG = PORT_PCR_MUX(5)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 8) {
			CORE_PIN8_CONFIG = PORT_PCR_MUX(7)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 48) {
			CORE_PIN48_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#endif	
		}
	}
	sda_pin_num = pin;
}

void TwoWire0::setSCL(uint8_t pin)
{
	if (pin == scl_pin_num) return;
	if ((SIM_SCGC4 & SIM_SCGC4_I2C0)) {
		if (scl_pin_num == 19) {
			CORE_PIN19_CONFIG = 0;
		} else if (scl_pin_num == 16) {
			CORE_PIN16_CONFIG = 0;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		} else if (scl_pin_num == 33) {
			CORE_PIN33_CONFIG = 0;
		} else if (scl_pin_num == 7) {
			CORE_PIN7_CONFIG = 0;
		} else if (scl_pin_num == 47) {
			CORE_PIN47_CONFIG = 0;
#endif	
		}

		if (pin == 19) {
			CORE_PIN19_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 16) {
			CORE_PIN16_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		} else if (pin == 33) {
			CORE_PIN33_CONFIG = PORT_PCR_MUX(5)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 7) {
			CORE_PIN7_CONFIG = PORT_PCR_MUX(7)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
		} else if (pin == 47) {
			CORE_PIN47_CONFIG = PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE;
#endif	
		}
	}
	scl_pin_num = pin;
}

void TwoWire0::begin(uint8_t address)
{
	begin();
	I2C0_A1 = address << 1;
	_pwires->slave_mode = 1;
	I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE;
	NVIC_ENABLE_IRQ(IRQ_I2C0);
}

uint8_t TwoWire0::checkSIM_SCG()
{
	return ( SIM_SCGC4 & SIM_SCGC4_I2C0)? 1 : 0;  //Test to see if we have done a begin...

}

void TwoWire0::end()
{
	if (!(SIM_SCGC4 & SIM_SCGC4_I2C0)) return;
	NVIC_DISABLE_IRQ(IRQ_I2C0);
	I2C0_C1 = 0;
	if (sda_pin_num == 18) {
		CORE_PIN18_CONFIG = 0;
	} else if (sda_pin_num == 17) {
		CORE_PIN17_CONFIG = 0;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	} else if (sda_pin_num == 34) {
		CORE_PIN34_CONFIG = 0;
	} else if (sda_pin_num == 8) {
		CORE_PIN8_CONFIG = 0;
	} else if (sda_pin_num == 48) {
		CORE_PIN48_CONFIG = 0;
#endif	
	}
	if (scl_pin_num == 19) {
		CORE_PIN19_CONFIG = 0;
	} else if (scl_pin_num == 16) {
		CORE_PIN16_CONFIG = 0;
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	} else if (scl_pin_num == 33) {
		CORE_PIN33_CONFIG = 0;
	} else if (scl_pin_num == 7) {
		CORE_PIN7_CONFIG = 0;
	} else if (scl_pin_num == 47) {
		CORE_PIN47_CONFIG = 0;
#endif	
	}
	SIM_SCGC4 &= ~SIM_SCGC4_I2C0; // TODO: use bitband
	TwoWire::end();
}

void i2c0_isr(void)
{
	if (Wire.isr(Wire._pwires, &KINETIS_I2C0 )) {
		// Hack that mainline function says to set
		attachInterrupt(Wire.sda_pin_num, TwoWire0::sda_rising_isr, RISING);
	}

}

// Detects the stop condition that terminates a slave receive transfer.
// Sadly, the I2C in Kinetis K series lacks the stop detect interrupt
// This pin change interrupt hack is needed to detect the stop condition
void TwoWire0::sda_rising_isr(void)
{
	WIRE_STRUCT *pwires = Wire._pwires;
	if (!(I2C0_S & I2C_S_BUSY)) {
		detachInterrupt(Wire.sda_pin_num);
		if (pwires->user_onReceive != NULL) {
			pwires->rxBufferIndex = 0;
			pwires->user_onReceive(pwires->rxBufferLength);
		}
		//delayMicroseconds(100);
	} else {
		if (++pwires->irqcount >= 2 || !pwires->slave_mode) {
			detachInterrupt(Wire.sda_pin_num);
		}
	}
}



//TwoWire Wire = TwoWire();
TwoWire0 Wire;


#endif // __MK20DX128__ || __MK20DX256__



#if defined(__AVR__)

extern "C" {
  #include <stdlib.h>
  #include <string.h>
  #include <inttypes.h>
  #include "twi.h"
}


// Initialize Class Variables //////////////////////////////////////////////////

uint8_t TwoWire::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire::rxBufferIndex = 0;
uint8_t TwoWire::rxBufferLength = 0;

uint8_t TwoWire::txAddress = 0;
uint8_t TwoWire::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire::txBufferIndex = 0;
uint8_t TwoWire::txBufferLength = 0;

uint8_t TwoWire::transmitting = 0;
void (*TwoWire::user_onRequest)(void);
void (*TwoWire::user_onReceive)(int);

// Constructors ////////////////////////////////////////////////////////////////

TwoWire::TwoWire()
{
}

// Public Methods //////////////////////////////////////////////////////////////

void TwoWire::begin(void)
{
  rxBufferIndex = 0;
  rxBufferLength = 0;

  txBufferIndex = 0;
  txBufferLength = 0;

  twi_init();
}

void TwoWire::begin(uint8_t address)
{
  twi_setAddress(address);
  twi_attachSlaveTxEvent(onRequestService);
  twi_attachSlaveRxEvent(onReceiveService);
  begin();
}

void TwoWire::begin(int address)
{
  begin((uint8_t)address);
}

void TwoWire::end()
{
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));
  digitalWrite(SDA, 0);
  digitalWrite(SCL, 0);
}

void TwoWire::setClock(uint32_t frequency)
{
  TWBR = ((F_CPU / frequency) - 16) / 2;
}

void TwoWire::setSDA(uint8_t pin)
{
}

void TwoWire::setSCL(uint8_t pin)
{
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
  // clamp to buffer length
  if(quantity > BUFFER_LENGTH){
    quantity = BUFFER_LENGTH;
  }
  // perform blocking read into buffer
  uint8_t read = twi_readFrom(address, rxBuffer, quantity, sendStop);
  // set rx buffer iterator vars
  rxBufferIndex = 0;
  rxBufferLength = read;

  return read;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

void TwoWire::beginTransmission(uint8_t address)
{
  // indicate that we are transmitting
  transmitting = 1;
  // set address of targeted slave
  txAddress = address;
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
}

void TwoWire::beginTransmission(int address)
{
  beginTransmission((uint8_t)address);
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
  // transmit buffer (blocking)
  int8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, 1, sendStop);
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
  // indicate that we are done transmitting
  transmitting = 0;
  return ret;
}

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void)
{
  return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data)
{
  if(transmitting){
  // in master transmitter mode
    // don't bother if buffer is full
    if(txBufferLength >= BUFFER_LENGTH){
      setWriteError();
      return 0;
    }
    // put byte in tx buffer
    txBuffer[txBufferIndex] = data;
    ++txBufferIndex;
    // update amount in buffer   
    txBufferLength = txBufferIndex;
  }else{
  // in slave send mode
    // reply to master
    twi_transmit(&data, 1);
  }
  return 1;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
  if(transmitting){
  // in master transmitter mode
    for(size_t i = 0; i < quantity; ++i){
      write(data[i]);
    }
  }else{
  // in slave send mode
    // reply to master
    twi_transmit(data, quantity);
  }
  return quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::available(void)
{
  return rxBufferLength - rxBufferIndex;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void)
{
  int value = -1;
  
  // get each successive byte on each call
  if(rxBufferIndex < rxBufferLength){
    value = rxBuffer[rxBufferIndex];
    ++rxBufferIndex;
  }

  return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void)
{
  int value = -1;
  
  if(rxBufferIndex < rxBufferLength){
    value = rxBuffer[rxBufferIndex];
  }

  return value;
}

void TwoWire::flush(void)
{
  // XXX: to be implemented.
}

// behind the scenes function that is called when data is received
void TwoWire::onReceiveService(uint8_t* inBytes, int numBytes)
{
  // don't bother if user hasn't registered a callback
  if(!user_onReceive){
    return;
  }
  // don't bother if rx buffer is in use by a master requestFrom() op
  // i know this drops data, but it allows for slight stupidity
  // meaning, they may not have read all the master requestFrom() data yet
  if(rxBufferIndex < rxBufferLength){
    return;
  }
  // copy twi rx buffer into local read buffer
  // this enables new reads to happen in parallel
  for(uint8_t i = 0; i < numBytes; ++i){
    rxBuffer[i] = inBytes[i];    
  }
  // set rx iterator vars
  rxBufferIndex = 0;
  rxBufferLength = numBytes;
  // alert user program
  user_onReceive(numBytes);
}

// behind the scenes function that is called when data is requested
void TwoWire::onRequestService(void)
{
  // don't bother if user hasn't registered a callback
  if(!user_onRequest){
    return;
  }
  // reset tx buffer iterator vars
  // !!! this will kill any pending pre-master sendTo() activity
  txBufferIndex = 0;
  txBufferLength = 0;
  // alert user program
  user_onRequest();
}

// sets function called on slave write
void TwoWire::onReceive( void (*function)(int) )
{
  user_onReceive = function;
}

// sets function called on slave read
void TwoWire::onRequest( void (*function)(void) )
{
  user_onRequest = function;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire Wire = TwoWire();

#endif // __AVR__

#endif // WIRE_DEFINE_WIRE0