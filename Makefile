# BG Remover Makefile
# Supports multiple build targets and linking modes

CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall
BINARY = bg-remover
SOURCE = src/bg-remover.cpp

# Default target
TARGET ?= local
LINK_MODE ?= dynamic
ML ?= 0

# Detect platform
UNAME_S := $(shell uname -s)

# Linking mode - static vs dynamic
ifeq ($(LINK_MODE),static)
    # Use static OpenCV libraries via pkg-config --static
    OPENCV_FLAGS = $(shell pkg-config --cflags --libs --static opencv4 2>/dev/null || pkg-config --cflags --libs opencv4)

    ifeq ($(UNAME_S),Linux)
        # Linux: Statically link GCC runtime libraries for maximum portability
        LDFLAGS += -static-libgcc -static-libstdc++
        # Set rpath to find bundled shared libraries (for ONNX Runtime)
        LDFLAGS += -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,'$$ORIGIN/lib'
    endif
    ifeq ($(UNAME_S),Darwin)
        # macOS: clang doesn't support -static-libgcc, use rpath for bundled libs
        LDFLAGS += -Wl,-rpath,@executable_path -Wl,-rpath,@executable_path/lib
    endif
else
    OPENCV_FLAGS = $(shell pkg-config --cflags --libs opencv4)
endif

# ML support (optional)
ifeq ($(ML),1)
    CXXFLAGS += -DWITH_ML
    # Try pkg-config first, fallback to homebrew paths on macOS
    ML_PKG_CONFIG := $(shell pkg-config --exists onnxruntime 2>/dev/null && echo "yes")
    ifeq ($(ML_PKG_CONFIG),yes)
        ML_INCLUDE = $(shell pkg-config --cflags onnxruntime)
        ML_LIB = $(shell pkg-config --libs onnxruntime)
    else
        # Fallback to macOS Homebrew paths (use brew --prefix for dynamic path)
        ONNX_PREFIX := $(shell brew --prefix onnxruntime 2>/dev/null)
        ifneq ($(ONNX_PREFIX),)
            ML_INCLUDE = -I$(ONNX_PREFIX)/include
            ML_LIB = -L$(ONNX_PREFIX)/lib -lonnxruntime
        endif
    endif
endif

# Platform-specific configurations
ifeq ($(TARGET),ubuntu)
    # Ubuntu build (glibc)
    DOCKERFILE = Dockerfile.ubuntu
    OUTPUT = bg-remover-ubuntu-x86_64
endif

ifeq ($(TARGET),macos)
    # macOS build (universal binary)
    CXXFLAGS += -arch arm64 -arch x86_64
    OUTPUT = bg-remover-macos-universal
endif

ifeq ($(TARGET),local)
    OUTPUT ?= $(BINARY)
endif

# Allow OUTPUT to be overridden from command line
ifdef OUTPUT
    OUTPUT := $(OUTPUT)
endif

# Default build (local)
all: $(OUTPUT)

$(OUTPUT): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(ML_INCLUDE) $(SOURCE) -o $(OUTPUT) $(OPENCV_FLAGS) $(ML_LIB) $(LDFLAGS)
	strip $(OUTPUT)

# Docker build targets
build-docker-ubuntu:
	docker build -f Dockerfile.ubuntu -t bg-remover-ubuntu .
	docker create --name bg-remover-extract bg-remover-ubuntu
	docker cp bg-remover-extract:/app/$(BINARY) ./bg-remover-ubuntu-x86_64
	# Also copy bundled ONNX Runtime library if needed
	docker cp bg-remover-extract:/usr/local/lib/libonnxruntime.so.1.19.2 ./libonnxruntime.so.1.19.2 2>/dev/null || true
	docker rm bg-remover-extract
	@echo "✅ Built: bg-remover-ubuntu-x86_64"

# Convenience targets
ubuntu: TARGET = ubuntu
ubuntu: DOCKERFILE = Dockerfile.ubuntu
ubuntu: build-docker-ubuntu

# Clean
clean:
	rm -f $(BINARY) bg-remover-* libonnxruntime.*

.PHONY: all clean ubuntu build-docker-ubuntu
