#ifndef COMPLEX_H
#define COMPLEX_H

#define DEBOUNCE_PIN PINC

#define UP 0
#define DOWN 1
#define PRESS 2
#define HOLD 3


void debounce(uint8_t (*p)[8]);

#endif
