NAME = mocha
INCLUDE = -Iexternal
ifeq ($(OS),Windows_NT)
    LFLAGS = -pthread -lm
else
    LFLAGS = -lpthread -lm -ldl
endif
