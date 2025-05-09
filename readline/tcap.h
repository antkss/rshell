/* tcap.h -- termcap library functions and variables. */

/* Copyright (C) 1996-2015,2023 Free Software Foundation, Inc.

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

#if !defined (_RLTCAP_H_)
#define _RLTCAP_H_

#include "config.h"

#include <termcap.h>
#if defined (HAVE_TERMCAP_H)
#  include <termcap.h>
/* On Solaris2, sys/types.h #includes sys/reg.h, which #defines PC.
   Unfortunately, PC is a global variable used by the termcap library. */
#ifdef PC
#  undef PC
#endif

extern char PC;
extern char *UP, *BC;

extern short ospeed;

extern int tgetent (char *, const char *);
extern int tgetflag (const char *);
extern int tgetnum (const char *);
extern char *tgetstr (const char *, char **);

extern int tputs (const char *, int, int (*)(int));

extern char *tgoto (const char *, int, int);

#endif /* HAVE_TERMCAP_H */

#endif /* !_RLTCAP_H_ */
