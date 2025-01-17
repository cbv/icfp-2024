/*
 * Simplified BSD License
 *
 * Copyright (c) 1988-1989, Digital Equipment Corporation & INRIA.
 * Copyright (c) 1992-2016, Eligis
 * All rights reserved.
 *
 * Redistribution and  use in  source and binary  forms, with  or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions  of  source  code must  retain  the  above copyright
 *   notice, this list of conditions and the following disclaimer.
 * o Redistributions  in  binary form  must reproduce the above copyright
 *   notice, this list of conditions and  the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
 * "AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE  ARE DISCLAIMED. IN NO EVENT  SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES (INCLUDING,  BUT  NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS  INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY,  OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *      bigz.c : provides an implementation of "unlimited-precision"
 *               arithmetic for signed integers.
 */

/*
 * Several conventions are used in the commentary:
 *    A "BigZ" is the name for an arbitrary-precision signed integer.
 *    Capital letters (e.g., "Z") are used to refer to the value of BigZs.
 */

// TODO: Moved from header. Just inline it? -tom7
#define BZ_OPTIMIZE_PRINT

// what is this?! -tom7
#if     !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE        1
#endif
#if     !defined(_CRT_NONSTDC_NO_DEPRECATE)
#define _CRT_NONSTDC_NO_DEPRECATE       1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bignum/bigz.h"

// GCC complains a lot in this file. There may be an actual bug,
// but at least suppress it for the programming contest.
#pragma GCC diagnostic ignored "-Warray-bounds"

#define MaxInt(a, b)            (((a) < (b)) ? (b) : (a))
#define AbsInt(x)               (((x) >= 0) ? (x) : -(x))

#define BZMAXINT                ((BzInt)((~(BzUInt)0) >> 1))
#define BZMAXUINT               (~(BzUInt)0)

#define BzFreeIf(cond, ptr)     if (cond) BzFree(ptr)

/*
 *      See ./etc/hextable.c if you need to change BigHexToDigit tables.
 */

/*
 * ASCII character class table
 */
static const int BigHexToDigit[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1
};

#define CTOI(c) ((((unsigned int)c) < (unsigned int)127) \
                 ? BigHexToDigit[(unsigned int)c]        \
                 : -1)

static BzSign   BzGetOppositeSign(const BigZ z);

#if     defined(BZ_DEBUG)
static void     BzShowBits(BigNumDigit n);
static void     BzShowUnsignedInt(unsigned int n);

static void
BzShowBits(BigNumDigit n) {
        int     i;

        for (i = (int)(BN_DIGIT_SIZE - 1); i >= 0; i--) {
                if ((n & (BN_ONE << (unsigned int)i)) != 0) {
                        (void)printf("1");
                } else  {
                        (void)printf("0");
                }
        }
}

static void
BzShowUnsignedInt(unsigned int n) {
        /*
         * Deal with different integer sizes. This avoid to build format
         * string dynamically which results to potential buffer overflow.
         * This also makes splint happy.
         */

        size_t size = sizeof(n);

        switch (size) {
        case  2: (void)printf("%04x",  n); break;
        case  4: (void)printf("%08x",  n); break;
        case  8: (void)printf("%016x", n); break;
        case 16: (void)printf("%032x", n); break;
        default: (void)printf("%016x", n); break;
        }
}

#define BZ_INT_CHUNKS   ((unsigned int)8) /* Assume BigNumDigit < (8 * int) */

void
BnDebug(const char *m,
        const BzChar *bzstr,
        const BigNum n,
        BigNumLength nl,
        BzSign       sign) {
        BigNumLength l;
        BzChar       c;

        if (m != NULL) {
                (void)printf("%-20s\n", m);
        }

        (void)printf("\t: BigZ = %s at 0x%p, ", (char *)bzstr, n);
        (void)printf("digit = %d, word = %d bytes\n",
                      (int)BN_DIGIT_SIZE,
                      (int)sizeof(void *));
        (void)printf("\t: <-- high %02d digit(s) low -->\n", (int)nl);

        if (sign == BZ_ZERO) {
                c = (BzChar)'0';
        } else  if (sign == BZ_MINUS) {
                c = (BzChar)'-';
        } else  {
                c = (BzChar)'+';
        }

        (void)printf("      %c : ", c);

        for (l = nl; l-- != 0;) {
                BigNumDigit d = n[l];
                unsigned int dsize = (unsigned int)sizeof(d);
                unsigned int isize = (unsigned int)sizeof(dsize); /* int */

                (void)printf("|");

                if ((dsize > isize) && ((dsize / isize) <= BZ_INT_CHUNKS)) {
                        /*
                         * sizeof(BigNumDigit) > sizeof(int).
                         * (can be 64bit BigNumDigit and 32bit int).
                         * NOTE: this code also works if isize == dsize.
                         */
                        unsigned int chunk[BZ_INT_CHUNKS];
                        unsigned int mask;
                        unsigned int shift;
                        unsigned int i;

                        mask  = ~((unsigned int)0); /* 0xff..ff */
                        shift = (unsigned int)(isize * BN_BYTE_SIZE);

                        /*
                         *      Split BigNumDigit in int chunks.
                         */

                        for (i = 0; i < (dsize / isize); ++i) {
                                chunk[i] = (unsigned int)(d & mask);
                                d >>= shift;
                        }

                        while (i-- != 0) {
                                BzShowUnsignedInt(chunk[i]);
                        }
                } else  {
                        /*
                         * sizeof(BigNumDigit) <= sizeof(int).
                         */
                        BzShowUnsignedInt((unsigned int)d);
                }
        }
        (void)printf("|\n");

        (void)printf("      %c : ", c);

        for (l = nl; l-- != 0;) {
                (void)printf("|");
                BzShowBits(n[l]);
        }

        (void)printf("|\n");
}

void
BzDebug(const char *m, const BigZ y) {
        BzChar * s = BzToString(y, (BigNumDigit)10, 0);

        BnDebug(m, s, BzToBn(y), BzNumDigits(y), BzGetSign(y));
        BzFreeString(s);
}
#endif

const char *
BzVersion(void) {
        return (BZ_VERSION);
}

/*
 *      constants used by BzToString() and BzFromString()
 */

#define BZ_MIN_BASE     2
#define BZ_MAX_BASE     36

/*
 *      following table is computed using:
 *
 *      for (i = 1; i <= BZ_MAX_BASE; ++i) {
 *              printf("\t%16.16f, // log(%2d)\n", log((double)i), i);
 *      }
 */

static const double BzLog[] = {
        0.0000000000000000,
        0.0000000000000000, /* log(1) */
        0.6931471805599453, /* log(2) */
        1.0986122886681098, /* log(3) */
        1.3862943611198906, /* log(4) */
        1.6094379124341003, /* log(5) */
        1.7917594692280550, /* log(6) */
        1.9459101490553132, /* log(7) */
        2.0794415416798357, /* log(8) */
        2.1972245773362196, /* log(9) */
        2.3025850929940459, /* log(10) */
        2.3978952727983707, /* log(11) */
        2.4849066497880004, /* log(12) */
        2.5649493574615367, /* log(13) */
        2.6390573296152584, /* log(14) */
        2.7080502011022101, /* log(15) */
        2.7725887222397811, /* log(16) */
        2.8332133440562162, /* log(17) */
        2.8903717578961645, /* log(18) */
        2.9444389791664403, /* log(19) */
        2.9957322735539909, /* log(20) */
        3.0445224377234230, /* log(21) */
        3.0910424533583161, /* log(22) */
        3.1354942159291497, /* log(23) */
        3.1780538303479458, /* log(24) */
        3.2188758248682006, /* log(25) */
        3.2580965380214821, /* log(26) */
        3.2958368660043291, /* log(27) */
        3.3322045101752038, /* log(28) */
        3.3672958299864741, /* log(29) */
        3.4011973816621555, /* log(30) */
        3.4339872044851463, /* log(31) */
        3.4657359027997265, /* log(32) */
        3.4965075614664802, /* log(33) */
        3.5263605246161616, /* log(34) */
        3.5553480614894135, /* log(35) */
        3.5835189384561099, /* log(36) */
};

BigZ
BzCreate(BigNumLength Size) {
        /*
         * Allocates a zeroed BigZ of the desired size.
         */

        BigZ    z;
        size_t  chunk;

        chunk = sizeof(BigZHeader) + Size * sizeof(BigNumDigit);

        if ((z = (BigZ)(BzAlloc(chunk))) != BZNULL) {
                /*
                 * reset digits
                 */

                BnnSetToZero(BzToBn(z), Size);

                /*
                 * init header
                 */

                BzSetSize(z, Size);
                BzSetSign(z, BZ_ZERO);
        }

        return (z);
}

BigNumLength BzNumDigits(const BigZ z) {
  /*
   * Returns the number of digits used by z.
   */
  BigNumLength sz = BzGetSize(z);
  return (BnnNumDigits(BzToBn(z), sz));
}

