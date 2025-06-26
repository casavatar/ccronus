# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 1.2.0
# date: 2025-06-26
# project: Tactical Aim Assist
#
# This Makefile handles the compilation and linking of the C++ project,
# including external dependencies like PortAudio and Aubio.

# --- Compiler and Flags ---
# Define the C++ compiler to be used. g++ is the standard for MinGW.
CXX = g++
# Define compilation flags:
# -std=c++17: Use the C++17 standard for modern features.
# -g: Include debugging information in the executable.
# -Wall -Wextra: Enable all standard and extra warning messages for robust code.
# -O2: Optimization level 2 for performance.
CXXFLAGS = -std=c++17 -g -Wall -Wextra -O2

# --- Directories ---
# Define paths for source, object, and external libraries.
# This makes the Makefile cleaner and easier to modify.
SRCDIR = src
OBJDIR = obj
LIBDIR = lib
INCDIR = include

# External dependency paths
# Assumes 'aubio' and 'portaudio' directories are at the project root.
AUBIO_PATH = aubio-0.4.7
PORTAUDIO_PATH = portaudio

# --- Files and Executable ---
# Name of the final executable file.
TARGET = TacticalAimAssist.exe

# List all C++ source files to be compiled.
# The new audio_system.cpp is included.
SRCS = $(SRCDIR)/main.cpp \
       $(SRCDIR)/globals.cpp \
       $(SRCDIR)/gui.cpp \
       $(SRCDIR)/input.cpp \
       $(SRCDIR)/movements.cpp \
       $(SRCDIR)/profiles.cpp \
       $(SRCDIR)/config.cpp \
       $(SRCDIR)/assist.cpp \
       $(SRCDIR)/systems.cpp \
       $(SRCDIR)/audio_system.cpp

# Generate a list of object file names from the source file names.
# e.g., src/main.cpp becomes obj/main.o
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include and Library Flags ---
# -I flag tells the compiler where to look for header files (#include <...>)
# We include the project's source directory and the include folders for our dependencies.
INC_FLAGS = -I$(SRCDIR) \
            -I$(AUBIO_PATH)/src \
            -I$(PORTAUDIO_PATH)/include

# -L flag tells the linker where to look for library files.
# We point to the build/lib directories of our dependencies.
LIB_FLAGS = -L$(AUBIO_PATH)/build/src \
            -L$(PORTAUDIO_PATH)/lib

# -l flag specifies the names of the libraries to link against.
# The linker will look for files like 'libaubio.a', 'libportaudio.a', etc.
# We also include system libraries required by PortAudio on Windows.
LIBS = -laubio -lportaudio -lwinmm -lole32

# --- Build Rules ---

# The 'all' rule is the default goal. It depends on the final executable.
# Typing 'make' or 'make all' will trigger this rule.
all: $(TARGET)

# Rule to link all the object files into the final executable.
# It depends on all object files being created first.
$(TARGET): $(OBJS)
	@echo "Linking executable..."
	$(CXX) $(OBJS) -o $@ $(LIB_FLAGS) $(LIBS)
	@echo "Build finished: $(TARGET)"

# Pattern rule to compile each .cpp file into a corresponding .o file.
# This is where the one-by-one compilation happens.
# $< is the first prerequisite (the .cpp file).
# $@ is the target (the .o file).
# -c flag tells the compiler to compile without linking.
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR) # Create the object directory if it doesn't exist.
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

# The 'clean' rule removes all generated files (object files and the executable).
# This is useful for a fresh rebuild.
clean:
	@echo "Cleaning up..."
	rm -f $(OBJDIR)/*.o $(TARGET)
	@echo "Cleanup complete."

# Phony targets are not actual files. They are just names for commands.
# This prevents 'make' from getting confused if a file named 'all' or 'clean' exists.
.PHONY: all clean