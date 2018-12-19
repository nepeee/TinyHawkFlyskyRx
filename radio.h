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

#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdint.h>

#define RAW_PACKET_LENGTH 72
#define RAW_PACKET_BUFFER_SIZE RAW_PACKET_LENGTH + 2
#define DECODED_PACKET_BUFFER_SIZE 40

void radio_init(void);
void radio_calibrate(void);
void radio_setup_rf_dma(uint8_t isTx);
void radio_transmit_packet(void);

#define radio_arm_dma() DMAIRQ &= ~DMAIRQ_DMAIF0; DMAARM = DMA_ARM_CH0;

#endif
