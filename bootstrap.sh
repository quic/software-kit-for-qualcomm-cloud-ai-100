#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

if [ ! -d build ]; then
    mkdir build
fi


pushd build
cmakerc=`cmake ..`
if [ $cmakerc -ne 0 ] 2>/dev/null; then
	echo "Cmake failed, exiting"
	exit $cmakerc
fi
popd
