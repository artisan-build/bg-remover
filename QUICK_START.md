# BG-Remover Quick Start Guide

## TL;DR

```bash
./bg-remover -i input.jpg -o output.png
```

That's it! Input goes in, transparent background PNG comes out.

## Installation

### Option 1: Use Pre-built Binary
1. Download the `bg-remover` binary for your platform
2. Make it executable: `chmod +x bg-remover`
3. Run: `./bg-remover -i input.jpg -o output.png`

### Option 2: Build from Source

```bash
# Clone repository
git clone <repository-url>
cd bg-remover

# Build using Docker (recommended)
chmod +x build-static-cpp.sh
./build-static-cpp.sh

# Or build locally (requires OpenCV dev libraries)
make
```

## Basic Usage

### Remove Background from Single Image

```bash
./bg-remover -i photo.jpg -o photo_transparent.png
```

### Process Multiple Images

```bash
for img in *.jpg; do
    ./bg-remover -i "$img" -o "processed_${img%.jpg}.png"
done
```

## Integration Snippets

### Python (subprocess)
```python
import subprocess
subprocess.run(["./bg-remover", "-i", "input.jpg", "-o", "output.png"], check=True)
```

### Node.js
```javascript
const { execSync } = require('child_process');
execSync('./bg-remover -i input.jpg -o output.png');
```

### PHP
```php
exec('./bg-remover -i input.jpg -o output.png', $output, $returnCode);
```

### Go
```go
cmd := exec.Command("./bg-remover", "-i", "input.jpg", "-o", "output.png")
cmd.Run()
```

## Key Points

- **Input**: Any common image format (JPEG, PNG, BMP, TIFF, etc.)
- **Output**: Always PNG with alpha channel transparency
- **Processing Time**: 1-5 seconds for typical photos
- **Memory Usage**: ~3-4x the input file size
- **Best Results**: Single subject on contrasting background

## Common Issues

**Problem**: `bg-remover: command not found`
- **Solution**: Use `./bg-remover` or add to PATH

**Problem**: Output file not created
- **Solution**: Ensure output directory exists and you have write permissions

**Problem**: Poor background removal quality
- **Solution**: Works best with clear subject-background contrast. Try images with simpler backgrounds.

## Full Documentation

See [INTEGRATION.md](INTEGRATION.md) for:
- Detailed integration examples (Python, Node.js, PHP, Go, Docker)
- REST API wrapper example
- Performance optimization tips
- Error handling strategies
- Batch processing patterns

## Support

- Report issues on the GitHub repository
- Check [README.md](README.md) for build instructions
- See [INTEGRATION.md](INTEGRATION.md) for integration details
