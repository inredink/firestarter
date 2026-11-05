#####################################
#
#  Standard Makefile for BlitzKrieg
#     designed by marcello@cellosoft.com
#
#####################################

NAME=firestarter

# Compiler
CXX=g++

# Mingw location
MINGW=c:/mingw/

# Compiler options
WIN32=1
DEBUGMODE=1

### Warn mode
WFLAGS = -Wall -W -Wno-unused -Wno-write-strings -fpermissive -std=gnu++98 -fmessage-length=0


CFLAGS = $(WFLAGS) -MMD -DXP_WIN -D_WINDOWS -DWIN32 -D_MINGW $(EXTRA_CFLAGS)


ifdef	DEBUGMODE
	CFLAGS	+= -DDEBUGMODE=$(DEBUGMODE) -DDEBUG=1 -g
	DEBUG_DIR=_debug
else
	CFLAGS	+= $(OFLAGS)
	DEBUG_DIR=
endif

### Libraries

LIBDIR  = 
LIBS = -laldmb -ldumb -lldpng -lpng -lz -lalleg

ifdef WIN32
	RM = rm -rf
	PATH_SEP = "\\"
	EXE = .exe
else
	RM = rm -rf
	PATH_SEP = /
	EXE = 
endif

### Get OBJS
include Makefile.lst


.PHONY: all test clean

first: test

all: $(NAME)$(DEBUG_DIR)$(EXE)

test: $(NAME)$(DEBUG_DIR)$(EXE)
	$(NAME)$(DEBUG_DIR)$(EXE)

src/Data.h: data/fonts.dat
	dat -h $@ -p DATA_FONTS_ $^

$(NAME)$(DEBUG_DIR)$(EXE): $(OBJS)
	$(CXX) $(OBJS) -o $@ -mwindows $(LIBDIR) $(LIBS)
	cp -f $@ $(NAME)$(EXE)

editor$(EXE): $(EDITOR_OBJS)
	$(CXX) $(EDITOR_OBJS) -o $@ $(LIBDIR) $(LIBS)

-include $(addprefix objs$(DEBUG_DIR)/,$(addsuffix .d,$(notdir $(MODULES) $(JS_MODULES))))


objs$(DEBUG_DIR)/%.o: src/%.cpp src/%.h
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(CFLAGS)
	
	
clean:
	$(RM) $(NAME)$(EXE) $(NAME)$(DEBUG_DIR)$(EXE)
	$(RM) objs$(DEBUG_DIR)/*.d objs$(DEBUG_DIR)/partypack/*.d
	$(RM) objs$(DEBUG_DIR)/*.o objs$(DEBUG_DIR)/partypack/*.o
