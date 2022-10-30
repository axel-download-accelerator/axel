/* Axel -- A lighter download accelerator for Linux and other Unices
 *
 * SipHash-2-4-32 hashing function
 *
 * Copyright 2016 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
 * Copyright 2022 Ismael Luceno <ismael@iodev.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 *
 * You must obey the GNU General Public License in all respects for all
 * of the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do
 * so, delete this exception statement from your version. If you delete
 * this exception statement from all source files in the program, then
 * also delete it here.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stddef.h>
#include <stdint.h>
#include "hash.h"

/* default: SipHash-2-4 */
#define cROUNDS 2
#define dROUNDS 4

#define FORCE_INLINE __attribute__((always_inline)) inline

FORCE_INLINE static
uint32_t
ROTL(uint32_t x, uint8_t b)
{
	return x << b | x >> (32 - b);
}

static inline
uint32_t
U8TO32_LE(const uint8_t *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

/**
 * Convert from/to little endian.
 */
FORCE_INLINE
uint32_t
cu32swap(uint32_t n)
{
	const union { int i; char c; } u = { 1 };
	return u.c ? n : n>>24 | (n>>8&0xff00) | (n<<8&0xff0000) | n<<24;
}

#define cpu_to_u32le cu32swap
#define u32le_to_cpu cu32swap

FORCE_INLINE static void SIPROUND(uint32_t v[4]);

/**
 * Computes a SipHash-like value.
 */
uint32_t
axel_hash32(const void *src, size_t len, const uint64_t *kk)
{
	const uint8_t *ni = src;

	const int tail = len & 3;
	const unsigned char *end = ni + (len & ~3);
	uint32_t b = len << 24;
	uint32_t v[4];

	v[0] = u32le_to_cpu(kk[0]);
	v[1] = u32le_to_cpu(kk[0] >> 32);
	v[2] = UINT32_C(0x6c796765) ^ v[0];
	v[3] = UINT32_C(0x74656462) ^ v[1];

	for (; ni != end; ni += 4) {
		uint32_t m = U8TO32_LE(ni);
		v[3] ^= m;

		for (int i = 0; i < cROUNDS; ++i)
			SIPROUND(v);

		v[0] ^= m;
	}

	switch (tail) {
	case 3: b |= ((uint32_t)ni[2]) << 16; /* fallthru */
	case 2: b |= ((uint32_t)ni[1]) << 8; /* fallthru */
	case 1: b |= ((uint32_t)ni[0]);
	default: break;
	}

	v[3] ^= b;

	for (int i = 0; i < cROUNDS; ++i)
		SIPROUND(v);

	v[0] ^= b;
	v[2] ^= 0xff;

	for (int i = 0; i < dROUNDS; ++i)
		SIPROUND(v);

	b = v[1] ^ v[3];
	return cpu_to_u32le(b);
}

void
SIPROUND(uint32_t v[4])
{
	v[0] += v[1];
	v[1] = ROTL(v[1], 5);
	v[1] ^= v[0];
	v[0] = ROTL(v[0], 16);
	v[2] += v[3];
	v[3] = ROTL(v[3], 8);
	v[3] ^= v[2];
	v[0] += v[3];
	v[3] = ROTL(v[3], 7);
	v[3] ^= v[0];
	v[2] += v[1];
	v[1] = ROTL(v[1], 13);
	v[1] ^= v[2];
	v[2] = ROTL(v[2], 16);
}
