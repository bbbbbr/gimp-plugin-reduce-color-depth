# Predefined constants
CC      = gcc
TARGET  = plugin-gimp-reduce-colordepth
SRC_DIR = src
OBJ_DIR = obj

# -Wno-deprecated, -Wno-deprecated-declarations is just for gtk warnings for now (migration to GEGL)
CFLAGS  = $(shell pkg-config --cflags gtk+-2.0) \
          $(shell pkg-config --cflags gimp-2.0) \
          -Wno-format-truncation \
          -Wno-deprecated \
          -Wno-deprecated-declarations
LFLAGS  = $(shell pkg-config --libs glib-2.0) \
          $(shell pkg-config --libs gtk+-2.0) \
          $(shell pkg-config --libs gimp-2.0) \
          $(shell pkg-config --libs gimpui-2.0)

# -std=gnu11 is a fix for aligned_alloc() return assignment warning and memory crashes with gcc versions lower than 5
GCC_VERSION := $(shell gcc -dumpversion)
MIN_VERSION := 5.0.0

ifeq ($(MIN_VERSION),$(firstword $(sort $(GCC_VERSION) $(MIN_VERSION))))
$(info $$GCC_VERSION [${GCC_VERSION}] is OK)
else
$(info $$GCC_VERSION [${GCC_VERSION}] is older than [${MIN_VERSION}], adding -std=gnu11 for aligned_alloc)
CFLAGS += -std=gnu11
endif

# File definitions
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ_DIR) $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LFLAGS)
	strip $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR):
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	rm $(TARGET)

updatebin:
	mkdir -p bin/linux/
	cp $(TARGET) bin/linux/

install:
# $(DESTDIR)$(exec_prefix)/lib/gimp/2.0/plug-ins
	mkdir -p ~/.config/GIMP/2.10/plug-ins
	cp $(TARGET) ~/.config/GIMP/2.10/plug-ins

uninstall:
	rm ~/.config/GIMP/2.10/plug-ins/$(TARGET)

.PHONY: clean install uninstall
