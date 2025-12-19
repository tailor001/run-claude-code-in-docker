#!/usr/bin/env bash
# =============================================================================
# Claude Code Plugin Installation Script
# =============================================================================
# Purpose: Automatically install required Claude plugins on first run
# Responsibilities:
#   - Check if plugin marketplace is configured
#   - Add marketplace and install superpowers plugin if needed
#   - Execute the original command passed from entrypoint
# =============================================================================

set -e

# =============================================================================
# PLUGIN MANAGEMENT FUNCTIONS
# =============================================================================

check_marketplace_status() {
    # Query marketplace status and capture output
    local output
    output=$(claude plugin marketplace list 2>&1)

    # Check if marketplaces are configured
    if echo "$output" | grep -q "No marketplaces configured"; then
        echo "first_run_needed"
    else
        echo "already_configured"
    fi
}

install_plugins_on_first_run() {
    echo "⚠️  First-time setup: Installing required plugins..."

    # Add marketplace for superpowers plugin
    claude plugin marketplace add /opt/claude-plugins/superpowers

    # Install superpowers plugin
    claude plugin install superpowers

    echo "✅ Plugin installation completed"
}

verify_plugin_status() {
    local marketplace_status
    marketplace_status=$(check_marketplace_status)

    if [ "$marketplace_status" = "first_run_needed" ]; then
        install_plugins_on_first_run
    else
        echo "✅ Plugins already installed"
    fi
}

# =============================================================================
# SCRIPT EXECUTION
# =============================================================================

main() {
    # Verify and install plugins if needed
    verify_plugin_status

    # Execute the original command passed from entrypoint
    exec "$@"
}

# =============================================================================
# SCRIPT ENTRY POINT
# =============================================================================

main "$@"
