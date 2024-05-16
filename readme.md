# About
An implementation of mesh network. Note that project is actively developing and some features are not present

# Build

First to get all necessary files with all dependencies:
```console
git clone https://github.com/masamonoke/mesh-net-pet --recursive
```

</br>

To build with cmake:
```console
mkdir build && cd build
cmake ..
cmake --build . -j 4
```
</br>

To build with make:
```console
make
```
</br>

If you are using clangd language server you can build using this commands:
```console
compiledb make
```
or
```console
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

By default make builds debug type. You can specify build type:
```console
make BUILD_TYPE=debug
```
or
```console
make BUILD_TYPE=release
```

CMake builds release type by default. To specify build type update cache:
```console
cmake .. -DCMAKE_BUILD_TYPE=Debug 
```
and then rebuild project.

# Run

## Server

```console
make server BUILD_TYPE=release
```

or

```console
cd build/server && ./server
```

## Client

```console
./client ping <node_label>
```

or

```console
make client TARGET_ARGS="ping <node_label>"
```
