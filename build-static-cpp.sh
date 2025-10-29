#!/bin/bash

# Build a properly working dynamic binary for Alpine

set -e

echo "🏗️  Building Background-Remover C++ Binary"
echo "======================================"
echo ""

docker build -f Dockerfile.cpp-fixed -t bg-remover-alpine-final .

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "📤 Extracting binary..."
docker create --name final-extract bg-remover-alpine-final
docker cp final-extract:/app/bg-remover ./bg-remover-alpine-final
docker rm final-extract

chmod +x bg-remover-alpine-final

echo ""
echo "✅ Binary created: bg-remover-alpine-final"
ls -lh bg-remover-alpine-final
file bg-remover-alpine-final
echo ""

echo "🔗 Checking dependencies..."
docker run --rm bg-remover-alpine-final ldd /app/bg-remover