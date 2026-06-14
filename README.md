# bg-remover

A self-hosted C++ background removal CLI that outputs PNG files with an alpha channel. The official binaries support ML segmentation by default, with OpenCV GrabCut available as an opt-out mode.

## Overview

`bg-remover` is designed as a standalone binary package for applications that need background removal without Python dependencies. Official Linux builds statically bundle OpenCV, libstdc++, and libgcc; the only non-system runtime library you must provide is the platform-matching ONNX Runtime shared library.

## Supported Targets

| Target | Binary | Runtime Notes |
|--------|--------|---------------|
| Linux x86_64 | `bg-remover-ubuntu-x86_64` | glibc >= 2.34; static OpenCV; requires `libonnxruntime.so.1` from ONNX Runtime linux-x64 1.19.2 |
| Linux arm64 | `bg-remover-linux-arm64` | glibc >= 2.34; static OpenCV; requires `libonnxruntime.so.1` from ONNX Runtime linux-aarch64 1.19.2 |
| macOS arm64 | `bg-remover-macos-arm64` | Development-only; dynamically links Homebrew OpenCV and ONNX Runtime |

There is no official Windows build. Alpine/musl and universal macOS binaries are not produced by the current release workflow.

## Runtime Contract

- Linux binaries are built with static OpenCV, static libstdc++, and static libgcc.
- Linux binaries dynamically load only `libonnxruntime.so.1` plus glibc system libraries.
- The Linux runtime search path is `$ORIGIN:$ORIGIN/lib`, so `libonnxruntime.so.1` must be next to the binary or in a sibling `lib/` directory.
- `libonnxruntime.so.1` is required at process load time, even when you pass `--grabcut`.
- The Linux glibc floor is `GLIBC_2.34`, so deploy on Debian 12, Ubuntu 22.04+, or another glibc >= 2.34 runtime.
- Official Linux binaries are tens of MB because OpenCV is statically linked. Static builds are intentionally larger than dynamically linked builds.
- macOS binaries are not self-contained and are intended for development machines with `brew install opencv onnxruntime`.

## Quick Start

