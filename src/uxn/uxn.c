/* #include <stdio.h> */
#include "uxn.h"

#define printf(...) do {} while(0)

/*
Copyright (u) 2021 Devine Lu Linvega
Copyright (u) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#pragma mark - Operations

/* clang-format off */
void   push8(Stack *s, uint8_t a) { if(s->ptr == 0xff) { s->error = 2; return; } s->dat[s->ptr++] = a; }
uint8_t  pop8_keep(Stack *s) { if(s->kptr == 0) { s->error = 1; return 0; } return s->dat[--s->kptr]; }
uint8_t  pop8_nokeep(Stack *s) { if(s->ptr == 0) { s->error = 1; return 0; } return s->dat[--s->ptr]; }
static uint8_t (*pop8)(Stack *s);
void   mempoke8(uint8_t *m, uint16_t a, uint8_t b) { m[a] = b; }
uint8_t  mempeek8(uint8_t *m, uint16_t a) { return m[a]; }
void   devpoke8(Device *d, uint8_t a, uint8_t b) { d->dat[a & 0xf] = b; d->talk(d, a & 0x0f, 1); }
uint8_t  devpeek8(Device *d, uint8_t a) { d->talk(d, a & 0x0f, 0); return d->dat[a & 0xf];  }
void   push16(Stack *s, uint16_t a) { push8(s, a >> 8); push8(s, a); }
uint16_t pop16(Stack *s) { return pop8(s) + (pop8(s) << 8); }
void   mempoke16(uint8_t *m, uint16_t a, uint16_t b) { mempoke8(m, a, b >> 8); mempoke8(m, a + 1, b); }
uint16_t mempeek16(uint8_t *m, uint16_t a) { return (mempeek8(m, a) << 8) + mempeek8(m, a + 1); }
void   devpoke16(Device *d, uint8_t a, uint16_t b) { devpoke8(d, a, b >> 8); devpoke8(d, a + 1, b); }
uint16_t devpeek16(Device *d, uint16_t a) { return (devpeek8(d, a) << 8) + devpeek8(d, a + 1); }
/* Stack */
void op_brk(Uxn *u) { u->ram.ptr = 0; }
void op_nop(Uxn *u) { (void)u; }
void op_lit(Uxn *u) { push8(u->src, mempeek8(u->ram.dat, u->ram.ptr++)); }
void op_pop(Uxn *u) { pop8(u->src); }
void op_dup(Uxn *u) { uint8_t a = pop8(u->src); push8(u->src, a); push8(u->src, a); }
void op_swp(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, a); push8(u->src, b); }
void op_ovr(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b); push8(u->src, a); push8(u->src, b); }
void op_rot(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src), c = pop8(u->src); push8(u->src, b); push8(u->src, a); push8(u->src, c); }
/* Logic */
void op_equ(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b == a); }
void op_neq(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b != a); }
void op_gth(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b > a); }
void op_lth(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b < a); }
void op_jmp(Uxn *u) { uint8_t a = pop8(u->src); u->ram.ptr += (int8_t)a; }
void op_jnz(Uxn *u) { uint8_t a = pop8(u->src); if(pop8(u->src)) u->ram.ptr += (int8_t)a; }
void op_jsr(Uxn *u) { uint8_t a = pop8(u->src); push16(u->dst, u->ram.ptr); u->ram.ptr += (int8_t)a; }
void op_sth(Uxn *u) { uint8_t a = pop8(u->src); push8(u->dst, a); }
/* Memory */
void op_pek(Uxn *u) { uint8_t a = pop8(u->src); push8(u->src, mempeek8(u->ram.dat, a)); }
void op_pok(Uxn *u) { uint8_t a = pop8(u->src); uint8_t b = pop8(u->src); mempoke8(u->ram.dat, a, b); }
void op_ldr(Uxn *u) { uint8_t a = pop8(u->src); push8(u->src, mempeek8(u->ram.dat, u->ram.ptr + (int8_t)a)); }
void op_str(Uxn *u) { uint8_t a = pop8(u->src); uint8_t b = pop8(u->src); mempoke8(u->ram.dat, u->ram.ptr + (int8_t)a, b); }
void op_lda(Uxn *u) { uint16_t a = pop16(u->src); push8(u->src, mempeek8(u->ram.dat, a)); }
void op_sta(Uxn *u) { uint16_t a = pop16(u->src); uint8_t b = pop8(u->src); mempoke8(u->ram.dat, a, b); }
void op_dei(Uxn *u) { uint8_t a = pop8(u->src); push8(u->src, devpeek8(&u->dev[a >> 4], a)); }
void op_deo(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); devpoke8(&u->dev[a >> 4], a, b); }
/* Arithmetic */
void op_add(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b + a); }
void op_sub(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b - a); }
void op_mul(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b * a); }
void op_div(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); if(a == 0) { u->src->error = 3; a = 1; } push8(u->src, b / a); }
void op_and(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b & a); }
void op_ora(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b | a); }
void op_eor(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b ^ a); }
void op_sft(Uxn *u) { uint8_t a = pop8(u->src), b = pop8(u->src); push8(u->src, b >> (a & 0x07) << ((a & 0x70) >> 4)); }
/* Stack */
void op_lit16(Uxn *u) { push16(u->src, mempeek16(u->ram.dat, u->ram.ptr++)); u->ram.ptr++; }
void op_pop16(Uxn *u) { pop16(u->src); }
void op_dup16(Uxn *u) { uint16_t a = pop16(u->src); push16(u->src, a); push16(u->src, a); }
void op_swp16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, a); push16(u->src, b); }
void op_ovr16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b); push16(u->src, a); push16(u->src, b); }
void op_rot16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src), c = pop16(u->src); push16(u->src, b); push16(u->src, a); push16(u->src, c); }
/* Logic(16-bits) */
void op_equ16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push8(u->src, b == a); }
void op_neq16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push8(u->src, b != a); }
void op_gth16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push8(u->src, b > a); }
void op_lth16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push8(u->src, b < a); }
void op_jmp16(Uxn *u) { u->ram.ptr = pop16(u->src); }
void op_jnz16(Uxn *u) { uint16_t a = pop16(u->src); if(pop8(u->src)) u->ram.ptr = a; }
void op_jsr16(Uxn *u) { push16(u->dst, u->ram.ptr); u->ram.ptr = pop16(u->src); }
void op_sth16(Uxn *u) { uint16_t a = pop16(u->src); push16(u->dst, a); }
/* Memory(16-bits) */
void op_pek16(Uxn *u) { uint8_t a = pop8(u->src); push16(u->src, mempeek16(u->ram.dat, a)); }
void op_pok16(Uxn *u) { uint8_t a = pop8(u->src); uint16_t b = pop16(u->src); mempoke16(u->ram.dat, a, b); }
void op_ldr16(Uxn *u) { uint8_t a = pop8(u->src); push16(u->src, mempeek16(u->ram.dat, u->ram.ptr + (int8_t)a)); }
void op_str16(Uxn *u) { uint8_t a = pop8(u->src); uint16_t b = pop16(u->src); mempoke16(u->ram.dat, u->ram.ptr + (int8_t)a, b); }
void op_lda16(Uxn *u) { uint16_t a = pop16(u->src); push16(u->src, mempeek16(u->ram.dat, a)); }
void op_sta16(Uxn *u) { uint16_t a = pop16(u->src); uint16_t b = pop16(u->src); mempoke16(u->ram.dat, a, b); }
void op_dei16(Uxn *u) { uint8_t a = pop8(u->src); push16(u->src, devpeek16(&u->dev[a >> 4], a)); }
void op_deo16(Uxn *u) { uint8_t a = pop8(u->src); uint16_t b = pop16(u->src); devpoke16(&u->dev[a >> 4], a, b); }
/* Arithmetic(16-bits) */
void op_add16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b + a); }
void op_sub16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b - a); }
void op_mul16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b * a); }
void op_div16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); if(a == 0) { u->src->error = 3; a = 1; } push16(u->src, b / a); }
void op_and16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b & a); }
void op_ora16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b | a); }
void op_eor16(Uxn *u) { uint16_t a = pop16(u->src), b = pop16(u->src); push16(u->src, b ^ a); }
void op_sft16(Uxn *u) { uint8_t a = pop8(u->src); uint16_t b = pop16(u->src); push16(u->src, b >> (a & 0x0f) << ((a & 0xf0) >> 4)); }

