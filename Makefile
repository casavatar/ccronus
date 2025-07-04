# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 1.6.1 (Final Linker Fix for DirectSound)
# date: 2025-06-26
# project: Tactical Aim Assist

# --- Compiler and Flags ---
CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Wextra -O2
LDFLAGS = -mwindows

# --- Directories and Paths ---
SRCDIR = src
OBJDIR = obj
AUBIO_PATH = aubio
PORTAUDIO_PATH = portaudio

# --- Files and Executable ---
TARGET = TacticalAimAssist.exe
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include Paths ---
INC_FLAGS = -I$(SRCDIR) \
            -I$(AUBIO_PATH)/src \
            -I$(PORTAUDIO_PATH)/include

# --- Library Search Paths ---
LIB_PATHS = -L$(AUBIO_PATH)/build/src \
            -L$(PORTAUDIO_PATH)/lib/.libs

# --- Libraries to Link ---
# FIX: Added -ldsound for the DirectSound dependency required by PortAudio.
LIBS = -laubio -lportaudio -ldsound -lwinmm -lole32 -luuid -lksguid -lgdi32

# --- Build Rules ---
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking executable with all system dependencies..."
	$(CXX) $(OBJS) -o $@ $(LIB_PATHS) $(LIBS) $(LDFLAGS)
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