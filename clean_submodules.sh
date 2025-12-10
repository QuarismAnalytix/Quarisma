#!/bin/bash

# Script to revert all changes in submodules including unstaged changes

echo "Resetting all submodules to their committed state..."
git submodule foreach --recursive git reset --hard

echo "Cleaning untracked files from all submodules..."
git submodule foreach --recursive git clean -fd

echo "Done! All submodules have been cleaned."
git status
