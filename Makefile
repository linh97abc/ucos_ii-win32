UCOSII_BASE := UCOSII
CONFIG_UCOSII := 1
CONFIG_UCOSII_WIN32 := 1

inc_1 := 
src_1 :=

-include $(UCOSII_BASE)/subdir.mk



SRC := $(src_1)
INC := $(inc_1)

SRC += hello_ucosii.c

OBJ := $(SRC:%=build/%.o)
APP_INC := $(INC:%=-I%)

CC := gcc
APP_CFLAGS := -g
APP_CFLAGS += $(APP_INC)
LINK := gcc
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

build/%.c.o: %.c
	$(compile_c)

all: ${OBJ}
	$(LINK) $(LINK_FLAG) -o main.exe ${OBJ} ${LIB}

clean:
	@rm -rf build
	@rm -rf main.exe
