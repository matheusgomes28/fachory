#!/usr/bin/env bash

SCRIPT_DIR="$(realpath "$(dirname .)")"
STYLE_FILE="${SCRIPT_DIR}/.clang-format"

Help()
{
   # Display Help
   echo "Runs clang-format on all tracked CPP files"
   echo
   echo "Syntax: ./clang-format [-f|--fix]"
   echo "options:"
   echo "f | --fix   Fixes all formatting issues in place"
   echo
}

FIX=false
while true; do
  case "$1" in
    -f | --fix ) FIX=true; shift ;;
    -h | --help ) Help; exit 2;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done


# Get all the tracked CPP items
CPP_FILES="$(git ls-tree --full-tree --name-only -r HEAD | grep -E ".*(\.hpp|\.cpp)")"

if [[ $FIX == true ]]; then
  clang-format --Werror -i --style=file:"${STYLE_FILE}" -- ${CPP_FILES}
else
  clang-format --Werror --dry-run --style=file:"${STYLE_FILE}" -- ${CPP_FILES}
fi

