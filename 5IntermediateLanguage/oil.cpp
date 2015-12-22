#include <assert.h>
#include <unordered_map>
#include <bitset>
#include <vector>
#include <string>

#include "oil.h"
#include "lyutils.h"

using namespace std;
size_t bregct = 1;
size_t iregct = 1;
size_t pregct = 1;
size_t sregct = 1;
size_t aregct = 1;
vector<string> regstk;

void emitTree(astree* root) {
   vector<astree*>& children = root->children; 
   for(vector<astree*>::iterator it = children.begin();
       it != children.end(); ++it) {
      switch((*it)->symbol) {
         case TOK_STRUCT:
            resetRegisters();
            emitStruct(*it);
            break;
         case TOK_FUNCTION:
            resetRegisters();
            emitFunction(*it);
            break;
         default:
            resetRegisters();
            break;
      }
   }
   oilfile << "void __ocmain (void)" << endl;
   oilfile << "{" << endl;
   emitBlock(root);
   oilfile << "}" << endl;
}

void resetRegisters() {
   bregct = 1;
   iregct = 1;
   pregct = 1;
   sregct = 1;
   aregct = 1;
}

string constructDec(symbol* root) {
   string result = "";
   if(root->attributes.test(ATTR_struct)) {
      result = result + "struct s_" + *root->typeId + "*";
   }
   if(root->attributes.test(ATTR_int)) {
      result = result + "int";
   }
   if(root->attributes.test(ATTR_void)) {
      result = result + "void";
   }
   if(root->attributes.test(ATTR_bool) or
      root->attributes.test(ATTR_char)) {
      result = result + "char";
   }
   if(root->attributes.test(ATTR_string)) {
      result = result + "char*";
   }
   if(root->attributes.test(ATTR_array)) {
      result = result + "*";
   }
   return result;
}

void emitStruct(astree* root) {
   astree* structId = root->children.front();
   oilfile << "struct s_" << *structId->lexinfo << " {" << endl;
   vector<astree*>& children = root->children; 
   for(vector<astree*>::iterator it = ++children.begin();
      it != children.end(); ++it) {
      astree* fieldId = (*it)->children.back();
      string printResult = constructDec(fieldId->syminfo);
      oilfile << "        " << printResult << " " <<
      "f_" << *structId->lexinfo << "_" << *fieldId->lexinfo << endl;
   }
   oilfile << "};" << endl;
}

void emitFunction(astree* root) {
   astree* funcId = root->children.front()->children.back();
   symbol* funcSym = funcId->syminfo;
   astree* paramlistNode = root->children.at(1);
   string printResult = constructDec(funcSym);
   oilfile << printResult << " __" << *funcId->lexinfo << "(" << endl;
   vector<symbol *>* params = funcSym->parameters;
   if(params->size() == 0) {
      oilfile << "        " << "void)" << endl;
   }
   size_t count = 0;
   for(vector<symbol*>::iterator it = params->begin(); 
             it != params->end(); ++it) {
      symbol* paramSym = *it;
      astree* paramNode = paramlistNode->children.at(count)
                          ->children.back();
      printResult = constructDec(paramSym);
      oilfile << "        " << printResult <<
      " _" << paramSym->block_nr << "_" << *paramNode->lexinfo;
      if((it+1) == params->end()) {
         oilfile << ")";
      }
      oilfile << endl;
      ++count;
   }
   oilfile <<  "{" << endl;
   emitBlock(root->children.back());
   oilfile << "}" << endl;
}

void emitBlock(astree* root) {
   vector<astree*>& children = root->children; 
   for(vector<astree*>::iterator it = children.begin();
       it != children.end(); ++it) {
      switch((*it)->symbol) {
         case TOK_VARDECL:
         case '=':
            emitVardecl(*it);
            break;
         case TOK_CALL:
            emitCall(*it);
            break;
         case TOK_IF:
            emitIf(*it);
            break;
         case TOK_WHILE:
            emitWhile(*it);
            break;
         case TOK_IFELSE:
            emitIfelse(*it);
            break;
         default:
            break;
      }
   }
}

