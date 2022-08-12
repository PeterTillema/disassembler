NAME        ?= DISASSEM
COMPRESSED  ?= YES
DESCRIPTION ?= "Disassembler"

CFLAGS ?= -DDEBUG_SUPPORT -O3 -Wall -Wextra

include $(shell cedev-config --makefile)

