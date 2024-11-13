#!/bin/bash

set -euo pipefail

REPO_DIR="$(pwd)"
GTEST_DIR="$REPO_DIR/googletest"

if [ ! -d "$GTEST_DIR" ]; then
  echo "Cloning googletest repository at $GTEST_DIR"
  git clone https://github.com/google/googletest.git googletest -b v1.15.2
else
  echo "Found googletest at $GTEST_DIR"
fi

echo "Building googletest"
cd "$GTEST_DIR"
mkdir -p build
cd build
cmake ..
make