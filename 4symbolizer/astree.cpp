
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include "sym.h"
astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo) {
   astree* tree = new astree();
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->syminfo = 0;
   tree->parent = 0;
   tree->lexinfo = intern_stringset (lexinfo);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   child->parent = root;
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}

astree* adopt3 (astree* root, astree* left, astree* middle, 
               astree* right) {
   adopt1 (root, left);
   adopt1 (root, middle);
   adopt1 (root, right);
   return root;
}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}

astree* adopt2sym (astree* root, astree* child1, 
                  astree* child2, int symbol) {
   root = adopt2 (root, child1, child2);
   root->symbol = symbol;
   return root;
}

astree* adopt3sym (astree* root, astree* child1, 
                  astree* child2, astree* child3, int symbol) {
   root = adopt3 (root, child1, child2, child3);
   root->symbol = symbol;
   return root;
}

astree* adoptFromParent (astree* root, astree* parent) {
   root->children.insert(root->children.end(), 
                parent->children.begin(), parent->children.end());
   for(vector<astree*>::iterator it = root->children.begin();
       it != root->children.end(); ++ it) {
      (*it)->parent = root;
   } 
   delete parent;
   return root;
}


static void dump_node (FILE* outfile, astree* node) {
   fprintf (outfile, "%p->{%s(%d) %ld:%ld.%03ld \"%s\" [",
            node, get_yytname (node->symbol), node->symbol,
            node->filenr, node->linenr, node->offset,
            node->lexinfo->c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
      fprintf (outfile, "%p", node->children.at(child));
   }
   fprintf (outfile, "]}");
}

static void dump_astree_rec (astree* root,
                             int depth) {
   if (root == NULL) return;
   for(int i = 0; i < depth; ++i) { astfile << "|  "; }
   string tokename = get_yytname(root->symbol);
   if(tokename.length() > 4) {
      if(tokename.substr(0, 4) == "TOK_") {
         tokename = tokename.substr(4, tokename.length());
      }
   }
   astfile << tokename << " " << "\"" << *(root->lexinfo) << "\""
           << " " << root->filenr << "." << root->linenr
           << "." << root->offset << " ";
   if(root->syminfo != 0) {
      symbol* sym = root->syminfo;
      if(sym->attributes.test(ATTR_field)) {
         astfile << " field {" << *sym->fieldId << "}";
      } else {
         astfile << " {" << sym->block_nr << "}";
      }
      if(sym->attributes.test(ATTR_struct)) {
         astfile << " struct \"" << *sym->typeId << "\"";
      }
       if(sym->attributes.test(ATTR_void)) {
         astfile << " void";
      }
      if(sym->attributes.test(ATTR_bool)) {
         astfile << " bool";
      }
      if(sym->attributes.test(ATTR_array)) {
         astfile << " array";
      }
      if(sym->attributes.test(ATTR_int)) {
         astfile << " int";
      }
      if(sym->attributes.test(ATTR_char)) {
         astfile << " char";
      }
      if(sym->attributes.test(ATTR_string)) {
         astfile << " string";
      }
      if(sym->attributes.test(ATTR_null)) {
         astfile << " null";
      }
      if(sym->attributes.test(ATTR_const)) {
         astfile << " const";
      }
      if(sym->attributes.test(ATTR_vreg)) {
         astfile << " vreg";
      }
      if(sym->attributes.test(ATTR_variable)) {
         astfile << " variable";
      }
      if(sym->attributes.test(ATTR_vaddr)) {
         astfile << " vaddr";
      }
      if(sym->attributes.test(ATTR_lval)) {
         astfile << " lval";
      }
      if(sym->attributes.test(ATTR_param)) {
         astfile << " param";
      }
      if(sym->attributes.test(ATTR_function)) {
         astfile << " function";
      }
   }
   astfile << endl;
   for (size_t child = 0; child < root->children.size();
        ++child) {
      dump_astree_rec (root->children[child],
                       depth + 1);
   }
}

void dump_astree (astree* root) {
   dump_astree_rec (root, 0);
   fflush (NULL);
}

void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n",
               get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
           root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}

RCSC("$Id: astree.cpp,v 1.1 2014-10-03 18:22:05-07 - - $")

