# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 6.0.0 (Final Static/Explicit Linking)
# date: 2025-07-04
# project: Tactical Aim Assist

# --- Compiler and Flags ---
CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Wextra -O2
LDFLAGS = -mwindows -static

# --- Directories and Paths ---
SRCDIR = src
OBJDIR = obj
PORTAUDIO_PATH = portaudio

# --- Files and Executable ---
TARGET = TacticalAimAssist.exe
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include Paths ---
INC_FLAGS = -I$(SRCDIR) \
            -I$(PORTAUDIO_PATH)/include

# --- Library Paths and Files ---
# Explicit path to the statically compiled PortAudio library.
PORTAUDIO_LIB_FILE = $(PORTAUDIO_PATH)/lib/.libs/libportaudio.a

# System libraries required for PortAudio, FFTW, and Windows GUI functions.
# The linker will find these in the MinGW system path.
SYSTEM_LIBS = -lfftw3f -ldsound -lwinmm -lole32 -luuid -lksguid -lgdi32

# --- Build Rules ---
all: $(TARGET)

# The linker command now uses the explicit path to the static portaudio library.
# The object files come first, then our specific libraries, then system libraries.
$(TARGET): $(OBJS)
	@echo "Linking executable with static PortAudio library..."
	$(CXX) $(OBJS) -o $@ $(PORTAUDIO_LIB_FILE) $(SYSTEM_LIBS) $(LDFLAGS)
	@echo "Build finished: $(TARGET)"

# Compile each source file into an object file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

# Clean up generated files
clean:
	@echo "Cleaning up..."
	rm -f $(OBJDIR)/*.o $(TARGET)
	@echo "Cleanup complete."

.PHONY: all clean