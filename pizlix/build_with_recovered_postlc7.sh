#!/bin/bash

set -e
set -x

./build_recover_postlc7.sh
./build_postlc8_continuation.sh

echo OK
