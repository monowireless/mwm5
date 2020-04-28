#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
/* note: changed name from getopt() to oss_getopt() to avoid conflict. */

#include "oss_getopt.h"

/*
* Copyright (c) 1987, 1993, 1994
*      The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by the University of
*      California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#include <string.h>
#include <stdio.h>

static ts_opt_getopt o = { .opterr = 1, .optind = 1 };
ts_opt_getopt* oss_getopt_ref() { return &o; }

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

/*
 * getopt --
 *      Parse argc/argv argument vector.
 */
int oss_getopt(int nargc, char * const nargv[], const char *ostr)
{
  static char *place = EMSG;              /* option letter processing */
  const char *oli;                              /* option letter list index */

  if (o.optreset || !*place) {              /* update scanning pointer */
    o.optreset = 0;
    if (o.optind >= nargc || *(place = nargv[o.optind]) != '-') {
      place = EMSG;
      return (-1);
    }
    if (place[1] && *++place == '-') {      /* found "--" */
      ++o.optind;
      place = EMSG;
      return (-1);
    }
  }                                       /* option letter okay? */
  if ((o.optopt = (int)*place++) == (int)':' ||
    !(oli = strchr(ostr, o.optopt))) {
      /*
      * if the user didn't specify '-' as an option,
      * assume it means -1.
      */
      if (o.optopt == (int)'-')
        return (-1);
      if (!*place)
        ++o.optind;
      if (o.opterr && *ostr != ':')
        (void)printf("illegal option -- %c\n", o.optopt);
      return (BADCH);
  }
  if (*++oli != ':') {                    /* don't need argument */
    o.optarg = NULL;
    if (!*place)
      ++o.optind;
  }
  else {                                  /* need an argument */
    if (*place)                     /* no white space */
      o.optarg = place;
    else if (nargc <= ++o.optind) {   /* no arg */
      place = EMSG;
      if (*ostr == ':')
        return (BADARG);
      if (o.opterr)
        (void)printf("option requires an argument -- %c\n", o.optopt);
      return (BADCH);
    }
    else                            /* white space */
      o.optarg = nargv[o.optind];
    place = EMSG;
    ++o.optind;
  }
  return (o.optopt);                        /* dump back option letter */
}

#endif // WIN/OSX/LINUX