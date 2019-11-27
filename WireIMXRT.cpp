
#include "Wire.h"


#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

//#include "debug/printf.h"
//#define DEBUG_PRINTS
#ifdef DEBUG_PRINTS
#define DBGPrintf Serial.printf
#else
void inline DBGPrintf(...) {};
#endif
void TwoWire::begin(void)
{
	//use 60Mhz clock 
	CCM_CSCDR2 = (CCM_CSCDR2 & ~CCM_CSCDR2_LPI2C_CLK_PODF(63)); 
	CCM_CCSR = 0;

	hardware.clock_gate_register |= hardware.clock_gate_mask;
    port->MCR = LPI2C_MCR_RST;
    setClock(100000);

    // Setup SDA register
	//*(portControlRegister(hardware.sda_pins[sda_pin_index_].pin)) |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
	*(portControlRegister(hardware.sda_pins[sda_pin_index_].pin)) = IOMUXC_PAD_ODE |IOMUXC_PAD_SRE |  IOMUXC_PAD_DSE(5)| IOMUXC_PAD_SPEED(1);
	*(portConfigRegister(hardware.sda_pins[sda_pin_index_].pin)) = hardware.sda_pins[sda_pin_index_].mux_val;
	if (hardware.sda_pins[sda_pin_index_].select_input_register) {
	 	*(hardware.sda_pins[sda_pin_index_].select_input_register) =  hardware.sda_pins[sda_pin_index_].select_val;		
	}	

	// setup SCL register
	//*(portControlRegister(hardware.scl_pins[scl_pin_index_].pin)) |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
	*(portControlRegister(hardware.scl_pins[scl_pin_index_].pin)) = IOMUXC_PAD_ODE |IOMUXC_PAD_SRE |  IOMUXC_PAD_DSE(5)| IOMUXC_PAD_SPEED(1);
	*(portConfigRegister(hardware.scl_pins[scl_pin_index_].pin)) = hardware.scl_pins[scl_pin_index_].mux_val;
	if (hardware.scl_pins[scl_pin_index_].select_input_register) {
	 	*(hardware.scl_pins[scl_pin_index_].select_input_register) =  hardware.scl_pins[scl_pin_index_].select_val;		
	}	
}

void TwoWire::begin(uint8_t address)
{
	// TODO: slave mode
}

void TwoWire::end()
{
}

void TwoWire::setSDA(uint8_t pin) {
	if (pin == hardware.sda_pins[sda_pin_index_].pin) return;
	uint32_t newindex=0;
	while (1) {
		uint32_t sda_pin = hardware.sda_pins[newindex].pin;
		if (sda_pin == 255) return;
		if (sda_pin == pin) break;
		if (++newindex >= sizeof(hardware.sda_pins)) return;
	}
	if ((hardware.clock_gate_register & hardware.clock_gate_mask)) {
		*(portConfigRegister(hardware.sda_pins[sda_pin_index_].pin)) = 5;	// hard to know what to go back to?

		// setup new one...
		*(portControlRegister(hardware.sda_pins[newindex].pin)) |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
		*(portConfigRegister(hardware.sda_pins[newindex].pin)) = hardware.sda_pins[newindex].mux_val;
		if (hardware.sda_pins[newindex].select_input_register) {
		 	*(hardware.sda_pins[newindex].select_input_register) =  hardware.sda_pins[newindex].select_val;		
		}	

	}
	sda_pin_index_ = newindex;

}

void TwoWire::setSCL(uint8_t pin) {
	if (pin == hardware.scl_pins[scl_pin_index_].pin) return;
	uint32_t newindex=0;
	while (1) {
		uint32_t scl_pin = hardware.scl_pins[newindex].pin;
		if (scl_pin == 255) return;
		if (scl_pin == pin) break;
		if (++newindex >= sizeof(hardware.scl_pins)) return;
	}
	if ((hardware.clock_gate_register & hardware.clock_gate_mask)) {
		*(portConfigRegister(hardware.scl_pins[scl_pin_index_].pin)) = 5;	// hard to know what to go back to?

		// setup new one...
		*(portControlRegister(hardware.scl_pins[newindex].pin)) |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
		*(portConfigRegister(hardware.scl_pins[newindex].pin)) = hardware.scl_pins[newindex].mux_val;
		if (hardware.scl_pins[newindex].select_input_register) {
		 	*(hardware.scl_pins[newindex].select_input_register) =  hardware.scl_pins[newindex].select_val;		
		}	

	}
	scl_pin_index_ = newindex;

}

