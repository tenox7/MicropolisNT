# Simple makefile for MicropolisNT

# Define directories
SRC_DIR = src
OBJ_DIR = obj

# Define source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/simulation.c \
       $(SRC_DIR)/zone.c \
       $(SRC_DIR)/power.c \
       $(SRC_DIR)/traffic.c \
       $(SRC_DIR)/scanner.c \
       $(SRC_DIR)/evaluation.c \
       $(SRC_DIR)/budget.c \
       $(SRC_DIR)/scenarios.c \
       $(SRC_DIR)/disasters.c \
       $(SRC_DIR)/tools.c \
       $(SRC_DIR)/animation.c

# Define object files
OBJS = $(OBJ_DIR)/main.obj \
       $(OBJ_DIR)/simulation.obj \
       $(OBJ_DIR)/zone.obj \
       $(OBJ_DIR)/power.obj \
       $(OBJ_DIR)/traffic.obj \
       $(OBJ_DIR)/scanner.obj \
       $(OBJ_DIR)/evaluation.obj \
       $(OBJ_DIR)/budget.obj \
       $(OBJ_DIR)/scenarios.obj \
       $(OBJ_DIR)/disasters.obj \
       $(OBJ_DIR)/tools.obj \
       $(OBJ_DIR)/animation.obj

# Compiler flags
CFLAGS = /O2 /nologo /I$(SRC_DIR)

all: $(OBJ_DIR) $(OBJS)
	link.exe /NOLOGO /SUBSYSTEM:WINDOWS $(OBJS) user32.lib gdi32.lib comdlg32.lib /OUT:MicropolisNT.exe

$(OBJ_DIR)/main.obj: $(SRC_DIR)/main.c $(SRC_DIR)/simulation.h $(SRC_DIR)/tools.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/main.c /Fo$(OBJ_DIR)/main.obj

$(OBJ_DIR)/simulation.obj: $(SRC_DIR)/simulation.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/simulation.c /Fo$(OBJ_DIR)/simulation.obj

$(OBJ_DIR)/zone.obj: $(SRC_DIR)/zone.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/zone.c /Fo$(OBJ_DIR)/zone.obj
	
$(OBJ_DIR)/power.obj: $(SRC_DIR)/power.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/power.c /Fo$(OBJ_DIR)/power.obj
	
$(OBJ_DIR)/traffic.obj: $(SRC_DIR)/traffic.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/traffic.c /Fo$(OBJ_DIR)/traffic.obj
	
$(OBJ_DIR)/scanner.obj: $(SRC_DIR)/scanner.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/scanner.c /Fo$(OBJ_DIR)/scanner.obj
	
$(OBJ_DIR)/evaluation.obj: $(SRC_DIR)/evaluation.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/evaluation.c /Fo$(OBJ_DIR)/evaluation.obj
	
$(OBJ_DIR)/budget.obj: $(SRC_DIR)/budget.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/budget.c /Fo$(OBJ_DIR)/budget.obj

$(OBJ_DIR)/scenarios.obj: $(SRC_DIR)/scenarios.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/scenarios.c /Fo$(OBJ_DIR)/scenarios.obj
	
$(OBJ_DIR)/disasters.obj: $(SRC_DIR)/disasters.c $(SRC_DIR)/simulation.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/disasters.c /Fo$(OBJ_DIR)/disasters.obj
	
$(OBJ_DIR)/tools.obj: $(SRC_DIR)/tools.c $(SRC_DIR)/simulation.h $(SRC_DIR)/tools.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/tools.c /Fo$(OBJ_DIR)/tools.obj
	
$(OBJ_DIR)/animation.obj: $(SRC_DIR)/animation.c $(SRC_DIR)/simulation.h $(SRC_DIR)/animtab.h
	cl.exe $(CFLAGS) /c $(SRC_DIR)/animation.c /Fo$(OBJ_DIR)/animation.obj

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

clean:
	-del *.obj 2>nul
	-del $(OBJ_DIR)\*.obj 2>nul
	-del MicropolisNT.exe 2>nul
	-del err.out 2>nul