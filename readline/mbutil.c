/* mbutil.c -- readline multibyte character utility functions */

/* Copyright (C) 2001-2024 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.      

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <sys/types.h>
#include <fcntl.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>	   /* for _POSIX_VERSION */
#endif /* HAVE_UNISTD_H */

#include <stdlib.h>
#include "rldefs.h"

#include <stdio.h>

/* System-specific feature definitions and include files. */
#include "rlmbutil.h"

#if defined (TIOCSTAT_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
#endif /* TIOCSTAT_IN_SYS_IOCTL */

/* Some standard library routines. */

#include "rlprivate.h"
#include "xmalloc.h"

/* Declared here so it can be shared between the readline and history
   libraries. */
#if defined (HANDLE_MULTIBYTE)
int rl_byte_oriented = 0;
#else
int rl_byte_oriented = 1;
#endif

/* Ditto */
int _rl_utf8locale = 0;

/* **************************************************************** */
/*								    */
/*		Multibyte Character Utility Functions		    */
/*								    */
/* **************************************************************** */

#if defined(HANDLE_MULTIBYTE)

/* **************************************************************** */
/*								    */
/*		UTF-8 specific Character Utility Functions	    */
/*								    */
/* **************************************************************** */

/* Return the length in bytes of the possibly-multibyte character beginning
   at S. Encoding is UTF-8. */
static int
_rl_utf8_mblen (const char *s, size_t n)
{
  unsigned char c, c1, c2, c3;

  if (s == 0)
    return (0);	/* no shift states */
  if (n <= 0)
    return (-1);

  c = (unsigned char)*s;
  if (c < 0x80)
    return (c != 0);
  if (c >= 0xc2)
    {
      c1 = (unsigned char)s[1];
      if (c < 0xe0)
	{
	  if (n == 1)
	    return -2;
	  if (n >= 2 && (c1 ^ 0x80) < 0x40)
	    return 2;
	}
      else if (c < 0xf0)
	{
	  if (n == 1)
	    return -2;
	  if ((c1 ^ 0x80) < 0x40
		&& (c >= 0xe1 || c1 >= 0xa0)
		&& (c != 0xed || c1 < 0xa0))
	    {
	      if (n == 2)
		return -2;
	      c2 = (unsigned char)s[2];
	      if ((c2 ^ 0x80) < 0x40)
		return 3;
	    }
	}
      else if (c < 0xf4)
	{
	  if (n == 1)
	    return -2;
	  if (((c1 ^ 0x80) < 0x40)
		&& (c >= 0xf1 || c1 >= 0x90)
		&& (c < 0xf4 || (c == 0xf4 && c1 < 0x90)))
	    {
	      if (n == 2)
		return -2;
	      c2 = (unsigned char)s[2];
	      if ((c2 ^ 0x80) < 0x40)
		{
		  if (n == 3)
		    return -2;
		  c3 = (unsigned char)s[3];
		  if ((c3 ^ 0x80) < 0x40)
		    return 4;
		}
	    }
	}
    }
  /* invalid or incomplete multibyte character */
  return -1;
}

static size_t
_rl_utf8_mbstrlen (const char *s)
{
  size_t clen, nc;
  int mb_cur_max;

  nc = 0;
  mb_cur_max = MB_CUR_MAX;
  while (*s && (clen = (size_t)_rl_utf8_mblen(s, mb_cur_max)) != 0)
    {
      if (MB_INVALIDCH (clen))
	clen = 1;
      s += clen;
      nc++;
    }
  return nc;
}

static size_t
_rl_gen_mbstrlen (const char *s)
{
  size_t clen, nc;
  mbstate_t mbs = { 0 }, mbsbak = { 0 };
  int f, mb_cur_max;

  nc = 0;
  mb_cur_max = MB_CUR_MAX;
  while (*s && (clen = (f = _rl_is_basic (*s)) ? 1 : mbrlen(s, mb_cur_max, &mbs)) != 0)
    {
      if (MB_INVALIDCH(clen))
	{
	  clen = 1;     /* assume single byte */
	  mbs = mbsbak;
	}

      if (f == 0)
	mbsbak = mbs;

      s += clen;
      nc++;
    }
  return nc;
}

size_t
_rl_mbstrlen (const char *s)
{
  if (MB_CUR_MAX == 1)
    return (strlen (s));
  else if (_rl_utf8locale)
    return (_rl_utf8_mbstrlen (s));
  else
    return (_rl_gen_mbstrlen (s));
}