void (*ops[])(Uxn *u) = {
	op_brk, op_lit, op_nop, op_pop, op_dup, op_swp, op_ovr, op_rot,
	op_equ, op_neq, op_gth, op_lth, op_jmp, op_jnz, op_jsr, op_sth,
	op_pek, op_pok, op_ldr, op_str, op_lda, op_sta, op_dei, op_deo,
	op_add, op_sub, op_mul, op_div, op_and, op_ora, op_eor, op_sft,
	/* 16-bit */
	op_brk,   op_lit16, op_nop,   op_pop16, op_dup16, op_swp16, op_ovr16, op_rot16,
	op_equ16, op_neq16, op_gth16, op_lth16, op_jmp16, op_jnz16, op_jsr16, op_sth16,
	op_pek16, op_pok16, op_ldr16, op_str16, op_lda16, op_sta16, op_dei16, op_deo16,
	op_add16, op_sub16, op_mul16, op_div16, op_and16, op_ora16, op_eor16, op_sft16
};

/* clang-format on */

#pragma mark - Core

static const char *errors[] = {"underflow", "overflow", "division by zero"};

int
haltuxn(Uxn *u, uint8_t error, char *name, int id)
{
	printf("Halted: %s %s#%04x, at 0x%04x\n", name, errors[error - 1], id, u->ram.ptr);
	u->ram.ptr = 0;
	return 0;
}

