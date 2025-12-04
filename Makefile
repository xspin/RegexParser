BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := src
TEST_DIR := test
BIN_DIR := /usr/local/bin

# YACC = /usr/local/opt/bison/bin/yacc
# BISON = /usr/local/opt/bison/bin/bison
YACC := flex
BISON := bison

CC := clang++
CFLAGS := -Wall -std=c++17 -I./src -I./build 

GTEST_FLAGS := -lgtest -lgtest_main 

LEX_CC := $(BUILD_DIR)/lex.yy.cc
BISON_CC := $(BUILD_DIR)/y.tab.cc
BISON_HH := $(BUILD_DIR)/y.tab.hh
TARGET := $(BUILD_DIR)/regexparser
TARGET_BIN := $(BIN_DIR)/regexparser
TARGET_TEST := $(BUILD_DIR)/test_parser
LEX_BIN := $(BUILD_DIR)/lexer

DEPENDS := $(SRC_DIR)/parser.l $(SRC_DIR)/parser.y $(wildcard $(SRC_DIR)/*.h)
SRC = $(BISON_CC) $(LEX_CC) $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(BUILD_DIR)/%.cc, $(OBJ_DIR)/%.o, \
	  $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC)))

TEST_OBJS := $(filter-out $(OBJ_DIR)/main.o, $(OBJ))
TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)

all: $(OBJ_DIR) $(TARGET)

lex: $(LEX_BIN)

test: $(TARGET_TEST)
	$(TARGET_TEST)

build: $(BISON_CC) $(LEX_CC)

install: $(TARGET)
	@echo "Install $(TARGET) to $(BIN_DIR) ..."
	install -m 0755 $(TARGET) $(TARGET_BIN)
	@ls -l $(TARGET_BIN)

uninstall:
	@echo "Uninstall $(TARGET_BIN)"
	rm -f $(TARGET_BIN)

$(LEX_BIN): $(LEX_CC) $(BISON_HH)
	$(CC) $(CFLAGS) -o $@ $< $(BISON_CC) $(SRC_DIR)/parser.cpp $(SRC_DIR)/utils.cpp -DLEXER_BIN

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) 

$(LEX_CC): $(SRC_DIR)/parser.l $(SRC_DIR)/parser.h
	$(YACC) -o $@ $< 

$(BISON_HH) $(BISON_CC): $(SRC_DIR)/parser.y
	$(BISON) -o $(BISON_CC) -d $^ --report=all -k

$(OBJ_DIR)/%.o: $(BUILD_DIR)/%.cc 
	$(CC) $(CFLAGS) -c $< -o $@ 
	@echo "cc $< → $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEPENDS)
	$(CC) $(CFLAGS) -c $< -o $@ 
	@echo "cc $< → $@"

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	@echo "mkdir: $@"

$(TARGET_TEST): $(TEST_OBJS) $(TEST_SRC)
	$(CC) $(CFLAGS) $(GTEST_FLAGS) -o $@ $^

clean:
	-rm -rf $(BUILD_DIR)/*

.PHONY: all build clean lex test install