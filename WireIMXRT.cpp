
#include "Wire.h"


#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

#include "debug/printf.h"

void i2c_setclock(uint32_t frequency);

void TwoWire::begin(void)
{
        // use 24 MHz clock
        CCM_CSCDR2 = (CCM_CSCDR2 & ~CCM_CSCDR2_LPI2C_CLK_PODF(63)) | CCM_CSCDR2_LPI2C_CLK_SEL;
        CCM_CCGR2 |= CCM_CCGR2_LPI2C1(CCM_CCGR_ON);
        LPI2C1_MCR = LPI2C_MCR_RST;
        i2c_setclock(100000);
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_00 = 3 | 0x10; // 18/A4  AD_B1_01  GPIO1.17  I2C1_SDA
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_01 = 3 | 0x10; // 19/A5  AD_B1_00  GPIO1.16  I2C1_SCL
        IOMUXC_LPI2C1_SCL_SELECT_INPUT = 1;
        IOMUXC_LPI2C1_SDA_SELECT_INPUT = 1;
        IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00 |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
        IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01 |= IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3);
}

void TwoWire::setClock(uint32_t frequency)
{
}

void TwoWire::setSDA(uint8_t pin)
{
}

void TwoWire::setSCL(uint8_t pin)
{
}

void TwoWire::begin(uint8_t address)
{
}

void TwoWire::end()
{
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
	uint32_t i=0, len, status;

	len = txBufferLength;
	if (!len) return 4; // no data to transmit

	// wait while bus is busy
	while (1) {
		status = LPI2C1_MSR; // pg 2899 & 2892
		if (!(LPI2C1_MSR & LPI2C_MSR_BBF)) break; // bus is available
		if (LPI2C1_MSR & LPI2C_MSR_MBF) break; // we already have bus control
		// TODO: timeout...
	}
	printf("idle\n");

	// TODO: is this correct if the prior use didn't send stop?
	LPI2C1_MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF; // clear flags

	printf("MSR=%lX, MFSR=%lX\n", status, LPI2C1_MFSR);

	while (1) {
		// transmit stuff, if we haven't already
		if (i <= len) {
			uint32_t fifo_used = LPI2C1_MFSR & 0x07; // pg 2914
			if (fifo_used < 4) printf("t=%ld\n", fifo_used);
			while (fifo_used < 4) {
				if (i == 0) {
					LPI2C1_MTDR = LPI2C_MTDR_CMD_START | (txBuffer[0] << 1);
					i = 1;
				} else if (i < len) {
					LPI2C1_MTDR = LPI2C_MTDR_CMD_TRANSMIT | txBuffer[i++];
				} else {
					if (sendStop) LPI2C1_MTDR = LPI2C_MTDR_CMD_STOP;
					i++;
					break;
				}
				fifo_used = fifo_used + 1;
			}
		}
		// monitor status
		status = LPI2C1_MSR; // pg 2899 & 2892
		if (status & LPI2C_MSR_ALF) {
			printf("arbitration lost\n");
			return 4; // we lost bus arbitration to another master
		}
		if (status & LPI2C_MSR_NDF) {
			printf("got NACK, fifo=%ld, i=%ld\n", LPI2C1_MFSR & 0x07, i);
			// TODO: check that hardware really sends stop automatically
			// TODO: do we need to clear the FIFO here?
			return 2; // NACK for address
			//return 3; // NACK for data TODO: how to discern addr from data?
		}
		//if (status & LPI2C_MSR_PLTF) {
			//printf("bus stuck - what to do?\n");
			//return 4;
		//}
		if (sendStop) {
			if (status & LPI2C_MSR_SDF) return 0;
		} else {
			uint32_t fifo_used = LPI2C1_MFSR & 0x07; // pg 2914
			if (fifo_used == 0) {
				//digitalWriteFast(15, HIGH);
				//delayMicroseconds(2);
				//digitalWriteFast(15, LOW);
				// TODO: this return before the data transmits!
				// Should verify every byte ACKs, arbitration not lost
				return 0;
			}
		}
	}
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t length, uint8_t sendStop)
{
	uint32_t cmd=0, status, fifo;
	// TODO: sendStop is currently ignored

	// wait while bus is busy
	while (1) {
		status = LPI2C1_MSR; // pg 2899 & 2892
		if (!(LPI2C1_MSR & LPI2C_MSR_BBF)) break; // bus is available
		if (LPI2C1_MSR & LPI2C_MSR_MBF) break; // we already have bus control
		// TODO: timeout...
	}
	printf("idle\n");

	// TODO: is this correct if the prior use didn't send stop?
	LPI2C1_MSR = LPI2C_MSR_PLTF | LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF; // clear flags

	//printf("MSR=%lX, MFSR=%lX\n", status, LPI2C1_MFSR);
	address = (address & 0x7F) << 1;
	if (length < 1) length = 1;
	if (length > 255) length = 255;
	rxBufferIndex = 0;
	rxBufferLength = 0;

	while (1) {
		// transmit stuff, if we haven't already
		if (cmd < 3) {
			fifo = LPI2C1_MFSR & 0x07; // pg 2914
			if (fifo < 4) printf("t=%ld\n", fifo);
			while (fifo < 4 && cmd < 3) {
				if (cmd == 0) {
					LPI2C1_MTDR = LPI2C_MTDR_CMD_START | 1 | address;
				} else if (cmd == 1) {
					// causes bus stuck... need way to recover
					//LPI2C1_MTDR = LPI2C_MTDR_CMD_START | (length - 1);
					LPI2C1_MTDR = LPI2C_MTDR_CMD_RECEIVE | (length - 1);
				} else {
					if (sendStop) LPI2C1_MTDR = LPI2C_MTDR_CMD_STOP;
				}
				cmd++;
				fifo = fifo + 1;
			}
		}
		// receive stuff
		if (rxBufferLength < sizeof(rxBuffer)) {
			fifo = (LPI2C1_MFSR >> 16) & 0x07;
			//if (fifo > 0) printf("r=%ld\n", fifo);
			while (fifo > 0 && rxBufferLength < sizeof(rxBuffer)) {
				rxBuffer[rxBufferLength++] = LPI2C1_MRDR;
				fifo = fifo - 1;
			}
		}
		// monitor status
		status = LPI2C1_MSR; // pg 2899 & 2892
		if (status & LPI2C_MSR_ALF) {
			printf("arbitration lost\n");
			break;
		}
		if (status & LPI2C_MSR_NDF) {
			printf("got NACK\n");
			// TODO: how to make sure stop is sent?
			break;
		}
		if (rxBufferLength >= length && cmd >= 3) break;
	}
	//digitalWriteFast(15, HIGH);
	//delayMicroseconds(2);
	//digitalWriteFast(15, LOW);
	return rxBufferLength;
}







