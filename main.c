

/**
 * main.c
 * I2C Part 1
 */

// Define selection for beaglebone black Hardware
#define HWREG(x) (*((volatile unsigned int *) (x))) // reads the value at some address

// GPIO1 Definitions
#define GPIO1BA             0x4804C000  // Base address for GPIO1 Module
#define GPIO_SETDATA_OUT    0x194       // Offset for GPIO_SETDATAOUT register
#define GPIO_CLEARDATA_OUT  0x190       // Offset for GPIO_CLEARDATAOUT register

// INTC Definitions
#define INTCBA              0x48200000  // base address of INTC

// Timer5 Definitions
#define TIMER5_BA           0x48046000  // base address for Timer5

// Other Definitions
#define CLKWKUPS            0x44E00000  // Base address CM_PER Clocks
#define LIGHT_BITS          0x01E00000  // Value to light all the USR LEDs on the BBB

// --------------- FUNCTION DECLARATIONS -------------------
void IntMasterIRQEnable();      // Enables the IRQ bit in the CPSR
void int_handler();             // Determines where interrupt signal came from
void turnoff_leds();            // turns off the USR LEDs
void turnon_leds();             // turns on the USR LEDs
void timer5_int();              // toggling the LED state & starting the timer
void waitloop();                // loop for waiting for an interrupt
void returnfromint();           // ????? NOT USED

// Global Variables
int current_state = 1;          // state of USR LEDs
int x;                          // return variable for HWREG()
volatile unsigned int USR_STACK[100];   // SVC Stack
volatile unsigned int INT_STACK[100];   // INT Stack

// ------------------------------ MAIN FUNCTION -----------------------------
int main(void) // Mainly for initialization of the program (Timer5, GPIOs, INTC, etc.)
{
    // setup Stacks and initialize both
    asm("LDR R13, =USR_STACK");
    asm("ADD R13, R13, #0x100");
    asm("CPS #0x12");                   // Change mode to IRQ mode
    asm("LDR R13, =INT_STACK");
    asm("ADD R13, R13, #0x100");
    asm("CPS #0x13");                    // Change mode back SVC mode

    // Turn on CLK to GPIO1 module & set the GPIOs at outputs
    HWREG(CLKWKUPS + 0xAC ) = 0x2;
    HWREG(GPIO1BA + 0x134) &= 0xFE1FFFFF;   // bitwise AND of value at address and store  address. This sets the LEDS as outputs.

    //TIMER5 initialization
    HWREG(CLKWKUPS + 0xEC) = 0x2;           // Turn on Timer5 module clock
    HWREG(CLKWKUPS + 0x518) = 0x2;          // set clock speed, CM_SEL_TIMER5_CLK, selects the 32.768KHz clk by programming the mux.

    HWREG(TIMER5_BA + 0x10) = 0x1;          // reset timer5
    HWREG(TIMER5_BA + 0x28) = 0x7;          // clear IRQs
    HWREG(TIMER5_BA + 0x2C) = 0x2;          // Enable overflow IRQ

    // INTC initialization
    HWREG(INTCBA + 0x10) = 0x2;             // reset the INTC
    HWREG(INTCBA + 0xC8) = 0x20000000;      // unmask INTC_TINT5

    // Enable IRQ
    IntMasterIRQEnable();

    // Initialize internal state
    HWREG(TIMER5_BA + 0x3C) = 0xFFFFE000;   // Set timer for 250ms
    HWREG(TIMER5_BA + 0x38) = 0x1;          // Start timer5

    // -------------- WAIT LOOP ---------------------
    waitloop();

	return 0;
}

void waitloop(void)
{
    while(1)
    {
        // Do nothing loop, Loop runs forever until interrupt occurs
    }
}

void timer5_int(void)
{
    HWREG(TIMER5_BA + 0x28) = 0x7;              // Clear Timer5 interrupts
    HWREG(INTCBA + 0x48)    = 0x1;              // clear NEWIRQ bit in INTC to prevent more interrupts
    if(current_state == 1)                     // toggle the current_state variable
    {
        current_state = 0;                      // set current_state to 0
        HWREG(TIMER5_BA + 0x3C) = 0xFFFFE000;   // set timer5 for 250ms
        HWREG(TIMER5_BA + 0x38) = 0x1;          // start timer5
        turnoff_leds();                         // turn on LEDs
    }
    else
    {
       current_state = 1;                       // set current_state to 1
       HWREG(TIMER5_BA + 0x3C) = 0xFFFFA002;    // set timer5 for 750ms
       HWREG(TIMER5_BA + 0x38) = 0x1;           // start timer5
       turnon_leds();                           // turn on LEDs

    }
}

void turnon_leds(void)
{
    HWREG(GPIO1BA + GPIO_SETDATA_OUT) = LIGHT_BITS;
}

void turnoff_leds(void)
{
    HWREG(GPIO1BA + GPIO_CLEARDATA_OUT) = LIGHT_BITS;
}

void int_handler(void)
{
    if(HWREG(0x482000D8) == 0x20000000)
    {
        timer5_int();
    }
    asm("LDMFD SP!, {LR}");
    asm("LDMFD SP!, {LR}");
    asm("SUBS PC, LR, #0x4");
}

void IntMasterIRQEnable(void)
{
    asm("mrs   r0, CPSR\n\t"
        "bic   r0, r0, #0x80\n\t"
        "msr   CPSR_c, r0");
}