static int
_rl_find_next_mbchar_internal (const char *string, int seed, int count, int find_non_zero)
{
  size_t tmp, len;
  mbstate_t ps;
  int point;
  WCHAR_T wc;

  tmp = 0;

  memset(&ps, 0, sizeof (mbstate_t));
  if (seed < 0)
    seed = 0;
  if (count <= 0)
    return seed;

  point = seed + _rl_adjust_point (string, seed, &ps);
  /* if _rl_adjust_point returns -1, the character or string is invalid.
     treat as a byte. */
  if (point == seed - 1)	/* invalid */
    return seed + 1;
    
  /* if this is true, means that seed was not pointing to a byte indicating
     the beginning of a multibyte character.  Correct the point and consume
     one char. */
  if (seed < point)
    count--;

  while (count > 0)  
    {
      len = strlen (string + point);
      if (len == 0)
	break;
      if (_rl_utf8locale && UTF8_SINGLEBYTE(string[point]))
	{
	  tmp = 1;
	  wc = (WCHAR_T) string[point];
	  memset(&ps, 0, sizeof(mbstate_t));
	}
      else
	tmp = MBRTOWC (&wc, string+point, len, &ps);
      if (MB_INVALIDCH ((size_t)tmp))
	{
	  /* invalid bytes. assume a byte represents a character */
	  point++;
	  count--;
	  /* reset states. */
	  memset(&ps, 0, sizeof(mbstate_t));
	}
      else if (MB_NULLWCH (tmp))
	break;			/* found wide '\0' */
      else
	{
	  /* valid bytes */
	  point += tmp;
	  if (find_non_zero)
	    {
	      if (WCWIDTH (wc) == 0)
		continue;
	      else
		count--;
	    }
	  else
	    count--;
	}
    }

  if (find_non_zero)
    {
      len = strlen (string + point);
      tmp = MBRTOWC (&wc, string + point, len, &ps);
      while (MB_NULLWCH (tmp) == 0 && MB_INVALIDCH (tmp) == 0 && WCWIDTH (wc) == 0)
	{
	  point += tmp;
	  len -= tmp;
	  tmp = MBRTOWC (&wc, string + point, len, &ps);
	}
    }

  return point;
}

static inline int
_rl_test_nonzero (const char *string, int ind, int len)
{
  size_t tmp;
  WCHAR_T wc;
  mbstate_t ps;

  memset (&ps, 0, sizeof (mbstate_t));
  tmp = MBRTOWC (&wc, string + ind, len - ind, &ps);
  /* treat invalid multibyte sequences as non-zero-width */
  return (MB_INVALIDCH (tmp) || MB_NULLWCH (tmp) || WCWIDTH (wc) > 0);
}

/* experimental -- needs to handle zero-width characters better */
static int
_rl_find_prev_utf8char (const char *string, int seed, int find_non_zero)
{
  unsigned char b;
  int save, prev;
  size_t len;

  if (find_non_zero)
    len = RL_STRLEN (string);

  prev = seed - 1;
  while (prev >= 0)
   {
      b = (unsigned char)string[prev];
      if (UTF8_SINGLEBYTE (b))
	return (prev);

      save = prev;

      /* Move back until we're not in the middle of a multibyte char */
      while (prev > 0 && (b = (unsigned char)string[--prev]) && UTF8_MBFIRSTCHAR (b) == 0);

      if (UTF8_MBFIRSTCHAR (b))
	{
	  if (find_non_zero)
	    {
	      if (_rl_test_nonzero (string, prev, len))
		return (prev);
	      else		/* valid but WCWIDTH (wc) == 0 */
		prev = prev - 1;
	    }
	  else
	    return (prev);
	}
      else
	return (save);			/* invalid utf-8 multibyte sequence */
    }

  return ((prev < 0) ? 0 : prev);
}  

/*static*/ int
_rl_find_prev_mbchar_internal (const char *string, int seed, int find_non_zero)
{
  mbstate_t ps;
  int prev, non_zero_prev, point, length;
  size_t tmp;
  WCHAR_T wc;

  if (_rl_utf8locale)
    return (_rl_find_prev_utf8char (string, seed, find_non_zero));

  memset(&ps, 0, sizeof(mbstate_t));
  length = strlen(string);
  
  if (seed < 0)
    return 0;
  else if (length < seed)
    return length;

  prev = non_zero_prev = point = 0;
  while (point < seed)
    {
      if (_rl_utf8locale && UTF8_SINGLEBYTE(string[point]))
	{
	  tmp = 1;
	  wc = (WCHAR_T) string[point];
	  memset(&ps, 0, sizeof(mbstate_t));
	}
      else
	tmp = MBRTOWC (&wc, string + point, length - point, &ps);
      if (MB_INVALIDCH ((size_t)tmp))
	{
	  /* in this case, bytes are invalid or too short to compose
	     multibyte char, so assume that the first byte represents
	     a single character anyway. */
	  tmp = 1;
	  /* clear the state of the byte sequence, because
	     in this case effect of mbstate is undefined  */
	  memset(&ps, 0, sizeof (mbstate_t));

	  /* Since we're assuming that this byte represents a single
	     non-zero-width character, don't forget about it. */
	  prev = point;
	}
      else if (MB_NULLWCH (tmp))
	break;			/* Found '\0' char.  Can this happen? */
      else
	{
	  if (find_non_zero)
	    {
	      if (WCWIDTH (wc) != 0)
		prev = point;
	    }
	  else
	    prev = point;  
	}

      point += tmp;
    }

  return prev;
}

