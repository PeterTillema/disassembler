NAME        ?= CEDISASM
COMPRESSED  ?= YES
DESCRIPTION ?= "Disassembler CE"

CFLAGS ?= -DDEBUG_SUPPORT -Oz -Wall -Wextra
CXXFLAGS ?= -O3 -Wall -Wextra

include $(shell cedev-config --makefile)

