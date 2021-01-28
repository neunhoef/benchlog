all:
	mkdir build ; cd build ; cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS=-march=native -DCMAKE_INSTALL_PREFIX=/home/neunhoef ; cmake --build .
