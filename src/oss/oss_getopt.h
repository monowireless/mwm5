#pragma once

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _oss_getopt_vars {
    int
        opterr,             /* if error message should be printed */
        optind,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
    char* optarg;         /* argument associated with option */
} ts_opt_getopt;

int oss_getopt(int nargc, char* const nargv[], const char* ostr);
ts_opt_getopt* oss_getopt_ref();
#ifdef __cplusplus
}
#endif

#endif