/* 
 *  Copyright (c) 2011 Shirou Maruyama
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *   1. Redistributions of source code must retain the above Copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above Copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 */


#include "decoder.h"
#include <stdio.h>

#define BUFF_SIZE 32768
char buffer[BUFF_SIZE];
char table[CHAR_SIZE];
uint bufpos = 0;

uint bits (uint n);
//void expandLeaf(RULE *rule, CODE code, FILE *output, USEDCHARTABLE *ut);

uint bits (uint n)
{ uint b = 0;
  while (n)
    { b++; n >>= 1; }
  return b;
}

void expandLeaf(RULE *rule, CODE leaf, FILE *output, USEDCHARTABLE *ut, uint64 *len) {
  if (leaf < ut->size) {
    buffer[bufpos] = rule[leaf].left;
    (*len)++;
    bufpos++;
    if (bufpos == BUFF_SIZE) {
      fwrite(buffer, 1, BUFF_SIZE, output);
      bufpos = 0;
    }
    return;
  }
  else {
    expandLeaf(rule, rule[leaf].left, output, ut, len);
    expandLeaf(rule, rule[leaf].right, output, ut, len);
    return; 
  }
}

void load_local_dictionary(IBITFS *dict_stream, RULE *rule, USEDCHARTABLE *ut, unsigned int shareddicsize, unsigned int width)
{
  unsigned int i;
  unsigned int localdicsize = ibitfs_get(dict_stream, width);
  //  printf("localdicsize = %d\n", localdicsize);

  for (i = shareddicsize; i < shareddicsize + localdicsize; i++) {
    rule[i].left  = ibitfs_get(dict_stream, width);
    rule[i].right = ibitfs_get(dict_stream, width);
    //    printf("%d -> %d, %d\n", i + CHAR_SIZE - ut->size, rule[i].left, rule[i].right);
  }
}


//���̊֐���������
void DecodeCFG(FILE *output, IBITFS *input, IBITFS *dict) {
  uint i;
  RULE *rule;
  uint num_rules, seq_len;
  uint64 txt_len;
  BITIN *bitin;
  IBITFS ibfs;
  uint exc, sp;
  CODE *stack;
  CODE newcode, leaf;
  uint cod;
  uint bitlen;
  uint64 currentlen;
  bool paren;
  uint width = 1; // warning: ��Ō��߂�K�v������D
  uint blocklength;
  USEDCHARTABLE ut;
  uint shareddicsize;
  uint b = 0;

  chartable_init(&ut);
  txt_len     = ibitfs_get(dict, 32);
  txt_len     <<= 32;
  txt_len     |= ibitfs_get(dict, 32);
  width       = ibitfs_get(dict,  5);
  blocklength = ibitfs_get(dict, 32);
  chartable_read(&ut, dict);
  printf("txt_len = %lld, codeword length = %d, block length = %d, ", 
	 txt_len, width, blocklength);



  rule = (RULE*)malloc(sizeof(RULE)*(1 << width));


  // rule�����l�̓ǂݍ���
  uint j = 0;
  for (i = 0; i < CHAR_SIZE; i++) {
    if(chartable_test(&ut, (unsigned char)i)){
      rule[j].left = (CODE)i;
      rule[j].right = DUMMY_CODE;
      j++;
    }
  }


  // �e�u���b�N�ɂ��ă��[�v (currentlen��blocklength�̔{���ɂȂ邲�Ƃɔ���)
  // ���[�J��������ǂݍ���
  // �����ɏ]���ăf�R�[�h����


  // shared������ǂݍ���
  shareddicsize = ibitfs_get(dict, width);
  //  printf("charsize = %d\n", ut.size);
  printf("shared dictionary size = %d\n", shareddicsize);
  printf("Decoding CFG...");
  fflush(stdout);
  
  for (; j < shareddicsize; j++) {
    rule[j].left  = ibitfs_get(dict, width);
    rule[j].right = ibitfs_get(dict, width);
    //    printf("%d -> %d, %d\n", j - ut.size + CHAR_SIZE, rule[j].left, rule[j].right);
  }

  currentlen = 0;
  //  load_local_dictionary(dict, rule, &ut, shareddicsize, width);
  while (currentlen < txt_len) {
    if (currentlen % blocklength == 0) {
      load_local_dictionary(dict, rule, &ut, shareddicsize, width);
      b++;
    }
    cod = ibitfs_get(input, width);
    expandLeaf(rule, (CODE) cod, output, &ut, &currentlen);
  }
  fwrite(buffer, 1, bufpos, output);
  printf("Finished!\n");
  free(rule);
  return;
}

