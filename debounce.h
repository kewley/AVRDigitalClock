/*
 * Copyright 2012 Craig Kewley
 *
 * This file is part of AVRDigitalClock.
 *
 * AVRDigitalClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AVRDigitalClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AVRDigitalClock.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <avr/io.h>

#define DEBOUNCE_PIN PINC

#define BTN_PRESS_THRESHOLD 3
#define BTN_HOLD_THRESHOLD  100

#define UP 0
#define DOWN 1
#define PRESS 2
#define HOLD 3


void debounce(uint8_t * button_state );

#endif
