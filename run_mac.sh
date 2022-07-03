#!/bin/bash
cd "$(dirname "$0")" # Set working directory to the directory of the script.

echo "Checking if recompile is needed..."
recompile=false
if [ -f bin/mac/WhoStoleTheSun ];
then
	for file in $(find src -name '*.c'; find src -name '*.cpp'; find src -name '*.h'; find src -name '*.hpp')
	do
		if [ $file -nt bin/mac/WhoStoleTheSun ];
		then
			recompile=true
		fi
	done
fi

if [ "$recompile" = false ];
then
	echo "Recompile not needed."
else
	for file in $(find . -name '*.c')
	do
		echo "Compiling C file $file..."
		clang -std=c11 -c $file -o ${file}_arm.o -target arm64-apple-macos11
		clang -std=c11 -c $file -o ${file}_x64.o -target x86_64-apple-macos10.12
	done
	for file in $(find . -name '*.cpp')
	do
		echo "Compiling C++ file $file..."
		clang++ -std=c++17 -c $file -o ${file}_arm.o -target arm64-apple-macos11
		clang++ -std=c++17 -c $file -o ${file}_x64.o -target x86_64-apple-macos10.12
	done

	echo "Linking..."
	mkdir -p bin/mac/temp
	frameworks="-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
	armfiles=$(find . -name '*_arm.o' | tr '\n' ' ')
	x64files=$(find . -name '*_x64.o' | tr '\n' ' ')
	clang++ $frameworks $armfiles -L./lib -lc++ -lraylib_mac_arm64 -o bin/mac/temp/WhoStoleTheSun_arm64 -target arm64-apple-macos11
	clang++ $frameworks $x64files -L./lib -lc++ -lraylib_mac_x64 -o bin/mac/temp/WhoStoleTheSun_x64 -target x86_64-apple-macos10.12
	lipo -create -output bin/mac/WhoStoleTheSun bin/mac/temp/WhoStoleTheSun_arm64 bin/mac/temp/WhoStoleTheSun_x64
	
	echo "Deleting temporary files..."
	find . -name '*.o' -delete
	
	echo "Done."
fi

./bin/mac/WhoStoleTheSun