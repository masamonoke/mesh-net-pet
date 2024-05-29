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

Note that Makefile makes debug build by default.

## Client

### Ping

```console
make client TARGET_ARGS="ping <node_label>"
```

### Send

```console
make client TARGET_ARGS="send -s <sender node> -r <receiver node>"
```

This command is interpreted like "send empty message to receiver node app 0 from sender node app 0".

To specify message:

```console
make client TARGET_ARGS="send -s <sender node> -r <receiver node> -a 'some meaningful message' -as <sender app> -ar <receiver app>"
```

### Kill

```console
 make client TARGET_ARGS="kill <label>"
```

### Revive

```console
 make client TARGET_ARGS="revive <label>"
```

### Reset

```console
 make client TARGET_ARGS="reset"
```

## Tests

Run server before testing

```console
make test
```

# Bugs
* <del>After killing node (nodes) there can be error 141 (broken pipe) when sending message to other nodes probably because of write to already closed fd
