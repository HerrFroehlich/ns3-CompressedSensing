
#define MIN_NOF_SRCNODES	3
#define DEFAULT_CHANNELDELAY_MS	1
#define DEFAULT_DRATE_BPS		1000000 // 1Mbitps

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-src-app.h"
#include "ns3/cs-node.h"
#include "ns3/my-simple-net-device-helper.h"
#include "ns3/simple-src-app-helper.h"
#include "ns3/simple-sink-app.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SingleCsCluster");

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	NS_LOG_INFO(Simulator::GetContext() << " " << Simulator::Now()
										<< " -Received:" << p->ToString());
}

// static void
// transmittingCb(Ptr<const Packet> p)
// {
// 	NS_LOG_FUNCTION(p);
// 	NS_LOG_INFO(Simulator::GetContext() << " " << Simulator::Now()
// 										<< " -Sending:" << p->ToString());
// }

int main(int argc, char *argv[])
{

	/*********  Command line arguments  **********/

	uint32_t nSrcNodes = MIN_NOF_SRCNODES,
			 dataRate = DEFAULT_DRATE_BPS;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS;
	bool verbose = false,
		 info = false;
	CommandLine cmd;

	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("nSrcNodes", "NOF source nodes in topology", nSrcNodes);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("channelDelay", "delay of all channels [ms]", channelDelayTmp);
	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);
	/*********  Logging  **********/
	if (verbose)
	{
		LogComponentEnableAll(LOG_LEVEL_ERROR);
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleChannel", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_FUNCTION);
		Packet::EnablePrinting();
	}
	else if (info)
	{
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_INFO);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_INFO);
		LogComponentEnable("SimpleSinkApp", LOG_LEVEL_INFO);
		Packet::EnablePrinting();
	}
	else
	{
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_WARN);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_WARN);
		
	}
	/*********  initialize nodes  **********/

	NS_LOG_INFO("Initialzing Nodes...");

	NodeContainer  tmp;
	Ptr<Node> sinkNode = CreateObject<Node>();
	CsNodeContainer srcNodes;
	srcNodes.Create(CsNode::NodeType::SOURCE,1);
	// srcNodes.Create(nSrcNodes);
	//sinkNode.Create(1);
	/*********  create netdevices and channels  **********/
	NS_LOG_INFO("Attaching Net Devices...");

	Ptr<MySimpleChannel> channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));
	Ptr<MySimpleNetDevice> devA = CreateObject<MySimpleNetDevice>(),
						   devB = CreateObject<MySimpleNetDevice>();

	devA->SetAttribute("DataRate", DataRateValue(dataRate));
	devB->SetAttribute("DataRate", DataRateValue(dataRate));
	// channel->Add(devA);
	// channel->Add(devB);
	

	Ptr<CsNode> src  = srcNodes.Get(0);
	src->AddTxDevice(devA);
	sinkNode->AddDevice(devB);

 	 devA->SetNode (src);
  	devA->SetChannel (channel);
 	 devB->SetNode (sinkNode);
  	devB->SetChannel (channel);
	// MySimpleNetDeviceHelper hSrcToCluster, hRelayToSink;
	// Ptr<RateErrorModel> errModSrc_ptr, errModRelay_ptr;

	// hSrcToCluster.SetDeviceAttribute("DataRate", DataRateValue(dataRate));
	// // hSrcToCluster.SetDeviceAttribute("ReceiveErrorModel", PointerValue(errModSrc_ptr));
	// hSrcToCluster.SetChannelAttribute("Delay", TimeValue(channelDelay));
	// tmp.Add(sinkNode.Get(0));
	// tmp.Add(srcNodes.Get(0));
	// hSrcToCluster.Install(tmp);
	/*********  Install the applications  **********/

	NS_LOG_INFO("Adding Applications...");

	Ptr<CsSrcApp> srcApp = CreateObject<CsSrcApp>();
	srcApp->Setup(srcNodes.Get(0), 0, 0, "./in");

	SimpleSinkApp sinkApp(1, cout);

	
	sinkApp.TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp.Setup(sinkNode);
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	srcApp->StartApplication();

//	Simulator::Stop(Seconds(30));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}