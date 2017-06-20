/**
* \file simple
*
* \author Tobias Waurick
* \date date
*
* 2 layer tree based topology containing nodes with a simple channel&application model
* Source	Relay	Sink
* O---------|
*			O-------|	
* O---------|		|
*			O-------|
* O---------|		O
* 			...				
* O---------|		|
*			O-------|	
* O---------|
* the two border nodes connect to one relay, the remaining nodes each to two relays
* relays connect directly to sink
*/
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/my-simple-channel.h"
#include "ns3/my-simple-net-device-helper.h"
#include "ns3/simple-src-app-helper.h"
#include "ns3/simple-sink-app.h"

#define MIN_NOF_SRCNODES	3
#define NOF_BORDER_NODES	2

//default calues for command line arguments
#define DEFAULT_CHANNELDELAY_MS	1
#define DEFAULT_RELAYDELAY_MS	1
#define DEFAULT_PACKETSIZE 		17
#define DEFAULT_DRATE_BPS		1000000 // 1Mbitps
// #define DEFAULT_SRC_ERATE		0.05
// #define DEFAULT_RELAY_ERATE		0.02
#define DEFAULT_NOF_PACKETS		1
