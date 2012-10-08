#ifndef CHARTABLE_H
#define CHARTABLE_H
#include <stdio.h>
#include "bitfs.h"

typedef struct _chartable
{
  unsigned char table[32];
  unsigned int size;
} USEDCHARTABLE;

void chartable_init(USEDCHARTABLE *ut);
void chartable_set(USEDCHARTABLE *ut, unsigned char c);
void chartable_write(USEDCHARTABLE *ut, OBITFS *obfs);
void chartable_read(USEDCHARTABLE *ut, IBITFS *ibfs);
unsigned int chartable_test(USEDCHARTABLE *ut, unsigned char c);


#endif
