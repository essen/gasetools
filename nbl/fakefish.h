/*
 *
 * Blowfish-like Cipher Algorithm.
 * Adapted from the Linux kernel Cryptographic API.
 *
 * Copyright (c) 2010 Loïc Hoguin <essen@dev-extend.eu>
 *
 * Original Blowfish Cipher Algorithm by Bruce Schneier.
 * http://www.counterpane.com/blowfish.html
 *
 * Adapted from Kerneli implementation.
 *
 * Copyright (c) Herbert Valerio Riedel <hvr@hvrlab.org>
 * Copyright (c) Kyle McMartin <kyle@debian.org>
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef __FAKEFISH_H__
#define __FAKEFISH_H__

typedef unsigned char u8;
typedef unsigned int u32;

struct bf_ctx {
	u32 p[18];
	u32 s[1024];
};

void bf_setkey(struct bf_ctx *ctx, const u8 *key, unsigned int keylen);
void bf_decrypt(struct bf_ctx *ctx, u8 *dst, const u8 *src);

#endif /* __FAKEFISH_H__ */
