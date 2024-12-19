#!/usr/bin/env bash

# Clean data
rm -rf data/*

# Build web application
cd web || exit
npm run build

cp -R dist/* ../data/
