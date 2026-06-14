# Forking BG Remover for Additional Platforms

## Overview

Artisan Build maintains official `bg-remover` binaries for the platforms used by this package:

- **Linux x86_64**: `bg-remover-ubuntu-x86_64`, built by `Dockerfile.ubuntu`
- **Linux arm64**: `bg-remover-linux-arm64`, built by `Dockerfile.ubuntu-arm64`
- **macOS arm64**: `bg-remover-macos-arm64`, built natively on macOS with Homebrew dependencies

The Linux builds are Debian 12/glibc builds with a `GLIBC_2.34` floor. They statically bundle OpenCV, libstdc++, and libgcc, and dynamically load the architecture-matching ONNX Runtime 1.19.2 `libonnxruntime.so.1` from `$ORIGIN` or `$ORIGIN/lib`.

## When to Fork

Fork this repository if you need a platform or packaging model that is not produced by the official release workflow, such as:

- Windows
- Alpine/musl
- Linux ARM32, RISC-V, or other architectures
- BSD variants
- Different glibc floors
- Fully self-contained ONNX Runtime packaging
- Different OpenCV or ONNX Runtime versions
- A distribution-specific package format

Linux arm64 is no longer a community-only target; it is officially supported for Laravel Cloud workers and other glibc >= 2.34 arm64 hosts.

## Current Build Pipeline

Before creating a fork, review the real upstream build pipeline:

- `Dockerfile.ubuntu` downloads ONNX Runtime linux-x64 1.19.2, builds OpenCV 4.10.0 static libraries, and links `bg-remover-ubuntu-x86_64` with static OpenCV plus dynamic ONNX Runtime.
- `Dockerfile.ubuntu-arm64` does the same for ONNX Runtime linux-aarch64 1.19.2 and outputs `bg-remover-linux-arm64`.
- `.github/workflows/build.yml` builds those two Linux targets with Docker and builds `bg-remover-macos-arm64` natively after `brew install opencv onnxruntime pkg-config`.
- The release publishes binaries and checksums. Consumers should fetch the ONNX Runtime package matching their platform from Microsoft and co-locate `libonnxruntime.so.1` with the binary.

Do not base a fork on stale Alpine, universal macOS, or system-OpenCV assumptions.

## How to Fork and Build

### Step 1: Fork The Repository

```bash
git clone https://github.com/YOUR-USERNAME/bg-remover.git
cd bg-remover
```

Keep the upstream remote handy:

```bash
git remote add upstream https://github.com/artisan-build/bg-remover.git
```

### Step 2: Pick Your Runtime Contract

Document these decisions before changing CI:

- Target OS and architecture
- glibc, musl, or non-Linux runtime assumptions
- Whether OpenCV is static or dynamic
- Whether ONNX Runtime is static, bundled, co-located, or provided by the host
- Binary and library asset names
- Minimum supported OS version

The official Linux contract is static OpenCV plus dynamic `libonnxruntime.so.1`. If your fork changes that contract, make it explicit in your README and release notes.

### Step 3: Add A Build Definition

Use the official Dockerfiles as templates for Linux-like targets. For example, a musl or older-glibc fork should include a new Dockerfile rather than overloading `Dockerfile.ubuntu`.

For non-Linux targets, use the native toolchain for that platform and ensure the C++ source is compiled with `-DWITH_ML` if you want the same default ML behavior as official builds.

### Step 4: Update Build Helpers

If you add a Makefile target, keep it specific and avoid changing official targets:

```makefile
ifeq ($(TARGET),my-platform)
    DOCKERFILE = Dockerfile.my-platform
    OUTPUT = bg-remover-my-platform
endif

build-docker-my-platform:
	docker build -f Dockerfile.my-platform -t bg-remover-my-platform .
	docker create --name bg-remover-my-platform-extract bg-remover-my-platform
	docker cp bg-remover-my-platform-extract:/app/bg-remover ./bg-remover-my-platform
	docker rm bg-remover-my-platform-extract
```

### Step 5: Test The Binary

Verify on a clean host or container, not just the build machine:

- The binary starts and prints `--help`.
- Default ML mode fails clearly without `--model` and succeeds with a valid ONNX model.
- `--grabcut` succeeds without a model.
- Required dynamic libraries are present and documented.
- Output is a PNG with alpha.
- Checksums are published for every asset.

For Linux, inspect dynamic dependencies with `readelf -d` and confirm they match your documented contract.

### Step 6: Set Up GitHub Actions

Add your target to a fork-specific workflow or matrix entry. If you publish runtime libraries, avoid ambiguous names: architecture-specific libraries must have architecture-specific asset names, or your release notes must tell users to fetch the correct upstream ONNX Runtime package.

### Step 7: Document Your Fork

Your fork README should include:

- Maintainer and repository URL
- Target OS/architecture
- Runtime library contract
- Exact ONNX Runtime and OpenCV versions
- Installation and co-location instructions
- Example ML and GrabCut invocations
- Relationship to upstream [artisan-build/bg-remover](https://github.com/artisan-build/bg-remover)

## Adding Your Fork To The Community List

Once your fork is stable and tested, submit a PR to add it to the community builds section in `README.md`.

In your PR description, include:

- Link to your fork
- Platform and architecture
- Runtime dependency contract
- How you test releases
- Confirmation that you will maintain platform-specific issues in your fork

## Maintenance Expectations

Artisan Build will:

- Link to active community forks when useful.
- Keep core C++ changes reasonably portable when possible.
- Review documentation-only PRs for community platform listings.

Artisan Build will not:

- Debug platform-specific build or runtime failures in your fork.
- Test unsupported platforms.
- Merge changes that break official Linux x86_64, Linux arm64, or macOS arm64 builds.
- Take over maintenance if a community fork becomes inactive.

You are responsible for:

- Keeping your fork up to date with upstream.
- Testing on the target platform.
- Responding to issues for your platform.
- Updating runtime libraries and documenting security-sensitive dependency changes.
- Maintaining your build infrastructure.

## Merging Upstream Changes

```bash
git fetch upstream
git checkout main
git merge upstream/main
make your-platform
git push origin main
```

Use non-interactive Git commands in automation, and never overwrite upstream changes without reviewing them.

## Legal

Your fork must comply with:

- `bg-remover`'s MIT License
- OpenCV's Apache 2.0 License
- ONNX Runtime's MIT License
- Any additional platform-specific license requirements

When distributing binaries, include required licenses and attribution notices.

## Questions

Open a discussion or issue in the main repository for core functionality questions: [artisan-build/bg-remover](https://github.com/artisan-build/bg-remover).
