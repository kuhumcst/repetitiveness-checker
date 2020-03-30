#profiling
#PG=-pg
PG=

PNAME=repver$(PG)
VERSION=2
MINOR_NUMBER=1

BIN=../../../bin/

LIB=

PNAMESTATIC=$(BIN)$(PNAME)s
PNAMEDYNAMIC=$(BIN)$(PNAME)
PNAMEDYNAMICLIB=$(BIN)$(PNAME)dl
LINKERNAME=$(LIB)lib$(PNAME).so
SONAME=$(LINKERNAME).$(VERSION)
REALNAME=$(SONAME).$(MINOR_NUMBER)

SRCDIR	 = .
LETTERFUNCDIR	= ../../letterfunc/src
GCCINC += -I$(SRCDIR) -I$(LETTERFUNCDIR)

CC=/usr/bin/g++ -O3 -Wall $(GCCINC) -pedantic -DNDEBUG 

# -fPIC or -fpic: enable 'position independent code' generation. Necessary for shared libs
# -fpic may generate smaller and faster code, but will have platform-dependent limitations
PIC=-fPIC
#PIC=

CCLINKSTATIC=/usr/bin/g++ -static
CCLINKDYNAMIC=/usr/bin/g++ 
CCCREATELIB=/usr/bin/g++ -shared -Wl,-soname,$(SONAME)


#DEBUG=-g
DEBUG=

GCCLINK=-L/usr/lib64 -lstdc++

RM=rm -f

PROJECTSRC=\
	argopt.cpp\
	$(LETTERFUNCDIR)/letter.cpp\
	$(LETTERFUNCDIR)/letterfunc.cpp\
	$(LETTERFUNCDIR)/utf8func.cpp\
	option.cpp

CSTPROJECTSRC=\
	repetitions.cpp

PROJECTOBJS=\
	argopt.o\
	letter.o\
	letterfunc.o\
	utf8func.o\
	option.o

CSTPROJECTOBJS=\
	repetitions.o

#							   #
#   Dependencies start here.	#
#							   #


$(PNAMEDYNAMIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) $(PG) -o $@ $(GCCLINK)

$(PNAMESTATIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKSTATIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) $(PG) -o $@ $(GCCLINK)

$(REALNAME) : $(PROJECTOBJS)
	$(CCCREATELIB) -o $@ $(PROJECTOBJS)
	ln -sf $(REALNAME) $(SONAME)
	ln -sf $(SONAME) $(LINKERNAME)

$(PNAMEDYNAMICLIB): $(REALNAME) $(CSTPROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) $(PG) -L. -l$(PNAME) -o $@ $(GCCLINK)


all: $(PNAMEDYNAMIC) $(REALNAME) $(PNAMEDYNAMICLIB)
allincludingstaticsonotformacosx: $(PNAMESTATIC) $(PNAMEDYNAMIC) $(REALNAME) $(PNAMEDYNAMICLIB)

.cpp.o:
	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $(PG) $(CSTPROJECTSRC) $(PROJECTSRC)

#.cpp.o:
#	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $?

clean:
	$(RM) *.o
	$(RM) $(REALNAME)
	$(RM) $(SONAME)
	$(RM) $(LINKERNAME)

