# BG-Remover Integration Guide

This document explains how to integrate the `bg-remover` CLI into an application or deployment pipeline.

## Overview

`bg-remover` removes image backgrounds and writes a PNG with an alpha channel. Official binaries default to ML segmentation and require `--model <path>`. Use `--grabcut` to opt out to OpenCV GrabCut.

## Prerequisites

- A supported `bg-remover` binary.
- Linux x86_64 or Linux arm64 runtime with glibc >= 2.34.
- For Linux, ONNX Runtime 1.19.2 `libonnxruntime.so.1` matching the binary architecture.
- For macOS development, `brew install opencv onnxruntime pkg-config`.
- Input images in common OpenCV-readable formats such as JPEG, PNG, BMP, or TIFF.

Linux binaries statically bundle OpenCV, libstdc++, and libgcc. They still dynamically link `libonnxruntime.so.1`, resolved via `$ORIGIN:$ORIGIN/lib`; place that file next to the binary or in a sibling `lib/` directory. The ONNX Runtime library is required at process load time even when running `--grabcut`.

## Command-Line Interface

```bash
# Default ML mode
./bg-remover -i <input_path> -o <output_path> --model <model_path>

# GrabCut opt-out mode
./bg-remover -i <input_path> -o <output_path> --grabcut
```

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `-i, --input` | Yes | Input image path, or `-` for stdin | `-i input.jpg` |
| `-o, --output` | Yes | Output PNG path, or `-` for stdout | `-o output.png` |
| `--model` | For ML mode | ONNX model path | `--model u2net.onnx` |
| `--grabcut` | No | Use OpenCV GrabCut instead of ML | `--grabcut` |
| `--ml` | No | No-op in official binaries because ML is already default | `--ml` |
| `-q, --quality` | No | GrabCut preset: `fast`, `balanced`, `quality` | `-q quality` |
| `-n, --iterations` | No | GrabCut iterations, 1-20 | `-n 12` |
| `-m, --margin` | No | GrabCut edge margin/inset in pixels | `-m 20` |
| `-e, --edge-mode` | No | GrabCut edge refinement: `blur`, `bilateral`, `guided` | `-e guided` |

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success; background removed and saved |
| Non-zero | Error occurred, such as missing input, invalid image, missing model, or missing runtime library |

## Integration Examples

### Shell Script

```bash
#!/bin/bash

INPUT_IMAGE="$1"
OUTPUT_IMAGE="$2"
MODEL_PATH="/app/models/u2net.onnx"

./bg-remover -i "$INPUT_IMAGE" -o "$OUTPUT_IMAGE" --model "$MODEL_PATH"

if [ $? -eq 0 ]; then
    echo "Background removed successfully: $OUTPUT_IMAGE"
else
    echo "Error: Failed to remove background from $INPUT_IMAGE"
    exit 1
fi
```

### Python

```python
import os
import subprocess

def remove_background(input_path, output_path, model_path, binary_path="./bg-remover"):
    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input file not found: {input_path}")

    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    subprocess.run([
        binary_path,
        "-i", input_path,
        "-o", output_path,
        "--model", model_path,
    ], check=True)

    return os.path.exists(output_path)
```

### Node.js

```javascript
const { spawn } = require('child_process');

function removeBackground(inputPath, outputPath, modelPath, binaryPath = './bg-remover') {
  return new Promise((resolve, reject) => {
    const child = spawn(binaryPath, [
      '-i', inputPath,
      '-o', outputPath,
      '--model', modelPath,
    ]);

    let stderr = '';
    child.stderr.on('data', data => { stderr += data.toString(); });
    child.on('close', code => {
      code === 0 ? resolve(true) : reject(new Error(`bg-remover failed: ${stderr}`));
    });
    child.on('error', reject);
  });
}
```

### PHP / Laravel

```php
use Illuminate\Support\Facades\Process;

Process::run([
    base_path('bin/bg-remover-linux-arm64'),
    '-i', $inputPath,
    '-o', $outputPath,
    '--model', base_path('models/u2net.onnx'),
])->throw();
```

### Go

