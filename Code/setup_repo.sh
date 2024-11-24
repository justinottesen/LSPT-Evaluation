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

  echo "> Ensuring unzip is installed ..."
  if ! which unzip > /dev/null 2>&1; then
    echo "unzip is not installed."
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

echo "> Ensuring sqlite3.o exists ..."
if [ ! -f "$REPO_DIR/sqlite/sqlite3.o" ]; then
  mkdir -p "$REPO_DIR/sqlite"

  if [ ! -f "$REPO_DIR/sqlite/download.zip" ]; then
    echo " > Downloading SQLite source files"
    curl -L https://www.sqlite.org/2024/sqlite-amalgamation-3470000.zip > sqlite/download.zip 2> /dev/null
  fi
  if [ ! -f "$REPO_DIR/sqlite/sqlite3.c" ]; then
    echo " > Unzipping SQLite download"
    unzip -j sqlite/download.zip -d sqlite > /dev/null 2>&1
  fi
  echo "  > Compiling sqlite3.c -> sqlite3.o"
  clang -o sqlite/sqlite3.o sqlite/sqlite3.c -c
fi

echo Setup Complete!