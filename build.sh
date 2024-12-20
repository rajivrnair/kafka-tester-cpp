#!/bin/bash

rm -rf build
mkdir build
cd build

cmake ..
make

cp kafka_producer ../
cp kafka_consumer ../