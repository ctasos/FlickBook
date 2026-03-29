#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define GRAYSCALE 0
#define NUMITEMS(arg) ((unsigned int)(sizeof(arg) / sizeof(arg[0])))

#if GRAYSCALE == 1
#define WHITE 65535
#define BLACK 0
#define PARTIAL_UPDATE_ALLOWED 0
#define PARTIAL_UPDATE_LIMIT_N 0
#else
#define WHITE 0
#define BLACK 1
#define PARTIAL_UPDATE_ALLOWED 1
#define PARTIAL_UPDATE_LIMIT_N 7
#endif

#endif
