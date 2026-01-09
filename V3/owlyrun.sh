
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
cat "${SOURCE}"
printf "\n"
if [ -z "$SOURCE" ]; then
  echo "Input file missing"
  exit 1
fi

BASENAME="${SOURCE%.owly}"
CFILE="${BASENAME}.c"
EXE="${BASENAME}"

# Step 0: Compile the Owly compiler
gcc -o owlyc3 owlyc3.c owlylexer.c parser.c memutils.c ast.c ast_to_json.c expressions.c SA.c SA_to_json.c -lcjson
if [ $? -ne 0 ]; then
  echo "Failed to compile owlyc.c"
  exit 1
fi

# Step 1: Compile Owly source to C
if [ $DEBUG -eq 1 ]; then
  ./owlyc3 "$SOURCE" "$CFILE" -d
else
  ./owlyc3 "$SOURCE" "$CFILE"
fi


if [ $? -ne 0 ]; then
  echo "Owly compilation failed"
  exit 1
fi

if [ $VERBOSE -eq 1 ]; then
  code "$CFILE"
fi

# Step 2: Compile generated C to executable
gcc -o "$EXE" "$CFILE"
if [ $? -ne 0 ]; then
  echo "C compilation failed"
  exit 1
fi

# Step 3: Run the resulting executable
echo "Running $EXE.owly"
./"$EXE"

# Step 4: Clean up intermediate files. Always remove <owly>.c file
rm -f "$CFILE"
# Optionally keep executable files
if [ $NODELETE -eq 0 ]; then
  rm -f "$EXE" "owlyc3"
fi