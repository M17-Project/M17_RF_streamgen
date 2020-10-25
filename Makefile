CC := gcc
CCFLAG := -Wall -O2 -std=c99
DBGFLAG := -g
CCOBJFLAG := $(CCFLAG) -c

BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := debug

TARGET_NAME := m17enc
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)
MAIN_SRC := src/M17_RF_streamgen.c

SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

DISTCLEAN_LIST := $(OBJ) $(OBJ_DEBUG)
CLEAN_LIST := $(TARGET) $(TARGET_DEBUG) $(DISTCLEAN_LIST)

default: all

$(TARGET): $(OBJ)
	$(CC) $(CCFLAG) -o $@ $?

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CC) $(CCOBJFLAG) -o $@ $<

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CC) $(CCOBJFLAG) $(DBGFLAG) -o $@ $<

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CCFLAG) $(DBGFLAG) $? -o $@

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@echo CLEAN $(DISTCLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)
