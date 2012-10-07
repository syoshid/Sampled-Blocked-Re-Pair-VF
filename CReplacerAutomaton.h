// -*- c++ -*-
//===================================================================
//  Replacer Automaton class
//
#ifndef REPLACER_AUTOMATON
#define REPLACER_AUTOMATON

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
using namespace std;

class CReplacerAutomaton {
private:
  int num_states;
  vector<basic_string<unsigned int> > ipats;
  vector<basic_string<unsigned int> > opats;
  vector< vector<int> > g;
  vector<int> f;
  vector<basic_string<unsigned int> > o;

  virtual void make_goto_function();
  virtual void make_failure_function();
  virtual unsigned int replacing(unsigned char *in, unsigned int *out, unsigned int len);
  
public:
  CReplacerAutomaton();
  int enter(basic_string<unsigned int> ipat, basic_string<unsigned int> opat);
  unsigned int run(unsigned char *in, unsigned int *out, unsigned int len);
};

#endif
