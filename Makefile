###############################################################
#An example Makefile for SpaceWire RMAP Library.
###############################################################

#Note 1:
#To compile a user application with SpaceWire RMAP Library,
#set SPACEWIRERMAPLIBRARY_PATH and XERCESDIR in the shell 
#initialization file first.
#
#Execute below to check if these variables are correctly
#set in your shell.
#
# > ls $SPACEWIRERMAPLIBRARY_PATH
# > ls $XERCESDIR
#
#If no error is observed, the paths seem valid.

#Note 2:
#This Makefile assumes a user-application source code named
#UserApplication.cc. If other source files, include paths,
#and/or linker flags are necessary for compile, add them to
#CXXFLAGS and LDFLAGS.

###############################################################


#Set target (binary names)
#See also the rule part below.
TARGETS = \
main_VirtualSpaceWireRouter

#Check CxxUtilities
ifndef CXXUTILITIES_PATH
CXXUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/CxxUtilities
endif

#VirtualSpaceWireRouter does not use XML-like RMAP Target node definition.
##Check XMLUtilities
#ifndef (XMLUTILITIES_PATH
#XMLUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/XMLUtilities
#endif

#Set compiler/linker flags
CXXFLAGS = -I$(SPACEWIRERMAPLIBRARY_PATH)/includes -I$(CXXUTILITIES_PATH)/includes


TARGETS_OBJECTS = $(addsuffix .o, $(basename $(TARGETS)))
TARGETS_SOURCES = $(addsuffix .cc, $(basename $(TARGETS)))

###############################################################

.PHONY : all

all : $(TARGETS)

main_VirtualSpaceWireRouter : main_VirtualSpaceWireRouter.o
	$(CXX) -g $(CXXFLAGS) -o $@ $@.cc $(LDFLAGS)
        
clean :
	rm -rf $(TARGETS) $(addsuffix .o, $(TARGETS))
