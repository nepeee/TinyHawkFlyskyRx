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
#include "radio.h"
#include "defines.h"
#include "dma.h"
#include "main.h"

EXTERNAL_MEMORY uint8_t pbuffer[RAW_PACKET_BUFFER_SIZE];

void radio_init(void) {
    RFST = RFST_SIDLE;

    SYNC1 = 0x54; //flysky sync word
    SYNC0 = 0x75;
    PKTLEN = RAW_PACKET_LENGTH;
    
    PKTCTRL1 = 0x05; //address check, append status
    PKTCTRL0 = 0x00; //fix packet length, crc disabled
    ADDR = 0xC5; //extra sync check byte

    FSCTRL1 = 0x10; //406.250KHk if

    MDMCFG4 = 0x0E; //500 kBaud data rate, channel filter bandwidth 812.5KHz
    MDMCFG3 = 0x3B;
    MDMCFG2 = 0x05; //FSK, 15 bit carrier-sense + 16 bit sync word match + carrier above threshold
    MDMCFG1 = 0x23; //4 byte permable transmitted
    MDMCFG0 = 0x3B; //channel spacing 250KHz
    DEVIATN = 0x67; //190.43KHz fsk deviation insted of the flysky 186Khz :(

    MCSM1 = 0x03;//go back to rx after tx
    MCSM0 = 0x14; //call mode: when going from IDLE to RX or TX (or FSTXON)   
    FOCCFG = 0x1D;
    BSCFG = 0x1C;
    AGCCTRL2 = 0xC7;
    AGCCTRL1 = 0x40;
    AGCCTRL0 = 0xB2;

    FSCAL1 = 0x00;
    FSCAL0 = 0x11;

    TEST1 = 0x31;
    TEST0 = 0x09;

    PA_TABLE0 = 0xFF; //1dBm tx power (max)

    radio_calibrate();

    radio_setup_rf_dma(0);
    radio_arm_dma();
}

void radio_calibrate() {
    //2400MHz base freq
    FREQ2 = 0x5C;
    FREQ1 = 0x4E;
    FREQ0 = 0xC4;

    CHANNR = 26; //bind channel
    
    FSCAL3 = 0xEA; //enable charge pump calibration
    RFST = RFST_SCAL;
    while (MARCSTATE!=0x01);

    FSCAL3 = FSCAL3 & 0xCF; //disable charge pump calibration
}

void radio_setup_rf_dma(uint8_t isTx) {
    dma_config[0].PRIORITY = DMA_PRI_HIGH;
    dma_config[0].M8       = DMA_M8_USE_8_BITS;
    dma_config[0].IRQMASK  = DMA_IRQMASK_DISABLE;
    dma_config[0].TRIG     = DMA_TRIG_RADIO;
    dma_config[0].TMODE    = DMA_TMODE_SINGLE;
    dma_config[0].WORDSIZE = DMA_WORDSIZE_BYTE;

    if (isTx) {
        SET_WORD(dma_config[0].SRCADDRH, dma_config[0].SRCADDRL, pbuffer);
        SET_WORD(dma_config[0].DESTADDRH, dma_config[0].DESTADDRL, &X_RFD);
        dma_config[0].VLEN = DMA_VLEN_FIRST_BYTE_P_1;
        SET_WORD(dma_config[0].LENH, dma_config[0].LENL, RAW_PACKET_LENGTH);
        dma_config[0].SRCINC = DMA_SRCINC_1;
        dma_config[0].DESTINC = DMA_DESTINC_0;
    }
    else {
        SET_WORD(dma_config[0].SRCADDRH, dma_config[0].SRCADDRL, &X_RFD);
        SET_WORD(dma_config[0].DESTADDRH, dma_config[0].DESTADDRL, pbuffer);
        dma_config[0].VLEN = DMA_VLEN_FIRST_BYTE_P_3;
        SET_WORD(dma_config[0].LENH, dma_config[0].LENL, RAW_PACKET_BUFFER_SIZE);
        dma_config[0].SRCINC = DMA_SRCINC_0;
        dma_config[0].DESTINC = DMA_DESTINC_1;        
    }

    SET_WORD(DMA0CFGH, DMA0CFGL, &dma_config[0]);
}

void radio_transmit_packet() {
    DMAARM = DMA_ARM_ABORT | DMA_ARM_CH0;
    radio_setup_rf_dma(1);

    radio_arm_dma();
    RFST = RFST_STX;
}
