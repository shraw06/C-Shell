############## LLM Generated Code Begins ##############

# === Project layout (root == shell directory) ===
# .
# ├── Makefile           <-- this file
# ├── src/               <-- .c files
# └── include/           <-- .h files
#
# Run:  make all      # builds ./shell.out
#       make clean    # removes build artifacts
#       make run      # runs ./shell.out

# Compiler & flags (POSIX/C99 as requested)
CC       := gcc
CPPFLAGS := -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
CSTD     := -std=c99
CWARN    := -Wall -Wextra -Werror -Wno-unused-parameter
CSEC     := -fno-asm
CFLAGS   := $(CSTD) $(CWARN) $(CSEC)
LDFLAGS  :=
LDLIBS   :=

# Dirs & target
SRCDIR   := src
INCDIR   := include
OBJDIR   := build
BINDIR   := .
TARGET   := $(BINDIR)/shell.out

# Sources/Objects
SOURCES  := $(wildcard $(SRCDIR)/*.c)
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

# Auto-deps
DEPFLAGS := -MMD -MP
DEPS     := $(OBJECTS:.o=.d)

# Default rule
.PHONY: all
all: $(TARGET)

# Link step -> produce shell.out in the shell directory
$(TARGET): $(OBJDIR) $(OBJECTS)
	$(CC) $(CPPFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

# Compile step -> objects into build/
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(INCDIR) $(DEPFLAGS) -c $< -o $@

# Ensure build dir exists
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Convenience targets
.PHONY: clean run
clean:
	rm -rf $(OBJDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Include auto-generated dependency files
-include $(DEPS)

############## LLM Generated Code Ends ################
