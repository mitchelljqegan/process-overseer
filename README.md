process-overseer
================
Overview
--------
This project consists of two programs, a server (`overseer`) and a client (`controller`) which are networked via BSD sockets. The `overseer` runs indefinitely, processing commands sent by `controller` clients. The `controller` only runs for an instant at a time; it is executed with varying arguments to issue commands to the `overseer`, then terminates.

Build
-----
Both the `overseer` and `controller` can be built using `make`.

Overseer Usage
--------------
- `overseer <port>` where:
  - `<port>` is the overseer port number.

Controller Usage
----------------
- `controller <address> <port> [-o out_file] [-log log_file] [-t seconds] <file> [arg...]` where:
  - `<address>` is the overseer IP address.
  - `<port>` is the overseer port number.
- `controller <address> <port> mem [pid]` where:
  - x
- `controller <address> <port> memkill <percent>` where:
  - x
