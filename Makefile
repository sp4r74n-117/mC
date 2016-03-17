CXX=/scratch/c703/c703432/gcc-5.3.0-install/bin/g++
RM=rm -f
CPPFLAGS=-g -Wall --std=c++14
LDFLAGS=-g

SRCS=$(shell ls *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

all:  $(OBJS)
	$(CXX) $(LDFLAGS) -o mC $(OBJS) $(LDLIBS) 
	
%.o : %.cpp
	$(CXX) $(CPPFLAGS) $< -c -o $@

clean:
	$(RM) $(OBJS)

print-%  : ; @echo $* = $($*)