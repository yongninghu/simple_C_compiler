#ifndef __OIL_H__
#define __OIL_H__

#include <iostream>
#include <fstream>
#include <string>

#include "astree.h"
#include "sym.h"

extern ofstream oilfile;

void emitTree(astree* root);
void resetRegisters();
void emitStruct(astree* root);
void emitFunction(astree* root);
void emitBlock(astree* root);
void emitReg(astree* root);
void emitVardecl(astree* root);
void emitCall(astree* root);
void emitIf(astree* root);
void emitWhile(astree* root);
void emitIfelse(astree* root);
string constructDec(symbol* root);
string constructReg(astree* root, symbol* sym);

#endif