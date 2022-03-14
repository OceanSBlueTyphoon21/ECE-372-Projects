/**
 * Design Project 4 - Stepper Motor & I2C (Part 2)
 * Author: Anthony Bruno
 *
 * Description: This program is an I2C Driver between the BeagleBone Black Rev C
 *              development platform and a PCA9685 Stepper Motor Driver Featherwing
 *              by Adafruit. The BBB is able generate I2C signals that commands the
 *              PCA9685 Driver Board to control a Stepper Motor. Specifically,
 *              this program will have the Stepper Motor step through 200 steps counter
 *              clockwise when a button is pressed. The Program can also be used with
 *              a logic analyzer to view the I2C signals generated.
  * STARTUP FILE EDITS:
 * - Line 120:      LDR   r10, = main2
 * - Line 165:      LDR   pc, =INT_DIRECTOR
 */

// import and header files or libraries
#include <stdio.h>

// Register Access Macro
#define HWREG(x) (*((volatile unsigned int *)(x)))

// Control Register Address
#define CONTROL_BA 0x44E10000

// GPIO module 1 addresses
#define GPIO1_CLKCTRL 0x44E000AC
#define GPIO1_CONTROL 0x4804C000

// INTC addresses
#define INTC_CONTROL 0x48200000

// DMTimer4 addresses
#define TIMER4_CLKCTRL 0x44E00088
#define TIMER4_CLKSEL  0x44E00510
#define TIMER4_CONTROL 0x48044000

// I2C Module 1 Addresses
#define I2C_CLKCTRL 0x44E00048
#define I2C_CONTROL 0x4802A000

// Function Declarations +++++++++
void INIT_MODS();
void WAIT_LOOP();
void IntMasterIRQEnable();
void INT_DIRECTOR();
void BUTTON_SVC();
void MOTOR_SEQ();
void DELAY();
void transmit();

// Global Variables ++++++++++++
int x;
volatile unsigned int SVC_STACK[100];
volatile unsigned int INT_STACK[100];
int button_count = 1;   // Button counter
int step_count = 0;     // Step Counter
int curr_step = 1;      // Current Step
int cyc = 5000;         // DELAY loop variable total

// ______________________________ MAIN ____________________________________________
int main(void){
    // Setup SVC & INT Stacks
    asm("LDR R13, =SVC_STACK");
    asm("ADD R13, R13, #0x1000");
    asm("CPS #0x12");
    asm("LDR R13, =INT_STACK");
    asm("ADD R13, R13, #0x1000");
    asm("CPS #0x13");

    INIT_MODS();    // Initialize the GPIO1, I2C1, DMTimer4, and INTC modules

    IntMasterIRQEnable(); // Enable IRQ input by clearing bit 7 in CPSR

    WAIT_LOOP(); // WAIT LOOP (For interrupts)

    return 0; // END OF PROGRAM
}

void WAIT_LOOP(void){
    while(1){ // Wait for button push interrupt or Timer4 overflow interrupt
    }
}

// ______________________________ FUNCTIONS ________________________________

void INT_DIRECTOR(void){
    if((HWREG(INTC_CONTROL + 0xF8)&0x4) == 0x0){      // If bit 2 = 0, Interrupt from DMTimer4
        MOTOR_SEQ();                                  // Go to MOTOR_SEQ() function

    }
    else{                   // Interrupt from Button push
        BUTTON_SVC();       // Go to BUTTON_SVC() function
    }

    // Return to WAIT_LOOP()
    asm("LDMFD SP!, {LR}");
    asm("LDMFD SP!, {LR}");
    asm("SUBS PC, LR, #4");
}

void BUTTON_SVC(void){
    HWREG(GPIO1_CONTROL + 0x2C) = 0x20000000;       // Clear GPIO1_29 Interrupt
    HWREG(INTC_CONTROL + 0x48) = 0x1;               // Clear NEWIRQ Bit in INTC

    if (button_count==1){     // Is the button count = 1
        step_count = 0;       // set step count to 0
        curr_step = 1;        // set current step to 1
        button_count = 2;     // Set button count to 1
        DELAY(1);             // Start DMTimer4
    }
}

void MOTOR_SEQ(void){
    HWREG(TIMER4_CONTROL + 0x28) = 0x7;    // Clear Timer4 interrupt
    HWREG(INTC_CONTROL + 0x48) = 0x1;      // Clear NEWIRQ bit in INTC

    if (step_count == 200){                // Has the stepper Motor Stepped through 200 Steps CCW?
        // Brake the motor
        transmit(0xFD, 0x10); DELAY(2);    // ALL_LED_OFF
        transmit(0xFD, 0x00); DELAY(2);    // ALL_LED_OFF (to re-enable the outputs of PCA9685)

        step_count = 0;     // Set step counter to 0
        button_count = 1;   // Set button counter to 1
        curr_step = 1;      // Set current step to 1
        return;             // Return back to WAIT_LOOP()
    }

    switch(curr_step)   // Determine the current step the Stepper Motor is on
    {
        case(1):
                curr_step = 2;  // Change Current Step of Stepper Motor to step 2
                transmit(0x17, 0x10); DELAY(2);
                transmit(0x13, 0x00); DELAY(2);
                transmit(0x1B, 0x10); DELAY(2);
                transmit(0x1F, 0x00); DELAY(2);
                break;

        case(2):
                curr_step = 3;
                transmit(0x17, 0x00); DELAY(2);
                transmit(0x13, 0x10); DELAY(2);
                transmit(0x1B, 0x10); DELAY(2);
                transmit(0x1F, 0x00); DELAY(2);
                break;

        case(3):
                curr_step = 4;
                transmit(0x17, 0x00); DELAY(2);
                transmit(0x13, 0x10); DELAY(2);
                transmit(0x1B, 0x00); DELAY(2);
                transmit(0x1F, 0x10); DELAY(2);
                break;

        case(4):
                curr_step = 1;
                transmit(0x17, 0x10); DELAY(2);
                transmit(0x13, 0x00); DELAY(2);
                transmit(0x1B, 0x00); DELAY(2);
                transmit(0x1F, 0x10); DELAY(2);
                break;

        default:
            break;
    }
    step_count = step_count + 1;    // Increment the step counter by 1
    DELAY(1);
}

