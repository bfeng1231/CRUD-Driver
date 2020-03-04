#
# CMPSC311 - Fall 2017
# Assignment #4 Makefile
#

# Variables
# replaces rhs words with inputs (lhs)
CC=gcc 
LINK=gcc
CFLAGS=-c -Wall -I. -fpic -g
LINKFLAGS=-L. -g
LINKLIBS=-lcrud -lgcrypt

# Files to build
# object files
HDD_CLIENT_OBJFILES=   hdd_sim.o \
                        hdd_file_io.o  \
                        hdd_client.o \
# name of file to be built                    
TARGETS=    hdd_client
             
# defines file types                    
# Suffix rules
.SUFFIXES: .c .o
# gcc -c -Wall -I. -fpic -g is the current rule and is the first preresiquisite
.c.o:
	$(CC) $(CFLAGS)  -o $@ $<

# Productions

all : $(TARGETS) 
# hdd_client depends on object files	    
hdd_client: $(HDD_CLIENT_OBJFILES)
	$(LINK) $(LINKFLAGS) -o $@ $(HDD_CLIENT_OBJFILES) $(LINKLIBS) 

# Cleanup
# deletes hdd_client and object files defined in HDD_CLIENT_OBJFILES 
clean:
	rm -f $(TARGETS) $(HDD_CLIENT_OBJFILES)
