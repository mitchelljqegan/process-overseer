process-overseer
================
Overview
--------
This project consists of two programs, a server (`overseer`) and a client (`controller`) which are networked via BSD sockets. The overseer runs indefinitely, processing commands sent by controller clients. The controller only runs for an instant at a time; it is executed with varying arguments to issue commands to the overseer, then terminates.

Build
-----
Both the `overseer` and `controller` can be built using `make`.

Overseer Usage
--------------
- `overseer <port>` where:
  - `port` is the overseer port number to be set.

Controller Usage
----------------
- `controller <address> <port> [-o out_file] [-log log_file] [-t seconds] <file> [arg...]` where:
  - `address` is the overseer IP address.
  - `port` is the overseer port number.
  - `out_file` is the file where the stdout and stderr of the executed `file` are to be redirected.
  - `log_file` is the file where the stdout of the overseer's management of the executed `file` is to be redirected.
  - `seconds` is the timeout for SIGTERM to be sent to the executed `file`.
  - `file` is the file to be executed.
  - `arg...` is an arbitrary quantity of arguments passed to the executed `file`.
- `controller <address> <port> mem [pid]` where:
  - `address` is the overseer IP address.
  - `port` is the overseer port number.
  - `pid` is the process identifier of the process to get memory information of.
- `controller <address> <port> memkill <percent>` where:
  - `address` is the overseer IP address.
  - `port` is the overseer port number.
  - `percent` is the percentage of memory usage required for SIGKILL to be sent to currently executing processes.
