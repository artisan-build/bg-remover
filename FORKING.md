# Forking BG Remover for Additional Platforms

## Overview

Artisan Build maintains bg-remover binaries for the platforms we actively use in production and development:

- **Alpine Linux (x86_64)** - Laravel Vapor deployments
- **Ubuntu (x86_64)** - Laravel Forge servers
- **macOS (Universal: ARM64 + x86_64)** - Development machines

If you need bg-remover on a different platform, you're welcome to fork this repository and maintain your own build. This document explains how.

## When to Fork

Fork this repository if you need bg-remover on:

- **Windows** (any architecture)
- **Other Linux distributions** (Arch, Fedora, CentOS, Debian, etc.)
- **ARM-based Linux** (Raspberry Pi, AWS Graviton, other ARM servers)
- **BSD variants** (FreeBSD, OpenBSD, NetBSD)
- **Different architectures** (ARM32, RISC-V, etc.)
- **Static binaries** for your specific use case
- **Specific OpenCV versions** not available in our builds

## Why We Don't Maintain These

We follow a simple maintenance policy: **we only maintain what we use**. This ensures:

- High-quality builds for supported platforms
- Timely updates and security patches
- Proper testing on real workloads
- Sustainable maintenance burden

We can't maintain platforms we don't actively use because we can't properly test them or respond to platform-specific issues.

## How to Fork and Build

### Step 1: Fork the Repository

