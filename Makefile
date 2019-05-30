CXX=g++
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lrrd

SRCS=main.cpp comm.cpp cobs.cpp rrd.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: pulse

pulse: $(OBJS)
	$(CXX) $(LDFLAGS) -o pulse $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