BigNumLength
BzLength(const BigZ z) {
        /*
         * Returns the number of bits used by z.
         */

        BigNumLength nl;

        switch (BzGetSign(z)) {
        case BZ_MINUS:
                nl = BnnNumLength(BzToBn(z), BzNumDigits(z));
                if (BnnIsPower2(BzToBn(z), BzNumDigits(z)) == BN_TRUE) {
                        return (nl - 1);
                } else  {
                        return (nl);
                }
        case BZ_PLUS:
                return (BnnNumLength(BzToBn(z), BzNumDigits(z)));
        default:
                return ((BigNumLength)0);
        }
}

BigZ
BzCopy(const BigZ z) {
        /*
         * Creates a copy of the passed BigZ.
         */

        BigZ            y;
        BigNumLength    zl;

        zl = BzNumDigits(z);

        if ((y = BzCreate(zl)) != BZNULL) {
                /*
                 * copy the digits
                 */
                BnnAssign(BzToBn(y), BzToBn(z), zl);

                /*
                 * copy the header WITHOUT the size !!
                 */
                BzSetSign(y, BzGetSign(z));
        }

        return (y);
}

static BzSign
BzGetOppositeSign(const BigZ z) {
        switch (BzGetSign(z)) {
        case BZ_MINUS:
                return (BZ_PLUS);
        case BZ_ZERO:
                return (BZ_ZERO);
        default:
                return (BZ_MINUS);
        }
}

BigZ
BzNegate(const BigZ z) {
        /*
         * Negates the passed BigZ.
         */

        BigZ    y;

        if ((y = BzCopy(z)) != BZNULL) {
                switch (BzGetSign(z)) {
                case BZ_MINUS:
                        BzSetSign(y, BZ_PLUS);
                        break;
                case BZ_ZERO:
                        BzSetSign(y, BZ_ZERO);
                        break;
                case BZ_PLUS:
                        BzSetSign(y, BZ_MINUS);
                        break;
                }
        }

        return (y);
}

BigZ
BzAbs(const BigZ z) {
        /*
         * Takes the absolute value of the passed BigZ.
         */

        BigZ    y;

        if ((y = BzCopy(z)) != BZNULL) {
                if (BzGetSign(z) == BZ_MINUS) {
                        BzSetSign(y, BZ_PLUS);
                }
        }

        return (y);
}

BzCmp
BzCompare(const BigZ y, const BigZ z) {
        /*
         * Returns BZ_GT        if Y > Z,
         *         BZ_LT        if Y < Z,
         *         BZ_EQ        otherwise.
         */

        if (BzGetSign(y) > BzGetSign(z)) {
                return (BZ_GT);
        } else  if (BzGetSign(y) < BzGetSign(z)) {
                return (BZ_LT);
        } else  if (BzGetSign(y) == BZ_PLUS) {
                return ((BzCmp)BnnCompare(BzToBn(y), BzGetSize(y),
                                           BzToBn(z), BzGetSize(z)));
        } else  if (BzGetSign(y) == BZ_MINUS) {
                return ((BzCmp)BnnCompare(BzToBn(z), BzGetSize(z),
                                           BzToBn(y), BzGetSize(y)));
        } else  {
                return (BZ_EQ);
        }
}

BigZ
BzAdd(const BigZ y, const BigZ z) {
        /*
         * Returns Y + Z.
         */

        BigZ            n;
        BigNumLength    yl;
        BigNumLength    zl;

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);

        if (BzGetSign(y) == BzGetSign(z)) {
                /*
                 * Add magnitudes if signs are the same
                 */
                switch (BnnCompare(BzToBn(y), yl, BzToBn(z), zl)) {
                case BN_EQ:
                case BN_GT:     /* |Y| >= |Z| */
                        if ((n = BzCreate(yl + 1)) != BZNULL) {
                                BnnAssign(BzToBn(n), BzToBn(y), yl);
                                (void)BnnAdd(BzToBn(n),
                                             yl + 1,
                                             BzToBn(z),
                                             zl,
                                             BN_NOCARRY);
                                BzSetSign(n, BzGetSign(y));
                        }
                        break;
                default:        /* BN_LT: |Y| < |Z| */
                        if ((n = BzCreate(zl+1)) != BZNULL) {
                                BnnAssign(BzToBn(n), BzToBn(z), zl);
                                (void)BnnAdd(BzToBn(n),
                                             zl + 1,
                                             BzToBn(y),
                                             yl,
                                             BN_NOCARRY);
                                BzSetSign(n, BzGetSign(z));
                        }
                        break;
                }
        } else  {
                /*
                 * Subtract magnitudes if signs are different
                 */
                switch (BnnCompare(BzToBn(y), yl, BzToBn(z), zl)) {
                case BN_EQ:     /* Y = -Z */
                        n = BzCreate((BigNumLength)1);
                        break;
                case BN_GT:     /* |Y| > |Z| */
                        if ((n = BzCreate(yl)) != BZNULL) {
                            BnnAssign(BzToBn(n), BzToBn(y), yl);
                            (void)BnnSubtract(BzToBn(n),
                                               yl,
                                               BzToBn(z),
                                               zl,
                                               BN_CARRY);
                            BzSetSign(n, BzGetSign(y));
                        }
                        break;

                default:        /* BN_LT: |Y| < |Z| */
                        if ((n = BzCreate(zl)) != BZNULL) {
                            BnnAssign(BzToBn(n), BzToBn(z), zl);
                            (void)BnnSubtract(BzToBn(n),
                                               zl,
                                               BzToBn(y),
                                               yl,
                                               BN_CARRY);
                            BzSetSign(n, BzGetSign(z));
                        }
                        break;
                }
        }

        return (n);
}

BigZ
BzSubtract(const BigZ y, const BigZ z) {
        /*
         * Returns Y - Z.
         */

        BigZ    diff;

        if (y != z) {
                BzSetSign(z, BzGetOppositeSign(z));
                diff = BzAdd(y, z);
                BzSetSign(z, BzGetOppositeSign(z));
        } else  {
                diff = BzFromInteger(0);
        }

        return (diff);
}

BigZ
BzMultiply(const BigZ y, const BigZ z) {
        /*
         * Returns Y * Z.
         */

        BigZ            n;
        BigNumLength    yl;
        BigNumLength    zl;

        if (BzGetSign(y) == BZ_ZERO || BzGetSign(z) == BZ_ZERO) {
                return (BzFromInteger(0));
        }

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);

        if ((n = BzCreate(yl + zl)) == BZNULL) {
                return BZNULL;
        }

        (void)BnnMultiply(BzToBn(n), yl + zl, BzToBn(y), yl, BzToBn(z), zl);

        if (BzGetSign(y) == BzGetSign(z)) {
                BzSetSign(n, BZ_PLUS);
        } else  {
                BzSetSign(n, BZ_MINUS);
        }

        return (n);
}

BigZ
BzDivide(const BigZ y, const BigZ z, BigZ *r) {
        /*
         * Returns Y div Z => Q
         * Sets    Y mod Z => R,
         *
         * such that Y = ZQ + R
         */

        BigZ            q;
        BigNumLength    yl;
        BigNumLength    zl;
        BigNumLength    ql;
        BigNumLength    rl;

        if (BzGetSign(z) == BZ_ZERO) {
                return BZNULL;
        }

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);
        ql = (BigNumLength)MaxInt((int)yl - (int)zl + 1, 1) + 1;
        rl = (BigNumLength)MaxInt(zl, yl) + 1;

        /*
         * Set up quotient, remainder
         */

        if ((q = BzCreate(ql)) == BZNULL) {
                return BZNULL;
        }

        if ((*r = BzCreate(rl)) == BZNULL) {
                BzFree(q);
                return BZNULL;
        }

        BnnAssign(BzToBn(*r), BzToBn(y), yl);

        /*
         * Do the division
         */

        BnnDivide(BzToBn(*r), rl, BzToBn(z), zl);
        BnnAssign(BzToBn(q), BzToBn(*r) + zl, rl - zl);
        BnnSetToZero(BzToBn(*r) + zl, rl - zl);
        rl = zl;

        /*
         * Correct the signs, adjusting the quotient and remainder
         */

        if (BnnIsZero(BzToBn(*r), rl) == BN_FALSE) {
                /*
                 * R<>0
                 */
                if (BzGetSign(y) != BzGetSign(z)) {
                        /*
                         * (Z-R) => R
                         */
                        BnnComplement(BzToBn(*r), rl);
                        (void)BnnAdd(BzToBn(*r), rl, BzToBn(z), zl, BN_CARRY);
                        /*
                         * (Q+1) => Q
                         */
                        (void)BnnAddCarry(BzToBn(q), ql, BN_CARRY);
                }
                /*
                 * The sign of the result (mod) is the sign of z.
                 */
                BzSetSign(*r, BzGetSign(z));
        } else  {
                /*
                 * Set sign to BZ_ZERO
                 * (already made by BzCreate but makes it clear)
                 */
                BzSetSign(*r, BZ_ZERO);
        }

        /*
         * Set the sign of the quotient.
         */

        if (BnnIsZero(BzToBn(q), ql) == BN_TRUE) {
                BzSetSign(q, BZ_ZERO);
        } else  if (BzGetSign(y) == BzGetSign(z)) {
                BzSetSign(q, BZ_PLUS);
        } else  {
                BzSetSign(q, BZ_MINUS);
        }

        return (q);
}

