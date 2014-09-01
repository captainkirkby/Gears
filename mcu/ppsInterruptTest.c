#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "LED.h"
#include "IR.h"
#include "UART.h"
#include "TWI.h"
#include "BMP180.h"
#include "FreeRunningADC.h"
#include "Trimble.h"
#include "packet.h"



int main(void) {
    // Initialize LEDs
    initLEDs();

    // Select Pin to listen to
    PCMSK3 |= (1 << PCINT30);

    // Enable Interrupts on selected pin
    PCICR |= (1 << PCIE3);

    // Enable Global Interrupts
    sei();

    while(1);
}

ISR(PCI3_vect){
    // Disable Interrupts on selected pin
    PCICR &= ~(1 << PCIE3);
    // Turn on the LED
    LED_TOGGLE(GREEN);
}
