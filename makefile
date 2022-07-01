BINAME=testdiff
SONAME=testdiff.so
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:%.cpp=%.o)
CXXFLAGS=-Og -Wall -Wextra -std=c++17 -fPIC -g $(shell pkg-config --cflags r_main)
LDFLAGS=$(shell pkg-config --libs r_main)
DEBUGFLAGS=-fsanitize=undefined,address


ifneq ($(DEBUG), )
	CXXFLAGS += $(DEBUGFLAGS)
	LDFLAGS += $(DEBUGFLAGS)
endif


all: $(OBJS)
	$(CXX) $(LDFLAGS) -o $(BINAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(BINAME)
	rm -f $(OBJS)


.PHONY: clean
