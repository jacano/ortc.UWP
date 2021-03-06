/*
 * Do not edit this file!
 * This file is generated from:
 *    UnicodeData.txt (version 6.3.0)
 */

#include <stddef.h>
#include <idn/utf32.h>
#include <idn/auto/width.h>

/*
 * Width conversion tables.
 */
#define WIDTH_BITS_0	9
#define WIDTH_BITS_1	7
#define WIDTH_BITS_2	5

static const unsigned short width_imap[] = {
	  272,   272,   272,   400,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   528, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	  272,   272,   272,   272,   272,   272,   272,   272, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    1,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    2,     3,     4,     5,     6,     7,     8,     9, 
};

static const struct {
	unsigned short tbl[32];
} width_table[] = {
	{{
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	}},
	{{
	    32,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	}},
	{{
	     0,     33,     34,     35,     36,     37,     38,     39, 
	    40,     41,     42,     43,     44,     45,     46,     47, 
	    48,     49,     50,     51,     52,     53,     54,     55, 
	    56,     57,     58,     59,     60,     61,     62,     63, 
	}},
	{{
	    64,     65,     66,     67,     68,     69,     70,     71, 
	    72,     73,     74,     75,     76,     77,     78,     79, 
	    80,     81,     82,     83,     84,     85,     86,     87, 
	    88,     89,     90,     91,     92,     93,     94,     95, 
	}},
	{{
	    96,     97,     98,     99,    100,    101,    102,    103, 
	   104,    105,    106,    107,    108,    109,    110,    111, 
	   112,    113,    114,    115,    116,    117,    118,    119, 
	   120,    121,    122,    123,    124,    125,    126,  10629, 
	}},
	{{
	 10630,  12290,  12300,  12301,  12289,  12539,  12530,  12449, 
	 12451,  12453,  12455,  12457,  12515,  12517,  12519,  12483, 
	 12540,  12450,  12452,  12454,  12456,  12458,  12459,  12461, 
	 12463,  12465,  12467,  12469,  12471,  12473,  12475,  12477, 
	}},
	{{
	 12479,  12481,  12484,  12486,  12488,  12490,  12491,  12492, 
	 12493,  12494,  12495,  12498,  12501,  12504,  12507,  12510, 
	 12511,  12512,  12513,  12514,  12516,  12518,  12520,  12521, 
	 12522,  12523,  12524,  12525,  12527,  12531,  12441,  12442, 
	}},
	{{
	 12644,  12593,  12594,  12595,  12596,  12597,  12598,  12599, 
	 12600,  12601,  12602,  12603,  12604,  12605,  12606,  12607, 
	 12608,  12609,  12610,  12611,  12612,  12613,  12614,  12615, 
	 12616,  12617,  12618,  12619,  12620,  12621,  12622,      0, 
	}},
	{{
	     0,      0,  12623,  12624,  12625,  12626,  12627,  12628, 
	     0,      0,  12629,  12630,  12631,  12632,  12633,  12634, 
	     0,      0,  12635,  12636,  12637,  12638,  12639,  12640, 
	     0,      0,  12641,  12642,  12643,      0,      0,      0, 
	}},
	{{
	   162,    163,    172,    175,    166,    165,   8361,      0, 
	  9474,   8592,   8593,   8594,   8595,   9632,   9675,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	     0,      0,      0,      0,      0,      0,      0,      0, 
	}},
};

/*
 * Table accessor.
 */
unsigned long
idn__sparsemap_getwidth(unsigned long v) {
	int idx0, idx1, idx2;

	if (v > UTF32_MAX)
		return (0);
	idx0 = v >> (WIDTH_BITS_1 + WIDTH_BITS_2);
	idx1 = (v >> WIDTH_BITS_2) & ((1 << WIDTH_BITS_1) - 1);
	idx2 = v & ((1 << WIDTH_BITS_2) - 1);
	return (unsigned long) width_table[width_imap[width_imap[idx0] + idx1]].tbl[idx2];
}
