#!/usr/bin/env bash
# =============================================================================
# Claude Code Docker Container Entry Point Script
# =============================================================================
# Purpose: Initialize container environment and execute commands as host user
# Responsibilities:
#   - Create user matching host system (UID/GID)
#   - Set up proper environment and permissions
#   - Handle Claude-specific configuration
#   - Execute commands with correct user context
# =============================================================================

set -e

# =============================================================================
# ENVIRONMENT INITIALIZATION
# =============================================================================

print_startup_banner() {
    echo "=============================================================================="
    echo "Claude Code Docker Container Entry Point"
    echo "=============================================================================="
}

load_environment_variables() {
    # Load host user information from environment variables or use defaults
    readonly USERNAME=${HOST_USER:-$(whoami)}
    readonly USER_ID=${HOST_UID:-1000}
    readonly GROUP_ID=${HOST_GID:-1000}
    readonly WORKDIR=${HOST_WORKDIR:-/workspace}

    echo "Host User: $USERNAME (UID:$USER_ID, GID:$GROUP_ID)"
    echo "Working Directory: $WORKDIR"
    echo ""
}

# =============================================================================
# USER MANAGEMENT FUNCTIONS
# =============================================================================

cleanup_existing_user() {
    # Remove existing user/group to handle UID/GID changes
    if id "$USERNAME" &>/dev/null; then
        userdel -f "$USERNAME" 2>/dev/null || true
    fi

    if getent group "$USERNAME" &>/dev/null; then
        groupdel "$USERNAME" 2>/dev/null || true
    fi
}

create_system_user() {
    echo "👤 Creating system user $USERNAME with UID:$USER_ID GID:$GROUP_ID"

    # Clean up any existing user/group with same name
    cleanup_existing_user

    # Create group first
    groupadd -g "$GROUP_ID" "$USERNAME" 2>/dev/null || groupadd -f "$USERNAME"

    # Create user with proper UID/GID and home directory
    useradd -m -u "$USER_ID" -g "$GROUP_ID" -s /bin/bash "$USERNAME"

    # Verify user creation
    if id "$USERNAME" &>/dev/null; then
        echo "✅ User $USERNAME created successfully"
    else
        echo "❌ Failed to create user $USERNAME, falling back to UID-only mode"
        echo "👤 Using direct UID/GID permission mapping: $USER_ID:$GROUP_ID"
    fi
    echo ""
}

# =============================================================================
# CLAUDE CONFIGURATION MANAGEMENT
# =============================================================================

setup_claude_directories() {
    # Ensure essential directories exist
    mkdir -p "/home/$USERNAME/.claude/session-env"
    mkdir -p "/home/$USERNAME/.claude/projects"
    mkdir -p "/home/$USERNAME/.claude/file-history"

    # Initialize claude.json if it doesn't exist
    local config_file="/home/$USERNAME/.claude/.claude.json"
    local default_config="/opt/claude.json"

    if [ ! -f "$config_file" ] && [ -f "$default_config" ]; then
        cp "$default_config" "$config_file"
        chown "$USER_ID:$GROUP_ID" "$config_file"
    fi
}

initialize_claude_config() {
    local config_file="/home/$USERNAME/.claude.json"
    local default_config="/opt/claude.json"
    local custom_config="/workspace/claude.json"

    # Initialize claude.json if it doesn't exist
    if [ ! -f "$config_file" ]; then
        echo "🔧 Initializing Claude configuration..."

        # Use custom config if provided by user, otherwise use default
        if [ -f "$custom_config" ]; then
            echo "📝 Using custom claude.json from project directory"
            cp "$custom_config" "$config_file"
        elif [ -f "$default_config" ]; then
            echo "📝 Using default claude.json from container"
            cp "$default_config" "$config_file"
        else
            echo "⚠️  No default claude.json found in container"
        fi

        # Set correct ownership
        if [ -f "$config_file" ]; then
            chown "$USER_ID:$GROUP_ID" "$config_file"
            echo "✅ claude.json initialized successfully"
        fi
    else
        echo "✅ claude.json already exists"

        # If user provided custom config, use it to override existing
        if [ -f "$custom_config" ]; then
            echo "📝 Updating with custom claude.json from project directory"
            cp "$custom_config" "$config_file"
            chown "$USER_ID:$GROUP_ID" "$config_file"
        fi
    fi
}

verify_claude_configuration() {
    if [ ! -f "/home/$USERNAME/.claude/settings.json" ]; then
        echo "⚠️  No Claude settings found - please run Claude on host first to configure"
    else
        echo "✅ Claude configuration available from host"
    fi

    # Ensure Claude command is available
    if command -v claude &> /dev/null; then
        echo "✅ Claude command is available"
    else
        echo "⚠️  Claude command not found - installation may be incomplete"
    fi
}

