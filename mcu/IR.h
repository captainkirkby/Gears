#ifndef IR_H
#define IR_H

// the port that the output pin belongs to
#define IR_PORT		PORTD
#define IR_DDR		DDRD

// bit number for the output pin
#define IR_PIN		7

// Initialize IR output level control
void initIR() {
	// We will eventually allow the IR level to be modified by driving a PWM waveform
	// from this pin (a la Arduino analogWrite) but for now we configure the port as
	// a digital output and drive it low (which gives the maximum IR output level).
	IR_DDR |= _BV(IR_PIN);
	IR_PORT &= ~_BV(IR_PIN);
}

#endif
