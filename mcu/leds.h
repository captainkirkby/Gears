#ifndef LEDS_H
#define LEDS_H

#include "pins.h"

#define LED_ON(X) 	{ digitalWrite(LED_##X,LOW); }
#define LED_OFF(X)	{ digitalWrite(LED_##X,HIGH); }
#define LED_TOGGLE(X) { digitalWrite(LED_##X,!digitalRead(LED_##X)); }

#endif