BigZ
BzDiv(const BigZ y, const BigZ z) {
        /*
         * Returns div(Y, Z).
         */

        BigZ    q;
        BigZ    r = BZNULL;

        q = BzDivide(y, z, &r);

        if (r != BZNULL) {
                BzFree(r);
        }

        return (q);
}

BigZ
BzTruncate(const BigZ y, const BigZ z) {
        /*
         * Returns truncate(Y, Z).
         */

        BigZ            q;
        BigZ            r = BZNULL;
        BigNumLength    ql;

        q  = BzDivide(y, z, &r);

        if (r == BZNULL) {
                /*
                 * This should never happen.
                 */
                BzFree(q);
                return BZNULL;
        }

        ql = BzNumDigits(q);

        if (BzGetSign(q) == BZ_MINUS && BzGetSign(r) != BZ_ZERO) {
                /*
                 *      Q < 0, R <> 0, 2*R>= Z : Q-1 => Q
                 */
                (void)BnnSubtractBorrow(BzToBn(q), ql, BN_NOCARRY);

                if (BnnIsZero(BzToBn(q), ql) == BN_TRUE) {
                        BzSetSign(q, BZ_ZERO);
                }
        } else  if (BnnIsZero(BzToBn(q), ql) == BN_TRUE &&
                    BzGetSign(y) == BzGetSign(z)) {
                /*
                 *      Q == 0, sign(Y) == sign(Z) : 0 => Q
                 */
                BzFree(q);
                q = BzFromInteger(0);
        }

        BzFree(r);

        return (q);
}

BigZ
BzFloor(const BigZ y, const BigZ z) {
        /*
         * Returns floor(Y, Z).
         */

        BigZ    q;
        BigZ    r = BZNULL;

        q = BzDivide(y, z, &r);

        if (r != BZNULL) {
                BzFree(r);
        }

        return (q);
}

BigZ
BzCeiling(const BigZ y, const BigZ z) {
        /*
         * Returns ceiling(Y, Z).
         */

        BigZ            q;
        BigZ            r = BZNULL;
        BigNumLength    ql;

        if ((q = BzDivide(y, z, &r)) == BZNULL) {
                return BZNULL;
        }

        if (r == BZNULL) {
                /*
                 * This should never happen.
                 */
                BzFree(q);
                return BZNULL;
        }

        ql = BzNumDigits(q);

        if (BzGetSign(q) == BZ_PLUS && BzGetSign(r) != BZ_ZERO) {
                /*
                 *      Q > 0, R <> 0 : Q+1 => Q
                 */
                BigNumDigit one = BN_ONE;
                (void)BnnAdd(BzToBn(q), ql, &one, (BigNumLength)1, BN_NOCARRY);
        } else  if (BzGetSign(q) == BZ_MINUS && BzGetSign(r) != BZ_ZERO) {
                /*
                 *      Q < 0, R <> 0 : Q-1 => Q
                 */
                (void)BnnSubtractBorrow(BzToBn(q), ql, BN_NOCARRY);
                if (BnnIsZero(BzToBn(q), ql) == BN_TRUE) {
                        BzSetSign(q, BZ_ZERO);
                }
        } else  if (BzGetSign(q) == BZ_ZERO
                    && BzGetSign(y) == BzGetSign(z)) {
                /*
                 *      Q == 0, sign(Y) == sign(Z) : 1 => Q
                 */
                BzFree(q);
                q = BzFromInteger((BzInt)1);
        }

        BzFree(r);

        return (q);
}

BigZ
BzRound(const BigZ y, const BigZ z) {
        /*
         * Returns round(Y, Z).
         */

        BigZ            q;
        BigZ            r = BZNULL;
        BigNumLength    ql;

        if ((q = BzDivide(y, z, &r)) == BZNULL) {
                return BZNULL;
        }

        if (r == BZNULL) {
                /*
                 * This should never happen.
                 */
                BzFree(q);
                return BZNULL;
        }

        ql = BzNumDigits(q);

        if (BzGetSign(q) == BZ_PLUS && BzGetSign(r) != BZ_ZERO) {
                BigNumDigit one = BN_ONE;
                BigZ        roundz;
                BzSign      sign = BzGetSign(z);

                BzSetSign(r, BZ_PLUS);
                BzSetSign(z, BZ_PLUS);

                roundz = BzAsh(r, 1);

                switch (BzCompare(roundz, z)) {
                case BZ_LT:
                        break;
                case BZ_EQ:
                        /*
                         *      Q > 0, R <> 0, 2*R= Z :
                         *
                         * roundz is exactly halfway between two integers,
                         * choose even number.
                         */
                        if (BzIsOdd(q) == BN_TRUE) {
                                (void)BnnAdd(BzToBn(q),
                                              ql,
                                              &one,
                                              (BigNumLength)1,
                                              BN_NOCARRY);
                        }
                        break;
                case BZ_GT:
                        /*
                         *      Q > 0, R <> 0, 2*R>= Z : Q+1 => Q
                         */
                        (void)BnnAdd(BzToBn(q),
                                      ql,
                                      &one,
                                      (BigNumLength)1,
                                      BN_NOCARRY);
                        break;
                }

                BzSetSign(z, sign);
                BzFree(roundz);
        } else  if (BzGetSign(q) == BZ_MINUS && BzGetSign(r) != BZ_ZERO) {
                /*
                 *      Q < 0, R <> 0, 2*R>= Z : Q-1 => Q
                 */
                BigZ    roundz;
                BzSign  sign = BzGetSign(z);

                BzSetSign(r, BZ_PLUS);
                BzSetSign(z, BZ_PLUS);

                roundz = BzAsh(r, 1);

                switch (BzCompare(roundz, z)) {
                case BZ_LT:
                        break;
                case BZ_EQ:
                        /*
                         * roundz is exactly halfway between two integers,
                         * choose even number.
                         */
                        if (BzIsOdd(q) == BN_TRUE) {
                                (void)BnnSubtractBorrow(BzToBn(q),
                                                         ql,
                                                         BN_NOCARRY);
                        }
                        break;
                case BZ_GT:
                        (void)BnnSubtractBorrow(BzToBn(q), ql, BN_NOCARRY);
                        break;
                }

                BzSetSign(z, sign);
                BzFree(roundz);

                if (BnnIsZero(BzToBn(q), ql) == BN_TRUE) {
                        BzSetSign(q, BZ_ZERO);
                }
        } else  if ((BzGetSign(q) == BZ_ZERO)
                    && (BzGetSign(y) == BzGetSign(z))) {
                /*
                 *      Q == 0, sign(Y) == sign(Z):
                 */
                BigZ    roundz;
                BzSign  sign = BzGetSign(z);

                BzFree(q);

                BzSetSign(r, BZ_PLUS);
                BzSetSign(z, BZ_PLUS);

                roundz = BzAsh(r, 1);

                if (BzCompare(roundz, z) == BZ_LT) {
                        /*
                         *      2*R< Z : 0 => Q
                         */
                        q = BzFromInteger(0);
                } else  {
                        /*
                         *      2*R>= Z : 1 => Q
                         */
                        q = BzFromInteger((BzInt)1);
                }

                BzSetSign(z, sign);
                BzFree(roundz);
        }

        BzFree(r);

        return (q);
}

BigZ
BzMod(const BigZ y, const BigZ z) {
        /*
         * Returns Y mod Z.
         */

        BigZ    r = BZNULL;

        BzFree(BzDivide(y, z, &r));

        return (r);
}

BigZ
BzRem(const BigZ y, const BigZ z) {
        /*
         * Returns Y rem Z.
         */

        BigZ q;
        BigZ r = BZNULL;
        BigZ rem;

        if ((q = BzDivide(y, z, &r)) != BZNULL) {
                BzFree(q);

                if (r == BZNULL) {
                        /*
                         * This should never happen.
                         */
                        return BZNULL;
                }

                if (BzGetSign(r) == BZ_ZERO) {
                        return (r);
                } else  if (BzGetSign(y) == BzGetSign(z)) {
                        return (r);
                } else  if (BzGetSign(y) == BZ_MINUS) {
                        if ((rem = BzSubtract(z, r)) != BZNULL) {
                                BzSetSign(rem, BZ_MINUS);
                        }
                        BzFree(r);
                        return (rem);
                } else  {
                        if ((rem = BzSubtract(r, z)) != BZNULL) {
                                BzSetSign(rem, BZ_PLUS);
                        }
                        BzFree(r);
                        return (rem);
                }
        } else  {
                return BZNULL;
        }
}

BigNumBool
BzIsEven(const BigZ y) {
        /*
         * Returns BN_TRUE iff y is even
         */

        return (BnnIsDigitEven(BzGetDigit(y, 0)));
}