void
opcuxn(Uxn *u, uint8_t instr)
{
	uint8_t op = instr & 0x3f, freturn = instr & 0x40, fkeep = instr & 0x80;
	u->src = freturn ? &u->rst : &u->wst;
	u->dst = freturn ? &u->wst : &u->rst;
	if(fkeep) {
		pop8 = pop8_keep;
		u->src->kptr = u->src->ptr;
	} else {
		pop8 = pop8_nokeep;
	}
	(*ops[op])(u);
}

int
stepuxn(Uxn *u, uint8_t instr)
{
	opcuxn(u, instr);
	if(u->wst.error)
		return haltuxn(u, u->wst.error, "Working-stack", instr);
	if(u->rst.error)
		return haltuxn(u, u->rst.error, "Return-stack", instr);
	return 1;
}

int
evaluxn(Uxn *u, uint16_t vec)
{
	u->ram.ptr = vec;
	u->wst.error = 0;
	u->rst.error = 0;
	while(u->ram.ptr)
		if(!stepuxn(u, u->ram.dat[u->ram.ptr++]))
			return 0;
	return 1;
}

int
bootuxn(Uxn *u)
{
	size_t i;
	char *cptr = (char *)u;
	for(i = 0; i < sizeof(*u); i++)
		cptr[i] = 0;
	return 1;
}

int
loaduxn(Uxn *u, char *filepath)
{
	/* FILE *f; */
	/* if(!(f = fopen(filepath, "rb"))) { */
	/* 	printf("Halted: Missing input rom.\n"); */
	/* 	return 0; */
	/* } */
	/* fread(u->ram.dat + PAGE_PROGRAM, sizeof(u->ram.dat) - PAGE_PROGRAM, 1, f); */
	/* printf("Uxn loaded[%s].\n", filepath); */
	return 1;
}

Device *
portuxn(Uxn *u, uint8_t id, char *name, void (*talkfn)(Device *d, uint8_t b0, uint8_t w))
{
	Device *d = &u->dev[id];
	d->addr = id * 0x10;
	d->u = u;
	d->mem = u->ram.dat;
	d->talk = talkfn;
	printf("Device added #%02x: %s, at 0x%04x \n", id, name, d->addr);
	return d;
}