```go
cmd := exec.Command(
    "./bg-remover",
    "-i", inputPath,
    "-o", outputPath,
    "--model", modelPath,
)
if output, err := cmd.CombinedOutput(); err != nil {
    return fmt.Errorf("bg-remover failed: %w\n%s", err, output)
}
```

### GrabCut Mode

```bash
./bg-remover -i input.jpg -o output.png --grabcut -q quality -n 12 -e guided
```

GrabCut does not use a model, but official Linux binaries still need `libonnxruntime.so.1` to be present because it is a load-time dependency.

## Docker Container Integration

Use a glibc-based image such as Debian 12 or Ubuntu 22.04+. The official Linux binary does not need OpenCV packages installed.

```dockerfile
FROM debian:12-slim

COPY bg-remover-ubuntu-x86_64 /usr/local/bin/bg-remover
COPY libonnxruntime.so.1 /usr/local/bin/libonnxruntime.so.1
RUN chmod +x /usr/local/bin/bg-remover

WORKDIR /app
ENTRYPOINT ["/usr/local/bin/bg-remover"]
CMD ["-i", "input.jpg", "-o", "output.png", "--model", "models/u2net.onnx"]
```

## Laravel Cloud Deployment

Laravel Cloud workers are arm64 Graviton hosts in a managed runtime, not a user-provided container. Use the official Linux arm64 binary directly.

Ship these files with your application:

- `bg-remover-linux-arm64`
- `libonnxruntime.so.1` from `onnxruntime-linux-aarch64-1.19.2.tgz`, next to the binary or under `lib/`
- Your ONNX model file, if using default ML mode

Run `chmod +x bg-remover-linux-arm64` during deployment, then invoke the binary from your worker process with `--model <path>` or `--grabcut`.

## Laravel Forge And Vapor Notes

- Forge on Ubuntu 22.04+ can run `bg-remover-ubuntu-x86_64` directly with the x64 ONNX Runtime library co-located.
- Vapor container images should use a glibc base if they use the official Linux binaries. Alpine/musl is not an official target in the current build pipeline.
- Laravel Cloud should use the arm64 guidance above and does not require a custom container.

## Batch Processing Example

```bash
#!/bin/bash

INPUT_DIR="./images"
OUTPUT_DIR="./processed"
MODEL_PATH="./models/u2net.onnx"

mkdir -p "$OUTPUT_DIR"

for img in "$INPUT_DIR"/*.{jpg,jpeg,png}; do
    [ -f "$img" ] || continue
    filename=$(basename "$img")
    name="${filename%.*}"

    ./bg-remover -i "$img" -o "$OUTPUT_DIR/${name}_no_bg.png" --model "$MODEL_PATH"
done
```

## Error Handling

| Error | Cause | Solution |
|-------|-------|----------|
| `libonnxruntime.so.1` not found | Missing dynamic ONNX Runtime library | Place the arch-matching library next to the binary or in `./lib/` |
| `GLIBC_2.34 not found` | Runtime glibc is too old | Use Debian 12, Ubuntu 22.04+, or another glibc >= 2.34 environment |
| ML mode requires `--model` | Default ML mode needs an ONNX model | Pass `--model <path>` or use `--grabcut` |
| File not found | Invalid input path | Check that the file exists and the path is correct |
| Cannot write output | Permission denied or invalid output path | Verify write permissions and directory exists |
| Invalid image format | Corrupted or unsupported file | Validate input is a valid image file |

## Output Format

- **Format**: PNG with alpha channel (RGBA)
- **Bit depth**: 8-bit per channel
- **Transparency**: Full alpha channel support (0-255)

## Limitations

1. Default ML mode requires a compatible ONNX segmentation model.
2. Official Linux binaries need `libonnxruntime.so.1` even for `--grabcut`.
3. GrabCut works best with single subjects on contrasting backgrounds.
4. The CLI does not include native batch mode; call it once per image.

## Support

Open binary package issues in [artisan-build/bg-remover](https://github.com/artisan-build/bg-remover).

## License

Refer to the main repository LICENSE file for usage terms.