BigNumBool
BzIsOdd(const BigZ y) {
        /*
         * Returns BN_TRUE iff y is odd
         */

        return (BnnIsDigitOdd(BzGetDigit(y, 0)));
}

#if     defined(BZ_OPTIMIZE_PRINT)
typedef struct {
        int MaxDigits;
        BigNumDigit  MaxValue;
} BzPrintTable;

#if (BZ_BUCKET_SIZE == 32)
static const BzPrintTable BzPrintBase[] = {
  {  0, (BigNumDigit)0U                     }, /*  0 */
  {  0, (BigNumDigit)0U                     }, /*  1 */
  { 31, (BigNumDigit)2147483648U            }, /*  2 */
  { 20, (BigNumDigit)3486784401U            }, /*  3 */
  { 15, (BigNumDigit)1073741824U            }, /*  4 */
  { 13, (BigNumDigit)1220703125U            }, /*  5 */
  { 12, (BigNumDigit)2176782336U            }, /*  6 */
  { 11, (BigNumDigit)1977326743U            }, /*  7 */
  { 10, (BigNumDigit)1073741824U            }, /*  8 */
  { 10, (BigNumDigit)3486784401U            }, /*  9 */
  {  9, (BigNumDigit)1000000000U            }, /* 10 */
  {  9, (BigNumDigit)2357947691U            }, /* 11 */
  {  8, (BigNumDigit)429981696U             }, /* 12 */
  {  8, (BigNumDigit)815730721U             }, /* 13 */
  {  8, (BigNumDigit)1475789056U            }, /* 14 */
  {  8, (BigNumDigit)2562890625U            }, /* 15 */
  {  7, (BigNumDigit)268435456U             }, /* 16 */
  {  7, (BigNumDigit)410338673U             }, /* 17 */
  {  7, (BigNumDigit)612220032U             }, /* 18 */
  {  7, (BigNumDigit)893871739U             }, /* 19 */
  {  7, (BigNumDigit)1280000000U            }, /* 20 */
  {  7, (BigNumDigit)1801088541U            }, /* 21 */
  {  7, (BigNumDigit)2494357888U            }, /* 22 */
  {  7, (BigNumDigit)3404825447U            }, /* 23 */
  {  6, (BigNumDigit)191102976U             }, /* 24 */
  {  6, (BigNumDigit)244140625U             }, /* 25 */
  {  6, (BigNumDigit)308915776U             }, /* 26 */
  {  6, (BigNumDigit)387420489U             }, /* 27 */
  {  6, (BigNumDigit)481890304U             }, /* 28 */
  {  6, (BigNumDigit)594823321U             }, /* 29 */
  {  6, (BigNumDigit)729000000U             }, /* 30 */
  {  6, (BigNumDigit)887503681U             }, /* 31 */
  {  6, (BigNumDigit)1073741824U            }, /* 32 */
  {  6, (BigNumDigit)1291467969U            }, /* 33 */
  {  6, (BigNumDigit)1544804416U            }, /* 34 */
  {  6, (BigNumDigit)1838265625U            }, /* 35 */
  {  6, (BigNumDigit)2176782336U            }  /* 36 */
};
#endif /* BZ_BUCKET_SIZE == 32 */

#if (BZ_BUCKET_SIZE == 64)
static const BzPrintTable BzPrintBase[] = {
  {  0, (BigNumDigit)0UL                    }, /*  0 */
  {  0, (BigNumDigit)0UL                    }, /*  1 */
  { 63, (BigNumDigit)9223372036854775808UL  }, /*  2 */
  { 40, (BigNumDigit)12157665459056928801UL }, /*  3 */
  { 31, (BigNumDigit)4611686018427387904UL  }, /*  4 */
  { 27, (BigNumDigit)7450580596923828125UL  }, /*  5 */
  { 24, (BigNumDigit)4738381338321616896UL  }, /*  6 */
  { 22, (BigNumDigit)3909821048582988049UL  }, /*  7 */
  { 21, (BigNumDigit)9223372036854775808UL  }, /*  8 */
  { 20, (BigNumDigit)12157665459056928801UL }, /*  9 */
  { 19, (BigNumDigit)10000000000000000000UL }, /* 10 */
  { 18, (BigNumDigit)5559917313492231481UL  }, /* 11 */
  { 17, (BigNumDigit)2218611106740436992UL  }, /* 12 */
  { 17, (BigNumDigit)8650415919381337933UL  }, /* 13 */
  { 16, (BigNumDigit)2177953337809371136UL  }, /* 14 */
  { 16, (BigNumDigit)6568408355712890625UL  }, /* 15 */
  { 15, (BigNumDigit)1152921504606846976UL  }, /* 16 */
  { 15, (BigNumDigit)2862423051509815793UL  }, /* 17 */
  { 15, (BigNumDigit)6746640616477458432UL  }, /* 18 */
  { 15, (BigNumDigit)15181127029874798299UL }, /* 19 */
  { 14, (BigNumDigit)1638400000000000000UL  }, /* 20 */
  { 14, (BigNumDigit)3243919932521508681UL  }, /* 21 */
  { 14, (BigNumDigit)6221821273427820544UL  }, /* 22 */
  { 14, (BigNumDigit)11592836324538749809UL }, /* 23 */
  { 13, (BigNumDigit)876488338465357824UL   }, /* 24 */
  { 13, (BigNumDigit)1490116119384765625UL  }, /* 25 */
  { 13, (BigNumDigit)2481152873203736576UL  }, /* 26 */
  { 13, (BigNumDigit)4052555153018976267UL  }, /* 27 */
  { 13, (BigNumDigit)6502111422497947648UL  }, /* 28 */
  { 13, (BigNumDigit)10260628712958602189UL }, /* 29 */
  { 13, (BigNumDigit)15943230000000000000UL }, /* 30 */
  { 12, (BigNumDigit)787662783788549761UL   }, /* 31 */
  { 12, (BigNumDigit)1152921504606846976UL  }, /* 32 */
  { 12, (BigNumDigit)1667889514952984961UL  }, /* 33 */
  { 12, (BigNumDigit)2386420683693101056UL  }, /* 34 */
  { 12, (BigNumDigit)3379220508056640625UL  }, /* 35 */
  { 12, (BigNumDigit)4738381338321616896UL  }  /* 36 */
};
#endif /* BZ_BUCKET_SIZE == 64 */
#endif /* BZ_OPTIMIZE_PRINT */

BzChar *
BzToString(const BigZ z, BigNumDigit base, int sign) {
        /*
         * wrapper to BzToStringBuffer that always allocate buffer.
         */
        return (BzToStringBuffer(z, base, sign, (BzChar *)0, (size_t *)0));
}

BzChar *
BzToStringBuffer(const BigZ z,
                 BigNumDigit base,
                 int sign,
                 BzChar * const buf,
                 size_t *len) {
        return (BzToStringBufferExt(z,
                                    base,
                                    sign,
                                    buf,
                                    len,
                                    (size_t *)0));
}

