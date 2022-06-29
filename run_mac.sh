#!/bin/bash
cd "$(dirname "$0")" # Set working directory to the directory of the script.

echo "Checking if recompile is needed..."
recompile=true
if [ -f bin/mac/WhoStoleTheSun ]; then
	mostrecent=$(ls -t src | head -n1)
	if [ bin/mac/WhoStoleTheSun -nt src/$mostrecent ]; then
		recompile=false
		echo "Recompile not needed."
	fi
fi

if [ "$recompile" = true ]; then
	echo "Looking for files to compile..."
	files=$(find . -name '*.c' | tr '\n' ' ')

	echo "Compiling..."
	frameworks="-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
	mkdir -p bin/mac/temp
	clang $frameworks $files -L./lib -lraylib_mac_arm64 -o bin/mac/temp/WhoStoleTheSun_arm64 -target arm64-apple-macos11
	clang $frameworks $files -L./lib -lraylib_mac_x64 -o bin/mac/temp/WhoStoleTheSun_x64 -target x86_64-apple-macos10.12
	lipo -create -output bin/mac/WhoStoleTheSun bin/mac/temp/WhoStoleTheSun_arm64 bin/mac/temp/WhoStoleTheSun_x64
	echo "Done."
fi

./bin/mac/WhoStoleTheSun