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

#include "encoder.h"
#include "chartable.h"

uint bits (uint n)
{ uint b = 0;
  while (n)
    { b++; n >>= 1; }
  return b;
}

void putLeaf(uint numcode, uint lvcode, BITOUT *bitout) {
  uint bitslen = bits(numcode);
  writeBits(bitout, lvcode, bitslen);
}

void putParen(uchar b, BITOUT *bitout) {
  if (b == OP) {
    writeBits(bitout, OP, 1);
  }
  else {
    writeBits(bitout, CP, 1);
  }
}


void encodeCFG_rec(uint code, EDICT *dict, BITOUT *bitout, OBITFS *obf, USEDCHARTABLE *ut) {
  static uint newcode = CHAR_SIZE;

  unsigned int i;
  unsigned int width = ut->size + dict->num_usedrules - CHAR_SIZE;
  // �ȉ��Clog�����߂�D
  if(width >= 1) width = ceil(log(width)/log(2.0));
  //width |= width >>  1;
  //width |= width >>  2;
  //width |= width >>  4;
  //width |= width >>  8;
  //width |= width >> 16;
  //width = (width & 0x55555555) + ((width >>  1) & 0x55555555);
  //width = (width & 0x33333333) + ((width >>  2) & 0x33333333);
  //width = (width & 0x0F0F0F0F) + ((width >>  4) & 0x0F0F0F0F);
  //width = (width & 0x00FF00FF) + ((width >>  8) & 0x00FF00FF);
  //width = (width & 0x0000FFFF) + ((width >> 16) & 0x0000FFFF);
  
  for(i = CHAR_SIZE; i < dict->num_usedrules; i++){
    obitfs_put(obf, dict->tcode[dict->rule[i].left],  width);
    obitfs_put(obf, dict->tcode[dict->rule[i].right], width);
  }
}

void ExpandandWrite(EDICT *dict, OBITFS *obf, CODE cod, uint width){
  if(cod >= dict->num_usedrules){
    ExpandandWrite(dict, obf, dict->rule[cod].left, width);
    ExpandandWrite(dict, obf, dict->rule[cod].right, width);
  }else{
    obitfs_put(obf, dict->tcode[cod], width);
  }
}



void EncodeCFG(EDICT *dict, OBITFS *obfs, unsigned int codewordlength) {
  uint i;
  for (i = 0; i < dict->seq_len; i++) {
    obitfs_put(obfs, dict->tcode[dict->comp_seq[i]], codewordlength);
  }
}

void outputHeader(OBITFS *obfs, DICT *dict, unsigned int codewordlength, unsigned int blocklength, USEDCHARTABLE *ut) {
  obitfs_put(obfs, (dict->txt_len) >> 32, 32);
  obitfs_put(obfs, (dict->txt_len) & 0xFFFFFFFF, 32);
  obitfs_put(obfs, codewordlength, 5);
  obitfs_put(obfs, blocklength,   32);
  chartable_write(ut, obfs);
}

void outputSharedDictionary(OBITFS *obfs, EDICT *dict, USEDCHARTABLE *ut, unsigned int codewordlength, unsigned int shared_dictsize, unsigned int blocknum) {
  unsigned int i;
  unsigned int s_size = dict->num_rules < shared_dictsize + CHAR_SIZE - ut->size ? dict->num_rules : shared_dictsize + CHAR_SIZE - ut->size;
  //  printf("shareddicsize = %d\n", s_size + ut->size - CHAR_SIZE);
  obitfs_put(obfs, s_size + ut->size - CHAR_SIZE, codewordlength);
  //  puts("********** SHARED DICTIONARY **********");
  for (i = CHAR_SIZE; i < s_size; i++) {
    //    printf("%d -> %d, %d\n", i, dict->tcode[dict->rule[i].left], dict->tcode[dict->rule[i].right]);
    //    puts("share");
    obitfs_put(obfs, dict->tcode[dict->rule[i].left],  codewordlength);
    obitfs_put(obfs, dict->tcode[dict->rule[i].right], codewordlength);
  }
  /* for (i = 0; i < blocknum; i++) { */
  /*   //    printf("localdic #%d = 0\n", i); */
  /*   obitfs_put(obfs, 0, codewordlength); */
  /* } */
}

