#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

script_dir=$(dirname "$(readlink -f "$0")")
code_base="$(realpath  $script_dir'/..')"

echo $script_dir

#------------------------------------------------------------------------------------
# Simple formatting script to comply with CLANG code formatting standards for c/c++
#------------------------------------------------------------------------------------
echo "Formatting code files..."
find $code_base -type f -regex '\(.*cpp\|.*h\|.*hpp\)' \
	-not -name "*.sh" \
	-not -path "*/modules/*" \
	-not -path "./.git/*" \
	-not -path "*_deps*" \
	-not -path "*/scripts/*" \
	-not -path "./build/*" \
	-exec clang-format-3.8 -style="{BasedOnStyle: llvm, SortIncludes: false}" -i {} \;

echo "Cleaning Cmake files ..."
find $code_base -type f -regex '\(.*CMakeLists.txt\|.*.cmake\)' \
	-not -path "*/modules/*" \
	-not -path "./.git/*" \
	-not -path "*_deps*" \
	-not -path "*/scripts/*" \
	-not -path "./build/*" \
	-exec sed -i -e 's/[[:blank:]]*$//' {} \;
