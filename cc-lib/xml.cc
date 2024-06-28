#include "xml.h"

// This parser is based on yxml, which is a single-file low-level
// parser (maybe "tokenizer") included right here. yxml is available
// under a MIT license (below). I made minor changes to assume C++,
// untabify, remove the #include of the header since it's embedded,
// wrap in anonymous namespace, etc.

// My contributions can be distributed under the same license, with
// the added copyright:
//
//   Portions Copyright (c) 2021 Tom Murphy VII


namespace {
// ------------------- yxml.h ----------------------------------


/* Copyright (c) 2013-2014 Yoran Heling

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdint.h>
#include <stddef.h>

/* Full API documentation for this library can be found in the "yxml.md" file
 * in the yxml git repository, or online at http://dev.yorhel.nl/yxml/man */

typedef enum {
  YXML_EEOF        = -5, /* Unexpected EOF                             */
  YXML_EREF        = -4, /* Invalid character or entity reference (&whatever;) */
  YXML_ECLOSE      = -3, /* Close tag does not match open tag (<Tag> .. </OtherTag>) */
  YXML_ESTACK      = -2, /* Stack overflow (too deeply nested tags or too long element/attribute name) */
  YXML_ESYN        = -1, /* Syntax error (unexpected byte)             */
  YXML_OK          =  0, /* Character consumed, no new token present   */
  YXML_ELEMSTART   =  1, /* Start of an element:   '<Tag ..'           */
  YXML_CONTENT     =  2, /* Element content                            */
  YXML_ELEMEND     =  3, /* End of an element:     '.. />' or '</Tag>' */
  YXML_ATTRSTART   =  4, /* Attribute:             'Name=..'           */
  YXML_ATTRVAL     =  5, /* Attribute value                            */
  YXML_ATTREND     =  6, /* End of attribute       '.."'               */
  YXML_PISTART     =  7, /* Start of a processing instruction          */
  YXML_PICONTENT   =  8, /* Content of a PI                            */
  YXML_PIEND       =  9  /* End of a processing instruction            */
} yxml_ret_t;

/* When, exactly, are tokens returned?
 *
 * <TagName
 *   '>' ELEMSTART
 *   '/' ELEMSTART, '>' ELEMEND
 *   ' ' ELEMSTART
 *     '>'
 *     '/', '>' ELEMEND
 *     Attr
 *       '=' ATTRSTART
 *         "X ATTRVAL
 *           'Y'  ATTRVAL
 *             'Z'  ATTRVAL
 *               '"' ATTREND
 *                 '>'
 *                 '/', '>' ELEMEND
 *
 * </TagName
 *   '>' ELEMEND
 */


typedef struct {
  /* PUBLIC (read-only) */

  /* Name of the current element, zero-length if not in any element. Changed
   * after YXML_ELEMSTART. The pointer will remain valid up to and including
   * the next non-YXML_ATTR* token, the pointed-to buffer will remain valid
   * up to and including the YXML_ELEMEND for the corresponding element. */
  char *elem;

  /* The last read character(s) of an attribute value (YXML_ATTRVAL), element
   * data (YXML_CONTENT), or processing instruction (YXML_PICONTENT). Changed
   * after one of the respective YXML_ values is returned, and only valid
   * until the next yxml_parse() call. Usually, this string only consists of
   * a single byte, but multiple bytes are returned in the following cases:
   * - "<?SomePI ?x ?>": The two characters "?x"
   * - "<![CDATA[ ]x ]]>": The two characters "]x"
   * - "<![CDATA[ ]]x ]]>": The three characters "]]x"
   * - "&#N;" and "&#xN;", where dec(n) > 127. The referenced Unicode
   *   character is then encoded in multiple UTF-8 bytes.
   */
  char data[8];

  /* Name of the current attribute. Changed after YXML_ATTRSTART, valid up to
   * and including the next YXML_ATTREND. */
  char *attr;

  /* Name/target of the current processing instruction, zero-length if not in
   * a PI. Changed after YXML_PISTART, valid up to (but excluding)
   * the next YXML_PIEND. */
  char *pi;

  /* Line number, byte offset within that line, and total bytes read. These
   * values refer to the position _after_ the last byte given to
   * yxml_parse(). These are useful for debugging and error reporting. */
  uint64_t byte;
  uint64_t total;
  uint32_t line;


  /* PRIVATE */
  int state;
  unsigned char *stack; /* Stack of element names + attribute/PI name, separated by \0. Also starts with a \0. */
  size_t stacksize, stacklen;
  unsigned reflen;
  unsigned quote;
  int nextstate; /* Used for '@' state remembering and for the "string" consuming state */
  unsigned ignore;
  unsigned char *string;
} yxml_t;


void yxml_init(yxml_t *, void *, size_t);


yxml_ret_t yxml_parse(yxml_t *, int);


/* May be called after the last character has been given to yxml_parse().
 * Returns YXML_OK if the XML document is valid, YXML_EEOF otherwise.  Using
 * this function isn't really necessary, but can be used to detect documents
 * that don't end correctly. In particular, an error is returned when the XML
 * document did not contain a (complete) root element, or when the document
 * ended while in a comment or processing instruction. */
yxml_ret_t yxml_eof(yxml_t *);


/* Returns the length of the element name (x->elem), attribute name (x->attr),
 * or PI name (x->pi). This function should ONLY be used directly after the
 * YXML_ELEMSTART, YXML_ATTRSTART or YXML_PISTART (respectively) tokens have
 * been returned by yxml_parse(), calling this at any other time may not give
 * the correct results. This function should also NOT be used on strings other
 * than x->elem, x->attr or x->pi. */
[[maybe_unused]]
static inline size_t yxml_symlen(yxml_t *x, const char *s) {
  return (x->stack + x->stacklen) - (const unsigned char*)s;
}

//  ----------------------- yxml.c -----------------------------------

/* This file is generated by yxml-gen.pl using yxml-states and yxml.c.in as input files.
 * It is preferable to edit those files instead of this one if you want to make a change.
 * The source files can be found through the homepage: https://dev.yorhel.nl/yxml */

/* Copyright (c) 2013-2014 Yoran Heling

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <string.h>

typedef enum {
  YXMLS_string,
  YXMLS_attr0,
  YXMLS_attr1,
  YXMLS_attr2,
  YXMLS_attr3,
  YXMLS_attr4,
  YXMLS_cd0,
  YXMLS_cd1,
  YXMLS_cd2,
  YXMLS_comment0,
  YXMLS_comment1,
  YXMLS_comment2,
  YXMLS_comment3,
  YXMLS_comment4,
  YXMLS_dt0,
  YXMLS_dt1,
  YXMLS_dt2,
  YXMLS_dt3,
  YXMLS_dt4,
  YXMLS_elem0,
  YXMLS_elem1,
  YXMLS_elem2,
  YXMLS_elem3,
  YXMLS_enc0,
  YXMLS_enc1,
  YXMLS_enc2,
  YXMLS_enc3,
  YXMLS_etag0,
  YXMLS_etag1,
  YXMLS_etag2,
  YXMLS_init,
  YXMLS_le0,
  YXMLS_le1,
  YXMLS_le2,
  YXMLS_le3,
  YXMLS_lee1,
  YXMLS_lee2,
  YXMLS_leq0,
  YXMLS_misc0,
  YXMLS_misc1,
  YXMLS_misc2,
  YXMLS_misc2a,
  YXMLS_misc3,
  YXMLS_pi0,
  YXMLS_pi1,
  YXMLS_pi2,
  YXMLS_pi3,
  YXMLS_pi4,
  YXMLS_std0,
  YXMLS_std1,
  YXMLS_std2,
  YXMLS_std3,
  YXMLS_ver0,
  YXMLS_ver1,
  YXMLS_ver2,
  YXMLS_ver3,
  YXMLS_xmldecl0,
  YXMLS_xmldecl1,
  YXMLS_xmldecl2,
  YXMLS_xmldecl3,
  YXMLS_xmldecl4,
  YXMLS_xmldecl5,
  YXMLS_xmldecl6,
  YXMLS_xmldecl7,
  YXMLS_xmldecl8,
  YXMLS_xmldecl9
} yxml_state_t;


#define yxml_isChar(c) 1
/* 0xd should be part of SP, too, but yxml_parse() already normalizes that into 0xa */
#define yxml_isSP(c) (c == 0x20 || c == 0x09 || c == 0x0a)
#define yxml_isAlpha(c) ((c|32)-'a' < 26)
#define yxml_isNum(c) (c-'0' < 10)
#define yxml_isHex(c) (yxml_isNum(c) || (c|32)-'a' < 6)
#define yxml_isEncName(c) (yxml_isAlpha(c) || yxml_isNum(c) || c == '.' || c == '_' || c == '-')
#define yxml_isNameStart(c) (yxml_isAlpha(c) || c == ':' || c == '_' || c >= 128)
#define yxml_isName(c) (yxml_isNameStart(c) || yxml_isNum(c) || c == '-' || c == '.')
/* XXX: The valid characters are dependent on the quote char, hence the access to x->quote */
#define yxml_isAttValue(c) (yxml_isChar(c) && c != x->quote && c != '<' && c != '&')
/* Anything between '&' and ';', the yxml_ref* functions will do further
 * validation. Strictly speaking, this is "yxml_isName(c) || c == '#'", but
 * this parser doesn't understand entities with '.', ':', etc, anwyay.  */
#define yxml_isRef(c) (yxml_isNum(c) || yxml_isAlpha(c) || c == '#')

#define INTFROM5CHARS(a, b, c, d, e) ((((uint64_t)(a))<<32) | (((uint64_t)(b))<<24) | (((uint64_t)(c))<<16) | (((uint64_t)(d))<<8) | (uint64_t)(e))


/* Set the given char value to ch (0<=ch<=255). */
static inline void yxml_setchar(char *dest, unsigned ch) {
  *(unsigned char *)dest = ch;
}


/* Similar to yxml_setchar(), but will convert ch (any valid unicode point) to
 * UTF-8 and appends a '\0'. dest must have room for at least 5 bytes. */
static void yxml_setutf8(char *dest, unsigned ch) {
  if(ch <= 0x007F)
    yxml_setchar(dest++, ch);
  else if(ch <= 0x07FF) {
    yxml_setchar(dest++, 0xC0 | (ch>>6));
    yxml_setchar(dest++, 0x80 | (ch & 0x3F));
  } else if(ch <= 0xFFFF) {
    yxml_setchar(dest++, 0xE0 | (ch>>12));
    yxml_setchar(dest++, 0x80 | ((ch>>6) & 0x3F));
    yxml_setchar(dest++, 0x80 | (ch & 0x3F));
  } else {
    yxml_setchar(dest++, 0xF0 | (ch>>18));
    yxml_setchar(dest++, 0x80 | ((ch>>12) & 0x3F));
    yxml_setchar(dest++, 0x80 | ((ch>>6) & 0x3F));
    yxml_setchar(dest++, 0x80 | (ch & 0x3F));
  }
  *dest = 0;
}


static inline yxml_ret_t yxml_datacontent(yxml_t *x, unsigned ch) {
  yxml_setchar(x->data, ch);
  x->data[1] = 0;
  return YXML_CONTENT;
}


static inline yxml_ret_t yxml_datapi1(yxml_t *x, unsigned ch) {
  yxml_setchar(x->data, ch);
  x->data[1] = 0;
  return YXML_PICONTENT;
}


static inline yxml_ret_t yxml_datapi2(yxml_t *x, unsigned ch) {
  x->data[0] = '?';
  yxml_setchar(x->data+1, ch);
  x->data[2] = 0;
  return YXML_PICONTENT;
}


static inline yxml_ret_t yxml_datacd1(yxml_t *x, unsigned ch) {
  x->data[0] = ']';
  yxml_setchar(x->data+1, ch);
  x->data[2] = 0;
  return YXML_CONTENT;
}


static inline yxml_ret_t yxml_datacd2(yxml_t *x, unsigned ch) {
  x->data[0] = ']';
  x->data[1] = ']';
  yxml_setchar(x->data+2, ch);
  x->data[3] = 0;
  return YXML_CONTENT;
}


static inline yxml_ret_t yxml_dataattr(yxml_t *x, unsigned ch) {
  /* Normalize attribute values according to the XML spec section 3.3.3. */
  yxml_setchar(x->data, ch == 0x9 || ch == 0xa ? 0x20 : ch);
  x->data[1] = 0;
  return YXML_ATTRVAL;
}


static yxml_ret_t yxml_pushstack(yxml_t *x, char **res, unsigned ch) {
  if(x->stacklen+2 >= x->stacksize)
    return YXML_ESTACK;
  x->stacklen++;
  *res = (char *)x->stack+x->stacklen;
  x->stack[x->stacklen] = ch;
  x->stacklen++;
  x->stack[x->stacklen] = 0;
  return YXML_OK;
}


static yxml_ret_t yxml_pushstackc(yxml_t *x, unsigned ch) {
  if(x->stacklen+1 >= x->stacksize)
    return YXML_ESTACK;
  x->stack[x->stacklen] = ch;
  x->stacklen++;
  x->stack[x->stacklen] = 0;
  return YXML_OK;
}


static void yxml_popstack(yxml_t *x) {
  do
    x->stacklen--;
  while(x->stack[x->stacklen]);
}


static inline yxml_ret_t yxml_elemstart  (yxml_t *x, unsigned ch) { return yxml_pushstack(x, &x->elem, ch); }
static inline yxml_ret_t yxml_elemname   (yxml_t *x, unsigned ch) { return yxml_pushstackc(x, ch); }
static inline yxml_ret_t yxml_elemnameend(yxml_t *x, unsigned ch) { return YXML_ELEMSTART; }


/* Also used in yxml_elemcloseend(), since this function just removes the last
 * element from the stack and returns ELEMEND. */
static yxml_ret_t yxml_selfclose(yxml_t *x, unsigned ch) {
  yxml_popstack(x);
  if(x->stacklen) {
    x->elem = (char *)x->stack+x->stacklen-1;
    while(*(x->elem-1))
      x->elem--;
    return YXML_ELEMEND;
  }
  x->elem = (char *)x->stack;
  x->state = YXMLS_misc3;
  return YXML_ELEMEND;
}


static inline yxml_ret_t yxml_elemclose(yxml_t *x, unsigned ch) {
  if(*((unsigned char *)x->elem) != ch)
    return YXML_ECLOSE;
  x->elem++;
  return YXML_OK;
}


static inline yxml_ret_t yxml_elemcloseend(yxml_t *x, unsigned ch) {
  if(*x->elem)
    return YXML_ECLOSE;
  return yxml_selfclose(x, ch);
}


static inline yxml_ret_t yxml_attrstart  (yxml_t *x, unsigned ch) { return yxml_pushstack(x, &x->attr, ch); }
static inline yxml_ret_t yxml_attrname   (yxml_t *x, unsigned ch) { return yxml_pushstackc(x, ch); }
static inline yxml_ret_t yxml_attrnameend(yxml_t *x, unsigned ch) { return YXML_ATTRSTART; }
static inline yxml_ret_t yxml_attrvalend (yxml_t *x, unsigned ch) { yxml_popstack(x); return YXML_ATTREND; }


static inline yxml_ret_t yxml_pistart  (yxml_t *x, unsigned ch) { return yxml_pushstack(x, &x->pi, ch); }
static inline yxml_ret_t yxml_piname   (yxml_t *x, unsigned ch) { return yxml_pushstackc(x, ch); }
static inline yxml_ret_t yxml_piabort  (yxml_t *x, unsigned ch) { yxml_popstack(x); return YXML_OK; }
static inline yxml_ret_t yxml_pinameend(yxml_t *x, unsigned ch) {
  return (x->pi[0]|32) == 'x' && (x->pi[1]|32) == 'm' && (x->pi[2]|32) == 'l' && !x->pi[3] ? YXML_ESYN : YXML_PISTART;
}
static inline yxml_ret_t yxml_pivalend (yxml_t *x, unsigned ch) { yxml_popstack(x); x->pi = (char *)x->stack; return YXML_PIEND; }


static inline yxml_ret_t yxml_refstart(yxml_t *x, unsigned ch) {
  memset(x->data, 0, sizeof(x->data));
  x->reflen = 0;
  return YXML_OK;
}


static yxml_ret_t yxml_ref(yxml_t *x, unsigned ch) {
  if(x->reflen >= sizeof(x->data)-1)
    return YXML_EREF;
  yxml_setchar(x->data+x->reflen, ch);
  x->reflen++;
  return YXML_OK;
}


static yxml_ret_t yxml_refend(yxml_t *x, yxml_ret_t ret) {
  unsigned char *r = (unsigned char *)x->data;
  unsigned ch = 0;
  if(*r == '#') {
    if(r[1] == 'x')
      for(r += 2; yxml_isHex((unsigned)*r); r++)
        ch = (ch<<4) + (*r <= '9' ? *r-'0' : (*r|32)-'a' + 10);
    else
      for(r++; yxml_isNum((unsigned)*r); r++)
        ch = (ch*10) + (*r-'0');
    if(*r)
      ch = 0;
  } else {
    uint64_t i = INTFROM5CHARS(r[0], r[1], r[2], r[3], r[4]);
    ch =
      i == INTFROM5CHARS('l','t', 0,  0, 0) ? '<' :
      i == INTFROM5CHARS('g','t', 0,  0, 0) ? '>' :
      i == INTFROM5CHARS('a','m','p', 0, 0) ? '&' :
      i == INTFROM5CHARS('a','p','o','s',0) ? '\'':
      i == INTFROM5CHARS('q','u','o','t',0) ? '"' : 0;
  }

  /* Codepoints not allowed in the XML 1.1 definition of a Char */
  if(!ch || ch > 0x10FFFF || ch == 0xFFFE || ch == 0xFFFF || (ch-0xDFFF) < 0x7FF)
    return YXML_EREF;
  yxml_setutf8(x->data, ch);
  return ret;
}


static inline yxml_ret_t yxml_refcontent(yxml_t *x, unsigned ch) { return yxml_refend(x, YXML_CONTENT); }
static inline yxml_ret_t yxml_refattrval(yxml_t *x, unsigned ch) { return yxml_refend(x, YXML_ATTRVAL); }


void yxml_init(yxml_t *x, void *stack, size_t stacksize) {
  memset(x, 0, sizeof(*x));
  x->line = 1;
  x->stack = (unsigned char*)stack;
  x->stacksize = stacksize;
  *x->stack = 0;
  x->elem = x->pi = x->attr = (char *)x->stack;
  x->state = YXMLS_init;
}


yxml_ret_t yxml_parse(yxml_t *x, int _ch) {
  /* Ensure that characters are in the range of 0..255 rather than -126..125.
   * All character comparisons are done with positive integers. */
  unsigned ch = (unsigned)(_ch+256) & 0xff;
  if(!ch)
    return YXML_ESYN;
  x->total++;

  /* End-of-Line normalization, "\rX", "\r\n" and "\n" are recognized and
   * normalized to a single '\n' as per XML 1.0 section 2.11. XML 1.1 adds
   * some non-ASCII character sequences to this list, but we can only handle
   * ASCII here without making assumptions about the input encoding. */
  if(x->ignore == ch) {
    x->ignore = 0;
    return YXML_OK;
  }
  x->ignore = (ch == 0xd) * 0xa;
  if(ch == 0xa || ch == 0xd) {
    ch = 0xa;
    x->line++;
    x->byte = 0;
  }
  x->byte++;

  switch((yxml_state_t)x->state) {
  case YXMLS_string:
    if(ch == *x->string) {
      x->string++;
      if(!*x->string)
        x->state = x->nextstate;
      return YXML_OK;
    }
    break;
  case YXMLS_attr0:
    if(yxml_isName(ch))
      return yxml_attrname(x, ch);
    if(yxml_isSP(ch)) {
      x->state = YXMLS_attr1;
      return yxml_attrnameend(x, ch);
    }
    if(ch == (unsigned char)'=') {
      x->state = YXMLS_attr2;
      return yxml_attrnameend(x, ch);
    }
    break;
  case YXMLS_attr1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'=') {
      x->state = YXMLS_attr2;
      return YXML_OK;
    }
    break;
  case YXMLS_attr2:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_attr3;
      x->quote = ch;
      return YXML_OK;
    }
    break;
  case YXMLS_attr3:
    if(yxml_isAttValue(ch))
      return yxml_dataattr(x, ch);
    if(ch == (unsigned char)'&') {
      x->state = YXMLS_attr4;
      return yxml_refstart(x, ch);
    }
    if(x->quote == ch) {
      x->state = YXMLS_elem2;
      return yxml_attrvalend(x, ch);
    }
    break;
  case YXMLS_attr4:
    if(yxml_isRef(ch))
      return yxml_ref(x, ch);
    if(ch == (unsigned char)'\x3b') {
      x->state = YXMLS_attr3;
      return yxml_refattrval(x, ch);
    }
    break;
  case YXMLS_cd0:
    if(ch == (unsigned char)']') {
      x->state = YXMLS_cd1;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return yxml_datacontent(x, ch);
    break;
  case YXMLS_cd1:
    if(ch == (unsigned char)']') {
      x->state = YXMLS_cd2;
      return YXML_OK;
    }
    if(yxml_isChar(ch)) {
      x->state = YXMLS_cd0;
      return yxml_datacd1(x, ch);
    }
    break;
  case YXMLS_cd2:
    if(ch == (unsigned char)']')
      return yxml_datacontent(x, ch);
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return YXML_OK;
    }
    if(yxml_isChar(ch)) {
      x->state = YXMLS_cd0;
      return yxml_datacd2(x, ch);
    }
    break;
  case YXMLS_comment0:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment1;
      return YXML_OK;
    }
    break;
  case YXMLS_comment1:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment2;
      return YXML_OK;
    }
    break;
  case YXMLS_comment2:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment3;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return YXML_OK;
    break;
  case YXMLS_comment3:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment4;
      return YXML_OK;
    }
    if(yxml_isChar(ch)) {
      x->state = YXMLS_comment2;
      return YXML_OK;
    }
    break;
  case YXMLS_comment4:
    if(ch == (unsigned char)'>') {
      x->state = x->nextstate;
      return YXML_OK;
    }
    break;
  case YXMLS_dt0:
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc1;
      return YXML_OK;
    }
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_dt1;
      x->quote = ch;
      x->nextstate = YXMLS_dt0;
      return YXML_OK;
    }
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_dt2;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return YXML_OK;
    break;
  case YXMLS_dt1:
    if(x->quote == ch) {
      x->state = x->nextstate;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return YXML_OK;
    break;
  case YXMLS_dt2:
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi0;
      x->nextstate = YXMLS_dt0;
      return YXML_OK;
    }
    if(ch == (unsigned char)'!') {
      x->state = YXMLS_dt3;
      return YXML_OK;
    }
    break;
  case YXMLS_dt3:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment1;
      x->nextstate = YXMLS_dt0;
      return YXML_OK;
    }
    if(yxml_isChar(ch)) {
      x->state = YXMLS_dt4;
      return YXML_OK;
    }
    break;
  case YXMLS_dt4:
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_dt1;
      x->quote = ch;
      x->nextstate = YXMLS_dt4;
      return YXML_OK;
    }
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_dt0;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return YXML_OK;
    break;
  case YXMLS_elem0:
    if(yxml_isName(ch))
      return yxml_elemname(x, ch);
    if(yxml_isSP(ch)) {
      x->state = YXMLS_elem1;
      return yxml_elemnameend(x, ch);
    }
    if(ch == (unsigned char)'/') {
      x->state = YXMLS_elem3;
      return yxml_elemnameend(x, ch);
    }
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return yxml_elemnameend(x, ch);
    }
    break;
  case YXMLS_elem1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'/') {
      x->state = YXMLS_elem3;
      return YXML_OK;
    }
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return YXML_OK;
    }
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_attr0;
      return yxml_attrstart(x, ch);
    }
    break;
  case YXMLS_elem2:
    if(yxml_isSP(ch)) {
      x->state = YXMLS_elem1;
      return YXML_OK;
    }
    if(ch == (unsigned char)'/') {
      x->state = YXMLS_elem3;
      return YXML_OK;
    }
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return YXML_OK;
    }
    break;
  case YXMLS_elem3:
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return yxml_selfclose(x, ch);
    }
    break;
  case YXMLS_enc0:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'=') {
      x->state = YXMLS_enc1;
      return YXML_OK;
    }
    break;
  case YXMLS_enc1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_enc2;
      x->quote = ch;
      return YXML_OK;
    }
    break;
  case YXMLS_enc2:
    if(yxml_isAlpha(ch)) {
      x->state = YXMLS_enc3;
      return YXML_OK;
    }
    break;
  case YXMLS_enc3:
    if(yxml_isEncName(ch))
      return YXML_OK;
    if(x->quote == ch) {
      x->state = YXMLS_xmldecl6;
      return YXML_OK;
    }
    break;
  case YXMLS_etag0:
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_etag1;
      return yxml_elemclose(x, ch);
    }
    break;
  case YXMLS_etag1:
    if(yxml_isName(ch))
      return yxml_elemclose(x, ch);
    if(yxml_isSP(ch)) {
      x->state = YXMLS_etag2;
      return yxml_elemcloseend(x, ch);
    }
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return yxml_elemcloseend(x, ch);
    }
    break;
  case YXMLS_etag2:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc2;
      return YXML_OK;
    }
    break;
  case YXMLS_init:
    if(ch == (unsigned char)'\xef') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_misc0;
      x->string = (unsigned char *)"\xbb\xbf";
      return YXML_OK;
    }
    if(yxml_isSP(ch)) {
      x->state = YXMLS_misc0;
      return YXML_OK;
    }
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_le0;
      return YXML_OK;
    }
    break;
  case YXMLS_le0:
    if(ch == (unsigned char)'!') {
      x->state = YXMLS_lee1;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_leq0;
      return YXML_OK;
    }
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_elem0;
      return yxml_elemstart(x, ch);
    }
    break;
  case YXMLS_le1:
    if(ch == (unsigned char)'!') {
      x->state = YXMLS_lee1;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi0;
      x->nextstate = YXMLS_misc1;
      return YXML_OK;
    }
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_elem0;
      return yxml_elemstart(x, ch);
    }
    break;
  case YXMLS_le2:
    if(ch == (unsigned char)'!') {
      x->state = YXMLS_lee2;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi0;
      x->nextstate = YXMLS_misc2;
      return YXML_OK;
    }
    if(ch == (unsigned char)'/') {
      x->state = YXMLS_etag0;
      return YXML_OK;
    }
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_elem0;
      return yxml_elemstart(x, ch);
    }
    break;
  case YXMLS_le3:
    if(ch == (unsigned char)'!') {
      x->state = YXMLS_comment0;
      x->nextstate = YXMLS_misc3;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi0;
      x->nextstate = YXMLS_misc3;
      return YXML_OK;
    }
    break;
  case YXMLS_lee1:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment1;
      x->nextstate = YXMLS_misc1;
      return YXML_OK;
    }
    if(ch == (unsigned char)'D') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_dt0;
      x->string = (unsigned char *)"OCTYPE";
      return YXML_OK;
    }
    break;
  case YXMLS_lee2:
    if(ch == (unsigned char)'-') {
      x->state = YXMLS_comment1;
      x->nextstate = YXMLS_misc2;
      return YXML_OK;
    }
    if(ch == (unsigned char)'[') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_cd0;
      x->string = (unsigned char *)"CDATA[";
      return YXML_OK;
    }
    break;
  case YXMLS_leq0:
    if(ch == (unsigned char)'x') {
      x->state = YXMLS_xmldecl0;
      x->nextstate = YXMLS_misc1;
      return yxml_pistart(x, ch);
    }
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_pi1;
      x->nextstate = YXMLS_misc1;
      return yxml_pistart(x, ch);
    }
    break;
  case YXMLS_misc0:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_le0;
      return YXML_OK;
    }
    break;
  case YXMLS_misc1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_le1;
      return YXML_OK;
    }
    break;
  case YXMLS_misc2:
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_le2;
      return YXML_OK;
    }
    if(ch == (unsigned char)'&') {
      x->state = YXMLS_misc2a;
      return yxml_refstart(x, ch);
    }
    if(yxml_isChar(ch))
      return yxml_datacontent(x, ch);
    break;
  case YXMLS_misc2a:
    if(yxml_isRef(ch))
      return yxml_ref(x, ch);
    if(ch == (unsigned char)'\x3b') {
      x->state = YXMLS_misc2;
      return yxml_refcontent(x, ch);
    }
    break;
  case YXMLS_misc3:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'<') {
      x->state = YXMLS_le3;
      return YXML_OK;
    }
    break;
  case YXMLS_pi0:
    if(yxml_isNameStart(ch)) {
      x->state = YXMLS_pi1;
      return yxml_pistart(x, ch);
    }
    break;
  case YXMLS_pi1:
    if(yxml_isName(ch))
      return yxml_piname(x, ch);
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi4;
      return yxml_pinameend(x, ch);
    }
    if(yxml_isSP(ch)) {
      x->state = YXMLS_pi2;
      return yxml_pinameend(x, ch);
    }
    break;
  case YXMLS_pi2:
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi3;
      return YXML_OK;
    }
    if(yxml_isChar(ch))
      return yxml_datapi1(x, ch);
    break;
  case YXMLS_pi3:
    if(ch == (unsigned char)'>') {
      x->state = x->nextstate;
      return yxml_pivalend(x, ch);
    }
    if(yxml_isChar(ch)) {
      x->state = YXMLS_pi2;
      return yxml_datapi2(x, ch);
    }
    break;
  case YXMLS_pi4:
    if(ch == (unsigned char)'>') {
      x->state = x->nextstate;
      return yxml_pivalend(x, ch);
    }
    break;
  case YXMLS_std0:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'=') {
      x->state = YXMLS_std1;
      return YXML_OK;
    }
    break;
  case YXMLS_std1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_std2;
      x->quote = ch;
      return YXML_OK;
    }
    break;
  case YXMLS_std2:
    if(ch == (unsigned char)'y') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_std3;
      x->string = (unsigned char *)"es";
      return YXML_OK;
    }
    if(ch == (unsigned char)'n') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_std3;
      x->string = (unsigned char *)"o";
      return YXML_OK;
    }
    break;
  case YXMLS_std3:
    if(x->quote == ch) {
      x->state = YXMLS_xmldecl8;
      return YXML_OK;
    }
    break;
  case YXMLS_ver0:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'=') {
      x->state = YXMLS_ver1;
      return YXML_OK;
    }
    break;
  case YXMLS_ver1:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'\'' || ch == (unsigned char)'"') {
      x->state = YXMLS_string;
      x->quote = ch;
      x->nextstate = YXMLS_ver2;
      x->string = (unsigned char *)"1.";
      return YXML_OK;
    }
    break;
  case YXMLS_ver2:
    if(yxml_isNum(ch)) {
      x->state = YXMLS_ver3;
      return YXML_OK;
    }
    break;
  case YXMLS_ver3:
    if(yxml_isNum(ch))
      return YXML_OK;
    if(x->quote == ch) {
      x->state = YXMLS_xmldecl4;
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl0:
    if(ch == (unsigned char)'m') {
      x->state = YXMLS_xmldecl1;
      return yxml_piname(x, ch);
    }
    if(yxml_isName(ch)) {
      x->state = YXMLS_pi1;
      return yxml_piname(x, ch);
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi4;
      return yxml_pinameend(x, ch);
    }
    if(yxml_isSP(ch)) {
      x->state = YXMLS_pi2;
      return yxml_pinameend(x, ch);
    }
    break;
  case YXMLS_xmldecl1:
    if(ch == (unsigned char)'l') {
      x->state = YXMLS_xmldecl2;
      return yxml_piname(x, ch);
    }
    if(yxml_isName(ch)) {
      x->state = YXMLS_pi1;
      return yxml_piname(x, ch);
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_pi4;
      return yxml_pinameend(x, ch);
    }
    if(yxml_isSP(ch)) {
      x->state = YXMLS_pi2;
      return yxml_pinameend(x, ch);
    }
    break;
  case YXMLS_xmldecl2:
    if(yxml_isSP(ch)) {
      x->state = YXMLS_xmldecl3;
      return yxml_piabort(x, ch);
    }
    if(yxml_isName(ch)) {
      x->state = YXMLS_pi1;
      return yxml_piname(x, ch);
    }
    break;
  case YXMLS_xmldecl3:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'v') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_ver0;
      x->string = (unsigned char *)"ersion";
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl4:
    if(yxml_isSP(ch)) {
      x->state = YXMLS_xmldecl5;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_xmldecl9;
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl5:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_xmldecl9;
      return YXML_OK;
    }
    if(ch == (unsigned char)'e') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_enc0;
      x->string = (unsigned char *)"ncoding";
      return YXML_OK;
    }
    if(ch == (unsigned char)'s') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_std0;
      x->string = (unsigned char *)"tandalone";
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl6:
    if(yxml_isSP(ch)) {
      x->state = YXMLS_xmldecl7;
      return YXML_OK;
    }
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_xmldecl9;
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl7:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_xmldecl9;
      return YXML_OK;
    }
    if(ch == (unsigned char)'s') {
      x->state = YXMLS_string;
      x->nextstate = YXMLS_std0;
      x->string = (unsigned char *)"tandalone";
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl8:
    if(yxml_isSP(ch))
      return YXML_OK;
    if(ch == (unsigned char)'?') {
      x->state = YXMLS_xmldecl9;
      return YXML_OK;
    }
    break;
  case YXMLS_xmldecl9:
    if(ch == (unsigned char)'>') {
      x->state = YXMLS_misc1;
      return YXML_OK;
    }
    break;
  }
  return YXML_ESYN;
}


