# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 6.2.0 (Fully Explicit Linking)
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
# Path to the MinGW system installation, where pacman installs libraries like FFTW.
MINGW_PATH = fftw3

# --- Files and Executable ---
TARGET = TacticalAimAssist.exe
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include Paths ---
# We add the MinGW include path to be explicit.
INC_FLAGS = -I$(SRCDIR) \
            -I$(PORTAUDIO_PATH)/include \
            -I$(MINGW_PATH)/include

# --- Library Paths and Files ---
# Explicit path to the statically compiled PortAudio library.
PORTAUDIO_LIB_FILE = $(PORTAUDIO_PATH)/lib/.libs/libportaudio.a
# Explicit path to the FFTW library installed by pacman.
FFTW_LIB_FILE = $(MINGW_PATH)/lib/libfftw3f.a

# System libraries required for PortAudio and Windows GUI functions.
SYSTEM_LIBS = -ldsound -lwinmm -lole32 -luuid -lksguid -lgdi32

# --- Build Rules ---
all: $(TARGET)

# The linker command now uses explicit paths for all major external libraries.
$(TARGET): $(OBJS)
	@echo "Linking executable with fully explicit library paths..."
	$(CXX) $(OBJS) -o $@ $(PORTAUDIO_LIB_FILE) $(FFTW_LIB_FILE) $(SYSTEM_LIBS) $(LDFLAGS)
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