constexpr TwoWire::I2C_Hardware_t TwoWire::i2c1_hardware = {
        CCM_CCGR2, CCM_CCGR2_LPI2C1(CCM_CCGR_ON),
        18, 17, 255, 255, 255,
        2, 2, 0, 0, 0,
        19, 16, 255, 255, 255,
        2, 2, 0, 0, 0,
        IRQ_LPI2C1
};

TwoWire Wire(0, TwoWire::i2c1_hardware);




void i2c_setclock(uint32_t frequency)
{
        // timing params, pg 2363
//SETHOLD: 2-63 
//CLKLO: 3-63  clock low time
//CLKHI: 1-63  clock high time
//DATAVD:
#if 0
        // 100 kHz using 24 MHz clock
        DIV = 2; 
        FILT = 5;  // filt 2.5
        CLOCK_PERIOD = (24 MHz / 100 kHz) / DIV = 120;
        RISETIME = 1; // depends on resistors
        LATENCY = (2 + FILT + RISETIME) / DIV = 4
        PRESCALE = log2(DIV) = 1; // div by 2
        CLKLO = CLOCK_PERIOD/2 - 1 = 59
        CLKHI = CLOCK_PERIOD/2 - LATENCY - 1 = 55
        DATAVD = CLKLO/3 - LATENCY = 15  // guesswork on /3
        SETHOLD = ??? = 36

        // 400 kHz using 24 MHz clock
        DIV = 1;
        FILT = 2;
        CLOCK_PERIOD = (24 MHz / 400 kHz) / DIV = 60
        RISETIME = 1; // depends on resistors
        LATENCY = (2 + FILT + RISETIME) / DIV = 3
        PRESCALE = log2(DIV) = 0
        CLKLO = CLOCK_PERIOD/2 - 1 = 29
        CLKHI = CLOCK_PERIOD/2 - LATENCY - 1 = 26
        DATAVD = CLKLO/3 - LATENCY = 7  // guesswork
        SETHOLD = ??? = 21

        // 1 MHz
        DIV = 1;
        FILT = 1;
        CLOCK_PERIOD = (24 MHz / 1 MHz) / DIV = 24
        RISETIME = 1; // depends on resistors
        LATENCY = (2 + FILT + RISETIME) / DIV = 2
        PRESCALE = log2(DIV) = 0
        CLKLO = CLOCK_PERIOD/2 - 1 = 29
        CLKHI = CLOCK_PERIOD/2 - LATENCY - 1 = 27
        DATAVD = CLKLO/3 - LATENCY = 7  // guesswork
        SETHOLD = ??? = 22

#endif
        LPI2C1_MCR = 0;
        LPI2C1_MCCR0 = LPI2C_MCCR0_CLKHI(55) | LPI2C_MCCR0_CLKLO(59) |
                LPI2C_MCCR0_DATAVD(25) | LPI2C_MCCR0_SETHOLD(40);
        LPI2C1_MCCR1 = LPI2C1_MCCR0;
        LPI2C1_MCFGR0 = 0;
        LPI2C1_MCFGR1 = /*LPI2C_MCFGR1_TIMECFG |*/ LPI2C_MCFGR1_PRESCALE(1);
        LPI2C1_MCFGR2 = LPI2C_MCFGR2_FILTSDA(5) | LPI2C_MCFGR2_FILTSCL(5) | LPI2C_MCFGR2_BUSIDLE(3900);
        LPI2C1_MCFGR3 = LPI2C_MCFGR3_PINLOW(3900);
        LPI2C1_MFCR = LPI2C_MFCR_RXWATER(1) | LPI2C_MFCR_TXWATER(1);
        LPI2C1_MCR = LPI2C_MCR_MEN;
}

#endif
