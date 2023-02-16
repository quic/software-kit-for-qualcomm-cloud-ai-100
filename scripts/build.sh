#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function print_help() {
  echo -e "\n build.sh [OPTIONS...]"
  echo "Usage: build.sh -b [ Release | Debug ]"

  if [ "$#" == 1 ]; then
    if [ "$1" == 0 ]; then
      exit 0
    else
      exit "$1"
    fi
  fi
}

while [ $# -gt 0 ]; do
  key="$1"
  case $key in
  --build_type | -b)
    shift
    build_type="$1"
    shift
    ;;
  --help | -h)
    print_help 0
    ;;
  *)
    echo "Unknown option: $1"
    print_help -1
    ;;
  esac
done


if [ -z "$build_type" ]; then
  build_type="Debug"
  echo 'Default build type , setting it to :' "$build_type"
fi

 echo "Building for $build_type"
 cmake -DCMAKE_BUILD_TYPE="$build_type"  .. && make -j $(nproc)
