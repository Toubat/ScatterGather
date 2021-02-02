# ScatterGather
The ScatterGather service is a software simulation of an online storage system such as OneDrive or Box. It has a basic protocol by which you communicate with it through a series of messages sent across a network. You will communicate with this service and submit and retrieve data in form of packets.

Implemented file OS operations (open, read, write, close, …) that enables creation and processing of bytes data based on the commands of structured data packets made up of operation code, data block, local/remote node ID and sequence, etc.

A LRU cache is developed to speedup data transmission process, which achieved 75.04% hit rate in 10,000 operations.

(Originated from PSU CMPSC 311 Course Project, any form of referencing or copying is strongly prohibited)


