#------------------------------------------------------------------------------
# Copyright (c) 2007, Stanford University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Stanford University nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#------------------------------------------------------------------------------ 


#------------------------------------------------------------------------------
# File: Makefile
# 
# Note: This Makefile requires GNU make.
#------------------------------------------------------------------------------

CC = gcc

OSTYPE = $(shell uname)

ifeq ($(OSTYPE),CYGWIN_NT-5.1)
ARCH = -D_CYGWIN_
endif

ifeq ($(OSTYPE),Linux)
ARCH = -D_LINUX_
endif

ifeq ($(OSTYPE),SunOS)
ARCH =  -D_SOLARIS_
endif

ifeq ($(OSTYPE),Darwin)
ARCH = -D_DARWIN_
endif

PATH_OUT = ./$(OSTYPE)/

CFLAGS = -Wall $(ARCH) -O3
#LIBS = -lm -lpthread -lrt -lcpc
LIBS = -lm -lpthread -lrt

REV_SEQ_OBJS = reverseindex-seq.o
REV_PTHREAD_OBJS = reverseindex-pthread.o
REV_OBJS = reverseindex.o
COM_OBJS = MapReduceScheduler.o

PROGS = reverseindex reverseindex-seq reverseindex-pthread

.PHONY: default clean

all: $(PROGS)

reverseindex-seq: $(REV_SEQ_OBJS)
	$(CC) $(CFLAGS) -o $@ $(REV_SEQ_OBJS) $(LIBS)

reverseindex-pthread: $(REV_PTHREAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(REV_PTHREAD_OBJS) $(LIBS)

reverseindex: $(COM_OBJS) $(REV_OBJS)
	echo "Target OS: $(OSTYPE), command: $(CC) $(CFLAGS) -o $@ $(COM_OBJS) $(REV_OBJS) $(LIBS)"
	$(CC) $(CFLAGS) -o $@ $(COM_OBJS) $(REV_OBJS) $(LIBS)

clean:
	rm -f $(PROGS) *.o *.a *~ *.tmp *.bak *.log
