P := ../../..
M := $(P)/.makefile

include $(P)/Makefile.common
TARGET_NAME := problematic
TARGET_DESCRIPTION := Problematic library for dynamic library testing
COMPILATION_MODE := release
DO_NOT_COMPILE_STATIC_LIB := 1

include $(M)/project/Makefile

LIB_FILES += problematic_lib.cpp

include $(M)/rules/Makefile