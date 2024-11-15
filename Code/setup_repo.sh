#!/bin/bash

set -euo pipefail

REPO_DIR="$(pwd)"

if [ "${1-}" != "no-install-checks" ]; then
  echo "> Ensuring python3 is installed ..."
  if ! python3 --version > /dev/null 2>&1; then
    echo "Python3 is missing or version is too low."
    exit 1
  fi

  echo "> Ensuring clang is installed ..."
  if ! clang --version > /dev/null 2>&1; then
    echo "clang is not installed."
    exit 1
  fi

  echo "> Ensuring make is installed ..."
  if ! make --version > /dev/null 2>&1; then
    echo "make is not installed."
    exit 1
  fi

  echo "> Ensuring cmake is installed ..."
  if ! cmake --version > /dev/null 2>&1; then
    echo "cmake is not installed."
    exit 1
  fi

  echo "> Ensuring pytest is installed ..."
  if ! pytest --version > /dev/null 2>&1; then
    echo "pytest is not installed."
    exit 1
  fi
fi

echo "> Ensuring GoogleTest libraries & headers are available ..."

GTEST_LIBS=("libgtest.a" "libgmock.a" "libgtest_main.a" "libgmock_main.a")

MISSING=false
for FILE in "${GTEST_LIBS[@]}"; do
  if [ ! -f "$REPO_DIR/googletest/build/lib/$FILE" ]; then
    MISSING=true
    break
  fi
done

if $MISSING; then
  if [ ! -d "$REPO_DIR/googletest" ]; then
    echo "  > Cloning GoogleTest repository ..."
    git clone https://github.com/google/googletest.git googletest -b v1.15.2 > /dev/null 2>&1
  fi
  if [ ! -f "$REPO_DIR/googletest/build/Makefile" ]; then
    echo "  > Building GoogleTest ..."
    cmake -S "$REPO_DIR/googletest" -B "$REPO_DIR/googletest/build" > /dev/null 2>&1
  fi
  echo "  > Compiling library binaries"
  make -C "$REPO_DIR/googletest/build" > /dev/null 2>&1
fi

echo Setup Complete!