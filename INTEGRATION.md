# BG-Remover Integration Guide

This document explains how to integrate the bg-remover background removal tool into your application.

## Overview

`bg-remover` is a command-line tool that removes backgrounds from images using OpenCV's GrabCut algorithm. It accepts an input image and outputs a PNG file with a transparent background.

## Prerequisites

- The `bg-remover` binary must be available on your system
- Input images should be in common formats (JPEG, PNG, BMP, TIFF, etc.)
- Sufficient disk space for output files (PNG with alpha channel)

## Command-Line Interface

### Basic Usage

```bash
./bg-remover -i <input_path> -o <output_path>
```

### Parameters

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `-i` | Yes | Path to input image file | `-i input.jpg` |
| `-o` | Yes | Path to output PNG file | `-o output.png` |

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success - background removed and saved |
| Non-zero | Error occurred (file not found, invalid image, etc.) |

## Integration Examples

### 1. Shell Script Integration

```bash
#!/bin/bash

INPUT_IMAGE="$1"
OUTPUT_IMAGE="$2"

# Run bg-remover
./bg-remover -i "$INPUT_IMAGE" -o "$OUTPUT_IMAGE"

if [ $? -eq 0 ]; then
    echo "Background removed successfully: $OUTPUT_IMAGE"
else
    echo "Error: Failed to remove background from $INPUT_IMAGE"
    exit 1
fi
```

### 2. Python Integration

```python
import subprocess
import os
from pathlib import Path

def remove_background(input_path, output_path, bg_remover_binary="./bg-remover"):
    """
    Remove background from an image using bg-remover.

    Args:
        input_path: Path to input image
        output_path: Path to output PNG file
        bg_remover_binary: Path to bg-remover executable

    Returns:
        bool: True if successful, False otherwise
    """
    # Validate input exists
    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input file not found: {input_path}")

    # Ensure output directory exists
    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    # Run bg-remover
    try:
        result = subprocess.run(
            [bg_remover_binary, "-i", input_path, "-o", output_path],
            capture_output=True,
            text=True,
            check=True
        )
        return os.path.exists(output_path)
    except subprocess.CalledProcessError as e:
        print(f"Error: {e.stderr}")
        return False

# Example usage
if __name__ == "__main__":
    success = remove_background("photo.jpg", "photo_no_bg.png")
    if success:
        print("Background removed successfully!")
```

### 3. Node.js Integration

```javascript
const { spawn } = require('child_process');
const fs = require('fs').promises;
const path = require('path');

/**
 * Remove background from an image using bg-remover
 * @param {string} inputPath - Path to input image
 * @param {string} outputPath - Path to output PNG file
 * @param {string} binaryPath - Path to bg-remover binary (default: './bg-remover')
 * @returns {Promise<boolean>} True if successful
 */
async function removeBackground(inputPath, outputPath, binaryPath = './bg-remover') {
    return new Promise((resolve, reject) => {
        // Ensure output directory exists
        const outputDir = path.dirname(outputPath);
        if (outputDir) {
            fs.mkdir(outputDir, { recursive: true }).catch(() => {});
        }

        const process = spawn(binaryPath, ['-i', inputPath, '-o', outputPath]);

        let stderr = '';

        process.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        process.on('close', (code) => {
            if (code === 0) {
                resolve(true);
            } else {
                reject(new Error(`bg-remover failed with code ${code}: ${stderr}`));
            }
        });

        process.on('error', (err) => {
            reject(new Error(`Failed to start bg-remover: ${err.message}`));
        });
    });
}

// Example usage
removeBackground('photo.jpg', 'photo_no_bg.png')
    .then(() => console.log('Background removed successfully!'))
    .catch(err => console.error('Error:', err.message));
```

### 4. PHP Integration

```php
<?php

/**
 * Remove background from an image using bg-remover
 *
 * @param string $inputPath Path to input image
 * @param string $outputPath Path to output PNG file
 * @param string $binaryPath Path to bg-remover binary
 * @return bool True if successful, false otherwise
 */
function removeBackground($inputPath, $outputPath, $binaryPath = './bg-remover') {
    // Validate input
    if (!file_exists($inputPath)) {
        throw new Exception("Input file not found: $inputPath");
    }

    // Ensure output directory exists
    $outputDir = dirname($outputPath);
    if ($outputDir && !is_dir($outputDir)) {
        mkdir($outputDir, 0755, true);
    }

    // Escape arguments for shell safety
    $cmd = sprintf(
        '%s -i %s -o %s',
        escapeshellarg($binaryPath),
        escapeshellarg($inputPath),
        escapeshellarg($outputPath)
    );

    // Execute command
    exec($cmd, $output, $returnCode);

    return $returnCode === 0 && file_exists($outputPath);
}

// Example usage
try {
    $success = removeBackground('photo.jpg', 'photo_no_bg.png');
    if ($success) {
        echo "Background removed successfully!\n";
    } else {
        echo "Failed to remove background\n";
    }
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
```

### 5. Go Integration

