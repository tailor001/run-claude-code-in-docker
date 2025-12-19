#!/bin/bash
# =============================================================================
# Docker Image Build Script for Claude Code Container
# =============================================================================
# Purpose: Build the claude-code:tailor Docker image from current Dockerfile
# Usage: ./build.sh
# Dependencies: docker command must be available and running
# =============================================================================

set -e

# Configuration
IMAGE_NAME="claude-code:tailor"

echo "=============================================================================="
echo "Building Claude Code Docker Image"
echo "=============================================================================="
echo "Target Image: $IMAGE_NAME"
echo "Build Context: $(pwd)"
echo "Dockerfile: $(pwd)/Dockerfile"
echo ""

# Execute Docker build command
echo "🔨 Building Docker image..."
docker build \
    --network host \
    --build-arg http_proxy="http://127.0.0.1:7890" \
    -t "$IMAGE_NAME" .

echo ""
echo "=============================================================================="
echo "✅ Build completed successfully!"
echo "=============================================================================="
echo "Image Name: $IMAGE_NAME"
echo "Run Container: ./claude-code.sh claude"
echo "=============================================================================="
