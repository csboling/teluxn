#pragma once

#include <stdio.h>
#include <stdint.h>

/*
Copyright (c) 2021 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define PAGE_PROGRAM 0x0100

typedef struct {
	uint8_t ptr, kptr, error;
	uint8_t dat[256];
} Stack;

typedef struct {
	uint16_t ptr;
	uint8_t dat[65536];
} Memory;

typedef struct Device {
	struct Uxn *u;
	uint8_t addr, dat[16], *mem;
	void (*talk)(struct Device *d, uint8_t, uint8_t);
} Device;

typedef struct Uxn {
	Stack wst, rst, *src, *dst;
	Memory ram;
	Device dev[16];
} Uxn;

struct Uxn;

void mempoke16(uint8_t *m, uint16_t a, uint16_t b);
uint16_t mempeek16(uint8_t *m, uint16_t a);

int loaduxn(Uxn *c, char *filepath);
int bootuxn(Uxn *c);
int evaluxn(Uxn *u, uint16_t vec);
Device *portuxn(Uxn *u, uint8_t id, char *name, void (*talkfn)(Device *, uint8_t, uint8_t));
