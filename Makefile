#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CPP = g++

# define any compile-time flags
CPPFLAGS = -Wall -g -std=c++11 -pthread

# define any directories containing header files other than /usr/include
#
INCLUDES = -I./include

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lrrd -lcurl

# define src and obj directories
SRC_DIR = src
OBJ_DIR = obj

# define the C source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# define the executable file 
MAIN = pulse

#
# get version info from git and compile into the program
# https://embeddedartistry.com/blog/2016/10/27/giving-you-build-a-version
#

version := $(subst -, ,$(shell git describe --long --dirty --tags))
COMMIT := $(strip $(word 3, $(version)))
COMMITS_PAST := $(strip $(word 2, $(version)))
DIRTY := $(strip $(word 4, $(version)))
ifneq ($(COMMITS_PAST), 0)
    BUILD_INFO_COMMITS := "."$(COMMITS_PAST)
endif
ifneq ($(DIRTY),)
    BUILD_INFO_DIRTY :="+"
endif

export BUILD_TAG :=$(strip $(word 1, $(version)))
export BUILD_INFO := $(COMMIT)$(BUILD_INFO_COMMITS)$(BUILD_INFO_DIRTY)

CPPFLAGS += -DVERSION_BUILD_DATE=\""$(shell date "+%F %T")"\" \
            -DVERSION_BUILD_MACHINE=\""$(shell echo `whoami`@`hostname`)"\" \
            -DVERSION_TAG=\"$(BUILD_TAG)\" \
            -DVERSION_BUILD=\"$(BUILD_INFO)\"

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean install out

all: $(MAIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(MAIN): $(OBJ_DIR) $(OBJS) 
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) $(OBJS) *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# define install directory
ifeq ($(DESTDIR),)
  DESTDIR = /usr/local
endif

install: all
	install -d $(DESTDIR)/bin/ 
	install -m 755 $(MAIN) $(DESTDIR)/bin/

# DO NOT DELETE THIS LINE -- make depend needs it
