# README #

## What is this repository for? ##
We want to make use of compressed sensing techniques to exploit temporal and spatial correlation in data of large sensor networks. By creating new models for the network simulator ns3 and by using the kl1p libary, a simulation environment shall be formed. The source files in this repo consist of already existing, default  ns3 models and our own developed models. A doxygen is present, but so far too big for uploading.

## Progress so far##
### Simple Network ###
* A simple connection scheme, where no transporation protocol is needed
* the common OSI layer structure is preserved

* Following classes/scripts where implemented(excerpt):

1. MySimpleChannel : bidirectional p2p-channel with optional delay
2. MySimpleNetDevice : net device accessing MySimpleChannel with optional error model
3. SimpleHeader : a packet header containing a 8bit nodeId and  a double data field of variable size
4. SimpleSrcApp: application, which creates random data and relays incoming packets
5. SimpleSink: application, which prints out received data
6. simple-topology-example: an example presenting a tree based graph, where packets are created from outer nodes and beeing relayed from inner nodes to a single sink


### Compressed Sensing ###
* Using kl1p this module provides several classes needed for using compressive sensing in a network
* So far a mighty class used for reconstruction of sparse data was created (Reconstructor)
* The sofar implemented classes are shown in this collaboration diagram:
![reconstructorCollab.png](https://bitbucket.org/repo/BgkAo9z/images/2703443595-reconstructorCollab.png)