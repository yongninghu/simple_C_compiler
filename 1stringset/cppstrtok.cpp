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
//#include "lyutils.h"
//#include "astree.h"
string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;
int yy_flex_debug;
int yydebug;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}


// Run cpp against the lines of the file.
void cpplines (vector<string> &tokens, FILE* pipe, char* filename) {
   int linenr = 1;
   char inputname[LINESIZE];
   strcpy (inputname, filename);
   for (;;) {
      char buffer[LINESIZE];
      char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      if (fgets_rc == NULL) break;
      chomp (buffer, '\n');
      //printf ("%s:line %d: [%s]\n", filename, linenr, buffer);
      // http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                              &linenr, filename);
      if (sscanf_rc == 2) {
         //printf ("DIRECTIVE: line %d file \"%s\"\n", linenr, filename);
         continue;
      }
      char* savepos = NULL;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) {
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = NULL;
         if (token == NULL) break;
         //printf ("token %d.%d: [%s]\n",
         //        linenr, tokenct, token);
         tokens.push_back(token);
      }
      ++linenr;
   }
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
      string fileExten;
      try {
         lastdot = filename.find_last_of(".");
         fileExten = filename.substr(lastdot, filename.length());
      } catch (out_of_range & e) {
         printUsage();
         return EXIT_FAILURE;
      }
      if(fileExten != ".oc") {
         printUsage();
         return EXIT_FAILURE;
      }
      string command = CPP + " " + file;
      //printf ("command=\"%s\"\n", command.c_str());
      FILE* pipe = popen (command.c_str(), "r");
      if (pipe == NULL) {
         syserrprintf (command.c_str());
      }else {
         cpplines (tokens, pipe, file);
         int pclose_rc = pclose (pipe);
         eprint_status (command.c_str(), pclose_rc);
      }
      break;
   }
   for(const string& token: tokens) {
      const string* str = intern_stringset(token.c_str());
      str = str;
      //cout << "intern(" << token << ") returned " << str
      //     << "->\"" << *str << "\"" << endl;
   }
   filename = filename.substr(0, lastdot) + ".str";

   ofstream ofile(filename.c_str(), ofstream::trunc);
   //streambuf *oldbuf = cout.rdbuf();
   //cout.rdbuf(ofile.rdbuf());

   //freopen(filename.c_str(), "w", stdout);
   dump_stringset(ofile);

   //cout.rdbuf(oldbuf);
   return get_exitstatus();
}