/* return the number of bytes parsed from the multibyte sequence starting
   at src, if a non-L'\0' wide character was recognized. It returns 0, 
   if a L'\0' wide character was recognized. It  returns (size_t)(-1), 
   if an invalid multibyte sequence was encountered. It returns (size_t)(-2) 
   if it couldn't parse a complete  multibyte character.  */
int
_rl_get_char_len (const char *src, mbstate_t *ps)
{
  size_t tmp, l;
  int mb_cur_max;

  /* Look at no more than MB_CUR_MAX characters */
  l = strlen (src);
  if (_rl_utf8locale && l >= 0 && UTF8_SINGLEBYTE(*src))
    tmp = (*src != 0) ? 1 : 0;
  else
    {
      mb_cur_max = MB_CUR_MAX;
      tmp = mbrlen(src, (l < mb_cur_max) ? l : mb_cur_max, ps);
    }
  if (tmp == (size_t)(-2))
    {
      /* too short to compose multibyte char */
      if (ps)
	memset (ps, 0, sizeof(mbstate_t));
      return -2;
    }
  else if (tmp == (size_t)(-1))
    {
      /* invalid to compose multibyte char */
      /* initialize the conversion state */
      if (ps)
	memset (ps, 0, sizeof(mbstate_t));
      return -1;
    }
  else if (tmp == (size_t)0)
    return 0;
  else
    return (int)tmp;
}

/* compare the specified two characters. If the characters matched,
   return 1. Otherwise return 0. */
int
_rl_compare_chars (const char *buf1, int pos1, mbstate_t *ps1, const char *buf2, int pos2, mbstate_t *ps2)
{
  int i, w1, w2;

  if ((w1 = _rl_get_char_len (&buf1[pos1], ps1)) <= 0 || 
	(w2 = _rl_get_char_len (&buf2[pos2], ps2)) <= 0 ||
	(w1 != w2) ||
	(buf1[pos1] != buf2[pos2]))
    return 0;

  for (i = 1; i < w1; i++)
    if (buf1[pos1+i] != buf2[pos2+i])
      return 0;

  return 1;
}

/* adjust pointed byte and find mbstate of the point of string.
   adjusted point will be point <= adjusted_point, and returns
   differences of the byte(adjusted_point - point).
   if point is invalid (point < 0 || more than string length),
   it returns -1 */
int
_rl_adjust_point (const char *string, int point, mbstate_t *ps)
{
  size_t tmp;
  int length, pos;

  tmp = 0;
  pos = 0;
  length = strlen(string);
  if (point < 0)
    return -1;
  if (length < point)
    return -1;
  
  while (pos < point)
    {
      if (_rl_utf8locale && UTF8_SINGLEBYTE(string[pos]))
	tmp = 1;
      else
	tmp = mbrlen (string + pos, length - pos, ps);
      if (MB_INVALIDCH ((size_t)tmp))
	{
	  /* in this case, bytes are invalid or too short to compose
	     multibyte char, so assume that the first byte represents
	     a single character anyway. */
	  pos++;
	  /* clear the state of the byte sequence, because
	     in this case effect of mbstate is undefined  */
	  if (ps)
	    memset (ps, 0, sizeof (mbstate_t));
	}
      else if (MB_NULLWCH (tmp))
	pos++;
      else
	pos += tmp;
    }

  return (pos - point);
}

int
_rl_is_mbchar_matched (const char *string, int seed, int end, char *mbchar, int length)
{
  int i;

  if ((end - seed) < length)
    return 0;

  for (i = 0; i < length; i++)
    if (string[seed + i] != mbchar[i])
      return 0;
  return 1;
}

