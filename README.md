# bg-remover

A self-hosted background removal tool using OpenCV's GrabCut algorithm. This C++ implementation works without Python dependencies and supports Alpine, Ubuntu, and macOS platforms.

## Overview

bg-remover is a command-line tool that removes backgrounds from images, creating transparent PNG outputs. It's designed as a self-hosted alternative to remove.bg that runs entirely in your own environment.

## Features

- Pure C++ implementation with OpenCV
- No Python dependencies required
- Multi-platform support:
  - **Alpine Linux (x86_64)** - for Laravel Vapor
  - **Ubuntu (x86_64)** - for Laravel Forge
  - **macOS (Universal)** - ARM64 + x86_64 for development
- GrabCut algorithm for accurate background segmentation
- Outputs PNG with alpha channel transparency
- Docker-based build system for reproducibility

## Quick Start

### Installation

**Option 1: Download Pre-built Binary**

Download the appropriate binary for your platform from the [latest release](https://github.com/artisan-build/bg-remover/releases/latest):
- `bg-remover-alpine-x86_64` - for Alpine Linux (Laravel Vapor)
- `bg-remover-ubuntu-x86_64` - for Ubuntu (Laravel Forge)
- `bg-remover-macos-universal` - for macOS (ARM64 + Intel)

```bash
# Make it executable
chmod +x bg-remover-*

# Run it
./bg-remover-alpine-x86_64 -i input.jpg -o output.png
```

**Option 2: Build from Source**

```bash
# Clone repository
git clone https://github.com/artisan-build/bg-remover.git
cd bg-remover

# Build for your platform
make alpine    # Alpine Linux via Docker
make ubuntu    # Ubuntu via Docker
make           # Local build (requires OpenCV installed)
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
| `-h, --help` | Show help message | `-h` |

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

| Platform | Use Case | Build Command |
|----------|----------|---------------|
| Alpine Linux | Laravel Vapor | `make alpine` |
| Ubuntu | Laravel Forge | `make ubuntu` |
| macOS | Development | `make` (requires OpenCV via Homebrew) |

### Build Requirements

**For Docker builds (Alpine/Ubuntu):**
- Docker installed
- No other dependencies needed

**For local macOS build:**
```bash
brew install opencv
make
```

### Build Options

```bash
# Build for specific target
make TARGET=alpine      # Alpine Linux
make TARGET=ubuntu      # Ubuntu
make TARGET=local       # Local system

# Build with static linking (if supported)
make TARGET=alpine LINK_MODE=static

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
  - Used for image processing and background removal algorithms
  - Must be installed separately (see Requirements section)
  - License: https://opencv.org/license/

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

OpenCV is licensed under the Apache 2.0 License, which is compatible with the MIT License used for this project.

## Repository Structure

- `src/` - C++ source code
- `Dockerfile.alpine` - Alpine Linux build configuration
- `Dockerfile.ubuntu` - Ubuntu build configuration
- `Makefile` - Build system with multi-platform support
- `INTEGRATION.md` - Comprehensive integration guide
- `FORKING.md` - Guide for community platform support

## Contributing

- Binary issues/features: Open in this repository
- Laravel package issues: Open in [artisan-build/scalpels](https://github.com/artisan-build/scalpels)
- Community platform builds: See [FORKING.md](FORKING.md)
