
#include "./simple-topology.h"
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SimpleTopology");

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	NS_LOG_INFO(Simulator::GetContext() << " " << Simulator::Now()
										<< " -Received:" << p->ToString());
}

static void
transmittingCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	NS_LOG_INFO(Simulator::GetContext() << " " << Simulator::Now()
										<< " -Sending:" << p->ToString());
}

int main(int argc, char *argv[])
{

	/*********  Command line arguments  **********/

	uint32_t nSrcNodes = MIN_NOF_SRCNODES,
			 packetSize = DEFAULT_PACKETSIZE,
			 dataRate = DEFAULT_DRATE_BPS,
			 nPackets = DEFAULT_NOF_PACKETS;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   relayDelayTmp = DEFAULT_RELAYDELAY_MS;
	bool verbose = false,
		 info = false;
	CommandLine cmd;

	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("nSrcNodes", "NOF source nodes in topology", nSrcNodes);
	cmd.AddValue("nPackets", "NOF packets per source node", nPackets);
	cmd.AddValue("packetSize", "size of each packet [bits]", packetSize);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("relayDelay", "delay when relaying a packet [ms]", relayDelayTmp);
	cmd.AddValue("channelDelay", "delay of all channels [ms]", channelDelayTmp);
	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp),
		 relayDelay = MilliSeconds(relayDelayTmp);

	if (nSrcNodes < MIN_NOF_SRCNODES)
	{
		nSrcNodes = MIN_NOF_SRCNODES;
	}

	/*********  Logging  **********/
	if (verbose)
	{
		LogComponentEnableAll(LOG_LEVEL_ERROR);
		LogComponentEnable("SimpleTopology", LOG_LEVEL_FUNCTION);
		LogComponentEnable("SimpleSrcApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleChannel", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_FUNCTION);
		Packet::EnablePrinting();
	}
	else if (info)
	{
		LogComponentEnable("SimpleTopology", LOG_LEVEL_INFO);
		LogComponentEnable("SimpleSrcApp", LOG_LEVEL_INFO);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_INFO);
		LogComponentEnable("MySimpleChannel", LOG_LEVEL_INFO);
		LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_INFO);
		Packet::EnablePrinting();
	}
	else
	{
		LogComponentEnable("SimpleTopology", LOG_LEVEL_WARN);
		LogComponentEnable("SimpleSrcApp", LOG_LEVEL_WARN);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_WARN);
		LogComponentEnable("MySimpleChannel", LOG_LEVEL_WARN);
		LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_WARN);
		
	}
	/*********  initialize nodes  **********/

	NS_LOG_INFO("Initialzing Nodes...");

	uint32_t nRelayNodes = nSrcNodes - 1;
	NodeContainer srcNodes, relayNodes, sinkNode;

	srcNodes.Create(nSrcNodes);
	relayNodes.Create(nRelayNodes);
	sinkNode.Create(1);
	/*********  create netdevices and channels  **********/

	NS_LOG_INFO("Attaching Net Devices...");
	MySimpleNetDeviceHelper hSrcToRelay, hRelayToSink;
	// Ptr<RateErrorModel> errModSrc_ptr, errModRelay_ptr;

	hSrcToRelay.SetDeviceAttribute("DataRate", DataRateValue(dataRate));
	// hSrcToRelay.SetDeviceAttribute("ReceiveErrorModel", PointerValue(errModSrc_ptr));
	hSrcToRelay.SetChannelAttribute("Delay", TimeValue(channelDelay));
	vector<uint32_t> relayDevIdx;

	hRelayToSink.SetDeviceAttribute("DataRate", DataRateValue(dataRate));
	hRelayToSink.SetChannelAttribute("Delay", TimeValue(channelDelay));
	//for each node create a individual channel and netdevice -> multiple netdevices on one node!
	for (uint32_t i = 0; i < nRelayNodes; i++)
	{
		NodeContainer tmpNodesSrcA, tmpNodesSrcB, tmpNodesRelay;
		//upcount node id
		tmpNodesSrcA.Add(srcNodes.Get(i));
		tmpNodesSrcA.Add(relayNodes.Get(i));
		hSrcToRelay.Install(tmpNodesSrcA);
		//connect also to second src
		tmpNodesSrcB.Add(srcNodes.Get(i + 1)); //since there is always one source node more
		tmpNodesSrcB.Add(relayNodes.Get(i));
		hSrcToRelay.Install(tmpNodesSrcB);
		//relay to sink
		tmpNodesRelay.Add(relayNodes.Get(i));
		tmpNodesRelay.Add(sinkNode);
		hRelayToSink.Install(tmpNodesRelay);
	}
	relayDevIdx.push_back(0); // net devices  used for receiving on each relay node
	relayDevIdx.push_back(1);

	/*********  Install the applications  **********/

	NS_LOG_INFO("Adding Applications...");

	SimpleSrcAppHelper srcAppHelper,
		relayAppHelper;
	SimpleSinkApp sinkApp(nSrcNodes, cout);

	ApplicationContainer srcApps, relayApps;
	srcAppHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
	srcAppHelper.SetAttribute("NofPackets", UintegerValue(nPackets));
	srcAppHelper.SetAttribute("RelayDelay", TimeValue(relayDelay));
	srcApps = srcAppHelper.Install(srcNodes, &transmittingCb);

	uint8_t nodeIdStart = srcAppHelper.GetNodeId();

	relayAppHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
	relayAppHelper.SetAttribute("NofPackets", UintegerValue(0));
	relayAppHelper.SetAttribute("RelayDelay", TimeValue(relayDelay));
	relayAppHelper.SetNodeId(nodeIdStart);

	relayApps = relayAppHelper.InstallRelay(relayNodes, relayDevIdx,
											&transmittingCb, &receiveCb);

	sinkApp.TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp.Setup(sinkNode.Get(0));
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	srcApps.Start(Seconds(0.));

	Simulator::Stop(Seconds(30));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}