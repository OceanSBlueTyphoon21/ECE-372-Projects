

/**
 * Design Project 4 - Stepper Motor & I2C
 * Author: Anthony Bruno
 *
 * Description: .....
 */

// Define for register-value access

#include <stdio.h>

#define HWREG(x) (*((volatile unsigned int *)(x)))

// module CLKWKUPS
#define CMPERWKUP        0x44E00000      // clock wakeup BA

// Pin Controls BA
#define ControlBA        0x44E10000      // BA for control module

// Peripheral Base Addresses
#define I2C1BA           0x4802A000     // Base address for I2C1 Module

// Global Variables & Stacks
int x;

volatile unsigned int USR_STACK[100];
volatile unsigned int IRQ_STACK[100];

// Function Declarations
void I2C_INIT(void);
void I2CTransmit(void);

// ----------------------- MAIN -------------------------
int main(void)
{
    // Setup the SVC and IRQ Stacks
    asm("LDR R13, =USR_STACK");
    asm("ADD R13, R13, #0x100");
    asm("CPS #0x12");
    asm("LDR R13, =IRQ_STACK");
    asm("ADD R13, R13, #0x100");
    asm("CPS #0x13");

    // Initialize the I2C1 Module
    I2C_INIT();
    I2CTransmit();


	return 0;
}

// ----------------  Function definitions  -------------------

void I2CTransmit(void)
{
    while((HWREG(I2C1BA + 0x24) & 0x1000) != 0x0){}   // Polling Bit 12 (BB) in I2C_IRQSTATUS_RAW
    // Setup the number of Data bytes per transmission
    HWREG(I2C1BA + 0x98) = 0x1;
    HWREG(I2C1BA + 0xA4) = 0x8603;      // Initiate a START/STOP condition

    while((HWREG(I2C1BA + 0x24) & 0x10) != 0x10){} // Polling the XRDY bit
    HWREG(I2C1BA + 0x28) = 0x10;    //
    HWREG(I2C1BA + 0x9C) = 0xA9;    // Data to Send
}

void I2C_INIT(void)
{
    HWREG(ControlBA + 0x95C) = 0x32;    // set pin 17 to I2C_SCL Mode
    HWREG(ControlBA + 0x958) = 0x32;    // set pin 18 to I2C_SDA Mode

    // turn on the I2C1 module with Clock
    HWREG(CMPERWKUP + 0x48) = 0x2;

    // SW reset of the I2C1 Module
    //HWREG(I2C1BA + 0x10) = 0x2;

    // program the Prescalar register to get 12MHz for Clock to I2C1 module
    HWREG(I2C1BA + 0xB0) = 0x3;

    // Setup the I2C_SCL to obtain 400Kbps (using I2C_SCLL & _SCLH)
    HWREG(I2C1BA + 0xB4) = 0x8;      // setup of I2C_SCLL
    HWREG(I2C1BA + 0xB8) = 0xA;      // setup of I2C_SCLH

    // Setup the I2C Own Address (of the master)
    HWREG(I2C1BA + 0xA8) = 0x0;

    // Take the I2C1 out of reset, configure as master mode and transmitter mode
    HWREG(I2C1BA + 0xA4) = 0x8000;

    // Setup the slave address
    HWREG(I2C1BA + 0xAC) = 0x65;
}

