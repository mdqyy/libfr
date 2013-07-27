#!/bin/bash

echo "Setting environment"

CUR_DIR=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CUR_DIR/lib

