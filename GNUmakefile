
TESTROWS?=1000
TESTCOLS?=1000

TARGETS = transpose colsum fill
SHARED_SRCS = memmap.cc
CPPFLAGS=
WFLAGS=-pedantic -W -Wall
OFLAGS?=-O0
GFLAGS=-ggdb

SHARED_OBJS = $(SHARED_SRCS:.cc=.o)

CXXFLAGS=-std=c++0x $(WFLAGS) $(GFLAGS) $(OFLAGS)
LDFLAGS=$(GFLAGS) $(OFLAGS)
LDLIBS=-lrt
CC=g++

all: $(TARGETS)

$(TARGETS): $(SHARED_SRCS:.cc=.o)

%.o : %.cc
	$(COMPILE.cc) -MD -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
              -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d

clean:
	$(RM) *.o *~ *.P .#* *.log $(TARGETS)

realclean: clean
	$(RM) *.png *.mpg *.trace* data.*


TESTFILL=data.$(TESTROWS)x$(TESTCOLS)
$(TESTFILL): fill
	./fill --rows $(TESTROWS) --cols $(TESTCOLS) -o $@ 2>&1 \
	| tee "$@".log
TESTTRANS=$(TESTFILL)T
$(TESTTRANS): $(TESTFILL) transpose
	./transpose --rows $(TESTROWS) --cols $(TESTCOLS) -i $< -o $@ 2>&1 \
	| tee "$@".log
TESTCOLSUM=$(TESTTRANS).sum
$(TESTCOLSUM): $(TESTTRANS) colsum
	./colsum --rows $(TESTROWS) --cols $(TESTCOLS) -i $< 2>&1 -o $@ \
	| tee "$@".log

test: $(TESTFILL) $(TESTCOLSUM) $(TESTTRANS)

hugetest:
	TESTROWS=1000000 TESTCOLS=3000 $(MAKE) test

-include $(patsubst %.cc,%.P,$(wildcard *.cc))
