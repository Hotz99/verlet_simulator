CXX := clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -O3 -Isrc
LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system

TARGET := executable
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
SRC := ./src/main.cpp
ASSETS_DIR := ./assets

# macOS Homebrew SFML detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    BREW_PATHS := /usr/local /opt/homebrew
    SFML_FOUND := 0
    
    $(foreach brew_path,$(BREW_PATHS),\
        $(if $(wildcard $(brew_path)/lib/libsfml-graphics.*),\
            $(eval LDFLAGS += -L$(brew_path)/lib) \
            $(eval CXXFLAGS += -I$(brew_path)/include) \
            $(eval SFML_FOUND := 1) \
            $(info Found SFML in Homebrew at $(brew_path))\
    )
    
    ifeq ($(SFML_FOUND),0)
        $(error SFML not found! Install via: brew install sfml)
    endif
endif

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "copying assets"
	@cp -r $(ASSETS_DIR) $(BIN_DIR)/

$(BIN_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	$(BIN_DIR)/$(TARGET)

.PHONY: all clean run
