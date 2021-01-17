# ScatterGather
A peer-to-peer device driver in C that allows communication with external nodes by sending and receiving data packets.
Implemented file OS operations (open, read, write, close, …) that enables creation and processing of bytes data based on the commands of structured data packets made up of operation code, data block, local/remote node ID and sequence, etc.
A LRU cache is developed to speedup data transmission process, which achieved 75.04% hit rate in 10,000 operations.


