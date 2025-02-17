#!/bin/bash

set -e

cd "$(dirname "$0")"/..

. ci/commons.bash

MODE=$1
if [ -z "$MODE" ]; then
  MODE="Release"
fi

if [ "$MODE" != "Asan" ] && [ "$MODE" != "Release" ]; then
  error "Invalid argument. Must be either 'Asan' or 'Release'."
  exit 1
fi

message "Got MODE: '$MODE'"

message "Preparing a developement environment..."
mkdir -p build
(cd build && cmake -Dlab1_DEVELOPER=ON -Dlab1_BENCHMARK=ON -Dlab2_CACHE=ON -DCMAKE_BUILD_TYPE=$MODE ..)

message "Preparing a optimized environment..."
mkdir -p build-opt
(cd build-opt && cmake -Dlab1_DEVELOPER=ON -Dlab1_BENCHMARK=ON -Dlab2_CACHE=ON -Dlab1_OPT=ON -DCMAKE_BUILD_TYPE=$MODE ..)
