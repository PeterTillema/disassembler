NAME        ?= DISASSEM
COMPRESSED  ?= YES
DESCRIPTION ?= "Disassembler"

CFLAGS ?= -DDEBUG_SUPPORT -Oz -Wall -Wextra
CXXFLAGS ?= -O3 -Wall -Wextra

include $(shell cedev-config --makefile)

