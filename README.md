# bg-remover

A self-hosted background removal tool using OpenCV's GrabCut algorithm with optional ML-powered segmentation. This C++ implementation works without Python dependencies and produces portable, statically-linked binaries.

## Overview

bg-remover is a command-line tool that removes backgrounds from images, creating transparent PNG outputs. It's designed as a self-hosted alternative to remove.bg that runs entirely in your own environment.

## Features

- Pure C++ implementation with OpenCV
- No Python dependencies required
- **Statically linked binaries** - no need to install OpenCV on target systems
- Multi-platform support:
  - **Ubuntu (x86_64)** - for Laravel Forge/Vapor
  - **macOS (ARM64)** - for Apple Silicon development
- Two background removal modes:
  - **GrabCut** (default) - Fast, works offline, no additional files needed
  - **ML mode** - Deep learning models (U2-Net, RMBG) for 80-95% better quality on complex images
- Outputs PNG with alpha channel transparency
- Docker-based build system for reproducibility

## Quick Start

### Installation

**Option 1: Download Pre-built Binary**

Download the appropriate binary for your platform from the [latest release](https://github.com/artisan-build/bg-remover/releases/latest):

| Platform | Binary | Archive | Notes |
|----------|--------|---------|-------|
| Ubuntu/Linux | `bg-remover-ubuntu-x86_64` | `.tar.gz` | Statically linked, portable |
| macOS | `bg-remover-macos-arm64` | `.tar.gz` | Apple Silicon |

**For basic usage (GrabCut mode):** Download just the binary.

**For ML mode:** Download the `.tar.gz` archive which includes the bundled ONNX Runtime library.

```bash
# Basic usage - download binary only
wget https://github.com/artisan-build/bg-remover/releases/latest/download/bg-remover-ubuntu-x86_64
chmod +x bg-remover-ubuntu-x86_64
./bg-remover-ubuntu-x86_64 -i input.jpg -o output.png

# For ML mode - download and extract archive
wget https://github.com/artisan-build/bg-remover/releases/latest/download/bg-remover-ubuntu-x86_64.tar.gz
tar -xzf bg-remover-ubuntu-x86_64.tar.gz
./bg-remover-ubuntu-x86_64 -i input.jpg -o output.png --ml --model /path/to/u2net.onnx
```

**Option 2: Build from Source**

```bash
# Clone repository
git clone https://github.com/artisan-build/bg-remover.git
cd bg-remover

# Build for your platform
make ubuntu    # Ubuntu via Docker (statically linked)
make           # Local build (requires OpenCV installed)
make ML=1      # Local build with ML support (requires OpenCV + ONNX Runtime)
```

### Basic Usage

```bash
./bg-remover -i input.jpg -o output.png
```

That's it! Input goes in, transparent background PNG comes out.

### Command-Line Options

| Option | Description | Example |
|--------|-------------|---------|
| `-i, --input` | Input image path | `-i photo.jpg` |
| `-o, --output` | Output PNG path | `-o result.png` |
| `--ml` | Use ML model instead of GrabCut | `--ml` |
| `--model` | Path to ONNX model file | `--model u2net.onnx` |
| `-h, --help` | Show help message | `-h` |

### ML Mode

For significantly better results on complex images (hair, transparent objects, complex backgrounds), use ML mode with a deep learning model:

```bash
# Download a model (e.g., U2-Net or RMBG)
wget https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2net.onnx

# Run with ML mode
./bg-remover -i input.jpg -o output.png --ml --model u2net.onnx
```

**Note:** ML mode requires the ONNX Runtime library. When using pre-built binaries, download the `.tar.gz` archive and keep the bundled `libonnxruntime.so` (Linux) or `libonnxruntime.dylib` (macOS) in the same directory as the binary.

## Integration

### Quick Examples

**Python**
```python
import subprocess
subprocess.run(["./bg-remover", "-i", "input.jpg", "-o", "output.png"], check=True)
```

**Node.js**
```javascript
const { execSync } = require('child_process');
execSync('./bg-remover -i input.jpg -o output.png');
```

**PHP (Laravel)**
```php
use Illuminate\Support\Facades\Process;

Process::run(['./bg-remover', '-i', 'input.jpg', '-o', 'output.png'])->throw();
```

**Go**
```go
cmd := exec.Command("./bg-remover", "-i", "input.jpg", "-o", "output.png")
err := cmd.Run()
```

### Laravel Package

For Laravel applications, use the official package:

```bash
composer require artisan-build/background-removal
```

See [artisan-build/background-removal](https://github.com/artisan-build/background-removal) for full Laravel integration documentation.

### Full Integration Guide

See [INTEGRATION.md](INTEGRATION.md) for:
- Complete examples in Python, Node.js, PHP, Go, and more
- REST API wrapper patterns
- Error handling strategies
- Batch processing examples
- Performance optimization tips

## Building

### Officially Supported Platforms

| Platform | Use Case | Build Command | Linking |
|----------|----------|---------------|---------|
| Ubuntu | Laravel Forge/Vapor | `make ubuntu` | Static |
| macOS | Development | `make ML=1` | Dynamic + bundled libs |

### Build Requirements

**For Docker builds (Ubuntu):**
- Docker installed
- No other dependencies needed

**For local macOS build:**
```bash
brew install opencv onnxruntime
make ML=1
```

### Build Options

```bash
# Build for specific target
make TARGET=ubuntu      # Ubuntu (via Docker, statically linked)
make TARGET=local       # Local system

# Build with ML support
make ML=1               # Enable ONNX Runtime ML models

# Build with static linking
make LINK_MODE=static   # Static linking (for portable binaries)

# Combine options
make ML=1 LINK_MODE=static OUTPUT=my-binary

# Clean build artifacts
make clean
```

## Community-Maintained Platform Builds

Need bg-remover on Windows, ARM Linux, or another platform we don't officially support?

See [FORKING.md](FORKING.md) for a complete guide on creating and maintaining your own platform builds. We maintain builds only for platforms we actively use (Alpine, Ubuntu, macOS), but the community is welcome to fork for additional platforms.

### How It Works

1. Fork this repository
2. Create a Dockerfile for your platform
3. Test and release binaries
4. Submit a PR to add your fork to the list below

### Community Forks

_No community forks yet. Be the first! See [FORKING.md](FORKING.md) to get started._

<!-- 
When adding forks, use this format:

| Platform | Maintainer | Repository | Status |
|----------|------------|------------|--------|
| Windows (x86_64) | [@username](https://github.com/username) | [username/bg-remover](https://github.com/username/bg-remover) | Active |
-->

## Technical Details

- **Algorithm**: OpenCV GrabCut for foreground/background segmentation
- **Processing**: Morphological operations (CLOSE, OPEN) for mask refinement
- **Edge Smoothing**: Gaussian blur for natural alpha channel transitions
- **Output Format**: PNG with RGBA (8-bit per channel)

## Performance

- **Processing Time**: 1-5 seconds for typical photos (1-5 MP)
- **Memory Usage**: ~3-4x input file size in RAM
- **Best Results**: Single subjects on contrasting backgrounds

## Troubleshooting

**Problem**: `bg-remover: command not found`
- **Solution**: Use `./bg-remover` or add to PATH

**Problem**: Output file not created
- **Solution**: Ensure output directory exists and you have write permissions

**Problem**: Poor background removal quality
- **Solution**: Works best with clear subject-background contrast. Try images with simpler backgrounds.

## Dependencies

This software uses the following open source packages:

- [OpenCV](https://opencv.org/) - Apache 2.0 License
  - Used for image processing and GrabCut algorithm
  - Statically linked in release binaries (no installation required)
  - License: https://opencv.org/license/

- [ONNX Runtime](https://onnxruntime.ai/) - MIT License (optional, for ML mode)
  - Used for running deep learning models
  - Bundled with `.tar.gz` release archives
  - License: https://github.com/microsoft/onnxruntime/blob/main/LICENSE

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

OpenCV is licensed under the Apache 2.0 License, which is compatible with the MIT License used for this project.

## Repository Structure

- `src/` - C++ source code
- `Dockerfile.ubuntu` - Ubuntu build configuration (static linking)
- `Makefile` - Build system with multi-platform support
- `INTEGRATION.md` - Comprehensive integration guide
- `FORKING.md` - Guide for community platform support

## Contributing

- Binary issues/features: Open in this repository
- Laravel package issues: Open in [artisan-build/scalpels.app](https://github.com/artisan-build/scalpels.app)
- Community platform builds: See [FORKING.md](FORKING.md)
