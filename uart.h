
/*
    TinyHawkFlyskyRx
    Copyright 2018 Peter Nemcsik

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    https://github.com/nepeee/TinyHawkFlyskyRx
*/

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include "defines.h"

// for a 26MHz Crystal:
#define CC2510_BAUD_E_115200  12
#define CC2510_BAUD_M_115200  34

union uart_config_t {
  uint8_t byte;
  struct {
        uint8_t START : 1;  // start bit level
        uint8_t STOP  : 1;  // stop bit level
        uint8_t SPB   : 1;  // stop bits (0=1, 1=2)
        uint8_t PARITY: 1;  // parity (on/off)
        uint8_t BIT9  : 1;  // 9 bit mode
        uint8_t D9    : 1;  // 9th bit level or parity type
        uint8_t FLOW  : 1;  // flow control
        uint8_t ORDER : 1;  // data bit order (LSB or MSB first)
  } bit;
};

void uart_init(void);
void uart_set_mode(EXTERNAL_MEMORY union uart_config_t *cfg);
void uart_start_transmission(uint8_t *data, uint8_t len);

#ifdef UART_INVERTED
  #define UART_PREPARE_DATA(a) (0xFF ^ (a))
#else
  #define UART_PREPARE_DATA(a) (a)
#endif

#endif
