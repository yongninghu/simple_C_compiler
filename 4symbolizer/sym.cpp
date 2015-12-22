#include <assert.h>
#include <unordered_map>
#include <bitset>
#include <vector>
#include <string>

#include "sym.h"
#include "lyutils.h"

using namespace std;
size_t blkct = 0;
vector<size_t> blkstk;
vector<symbol_table*> symstk;
bool firstPrint = true;

void symbolizeTree(astree* root) {
   int tokename = root->symbol;
   switch(tokename) {
      case TOK_ROOT:
         enterTable();
         enterBlock();
         break;
      case TOK_FUNCTION:
         backtrack();
         createFuncSym(root);
         break;
      case TOK_PROTOTYPE:
         backtrack();
         createProSym(root);
         break;
      case TOK_STRUCT:
         backtrack();
         createStructSym(root);
         break;
      case TOK_BLOCK:
         createBlkSym(root);
         break;
      case TOK_VARDECL:
         if(root->parent->symbol == TOK_ROOT) {
            backtrack();
         }
         createDecSym(root);
         break;
      default:
         break;
   }
   vector<astree*>& children = root->children; 
   for(vector<astree*>::iterator it = children.begin();
       it != children.end(); ++it) {
      symbolizeTree(*it);
   }
   switch(tokename) {
      case '=':
      case TOK_VARDECL:
         checkAssignType(root);
         break;
      case TOK_RETURN:
      case TOK_RETURNVOID:
         checkReturnType(root);
         break;        
      case TOK_WHILE:
      case TOK_IF:
      case TOK_IFELSE:
         checkExpBoolType(root);
         break;
      case TOK_EQ:
      case TOK_NE:
      case TOK_GT:
      case TOK_LT:
      case TOK_GE:
      case TOK_LE:
         checkBoolType(root);
         break;
      case '+':
      case '-':
      case '*':
      case '/':
      case '%':
      case '!':
      case TOK_ORD:
      case TOK_CHR:
      case TOK_NEG:
      case TOK_POS:
         checkOpType(root);
         break;
      case TOK_NEW:
         checkNewType(root);
         break;
      case TOK_NEWARRAY:
      case TOK_ARRAY:
         checkNewArray(root);
         break;
      case TOK_NEWSTRING:
         checkNewString(root);
         break;
      case '.':
         checkFieldType(root);
         break;
      case TOK_IDENT:
         checkIdentType(root);
         break;
      case TOK_INDEX:
         checkIndex(root);
         break;
      case TOK_TYPEID:
         checkTypeid(root);
         break;
      case TOK_INTCON:
      case TOK_STRINGCON:
      case TOK_CHARCON:
      case TOK_FALSE:
      case TOK_TRUE:
      case TOK_NULL:
         checkConType(root);
         break;
      case TOK_CALL:
         checkCall(root);
         break;
      default:
         break;
   }
}

