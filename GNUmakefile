
TARGETS = main
SHARED_SRCS = memmap.cc
CPPFLAGS=
WFLAGS=-pedantic -W -Wall
OFLAGS=-O3
GFLAGS=-ggdb


SHARED_OBJS = $(SHARED_SRCS:.cc=.o)

CXXFLAGS=-std=c++0x $(WFLAGS) $(GFLAGS) $(OFLAGS)
LDFLAGS=$(GFLAGS)
LDLIBS=-lrt
CC=g++

all: $(TARGETS)

$(TARGETS): $(SHARED_SRCS:.cc=.o) $(TARGETS:=.o)

%.o : %.cc
	$(COMPILE.cc) -MD -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
              -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d

clean:
	$(RM) *.o *~ *.P .#* $(TARGETS)

test:	$(TARGETS)
	@date; \
	for x in $(realpath $^); do \
	  echo $$x; \
	  $$x --rows 10000 --cols 10000 --strategy mmap1; \
	  date; \
	done

-include $(patsubst %.cc,%.P,$(wildcard *.cc))
