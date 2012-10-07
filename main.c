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

#ifdef REPAIR
#include <stdlib.h>
#include <getopt.h>
#include "repair.h"
#include "encoder.h"
#include "string.h"
#include "chartable.h"
#include "CReplacerAutomaton.h"
#include <string>

using namespace std;

EDICT *convertDict(DICT *dict, USEDCHARTABLE *ut)
{
  EDICT *edict = (EDICT*)malloc(sizeof(EDICT));
  uint i;
  uint code = 0;
  edict->txt_len = dict->txt_len;
  edict->seq_len = dict->seq_len;
  edict->num_rules = dict->num_rules;
  edict->num_usedrules = dict->num_usedrules;
  edict->comp_seq = dict->comp_seq;
  edict->rule  = dict->rule;
  edict->tcode = (CODE*)malloc(sizeof(CODE)*dict->num_rules);

  for (i = 0; i < CHAR_SIZE; i++) {
    if(chartable_test(ut, i))
      edict->tcode[i] = code++;
  }
  for(i = CHAR_SIZE; i < dict->num_rules; i++){
    edict->tcode[i] = code++;
  }
  
  return edict;
}

basic_string<unsigned int> expand(DICT *dict, CODE i)
{
  basic_string<unsigned int> res;
  if (i <= CHAR_SIZE) {
    res = expand(dict, dict->rule[i].left) + expand(dict, dict->rule[i].right);
  } else {
    res += i;
  }
  return res;
}
  

void help(char **argv)
{
   printf("Usage: %s -r <input filename> -w <output filename> -d <dicitonary filename> -b <block size> -l <codeword length> -c <common_dictinoary size>\n"
	   "Compresses <input filename> with repair and creates "
	   "<output filename> compressed files\n\n", argv[0]);
   exit(EXIT_FAILURE);
}
int main(int argc, char *argv[])
{
  char *target_filename = NULL;
  //char output_filename[1024];
  char *output_filename = NULL;
  char *dict_filename = NULL;
  unsigned int codewordlength = 0;
  unsigned int shared_dictsize = 0;
  unsigned int block_length = 0;
  unsigned int length;
  char *rest;
  FILE *input, *output, *dictfile;
  DICT *dict;
  EDICT *edict;
  USEDCHARTABLE ut;
  int result;
  unsigned int b;
  unsigned char *buf;
  unsigned int  *buf2;
  OBITFS seqout, dicout;
  int header_output = 0;

   /* オプションの解析 */
  while ((result = getopt(argc, argv, "r:w:b:l:d:s:")) != -1) {
    switch (result) {
    case 'r':
      target_filename = optarg;
      break;
      
    case 'w':
      output_filename = optarg;
      break;
      
    case 'd':
      dict_filename = optarg;
      break;
      
    case 'b':
      block_length = strtoul(optarg, &rest, 10);
      if (*rest != '\0') {
	help(argv);
      }
      break;
      
    case 'l':
      codewordlength = strtoul(optarg, &rest, 10);
      if (*rest != '\0') {
	help(argv);
      }
      break;
      
    case 's':
      shared_dictsize = strtoul(optarg, &rest, 10);
      if (*rest != '\0') {
	help(argv);
      }
      break;
      
    case '?':
      help(argv);
      break;
    }
  }

  // 必要なオプションがそろっているかを確認する
  if (!(target_filename && output_filename && dict_filename && block_length && codewordlength && shared_dictsize)) {
    help(argv);
  }
  
  // 入力ファイルをオープンする
  input  = fopen(target_filename, "r");
  if (input == NULL) {
    puts("Input file open error at the beginning.");
    exit(1);
  }
  
  // 圧縮データファイルをオープンする
  output = fopen(output_filename, "wb");
  if (output == NULL) {
    puts("Output file open error at the beginning.");
    exit(1);
  }
  
  // 辞書ファイルをオープンする
  dictfile = fopen(dict_filename, "wb");
  if (!dictfile) {
    puts("Dictionary file open error at the beginning.");
    exit(EXIT_FAILURE);
  }

  if (NULL == (buf = (unsigned char*)malloc(sizeof(unsigned char) * block_length)) || NULL == (buf2 = (unsigned int*)malloc(sizeof(unsigned int) * block_length))) {
    puts("malloc fault.");
    exit(EXIT_FAILURE);
  }

  chartable_init(&ut);
  fill_chartable(input, &ut);
  fseek(input, 0, SEEK_END);
  dict = createDict(ftell(input));
  fseek(input, 0, SEEK_SET);
  b = 0;
  obitfs_init(&seqout, output);
  obitfs_init(&dicout, dictfile);
  printf("Generating CFG..."); fflush(stdout);
  outputHeader(&dicout, dict, (unsigned int) codewordlength, (unsigned int) block_length, &ut);
  while (!feof(input)) {
    length = fread(buf, sizeof(unsigned char), block_length, input);
    {
      // run replacerautomaton
      CReplacerAutomaton acm;
      unsigned int sdict_size = dict->num_rules >= shared_dictsize + CHAR_SIZE - ut.size ? shared_dictsize + CHAR_SIZE - ut.size : dict->num_rules;
      unsigned int i;
      for (i = CHAR_SIZE; i < sdict_size; i++) {
	acm.enter(expand(dict, i), basic_string<unsigned int>(1, i));
      }
      length = acm.run(buf, buf2, length);
    } 
      
    dict = RunRepair(dict, buf2, length, shared_dictsize, codewordlength, &ut);
    edict = convertDict(dict, &ut);
    if (dict->num_rules - CHAR_SIZE + ut.size >= shared_dictsize && !header_output) {
      header_output = 1;
      outputSharedDictionary(&dicout, edict, &ut, codewordlength, shared_dictsize, b);
    }
    if (header_output) {
      outputLocalDictionary(&dicout, edict, &ut, codewordlength, shared_dictsize, b);
    }
    EncodeCFG(edict, &seqout, codewordlength);
    DestructEDict(edict);
    b++;
  }
  free(dict);

  fclose(input);
  fclose(output);
  fclose(dictfile);
  exit(0);
}
#endif

