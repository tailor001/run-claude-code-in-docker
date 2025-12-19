#!/usr/bin/env bash
# Claude Code Docker container wrapper script
set -e

# SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="claude-code:tailor"
CONTAINER_NAME="claude-code-$(date +%s-%N)"
CURRENT_DIR="$(pwd)"
HOST_USER="$(whoami)"
HOST_UID="$(id -u)"
HOST_GID="$(id -g)"

echo "=== Claude Code Docker Wrapper ==="
echo "Image: $IMAGE_NAME"
echo "Container: $CONTAINER_NAME"
echo "User: $HOST_USER (UID:$HOST_UID, GID:$HOST_GID)"
echo "Directory: $CURRENT_DIR"

# Validate working directory
if [ ! -w "$CURRENT_DIR" ]; then
    echo "Error: Current directory is not writable: $CURRENT_DIR" >&2
    exit 1
fi

# Handle .env file
DOT_ENV_ARGS=""
if [ -f "$CURRENT_DIR/.env" ]; then
    echo "Loading environment variables from .env file..."
    while IFS='=' read -r key value; do
        # Skip comments and empty lines
        [[ $key =~ ^[[:space:]]*# ]] && continue
        [[ -z $key ]] && continue

        # Remove quotes and spaces
        key=$(echo "$key" | xargs)
        value=$(echo "$value" | sed 's/^["\x27]//' | sed 's/["\x27]$//' | xargs)

        if [ -n "$key" ] && [ -n "$value" ]; then
            DOT_ENV_ARGS="$DOT_ENV_ARGS --env $key=$value"
            echo "  $key=$value"
        fi
    done < "$CURRENT_DIR/.env"
fi

# Build base Docker arguments
DOCKER_BASE_ARGS="--rm \
    --name $CONTAINER_NAME \
    --network host \
    --env HOST_USER=$HOST_USER \
    --env HOST_UID=$HOST_UID \
    --env HOST_GID=$HOST_GID \
    --env HOST_WORKDIR=/workspace \
    $DOT_ENV_ARGS \
    --volume $CURRENT_DIR:/workspace \
    --volume $CURRENT_DIR/.claude:/workspace/.claude \
    --volume $CURRENT_DIR/.claude-system:/home/$HOST_USER/.claude \
    --workdir /workspace"

echo "Starting container..."
echo ""

# Determine interactive mode based on terminal availability
INTERACTIVE_MODE=""
if [ -t 0 ] && [ -t 1 ]; then
    INTERACTIVE_MODE="-it"
    echo "Interactive mode: enabled"
else
    INTERACTIVE_MODE=""
    echo "Interactive mode: disabled (piped input/output)"
fi

# Execute Docker command with proper argument passing
if [ $# -gt 0 ]; then
    # With arguments: pass them to container entrypoint
    echo "Executing: $@"
    exec docker run $INTERACTIVE_MODE $DOCKER_BASE_ARGS "$IMAGE_NAME" "$@"
else
    # Without arguments: use container's default CMD (interactive bash if available)
    if [ -n "$INTERACTIVE_MODE" ]; then
        echo "Starting interactive bash shell..."
    else
        echo "Starting bash shell (non-interactive)..."
    fi
    exec docker run $INTERACTIVE_MODE $DOCKER_BASE_ARGS "$IMAGE_NAME"
fi