WCHAR_T
_rl_char_value (const char *buf, int ind)
{
  size_t tmp;
  WCHAR_T wc;
  mbstate_t ps;
  size_t l;

  if (MB_LEN_MAX == 1 || rl_byte_oriented)
    return ((WCHAR_T) buf[ind]);
  if (_rl_utf8locale && UTF8_SINGLEBYTE(buf[ind]))
    return ((WCHAR_T) buf[ind]);
  l = strlen (buf);
  if (ind + 1 >= l)
    return ((WCHAR_T) buf[ind]);
  if (l < ind)			/* Sanity check */
    l = strlen (buf+ind);
  memset (&ps, 0, sizeof (mbstate_t));
  tmp = MBRTOWC (&wc, buf + ind, l - ind, &ps);
  if (MB_INVALIDCH (tmp) || MB_NULLWCH (tmp))  
    return ((WCHAR_T) buf[ind]);
  return wc;
}
#endif /* HANDLE_MULTIBYTE */

/* Find next `count' characters started byte point of the specified seed.
   If flags is MB_FIND_NONZERO, we look for non-zero-width multibyte
   characters. */
#undef _rl_find_next_mbchar
int
_rl_find_next_mbchar (const char *string, int seed, int count, int flags)
{
#if defined (HANDLE_MULTIBYTE)
  return _rl_find_next_mbchar_internal (string, seed, count, flags);
#else
  return (seed + count);
#endif
}

/* Find previous character started byte point of the specified seed.
   Returned point will be point <= seed.  If flags is MB_FIND_NONZERO,
   we look for non-zero-width multibyte characters. */
#undef _rl_find_prev_mbchar
int
_rl_find_prev_mbchar (const char *string, int seed, int flags)
{
#if defined (HANDLE_MULTIBYTE)
  return _rl_find_prev_mbchar_internal (string, seed, flags);
#else
  return ((seed == 0) ? seed : seed - 1);
#endif
}

/* Compare the first N characters of S1 and S2 without regard to case. If
   FLAGS&1, apply the mapping specified by completion-map-case and make
   `-' and `_' equivalent. Returns 1 if the strings are equal. */
int
_rl_mb_strcaseeqn (const char *s1, size_t l1, const char *s2, size_t l2, size_t n, int flags)
{
  size_t v1, v2;
  mbstate_t ps1, ps2;
  WCHAR_T wc1, wc2;

  memset (&ps1, 0, sizeof (mbstate_t));
  memset (&ps2, 0, sizeof (mbstate_t));

  do
    {
      v1 = MBRTOWC(&wc1, s1, l1, &ps1);
      v2 = MBRTOWC(&wc2, s2, l2, &ps2);
      if (v1 == 0 && v2 == 0)
	return 1;
      else if (MB_INVALIDCH(v1) || MB_INVALIDCH(v2))
 	{
 	  int d;
 	  d = _rl_to_lower (*s1) - _rl_to_lower (*s2);	/* do byte comparison */
	  if ((flags & 1) && (*s1 == '-' || *s1 == '_') && (*s2 == '-' || *s2 == '_'))
	    d = 0;		/* case insensitive character mapping */
	  if (d != 0)
	    return 0;
	  s1++;
	  s2++;
	  n--;
	  continue;
 	}
      wc1 = towlower(wc1);
      wc2 = towlower(wc2);
      s1 += v1;
      s2 += v1;
      n -= v1;
      if ((flags & 1) && (wc1 == L'-' || wc1 == L'_') && (wc2 == L'-' || wc2 == L'_'))
	continue;
      if (wc1 != wc2)
	return 0;
    }
  while (n != 0);

  return 1;
}

/* Return 1 if the multibyte characters pointed to by S1 and S2 are equal
   without regard to case. If FLAGS&1, apply the mapping specified by
   completion-map-case and make `-' and `_' equivalent. */
int
_rl_mb_charcasecmp (const char *s1, mbstate_t *ps1, const char *s2, mbstate_t *ps2, int flags)
{
  int d;
  size_t v1, v2;
  wchar_t wc1, wc2;

  d = MB_CUR_MAX;
  v1 = MBRTOWC(&wc1, s1, d, ps1);
  v2 = MBRTOWC(&wc2, s2, d, ps2);
  if (v1 == 0 && v2 == 0)
    return 1;
  else if (MB_INVALIDCH(v1) || MB_INVALIDCH(v2))
    {
      if ((flags & 1) && (*s1 == '-' || *s1 == '_') && (*s2 == '-' || *s2 == '_'))
	return 1;
      return (_rl_to_lower (*s1) == _rl_to_lower (*s2));
    }
  wc1 = towlower(wc1);
  wc2 = towlower(wc2);
  if ((flags & 1) && (wc1 == L'-' || wc1 == L'_') && (wc2 == L'-' || wc2 == L'_'))
    return 1;
  return (wc1 == wc2);
}