void emitIf(astree* root) {
   if(root->children.front()->syminfo != 0) {
      astree* op = root->children.front();
      if(op->syminfo->attributes.test(ATTR_bool)) {
         emitReg(op);
      }
   } else {
      return;
   }
   oilfile << "        if(!" << *root->children.front()->lexinfo
   <<")" << " goto " << "fi_" << root->filenr << "_"<<root->linenr
   <<"_"<<root->offset << ";" << endl;
   if(root->children.back()->symbol==TOK_BLOCK) {
      emitBlock(root->children.back());
   }
   oilfile << "fi__" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ":;" << endl;
}

void emitWhile(astree* root) {
   oilfile << "while_" << root->filenr << "_"<<root->linenr
   <<"_"<<root->offset << ":;" << endl;
   if(root->children.front()->syminfo != 0) {
      astree* op = root->children.front();
      if(op->syminfo->attributes.test(ATTR_bool)) {
         emitReg(op);
      }
   }
   if(root->children.back()->symbol==TOK_BLOCK) {
      emitBlock(root->children.back());
   }
   oilfile << "        goto " <<  "while_" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ";" << endl;
   oilfile << "break_" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ":;" << endl;
}

void emitIfelse(astree* root) {
   if(root->children.front()->syminfo != 0) {
      astree* op = root->children.front();
      if(op->syminfo->attributes.test(ATTR_bool)) {
         emitReg(op);
      }
   } else {
      return;
   }
   oilfile << "        if(!" << *root->children.front()->lexinfo
   <<")" << " goto else" << "fi_" << root->filenr << "_"<<root->linenr
   <<"_"<<root->offset << ";" << endl;
   if(root->children.at(1)->symbol==TOK_BLOCK) {
      emitBlock(root->children.at(1));
   }
   oilfile << "        goto fi__" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ";" << endl;
   oilfile << "else__" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ":;" << endl;
   if(root->children.back()->symbol==TOK_BLOCK) {
      emitBlock(root->children.back());
   }
   oilfile << "fi__" << root->filenr 
   << "_"<<root->linenr<<"_"<<root->offset << ":;" << endl;
}

string constructReg(astree* root, symbol* sym) {
   string result = "";
   
   if(sym->attributes.test(ATTR_vaddr)) {
      result = result + "a" + to_string(aregct++);
   }  else if(root->symbol == TOK_NEW ||
      root->symbol == TOK_NEWARRAY ||
      root->symbol == TOK_NEWSTRING ||
      sym->attributes.test(ATTR_struct)) {
      result = result + "p" + to_string(pregct++);
   }  else if(sym->attributes.test(ATTR_string)) {
      result = result + "s" + to_string(sregct++);
   } else if(sym->attributes.test(ATTR_bool)) {
      result = result + "b" + to_string(bregct++);
   } else if(sym->attributes.test(ATTR_int)) {
      result = result + "i" + to_string(iregct++);
   }
   regstk.push_back(result);
   return result;
}

string constructNew(astree* root, symbol* sym) {
   sym = sym;
   string result = "xcalloc (";
   if(root->symbol == TOK_NEW) {
      result = result + "1, sizeof (";
   } else {
      astree* exp = root->children.back();
      if(exp->syminfo->attributes.test(ATTR_vreg) or
        exp->syminfo->attributes.test(ATTR_vaddr)) {
         emitReg(exp);
         result = result + regstk.back() + ", sizeof (";
         regstk.pop_back();
      } else {
         if(exp->symbol == TOK_IDENT) {
            result = result + "_";
            if(exp->syminfo->block_nr>0){
               result = result + to_string(exp->syminfo->block_nr);
            }
            result = result +  "_" +*exp->lexinfo + ", sizeof (";
         } else {
            result = result + *exp->lexinfo + ", sizeof (";
         }
      }
   }
   return result;
}

