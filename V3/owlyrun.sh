#!/bin/bash

if [ $# -lt 1 ]; then
  echo "Usage: $0 [--verbose|-v] [--nodelete|-n] [--debug|-d] <file.owly>"
  exit 1
fi

VERBOSE=0
NODELETE=0
DEBUG=0

while [[ "$1" == -* ]]; do
  case "$1" in
    -v|--verbose)
      VERBOSE=1
      shift
      ;;
    -n|--nodelete)
      NODELETE=1
      shift
      ;;
    -d|--debug)
      DEBUG=1
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

SOURCE="$1"

if [ -z "$SOURCE" ]; then
  echo "Input file missing"
  exit 1
fi

if [ ! -f "$SOURCE" ]; then
  echo "File not found: $SOURCE"
  exit 1
fi

cat "$SOURCE"
printf "\n"

BASENAME="${SOURCE%.owly}"
# Step 0: Build the Owly compiler
bear -- make clean
bear -- make
if [ $? -ne 0 ]; then
  echo "Failed to build owlyc3"
  exit 1
fi

# Step 1: Compile Owly → C
if [ $DEBUG -eq 1 ]; then
  ./owlyc3 "$SOURCE" -d
else
  ./owlyc3 "$SOURCE"
fi

if [ $? -ne 0 ]; then
  echo "[X] Owly compilation failed"
  exit 1
else 
  echo "[X] Owly compilation successful"
fi


exit 0
if [ $NODELETE -eq 0 ]; then
  bear -- make clean
fi