bool typeCompatible(symbol* left, symbol* right) {
   if(!left->attributes.test(ATTR_array) &&
      right->attributes.test(ATTR_array)) {
      return false;
   }
   if(left->attributes.test(ATTR_int) &&
      !left->attributes.test(ATTR_const) &&
      right->attributes.test(ATTR_int)) {
      return true;
   }
   if(left->attributes.test(ATTR_bool) &&
      !left->attributes.test(ATTR_const)&&
      right->attributes.test(ATTR_bool)) {
      return true;
   }
   if(left->attributes.test(ATTR_string) &&
      !left->attributes.test(ATTR_const) &&
      (right->attributes.test(ATTR_string) ||
       right->attributes.test(ATTR_null))) {
      return true;
   }
   if(left->attributes.test(ATTR_char) &&
      !left->attributes.test(ATTR_const) &&
      right->attributes.test(ATTR_char)) {
      return true;
   }
   if(left->attributes.test(ATTR_struct) &&
      right->attributes.test(ATTR_struct)) {
      if(*left->typeId == *right->typeId) {
         return true;
      }
   }
   if(left->attributes.test(ATTR_struct) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   if(left->attributes.test(ATTR_array) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   return false;
}

bool compCompatible(symbol* left, symbol* right) {
   if(left->attributes.test(ATTR_array) ||
      right->attributes.test(ATTR_array)) {
      return false;
   }
   if(left->attributes.test(ATTR_int) &&
      right->attributes.test(ATTR_int)) {
      return true;
   }
   if(left->attributes.test(ATTR_bool) &&
      right->attributes.test(ATTR_bool)) {
      return true;
   }
   if(left->attributes.test(ATTR_char) &&
      right->attributes.test(ATTR_char)) {
      return true;
   }
   return false;
}

bool anyCompatible(symbol* left, symbol* right) {
   if(left->attributes.test(ATTR_string) &&
      right->attributes.test(ATTR_string)) {
      return true;
   }
   if(left->attributes.test(ATTR_array) &&
      right->attributes.test(ATTR_array)) {
      return true;
   }
   if(left->attributes.test(ATTR_array) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   if(left->attributes.test(ATTR_null) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   if(left->attributes.test(ATTR_string) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   if(left->attributes.test(ATTR_struct) &&
      right->attributes.test(ATTR_null)) {
      return true;
   }
   if(left->attributes.test(ATTR_struct) &&
      right->attributes.test(ATTR_struct)) {
      if(*left->typeId == *right->typeId) {
         return true;
      }
   }
   return false;
}

void checkAssignType(astree* root) {
   if(root->symbol == TOK_VARDECL) {
      astree* leftType = root->children.front();
      astree* typeName = leftType->children.back();
      astree* rightType = root->children.back();
      if(typeName->syminfo == 0 || rightType->syminfo==0) {
         cerr<< "incompatible type assignment: " << *typeName->lexinfo
         << endl;
         return;
      }
      bool compat = typeCompatible
                 (typeName->syminfo, rightType->syminfo);
      if(compat == false) {
         cerr<< "incompatible type declaration: " << *typeName->lexinfo
         << endl;
      }
   } 
   if(root->symbol == '=') {
      astree* leftType = root->children.front();
      astree* rightType = root->children.back();
      if(leftType->syminfo == 0 || rightType->syminfo==0) {
         cerr<< "incompatible type assignment: " << *leftType->lexinfo
         << endl;
         return;
      }
      bool compat = typeCompatible
                      (leftType->syminfo, rightType->syminfo);
      if(compat == false) {
         cerr<< "incompatible type assignment: " << *leftType->lexinfo
         << endl;
      }
   }
}

bool checkSymExistence(astree* root) {
   bool found = false;
   for(vector<symbol_table*>::iterator it = symstk.begin();
          it != symstk.end(); ++it) {
         symbol_table* table = *it;
         symbol_table::iterator got = table->find(root->lexinfo);
         if(got != table->end()) {
            symbol* sym = (*got).second;
            if(sym->fields != 0) {
               return false;
            } else {
               return true;
            }
         }
   }
   return found;
}
void checkTypeid(astree* root) {
   bool notFound = true;
   for(vector<symbol_table*>::iterator it = symstk.begin();
          it != symstk.end(); ++it) {
         symbol_table* table = *it;
         symbol_table::iterator got = table->find(root->lexinfo);
         if(got != table->end()) {
            notFound = false;
         }
   }
   if(notFound) { 
      cerr << "unidentifiable type : "
      << *root->lexinfo << endl;
   }
}

void checkReturnType(astree* root) {
   astree* returnType = root;
   while(returnType->symbol != TOK_FUNCTION) {
      returnType = returnType->parent;
      if(returnType == 0) {
         cerr << "return is not called in function" << endl;
         return;
      }
   }
   returnType = returnType->children.front()->children.back();    
   if(root->symbol == TOK_RETURNVOID) {    
      if(!returnType->syminfo->attributes.test(ATTR_void)) {
         cerr << "incompatible void return" << endl;
      }
   } else {
      astree* toReturn = root->children.back();
      bool compat = typeCompatible
                  (returnType->syminfo, toReturn->syminfo);
      if(!compat) {
         cerr<< "incompatible return type: " << *toReturn->lexinfo
         << endl;
      } 
   }
}

void checkExpBoolType(astree* root) {
   astree* comp = root->children.front();
   if(comp->syminfo == 0) {
      return;
   }
   symbol* sym = comp->syminfo;
   if(!sym->attributes.test(ATTR_bool)) {
      cerr << "not boolean type: " << *comp->lexinfo << endl;
   }
}

void checkBoolType(astree* root) {
   symbol* newSym = createSymbol(root);
   if(root->symbol == TOK_GT ||
      root->symbol == TOK_GE ||
      root->symbol == TOK_LT ||
      root->symbol == TOK_LE) {
      astree* left = root->children.front();
      symbol* leftSym = left->syminfo;
      astree* right = root->children.back();
      symbol* rightSym = right->syminfo;
      if(leftSym == 0 or rightSym == 0) {
         return;
      }
      if(compCompatible(leftSym, rightSym)) {
         newSym->attributes.set(ATTR_bool);
         newSym->attributes.set(ATTR_vreg);
         root->syminfo = newSym;
      } else {
         cerr << "incompatible comparison operation: " 
         << *root->lexinfo <<endl;
      }
   } else {
      astree* left = root->children.front();
      symbol* leftSym = left->syminfo;
      astree* right = root->children.back();
      symbol* rightSym = right->syminfo;
      if(leftSym == 0 or rightSym == 0) {
         return;
      }
      if(anyCompatible(leftSym, rightSym)||
         compCompatible(leftSym, rightSym)) {
         newSym->attributes.set(ATTR_bool);
         newSym->attributes.set(ATTR_vreg);
         root->syminfo = newSym;
      } else {
         cerr << "incompatible comparison operation: " 
         << *root->lexinfo <<endl;
      } 
   }
}
bool intCompatible(symbol* left, symbol* right) {
   if(left->attributes.test(ATTR_array) ||
      right->attributes.test(ATTR_array)) {
      return false;
   }
   if(left->attributes.test(ATTR_int) &&
      right->attributes.test(ATTR_int)) {
      return true;
   }
   return false;
}
void checkOpType(astree* root) {
   symbol* newSym = createSymbol(root); 
   if(root->children.size()>1) {
      astree* left = root->children.front();
      astree* right = root->children.back();
      if(left->syminfo == 0 || right->syminfo==0) {
         return;
      }
      if(intCompatible(left->syminfo, right->syminfo)) {
         newSym->attributes.set(ATTR_int);
         newSym->attributes.set(ATTR_vreg);
         root->syminfo = newSym;
      } else {
         cerr<< "incompatible type operation: " << *root->lexinfo
         <<endl;
      }
   } else {
      if(root->symbol == TOK_POS ||
         root->symbol == TOK_NEG) {
         astree* child = root->children.front();
         symbol* sym = child->syminfo;
         if(sym->attributes.test(ATTR_int)) {
            newSym->attributes.set(ATTR_int);
            newSym->attributes.set(ATTR_vreg);
            root->syminfo = newSym;
         }else {
            cerr<< "incompatible type operation: " << *root->lexinfo
            <<endl;
         }
      }
      if(root->symbol == TOK_ORD) {
         astree* child = root->children.front();
         symbol* sym = child->syminfo;
         if(sym->attributes.test(ATTR_char)) {
            newSym->attributes.set(ATTR_int);
            newSym->attributes.set(ATTR_vreg);
            root->syminfo = newSym;
         }else {
            cerr<< "incompatible type operation: " << *root->lexinfo
            <<endl;
         }
      }
      if(root->symbol == '!') {
         astree* child = root->children.front();
         symbol* sym = child->syminfo;
         if(sym->attributes.test(ATTR_bool)) {
            newSym->attributes.set(ATTR_bool);
            newSym->attributes.set(ATTR_vreg);
            root->syminfo = newSym;
         }else {
            cerr<< "incompatible type operation: " << *root->lexinfo
            <<endl;
         }
      }
      if(root->symbol == TOK_CHR) {
         astree* child = root->children.front();
         symbol* sym = child->syminfo;
         if(sym->attributes.test(ATTR_int)) {
            newSym->attributes.set(ATTR_char);
            newSym->attributes.set(ATTR_vreg);
            root->syminfo = newSym;
         }else {
            cerr<< "incompatible type operation: " << *root->lexinfo
            <<endl;
         }
      }
   }
}

void checkNewType(astree* root) {
   astree* child = root->children.front();
   bool notFound = true;
   for(vector<symbol_table*>::iterator it = symstk.begin();
       it != symstk.end(); ++it) {
      symbol_table* table = *it;
      symbol_table::iterator got = table->find(child->lexinfo);
      if(got != table->end()) {
         symbol* sym = (*got).second;
         symbol_table* fieldTable = sym->fields;
         if(fieldTable == 0) {
            break;
         }
         symbol* newSymbol = createSymbol(root);
         newSymbol->attributes = (*got).second->attributes;
         newSymbol->typeId = (*got).second->typeId;
         newSymbol->fieldId = (*got).second->fieldId;
         newSymbol->attributes.set(ATTR_vreg);
         root->syminfo = newSymbol;
         notFound = false;
      }
   }
   
   if(notFound) { 
      cerr << "unidentifiable type : "<< *child->lexinfo << endl;
   } 
}

void checkNewString(astree* root) {
   if(!(root->children.front()->syminfo->attributes.test(ATTR_int))) {
      cerr << "new string can only take in int" << endl;
      return;
   }
   symbol* newSymbol = createSymbol(root);
   newSymbol->attributes.set(ATTR_string);
   newSymbol->attributes.set(ATTR_vreg);
   root->syminfo = newSymbol;
}

void checkNewArray(astree* root) { 
   astree* child = root->children.front();
   bool notFound = false;
   if(child->symbol == TOK_TYPEID) {
      for(vector<symbol_table*>::iterator it = symstk.begin();
          it != symstk.end(); ++it) {
         symbol_table* table = *it;
         symbol_table::iterator got = table->find(child->lexinfo);
         if(got != table->end()) {
            symbol* sym = (*got).second;
            symbol_table* fieldTable = sym->fields;
            if(fieldTable == 0) {
               notFound = true;
               break;
            }
            symbol* newSymbol = createSymbol(root);
            newSymbol->attributes = (*got).second->attributes;
            newSymbol->typeId = (*got).second->typeId;
            newSymbol->fieldId = (*got).second->fieldId;
            newSymbol->attributes.set(ATTR_array);
            newSymbol->attributes.set(ATTR_vreg);
            root->syminfo = newSymbol;
         }
      }
   } else {
      symbol* newSymbol = createSymbol(root);
      switch(child->symbol) {
         case TOK_INT:
            newSymbol->attributes.set(ATTR_int);
            break;
         case TOK_STRING:
            newSymbol->attributes.set(ATTR_string);
            break;
         case TOK_CHAR:
            newSymbol->attributes.set(ATTR_char);
            break;
         case TOK_BOOL:
            newSymbol->attributes.set(ATTR_bool);
            break;
         case TOK_NULL:
            newSymbol->attributes.set(ATTR_null);
            break;
         default:
            notFound = true;
            break;
      }
      if(root->symbol == TOK_NEWARRAY) {
         if(!(root->children.back()->
             syminfo->attributes.test(ATTR_int))) {
            notFound = true;
         }
      }
      if(!notFound) {
         newSymbol->attributes.set(ATTR_array);
         newSymbol->attributes.set(ATTR_vreg);
         root->syminfo = newSymbol;
      }
   }
   
   if(notFound) { 
      cerr << "unidentifiable type or invalid allocation : "
      << *child->lexinfo << endl;
   }
}

void checkIndex(astree* root) {
   astree* ident = root->children.front();
   symbol* sym = ident->syminfo;
   if(sym == 0) {
      return;
   }
   symbol* newSymbol = createSymbol(root);
   if(sym->attributes.test(ATTR_array)) {
      if(sym->attributes.test(ATTR_int)) {
         newSymbol->attributes.set(ATTR_int);
      }
      if(sym->attributes.test(ATTR_string)) {
         newSymbol->attributes.set(ATTR_string);
      }
      if(sym->attributes.test(ATTR_char)) {
         newSymbol->attributes.set(ATTR_char);
      }
      if(sym->attributes.test(ATTR_bool)) {
         newSymbol->attributes.set(ATTR_bool);
      }
      if(sym->attributes.test(ATTR_null)) {
         newSymbol->attributes.set(ATTR_null);
      }
      if(sym->attributes.test(ATTR_struct)) {
         newSymbol->attributes.set(ATTR_struct);
         newSymbol->typeId = sym->typeId;
         newSymbol->fieldId = sym->fieldId;
      }
      newSymbol->attributes.set(ATTR_vaddr);
      newSymbol->attributes.set(ATTR_lval);
      root->syminfo = newSymbol;
   } else {
      if(sym->attributes.test(ATTR_string)) {
         newSymbol->attributes.set(ATTR_char);
         newSymbol->attributes.set(ATTR_vaddr);
         newSymbol->attributes.set(ATTR_lval);
         root->syminfo = newSymbol;
      } else {
         cerr << "invalid index reference" << endl;
      }
   }
}

void checkFieldType(astree* root) {
   astree* ident = root->children.front();
   symbol* structSym = ident->syminfo;
   astree* field = root->children.back();
   bool notFound = false;
   
   for(vector<symbol_table*>::iterator it = symstk.begin();
       it != symstk.end(); ++it) {
      symbol_table* table = *it;
      symbol_table::iterator got = table->find(structSym->typeId);
      if(got != table->end()) {
         symbol* sym = (*got).second;
         symbol_table* fieldTable = sym->fields;
         if(fieldTable == 0) {
            notFound = true;
            break;
         }
         symbol_table::iterator gotF =fieldTable->find(field->lexinfo);
         if(gotF == table->end()) {
            notFound = true;
            break;
         } else {
            symbol* newSymbol = createSymbol(root);
            newSymbol->attributes = (*gotF).second->attributes;
            newSymbol->typeId = (*gotF).second->typeId;
            newSymbol->fieldId = (*gotF).second->fieldId;
            newSymbol->attributes.set(ATTR_vaddr);
            newSymbol->attributes.set(ATTR_lval);
            root->syminfo = newSymbol;   
            break;
         }
      }
   } 
   
   if(notFound) { 
      cerr << "struct or field doesn't exist: "<< *ident->lexinfo
      << " or " << *field->lexinfo << endl;
   } 
}

void checkCall(astree* root) {
   bool notFound = false;
   bool paramFail = false;
   for(vector<symbol_table*>::iterator it = symstk.begin();
       it != symstk.end(); ++it) {
      symbol_table* table = *it;
      symbol_table::iterator got = table->find(root->
                                     children.front()->lexinfo);                             
      if(got != table->end()) {
         symbol* sym = (*got).second;
         vector<symbol*>* parameters = sym->parameters;

         if(!sym->attributes.test(ATTR_function)) {
            notFound = true;
            break;
         }
         if(parameters->size() != root->children.size()-1) {
            paramFail = true;
            break;
         }
         notFound = false;
         size_t pct = 1;
         for(vector<symbol*>::iterator it = parameters->begin(); 
             it != parameters->end(); ++it) {
            symbol* paramSym = *it;
            symbol* compSym = root->children.at(pct)->syminfo;
            if(!compCompatible(paramSym, compSym)&&
               !anyCompatible(paramSym, compSym)) {
               paramFail = true;
               break;
            }              
            ++pct;   
         }
         break;
      } else {
         notFound = true;
      }         
   }
   if(!notFound and !paramFail) {
      symbol* newSymbol = createSymbol(root);
      astree* funcIdent = root->children.front();
      newSymbol->attributes = funcIdent->syminfo->attributes;
      newSymbol->typeId = funcIdent->syminfo->typeId;
      newSymbol->fieldId = funcIdent->syminfo->fieldId;
      newSymbol->attributes.set(ATTR_vreg);
      newSymbol->attributes.set(ATTR_function, 0);
      root->syminfo = newSymbol;
   }
   if(paramFail) {
      cerr << "Wrong params for function: " 
        << *root->children.front()->lexinfo << endl;
   }
   if(notFound) {
      cerr << "function undefined: " << *root->
      children.front()->lexinfo << endl;
   }
}

void checkIdentType(astree* root) {
   //check call compatibility
   bool notFound = true;
   for(vector<symbol_table*>::iterator it = symstk.begin();
       it != symstk.end(); ++it) {
      symbol_table* table = *it;
      symbol_table::iterator got = table->find(root->lexinfo);
      if(got != table->end()) {
         symbol* sym = (*got).second;
         symbol* newSym = createSymbol(root);
         newSym->filenr = sym->filenr;
         newSym->linenr = sym->linenr;
         newSym->offset = sym->offset;
         newSym->attributes = sym->attributes;
         newSym->block_nr = blkstk.back();
         newSym->typeId = sym->typeId;
         newSym->fieldId = sym->fieldId;
         root->syminfo = newSym;
         notFound = false;
         break;
      }
   }
   if(notFound) {
      cerr << "identifier undefined: " << *root->lexinfo << endl;
   }
}

void checkConType(astree* root) {
   symbol* newSym = createSymbol(root);
   switch(root->symbol) {
      case TOK_INTCON:
         newSym->attributes.set(ATTR_const);
         newSym->attributes.set(ATTR_int);
         break;
      case TOK_CHARCON:
         newSym->attributes.set(ATTR_const);
         newSym->attributes.set(ATTR_char);
         break;
      case TOK_STRINGCON:
         newSym->attributes.set(ATTR_const);
         newSym->attributes.set(ATTR_string);
         break;
      case TOK_NULL:
         newSym->attributes.set(ATTR_const);
         newSym->attributes.set(ATTR_null);
         break;
      case TOK_FALSE:
      case TOK_TRUE:
         newSym->attributes.set(ATTR_const);
         newSym->attributes.set(ATTR_bool);
         break;
      default:
         break;
   }
   root->syminfo = newSym;
}
   
void backtrack() {
   if(!firstPrint) symfile << endl;
   while(symstk.size()>1) {
      exitTable();
   }
   while(blkstk.size()>1) {
     exitBlock();
   }
}

symbol* createSymbol(astree* root) {
   symbol* newSymbol = new symbol();
   newSymbol->filenr = root->filenr;
   newSymbol->linenr = root->linenr;
   newSymbol->offset = root->offset;
   newSymbol->block_nr = blkstk.back();
   newSymbol->typeId = 0;
   newSymbol->fieldId = 0;
   newSymbol->fields = 0;
   newSymbol->parameters = 0;
   return newSymbol; 
}

void createDecSym(astree* root) {
   astree* var = root->children.front()->children.back();
   if(checkSymExistence(var)) {
      cerr << "type redeclaration error: " << 
      *var->lexinfo << endl;
   }
   symbol* varSymbol = createSymbol(var);
   var->syminfo = varSymbol;
   assignType(varSymbol, root->children.front());
   if(root->children.front()->children.size()>1) {
      assignType(varSymbol, root->children.front()->children.front());
   }
   symbol_entry fieldEntry = make_pair(var->lexinfo, varSymbol);
   insertAndPrint(fieldEntry);
}

void createFuncSym(astree* root) {
   astree* type_id = root->children.front();
   astree* decl_id = type_id->children.back();
   symbol* newSymbol = createSymbol(decl_id);
   decl_id->syminfo = newSymbol;
   newSymbol->attributes.set(ATTR_function);
   assignType(newSymbol, type_id);
   if(type_id->symbol==TOK_ARRAY) {
      assignType(newSymbol, type_id->children.front());
   }
   newSymbol->attributes.set(ATTR_variable, 0);
   newSymbol->attributes.set(ATTR_lval, 0);
   symbol_entry newEntry = make_pair(decl_id->lexinfo, newSymbol);
   insertAndPrint(newEntry);
   astree* paramlist = root->children.at(1);
   vector<astree*>& params = paramlist->children;
   enterBlock();
   enterTable();
   vector<symbol*>* paramStack = new vector<symbol*>();
   for(vector<astree*>::iterator it = params.begin();
       it != params.end(); ++ it) {
      astree* child = *it;
      astree* typeName = child->children.back();
      symbol* typeSymbol = createSymbol(typeName);
      typeName->syminfo = typeSymbol;
      typeSymbol->attributes.set(ATTR_param);
      assignType(typeSymbol, child);
      if(child->symbol==TOK_ARRAY) {
         assignType(typeSymbol, child->children.front());
      }
      symbol_entry typeEntry = make_pair(typeName->lexinfo,typeSymbol);
      insertAndPrint(typeEntry);
      paramStack->push_back(typeSymbol);    
   } 
   newSymbol->parameters = paramStack;
   if(params.size() > 0) {
      symfile << endl;
   }
}

void createProSym(astree* root) {
   astree* type_id = root->children.front();
   astree* decl_id = type_id->children.back();
   if(checkSymExistence(decl_id)) {
      cerr << "function name is already taken: " << 
      *decl_id->lexinfo << endl;
      return;
   }
   symbol* newSymbol = createSymbol(decl_id);
   decl_id->syminfo = newSymbol;
   newSymbol->attributes.set(ATTR_function);
   assignType(newSymbol, type_id);
   if(type_id->symbol==TOK_ARRAY) {
      assignType(newSymbol, type_id->children.front());
   }
   newSymbol->attributes.set(ATTR_variable, 0);
   newSymbol->attributes.set(ATTR_lval, 0);
   symbol_entry newEntry = make_pair(decl_id->lexinfo, newSymbol);
   insertAndPrint(newEntry);
   astree* paramlist = root->children.at(1);
   vector<astree*>& params = paramlist->children;
   vector<symbol*>* paramStack = new vector<symbol*>();
   if(params.size() > 0) {
      enterBlock();
      enterTable();
   }
   for(vector<astree*>::iterator it = params.begin();
       it != params.end(); ++ it) {
      astree* child = *it;
      astree* typeName = child->children.back();
      symbol* typeSymbol = createSymbol(typeName);
      typeName->syminfo = typeSymbol;
      typeSymbol->attributes.set(ATTR_param);
      assignType(typeSymbol, child);
      if(child->symbol==TOK_ARRAY) {
         assignType(typeSymbol, child->children.front());
      }
      symbol_entry typeEntry = make_pair(typeName->lexinfo,typeSymbol);
      insertAndPrint(typeEntry);
      paramStack->push_back(typeSymbol);
   }
   newSymbol->parameters = paramStack;
   if(params.size() > 0) {
      symfile << endl;
      exitBlock();
      exitTable();
   }
}

void createStructSym(astree* root) {
   astree* type_id = root->children.front();
   symbol* newSymbol = createSymbol(type_id);
   newSymbol->attributes.set(ATTR_struct);
   type_id->syminfo = newSymbol;
   assignType(newSymbol, type_id);
   newSymbol->attributes.set(ATTR_variable, 0);
   newSymbol->attributes.set(ATTR_lval, 0);
   symbol_entry newEntry = make_pair(type_id->lexinfo, newSymbol);
   insertAndPrint(newEntry);
   vector<astree*>& children = root->children;
   newSymbol->fields = new symbol_table();
   if(children.size() > 1) {
      enterTable();
   }
   for(vector<astree*>::iterator it = ++children.begin();
       it != children.end(); ++it) {
      astree* child = *it;
      astree* field = child->children.back();
      symbol* fieldSymbol = createSymbol(field);
      field->syminfo = fieldSymbol;
      fieldSymbol->attributes.set(ATTR_field);
      assignType(fieldSymbol, child);
      if(child->symbol==TOK_ARRAY) {
         assignType(fieldSymbol, child->children.front());
      }
      fieldSymbol->fieldId = type_id->lexinfo;
      fieldSymbol->attributes.set(ATTR_lval, 0);
      fieldSymbol->attributes.set(ATTR_variable, 0);
      symbol_entry fieldEntry = make_pair(field->lexinfo, fieldSymbol);
      insertAndPrint(fieldEntry);
   }
   if(children.size() > 1) {
      newSymbol->fields = symstk.back();
      exitTable();
   }
}

void createBlkSym(astree* root) {
   int scenario = 1;
   astree* parent = root->parent;
   if(parent->symbol == TOK_FUNCTION) {
      vector<astree*>& children = parent->children;
      for(vector<astree*>::iterator it = children.begin();
          it != children.end(); ++it) {
         if((*it)->symbol == TOK_BLOCK) {
            if(root == *it) scenario = 2;
            break;
         }
      }
   }
   if(parent->symbol != TOK_ROOT) {
     if(parent->parent->symbol == TOK_ROOT) {
        if(parent->symbol != TOK_FUNCTION &&
           parent->symbol != TOK_PROTOTYPE &&
           parent->symbol != TOK_BLOCK) {
           scenario = 3;
        }         
     } 
   }
   if(parent->symbol == TOK_IFELSE) {
      if(root == parent->children.at(2)) {
         scenario = 3;
      }
   }
   switch(scenario) {
      case 1:
         enterBlock();
         enterTable();
         break;
      case 2:
         break;
      case 3:
         enterBlock();
         break;
   }
}

void assignType(symbol* sym, astree* root) {
   switch(root->symbol) {
      case TOK_TYPEID:
         sym->attributes.set(ATTR_struct);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         sym->typeId = root->lexinfo;
         break;
      case TOK_INT:
         sym->attributes.set(ATTR_int);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      case TOK_STRING:
         sym->attributes.set(ATTR_string);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      case TOK_CHAR:
         sym->attributes.set(ATTR_char);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      case TOK_VOID:
         sym->attributes.set(ATTR_void);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      case TOK_ARRAY:
         sym->attributes.set(ATTR_array);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      case TOK_BOOL:
         sym->attributes.set(ATTR_bool);
         sym->attributes.set(ATTR_variable);
         sym->attributes.set(ATTR_lval);
         break;
      default:
         break; 
   }
}

void insertAndPrint(symbol_entry& newEntry) {
   symbol_table* currentTable = symstk.back();
   currentTable->insert(newEntry);
   printSymbol(newEntry);
}

void printSymbol(symbol_entry& entry) {
   firstPrint = false;
   for(size_t i = 1; i < symstk.size(); ++i) {
      symfile << "   ";
   }
   symbol* sym = entry.second;
   symfile << *(entry.first);
    
   symfile << " (" << sym->filenr;
   symfile << "." << sym->linenr;
   symfile << "." << sym->offset << ")";
   if(sym->attributes.test(ATTR_field)) {
      symfile << " field {" << *sym->fieldId << "}";
   } else {
      symfile << " {" << sym->block_nr << "}";
   }
   if(sym->attributes.test(ATTR_struct)) {
      symfile << " struct \"" << *sym->typeId << "\"";
   }

   if(sym->attributes.test(ATTR_void)) {
      symfile << " void";
   }   

   if(sym->attributes.test(ATTR_bool)) {
      symfile << " bool";
   }

   if(sym->attributes.test(ATTR_array)) {
      symfile << " array";
   }
   if(sym->attributes.test(ATTR_int)) {
      symfile << " int"; 
   }
   if(sym->attributes.test(ATTR_string)) {
      symfile << " string";
   }
   if(sym->attributes.test(ATTR_variable)) {
      symfile << " variable";
   }
   if(sym->attributes.test(ATTR_lval)) {
      symfile << " lval";
   }
   if(sym->attributes.test(ATTR_param)) {
      symfile << " param";
   }
   if(sym->attributes.test(ATTR_function)) {
      symfile << " function";
   }
   symfile << endl;
}

void enterTable() {
   symbol_table* newTable = new symbol_table();
   symstk.push_back(newTable);
}

void exitTable() {
   symstk.pop_back();
}

void enterBlock() {
   blkstk.push_back(blkct);
   ++blkct;
}

void exitBlock() {
   blkstk.pop_back();
}
