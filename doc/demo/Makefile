###################################################
# Location of the project directory and Makefiles
#
P := ../..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := Demo
TARGET_DESCRIPTION := Demo for DynamicLibrary
COMPILATION_MODE := release
CXX_STANDARD := --std=c++20

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
# Make the list of compiled files for the application
#
SRC_FILES += demo.cpp

###################################################
# Linkage against our project library
#
INTERNAL_LIBS := $(call internal-lib,$(PROJECT_NAME))
LINKER_FLAGS +=

###################################################
# Sharable information between all Makefiles
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
pre-build:: compile-lib-example compile-lib-good compile-lib-problematic compile-lib-static

###################################################
# Compile the lib used for the demo
#
.PHONY: compile-lib-example
compile-lib-example:
	$(Q)$(MAKE) --no-print-directory --directory=libexample all

###################################################
# Compile the lib used for the demo
#
.PHONY: compile-lib-good
compile-lib-good:
	$(Q)$(MAKE) --no-print-directory --directory=libgood all

###################################################
# Compile the lib used for the demo
#
.PHONY: compile-lib-problematic
compile-lib-problematic:
	$(Q)$(MAKE) --no-print-directory --directory=libproblematic all

###################################################
# Compile the lib used for the demo
#
.PHONY: compile-lib-static
compile-lib-static:
	$(Q)$(MAKE) --no-print-directory --directory=libstatic all