1. Click "Fork" on the [bg-remover repository](https://github.com/artisan-build/bg-remover)
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR-USERNAME/bg-remover.git
   cd bg-remover
   ```

### Step 2: Create a Dockerfile for Your Platform

Create a new Dockerfile for your target platform. Use our existing Dockerfiles as templates:

**Example: `Dockerfile.windows` (hypothetical)**

```dockerfile
# Use Windows Server Core as base
FROM mcr.microsoft.com/windows/servercore:ltsc2022

# Install Visual Studio Build Tools and vcpkg for OpenCV
# (Windows-specific setup here)

WORKDIR /app

# Copy source
COPY src/bg-remover.cpp src/
COPY Makefile .

# Build (adjust for Windows toolchain)
RUN cl.exe src/bg-remover.cpp /link opencv_world.lib

CMD ["cmd"]
```

**Example: `Dockerfile.arch` (Arch Linux)**

```dockerfile
FROM archlinux:latest

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm gcc make pkgconf opencv

WORKDIR /app

COPY src/bg-remover.cpp src/
COPY Makefile .

RUN make

CMD ["bash"]
```

### Step 3: Update the Makefile

Add your platform to the Makefile:

```makefile
ifeq ($(TARGET),windows)
    DOCKERFILE = Dockerfile.windows
    OUTPUT = bg-remover-windows-x86_64.exe
endif

ifeq ($(TARGET),arch)
    DOCKERFILE = Dockerfile.arch
    OUTPUT = bg-remover-arch-x86_64
endif

# Add build targets
build-docker-windows:
	docker build -f Dockerfile.windows -t bg-remover-windows .
	# Extract binary...

build-docker-arch:
	docker build -f Dockerfile.arch -t bg-remover-arch .
	# Extract binary...

windows: TARGET = windows
windows: build-docker-windows

arch: TARGET = arch
arch: build-docker-arch
```

### Step 4: Test Your Build Locally

```bash
# Build for your platform
make windows
# or
make arch

# Test the binary
./bg-remover-windows-x86_64.exe -i test.jpg -o output.png
```

Verify:
- ✅ Binary executes without errors
- ✅ Dependencies are satisfied or bundled
- ✅ Output PNG has transparent background
- ✅ Works on a clean system (not just your dev machine)

### Step 5: Set Up GitHub Actions (Optional but Recommended)

Add your platform to `.github/workflows/build.yml`:

```yaml
strategy:
  matrix:
    include:
      # Existing platforms...
      
      - platform: windows
        runner: windows-latest
        dockerfile: Dockerfile.windows
        output: bg-remover-windows-x86_64.exe
      
      - platform: arch
        runner: ubuntu-latest
        dockerfile: Dockerfile.arch
        output: bg-remover-arch-x86_64
```

This automatically builds and attaches binaries to releases.

### Step 6: Create a Release

1. Tag a version in your fork:
   ```bash
   git tag -a v1.0.0-windows -m "Windows build"
   git push origin v1.0.0-windows
   ```

2. GitHub Actions will build and attach your binary to the release

3. Or manually create a release and upload the binary

### Step 7: Document Your Fork

Update your fork's README.md:

```markdown
# BG Remover - Windows Fork

This is a community-maintained fork providing Windows builds of bg-remover.

**Maintained by:** [@your-username](https://github.com/your-username)

**Platform:** Windows Server 2019+, Windows 10+

**Download:** See [Releases](https://github.com/your-username/bg-remover/releases)

**Upstream:** [artisan-build/bg-remover](https://github.com/artisan-build/bg-remover)

## Installation

1. Download `bg-remover-windows-x86_64.exe` from releases
2. Place in your PATH or project directory
3. Run: `bg-remover-windows-x86_64.exe -i input.jpg -o output.png`

## Maintenance

This fork is maintained independently. For issues specific to Windows builds, please open issues in this repository.

For core functionality issues, please check the [upstream repository](https://github.com/artisan-build/bg-remover).
```

## Adding Your Fork to the Community List

Once your fork is stable and tested, submit a PR to the main repository to add your fork to the README:

### Step 1: Fork the Main Repository (if you haven't)

### Step 2: Edit README.md

Find the "Unsupported Platforms" section and update it:

```markdown
## Community-Maintained Platform Builds

The following community members maintain builds for additional platforms:

| Platform | Maintainer | Repository | Status |
|----------|------------|------------|--------|
| Windows (x86_64) | [@your-username](https://github.com/your-username) | [your-username/bg-remover](https://github.com/your-username/bg-remover) | Active |
| Arch Linux | [@other-user](https://github.com/other-user) | [other-user/bg-remover](https://github.com/other-user/bg-remover) | Active |
```

### Step 3: Submit a Pull Request

```bash
git checkout -b add-windows-fork
# Make your changes
git commit -m "Add Windows fork to community platforms"
git push origin add-windows-fork
gh pr create --title "Add Windows fork to community platforms"
```

In your PR description:
- Link to your fork
- Confirm you'll maintain it
- Note your testing environment
- Provide example usage

## Maintenance Expectations

### What We'll Do (Artisan Build)

- ✅ Link to your fork in our README
- ✅ Direct users to your fork for your platform
- ✅ Keep the core C++ code compatible when possible

### What We Won't Do

- ❌ Debug platform-specific issues
- ❌ Test on your platform
- ❌ Provide support for your builds
- ❌ Merge platform-specific code that breaks our builds
- ❌ Take over maintenance if you stop

### What You're Responsible For

- ✅ Keeping your fork up to date with upstream
- ✅ Testing on your target platform
- ✅ Responding to issues in your fork
- ✅ Security updates for your platform
- ✅ Maintaining build infrastructure (Dockerfiles, CI/CD)

## Merging Upstream Changes

Stay up to date with the main repository:

```bash
# Add upstream remote (once)
git remote add upstream https://github.com/artisan-build/bg-remover.git

# Fetch upstream changes
git fetch upstream

# Merge into your main branch
git checkout main
git merge upstream/main

# Resolve any conflicts
# Test your build
make your-platform

# Push to your fork
git push origin main
```

## Tips for Success

### 1. Start Small
Get a basic build working before adding features.

### 2. Use Docker When Possible
Docker ensures reproducible builds and makes CI/CD easier.

### 3. Document Everything
Your users will have different setups. Clear documentation prevents issues.

### 4. Automate Builds
GitHub Actions or similar CI/CD ensures consistency.

### 5. Test Thoroughly
- Test on clean systems (not just your dev machine)
- Test with different image formats
- Test error cases (missing files, invalid images)

### 6. Communicate Status
If you can't maintain the fork anymore, update your README and notify us so we can remove the link.

## Example: Raspberry Pi (ARM64)

Here's a complete example for Raspberry Pi:

**Dockerfile.raspberrypi**
```dockerfile
FROM arm64v8/ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ make pkg-config libopencv-dev

WORKDIR /app

COPY src/bg-remover.cpp src/
COPY Makefile .

RUN make

CMD ["bash"]
```

**Build and test:**
```bash
# Build on Raspberry Pi or via QEMU
docker build -f Dockerfile.raspberrypi -t bg-remover-rpi .

# Extract binary
docker create --name rpi-extract bg-remover-rpi
docker cp rpi-extract:/app/bg-remover ./bg-remover-raspberrypi-arm64
docker rm rpi-extract

# Test
./bg-remover-raspberrypi-arm64 -i test.jpg -o output.png
```

## Getting Help

### For Platform-Specific Issues
- Ask in forums/communities for your platform
- Check OpenCV installation guides for your OS
- Review Docker documentation for your base image

### For Core Functionality Issues
- Open an issue in the [main repository](https://github.com/artisan-build/bg-remover/issues)
- Check if it reproduces on officially supported platforms first

### For Maintenance Questions
- See this document
- Look at how we maintain our platforms
- Ask questions in GitHub Discussions

## Legal

Your fork must comply with:
- **bg-remover's MIT License** - Keep the license file and attribution
- **OpenCV's Apache 2.0 License** - Acknowledge OpenCV usage
- Any additional platform-specific license requirements

When distributing binaries, include LICENSE files and attributions.

## Summary

1. ✅ Fork the repository
2. ✅ Create Dockerfile for your platform
3. ✅ Update Makefile with build targets
4. ✅ Test thoroughly on your platform
5. ✅ Set up GitHub Actions (optional)
6. ✅ Create releases with binaries
7. ✅ Document your fork
8. ✅ Submit PR to add to community list
9. ✅ Maintain your fork independently
10. ✅ Stay up to date with upstream

We appreciate community contributions! While we can't maintain every platform, we're happy to support community members who do.

---

**Questions?** Open a discussion in the [main repository](https://github.com/artisan-build/bg-remover/discussions).

**Ready to contribute?** We'd love to link to your fork!
