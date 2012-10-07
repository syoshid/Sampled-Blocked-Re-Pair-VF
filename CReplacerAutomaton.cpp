//===================================================================
//  Replacer Automaton class
//
#include "CReplacerAutomaton.h"

const int GOTO_FAIL(-1);
const int CHAR_SIZE(256);

CReplacerAutomaton::CReplacerAutomaton()
{
  num_states = 0;
  ipats.reserve(128);
  ipats.push_back("");
  opats.reserve(128);
  opats.push_back("");
}

//-------------------------------------------------------------------
// enter pattern and return the ID
int CReplacerAutomaton::enter(string ipat, string opat)
{
  if (ipat == "") { // null pattern
    return -1;
  }
  vector<string>::iterator p = find(ipats.begin(), ipats.end(), ipat);
  if (p != ipats.end()) { // there is the same pattern
    return -2;
  }
  ipats.push_back(ipat);
  opats.push_back(opat);
  return ipats.size();
}


//-------------------------------------------------------------------
//
void CReplacerAutomaton::run(ifstream& fin, ofstream& fout)
{
  make_goto_function();
  make_failure_function();
  replacing(fin, fout);
}

//-------------------------------------------------------------------
//
void CReplacerAutomaton::make_goto_function()
{
  // initialize g[][] and o[][]
  static vector<int> nul_g(CHAR_SIZE, GOTO_FAIL); //nul_g[0..CHAR_SIZE]=GOTO_FAIL;
  static string nul_o("");
  g.push_back(nul_g); // for initial state
  o.push_back(nul_o); // for initial state

  for (int id=1; id<ipats.size(); id++) {
    int j;
    int state(0);
    int m = ipats[id].length();
    // retrieve goto
    for (j = 0; g[state][static_cast<int>(ipats[id][j])] != GOTO_FAIL && (j < m); j++) {
      state = g[state][static_cast<int>(ipats[id][j])];
    }
    // create new state
    for (int k=j; k<m; k++) {
      g.push_back(nul_g);
      o.push_back(nul_o);
      num_states++;
      g[state][static_cast<int>(ipats[id][k])] = num_states;
      state = num_states;
    }
    // add the output for the leaf
    o[state] = opats[id];
  }

  for (int i=0; i<CHAR_SIZE; i++) {
    if (g[0][i] == GOTO_FAIL)
      g[0][i] = 0;
  }
}


//-------------------------------------------------------------------
//
void CReplacerAutomaton::make_failure_function()
{
  queue<int> q;
  f.reserve(num_states+1);
  
  for (int i=0; i<CHAR_SIZE; i++) {
    if (g[0][i] != 0) {
      int s = g[0][i];
      q.push(s);
      f[s] = 0;
      if (o[s] == "") {
        o[s] = static_cast<unsigned char>(i);
      }
    }
  }
  
  while (!q.empty()) {
    int r = q.front();
    q.pop();
    for (int i=0; i<CHAR_SIZE; i++) {
      if (g[r][i] == GOTO_FAIL) {
	continue;
      }
      int s = g[r][i];
      q.push(s);
      if (o[s] != "") {
	f[s] = 0;
      } else {
	int state = f[r];
        o[s] = o[r];
	while (g[state][i] == GOTO_FAIL) {
	  o[s] += o[state];
	  state = f[state];
	}
	f[s] = g[state][i];
	if (f[s] == 0) {
	  o[s] += static_cast<unsigned char>(i);
	}
      }
    }
  }
}

//-------------------------------------------------------------------
//
void CReplacerAutomaton::replacing(ifstream& fin, ofstream& fout)
{
  unsigned char c;
  int state(0);

  while (fin.get(c)) {
    while ((g[state][c]) == GOTO_FAIL) {
      fout << o[state];
      state = f[state];
    }
    state = g[state][c];
    if (state == 0)
      fout << c;
  }
  while (state != 0) { 
    fout << o[state];
    state = f[state];
  }
}
