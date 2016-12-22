/*
  TwoWire.h - TWI/I2C library for Arduino & Wiring
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

#ifndef TwoWire_h
#define TwoWire_h

#include <inttypes.h>
#include "Arduino.h"

// You can choose which of these optional Wire objects to be created.  Note: they will only be created
// for those boards who support a particular Wire buss...
#define WIRE_DEFINE_WIRE0 
#define WIRE_DEFINE_WIRE1
#define WIRE_DEFINE_WIRE2
#define WIRE_DEFINE_WIRE3

#define BUFFER_LENGTH 32
#define WIRE_HAS_END 1

// If it is not ARM and CORE_TEENSY we will do default stuff...
#if defined(__arm__) && defined(CORE_TEENSY)
// Lets create a base class to see if we can share  code 
#ifndef WIRE_RX_BUFFER_LENGTH
#define WIRE_RX_BUFFER_LENGTH BUFFER_LENGTH
#endif

#ifndef WIRE_TX_BUFFER_LENGTH
#define WIRE_TX_BUFFER_LENGTH (BUFFER_LENGTH+1)
#endif

typedef struct wire_struct_{
    // Move all of the main data out of class and only allocate if the user calls begin...
    uint8_t rxBuffer[WIRE_RX_BUFFER_LENGTH];
    volatile uint8_t rxBufferIndex;
    volatile uint8_t rxBufferLength;

    uint8_t txAddress;
    uint8_t txBuffer[WIRE_TX_BUFFER_LENGTH];
    volatile uint8_t txBufferIndex;
    volatile uint8_t txBufferLength;
    uint8_t slave_mode;

    volatile uint8_t transmitting;
    volatile uint8_t receiving;          // Our we receiving...
    volatile uint8_t irqcount;

    void (*user_onRequest)(void);
    void (*user_onReceive)(int);

} WIRE_STRUCT;

class TwoWire: public Stream
{
  protected:
    struct wire_struct_ *_pwires;

    void onRequestService(void);
    void onReceiveService(uint8_t*, int);
    static void sda_rising_isr(void);
    uint8_t sda_pin_num;
    uint8_t scl_pin_num;
    static uint8_t isr(WIRE_STRUCT *pwires,  KINETIS_I2C_t * kinetisk_pi2c);			// Process each of the ISRs...

    inline uint8_t i2c_status(KINETIS_I2C_t  *kinetisk_pi2c);
	inline void i2c_wait(KINETIS_I2C_t  *kinetisk_pi2c);
    void checkAndAllocateStruct(void);
  public:
    TwoWire();
    virtual void begin() = 0;               // Alllocate structure
    virtual void begin(uint8_t) = 0;
    void begin(int);
    virtual void end();                     // Frees up data structures
    void setClock(uint32_t);
    virtual void setSDA(uint8_t) = 0;
    virtual void setSCL(uint8_t) = 0;
    virtual uint8_t checkSIM_SCG() = 0;
    void beginTransmission(uint8_t);
    void beginTransmission(int);
    uint8_t endTransmission(void);
    uint8_t endTransmission(uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
    uint8_t requestFrom(int, int);
    uint8_t requestFrom(int, int, int);
    void send(uint8_t *s, uint8_t n);
    void send(int n);
    void send(char *s);
    uint8_t receive(void);
    
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *, size_t);
    virtual int available(void);
    virtual int read(void);
    virtual int peek(void);
	virtual void flush(void);
    void onReceive( void (*)(int) );
    void onRequest( void (*)(void) );
    virtual KINETIS_I2C_t *kinetisk_i2c (void) = 0;
  
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write;
};

#if defined(WIRE_DEFINE_WIRE0) 
extern "C" void i2c0_isr(void);

class TwoWire0 : public TwoWire
{
  private:
    static void sda_rising_isr(void);
    friend void i2c0_isr(void);
  public:
    TwoWire0();
    virtual void begin();
    virtual void begin(uint8_t);
    virtual void end();
    virtual void setSDA(uint8_t);
    virtual void setSCL(uint8_t);
    virtual uint8_t checkSIM_SCG();
    virtual KINETIS_I2C_t *kinetisk_i2c (void);



    using TwoWire::write;
};

extern TwoWire0 Wire;
#endif

#else

// AVR version 
#if defined(WIRE_DEFINE_WIRE0) 
class TwoWire : public Stream
{
  private:
    static uint8_t rxBuffer[];
    static uint8_t rxBufferIndex;
    static uint8_t rxBufferLength;

    static uint8_t txAddress;
    static uint8_t txBuffer[];
    static uint8_t txBufferIndex;
    static uint8_t txBufferLength;

    static uint8_t transmitting;
    static void onRequestService(void);
    static void onReceiveService(uint8_t*, int);
    static void (*user_onRequest)(void);
    static void (*user_onReceive)(int);
    static void sda_rising_isr(void);
  public:
    TwoWire();
    void begin();
    void begin(uint8_t);
    void begin(int);
    void end();
    void setClock(uint32_t);
    void setSDA(uint8_t);
    void setSCL(uint8_t);
    void beginTransmission(uint8_t);
    void beginTransmission(int);
    uint8_t endTransmission(void);
    uint8_t endTransmission(uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
    uint8_t requestFrom(int, int);
    uint8_t requestFrom(int, int, int);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *, size_t);
    virtual int available(void);
    virtual int read(void);
    virtual int peek(void);
	virtual void flush(void);
    void onReceive( void (*)(int) );
    void onRequest( void (*)(void) );
  
#ifdef CORE_TEENSY
    // added by Teensyduino installer, for compatibility
    // with pre-1.0 sketches and libraries
    void send(uint8_t b)               { write(b); }
    void send(uint8_t *s, uint8_t n)   { write(s, n); }
    void send(int n)                   { write((uint8_t)n); }
    void send(char *s)                 { write(s); }
    uint8_t receive(void) {
        int c = read();
        if (c < 0) return 0;
        return c;
    }
#endif
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write;
};

extern TwoWire Wire;
#endif
#endif

#if defined(__arm__) && defined(CORE_TEENSY)
class TWBRemulation
{
public:
	inline TWBRemulation & operator = (int val) __attribute__((always_inline)) {
		if (val == 12 || val == ((F_CPU / 400000) - 16) / 2) { // 22, 52, 112
			I2C0_C1 = 0;
			#if F_BUS == 120000000
			I2C0_F = I2C_F_DIV288; // 416 kHz
			#elif F_BUS == 108000000
			I2C0_F = I2C_F_DIV256; // 422 kHz
			#elif F_BUS == 96000000
			I2C0_F = I2C_F_DIV240; // 400 kHz
			#elif F_BUS == 90000000
			I2C0_F = I2C_F_DIV224; // 402 kHz
			#elif F_BUS == 80000000
			I2C0_F = I2C_F_DIV192; // 416 kHz
			#elif F_BUS == 72000000
			I2C0_F = I2C_F_DIV192; // 375 kHz
			#elif F_BUS == 64000000
			I2C0_F = I2C_F_DIV160; // 400 kHz
			#elif F_BUS == 60000000
			I2C0_F = I2C_F_DIV144; // 416 kHz
			#elif F_BUS == 56000000
			I2C0_F = I2C_F_DIV144; // 389 kHz
			#elif F_BUS == 54000000
			I2C0_F = I2C_F_DIV128; // 422 kHz
			#elif F_BUS == 48000000
			I2C0_F = I2C_F_DIV112; // 400 kHz
			#elif F_BUS == 40000000
			I2C0_F = I2C_F_DIV96;  // 416 kHz
			#elif F_BUS == 36000000
			I2C0_F = I2C_F_DIV96;  // 375 kHz
			#elif F_BUS == 24000000
			I2C0_F = I2C_F_DIV64;  // 375 kHz
			#elif F_BUS == 16000000
			I2C0_F = I2C_F_DIV40;  // 400 kHz
			#elif F_BUS == 8000000
			I2C0_F = I2C_F_DIV20;  // 400 kHz
			#elif F_BUS == 4000000
			I2C0_F = I2C_F_DIV20;  // 200 kHz
			#elif F_BUS == 2000000
			I2C0_F = I2C_F_DIV20;  // 100 kHz
			#endif
			I2C0_C1 = I2C_C1_IICEN;
		} else if (val == 72 || val == ((F_CPU / 100000) - 16) / 2) { // 112, 232, 472
			I2C0_C1 = 0;
			#if F_BUS == 120000000
			I2C0_F = I2C_F_DIV1152; // 104 kHz
			#elif F_BUS == 108000000
			I2C0_F = I2C_F_DIV1024; // 105 kHz
			#elif F_BUS == 96000000
			I2C0_F = I2C_F_DIV960; // 100 kHz
			#elif F_BUS == 90000000
			I2C0_F = I2C_F_DIV896; // 100 kHz
			#elif F_BUS == 80000000
			I2C0_F = I2C_F_DIV768; // 104 kHz
			#elif F_BUS == 72000000
			I2C0_F = I2C_F_DIV640; // 112 kHz
			#elif F_BUS == 64000000
			I2C0_F = I2C_F_DIV640; // 100 kHz
			#elif F_BUS == 60000000
			I2C0_F = I2C_F_DIV576; // 104 kHz
			#elif F_BUS == 56000000
			I2C0_F = I2C_F_DIV512; // 109 kHz
			#elif F_BUS == 54000000
			I2C0_F = I2C_F_DIV512; // 105 kHz
			#elif F_BUS == 48000000
			I2C0_F = I2C_F_DIV480; // 100 kHz
			#elif F_BUS == 40000000
			I2C0_F = I2C_F_DIV384; // 104 kHz
			#elif F_BUS == 36000000
			I2C0_F = I2C_F_DIV320; // 113 kHz
			#elif F_BUS == 24000000
			I2C0_F = I2C_F_DIV240; // 100 kHz
			#elif F_BUS == 16000000
			I2C0_F = I2C_F_DIV160; // 100 kHz
			#elif F_BUS == 8000000
			I2C0_F = I2C_F_DIV80; // 100 kHz
			#elif F_BUS == 4000000
			I2C0_F = I2C_F_DIV40; // 100 kHz
			#elif F_BUS == 2000000
			I2C0_F = I2C_F_DIV20; // 100 kHz
			#endif
			I2C0_C1 = I2C_C1_IICEN;
		}
		return *this;
	}
	inline operator int () const __attribute__((always_inline)) {
		#if F_BUS == 120000000
		if (I2C0_F == I2C_F_DIV288) return 12;
		#elif F_BUS == 108000000
		if (I2C0_F == I2C_F_DIV256) return 12;
		#elif F_BUS == 96000000
		if (I2C0_F == I2C_F_DIV240) return 12;
		#elif F_BUS == 90000000
		if (I2C0_F == I2C_F_DIV224) return 12;
		#elif F_BUS == 80000000
		if (I2C0_F == I2C_F_DIV192) return 12;
		#elif F_BUS == 72000000
		if (I2C0_F == I2C_F_DIV192) return 12;
		#elif F_BUS == 64000000
		if (I2C0_F == I2C_F_DIV160) return 12;
		#elif F_BUS == 60000000
		if (I2C0_F == I2C_F_DIV144) return 12;
		#elif F_BUS == 56000000
		if (I2C0_F == I2C_F_DIV144) return 12;
		#elif F_BUS == 54000000
		if (I2C0_F == I2C_F_DIV128) return 12;
		#elif F_BUS == 48000000
		if (I2C0_F == I2C_F_DIV112) return 12;
		#elif F_BUS == 40000000
		if (I2C0_F == I2C_F_DIV96) return 12;
		#elif F_BUS == 36000000
		if (I2C0_F == I2C_F_DIV96) return 12;
		#elif F_BUS == 24000000
		if (I2C0_F == I2C_F_DIV64) return 12;
		#elif F_BUS == 16000000
		if (I2C0_F == I2C_F_DIV40) return 12;
		#elif F_BUS == 8000000
		if (I2C0_F == I2C_F_DIV20) return 12;
		#elif F_BUS == 4000000
		if (I2C0_F == I2C_F_DIV20) return 12;
		#endif
		return 72;
	}
};
extern TWBRemulation TWBR;
#endif


// T3.1, 3.2, 3.5, 3.6 and TLC all have WIRE1...
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__MKL26Z64__)

#if defined(WIRE_DEFINE_WIRE1)
extern "C" void i2c1_isr(void);

class TwoWire1: public TwoWire
{
  private:
    static void sda_rising_isr(void);
    friend void i2c1_isr(void);
  public:
    TwoWire1();
    virtual void begin();
    virtual void begin(uint8_t);
    virtual void end();
    virtual void setSDA(uint8_t);
    virtual void setSCL(uint8_t);
    virtual uint8_t checkSIM_SCG();
    virtual KINETIS_I2C_t *kinetisk_i2c (void);

    using TwoWire::write;
};

extern TwoWire1 Wire1;
#endif
#endif

// Teensy 3.5 and T3.6 also have Wire2
#if defined(WIRE_DEFINE_WIRE2) && (defined(__MK64FX512__) || defined(__MK66FX1M0__))
extern "C" void i2c2_isr(void);

class TwoWire2: public TwoWire
{
  private:
    static void sda_rising_isr(void);
    friend void i2c2_isr(void);
  public:
    TwoWire2();
    virtual void begin();
    virtual void begin(uint8_t);
    virtual void end();
    virtual void setSDA(uint8_t);
    virtual void setSCL(uint8_t);
    virtual uint8_t checkSIM_SCG();
    virtual KINETIS_I2C_t *kinetisk_i2c (void);

    using TwoWire::write;
};

extern TwoWire2 Wire2;
#endif

// Only T3.6 has Wire3
#if defined(WIRE_DEFINE_WIRE3) && defined(__MK66FX1M0__)
extern "C" void i2c3_isr(void);

class TwoWire3: public TwoWire
{
  private:
    static void sda_rising_isr(void);
    friend void i2c3_isr(void);
  public:
    TwoWire3();
    virtual void begin();
    virtual void begin(uint8_t);
    virtual void end();
    virtual void setSDA(uint8_t);
    virtual void setSCL(uint8_t);
    virtual uint8_t checkSIM_SCG();
    virtual KINETIS_I2C_t *kinetisk_i2c (void);

    using TwoWire::write;
};

extern TwoWire3 Wire3;
#endif


#endif