void outputLocalDictionary(OBITFS *obfs, EDICT *dict, USEDCHARTABLE *ut, unsigned int codewordlength, unsigned int shared_dictsize, unsigned int blocknum) {
  unsigned int i;
  //  unsigned int share_size = dict->num_rules + ut->size - CHAR_SIZE < shared_dictsize ? dict->num_rules + ut->size - CHAR_SIZE : shared_dictsize; 
  obitfs_put(obfs, dict->num_rules + ut->size - shared_dictsize - CHAR_SIZE, codewordlength);
  //  printf("localdic #%d = %d\n", blocknum, dict->num_rules + ut->size - share_size - CHAR_SIZE);
  //  printf("sharedicsize = %d\n", shared_dictsize);
    
  for (i = shared_dictsize + CHAR_SIZE - ut->size; i < dict->num_rules; i++) {
    //    putchar('&');
    //    printf("%d -> %d, %d\n", i, dict->tcode[dict->rule[i].left], dict->tcode[dict->rule[i].right]);
    obitfs_put(obfs, dict->tcode[dict->rule[i].left],  codewordlength);
    obitfs_put(obfs, dict->tcode[dict->rule[i].right], codewordlength);    
  }
}

EDICT *ReadCFG(FILE *input) {
  uint i;
  uint num_rules, seq_len, txt_len;
  EDICT *dict;
  RULE *rule;
  CODE *comp_seq;
  CODE *tcode;

  fread(&txt_len, sizeof(uint), 1, input);
  fread(&num_rules, sizeof(uint), 1, input);
  fread(&seq_len, sizeof(uint), 1, input);
  printf("txt_len = %d\n", txt_len);
  printf("num_rules = %d\n", num_rules);
  printf("seq_len = %d\n", seq_len);

  rule = (RULE*)malloc(sizeof(RULE)*(num_rules));

  //chartable�̓ǂݍ���
  for (i = 0; i <= CHAR_SIZE; i++) {
    rule[i].left = (CODE)i;
    rule[i].right = DUMMY_CODE;
  }
  fread(rule+CHAR_SIZE+1, sizeof(RULE), num_rules-(CHAR_SIZE+1), input);

  // seq�o�b�t�@
  comp_seq = (CODE*)malloc(sizeof(CODE)*seq_len);
  fread(comp_seq, sizeof(CODE), seq_len, input);

  // tcode�̓ǂݍ���
  tcode = (CODE*)malloc(sizeof(CODE)*num_rules);
  for (i = 0; i <= CHAR_SIZE; i++) {
    tcode[i] = i;
  }
  for (i = CHAR_SIZE+1; i < num_rules; i++) {
    tcode[i] = DUMMY_CODE;
  }

  dict = (EDICT*)malloc(sizeof(EDICT));
  dict->txt_len = txt_len;
  dict->num_rules = num_rules;
  dict->rule = rule;
  dict->seq_len = seq_len;
  dict->comp_seq = comp_seq;
  dict->tcode = tcode;
  return dict;
}

void DestructEDict(EDICT *dict)
{
  free(dict->rule);
  free(dict->comp_seq);
  free(dict->tcode);
  dict->rule = NULL;
  dict->comp_seq = NULL;
  dict->tcode = NULL;
  free(dict);
}

void CleanEDict(EDICT *dict)
{
  free(dict->tcode);
  free(dict);
}

void fill_chartable(FILE *input, USEDCHARTABLE *ut)
{
  int c;
  rewind(input);
  while ((c = getc(input)) != EOF)
    chartable_set(ut, (unsigned char)(c));
  printf("text size = %lld(bytes)\n", ftello(input));
  rewind(input);
}
