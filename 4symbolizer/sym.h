#ifndef __SYM_H__
#define __SYM_H__

#include <unordered_map>
#include <bitset>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "astree.h"
using namespace std;
extern ofstream symfile;

enum { ATTR_void, ATTR_bool, ATTR_char, ATTR_int, ATTR_null,
       ATTR_string, ATTR_struct, ATTR_array, ATTR_function,
       ATTR_variable, ATTR_field, ATTR_typeid, ATTR_param, ATTR_lval,
       ATTR_const, ATTR_vreg, ATTR_vaddr, ATTR_bitset_size,
};

using attr_bitset = bitset<ATTR_bitset_size>;

struct symbol;
using symbol_table = unordered_map<const string*, symbol*>;
using symbol_entry = symbol_table::value_type;

struct symbol {
   attr_bitset attributes;
   symbol_table* fields;
   size_t filenr, linenr, offset;
   size_t block_nr;
   vector<symbol *>* parameters;
   const string* typeId;
   const string* fieldId;
};

//using symbol_table = unordered_map<string*, symbol*>;
//using symbol_entry = symbol_table::value_type;
void symbolizeTree(astree* root);
void checkAssignType(astree* root);
bool checkSymExistence(astree* root);
void checkTypeid(astree* root);
void checkReturnType(astree* root);
void checkExpBoolType(astree* root);
void checkBoolType(astree* root);
void checkOpType(astree* root);
void checkNewType(astree* root);
void checkNewString(astree* root);
void checkNewArray(astree* root);
void checkIndex(astree* root);
void checkFieldType(astree* root);
void checkIdentType(astree* root);
void checkArrayType(astree* root);
void checkConType(astree* root);
void checkCall(astree* root);
void backtrack();
symbol* createSymbol(astree* root);
void createDecSym(astree* root);
void createFuncSym(astree* root);
void createProSym(astree* root);
void createStructSym(astree* root);
void createBlkSym(astree* root);
void assignType(symbol* sym, astree* root);
void insertAndPrint(symbol_entry& newEntry);
void printSymbol(symbol_entry& entry);
void enterTable();
void exitTable();
void enterBlock();
void exitBlock();
#endif
