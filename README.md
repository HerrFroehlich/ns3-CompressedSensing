# README #

## What is this repository for? ##
We want to make use of compressed sensing techniques to exploit temporal and spatial correlation in data of large sensor networks. By creating new models for the network simulator ns3 and by using the kl1p libary, a simulation environment shall be formed. The source files in this repo consist of already existing, default  ns3 models and our own developed models. A doxygen is present, but currently it is too big for uploading.

## Installing ##
1. download ns3-26 from [https://www.nsnam.org/ns-3-26/download/](URL)
2. clone/copy the data from this repo in to *.../ns3-allinone/ns-3.26/*
3. in .../ns3-allinone/ns-3.26/ (= work directory) run:  
	*./waf configure -d optimized --enable-examples*  
	OR for a debug build:  
	*./waf configure --enable-examples*
4. to build run  
	*./waf build*

### 	Rebuilding 3rd party libraries ###
The matio and kl1p libraries where build with Ubuntu 16.04 (64bit). Thus it might be necessary to rebuild them when using a different system.
##### kl1p #####
The kl1p library source files can be found in the work directory (*./ns3-allinone/ns-3.26/*) under *libs_additional/KL1p-0.4.2*.
Here in the *build* directory choose the correct build script for your system (windows/macos/unix, x86/x64) and run it.
The output can be found in the *bin* folder in a subdirectory.
Finally copy those files to *ns3-allinone/ns-3.26/src/compressed-sensing/lib/*.
##### matio #####
The matio library source files can be found in the work directory (*ns3-allinone/ns-3.26/*) under *libs_additional/matio-1.5.10*.
Here first create an output directory with e.g:  
    *mkdir out*  
To configure run:  
    *./configure --prefix=ABS_PATH/out --enable-shared*  
where *ABS_PATH* is the absoulte path to *libs_additional/matio-1.5.10*.
Then run:   
    *make*
And finally:    
    *make install*
The output can be fund under *out/lib*. Place *libmatio.a* and *libmatio.la* in *ns3-allinone/ns-3.26/src/compressed-sensing/lib/*.
The *.so* files have to be put under *ns3-allinone/ns-3.26/build*. Be aware that when you run *./waf clean* those will be deleted also and have to be recopied! 
### Doxygen ###
To change the configuration edit in the work directory *doc/doxygen.conf*
To build the doxygen run (doxygen must have been installed)    
    *./waf doxygen*  
The output can be found  under *doc/html* or (if set in the config) *doc/latex*.
The start page for the html version is *doc/html/index.html*.

## Running Scripts ##
To run a script *cd* to the work directory (*ns3-allinone/ns-3.26/*) and use the following syntax    
    *./waf --run "SCRIPT --flags"*  
where *SCRIPT* is the name of the program and *--flags* are its corresponding flag options.

### Creating Data / Evaluation ###
Simulation data is written and read from MATLAB files (*.mat*).
There are several scripts in the work direcory to create data in */src_additional/Matlab/createData*.
Currently we use sparse signals (spatially&temporally) in the DCT, to create those use with MATLAB the script *createDCT2d*.
Scripts to evaluate the simulation output are found under */src_additional/Matlab/eval*.

## Progress so far ##
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
Using kl1p this module provides several classes needed for using compressive sensing in a network    
 * Reconstructor
    * reconstructs spatially and temporally compressed data
    * uses RandomMatrix, TransMatrix, CsAlgorithm (those are forming the API kl1p <=> ns3)
 * Applications
    * Source Application CsSrcApp: compresses temporally in measurement intervalls
    * Cluster Head Application CsClusterApp:
        * does network coding of the data from the source nodes ("spatial compression")
        * recombines optionally incoming data from other cluster heads
 * Network
    * CsNode: extension of the ns3::Node to differ Tx/Rx net devices, node IDs, etc...
    * CsCluster: represents a cluster of several CsNode instances
 * Utils/ Helpers:
    * MatFileHandler: Reading and writing from MATLAB files(*.mat*)
    * CsClusterSimpleHelper: Creates a CsCluster with MySimpleChannel and MySimpleNetDevice instances for all Nodes
    * TopologySimpleHelper: Connects CsCluster instances to each other and a sink (CsNode) with MySimpleChannel and  MySimpleNetDevice instances for all cluster heads/sink
 * Scripts:
    * single-cs-cluster-example : A single cluster connected to a sink with several source nodes
    * scratch/tree: 3 clusters connected in a tree topology:  
    * C --- C ---S   
      C --- |
    * cratch/diamond : 3 clusters connected in a diamond topology:  
    * C --- C --- S   
      | --- C ---|
