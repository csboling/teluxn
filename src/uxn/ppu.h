#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
Copyright (c) 2021 Devine Lu Linvega
Copyright (c) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

typedef struct Layer {
	uint8_t *pixels;
	uint8_t colors[4];
} Layer;

typedef struct Ppu {
	uint16_t hor, ver, width, height;
	Layer fg, bg;
} Ppu;

int initppu(Ppu *p, uint8_t hor, uint8_t ver, uint8_t *bg, uint8_t *fg);
void putcolors(Ppu *p, uint8_t *addr);
void putpixel(Ppu *p, Layer *layer, uint16_t x, uint16_t y, uint8_t color);
void puticn(Ppu *p, Layer *layer, uint16_t x, uint16_t y, uint8_t *sprite, uint8_t color, uint8_t flipx, uint8_t flipy);
void putchr(Ppu *p, Layer *layer, uint16_t x, uint16_t y, uint8_t *sprite, uint8_t color, uint8_t flipx, uint8_t flipy);
void inspect(Ppu *p, uint8_t *stack, uint8_t ptr);
