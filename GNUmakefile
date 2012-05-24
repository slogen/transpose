
SRCS = transpose.cc memmap.cc
TARGET = transpose

CXXFLAGS=-std=c++0x -pedantic -W -Wall -ggdb -O3
LDFLAGS=-ggdb
MAKEDEPEND = g++ -M $(CPPFLAGS) $(CXXFLAGS) -o $*.d $<

all: $(TARGET)

%.o : %.cc
	$(COMPILE.cc) -MD -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
              -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d

clean:
	$(RM) *.o *~ .#* $(TARGET)

$(TARGET): $(SRCS:.cc=.o)
	$(CXX) $(LDFLAGS) -o $@ $^

test:	$(TARGET)
	(date; /usr/bin/time -v ./$(TARGET); date) | tee test.log

-include $(SRCS:.cc=.P)