size_t TwoWire::write(uint8_t data)
{
	if (transmitting || slave_mode) {
		if (txBufferLength >= BUFFER_LENGTH+1) {
			setWriteError();
			return 0;
		}
		txBuffer[txBufferLength++] = data;
		return 1;
	}
	return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
	if (transmitting || slave_mode) {
		size_t avail = BUFFER_LENGTH+1 - txBufferLength;
		if (quantity > avail) {
			quantity = avail;
			setWriteError();
		}
		memcpy(txBuffer + txBufferLength, data, quantity);
		txBufferLength += quantity;
		return quantity;
	}
	return 0;
}

uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
	//printf("\nendTransmission %d (%x %x %x) %x\n", txBufferLength,txBuffer[0], txBuffer[1], txBuffer[2], sendStop);
	uint32_t i=0, len, status;

	len = txBufferLength;
	if (!len) return 4; // no data to transmit

	// wait while bus is busy
	while (1) {
		status = port->MSR; // pg 2899 & 2892
		if (!(status & LPI2C_MSR_BBF)) break; // bus is available
		if (status & LPI2C_MSR_MBF) break; // we already have bus control
		// TODO: timeout...
	}
	DBGPrintf("EndTrans=%x\n", status);

	// Wonder if MFSR we should maybe clear it?
	if ( (port->MFSR & 0x7) && 
			((port->MSR & (LPI2C_MSR_BBF|LPI2C_MSR_MBF)) != (LPI2C_MSR_BBF|LPI2C_MSR_MBF))) {
		port->MCR = LPI2C_MCR_MEN | LPI2C_MCR_RTF;  // clear the FIFO
		port->MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF | LPI2C_MSR_FEF; // clear flags
		DBGPrintf("Clear TX Fifo %lx %lx\n", port->MSR, port->MFSR);
	}
	// TODO: is this correct if the prior use didn't send stop?
	//port->MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF; // clear flags
	port->MSR = status;

	DBGPrintf("MSR=%lX, MFSR=%lX\n", status, port->MFSR);
	elapsedMillis timeout=0;

	while (1) {
		// transmit stuff, if we haven't already
		if (i <= len) {
			uint32_t fifo_used = port->MFSR & 0x07; // pg 2914
			if (fifo_used < 4) DBGPrintf("i=%d t=%ld\n", i, fifo_used);
			while (fifo_used < 4) {
				if (i == 0) {
					//printf("start %x\n", txBuffer[0]);
					port->MTDR = LPI2C_MTDR_CMD_START | txBuffer[0];
					i = 1;
				} else if (i < len) {
					port->MTDR = LPI2C_MTDR_CMD_TRANSMIT | txBuffer[i++];
				} else {
					if (sendStop) port->MTDR = LPI2C_MTDR_CMD_STOP;
					i++;
					break;
				}
				fifo_used = fifo_used + 1;
			}
		}
		// monitor status
		status = port->MSR; // pg 2899 & 2892
		if (status & LPI2C_MSR_ALF) {
			DBGPrintf("arbitration lost\n");
			return 4; // we lost bus arbitration to another master
		}
		if (status & LPI2C_MSR_NDF) {
			DBGPrintf("NACK, f=%d, i=%d\n", port->MFSR & 0x07, i);
			// TODO: check that hardware really sends stop automatically
			port->MCR = LPI2C_MCR_MEN | LPI2C_MCR_RTF;  // clear the FIFO
			// TODO: is always sending a stop the right way to recover?
			port->MTDR = LPI2C_MTDR_CMD_STOP;
			return 2; // NACK for address
			//return 3; // NACK for data TODO: how to discern addr from data?
		}
		//if (status & LPI2C_MSR_PLTF) {
			//printf("bus stuck - what to do?\n");
			//return 4;
		//}

		if (timeout > 100) {
			DBGPrintf("status = %x\n", status);
			timeout = 0;
		}

		if (sendStop) {
			if (status & LPI2C_MSR_SDF) {
				// master automatically sends stop condition on some
				// types of errors, so this flag only means success
				// when all comments in fifo have been fully used
				uint32_t fifo = port->MFSR & 0x07;
				if (fifo == 0)  {
					return 0;
				}
			}
		} else {
			uint32_t fifo_used = port->MFSR & 0x07; // pg 2914
			if (fifo_used == 0) {
				//digitalWriteFast(15, HIGH);
				//delayMicroseconds(2);
				//digitalWriteFast(15, LOW);
				// TODO: this returns before the last data transmits!
				// Should verify every byte ACKs, arbitration not lost
				//printf("fifo empty, msr=%x\n", status);
				return 0;
			}
		}
	}
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t length, uint8_t sendStop)
{
	uint32_t cmd=0, status, fifo;

	// wait while bus is busy
	DBGPrintf("\nrequestFrom %x %x %x\n", address, length, sendStop);

	while (1) {
		status = port->MSR; // pg 2899 & 2892
		if (!(status & LPI2C_MSR_BBF)) break; // bus is available
		if (status & LPI2C_MSR_MBF) break; // we already have bus control
		// TODO: timeout...
	}
	DBGPrintf("idle2, msr=%x\n", status);

	// TODO: is this correct if the prior use didn't send stop?
	port->MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF | LPI2C_MSR_FEF; // clear flags

	DBGPrintf("MSR=%lX, MCR:%lx, MFSR=%lX\n", status, port->MCR, port->MFSR); Serial.flush();

	// Wonder if MFSR we should maybe clear it?
	if ( (port->MFSR & 0x7) && 
			((port->MSR & (LPI2C_MSR_BBF|LPI2C_MSR_MBF)) != (LPI2C_MSR_BBF|LPI2C_MSR_MBF))) {
		port->MCR = LPI2C_MCR_MEN | LPI2C_MCR_RTF;  // clear the FIFO
		port->MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF | LPI2C_MSR_FEF; // clear flags
		DBGPrintf("Clear TX Fifo %lx %lx\n", port->MSR, port->MFSR); Serial.flush();
	}
	address = (address & 0x7F) << 1;
	if (length < 1) length = 1;
	if (length > 255) length = 255;
	rxBufferIndex = 0;
	rxBufferLength = 0;

	//elapsedMillis timeout=0;

	while (1) {
		// transmit stuff, if we haven't already
		if (cmd < 3) {
			fifo = port->MFSR & 0x07; // pg 2914
			//if (fifo < 4) printf("t=%ld\n", fifo);
			while (fifo < 4 && cmd < 3) {
				if (cmd == 0) {
					//if ((port->MSR & (LPI2C_MSR_BBF|LPI2C_MSR_MBF)) == (LPI2C_MSR_BBF|LPI2C_MSR_MBF)) {
					//	DBGPrintf("BBF|MBF : ")
					//}
					port->MTDR = LPI2C_MTDR_CMD_START | 1 | address;
				} else if (cmd == 1) {
					// causes bus stuck... need way to recover
					//port->MTDR = LPI2C_MTDR_CMD_START | (length - 1);
					port->MTDR = LPI2C_MTDR_CMD_RECEIVE | (length - 1);
				} else {
					if (sendStop) port->MTDR = LPI2C_MTDR_CMD_STOP;
				}
				cmd++;
				fifo = fifo + 1;
			}
		}
		// receive stuff
		if (rxBufferLength < sizeof(rxBuffer)) {
			fifo = (port->MFSR >> 16) & 0x07;
			//if (fifo > 0) printf("r=%ld\n", fifo);
			while (fifo > 0 && rxBufferLength < sizeof(rxBuffer)) {
				rxBuffer[rxBufferLength++] = port->MRDR;
				fifo = fifo - 1;
			}
		}
		// monitor status
		status = port->MSR; // pg 2899 & 2892
		if (status & LPI2C_MSR_ALF) {
			DBGPrintf("arbitration lost\n");
			break;
		}
		if (status & LPI2C_MSR_NDF) {
			DBGPrintf("got NACK\n");
			// TODO: how to make sure stop is sent?
			break;
		}

		//if (timeout > 250) {
			//printf("Status = %x\n", status);
			//timeout = 0;
		//}

		if (rxBufferLength >= length && cmd >= 3) break;
	}
	//digitalWriteFast(15, HIGH);
	//delayMicroseconds(2);
	//digitalWriteFast(15, LOW);
	return rxBufferLength;
}

