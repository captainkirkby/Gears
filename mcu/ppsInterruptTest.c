#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include <stdint.h>

#include "LED.h"

int main(void) {
    initLEDs();

    LED_OFF(GREEN);

    // Set pin as input
    DDRD &= ~0B01000000;

    // Enable internal pull-up resistor
    PORTD |= 0B01000000;

    // Enable Interrupts on selected pin
    PCICR |= 0B00001000;

    // Select Pin to listen to
    PCMSK3 |= 0B01000000;

    // Enable Global Interrupts
    sei();

    while(1){
    }

    return 0;
}

ISR(PCINT3_vect){
    if(PIND & 0B01000000)
    {
        /* LOW to HIGH pin change */
        LED_TOGGLE(GREEN);
        // Disable Interrupts on selected pin
        PCICR &= ~0B00001000;
    }
}
