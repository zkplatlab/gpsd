/*
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "gps.h"

/*@-type@*/
static struct
{
    struct tm t;
    time_t result;
} tests[] = {
	/* *INDENT-OFF* */
	/* sec, min,  h, md, mon, year, wd,  yd, isdst, gmtoff, zone    timestamp        what	*/
	{{   0,   0,  0,  1,   0,   70,  0,   0,     0,      0,    0, }, 0 },
	{{   0,   0,  0,  1,   0,   70,  0,   0,     0,      0,    0, }, 0	      }, /* lower limit */
	{{   7,  14,  3, 19,   0,  138,  0,   0,     0,      0,    0, }, 0x7fffffff }, /* upper limit */
	{{   0,   0, 12,  1,   0,   99,  0,   0,     0,      0,    0, }, 915192000  }, /* leap year */
	{{   0,   0, 12,  1,   1,   99,  0,   0,     0,      0,    0, }, 917870400  }, /* leap year */
	{{   0,   0, 12,  1,   2,   99,  0,   0,     0,      0,    0, }, 920289600  }, /* leap year */
	{{   0,   0, 12,  1,   8,   99,  0,   0,     0,      0,    0, }, 936187200  }, /* leap year */
	{{   0,   0, 12,  1,   0,  100,  0,   0,     0,      0,    0, }, 946728000  }, /* leap year */
	{{   0,   0, 12,  1,   1,  100,  0,   0,     0,      0,    0, }, 949406400  }, /* leap year */
	{{   0,   0, 12,  1,   2,  100,  0,   0,     0,      0,    0, }, 951912000  }, /* leap year */
	{{   0,   0, 12,  1,   8,  100,  0,   0,     0,      0,    0, }, 967809600  }, /* leap year */
	{{   0,   0, 12,  1,   0,  101,  0,   0,     0,      0,    0, }, 978350400  }, /* leap year */
	{{   0,   0, 12,  1,   1,  101,  0,   0,     0,      0,    0, }, 981028800  }, /* leap year */
	{{   0,   0, 12,  1,   2,  101,  0,   0,     0,      0,    0, }, 983448000  }, /* leap year */
	{{   0,   0, 12,  1,   8,  101,  0,   0,     0,      0,    0, }, 999345600  }, /* leap year */
	{{   0,   0, 12,  1,   0,  102,  0,   0,     0,      0,    0, }, 1009886400 }, /* leap year */
	{{   0,   0, 12,  1,   1,  102,  0,   0,     0,      0,    0, }, 1012564800 }, /* leap year */
	{{   0,   0, 12,  1,   2,  102,  0,   0,     0,      0,    0, }, 1014984000 }, /* leap year */
	{{   0,   0, 12,  1,   8,  102,  0,   0,     0,      0,    0, }, 1030881600 }, /* leap year */
	{{   0,   0, 12,  1,   0,  103,  0,   0,     0,      0,    0, }, 1041422400 }, /* leap year */
	{{   0,   0, 12,  1,   1,  103,  0,   0,     0,      0,    0, }, 1044100800 }, /* leap year */
	{{   0,   0, 12,  1,   2,  103,  0,   0,     0,      0,    0, }, 1046520000 }, /* leap year */
	{{   0,   0, 12,  1,   8,  103,  0,   0,     0,      0,    0, }, 1062417600 }, /* leap year */
	{{   0,   0, 12,  1,   0,  104,  0,   0,     0,      0,    0, }, 1072958400 }, /* leap year */
	{{   0,   0, 12,  1,   1,  104,  0,   0,     0,      0,    0, }, 1075636800 }, /* leap year */
	{{   0,   0, 12,  1,   2,  104,  0,   0,     0,      0,    0, }, 1078142400 }, /* leap year */
	{{   0,   0, 12,  1,   8,  104,  0,   0,     0,      0,    0, }, 1094040000 }, /* leap year */
	{{   0,   0, 12,  1,   0,  108,  0,   0,     0,      0,    0, }, 1199188800 }, /* leap year */
	{{   0,   0, 12,  1,   1,  108,  0,   0,     0,      0,    0, }, 1201867200 }, /* leap year */
	{{   0,   0, 12,  1,   2,  108,  0,   0,     0,      0,    0, }, 1204372800 }, /* leap year */
	{{   0,   0, 12,  1,   8,  108,  0,   0,     0,      0,    0, }, 1220270400 }, /* leap year */
	{{  59,  59, 23, 31,  12,  110,  0,   0,     0,      0,    0, }, 1296518399 }, /* year wrap */
	{{   0,   0,  0,  1,   0,  111,  0,   0,     0,      0,    0, }, 1293840000 }, /* year wrap */
	{{  59,  59, 23, 31,  12,  111,  0,   0,     0,      0,    0, }, 1328054399 }, /* year wrap */
	{{   0,   0,  0,  1,   0,  112,  0,   0,     0,      0,    0, }, 1325376000 }, /* year wrap */
	{{  59,  59, 23, 31,  12,  112,  0,   0,     0,      0,    0, }, 1359676799 }, /* year wrap */
	{{   0,   0,  0,  1,   0,  113,  0,   0,     0,      0,    0, }, 1356998400 }, /* year wrap */
	{{  59,  59, 23, 31,   0,  115,  0,   0,     0,      0,    0, }, 1422748799 }, /* month wrap */
	{{   0,   0,  0,  1,   1,  115,  0,   0,     0,      0,    0, }, 1422748800 }, /* month wrap */
	{{  59,  59, 23, 28,   1,  115,  0,   0,     0,      0,    0, }, 1425167999 }, /* month wrap */
	{{   0,   0,  0,  1,   2,  115,  0,   0,     0,      0,    0, }, 1425168000 }, /* month wrap */
	{{  59,  59, 23, 31,   2,  115,  0,   0,     0,      0,    0, }, 1427846399 }, /* month wrap */
	{{   0,   0,  0,  1,   3,  115,  0,   0,     0,      0,    0, }, 1427846400 }, /* month wrap */
	{{  59,  59, 23, 30,   3,  115,  0,   0,     0,      0,    0, }, 1430438399 }, /* month wrap */
	{{   0,   0,  0,  1,   4,  115,  0,   0,     0,      0,    0, }, 1430438400 }, /* month wrap */
	{{  59,  59, 23, 31,   4,  115,  0,   0,     0,      0,    0, }, 1433116799 }, /* month wrap */
	{{   0,   0,  0,  1,   5,  115,  0,   0,     0,      0,    0, }, 1433116800 }, /* month wrap */
	{{  59,  59, 23, 30,   5,  115,  0,   0,     0,      0,    0, }, 1435708799 }, /* month wrap */
	{{   0,   0,  0,  1,   6,  115,  0,   0,     0,      0,    0, }, 1435708800 }, /* month wrap */
	{{  59,  59, 23, 31,   6,  115,  0,   0,     0,      0,    0, }, 1438387199 }, /* month wrap */
	{{   0,   0,  0,  1,   7,  115,  0,   0,     0,      0,    0, }, 1438387200 }, /* month wrap */
	{{  59,  59, 23, 31,   7,  115,  0,   0,     0,      0,    0, }, 1441065599 }, /* month wrap */
	{{   0,   0,  0,  1,   8,  115,  0,   0,     0,      0,    0, }, 1441065600 }, /* month wrap */
	{{  59,  59, 23, 30,   8,  115,  0,   0,     0,      0,    0, }, 1443657599 }, /* month wrap */
	{{   0,   0,  0,  1,   9,  115,  0,   0,     0,      0,    0, }, 1443657600 }, /* month wrap */
	{{  59,  59, 23, 31,   9,  115,  0,   0,     0,      0,    0, }, 1446335999 }, /* month wrap */
	{{   0,   0,  0,  1,  10,  115,  0,   0,     0,      0,    0, }, 1446336000 }, /* month wrap */
	{{  59,  59, 23, 30,  10,  115,  0,   0,     0,      0,    0, }, 1448927999 }, /* month wrap */
	{{   0,   0,  0,  1,  11,  115,  0,   0,     0,      0,    0, }, 1448928000 }, /* month wrap */
	{{  59,  59, 23, 31,  11,  115,  0,   0,     0,      0,    0, }, 1451606399 }, /* month wrap */
	{{   0,   0,  0,  1,   0,  116,  0,   0,     0,      0,    0, }, 1451606400 }, /* month wrap */
	/* *INDENT-ON* */
};

/*@-type@*/

/*@+longunsignedintegral*/
int main(int argc UNUSED, char *argv[] UNUSED)
{
    int i;
    char tbuf[128];
    time_t ts;
    bool failed = false;

    (void)setenv("TZ", "GMT", 1);

    for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
#if 0				/* use this to calculate with glibc */
	ts = mktime(&tests[i].t);
#else
	ts = mkgmtime(&tests[i].t);
#endif
	if (ts != tests[i].result) {
	    failed = true;
	    (void)strftime(tbuf, sizeof(tbuf), "%F %T", &tests[i].t);
	    (void)printf("test %2d failed. "
			 "Time returned from: %s should be %lu (but was: %lu)\n",
			 i, tbuf, (unsigned long)tests[i].result,
			 (unsigned long)ts);
	}
    }
    return (int)failed;
}

/*@-longunsignedintegral*/