BzChar *
BzToStringBufferExt(const BigZ z,
                    BigNumDigit base,
                    int sign,
                    BzChar * const buf,
                    size_t *len,
                    size_t *slen) {
        /*
         * Returns  a  pointer  to  a string that represents Z in the
         * specified   base.   Assumes   BZ_MIN_BASE   <=   base   <=
         * BZ_MAX_BASE.  If  optional  buffer  is supplied,  len is a
         * pointer  of  this buffer size.  If there is enough room to
         * print  the  number  buf is used otherwise function returns
         * NULL and len contains the required size.  If buf is passed
         * as NULL,  this string is allocated on the heap, so it must
         * be deallocated by the user.
         */

        static const BzChar Digit[] = {
                (BzChar)'0', (BzChar)'1', (BzChar)'2', (BzChar)'3',
                (BzChar)'4', (BzChar)'5', (BzChar)'6', (BzChar)'7',
                (BzChar)'8', (BzChar)'9', (BzChar)'a', (BzChar)'b',
                (BzChar)'c', (BzChar)'d', (BzChar)'e', (BzChar)'f',
                (BzChar)'g', (BzChar)'h', (BzChar)'i', (BzChar)'j',
                (BzChar)'k', (BzChar)'l', (BzChar)'m', (BzChar)'n',
                (BzChar)'o', (BzChar)'p', (BzChar)'q', (BzChar)'r',
                (BzChar)'s', (BzChar)'t', (BzChar)'u', (BzChar)'v',
                (BzChar)'w', (BzChar)'x', (BzChar)'y', (BzChar)'z'
        };

        BigZ            y;
        BigZ            q;
        BigNumLength    zl;
        BigNumLength    sl;
        BzChar *        s;
        BzChar *        strg;

        if (base < (BigNumDigit)BZ_MIN_BASE
            || base > (BigNumDigit)BZ_MAX_BASE) {
                if (len != 0) {
                        *len = 0;
                }
                return ((BzChar *)NULL);
        }

        /*
         * Allocate BigNums and set up string
         */

        zl = BzNumDigits(z) + 1;
        sl = (BigNumLength)((BzLog[2] * BN_DIGIT_SIZE * zl) / BzLog[base] + 3);

        if (buf != (BzChar *)NULL
            && len != (size_t *)NULL
            && (sl > (BigNumLength)*len)) {
                /*
                 * a buffer is passed but there is not enough room,
                 * return NULL and set required size in len.
                 */
                *len = (size_t)sl;
                return ((BzChar *)NULL);
        }

        if (len != (size_t *)NULL) {
                /*
                 * set len to 0 in case BzToStringBuffer returns NULL
                 * because of allocation failure. It should be checked by
                 * caller that may otherwise allocate a bigger buffer.
                 */
                *len = 0;
        }

        if ((y = BzCreate(zl)) == BZNULL) {
                return ((BzChar *)NULL);
        }

        if ((q = BzCreate(zl)) == BZNULL) {
                BzFree(y);
                return ((BzChar *)NULL);
        }

        if (buf != (BzChar *)NULL) {
                strg = buf;
        } else  {
                strg = (BzChar *)BzStringAlloc((size_t)sl);
                if (strg == (BzChar *)NULL) {
                        BzFree(y);
                        BzFree(q);
                        return ((BzChar *)NULL);
                }
                if (len != (size_t *)NULL) {
                        /*
                         * a buffer is allocated and caller wants to know
                         * allocated size.
                         */
                        *len = (size_t)sl;
                }
        }

        BnnAssign(BzToBn(y), BzToBn(z), zl - 1);
        s = strg + sl;

        /*
         * Divide Z by base repeatedly; successive digits given by remainders
         */

        *--s = (BzChar)'\0';

        if (BzGetSign(z) == BZ_ZERO) {
                *--s = (BzChar)'0';
#if     defined(BZ_OPTIMIZE_PRINT)
        } else  {
                /*
                 * Compute maxval and digits that can be used with
                 * this base.
                 */

                BigNumDigit  maxval = (BigNumDigit)BzPrintBase[base].MaxValue;
                BigNumLength digits = (BigNumLength)BzPrintBase[base].MaxDigits;

                /*
                 * This optimization makes BigZ output 10 to 20x faster.
                 */
                do {
                        BigZ            v;
                        BigNumDigit     r;
                        /*
                         * compute: y div maxval => q,
                         * returns r = y mod maxval
                         *
                         * maxval is the greatest integer in base 'base'
                         * that fits in a BigNumDigit.
                         */

                        r = BnnDivideDigit(BzToBn(q),
                                           BzToBn(y),
                                           zl,
                                           maxval);

                        if (BnnIsZero(BzToBn(q), zl) == BN_FALSE) {
                                /*
                                 * More digits to come on left, add exactly
                                 * the number of digits with possible
                                 * leading 0 (when r becomes 0).
                                 */
                                int     i;
                                for (i = 0; i < (int)digits; ++i) {
                                        if (r == 0) {
                                                /*
                                                 * No need to divide, fill
                                                 * the rest with '0'.
                                                 */
                                                *--s = (BzChar)'0';
                                        } else {
                                                *--s = Digit[r % base];
                                                r = r / base;
                                        }
                                }
                        } else  {
                                /*
                                 * Last serie (top left). Print only available
                                 * digits (stop when r becomes 0).
                                 */
                                while (r != 0) {
                                        *--s = Digit[r % base];
                                        r = r / base;
                                }
                        }

                        /*
                         * exchange y and q (to avoid BzMove(y, q))
                         */

                        v = q;
                        q = y;
                        y = v;
                } while (BnnIsZero(BzToBn(y), zl) == BN_FALSE);
        }
#else   /* BZ_OPTIMIZE_PRINT */
        } else  do {
                BigZ            v;
                BigNumDigit     r;
                /* compute: y div base => q, returns r = y mod base */

                r = BnnDivideDigit(BzToBn(q), BzToBn(y), zl, base);
                *--s = Digit[r];

                /*
                 * exchange y and q (to avoid BzMove(y, q))
                 */

                v = q;
                q = y;
                y = v;
        } while (BnnIsZero(BzToBn(y), zl) == BN_FALSE);
#endif  /* BZ_OPTIMIZE_PRINT */

        /*
         * Add sign if needed.
         */

        switch (BzGetSign(z)) {
        case BZ_MINUS:
                /*
                 * z < 0, always add '-' sign.
                 */
                *--s = (BzChar)'-';
                break;
        case BZ_ZERO:
                /*
                 * z = 0 -> no sign even if BZ_FORCE_SIGN is passed.
                 */
                break;
        case BZ_PLUS:
                /*
                 * z > 0, add '+' only if sign is required.
                 */
                if (sign == BZ_FORCE_SIGN) {
                        *--s = (BzChar)'+';
                }
        }

#if 0
        /*
         * and move string into position
         */

        if ((s - strg) > 0) {
                BigNumLength i;

                for (i = 0; s[i] != (BzChar)'\000'; ++i) {
                        strg[i] = s[i];
                }

                strg[i] = (BzChar)'\000';
        }
#else
        if (buf != (BzChar *)NULL) {
                strg = &s[0];
        } else if ((s - strg) > 0) {
               /*
                * and move string into position as no buffer
                * has been supplied.
                */
                BigNumLength i;

                for (i = 0; s[i] != (BzChar)'\000'; ++i) {
                        strg[i] = s[i];
                }

                strg[i] = (BzChar)'\000';
        }
#endif

        /*
         * Free temporary BigNums and return the string
         */

        BzFree(y);
        BzFree(q);

        if (slen != (size_t *)NULL) {
                /*
                 * A non null pointer was passed to get string length
                 * (which is not the same a buffer length used to build string).
                 */
                *slen = (size_t)(strg + sl - s - 1);
        }

        return (strg);
}

size_t
BzStrLen(const BzChar *s) {
        size_t  len;

        for (len = (size_t)0; *s++ != (BzChar)'\000'; ++len) {
                continue;
        }

        return (len);
}

BigZ
BzFromStringLen(const BzChar *s, size_t len, BigNumDigit base, BzStrFlag flag) {
        /*
         * Creates  a  BigZ whose value is represented by "string" in
         * the  specified  base.  The  "string"  may  contain leading
         * spaces,  followed  by  an  optional  sign,  followed  by a
         * series of digits. Assumes BZ_MIN_BASE<=base <=BZ_MAX_BASE.
         * When called from C, only the first 2 arguments are passed.
         */

        BigZ            z;
        BigZ            p;
        BzSign          sign;
        BigNumLength    zl;
        size_t          i;

        /*
         * Throw away any initial space
         */

        while ((*s == (BzChar)' ')
               || (*s == (BzChar)'\t')
               || (*s == (BzChar)'\n')
               || (*s == (BzChar)'\r')) {
                ++s;
                --len;
        }

        /*
         * Set up sign, base, initialize result
         */

        switch (*s) {
        case ((BzChar)'-'):
                sign = BZ_MINUS;
                ++s;
                --len;
                break;
        case ((BzChar)'+'):
                sign = BZ_PLUS;
                ++s;
                --len;
                break;
        default:
                sign = BZ_PLUS;
        }

        /*
         * Allocate BigNums
         */

        zl = (BigNumLength)(len*BzLog[base] / (BzLog[2]*BN_DIGIT_SIZE)+1);

        if ((z = BzCreate(zl)) == BZNULL) {
                return BZNULL;
        }

        if ((p = BzCreate(zl)) == BZNULL) {
                BzFree(z);
                return BZNULL;
        }

        /*
         * Multiply in the digits of the string, one at a time
         */

        for (i = 0; i < len; ++i) {
                BigZ        v;
                int         val  = CTOI(s[i]);
                BigNumDigit next = (BigNumDigit)val;

                if (val == -1 || next >= base) {
                        if (flag == BZ_UNTIL_INVALID) {
                                break;
                        }
                        /*
                         * Invalid syntax for base.
                         */
                        BzFree(p);
                        BzFree(z);
                        return BZNULL;
                }

                BnnSetToZero(BzToBn(p), zl);
                BnnSetDigit(BzToBn(p), next);
                (void)BnnMultiplyDigit(BzToBn(p), zl, BzToBn(z), zl, base);

                /*
                 * exchange z and p (to avoid BzMove (z, p)
                 */

                v = p;
                p = z;
                z = v;
        }

        /*
         * Set sign of result
         */

        BzSetSign(z, (BnnIsZero(BzToBn(z), zl) == BN_TRUE) ? BZ_ZERO : sign);

        /*
         * Free temporary BigNums
         */

        BzFree(p);

        return (z);
}

BigZ
BzFromString(const BzChar *s, BigNumDigit base, BzStrFlag flag) {
        return (BzFromStringLen(s, BzStrLen(s), base, flag));
}

