#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

if [ ! -d build ]; then
    mkdir build
fi


pushd build
cmakerc=`cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..`
if [ $cmakerc -ne 0 ] 2>/dev/null; then
	echo "Cmake failed, exiting"
	exit $cmakerc
fi
popd