uint8_t TwoWire::requestFrom(uint8_t addr, uint8_t qty, uint32_t iaddr, uint8_t n, uint8_t stop)
{
	if (n > 0) {
		union { uint32_t ul; uint8_t b[4]; } iaddress;
		iaddress.ul = iaddr;
		beginTransmission(addr);
		if (n > 3) n = 3;
		do {
			n = n - 1;
			write(iaddress.b[n]);
		} while (n > 0);
		endTransmission(false);
	}
	if (qty > BUFFER_LENGTH) qty = BUFFER_LENGTH;
	return requestFrom(addr, qty, stop);
}



PROGMEM
constexpr TwoWire::I2C_Hardware_t TwoWire::i2c1_hardware = {
        CCM_CCGR2, CCM_CCGR2_LPI2C1(CCM_CCGR_ON),
		{{18, 3 | 0x10, &IOMUXC_LPI2C1_SDA_SELECT_INPUT, 1}, {0xff, 0xff, nullptr, 0}},
		{{19, 3 | 0x10, &IOMUXC_LPI2C1_SCL_SELECT_INPUT, 1}, {0xff, 0xff, nullptr, 0}},
        IRQ_LPI2C1
};
TwoWire Wire(&IMXRT_LPI2C1, TwoWire::i2c1_hardware);

