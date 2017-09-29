#!/bin/bash

if [ "$1" == "--help" ]; then
	echo "This script runs the segmentation demo for one point cloud"
	echo "It will create a folder 'data_demo_segmented' and dump the segmented point clouds in there"
	echo "It will create a folder 'data_demo_segmented_unique' and dump the unique segmented point clouds in there"
	echo "Please install CGAL and compile my code before running this demo."
	echo "Run easy_install.sh"
	echo "By Paulo Abelha"
	echo "github.com/pauloabelha"
	echo "pauloabelha.com"
	exit 0
fi

echo "Runing command: ./segment_meshes -i data_demo -o data_demo_segmented -z 3:2:5,0.3:0.1:0.5 -e ply"
echo ""
echo ""

./segment_meshes -i data_demo -o data_demo_segmented -z 3:2:7,0.2:0.1:0.8 -e ply --verbose

./remove_similar_segm -i data_demo_segmented -o data_demo_segmented_unique --verbose