handle_claude_verification() {
    if [ "$1" = "claude" ] || [[ "$1" == *"claude"* ]]; then
        echo "🔧 Verifying Claude configuration..."
        setup_claude_directories
        initialize_claude_config
        verify_claude_configuration
    else
        echo "ℹ️  Skipping Claude verification (non-Claude command)"
    fi
    echo ""
}

# =============================================================================
# SYSTEM HEALTH CHECKS
# =============================================================================

check_working_directory() {
    if [ ! -d "$WORKDIR" ]; then
        echo "❌ Working directory does not exist: $WORKDIR"
        exit 1
    fi

    # Check if we can write to working directory
    if ! touch "$WORKDIR/.container_test" 2>/dev/null; then
        echo "❌ Cannot write to working directory: $WORKDIR"
        exit 1
    else
        rm -f "$WORKDIR/.container_test"
        echo "✅ Working directory is accessible"
    fi
}

check_claude_availability() {
    if [ "$1" = "claude" ] || [[ "$1" == *"claude"* ]]; then
        if command -v claude &> /dev/null; then
            echo "✅ Claude command is available"
        else
            echo "⚠️  Claude command not found - this might indicate installation issues"
        fi
    fi
}

fix_file_ownership() {
    # Fix file ownership for .claude directory to match host user
    if [ -d "/home/$USERNAME/.claude" ]; then
        chown -R "$USER_ID:$GROUP_ID" "/home/$USERNAME/.claude" 2>/dev/null || true
        chown -R "$USER_ID:$GROUP_ID" "/workspace/.claude" 2>/dev/null || true
        echo "✅ Fixed ownership for .claude directory"
    fi
}

run_health_checks() {
    echo "🏥 Running container health checks..."
    check_working_directory
    check_claude_availability "$@"
    fix_file_ownership
    echo ""
}

# =============================================================================
# USER ENVIRONMENT SETUP
# =============================================================================

setup_user_environment() {
    echo "🔐 Setting up complete user environment for $USERNAME"

    # Export all current environment variables to preserve them for user session
    export $(printenv | grep -v 'PWD' | cut -d'=' -f1)

    # Ensure user home directory exists and has correct permissions
    mkdir -p "/home/$USERNAME"
    chown -R "$USER_ID:$GROUP_ID" "/home/$USERNAME"

    # Copy essential bash files if they don't exist
    if [ ! -f "/home/$USERNAME/.bashrc" ]; then
        cat > "/home/$USERNAME/.bashrc" << 'EOF'
# ~/.bashrc: executed by bash(1) for non-login shells.

export PS1='\u@\h:\w\$ '
[ -z "$PS1" ] && return
EOF
        chown "$USER_ID:$GROUP_ID" "/home/$USERNAME/.bashrc"
    fi
}

execute_as_user() {
    local command_args=("$@")

    if [ ${#command_args[@]} -eq 0 ]; then
        # No arguments: start interactive shell as actual user with preserved environment
        echo "Starting interactive shell as $USERNAME"
        cd "/home/$USERNAME"
        exec env -i \
            "USER=$USERNAME" \
            "LOGNAME=$USERNAME" \
            "HOME=/home/$USERNAME" \
            "SHELL=/bin/bash" \
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
            $(printenv | grep -v '^USER=\|^LOGNAME=\|^HOME=\|^SHELL=\|^PATH=' | tr '\n' ' ') \
            setpriv --reuid "$USER_ID" --regid "$GROUP_ID" --clear-groups bash
    else
        # With arguments: execute command as actual user with preserved environment
        echo "Executing command as $USERNAME: ${command_args[*]}"

        if id "$USERNAME" &>/dev/null; then
            echo "✅ User $USERNAME created successfully"
        fi

        exec env -i \
            "USER=$USERNAME" \
            "LOGNAME=$USERNAME" \
            "HOME=/home/$USERNAME" \
            "SHELL=/bin/bash" \
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
            $(printenv | grep -v '^USER=\|^LOGNAME=\|^HOME=\|^SHELL=\|^PATH=' | tr '\n' ' ') \
            setpriv --reuid "$USER_ID" --regid "$GROUP_ID" --clear-groups \
            /opt/claude-setup/install-plugins.sh "${command_args[@]}"
    fi
}

main() {
    # Print startup banner
    print_startup_banner

    # Load environment variables from host
    load_environment_variables

    # Change to working directory
    cd "$WORKDIR" || exit 1

    # Create system user matching host
    create_system_user

    # Handle Claude-specific configuration if needed
    handle_claude_verification "$@"

    # Run system health checks
    run_health_checks "$@"

    # Set up user environment
    setup_user_environment

    echo "🔄 Switching to user: $USERNAME"
    echo "🚀 Container is ready - executing command as $USERNAME..."
    echo ""

    # Execute command as target user
    execute_as_user "$@"
}

# =============================================================================
# SCRIPT ENTRY POINT
# =============================================================================

main "$@"
