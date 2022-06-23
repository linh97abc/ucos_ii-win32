UCOSII_BASE := UCOSII
CONFIG_UCOSII := 1
CONFIG_UCOSII_WIN32 := 1

APP_BASE := osal

inc_1 := 
src_1 :=
def_1 :=

-include $(UCOSII_BASE)/subdir.mk

##app
-include $(APP_BASE)/subdir.mk

SRC := $(src_1)
INC := $(inc_1)
DEF := $(def_1)

OBJ := $(SRC:%=build/%.o)
APP_INC := $(INC:%=-I%)

EXE := main.exe

CC := gcc
CPP := g++
CPPFLAG := -std=c++11
APP_CFLAGS := -g -O0
APP_CFLAGS += $(APP_INC) $(DEF:%=-D%)
LINK := g++
LINK_FLAG := -g
ECHO := echo
MKDIR := mkdir -p

MINGW_LIB:=/mingw64/x86_64-w64-mingw32/lib
LIB:=$(MINGW_LIB)/libwinmm.a 

define compile_c
@$(ECHO) Info: Compiling $< to $@
@$(MKDIR) $(@D)
$(CC) -MP -MMD -c $(APP_CFLAGS) -o $@ $<
$(CC_POST_PROCESS)
endef

define compile_cpp
@$(ECHO) Info: Compiling $< to $@
@$(MKDIR) $(@D)
$(CPP) -MP -MMD -c $(APP_CFLAGS) $(CPPFLAG) -o $@ $<
$(CPP_POST_PROCESS)
endef

build/%.c.o: %.c
	$(compile_c)

build/%.cc.o: %.cc
	$(compile_cpp)

build/%.cpp.o: %.cpp
	$(compile_cpp)

build/%.cxx.o: %.cpp
	$(compile_cpp)

.PHONY : all

all:
	@$(ECHO) $(EXE) Build complete

all: $(EXE)

# Create list of dependancy files for each object file.
APP_DEPS := $(OBJ:.o=.d)

# Include the dependency files unless the make goal is performing a clean
# of the application.
ifneq ($(firstword $(MAKECMDGOALS)),clean)
ifneq ($(firstword $(MAKECMDGOALS)),clean_all)
-include $(APP_DEPS)
endif
endif

$(EXE): ${OBJ}
	@$(ECHO) Info: Linking $@
	$(LINK) $(LINK_FLAG) -o $(EXE) ${OBJ} ${LIB}

clean:
	@rm -rf build
	@rm -rf $(EXE)
