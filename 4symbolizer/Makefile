# $Id: Makefile,v 1.9 2015-03-25 19:01:26-07 - - $

GCC        = g++ -g -O0 -Wall -Wextra -std=gnu++11
MKDEP      = g++ -MM -std=gnu++11
VALGRIND   = valgrind --leak-check=full --show-reachable=yes

MKFILE     = Makefile
DEPFILE    = Makefile.dep
SOURCES    = auxlib.cpp cppstrtok.cpp stringset.cpp lyutils.cpp \
astree.cpp sym.cpp
HEADERS    = auxlib.h stringset.h lyutils.h astree.h sym.h
LSOURCES   = scanner.l
YSOURCES   = parser.y
CLGEN      = yylex.cpp
HYGEN      = yyparse.h
CYGEN      = yyparse.cpp
LREPORT    = yylex.output
YREPORT    = yyparse.output
OBJECTS    = ${ALLSOURCE:.cpp=.o}
ALLSOURCE  = ${HEADERS} ${SOURCES} ${CLGEN} ${CYGEN}
EXECBIN    = oc
SRCFILES   = ${HEADERS} ${SOURCES} ${MKFILE}

all : ${CLGEN} ${CYGEN} ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o ${EXECBIN} ${OBJECTS}

%.o : %.cpp
	${GCC} -c $<

${CLGEN} : ${LSOURCES}
	flex --outfile=${CLGEN} ${LSOURCES} 2>${LREPORT}
	- grep -v '^  ' ${LREPORT}

${CYGEN} ${HYGEN} : ${YSOURCES}
	bison --defines=${HYGEN} --output=${CYGEN} ${YSOURCES}

ci :
	cid + ${CHECKINS}
	checksource ${CHECKINS}

clean :
	- rm ${OBJECTS}

spotless : clean
	- rm ${EXECBIN} ${LISTING} ${LISTING:.ps=.pdf} ${DEPFILE} \
	     test.out test.err misc.lis

