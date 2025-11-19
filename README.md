# bg-remover

A self-hosted background removal tool using OpenCV's GrabCut algorithm. This is a C++ implementation that works without Python dependencies and supports both x86_64 and ARM64 architectures.

## Overview

bg-remover is a command-line tool that removes backgrounds from images, creating transparent PNG outputs. It's designed as a self-hosted alternative to remove.bg that runs entirely in your own environment.

## Features

- Pure C++ implementation with OpenCV
- No Python dependencies required
- Support for x86_64 and ARM64 architectures
- GrabCut algorithm for accurate background segmentation
- Outputs PNG with alpha channel transparency
- Docker-based build system for reproducibility

## Building

### Build with Docker (Recommended)

```bash
chmod +x build-static-cpp.sh
./build-static-cpp.sh
```

This creates a statically-linked binary that works across different systems.

### Build Locally

Requires OpenCV development libraries installed on your system.

```bash
make
```

## Usage

### Basic Command

```bash
./bg-remover -i input.jpg -o output.png
```

### Parameters

- `-i` - Input image path (required)
- `-o` - Output PNG path (required)

### Example

```bash
./bg-remover -i photo.jpg -o photo_transparent.png
```

## Integration

To integrate bg-remover into your application:

- **Quick Start**: See [QUICK_START.md](QUICK_START.md) for installation and quick integration examples
- **Full Documentation**: See [INTEGRATION.md](INTEGRATION.md) for complete integration guide with examples in Python, Node.js, PHP, Go, Docker, and more

### Quick Integration Example (Python)

```python
import subprocess
subprocess.run(["./bg-remover", "-i", "input.jpg", "-o", "output.png"], check=True)
```

## Technical Details

- **Algorithm**: OpenCV GrabCut for foreground/background segmentation
- **Processing**: Morphological operations (CLOSE, OPEN) for mask refinement
- **Edge Smoothing**: Gaussian blur for natural alpha channel transitions
- **Output Format**: PNG with RGBA (8-bit per channel)

## Requirements

- OpenCV runtime libraries (for local builds)
- Standard image formats supported: JPEG, PNG, BMP, TIFF, etc.

## Performance

- Processing time: 1-5 seconds for typical photos (1-5 MP)
- Memory usage: ~3-4x input file size in RAM
- Best results with single subjects on contrasting backgrounds

## License

See LICENSE file for details.