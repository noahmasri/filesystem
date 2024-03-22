#ifndef __PAR__H__
#define __PAR__H__

#define __USE_XOPEN
#define _GNU_SOURCE
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

void parsing(const char *path);
void unparsing(char *path);

#endif