# Recorder
This project aims to read simulated sensor data and write it to a log file.

## Building
This project requires a compiler supporting C++17 and uses
[CMake](https://cmake.org/) for building.
[asio](https://github.com/chriskohlhoff/asio) is used for network
communication and [json](https://github.com/nlohmann/json) is used to encode to
JSON. [Google Test](https://github.com/google/googletest) is used for testing.

It has been tested with GCC 7 and Clang 7 on Ubuntu 18.04

To build
```
oliver@canopus:~/repos/recorder$ git submodule update --init --recursive --recommend-shallow
oliver@canopus:~/repos/recorder$ cmake -S . -B build
oliver@canopus:~/repos/recorder$ cmake --build build
```

To run tests
```
oliver@canopus:~/repos/recorder$ cd build
oliver@canopus:~/repos/recorder/build$ GTEST_COLOR=1 ctest -V
```

## Usage
Here's a simple usage example.

Start the log server first
```
oliver@canopus:~/repos/recorder$ ./build/bin/logger 12345
```

Connect to the log server with a reader client
```
oliver@canopus:~/repos/recorder$ <sensor-simulator-bin> | ./build/bin/reader localhost 12345
```
The sensor data is piped to the reader client, which then forwards data to the
remote log server.

The server accepts connections from multiple clients simultaneously. Status
messages are also displayed when clients connect or disconnect.

Here's an example of logger output with two connections:
```
oliver@canopus:~/repos/recorder$ ./build/bin/logger 12345
Starting logger on port 12345
[127.0.0.1:56184] Established connection
{
    "humidity": 80.80000305175781,
    "name": "e74bb50f-5a1e-4742-8827-20bc80a17297",
    "temperature": 3161.130126953125,
    "timestamp": "2020-06-28T16:51:50.240+0200"
}
{
    "humidity": 67.9000015258789,
    "name": "e74bb50f-5a1e-4742-8827-20bc80a17297",
    "temperature": 2731.8701171875,
    "timestamp": "2020-06-28T16:51:54.732+0200"
}
{
    "name": "e74bb50f-5a1e-4742-8827-20bc80a17297",
    "timestamp": "2020-06-28T16:51:59.570+0200"
}
[127.0.0.1:56185] Established connection
{
    "humidity": 17.799999237060547,
    "name": "héllö",
    "timestamp": "2020-06-28T16:52:04.006+0200"
}
{
    "name": "e74bb50f-5a1e-4742-8827-20bc80a17297",
    "temperature": 3479.840087890625,
    "timestamp": "2020-06-28T16:52:03.403+0200"
}
{
    "name": "héllö",
    "temperature": 3440.93017578125,
    "timestamp": "2020-06-28T16:52:05.613+0200"
}
{
    "humidity": 85.9000015258789,
    "name": "e74bb50f-5a1e-4742-8827-20bc80a17297",
    "temperature": 2462.35009765625,
    "timestamp": "2020-06-28T16:52:05.668+0200"
}
[127.0.0.1:56184] Terminating connection
{
    "humidity": 5.5,
    "name": "héllö",
    "temperature": 732.1300048828125,
    "timestamp": "2020-06-28T16:52:09.358+0200"
}
```

To log to a file, redirect the output of the logger:
```
oliver@canopus:~/repos/recorder$ ./build/bin/logger 12345 > sensor.log
```
Note that status messages are written to stderr.

## Notes
The sensors transmit a timestamp without an associated timezone. The log process
uses the machine local timezone.
