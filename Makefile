CXX=g++
RM=rm -f
CPPFLAGS=-g -Wall --std=c++14 -I.
LDFLAGS=

SRCS=$(shell ls *.cpp) $(shell ls frontend/*.cpp) $(shell ls backend/*.cpp) $(shell ls tests/*.cpp) $(shell ls core/*.cpp) $(shell ls core/analysis/*.cpp) $(shell ls core/checks/*.cpp) $(shell ls core/passes/*.cpp) $(shell ls utils/*.cpp) $(shell ls core/arithmetic/*.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

all:  $(OBJS)
	$(CXX) $(LDFLAGS) -o mC $(OBJS) $(LDLIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $< -c -o $@

clean:
	$(RM) $(OBJS)
	$(RM) mC

print-%  : ; @echo $* = $($*)
