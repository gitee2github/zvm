/* Copyright © 2014, Owen Shepherd
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef ELFARCH_H
#define ELFARCH_H

#include "elf.h"


#if defined(__arm__)
#define EM_THIS EM_ARM
#elif defined(__aarch64__)
#define EM_THIS EM_AARCH64
#define EL_ARCH_USES_RELA
#define EL_ARCH_USES_REL
#else
#error specify your ELF architecture
#endif /* __arm__ */

#if defined(__LP64__) || defined(__LLP64__)
#define ELFSIZE 64
#else
#define ELFSIZE 32
#endif /* __LP64__ || __LLP64__ */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ELFDATATHIS ELFDATA2LSB
#else
#define ELFDATATHIS ELFDATA2MSB
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

#endif /* ELFARCH_H */