PROGMEM
constexpr TwoWire::I2C_Hardware_t TwoWire::i2c3_hardware = {
        CCM_CCGR2, CCM_CCGR2_LPI2C3(CCM_CCGR_ON),
		{{17, 1 | 0x10, &IOMUXC_LPI2C3_SDA_SELECT_INPUT, 2}, {36, 2 | 0x10, &IOMUXC_LPI2C3_SDA_SELECT_INPUT, 1}},
		{{16, 1 | 0x10, &IOMUXC_LPI2C3_SCL_SELECT_INPUT, 2}, {37, 2 | 0x10, &IOMUXC_LPI2C3_SCL_SELECT_INPUT, 1}},
        IRQ_LPI2C3
};
TwoWire Wire1(&IMXRT_LPI2C3, TwoWire::i2c3_hardware);

PROGMEM
constexpr TwoWire::I2C_Hardware_t TwoWire::i2c4_hardware = {
        CCM_CCGR6, CCM_CCGR6_LPI2C4_SERIAL(CCM_CCGR_ON),
		{{25, 0 | 0x10, &IOMUXC_LPI2C4_SDA_SELECT_INPUT, 1}, {0xff, 0xff, nullptr, 0}},
		{{24, 0 | 0x10, &IOMUXC_LPI2C4_SCL_SELECT_INPUT, 1}, {0xff, 0xff, nullptr, 0}},
        IRQ_LPI2C4
};
TwoWire Wire2(&IMXRT_LPI2C4, TwoWire::i2c4_hardware);





void TwoWire::setClock(uint32_t frequency)
{
        port->MCR = 0;
	if (frequency < 400000) {
		// 100 kHz
		port->MCCR0 = LPI2C_MCCR0_CLKHI(55) | LPI2C_MCCR0_CLKLO(59) |
			LPI2C_MCCR0_DATAVD(25) | LPI2C_MCCR0_SETHOLD(40);
		port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(2);
		port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(5) | LPI2C_MCFGR2_FILTSCL(5) |
			LPI2C_MCFGR2_BUSIDLE(3900);
	} else if (frequency < 1000000) {
		// 400 kHz
		port->MCCR0 = LPI2C_MCCR0_CLKHI(31) | LPI2C_MCCR0_CLKLO(40) |
			LPI2C_MCCR0_DATAVD(8) | LPI2C_MCCR0_SETHOLD(17);
		port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(1);
		port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(2) | LPI2C_MCFGR2_FILTSCL(2) |
			LPI2C_MCFGR2_BUSIDLE(3900);
	} else if (frequency < 3000000) {
		// 1 MHz
		port->MCCR0 = LPI2C_MCCR0_CLKHI(9) | LPI2C_MCCR0_CLKLO(18) |
			LPI2C_MCCR0_DATAVD(4) | LPI2C_MCCR0_SETHOLD(9);
		port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(1);
		port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(1) | LPI2C_MCFGR2_FILTSCL(1) |
			LPI2C_MCFGR2_BUSIDLE(3900);
	} else {
		// 3.33 MHz
		port->MCCR0 = LPI2C_MCCR0_CLKHI(2) | LPI2C_MCCR0_CLKLO(4) |
			LPI2C_MCCR0_DATAVD(1) | LPI2C_MCCR0_SETHOLD(4);
		port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(1);
		port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(1) | LPI2C_MCFGR2_FILTSCL(1) |
			LPI2C_MCFGR2_BUSIDLE(3900);
	}        port->MCCR1 = port->MCCR0;

    port->MCFGR0 = 0;
    port->MCFGR3 = LPI2C_MCFGR3_PINLOW(3900);
    port->MFCR = LPI2C_MFCR_RXWATER(1) | LPI2C_MFCR_TXWATER(1);
    port->MCR = LPI2C_MCR_MEN;
}

#endif
