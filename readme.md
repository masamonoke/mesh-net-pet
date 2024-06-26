# About
An implementation of mesh network. Average throughput is 20 requests per second.

# Build

First to get all necessary files with all dependencies:
```console
git clone https://github.com/masamonoke/mesh-net-pet --recursive
```
</br>

Before any build command do:
```console
cd deps/zlib/ && ./configure

```

Then you can build as usual:


```console
make
```
</br>

If you are using clangd language server you can build using this commands:
```console
compiledb make
```

By default make builds release type. You can specify build type:
```console
make BUILD_TYPE=debug
```

# Run

## Server

```console
make server
```
Note that Makefile makes release build by default.

## Client

### Ping

```console
make client TARGET_ARGS="ping <node_addr>"
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
There is limitation in 150 symbols. Otherwise your message will be clipped.

### Kill

```console
 make client TARGET_ARGS="kill <addr>"
```

### Revive

```console
 make client TARGET_ARGS="revive <addr>"
```

### Reset

```console
 make client TARGET_ARGS="reset"
```

### Broadcast

```console
 make client TARGET_ARGS="broadcast -s <sender node addr> -a '<message>'"
```

### Unicast

```console
 make client TARGET_ARGS="unicast -s <sender node addr> -a '<message>'"
```

Unicast works similiar to broadcast but request is handled by one node only.

## Tests

Run server before testing

```console
make test
```

## Benchmark

Run server before benchmarking

```console
make benchmark
```

# Bugs
* <del>After killing node (nodes) there can be error 141 (broken pipe) when sending message to other nodes probably because of write to already closed fd</del>
* <del>Server can fail to send request result to client but the network handled request successfully when running parallel clients</del>
