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
  vector<string> ipats;
  vector<string> opats;
  vector< vector<int> > g;
  vector<int> f;
  vector<string> o;

  virtual void make_goto_function();
  virtual void make_failure_function();
  virtual void replacing(ifstream& fin, ofstream& fout);
  
public:
  CReplacerAutomaton();
  int enter(string ipat, string opat);
  void run(ifstream& fin, ofstream& fout);
};

#endif
