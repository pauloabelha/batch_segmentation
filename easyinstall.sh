#!/bin/bash

if [ "$1" == "--help" ]; then
	echo "This script installs CGAL and the compile the segmentation code."
	echo "It will ask for your sudo to update and install the packages"
	echo "By Paulo Abelha"
	echo "github.com/pauloabelha"
	echo "pauloabelha.com"
	exit 0
fi

sudo apt-get update

sudo apt-get install libcgal-dev

cmake .

make -j 4

echo "Everything installed and compiled!"
echo "Please run: ./segment_meshes --help"
