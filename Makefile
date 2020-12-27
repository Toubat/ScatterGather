#
# CMPSC311 - ScatterGather.co, - Assignment #4
# Makefile - makefile to build the base system
#

# Locations

# Make environment
INCLUDES=-I.
CC=gcc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-lm -lcmpsc311 -L. -lgcrypt -lpthread -lcurl

# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<
	
# Files
OBJECT_FILES=	sg_sim.o \
				sg_driver.o \
				sg_cache.o \
				
# Productions
all : sg_sim

sg_sim : $(OBJECT_FILES)
	$(CC) $(LINKARGS) $(OBJECT_FILES) -o $@ -lsglib $(LIBS)

test:
	./sg_sim -v cmpsc311-assign4-workload.txt

debug:
	gdb ./sg_sim -ex "r -v cmpsc311-assign4-workload.txt"

valgrind:
	valgrind ./sg_sim -v cmpsc311-assign4-workload.txt

clean : 
	rm -f sg_sim $(OBJECT_FILES) 
	