yxml_ret_t yxml_eof(yxml_t *x) {
  if(x->state != YXMLS_misc3)
    return YXML_EEOF;
  return YXML_OK;
}


// ---------------------- end yxml.c ----------------------------

}  // namespace


using namespace std;

optional<XML::Node> XML::Parse(const string &xml_bytes,
                               string *error) {

  // PERF: yxml internally uses a buffer to keep track of the nesting;
  // it should not be possible to need more than the size of the input
  // document itself, but this is overkill! Best fix here is just to
  // make the stacksize dynamic in yxml, which looks pretty easy.
  const int BUFSIZE = 1 + xml_bytes.size();

  yxml_t yxml;
  vector<uint8_t> yxml_buffer;
  yxml_buffer.resize(BUFSIZE);

  yxml_init(&yxml, yxml_buffer.data(), BUFSIZE);

  // Root node, which is what we return if successful.
  Node root_node;

  // This is the stack of nodes we are inside, with the .back()
  // element (if present) being the current one. Begins empty because
  // we have not entered the root node.
  //
  // Note that since we are using values (children is a vector of
  // nodes, not pointers), we have to be careful about resizing vectors
  // while there's an outstanding reference in this stack. We only
  // modify the .back() element.
  vector<Node *> node_stack;

  // The attribute value currently being accumulated.
  string cur_attribute;
  // Same for text node contents.
  string cur_contents;

  // Flush (non-empty) content whenever we see an element start or end.
  // Clears cur_contents. Returns false on an error.
  auto FlushContent = [&error, &cur_contents, &node_stack]() -> bool {
      if (!cur_contents.empty()) {
        if (node_stack.empty()) {
          if (error != nullptr)
            *error = "bug? Flushed non-empty content with no active node";
          return false;
        }

        // Add child.
        Node *cur = node_stack.back();
        cur->children.resize(cur->children.size() + 1);
        Node *child = &cur->children.back();
        child->type = NodeType::Text;
        child->contents = std::move(cur_contents);
        cur_contents.clear();
      }
      return true;
    };

  for (int idx = 0; idx < (int)xml_bytes.size(); idx++) {
    const char ch = xml_bytes[idx];
    if (ch == 0) {
      if (error != nullptr) *error = "0 byte in xml document";
      return nullopt;
    }

    const yxml_ret_t ret = yxml_parse(&yxml, ch);

    if (ret < 0) {
      // Errors. We could include more information here...
      if (error != nullptr) {
        switch (ret) {
        case YXML_EREF:
          *error = "Invalid character or entity reference, "
            "e.g. &whatever; or &#ABC;.";
          break;
        case YXML_ECLOSE:
          *error = "Close tag does not match open tag, "
            "e.g. <Tag> .. </SomeOtherTag>.";
          break;
        case YXML_ESTACK:
          *error = "Internal stack overflow. Should not be possible?";
          break;
        case YXML_ESYN:
          *error = "Miscellaneous syntax error, e.g. multiple root nodes.";
          break;
        default:
          *error = "Unknown error code??";
          break;
        }
      }
      return nullopt;
    }

    // HERE
    switch (ret) {
    case YXML_PISTART:
    case YXML_PICONTENT:
    case YXML_PIEND:
      // Ignore processing instructions.
      break;
    case YXML_OK:
      // Nothing to do.
      break;

    case YXML_ELEMSTART: {
      FlushContent();

      if (node_stack.empty()) {
        node_stack.push_back(&root_node);
      } else {
        Node *cur = node_stack.back();
        cur->children.resize(cur->children.size() + 1);
        Node *child = &cur->children.back();
        node_stack.push_back(child);
      }

      Node *elem = node_stack.back();
      elem->type = NodeType::Element;
      elem->tag = yxml.elem;
      break;
    }

    case YXML_ELEMEND: {
      if (node_stack.empty()) {
        if (error != nullptr)
          *error = "ELEMEND with empty node stack";
        return nullopt;
      }

      FlushContent();

      node_stack.pop_back();
      break;
    }

    case YXML_ATTRSTART: {
      cur_attribute.clear();
      break;
    }

    case YXML_ATTRVAL: {
      // Usually one char; null-terminated.
      cur_attribute += (char *)&yxml.data[0];
      break;
    }

    case YXML_ATTREND: {
      if (node_stack.empty()) {
        if (error != nullptr)
          *error = "ATTREND with empty node stack";
        return nullopt;
      }
      Node *cur = node_stack.back();

      string attr = yxml.attr;

      if (cur->attrs.find(attr) != cur->attrs.end()) {
        if (error != nullptr) {
          *error = "Duplicate attribute " + attr;
        }
        return nullopt;
      }

      cur->attrs[attr] = std::move(cur_attribute);
      cur_attribute.clear();
      break;
    }

    case YXML_CONTENT: {
      // Usually one char; null-terminated.
      // Once this contents is non-empty, it gets added as a
      // child via FlushContent.
      cur_contents += (char *)&yxml.data[0];
      break;
    }

    case YXML_EEOF:
    case YXML_EREF:
    case YXML_ECLOSE:
    case YXML_ESTACK:
    case YXML_ESYN:
      if (error != nullptr)
        *error = "bug? unhandled error";
      return nullopt;
      break;

    }
  }

  // Finally, check eof conditions.
  switch (yxml_eof(&yxml)) {
  case YXML_OK:
    return {root_node};

  default:
    // Consider anything to be a syntax error here.
  case YXML_EEOF:
    if (error != nullptr)
      *error = "Syntax error at EOF.";
    return nullopt;
  }
}
