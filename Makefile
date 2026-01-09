# BG Remover Makefile
# Supports multiple build targets and linking modes

CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui
OPENCV_FLAGS = `pkg-config --cflags opencv4` $(OPENCV_LIBS)
BINARY = bg-remover
SOURCE = src/bg-remover.cpp

# Default target
TARGET ?= local
LINK_MODE ?= dynamic

# Platform-specific configurations
ifeq ($(TARGET),alpine)
    # Alpine Linux build (musl libc)
    DOCKERFILE = Dockerfile.alpine
    OUTPUT = bg-remover-alpine-x86_64
endif

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
    OUTPUT = $(BINARY)
endif

# Linking mode
ifeq ($(LINK_MODE),static)
    LDFLAGS += -static
endif

# Default build (local)
all: $(OUTPUT)

$(OUTPUT): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(OUTPUT) $(OPENCV_FLAGS) $(LDFLAGS)
	strip $(OUTPUT)

# Docker build targets
build-docker-alpine:
	docker build -f Dockerfile.alpine -t bg-remover-alpine .
	docker create --name bg-remover-extract bg-remover-alpine
	docker cp bg-remover-extract:/app/$(BINARY) ./bg-remover-alpine-x86_64
	docker rm bg-remover-extract
	@echo "✅ Built: bg-remover-alpine-x86_64"

build-docker-ubuntu:
	docker build -f Dockerfile.ubuntu -t bg-remover-ubuntu .
	docker create --name bg-remover-extract bg-remover-ubuntu
	docker cp bg-remover-extract:/app/$(BINARY) ./bg-remover-ubuntu-x86_64
	docker rm bg-remover-extract
	@echo "✅ Built: bg-remover-ubuntu-x86_64"

# Convenience targets
alpine: TARGET = alpine
alpine: DOCKERFILE = Dockerfile.alpine
alpine: build-docker-alpine

ubuntu: TARGET = ubuntu
ubuntu: DOCKERFILE = Dockerfile.ubuntu
ubuntu: build-docker-ubuntu

# Clean
clean:
	rm -f $(BINARY) bg-remover-*

.PHONY: all clean alpine ubuntu build-docker-alpine build-docker-ubuntu
