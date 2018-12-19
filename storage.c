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

#include <cc2510fx.h>
#include "storage.h"
#include "defines.h"

__xdata HAL_DMA_DESC flash_dma_config;
// no ini value -> sdcc does not init this!
__code __at(STORAGE_LOCATION) uint8_t storage_on_flash[STORAGE_PAGE_SIZE];


void storage_write(uint8_t *buffer, uint16_t len) {
    storage_flash_write((uint16_t)storage_on_flash, buffer, len);
}

void storage_read(uint8_t *storage_ptr, uint16_t len) {
    uint16_t i;

    // copy from persistant flash to ram:
    for (i = 0; i < len; i++) {
        storage_ptr[i] = storage_on_flash[i];
        //wdt_reset();
    }
}

void storage_flash_write(uint16_t address, uint8_t *data, uint16_t len) {
    uint16_t i = 0;
    uint8_t *flash_ptr = 0;

    // this is VERY important:
    // make sure to write an even number of bytes!
    // simply write one extra byte
    if (len & 0x0001)
        len++;

    // disable interrupts
    cli();

    // cancel _ALL_ ongoing DMA transfers:
    DMAARM = DMA_ARM_ABORT | 0x1F;

    // high prio
    flash_dma_config.PRIORITY       = DMA_PRI_HIGH;
    flash_dma_config.M8             = DMA_M8_USE_8_BITS;
    flash_dma_config.IRQMASK        = DMA_IRQMASK_DISABLE;
    flash_dma_config.TRIG           = DMA_TRIG_FLASH;
    flash_dma_config.TMODE          = DMA_TMODE_SINGLE;
    flash_dma_config.WORDSIZE       = DMA_WORDSIZE_BYTE;

    SET_WORD(flash_dma_config.SRCADDRH,  flash_dma_config.SRCADDRL,  data);
    SET_WORD(flash_dma_config.DESTADDRH, flash_dma_config.DESTADDRL, &X_FWDATA);
    flash_dma_config.VLEN           = DMA_VLEN_USE_LEN;
    SET_WORD(flash_dma_config.LENH, flash_dma_config.LENL, len);
    flash_dma_config.SRCINC         = DMA_SRCINC_1;
    flash_dma_config.DESTINC        = DMA_DESTINC_0;

    // Save pointer to the DMA configuration struct into DMA-channel 0
    // configuration registers
    SET_WORD(DMA0CFGH, DMA0CFGL, flash_dma_config);

    // waiting for the flash controller to be ready
    while (FCTL & FCTL_BUSY) {}

    // configure flash controller for 26mhz clock
    FWT = 0x2A;  // (21 * 26) / (16);

    // set up address:
    SET_WORD(FADDRH, FADDRL, ((uint16_t)address)>>1);

    // re enable ints
    sei();
    // disable interrupts
    cli();

    // clear any pending flags
    DMAIRQ = 0;

    // erase that page
    // has to be 2byte aligned. use a hack to place it at a given adress:
    storage_flash_erase_page();

    // Wait for the erase operation to complete
    while (FCTL & FCTL_BUSY) {}

    // FCTL = 0;

    // re enable ints
    sei();

    // disable interrupts
    cli();

    // arm the DMA channel, so that a DMA trigger will initiate DMA writing
    DMAARM = DMA_ARM_CH0;
    NOP();

    // enable flash write. this generates a DMA trigger.
    // must be aligned on a 2-byte boundary and is therefor implemented in assembly!
    storage_flash_enable_write();


    // wait for dma finish
    while (!(DMAIRQ & DMAIRQ_DMAIF0)) {
       //wdt_reset();
    }

    // wait until flash controller not busy
    while (FCTL & (FCTL_BUSY | FCTL_SWBUSY)) {}

    sei();

    // by now, the transfer is completed, so the transfer count is reached.
    // the DMA channel 0 interrupt flag is then set, so we clear it here.
    DMAIRQ &= ~DMAIRQ_DMAIF0;
}

void storage_flash_enable_write(void) {
    __asm
    .even              // IMPORTANT: PLACE THIS ON A 2BYTE BOUNDARY!
    ORL _FCTL, #0x02;  // FCTL |=  FCTL_WRITE
    NOP;
    __endasm;
}

void storage_flash_erase_page(void) {
    __asm
    .even              // IMPORTANT: PLACE THIS ON A 2BYTE BOUNDARY!
    ORL _FCTL, #0x01;  // FCTL |= FCTL_ERASE
    NOP;               // required sequence!
    __endasm;
}
