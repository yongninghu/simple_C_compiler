#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>

#include "auxlib.h"

using namespace std;

struct astree {
   int symbol;               // token code
   size_t filenr;            // index into filename stack
   size_t linenr;            // line number from source code
   size_t offset;            // offset of token with current line
   const string* lexinfo;    // pointer to lexical information
   struct symbol* syminfo;
   vector<astree*> children; // children of this n-way node
   astree* parent;
};

extern ofstream astfile;

astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo);
astree* adopt1 (astree* root, astree* child);
astree* adopt2 (astree* root, astree* left, astree* right);
astree* adopt3 (astree* root, astree* left, 
                astree* middle, astree* right);
astree* adopt1sym (astree* root, astree* child, int symbol);
astree* adopt2sym (astree* root, astree* child1, 
                   astree* child2, int symbol);
astree* adopt3sym (astree* root, astree* child1, astree* child2, 
                   astree* child3, int symbol);
astree* adoptFromParent (astree* root, astree* parent);
void dump_astree (astree* root);
void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep);
void free_ast (astree* tree);
void free_ast2 (astree* tree1, astree* tree2);
RCSH("$Id: astree.h,v 1.2 2013-10-11 18:52:46-07 - - $")
#endif
