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

You should see something like this:
```
oliver@canopus:~/repos/recorder/build$ ./bin/logger 12345
Starting logger on port 12345
Established connection with client 127.0.0.1:54673
{
    "humidity": 81.4000015258789,
    "name": "a5350d06-d165-44a7-badf-27149714e1d4",
    "timestamp": "2020-06-27T20:46:21.837+0200"
}
Established connection with client 127.0.0.1:54676
{
    "humidity": 58.70000076293945,
    "name": "5c5b23dc-c99e-4ba2-9009-72d92652f1b5",
    "temperature": 4941.89990234375,
    "timestamp": "2020-06-27T20:46:26.751+0200"
}
{
    "humidity": 12.100000381469727,
    "name": "a5350d06-d165-44a7-badf-27149714e1d4",
    "temperature": 2117.440185546875,
    "timestamp": "2020-06-27T20:46:25.287+0200"
}
{
    "name": "5c5b23dc-c99e-4ba2-9009-72d92652f1b5",
    "temperature": 2447.10009765625,
    "timestamp": "2020-06-27T20:46:28.843+0200"
}
{
    "name": "a5350d06-d165-44a7-badf-27149714e1d4",
    "temperature": 2505.440185546875,
    "timestamp": "2020-06-27T20:46:30.222+0200"
}
```

To log to a file:
```
oliver@canopus:~/repos/recorder$ ./build/bin/logger 12345 > sensor.log
```
Note that status messages are written to stderr.

## Notes
The sensors transmit a timestamp without an associated timezone. The log process
uses the machine local timezone.
