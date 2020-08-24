CC := gcc
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include
LIBD := lib
UTILD := util

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_LIBF := $(shell find $(LIBD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))
FUNC_FILES := $(filter-out build/main.o, $(ALL_OBJF))

TEST_SRC := $(shell find $(TSTD) -type f -name *.c)

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wno-unused-function -MMD
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=c99
POSIX := -D_POSIX_SOURCE
BSD := -D_DEFAULT_SOURCE
TEST_LIB := -lcriterion
LIBS := $(LIBD)/sf_event.o -lm

CFLAGS += $(STD) $(POSIX) $(BSD)

EXEC := jobber
TEST := $(EXEC)_tests

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(BIND)/$(TEST)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(ALL_OBJF)
	$(CC) $^ -o $@ $(LIBS)

$(BIND)/$(TEST): $(FUNC_FILES) $(TEST_SRC)
	$(CC) $(CFLAGS) $(INC) $(FUNC_FILES) $(TEST_SRC) $(TEST_LIB) $(LIBS) -o $@

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(LIBD)/%.o: $(LIBD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf $(BLDD) $(BIND)

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
