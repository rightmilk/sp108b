#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "lexer.h"
#include "ir.h"

extern void parse();

#define GOTOLABEL_MAX 10000

typedef struct {
  int index;
  char *label;
} GOTOLABEL;

#endif