BigZ
BzFromInteger(BzInt i) {
        BigZ    z;

        z = BzCreate((BigNumLength)1);

        if (z != BZNULL) {
                BzSetDigit(z, 0, (BigNumDigit)AbsInt(i));

                if (i > 0) {
                        BzSetSign(z, BZ_PLUS);
                } else  if (i < 0) {
                        BzSetSign(z, BZ_MINUS);
                } else  {
                        BzSetSign(z, BZ_ZERO);
                }
        }

        return (z);
}

BigZ
BzFromUnsignedInteger(BzUInt i) {
        BigZ    z;

        z = BzCreate((BigNumLength)1);

        if (z != BZNULL) {
                BzSetDigit(z, 0, (BigNumDigit)i);
                BzSetSign(z, ((i > (BzInt)0) ? BZ_PLUS : BZ_ZERO));
        }

        return (z);
}

BzInt
BzToInteger(const BigZ z) {
        if (BzNumDigits(z) > (BigNumLength)1) {
                return (BZMAXINT);
        } else  if (BzGetSign(z) == BZ_MINUS) {
                return (- (BzInt)BzGetDigit(z, 0));
        } else  {
                return ((BzInt)BzGetDigit(z, 0));
        }
}

int
BzToIntegerPointer(const BigZ z, BzInt *p) {
        BzInt value;

        if (BzNumDigits(z) > (BigNumLength)1) {
                *p = BZMAXINT;
                return (0);
        }

        value = (BzInt)BzGetDigit(z, 0);

        if (value < 0) {
                *p = BZMAXINT;
                return (0);
        } else  if (BzGetSign(z) == BZ_MINUS) {
                *p = -value;
        } else  {
                *p =  value;
        }

        return (1);
}

BzUInt
BzToUnsignedInteger(const BigZ z) {
        if (BzNumDigits(z) > (BigNumLength)1) {
                return (BZMAXUINT);
        } else  {
                return ((BzUInt)BzGetDigit(z, 0));
        }
}

int
BzToUnsignedIntegerPointer(const BigZ z, BzUInt *p) {
        if (BzNumDigits(z) > (BigNumLength)1) {
                *p = BZMAXUINT;
                return (0);
        } else  {
                *p = (BzUInt)BzGetDigit(z, 0);

                return (1);
        }
}

BigZ
BzFromBigNum(const BigNum n, BigNumLength nl) {
        BigZ            z;
        BigNumLength    i;

        z = BzCreate(nl);

        if (z != BZNULL) {
                /*
                 * set the sign of z such that the pointer n is unchanged yet
                 */

                if (BnnIsZero(n, nl) == BN_TRUE) {
                        BzSetSign(z, BZ_ZERO);
                } else  {
                        BzSetSign(z, BZ_PLUS);
                }

                for (i = 0; i < nl; ++i) {
                        BzSetDigit(z, i, n[i]);
                }
        }

        return (z);
}

BigNum
BzToBigNum(const BigZ z, BigNumLength *nl) {
        BigNum          n;
        BigNum          m;
        BigNumLength    i;
        size_t          size;

        if (BzGetSign(z) == BZ_MINUS) {
                return ((BigNum)NULL);
        }

        *nl  = BzNumDigits(z);
        size = ((size_t)(*nl + 1)) * sizeof(BigNumDigit);

        if ((n = (BigNum)(BzAlloc(size))) != NULL) {
                *n = (BigNumDigit)*nl; /* set size */

                for (i = 0, m = ++n; i < *nl; i++, m++) {
                        *m = BzGetDigit(z, i);
                }

        }

        return (n);
}

/*
 * The  logical  operations  provide a convenient way to represent an
 * infinite  vector of bits.  Let such a conceptual vector be indexed
 * by the non-negative integers.  Then bit j is assigned a ``weight''
 * 2**j.  Assume  that only a finite number of bits are 1's or only a
 * finite number of bits are 0's.  A vector with only a finite number
 * of  one-bits  is  represented  as  the  sum  of the weights of the
 * one-bits,  a positive integer.  A vector with only a finite number
 * of  zero-bits is represented as -1 minus the sum of the weights of
 * the zero-bits, a negative integer.
 *
 * This  method  of  using  integers  to represent bit-vectors can in
 * turn  be  used  to  represent  sets.  Suppose  that some (possibly
 * countably  infinite) universe of discourse for sets is mapped into
 * the non-negative integers.  Then a set can be represented as a bit
 * vector;  an  element  is  in  the  set  if  the  bit  whose  index
 * corresponds  to that element is a one-bit.  In this way all finite
 * sets  can  be  represented (by positive integers),  as well as all
 * sets whose complements are finite (by negative integers).
 */

#define BZ_SIGN1        0x1     /* 01 */
#define BZ_SIGN2        0x2     /* 10 */

BigZ
BzNot(const BigZ z) {
        /*
         * Negates the passed BigZ.
         */

        BigZ    y;

        switch (BzGetSign(z)) {
        case BZ_MINUS:
             if ((y = BzCopy(z)) != BZNULL) {
               BnnComplement2(BzToBn(y), BzNumDigits(y));
               BnnComplement(BzToBn(y), BzNumDigits(y));
               if (BnnIsZero(BzToBn(y), BzNumDigits(y)) == BN_TRUE) {
                 /*
                  * ~(-1) -> 0
                  */
                 BzSetSign(y, BZ_ZERO);
               } else {
                 BzSetSign(y, BZ_PLUS);
               }
             }
             break;
        case BZ_ZERO:
             y = BzFromInteger((BzInt)-1);
             break;
        default: /* case BZ_PLUS: */
             if ((y = BzCopy(z)) != BZNULL) {
                     BnnComplement(BzToBn(y), BzNumDigits(y));
                     BnnComplement2(BzToBn(y), BzNumDigits(y));
                     BzSetSign(y, BZ_MINUS);
             }
             break;
        }

        return (y);
}

BigZ
BzAnd(const BigZ y, const BigZ z) {
        /*
         * Returns Y & Z.
         */

        BigZ            n;
        BigZ            yy;
        BigZ            zz;
        BigNumLength    yl;
        BigNumLength    zl;
        BigNumLength    l;
        unsigned int    sign = 0;

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);

        if (BzGetSign(y) == BZ_MINUS) {
                if ((yy = BzCopy(y)) == BZNULL) {
                        return BZNULL;
                }
                BnnComplement2(BzToBn(yy), yl);
                sign |= BZ_SIGN1;
        } else  {
                yy = y;
        }

        if (BzGetSign(z) == BZ_MINUS) {
                if ((zz = BzCopy(z)) == BZNULL) {
                        if ((sign & BZ_SIGN1) != 0) {
                                BzFree(yy);
                        }
                        return BZNULL;
                }
                BnnComplement2(BzToBn(zz), zl);
                sign |= BZ_SIGN2;
        } else  {
                zz = z;
        }

        if (yl < zl) {
                if ((n = BzCopy(zz)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN1) == 0) {
                                for (l = (zl - 1); l >= yl; --l) {
                                  BnnAndDigits(BzToBn(n) + l, BN_ZERO);
                                }
                        }
                        for (l = 0; l < yl; ++l) {
                          BnnAndDigits(BzToBn(n) + l, *(BzToBn(yy) + l));
                        }
                }
        } else  {
                if ((n = BzCopy(yy)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN2) == 0) {
                                for (l = (yl - 1); l >= zl; --l) {
                                  BnnAndDigits(BzToBn(n) + l, BN_ZERO);
                                }
                        }
                        for (l = 0; l < zl; ++l) {
                          BnnAndDigits(BzToBn(n) + l, *(BzToBn(zz) + l));
                        }
                }
        }

        if (n != BZNULL) {
                if (BnnIsZero(BzToBn(n), BzNumDigits(n)) == BN_TRUE) {
                        BzSetSign(n, BZ_ZERO);
                } else  if (sign == (unsigned int)(BZ_SIGN1 | BZ_SIGN2)) {
                        BnnComplement2(BzToBn(n), BzNumDigits(n));
                        BzSetSign(n, BZ_MINUS);
                }
        }

        /*
         *  Free copies.
         */

        if ((sign & BZ_SIGN1) != 0) {
                BzFree(yy);
        }

        if ((sign & BZ_SIGN2) != 0) {
                BzFree(zz);
        }

        return (n);
}

