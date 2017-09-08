/*
 * Do not edit this file!
 * This file is generated from:
 *    UnicodeData.txt (version 6.3.0)
 */

#ifndef IDN_BIDICLASS_H
#define IDN_BIDICLASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*
 * Bidi classes.
 */
typedef enum {
	idn__bidiclass_L   =  0,
	idn__bidiclass_LRE =  1,
	idn__bidiclass_LRO =  2,
	idn__bidiclass_R   =  3,
	idn__bidiclass_AL  =  4,
	idn__bidiclass_RLE =  5,
	idn__bidiclass_RLO =  6,
	idn__bidiclass_PDF =  7,
	idn__bidiclass_EN  =  8,
	idn__bidiclass_ES  =  9,
	idn__bidiclass_ET  = 10,
	idn__bidiclass_AN  = 11,
	idn__bidiclass_CS  = 12,
	idn__bidiclass_NSM = 13,
	idn__bidiclass_BN  = 14,
	idn__bidiclass_B   = 15,
	idn__bidiclass_S   = 16,
	idn__bidiclass_WS  = 17,
	idn__bidiclass_ON  = 18,
	idn__bidiclass_LRI = 19,
	idn__bidiclass_RLI = 20,
	idn__bidiclass_FSI = 21,
	idn__bidiclass_PDI = 22,
	idn__bidiclass_unknown = -1
} idn__bidiclass_t;

/*
 * Table accessor.
 */
extern idn__bidiclass_t
idn__sparsemap_getbidiclass(unsigned long v);

#ifdef __cplusplus
}
#endif

#endif