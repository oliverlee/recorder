# Recorder
This project aims to read simulated sensor data and write it to a log file.

## Building
This project requires a compiler supporting C++17 and uses
[CMake](https://cmake.org/) for building.
[asio](https://github.com/chriskohlhoff/asio) is used for network
communication and [json](https://github.com/nlohmann/json) is used to encode to
JSON. [Google Test](https://github.com/google/googletest) is used for testing.

It has been tested with GCC 7 and Clang 7 on Ubuntu 18.04

```
oliver@canopus:~/repos/recorder$ git submodule update --init --recursive --recommend-shallow
oliver@canopus:~/repos/recorder$ cmake -S . -B build
oliver@canopus:~/repos/recorder$ cmake --build build
```

## Usage (if logger is incomplete)
Here's a simple usage example.

```
oliver@canopus:~/repos/recorder$ <sensor-simulator-bin> | ./build/reader/reader
```

Decoded messages are written to stdout. To log to file, simply redirect.

```
oliver@canopus:~/repos/recorder$ <sensor-simulator-bin> | ./build/reader/reader > <logname>
```

## Notes
The sensor reports a timestamp without an associated timezone. The machine
local timezone is used.
