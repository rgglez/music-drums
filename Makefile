CXX ?= g++
PKG_CONFIG ?= pkg-config

APP_NAME := DrumBox
BUILD_DIR := build
BIN_DIR := bin
APP := $(BIN_DIR)/$(APP_NAME)

SRC := main.cpp mainwindow.cpp drumengine.cpp
MOC_HEADERS := mainwindow.h drumengine.h

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC:.cpp=.o))
MOC_SRC := $(addprefix $(BUILD_DIR)/moc_,$(MOC_HEADERS:.h=.cpp))
MOC_OBJ := $(MOC_SRC:.cpp=.o)
DEP := $(OBJ:.o=.d) $(MOC_OBJ:.o=.d)

QT_PW_PACKAGES := Qt6Core Qt6Gui Qt6Widgets libpipewire-0.3
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(QT_PW_PACKAGES))
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(QT_PW_PACKAGES))

MOC ?= $(shell \
	if command -v moc-qt6 >/dev/null 2>&1; then \
		command -v moc-qt6; \
	elif command -v moc6 >/dev/null 2>&1; then \
		command -v moc6; \
	elif command -v qtpaths6 >/dev/null 2>&1 && [ -x "$$(qtpaths6 --query QT_INSTALL_LIBEXECS 2>/dev/null)/moc" ]; then \
		echo "$$(qtpaths6 --query QT_INSTALL_LIBEXECS 2>/dev/null)/moc"; \
	else \
		command -v moc 2>/dev/null; \
	fi)
ifeq ($(strip $(MOC)),)
$(error Could not find Qt moc tool (moc-qt6 or moc) in PATH)
endif

CPPFLAGS += -MMD -MP
CXXFLAGS += -std=c++17 -O2

.PHONY: all run clean distclean

all: $(APP)

$(APP): $(OBJ) $(MOC_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(PKG_LIBS)

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PKG_CFLAGS) -c $< -o $@

$(BUILD_DIR)/moc_%.cpp: %.h | $(BUILD_DIR)
	$(MOC) $(PKG_CFLAGS) $< -o $@

$(BUILD_DIR)/moc_%.o: $(BUILD_DIR)/moc_%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PKG_CFLAGS) -c $< -o $@

run: $(APP)
	$(APP)

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -f $(APP)
	rmdir --ignore-fail-on-non-empty $(BIN_DIR)

-include $(DEP)