#ifdef DESPAIR
#include "decoder.h"
int main(int argc, char *argv[])
{
  //char input_filename[1024];
  char *input_filename;
  FILE *input, *output;

  if (argc != 3) {
    printf("Usage: %s <input filename> <output filename>\n"
	   "Restore <output filename> from <input filename>\n\n", argv[0]);
    exit(1);
  }
  
  //strcpy(input_filename, argv[1]);
  //strcat(input_filename, ".rp");
  input_filename = argv[1];

  input = fopen(input_filename, "rb");
  output = fopen(argv[2], "w");
  if (input == NULL || output == NULL) {
    printf("File open error.\n");
    exit(1);
  }
  DecodeCFG(input, output);
  fclose(input); fclose(output);
  exit(0);
}
#endif

#ifdef TXT2ENC
#include "repair.h"
#include "encoder.h"

EDICT *convertDict(DICT *dict)
{
  EDICT *edict = (EDICT*)malloc(sizeof(EDICT));
  uint i;
  edict->txt_len = dict->txt_len;
  edict->seq_len = dict->seq_len;
  edict->num_rules = dict->num_rules;
  edict->comp_seq = dict->comp_seq;
  edict->rule  = dict->rule;
  edict->tcode = (CODE*)malloc(sizeof(CODE)*dict->num_rules);

  for (i = 0; i <= CHAR_SIZE; i++) {
    edict->tcode[i] = i;
  }
  for (i = CHAR_SIZE+1; i < dict->num_rules; i++) {
    edict->tcode[i] = DUMMY_CODE;
  }

  free(dict);
  return edict;
}

int main(int argc, char *argv[])
{
  char *target_filename;
  char *output_filename;
  FILE *input, *output;
  DICT *dict;
  EDICT *edict;

  if (argc != 3) {
    printf("usage: %s target_text_file output_file\n", argv[0]);
    exit(1);
  }
  target_filename = argv[1];
  output_filename = argv[2];
  
  input  = fopen(target_filename, "r");
  output = fopen(output_filename, "wb");
  if (input == NULL || output == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }

  dict = RunRepair(input);
  edict = convertDict(dict);
  EncodeCFG(edict, output);
  DestructEDict(edict);

  fclose(input);
  fclose(output);
  exit(0);
}
#endif

#ifdef ENC2TXT
#include "decoder.h"
int main(int argc, char *argv[])
{
  FILE *input, *output;

  if (argc != 3) {
    printf("usage: %s target_enc_file output_txt_file\n", argv[0]);
    puts("argument error.");
    exit(1);
  }
  input = fopen(argv[1], "rb");
  output = fopen(argv[2], "w");
  if (input == NULL || output == NULL) {
    printf("File open error.\n");
    exit(1);
  }
  DecodeCFG(input, output);
  fclose(input); fclose(output);
  exit(0);
}
#endif

#ifdef TXT2CFG
#include "repair.h"

int main(int argc, char *argv[])
{
  char *target_filename;
  char *output_filename;
  FILE *input, *output;
  DICT *dict;

  if (argc != 3) {
    printf("usage: %s target_text_file output_file\n", argv[0]);
    exit(1);
  }
  target_filename = argv[1];
  output_filename = argv[2];
  
  input  = fopen(target_filename, "r");
  output = fopen(output_filename, "wb");
  if (input == NULL || output == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }

  dict = RunRepair(input);
  OutputGeneratedCFG(dict, output);
  DestructDict(dict);

  fclose(input);
  fclose(output);
  exit(0);
}
#endif

#if CFG2ENC
#include "encoder.h"

int main(int argc, char *argv[])
{
  FILE *input, *output;
  EDICT *dict;

  if (argc != 3) {
    printf("usage: %s target_cfg_file output_enc_file\n", argv[0]);
    exit(1);
  }
  input = fopen(argv[1], "rb");
  output = fopen(argv[2], "wb");
  if (input == NULL || output == NULL) {
    printf("File open error.\n");
    exit(1);
  }
  dict = ReadCFG(input);
  EncodeCFG(dict, output);
  DestructEDict(dict);
  fclose(input); fclose(output);
  exit(0);
}
#endif