BigZ
BzOr(const BigZ y, const BigZ z) {
        /*
         * Returns Y | Z.
         */

        BigZ            n;
        BigZ            yy;
        BigZ            zz;
        BigNumLength    yl;
        BigNumLength    zl;
        BigNumLength    l;
        unsigned int    sign = 0;

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);

        if (BzGetSign(y) == BZ_MINUS) {
                if ((yy = BzCopy(y)) == BZNULL) {
                        return BZNULL;
                }
                BnnComplement2(BzToBn(yy), yl);
                sign |= BZ_SIGN1;
        } else  {
                yy = y;
        }

        if (BzGetSign(z) == BZ_MINUS) {
                if ((zz = BzCopy(z)) == BZNULL) {
                        if ((sign & BZ_SIGN1) != 0) {
                                BzFree(yy);
                        }
                        return BZNULL;
                }
                BnnComplement2(BzToBn(zz), zl);
                sign |= BZ_SIGN2;
        } else  {
                zz = z;
        }

        if (yl < zl) {
                if ((n = BzCopy(zz)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN1) != 0) {
                                for (l = (zl - 1); l >= yl; --l) {
                                  BnnAndDigits(BzToBn(n) + l, BN_ZERO);
                                }
                        }
                        for (l = 0; l < yl; ++l) {
                          BnnOrDigits(BzToBn(n) + l, *(BzToBn(yy) + l));
                        }
                }
        } else  {
                if ((n = BzCopy(yy)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN2) != 0) {
                                for (l = (yl - 1); l >= zl; --l) {
                                  BnnAndDigits(BzToBn(n) + l, BN_ZERO);
                                }
                        }
                        for (l = 0; l < zl; ++l) {
                          BnnOrDigits(BzToBn(n) + l, *(BzToBn(zz) + l));
                        }
                }
        }

        if (n != BZNULL) {
                if (BnnIsZero(BzToBn(n), BzNumDigits(n)) == BN_TRUE) {
                        BzSetSign(n, BZ_ZERO);
                } else  if (sign != 0) {
                        BnnComplement2(BzToBn(n), BzNumDigits(n));
                        BzSetSign(n, BZ_MINUS);
                }
        }

        /*
         *  Free copies.
         */

        if ((sign & BZ_SIGN1) != 0) {
                BzFree(yy);
        }

        if ((sign & BZ_SIGN2) != 0) {
                BzFree(zz);
        }

        return (n);
}

BigZ
BzXor(const BigZ y, const BigZ z) {
        /*
         * Returns Y ^ Z.
         */

        BigZ            n;
        BigZ            yy;
        BigZ            zz;
        BigNumLength    yl;
        BigNumLength    zl;
        BigNumLength    l;
        int             sign = 0;

        yl = BzNumDigits(y);
        zl = BzNumDigits(z);

        if (BzGetSign(y) == BZ_MINUS) {
                if ((yy = BzCopy(y)) == BZNULL) {
                        return BZNULL;
                }
                BnnComplement2(BzToBn(yy), yl);
                sign |= BZ_SIGN1;
        } else  {
                yy = y;
        }

        if (BzGetSign(z) == BZ_MINUS) {
                if ((zz = BzCopy(z)) == BZNULL) {
                        if ((sign & BZ_SIGN1) != 0) {
                                BzFree(yy);
                        }
                        return BZNULL;
                }
                BnnComplement2(BzToBn(zz), zl);
                sign |= BZ_SIGN2;
        } else  {
                zz = z;
        }

        if (yl < zl) {
                if ((n = BzCopy(zz)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN1) != 0) {
                                for (l = (zl - 1); l >= yl; --l) {
                                  BnnXorDigits(BzToBn(n) + l, BN_COMPLEMENT);
                                }
                        }
                        for (l = 0; l < yl; ++l) {
                          BnnXorDigits(BzToBn(n) + l, *(BzToBn(yy) + l));
                        }
                }
        } else  {
                if ((n = BzCopy(yy)) != BZNULL) {
                        BzSetSign(n, BZ_PLUS);
                        if ((sign & BZ_SIGN2) != 0) {
                                for (l = (yl - 1); l >= zl; --l) {
                                  BnnXorDigits(BzToBn(n) + l, BN_COMPLEMENT);
                                }
                        }
                        for (l = 0; l < zl; ++l) {
                          BnnXorDigits(BzToBn(n) + l, *(BzToBn(zz) + l));
                        }
                }
        }

        if (n != BZNULL) {
                if (BnnIsZero(BzToBn(n), BzNumDigits(n)) == BN_TRUE) {
                        BzSetSign(n, BZ_ZERO);
                } else  if (sign == BZ_SIGN1 || sign == BZ_SIGN2) {
                        BnnComplement2(BzToBn(n), BzNumDigits(n));
                        BzSetSign(n, BZ_MINUS);
                }
        }

        /*
         *  Free copies.
         */

        if ((sign & BZ_SIGN1) != 0) {
                BzFree(yy);
        }

        if ((sign & BZ_SIGN2) != 0) {
                BzFree(zz);
        }

        return (n);
}

BigNumBool
BzTestBit(BigNumLength bit, const BigZ z) {
        BigNumLength    zl;
        BigNumBool      r;

        /*
         * Returns  BN_TRUE  iff bit is on (i.e.  2**bit is one).  It
         * assumes that bit is a non-negative integer.
         */

        zl = (BigNumLength)(bit / BN_DIGIT_SIZE);

        if (zl >= BzNumDigits(z)) {
                return ((BzGetSign(z) == BZ_MINUS) ? BN_TRUE : BN_FALSE);
        }

        bit = (bit % BN_DIGIT_SIZE);

        if (BzGetSign(z) == BZ_MINUS) {
                BigZ y;
                BigNumLength yl;

                if ((y = BzCopy(z)) == BZNULL) {
                        return (BN_FALSE);
                }

                yl = BzNumDigits(y);

                BnnComplement2(BzToBn(y), yl);
                BzSetSign(y, BZ_PLUS);
                r = (BigNumBool)(((*(BzToBn(y) + zl)) & (BN_ONE << bit)) != 0);
                BzFree(y);
        } else  {
                r = (BigNumBool)(((*(BzToBn(z) + zl)) & (BN_ONE << bit)) != 0);
        }

        return (r);
}

BigNumLength
BzBitCount(const BigZ z) {
        /*
         * Returns the number of bits set in z.
         */

        BigNumLength nl = (BigNumLength)0;
        BigZ    y;

        switch (BzGetSign(z)) {
        case BZ_MINUS:
                if ((y = BzCopy(z)) == BZNULL) {
                        return ((BigNumLength)0);
                }
                BnnComplement2(BzToBn(y), BzNumDigits(y));
                BnnComplement(BzToBn(y), BzNumDigits(y));
                nl = BnnNumCount(BzToBn(y), BzNumDigits(y));
                BzFree(y);
                break;
        case BZ_ZERO:
                nl = (BigNumLength)0;
                break;
        case BZ_PLUS:
                nl = BnnNumCount(BzToBn(z), BzNumDigits(z));
                break;
        }

        return (nl);
}

/*
 *      Simple logical equivalence rules.
 */

BigZ
BzNand(const BigZ x, const BigZ y) {
        BigZ tmp = BzAnd(x, y);
        BigZ res = BzNot(tmp);
        BzFree(tmp);
        return (res);
}

BigZ
BzNor(const BigZ x, const BigZ y) {
        BigZ tmp = BzOr(x, y);
        BigZ res = BzNot(tmp);
        BzFree(tmp);
        return (res);
}

BigZ
BzEqv(const BigZ x, const BigZ y) {
        BigZ tmp = BzXor(x, y);
        BigZ res = BzNot(tmp);
        BzFree(tmp);
        return (res);
}

BigZ
BzAndC1(const BigZ x, const BigZ y) {
        BigZ tmp = BzNot(x);
        BigZ res = BzAnd(tmp, y);
        BzFree(tmp);
        return (res);
}

BigZ
BzAndC2(const BigZ x, const BigZ y) {
        BigZ tmp = BzNot(y);
        BigZ res = BzAnd(x, tmp);
        BzFree(tmp);
        return (res);
}

BigZ
BzOrC1(const BigZ x, const BigZ y) {
        BigZ tmp = BzNot(x);
        BigZ res = BzOr(tmp, y);
        BzFree(tmp);
        return (res);
}

BigZ
BzOrC2(const BigZ x, const BigZ y) {
        BigZ tmp = BzNot(y);
        BigZ res = BzOr(x, y);
        BzFree(tmp);
        return (res);
}