Download the binary for your platform from the [latest release](https://github.com/artisan-build/bg-remover/releases/latest). Then download the matching ONNX Runtime package from [microsoft/onnxruntime](https://github.com/microsoft/onnxruntime/releases/tag/v1.19.2) and place its shared library with the binary.

```bash
# Linux x86_64 example
wget https://github.com/artisan-build/bg-remover/releases/latest/download/bg-remover-ubuntu-x86_64
mkdir -p lib

# Extract libonnxruntime.so.1 from onnxruntime-linux-x64-1.19.2.tgz into ./lib/
chmod +x bg-remover-ubuntu-x86_64

# ML is the default and requires --model
./bg-remover-ubuntu-x86_64 -i input.jpg -o output.png --model /path/to/u2net.onnx

# GrabCut is the opt-out mode and does not use a model
./bg-remover-ubuntu-x86_64 -i input.jpg -o output.png --grabcut
```

## Command-Line Interface

| Option | Description | Example |
|--------|-------------|---------|
| `-i, --input` | Input image path, or `-` for stdin | `-i photo.jpg` |
| `-o, --output` | Output PNG path, or `-` for stdout | `-o result.png` |
| `-q, --quality` | GrabCut quality preset: `fast`, `balanced`, `quality` | `-q quality` |
| `-n, --iterations` | GrabCut iterations, 1-20 | `-n 12` |
| `-m, --margin` | GrabCut edge margin/inset in pixels | `-m 20` |
| `-e, --edge-mode` | GrabCut edge refinement: `blur`, `bilateral`, `guided` | `-e guided` |
| `--model` | ONNX model path for default ML mode | `--model u2net.onnx` |
| `--grabcut` | Use OpenCV GrabCut instead of ML | `--grabcut` |
| `--ml` | No-op for official binaries because ML is already default | `--ml` |
| `-h, --help` | Show help message | `-h` |

The output is always a PNG with an alpha channel.

## Integration

### Python

```python
import subprocess

subprocess.run([
    "./bg-remover-ubuntu-x86_64",
    "-i", "input.jpg",
    "-o", "output.png",
    "--model", "/path/to/u2net.onnx",
], check=True)
```

### Node.js

```javascript
const { execFileSync } = require('child_process');

execFileSync('./bg-remover-ubuntu-x86_64', [
  '-i', 'input.jpg',
  '-o', 'output.png',
  '--grabcut',
]);
```

### PHP / Laravel

```php
use Illuminate\Support\Facades\Process;

Process::run([
    './bg-remover-linux-arm64',
    '-i', 'input.jpg',
    '-o', 'output.png',
    '--model', base_path('models/u2net.onnx'),
])->throw();
```

### Laravel Package

This repository is the canonical binary package: [artisan-build/bg-remover](https://github.com/artisan-build/bg-remover).

TODO: The canonical Laravel wrapper package name needs confirmation. Existing historical references used multiple names, so this README intentionally avoids naming a wrapper package until it is verified.

See [INTEGRATION.md](INTEGRATION.md) for deployment and process-wrapper examples.

## Laravel Cloud

Laravel Cloud workers run on arm64 Graviton hosts in a managed runtime, not inside a user-provided container. Use the self-contained Linux arm64 binary directly:

1. Ship `bg-remover-linux-arm64` with your application.
2. Ship ONNX Runtime 1.19.2 `libonnxruntime.so.1` from `onnxruntime-linux-aarch64-1.19.2.tgz` next to the binary or under `lib/`.
3. Mark the binary executable during deployment.
4. Run with `--model <path>` for ML mode, or `--grabcut` to opt out to GrabCut.

This is distinct from Forge/Vapor guidance: Laravel Cloud does not require you to build or run a container to use the binary.

## Building

The release workflow builds the three official targets as follows:

| Target | Build Path | Linkage |
|--------|------------|---------|
| Linux x86_64 | `Dockerfile.ubuntu` on Debian 12 | OpenCV/libstdc++/libgcc static; ONNX Runtime dynamic |
| Linux arm64 | `Dockerfile.ubuntu-arm64` on Debian 12 arm64 | OpenCV/libstdc++/libgcc static; ONNX Runtime dynamic |
| macOS arm64 | Native GitHub Actions runner with Homebrew | OpenCV and ONNX Runtime dynamic |

The Dockerfiles build OpenCV 4.10.0 from source as static libraries and link with ONNX Runtime 1.19.2. They copy `libonnxruntime.so.1` into `/app/lib` for verification, but release consumers should fetch the architecture-matching ONNX Runtime package themselves.

Local build commands:

```bash
make ubuntu       # Docker x86_64 Linux build
make linux-arm64  # Docker arm64 Linux build

# macOS development build
brew install opencv onnxruntime pkg-config
make ML=1 TARGET=local OUTPUT=bg-remover-macos-arm64 CXXFLAGS="-std=c++17 -O3 -Wall"
```

The historical `TARGET=alpine` and `Dockerfile.alpine` paths are not part of the current build pipeline.

## Community-Maintained Platform Builds

Need bg-remover on Windows, Alpine/musl, Linux ARM32, RISC-V, BSD, or another unsupported target? See [FORKING.md](FORKING.md) for guidance on maintaining a fork.

## Technical Details

- **Default mode**: ML segmentation with an ONNX model supplied by `--model`.
- **GrabCut mode**: OpenCV GrabCut via `--grabcut`; supports quality, iteration, margin, and edge-mode flags.
- **Output format**: PNG with RGBA, 8-bit per channel.
- **Model runtime**: ONNX Runtime 1.19.2.

## Performance

- **Processing time**: Depends on model, image resolution, and GrabCut settings.
- **Memory usage**: Typically several times the input image size plus model/runtime memory.
- **Binary size**: Linux release binaries are tens of MB because OpenCV is statically linked.

## Troubleshooting

**Problem**: `error while loading shared libraries: libonnxruntime.so.1`

**Solution**: Place the architecture-matching `libonnxruntime.so.1` next to the binary or in `./lib/`. This is required even for `--grabcut` because the official Linux binaries link ONNX Runtime at load time.

**Problem**: `GLIBC_2.34 not found`

**Solution**: Run on Debian 12, Ubuntu 22.04+, or another glibc >= 2.34 environment.

**Problem**: Default invocation fails with `ML mode (default) requires --model <path>`

**Solution**: Supply `--model /path/to/model.onnx`, or pass `--grabcut` to use the non-ML path.

**Problem**: Poor GrabCut output quality

**Solution**: Try `-q quality`, increase `-n`, or use ML mode with a suitable segmentation model.

## Dependencies And Licenses

- [OpenCV](https://opencv.org/) - Apache 2.0 License; statically linked into official Linux binaries and dynamically linked on macOS.
- [ONNX Runtime](https://onnxruntime.ai/) - MIT License; `libonnxruntime.so.1` is dynamically loaded on Linux and must match the target architecture.

This project is licensed under the MIT License. See [LICENSE](LICENSE).

## Repository Structure

- `src/` - C++ source code
- `Dockerfile.ubuntu` - Linux x86_64 static-OpenCV build
- `Dockerfile.ubuntu-arm64` - Linux arm64 static-OpenCV build
- `Makefile` - Local and Docker build helpers
- `.github/workflows/build.yml` - Official release build matrix
- `INTEGRATION.md` - Integration and deployment guide
- `FORKING.md` - Guide for community platform support

## Contributing

- Binary issues/features: Open in this repository.
- Laravel wrapper issues: TODO pending canonical wrapper package confirmation.
- Community platform builds: See [FORKING.md](FORKING.md).
