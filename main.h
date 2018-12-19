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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>
#include "defines.h"

#define RTX_ID_SIZE 4
#define HOPTABLE_SIZE 16

typedef struct {
	uint8_t trxId[8];
	uint8_t hopTable[HOPTABLE_SIZE];
} FLASH_STORAGE;


void next_channel(void);

#endif