string constructVaddr(astree* root, symbol* sym) {
   sym = sym;
   string result = "";
   if(root->symbol == TOK_INDEX) {
      astree* exp = root->children.back();
      if(exp->syminfo->attributes.test(ATTR_vreg) or
        exp->syminfo->attributes.test(ATTR_vaddr)) {
         emitReg(exp);
         result = result + regstk.back();
         regstk.pop_back();
      } else {
         astree* ident = root->children.back();
         if(ident->symbol == TOK_IDENT) {
            result = result + "_";
            if(ident->syminfo->block_nr>0){
               result = result + to_string(ident->syminfo->block_nr);
            }
            result = result + "_" + *ident->lexinfo;
         } else {
            result = result + *ident->lexinfo;
         }
      }
   }
   return result;
}

void emitReg(astree* root) {
   string regPrint = constructReg(root, root->syminfo);
   string typePrint = constructDec(root->syminfo);
   if(*root->lexinfo== "new") {
      string allocation = constructNew(root, root->syminfo);
      oilfile << "        " << typePrint << " "<< regPrint << " = "
      << allocation << typePrint.substr(0,typePrint.length()-1)
      << "));" << endl;
   } else if(root->syminfo->attributes.test(ATTR_vaddr)){
      if(root->symbol == TOK_INDEX) {
         string vaddrPrint = constructVaddr(root, root->syminfo);
         oilfile <<"        " << typePrint << "* " << regPrint << " = "
         << "&_" ;
         if(root->syminfo->block_nr>0) {
            oilfile<<root->syminfo->block_nr;
         } 
         oilfile << "_"  <<*root->children.front()->lexinfo <<"[" 
         << vaddrPrint  << "];" << endl;
         
      } else {
         oilfile << "        " << typePrint << "* " << regPrint<< " = "
         << "&_";
         if(root->children.front()->syminfo->block_nr>0) {
            oilfile<<root->children.front()->syminfo->block_nr;
         } 
         oilfile << "_" << *root->syminfo->fieldId<< "->"
         << "f_"<< *root->syminfo->fieldId << "_"
         << *root->children.back()->lexinfo << ";" << endl;
      }
   } else if(root->symbol == '+' ||
             root->symbol == '-' ||
             root->symbol == '/' ||
             root->symbol == '*' ||
             root->symbol == '%' ||
             root->symbol == TOK_LT||
             root->symbol == TOK_EQ||
             root->symbol == TOK_NE||
             root->symbol == TOK_GT||
             root->symbol == TOK_GE||
             root->symbol == TOK_LE) {
      astree* left = root->children.front();
      astree* right = root->children.back();
      string printOp = "        " + typePrint + " " +regPrint + " = ";
      bool leftop = false;
      bool rightop = false;
      if(left->syminfo->attributes.test(ATTR_vreg) ||
         (left->syminfo->attributes.test(ATTR_string) and
         left->syminfo->attributes.test(ATTR_const)) ||
         left->syminfo->attributes.test(ATTR_vaddr)) {
         emitReg(left);
         leftop = true;
      }
      if(leftop == true) {
         string reg = regstk.back();
         printOp = printOp + reg + " " +
         *root->lexinfo + " ";
         regstk.pop_back();
      } else {
         printOp = printOp + *left->lexinfo + " " +
         *root->lexinfo + " ";
      }
      if(right->syminfo->attributes.test(ATTR_vreg) ||
         (right->syminfo->attributes.test(ATTR_string) and
         right->syminfo->attributes.test(ATTR_const)) ||
         right->syminfo->attributes.test(ATTR_vaddr)) {
         emitReg(right);
         rightop = true;
      }
      if(rightop == true) {
         string reg = regstk.back();
         printOp = printOp + reg + " " +
         *root->lexinfo + " ";
         regstk.pop_back();
      } else {
         printOp = printOp + *right->lexinfo;
      }
      oilfile << printOp << endl;
   } else if(root->symbol == TOK_NEG ||
             root->symbol == TOK_POS ||
             root->symbol == TOK_CHR ||
             root->symbol == TOK_ORD) {
      string printUop = "        " + typePrint + " " +regPrint + " = ";
      astree* child = root->children.front();
      bool childop = false;
      if(child->syminfo->attributes.test(ATTR_vreg) ||
         (child->syminfo->attributes.test(ATTR_string) and
         child->syminfo->attributes.test(ATTR_const)) ||
         child->syminfo->attributes.test(ATTR_vaddr)) {
         emitReg(child);
         childop = true;
      }
      if(childop == true) {
         string reg = regstk.back();
         printUop = printUop + reg;
         regstk.pop_back();
      } else {
         if(root->symbol == TOK_CHR) {
            printUop = printUop + "(int)";
         } else if(root->symbol == TOK_ORD) {
            printUop = printUop + "(char)";
         } else {
            printUop = printUop + *root->lexinfo;
         }
         printUop = printUop + *child->lexinfo;
      }
      oilfile << printUop << endl;
   } else if(root->symbol == TOK_CALL) {
      oilfile << "        " << typePrint << " " << regPrint << " = ";
      emitCall(root);
   }  else {
      oilfile << "        " << typePrint << " " << regPrint << " = "
      << *root->lexinfo << endl;
   }
}

