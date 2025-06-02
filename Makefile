###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := C++ Wrapper for hot-reloading dynamic libraries
COMPILATION_MODE := release

###################################################
# Project definition
#
include $(M)/project/Makefile

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/include

###################################################
# Project defines
#
DEFINES +=

###################################################
# Linkage
#
LINKER_FLAGS += -ldl

###################################################
# Make the list of compiled files for the application
#
LIB_FILES += $(P)/src/DynamicLibrary.cpp

###################################################
# Sharable information between all Makefiles
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
all:: compile-demo

###################################################
# Compile the demo
#
.PHONY: compile-demo
compile-demo:
	$(Q)$(MAKE) --no-print-directory --directory=doc/demo all