BigZ
BzAsh(const BigZ y, int n) {
        BigZ    z;

        if (n > 0) {
                /*
                 *      Create a copy + space for the shift.
                 */

                BigNumLength zl;
                BigNumLength len = (BigNumLength)(BN_DIGIT_SIZE - 1);
                int          ll;

                if ((BzNumDigits(y) == (BigNumLength)1)
                    && (BzGetDigit(y, 0) == (BigNumDigit)1)) {
                        /*
                         * Optimize case where y == 1 (power of two).
                         */
                        int bitnb = n + 1;
                        BigNumDigit digit;

                        /*
                         * Compute the number of buckets.
                         */

                        zl = (BigNumLength)(bitnb / BN_DIGIT_SIZE);

                        if ((bitnb % BN_DIGIT_SIZE) != 0) {
                                zl++;
                        }

                        /*
                         * Create a zeroed BigZ of zl buckets.
                         */

                        if ((z = BzCreate(zl)) == BZNULL) {
                                return (z);
                        }

                        /*
                         * Set the highest bit.
                         */

                        digit = ((BigNumDigit)1 << ((BzUInt)n % BN_DIGIT_SIZE));
                        BzSetDigit(z, zl - 1, digit);

                        /*
                         * Sign is the sign of y.
                         */
                        BzSetSign(z, BzGetSign(y));
                        return (z);
                } else  {
                        zl = (BigNumLength)(n / BN_DIGIT_SIZE);

                        if ((n % BN_DIGIT_SIZE) != 0) {
                                zl++;
                        }

                        zl += BzNumDigits(y);

                        if ((z = BzCreate(zl)) == BZNULL) {
                                return (z);
                        }

                        BnnAssign(BzToBn(z), BzToBn(y), BzNumDigits(y));
                        BzSetSign(z, BzGetSign(y));

                        /*
                         *      Now do the shift by BN_DIGIT_SIZE increment.
                         */

                        for (ll = n; ll >= (int)BN_DIGIT_SIZE; ll -= len) {
                                (void)BnnShiftLeft(BzToBn(z), zl, len);
                        }

                        (void)BnnShiftLeft(BzToBn(z), zl, (BigNumLength)ll);
                }
        } else  {
                BigZ    one;
                BigZ    d;

                if (y == BZNULL) {
                        return BZNULL;
                } else  if (BzGetSign(y) == BZ_ZERO) {
                        return (BzCopy(y));
                }

                if ((one = BzFromInteger((BzInt)1)) == BZNULL) {
                        return BZNULL;
                }

                if ((d = BzAsh(one, -n)) == BZNULL) {
                        BzFree(one);
                        return BZNULL;
                }

                z = BzFloor(y, d);
                BzFree(d);
                BzFree(one);
        }

        return (z);
}

BigZ
BzSqrt(const BigZ z) {
        BigNumLength    n;
        BigZ            x;
        BigZ            two;

        if (BzGetSign(z) == BZ_ZERO) {
                return (BzFromInteger((BzInt)0));
        }

        n   = BzLength(z);

        if ((n & 1) != 0) {
                /*
                 * n is odd.
                 */
                n = (n >> 1) + 1;
        } else  {
                /*
                 * n is even.
                 */
                n = (n >> 1);
        }

        {
                BigZ one = BzFromInteger((BzInt)1);

                x = BzAsh(one, (int)n);
                BzFree(one);
        }

        two = BzFromInteger((BzInt)2);

        for (;;) {
                BigZ v;
                BigZ y = BzFloor(z, x);

                if (BzCompare(x, y) != BZ_GT) {
                        BzFree(y);
                        break;
                }

                v = BzAdd(x, y);

                BzFree(x);
                x = BzFloor(v, two);
                BzFree(v);
                BzFree(y);
        }

        /*
         * Free temporary two
         */

        BzFree(two);

        return (x);
}

BigZ
BzLcm(const BigZ y, const BigZ z) {
        BigZ    a;
        BigZ    b;
        BigZ    r;

        /*
         * Returns lcm(Y, Z).
         */

        r = BZNULL;

        if ((a = BzMultiply(y, z)) != BZNULL) {
                if (BzGetSign(a) == BZ_MINUS) {
                        BzSetSign(a, BZ_PLUS);
                }

                if ((b = BzGcd(y, z)) != BZNULL) {
                        r = BzTruncate(a, b);
                        BzFree(b);
                }
                BzFree(a);
        }

        return (r);
}

BigZ
BzGcd(const BigZ y, const BigZ z) {
        /*
         * Returns gcd(Y, Z).
         */

        if (BzGetSign(y) == BZ_ZERO) {
                return (BzAbs(z));
        } else  if (BzGetSign(z) == BZ_ZERO) {
                return (BzAbs(y));
        } else  {
                BigZ yc;
                BigZ zc;

                if ((yc = BzAbs(y)) == BZNULL) {
                        /* a fresh copy failed */
                        return BZNULL;
                }

                if ((zc = BzAbs(z)) == BZNULL) {
                        /* a fresh copy failed */
                        BzFree(yc);
                        return BZNULL;
                }

                while (BzGetSign(zc) != BZ_ZERO) {
                        BigZ tmp = BzMod(yc, zc);
                        BzFree(yc);

                        if (tmp == BZNULL) {
                                yc = BZNULL;
                                break;
                        } else {
                                yc = zc;
                                zc = tmp;
                        }
                }

                BzFree(zc);

                return (yc);
        }
}

BigZ
BzPow(const BigZ base, BzUInt exponent) {
        if (exponent == 0) {
                /*
                 * Any nonzero number raised by the exponent 0 is 1
                 */
                return (BzFromInteger(1));
        } else  {
                BigZ x;
                BigZ y;

                if ((x = BzMultiply(base, base)) == BZNULL) {
                        return BZNULL;
                }

                y = BzPow(x, exponent / 2);

                BzFree(x);

                if (y == BZNULL) {
                        return BZNULL;
                } else  if ((exponent % (BzUInt)2) != 0) {
                        x = BzMultiply(y, base);
                        BzFree(y);
                        return (x);
                } else {
                        return (y);
                }
        }
}

/*
 *  Right-to-left binary method
 *  https://en.wikipedia.org/wiki/Modular_exponentiation
 *  function modular_pow(base, exponent, modulus)
 *    if (modulus == 1)
 *      return 0
 *    result := 1
 *    base := base mod modulus
 *    while exponent > 0
 *        if (exponent mod 2 == 1)
 *           result := (result * base) mod modulus
 *        exponent := exponent >> 1
 *        base := (base * base) mod modulus
 *    return result
 */
BigZ
BzModExp(const BigZ base, const BigZ exponent, const BigZ modulus) {
        BigZ result;
        BigZ mod;
        BigZ expnt;
        BigZ b;
        int  neg;

        if ((result = BzFromInteger(1)) == BZNULL) {
          return BZNULL;
        }

        switch (BzGetSign(exponent)) {
        case BZ_ZERO:
                /*
                 * exponent == 0, (base ** 0) == 1, two cases to consider:
                 */
                if (BzCompare(modulus, result) == BZ_EQ) {
                        /*
                         * modulus == 1 => 0
                         */
                        BnnSetToZero(BzToBn(result), (BigNumLength)1);
                        BzSetSign(result, BZ_ZERO);
                        return (result);
                } else  if (BzGetSign(modulus) == BZ_MINUS) {
                        /*
                         * modulus < 0 => modulus + 1
                         */
                        BigZ tmp = BzAdd(modulus, result);
                        BzFree(result);
                        return (tmp);
                } else  {
                        /*
                         * modulus > 1 => 1
                         */
                        return (result);
                }
        case BZ_MINUS:
                /*
                 * Negative exponent is not supported.
                 */
                BzFree(result);
                return BZNULL;
        default:
                if (BzGetSign(modulus) == BZ_PLUS) {
                        /*
                         * modulus is positive, don't need to copy it.
                         */
                        neg = 0;
                        mod = modulus;
                } else  {
                        /*
                         * Make a copy of modulus as positive value.
                         */

                        if ((mod = BzNegate(modulus)) == BZNULL) {
                                BzFree(result);
                                return BZNULL;
                        }

                        neg = 1;
                }

                /*
                 * Copy base as it will be modified.
                 */

                if ((b = BzCopy(base)) == BZNULL) {
                        BzFreeIf(neg, mod);
                        BzFree(result);
                        return BZNULL;
                }

                /*
                 * Copy exponent as it will be modified.
                 */

                if ((expnt = BzCopy(exponent)) == BZNULL) {
                        BzFreeIf(neg, mod);
                        BzFree(b);
                        BzFree(result);
                        return BZNULL;
                }

                while (BzGetSign(expnt) == BZ_PLUS) {
                        BigZ tmp;
                        if (BzIsOdd(expnt)) {
                                tmp = BzMultiply(result, b);
                                BzFree(result);
                                if (tmp == BZNULL) {
                                        BzFreeIf(neg, mod);
                                        BzFree(expnt);
                                        BzFree(b);
                                        return BZNULL;
                                }
                                result = BzMod(tmp, mod);
                                BzFree(tmp);
                                if (result == BZNULL) {
                                        BzFreeIf(neg, mod);
                                        BzFree(expnt);
                                        BzFree(b);
                                        return BZNULL;
                                }
                        }
                        /*
                         * expnt = expnt >> 1;
                         */
                        tmp = BzAsh(expnt, -1);
                        BzFree(expnt);
                        expnt = tmp;
                        tmp = BzMultiply(b, b);
                        BzFree(b);
                        if (tmp == BZNULL) {
                                BzFreeIf(neg, mod);
                                BzFree(expnt);
                                BzFree(result);
                                return BZNULL;
                        }
                        b = BzMod(tmp, mod);
                        BzFree(tmp);
                        if (b == BZNULL) {
                                BzFreeIf(neg, mod);
                                BzFree(expnt);
                                BzFree(result);
                                return BZNULL;
                        }
                }

                BzFree(expnt);
                BzFree(b);

                if (neg) {
                        /*
                         * Modulus is negative. Adjust value.
                         */
                        BigZ tmp;
                        BzFree(mod);
                        tmp = BzMod(result, modulus);
                        BzFree(result);
                        return (tmp);
                } else  {
                        return (result);
                }
        }
}
