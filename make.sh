#!/bin/bash
set -e

CC="--out-fmt-ihx --code-loc 0x000 --xram-loc 0xf000 --xram-size 0x7FF --iram-size 0x100 --model-small --opt-code-speed -I /usr/share/sdcc/include -DFLASH_SIZE=0x4000 -DFLASH_PAGESIZE=0x400"
INCS=(clocksource delay dma radio storage uart wdt)

RELS=""
for inc in "${INCS[@]}"; do
    sdcc -c $CC $inc.c
    RELS+="$inc.rel "
done

sdcc $CC main.c $RELS