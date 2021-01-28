# benchlog

Micro-benchmark to compute logs and in particular log base 2.

## Installation

Install the google benchmark library:

  https://github.com/google/benchmark

Use `-DCMAKE_INSTALL_PREFIX=/usr/local` to install it in `/usr/local`, other
places should work, too.

Then use this cmake for this repo:

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS=-march=native -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .
```

Replace `/usr/local` with the same place as you installed benchmark above. Then cmake
will find it automatically.