void emitCall(astree* root) {
   astree* funcId = root->children.front();
   if(funcId->syminfo == 0) return;
   vector<astree*>& children = root->children; 
   string printString = "";
   if(root->parent->symbol == TOK_ROOT) {
      printString = "        ";
   }
   printString = printString + "__" + *funcId->lexinfo + "(";
   for(vector<astree*>::iterator it = ++children.begin();
       it != children.end(); ++it) {
      symbol* sym = (*it)->syminfo;
      bool gotReg = false;
      if(sym->attributes.test(ATTR_vreg) ||
      (sym->attributes.test(ATTR_string) and
      sym->attributes.test(ATTR_const)) ||
      sym->attributes.test(ATTR_vaddr)) {
         emitReg(*it);
         gotReg = true;
      }
      if(gotReg == true) {
         string regPrint = regstk.back();
         printString = printString + regPrint;
         if((it+1) != children.end()) {
            printString = printString + ", ";
         }
         regstk.pop_back();
      } else {
         if((*it)->symbol == TOK_IDENT) {
            if((*it)->syminfo->block_nr > 0) {
               printString = printString + " _" + 
               to_string((*it)->syminfo->block_nr) + "_" +
               *((*it)->lexinfo);
            } else {
               printString = printString + " __" + *((*it)->lexinfo);
            } 
         } else {
            printString = printString + *((*it)->lexinfo);
         }
         if((it+1) != children.end()) {
            printString = printString + ", ";
         }
      }
   }
   oilfile << printString << ")" << endl;
}

void emitVardecl(astree* root) {
   astree* leftType = root->children.front();
   astree* typeName = 0;
   if(root->symbol == '=') {
      typeName = leftType;
   } else {
      typeName = leftType->children.back();
   }
   astree* rightType = root->children.back();
   symbol* rightSym = rightType->syminfo;
   if(rightSym == 0 || typeName->syminfo == 0) return;
   bool gotReg = false;
   if(rightSym->attributes.test(ATTR_vreg) ||
      (rightSym->attributes.test(ATTR_string) and
      rightSym->attributes.test(ATTR_const)) ||
      rightSym->attributes.test(ATTR_vaddr)) {
      emitReg(rightType);
      gotReg = true;
   }
   string typePrint = constructDec(typeName->syminfo);
   oilfile << "        ";
   if(root->symbol != '=') {
      oilfile << typePrint;
   }
   if(typeName->syminfo->block_nr > 0) {
      oilfile << " _" << typeName->syminfo->block_nr << "_";
   } else {
      oilfile << " __";
   } 
   oilfile << *typeName->lexinfo << " = ";
   if(gotReg == true) {
      string regPrint = regstk.back();
      oilfile << regPrint << ";" << endl;
      regstk.pop_back();
   } else {
      oilfile << *rightType->lexinfo << ";" << endl;
   }
}

