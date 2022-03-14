/**
 * Design Project 4 - Stepper Motor & I2C (part 1)
 * Author: Anthony Bruno
 *
 * Description: This program is meant to establish a working I2C serial transmission
 *              to an Adafruit Featherwing Motor Driver. This goal of this program
 *              is to collect I2C SDA and SCL samples with a logic analyzer. The
 *              Sampling rate is set for 1MHz and the number of samples taken is
 *              20M Samples. The I2C is set for a data rate of 400Kbps (Fast rate).
 *
 * STARTUP FILE EDITS:
 * - Line 120:      LDR   r10, = main
 * - Line 165:LDR   pc, [pc,#-8]
 *
 */

// Libraries and imports
#include <stdio.h>

// Register Access Macro
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

    I2C_INIT();         // Initialize the I2C1 Module
    I2CTransmit();      // Transmit data to Featherwing board

    return 0;           // Terminate Program
}

// __________________________ FUNCTIONS _____________________________________

void I2CTransmit(void)
{
    while((HWREG(I2C1BA + 0x24) & 0x1000) != 0x0){}   // Polling Bit 12 (BB) in I2C_IRQSTATUS_RAW
    HWREG(I2C1BA + 0x98) = 0x2;         // Setup the number of Data Bytes
    HWREG(I2C1BA + 0xA4) = 0x8603;      // Initiate a START/STOP condition, setup I2C as master & Transmitter

    HWREG(I2C1BA + 0x2C) = 0x10;    // Set XRDY_IE bit in I2C_IRQENABLE_SET
    HWREG(I2C1BA + 0x9C) = 0x00;    // PCA9685 Control Register (MODE1 Register)
    HWREG(I2C1BA + 0x2C) = 0x10;    // Set XRDY_IE bit in I2C_IRQENABLE_SET
    HWREG(I2C1BA + 0x9C) = 0x11;    // Data to MODE1 register
}

void I2C_INIT(void)
{
    HWREG(ControlBA + 0x95C) = 0x32;    // set pin 17 to I2C_SCL Mode
    HWREG(ControlBA + 0x958) = 0x32;    // set pin 18 to I2C_SDA Mode
    HWREG(CMPERWKUP + 0x48) = 0x2;      // Turn on I2C1 Clock Module
    HWREG(I2C1BA + 0xB0) = 0x3;         // Set Prescalar register to 12MHz
    HWREG(I2C1BA + 0xB4) = 0x8;         // setup of I2C_SCLL for 400Kbps
    HWREG(I2C1BA + 0xB8) = 0xA;         // setup of I2C_SCLH for 400Kbps
    HWREG(I2C1BA + 0xA8) = 0x0;         // setup I2C1 Own Address (Master)
    HWREG(I2C1BA + 0xA4) = 0x8000;      // Take the I2C1 out of reset
    HWREG(I2C1BA + 0xAC) = 0x60;        // Setup SLAVE Address
}
