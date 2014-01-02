#ifndef LEDS_H
#define LEDS_H

#include "pins.h"

#define LED_ON(X) 	{ digitalWrite(LED_##X,LOW); }
#define LED_OFF(X)	{ digitalWrite(LED_##X,HIGH); }
#define LED_TOGGLE(X) { digitalWrite(LED_##X,!digitalRead(LED_##X)); }

void rippleUp() {
	// Quickly ripple through GREEN-YELLOW-RED to indicate the start of a new digit
	LED_ON(GREEN); delay(50);
	LED_OFF(GREEN); LED_ON(YELLOW); delay(50);
	LED_OFF(YELLOW); LED_ON(RED); delay(50);
	LED_OFF(RED); delay(350);	
}

void rippleDown() {
	// Quickly ripple through GREEN-YELLOW-RED to indicate the start of a new digit
	LED_ON(RED); delay(50);
	LED_OFF(RED); LED_ON(YELLOW); delay(50);
	LED_OFF(YELLOW); LED_ON(GREEN); delay(50);
	LED_OFF(GREEN); delay(350);	
}

void flashDigit(int digit) {
	// Turn all LEDs off
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	rippleUp();
	// Add up the number of LEDs turned on to get the digit
	while(digit > 3) {
		LED_ON(GREEN); LED_ON(YELLOW); LED_ON(RED);
		delay(500);
		LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
		delay(500);
		digit -= 3;
	}
	switch(digit) {
		case 3:
			LED_ON(RED);
		case 2:
			LED_ON(YELLOW);
		case 1:
			LED_ON(GREEN);
	}
	delay(500);
	LED_OFF(GREEN); LED_OFF(YELLOW); LED_OFF(RED);
	delay(500);
}

void flashNumber(uint32_t value) {
	while(value) {
		flashDigit(value % 10);
		value /= 10;
	}
	rippleDown();
}

#endif
