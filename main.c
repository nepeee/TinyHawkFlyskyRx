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
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "config.h"
#include "defines.h"
#include "clocksource.h"
#include "led.h"
#include "uart.h"
#include "delay.h"
#include "storage.h"
#include "radio.h"
#include "wdt.h"
#include "tables.h"

extern EXTERNAL_MEMORY uint8_t pbuffer[RAW_PACKET_BUFFER_SIZE];
EXTERNAL_MEMORY FLASH_STORAGE flashStorage;
uint8_t chanIndex = 0;
uint8_t lostRxCnt = 16;

void main(void) {
    EXTERNAL_MEMORY uint8_t packet[DECODED_PACKET_BUFFER_SIZE];
    uint8_t n = 0, n2, i, i2, out7[8], *in8, isBinding;
    uint16_t crc, rssiAvg = 1020;

    PORT2DIR(BIND_PORT) &= ~(1 << BIND_PIN); //set bind pin as input
    PORT2INP(BIND_PORT) &= ~(1 << BIND_PIN); //set pullup

    clocksource_init();
    led_init();
    wdt_init();

    delay_ms(100);
    if (BIND_PORT & (1 << BIND_PIN)) //bind button is pressed, enter bind mode
        isBinding = 0;
    else
        isBinding = 1;

    if (isBinding) {
        led_green_on();

        //generate random rxId
        ADCCON2 = ADCCON2_SREF_AVDD | ADCCON2_SDIV_12BIT | ADCCON2_SCH_TEMP;
        for (i=0;i<4;i++) {
            ADCCON1 = 0x73;
            while (ADCCON1 & ADCCON1_ST) {}
            if (i & 1)
                RNDL = n & ADCL;
            else
                n = ADCL >> 4;
        }

        for (i=4;i<RTX_ID_SIZE*2;i+=2) {
            ADCCON1 |= 0x04;
            while (ADCCON1 & 0x0C) {}

            flashStorage.trxId[i] = RNDL;
            flashStorage.trxId[i+1] = RNDH;
        }

        flashStorage.hopTable[0] = 13; //set bind channel
    }
    else
        storage_read((uint8_t*)&flashStorage, sizeof(flashStorage)); //read rxId, txId and hopTable from flash

    CLKCON |= CLKCON_TICKSPD_111; //set up channel hopping timer
    T3CC0 = 97;
    T3CTL = T3CTL_DIV_8 | T3CTL_START | T3CTL_OVFIM | T3CTL_CLR | T3CTL_MODE_MODULO;
    EA = 1;

    uart_init();
    radio_init();
    next_channel();

    if (!isBinding)
        IEN1 |= IEN1_T3IE; //enable channel hopping interrupt

    while(1) {
        if (DMAIRQ & DMAIRQ_DMAIF0) {
            if ((pbuffer[0]==0xC5) && (pbuffer[1]==0x2A)) { //check sync word, the cc2510 only supports 2byte + 1byte address, flysky uses 4 bytes 
                n = 2; //Hamming(7,4) decode
                n2 = 0;
                while (n < RAW_PACKET_LENGTH) {
                    in8 = pbuffer + n;

                    out7[0] = in8[0];
                    out7[1] = (in8[0] << 7) | (in8[1] >> 1);
                    out7[2] = (in8[1] << 6) | (in8[2] >> 2);
                    out7[3] = (in8[2] << 5) | (in8[3] >> 3);
                    out7[4] = (in8[3] << 4) | (in8[4] >> 4);
                    out7[5] = (in8[4] << 3) | (in8[5] >> 5);
                    out7[6] = (in8[5] << 2) | (in8[6] >> 6);
                    out7[7] = (in8[6] << 1);

                    for (i=0;i<8;i++)
                        out7[i] = hammingMap[out7[i] >> 1];

                    packet[n2++] = (out7[0] & 0xF0) | (out7[1] >> 4);
                    packet[n2++] = (out7[2] & 0xF0) | (out7[3] >> 4);
                    packet[n2++] = (out7[4] & 0xF0) | (out7[5] >> 4);
                    packet[n2++] = (out7[6] & 0xF0) | (out7[7] >> 4);

                    n += 7;
                }

                radio_arm_dma();

                crc = 0x1D0F; //packet crc calc
                for(i=0;i<38;i++)
                    crc = (crc<<8) ^ crc16tab[((crc>>8) ^ packet[i]) & 0x00FF];

                if ((packet[38]==HI(crc)) && (packet[39]==LO(crc))) { //crc check
                    n = packet[0];

                    if (n==0x58) { //normal rc packet
                        if (memcmp(packet+1, flashStorage.trxId, RTX_ID_SIZE*2)==0) { //txId and rxId matched, this pocket is for us
                            led_red_on();

                            if (lostRxCnt>15) { //hopping desynced
                                next_channel();
                                T3CTL = T3CTL_DIV_8 | T3CTL_START | T3CTL_OVFIM | T3CTL_CLR | T3CTL_MODE_MODULO;
                                T3CC0 = 87;
                            }
                            else { //sync hopping timer
                                T3CC0 = 97 + T3CNT - 10;
                                if ((T3CC0>101) || (T3CC0<93))
                                    T3CC0 = 97;
                            }
                            lostRxCnt = 0;

                            //rssi calc
                            i2 = pbuffer[RAW_PACKET_LENGTH] ^ 0x80;
                            rssiAvg = (((uint16_t)i2 << 2) + rssiAvg * 63) >> 6;
                            crc = 1000 + rssiAvg;
                            packet[35] = LO(crc);
                            packet[36] = HI(crc);

                            //send ibus packet
                            packet[7] = UART_PREPARE_DATA(0x20);
                            packet[8] = UART_PREPARE_DATA(0x40);

                            crc = 0xFF9F;
                            for (i=9;i<37;i++) {
                                n = packet[i];
                                crc -= n;
                                packet[i] = UART_PREPARE_DATA(n); 
                            }

                            packet[37] = UART_PREPARE_DATA(LO(crc));
                            packet[38] = UART_PREPARE_DATA(HI(crc));
                            uart_start_transmission(packet+7, 31);

                            led_red_off();
                        }
                    }
                    else if ((isBinding) && ((n==0xBB) || (n==0xBC))) { //bind packet
                        RFST = RFST_SIDLE;

                        delay_us(5350); //some delay to match with rx ?

                        if (packet[9]>1) {
                            memcpy(flashStorage.trxId, packet+1, RTX_ID_SIZE); // copies rxid somehow ???
                            isBinding = 0;
                        }
 
                        if (packet[11]!=0xFF) //hopTable found, copy to flashStorage struct
                            memcpy(flashStorage.hopTable, packet+11, HOPTABLE_SIZE);

                        memcpy(packet+5, flashStorage.trxId+4, RTX_ID_SIZE); //set our rxId
                        memset(packet+11, 0xFF, 26); //clear the remaining buffer

                        crc = 0x1D0F; //packet crc calc
                        for(i=0;i<38;i++)
                            crc = (crc<<8) ^ crc16tab[((crc>>8) ^ packet[i]) & 0x00FF];
                        packet[38] = HI(crc);
                        packet[39] = LO(crc);

                        n = 2; //Hamming(7,4) encode
                        n2 = 0;
                        while (n<RAW_PACKET_LENGTH) {
                            i = hammingMapTx[packet[n2] >> 4];
                            i2 = hammingMapTx[packet[n2++] & 0x0F];
                            pbuffer[n++] = i | (i2 >> 7);
                            i = hammingMapTx[packet[n2] >> 4];
                            pbuffer[n++] = (i2 << 1) | (i >> 6);
                            i2 = hammingMapTx[packet[n2++] & 0x0F];
                            pbuffer[n++] = (i << 2) | (i2 >> 5);
                            i = hammingMapTx[packet[n2] >> 4];
                            pbuffer[n++] = (i2 << 3) | (i >> 4);
                            i2 = hammingMapTx[packet[n2++] & 0x0F];
                            pbuffer[n++] = (i << 4) | (i2 >> 3);
                            i = hammingMapTx[packet[n2] >> 4];
                            pbuffer[n++] = (i2 << 5) | (i >> 2);
                            i2 = hammingMapTx[packet[n2++] & 0x0F];
                            pbuffer[n++] = (i << 6) | (i2 >> 1);
                        }

                        radio_transmit_packet();
                        while ((DMAIRQ & DMAIRQ_DMAIF0)==0) {};
                        while (MARCSTATE!=0x0D) {}; //the cc2510 automatically enters rx after tx is complete

                        if (!isBinding) { //binded save rxId, txId and hopTable, exit binding mode
                            storage_write((uint8_t*)&flashStorage, sizeof(flashStorage));
                            
                            radio_setup_rf_dma(0);
                            radio_arm_dma();

                            IEN1 |= IEN1_T3IE;
                            next_channel();

                            led_green_off();
                        }
                        else {
                            radio_setup_rf_dma(0);
                            radio_arm_dma();
                        }
                    }
                }
            }
            else
                radio_arm_dma();
        }
        else if (!(MARCSTATE & 0x0C)) { //RX overflow or other error, handle it
            RFST = RFST_SIDLE;
            while (MARCSTATE!=0x01);

            radio_calibrate();
            radio_arm_dma();

            chanIndex = 0;
            next_channel();
        }

        wdt_reset();
    }
}


void timeout_interrupt(void) __interrupt T3_VECTOR { //hopping timer interrupt
    T3IF = 0;
    
    if (lostRxCnt<16) {
        lostRxCnt++;
        next_channel();
    }
}

//we need to use two base frequencies because the cc2510 matching channel step size is 250KHz, flysky is 500KHz
void next_channel() {
    uint8_t chan;

    RFST = RFST_SIDLE;

    chan = flashStorage.hopTable[chanIndex];
    chanIndex = (chanIndex + 1) & 0x0F;

    if (chan<128) { //2400MHz base freq
        FREQ2 = 0x5C;
        FREQ1 = 0x4E;
        FREQ0 = 0xC4;
    }
    else { //2419MHz base freq
        chan -= 38;
        FREQ2 = 0x5D;
        FREQ1 = 0x09;
        FREQ0 = 0xD8;
    }
    CHANNR = chan * 2;

    RFST = RFST_SRX;
}