```go
package main

import (
    "fmt"
    "os"
    "os/exec"
    "path/filepath"
)

// RemoveBackground removes the background from an image using bg-remover
func RemoveBackground(inputPath, outputPath, binaryPath string) error {
    // Validate input exists
    if _, err := os.Stat(inputPath); os.IsNotExist(err) {
        return fmt.Errorf("input file not found: %s", inputPath)
    }

    // Ensure output directory exists
    outputDir := filepath.Dir(outputPath)
    if outputDir != "" {
        if err := os.MkdirAll(outputDir, 0755); err != nil {
            return fmt.Errorf("failed to create output directory: %w", err)
        }
    }

    // Run bg-remover
    cmd := exec.Command(binaryPath, "-i", inputPath, "-o", outputPath)
    output, err := cmd.CombinedOutput()

    if err != nil {
        return fmt.Errorf("bg-remover failed: %w\nOutput: %s", err, output)
    }

    // Verify output file was created
    if _, err := os.Stat(outputPath); os.IsNotExist(err) {
        return fmt.Errorf("output file not created: %s", outputPath)
    }

    return nil
}

func main() {
    err := RemoveBackground("photo.jpg", "photo_no_bg.png", "./bg-remover")
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error: %v\n", err)
        os.Exit(1)
    }
    fmt.Println("Background removed successfully!")
}
```

### 6. Docker Container Integration

```dockerfile
FROM alpine:3.19

# Copy the bg-remover binary into the container
COPY bg-remover /usr/local/bin/bg-remover
RUN chmod +x /usr/local/bin/bg-remover

# Install OpenCV runtime libraries (not dev packages)
RUN apk add --no-cache opencv libstdc++

# Set working directory
WORKDIR /app

# Default command
ENTRYPOINT ["/usr/local/bin/bg-remover"]
CMD ["-i", "input.jpg", "-o", "output.png"]
```

Run with:
```bash
docker run -v $(pwd):/app your-image -i /app/input.jpg -o /app/output.png
```

### 7. REST API Wrapper (Python Flask Example)

```python
from flask import Flask, request, send_file, jsonify
import subprocess
import os
import uuid
from werkzeug.utils import secure_filename

app = Flask(__name__)
UPLOAD_FOLDER = '/tmp/bg-remover-uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/remove-background', methods=['POST'])
def remove_background_api():
    """
    API endpoint to remove background from uploaded image.

    POST /remove-background
    Form data: image file
    Returns: PNG file with transparent background
    """
    if 'image' not in request.files:
        return jsonify({'error': 'No image provided'}), 400

    file = request.files['image']
    if file.filename == '':
        return jsonify({'error': 'Empty filename'}), 400

    # Generate unique filenames
    file_id = str(uuid.uuid4())
    input_path = os.path.join(UPLOAD_FOLDER, f"{file_id}_input")
    output_path = os.path.join(UPLOAD_FOLDER, f"{file_id}_output.png")

    try:
        # Save uploaded file
        file.save(input_path)

        # Run bg-remover
        result = subprocess.run(
            ['./bg-remover', '-i', input_path, '-o', output_path],
            capture_output=True,
            text=True,
            check=True
        )

        # Return the processed image
        return send_file(output_path, mimetype='image/png')

    except subprocess.CalledProcessError as e:
        return jsonify({'error': 'Background removal failed', 'details': e.stderr}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        # Cleanup
        if os.path.exists(input_path):
            os.remove(input_path)
        if os.path.exists(output_path):
            os.remove(output_path)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

## Performance Considerations

### Processing Time
- Depends on image resolution and complexity
- Typical processing: 1-5 seconds for standard photos (1-5 MP)
- Larger images (10+ MP) may take 10-30 seconds

### Resource Usage
- **Memory**: Proportional to image size (~3-4x the input file size in RAM)
- **CPU**: Single-threaded OpenCV processing
- **Disk**: Output PNG files are typically larger than JPEG inputs due to alpha channel

### Optimization Tips
1. **Resize large images** before processing if high resolution isn't needed
2. **Process in parallel** - bg-remover can handle multiple simultaneous executions
3. **Use temp storage** for intermediate files to avoid cluttering the filesystem
4. **Set timeouts** to handle edge cases where processing takes too long

## Batch Processing Example

```bash
#!/bin/bash
# Process all images in a directory

INPUT_DIR="./images"
OUTPUT_DIR="./processed"

mkdir -p "$OUTPUT_DIR"

for img in "$INPUT_DIR"/*.{jpg,jpeg,png}; do
    [ -f "$img" ] || continue
    filename=$(basename "$img")
    name="${filename%.*}"

    echo "Processing: $filename"
    ./bg-remover -i "$img" -o "$OUTPUT_DIR/${name}_no_bg.png"
done

echo "Batch processing complete!"
```

## Error Handling

Common errors and solutions:

| Error | Cause | Solution |
|-------|-------|----------|
| File not found | Invalid input path | Check file exists and path is correct |
| Cannot write output | Permission denied or invalid output path | Verify write permissions and directory exists |
| Invalid image format | Corrupted or unsupported file | Validate input is a valid image file |
| Segmentation fault | Extremely large image or insufficient memory | Reduce image size or increase available RAM |

## Output Format

- **Format**: PNG with alpha channel (RGBA)
- **Bit depth**: 8-bit per channel
- **Transparency**: Full alpha channel support (0-255)
- **Compression**: PNG default compression

## Limitations

1. **Single subject focus**: Works best with single subjects on contrasting backgrounds
2. **Complex backgrounds**: May struggle with intricate details (hair, fur) against similar-colored backgrounds
3. **Static algorithm**: Uses predefined GrabCut parameters (no runtime tuning)
4. **No batch mode**: Must call binary separately for each image

## Support

For issues or questions about this tool, contact the repository maintainer or open an issue on the project repository.

## License

Refer to the main repository LICENSE file for usage terms.