void transmit(unsigned int i, unsigned int d){
    while((HWREG(I2C_CONTROL + 0x24) & 0x1000) != 0x0){} // Poll bit 12 (BB) in I2C_IRQSTATUS_RAW

    HWREG(I2C_CONTROL + 0x98) = 0x2;                     // Set the number of bytes to transfer.
    HWREG(I2C_CONTROL + 0xA4) = 0x8603;                  // Initiate a START/STOP condition
    HWREG(I2C_CONTROL + 0x2C) = 0x10;                    // Set XRDY_IE bit in I2C_IRQENABLE_SET
    HWREG(I2C_CONTROL + 0x9C) = i;                       // PCA9685 Control Address byte (i)
    HWREG(I2C_CONTROL + 0x2C) = 0x10;                    // Set XRDY_IE bit in I2C_IRQENABLE_SET
    HWREG(I2C_CONTROL + 0x9C) = d;                       // Data to write at address (d)
}

void DELAY(int c){
    switch(c)
    {
        case(1):
                HWREG(TIMER4_CONTROL + 0x3C) = 0xFFFFFFCA; // set timer for a delay loop of ~ 1ms
                HWREG(TIMER4_CONTROL + 0x38) = 0x1;        // Star the Timer4
                break;
        case(2):    // Delay loop of 5000
                cyc = 5000;
                while(cyc != 0){
                    cyc--;
                }
        default:
                break;
    }
}

void IntMasterIRQEnable(void){
    asm("MRS R3, CPSR");
    asm("BIC R3, #0x80");
    asm("MSR CPSR_c, R3");
}

void INIT_MODS(void){
    // GPIO1 Module Setup
    HWREG(GPIO1_CLKCTRL) = 0x2;                 // Turn on GPIO1 Module Clock
    HWREG(GPIO1_CONTROL + 0x14C) |= 0x20000000;  // Setup GPIO1_29 to detect falling edge (button push)
    HWREG(GPIO1_CONTROL + 0x34) = 0x20000000;  // Enable GPIO1_29 to assert POINTRPEND1
    // INTC Setup
    HWREG(INTC_CONTROL + 0x10) = 0x2;       // Reset the INTC
    HWREG(INTC_CONTROL + 0xE8) = 0x4;       // Enable GPIO1_29 interrupt
    HWREG(INTC_CONTROL + 0xC8) = 0x10000000; // Enable Timer4 interrupt
    // DMTimer4 Setup
    HWREG(TIMER4_CLKCTRL) = 0x2;            // Turn on the TIMER4 Clock
    HWREG(TIMER4_CLKSEL) = 0x2;             // Set ref. clk to 32.768kHz
    HWREG(TIMER4_CONTROL + 0x10) = 0x1;     // Reset Timer4
    HWREG(TIMER4_CONTROL + 0x28) = 0x7;     // Clear Timer4 Interrupt
    HWREG(TIMER4_CONTROL + 0x2C) = 0x2;     // Enable timer4 overflow interrupt
    // Initialize the I2C1 Module
    HWREG(CONTROL_BA + 0x95C) = 0x32;       // Set pin 17 to I2C_SCL mode
    HWREG(CONTROL_BA + 0x958) = 0x32;       // Set pin 18 to I2C_SDA mode
    HWREG(I2C_CLKCTRL) = 0x2;            // Turn on I2C module 1
    HWREG(I2C_CONTROL + 0xB0) = 0x3;     // Set Prescalar to 12MHz
    HWREG(I2C_CONTROL + 0xB4) = 0x8;     // Setup of I2C_SCLL for 400Kbps
    HWREG(I2C_CONTROL + 0xB8) = 0xA;     // Setup of I2C_SCLH for 400Kbps
    HWREG(I2C_CONTROL + 0xA8) = 0x0;     // Setup I2C Own Address of Master
    HWREG(I2C_CONTROL + 0xA4) = 0x8000;  // Take I2C out of reset, configure as master mode and transmitter mode
    HWREG(I2C_CONTROL + 0xAC) = 0x60;    // setup Slave device address (featherwing is 0x60 base/default)

    // PCA9685 Initalization
    transmit(0x00, 0x11); DELAY(2);  // Put the PCA9685 MODE 1 to sleep mode (needed for set the Prescale)
    transmit(0xFE, 0x05); DELAY(2);  // Setup Prescale for 1KHz , delay(2),
    transmit(0x00, 0x01); DELAY(2);  // Take MODE 1 of PCA9685 out of sleep mode
    transmit(0x01, 0x04); DELAY(2);  // Setup Mode 2 (output: Totem Pole, non-inverted), delay(2)
    // Setup of PWM outputs (always on in truth table)
    transmit(0xFD, 0x00); DELAY(2);  // ALL_LED_OFF zero'd
    transmit(0x0F, 0x10); DELAY(2);  // Set pwmA to HIGH (PWM2), delay(2)
    transmit(0x23, 0x10); DELAY(2);  // Set pwmB to HIGH (PWM7), delay(2)
}
