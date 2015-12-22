// $Id: cppstrtok.cpp,v 1.3 2014-10-07 18:09:11-07 - - $

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
using namespace std;

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdexcept>

#include "auxlib.h"
#include "stringset.h"
#include "lyutils.h"
#include "astree.h"
#include "sym.h"
#include "oil.h"

string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;
ofstream tokfile;
ofstream astfile;
ofstream symfile;
ofstream oilfile;

void yyin_cpp_popen (const char* filename) {
   CPP += " ";
   CPP += filename;
   yyin = popen (CPP.c_str(), "r");
   if (yyin == NULL) {
      syserrprintf (CPP.c_str());
      exit (get_exitstatus());
   }
}

void yyin_cpp_pclose (void) {
   int pclose_rc = pclose (yyin);
   eprint_status (CPP.c_str(), pclose_rc);
   if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

void scan_options(int argc, char** argv) {
   yy_flex_debug = 0;
   yydebug = 0;
   for(;;) {
      int option = getopt(argc, argv, "@:D:ly");
      if(option == EOF) break;
      switch(option) {
         case '@':
            set_debugflags(optarg);
            break;
         case 'D':
            CPP = CPP + " -D" + optarg;
            break;
         case 'l':
            yy_flex_debug = 1;
            break;
         case 'y':
            yydebug = 1;
            break;
         default:
            cerr << "%:bad option (" << optopt << ")\n";
            exit(1);
            break;
      }
   }
}

bool want_echo () {
   return not (isatty (fileno (stdin)) and isatty (fileno (stdout)));
}

void printUsage() {
   cerr << "Usage: oc [-ly] [-@ flag ...] [-D string] program.oc\n";
}

int main (int argc, char** argv) {
   set_execname (argv[0]);
   scan_options(argc, argv);
   vector<string> tokens;
   char* file;
   string filename;
   size_t lastdot;
   if ( optind == argc) {
      printUsage();
      return EXIT_FAILURE;
   }
   for (int argi = optind; argi < argc; ++argi) {
      file = argv[argi];
      filename = file;
      try {
         lastdot = filename.find_last_of(".");
         string fileExten = filename.substr(lastdot, filename.length());
         if(fileExten != ".oc") {
            if(fileExten != ".oc") {
               printUsage();
               return EXIT_FAILURE;
            }
         }
      } catch (out_of_range & e) {
         printUsage();
         return EXIT_FAILURE;
      }
      //printf ("command=\"%s\"\n", command.c_str());
      yyin_cpp_popen(file);
      
      
      break;
   }
   string tokfilename = filename.substr(0, lastdot) + ".tok";
   string strfilename = filename.substr(0, lastdot) + ".str";
   string astfilename = filename.substr(0, lastdot) + ".ast";
   string symfilename = filename.substr(0, lastdot) + ".sym";
   string oilfilename = filename.substr(0, lastdot) + ".oil"; 
  
   tokfile.open(tokfilename.c_str(), ofstream::trunc);
   astfile.open(astfilename.c_str(), ofstream::trunc);
   symfile.open(symfilename.c_str(), ofstream::trunc);   
   oilfile.open(oilfilename.c_str(), ofstream::trunc); 

   yyparse();
   yyin_cpp_pclose();
   symbolizeTree(yyparse_astree);
   emitTree(yyparse_astree);
   dump_astree(yyparse_astree);
   ofstream ofile(strfilename.c_str(), ofstream::trunc);
   //streambuf *oldbuf = cout.rdbuf();
   //cout.rdbuf(ofile.rdbuf());

   //freopen(filename.c_str(), "w", stdout);
   dump_stringset(ofile);
   yylex_destroy();
   //cout.rdbuf(oldbuf);
   return get_exitstatus();
}

