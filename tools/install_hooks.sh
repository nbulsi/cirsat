#!/bin/bash

# Create symbolic link for pre-commit hook
ln -sf ../../.hooks/pre-commit "$(git rev-parse --git-dir)/hooks/pre-commit"

# Make pre-commit hook executable
chmod +x "$(git rev-parse --git-dir)/hooks/pre-commit"

echo "Git hooks installed successfully!"