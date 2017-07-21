
#define DEFAULT_NOF_SRCNODES 100
#define DEFAULT_CHANNELDELAY_MS 1
#define DEFAULT_DRATE_BPS 1000000 // 1Mbitps
#define DEFAULT_N 256
#define DEFAULT_M 128
#define DEFAULT_L 32
#define DEFALUT_DIR "./IOdata"

#define TXPROB_MODIFIER 1.1

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/simple-sink-app.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SingleCsCluster");

std::string dir = "./IOdata";

static void
compressCb(arma::Mat<double> matIn, arma::Mat<double> matOut)
{

	NS_LOG_INFO("\n"<<Simulator::Now() << " Node " << Simulator::GetContext() << " compressed.");
	static uint32_t count = 0;
	matIn.save(dir + "/_notcompress" + std::to_string(count), arma::csv_ascii);
	matOut.save(dir + "/_compress" + std::to_string(count++), arma::csv_ascii);
}

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	NS_LOG_INFO("\n"<<Simulator::Now() << " Node " << Simulator::GetContext() << " Received:");
	p->Print(cout);
}

static void
transmittingCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	NS_LOG_INFO("\n"<<Simulator::Now() << " Node " << Simulator::GetContext() << " Sends:");
	p->Print(cout);
}

int main(int argc, char *argv[])
{

	/*********  Command line arguments  **********/

	uint32_t nSrcNodes = DEFAULT_NOF_SRCNODES,
			 dataRate = DEFAULT_DRATE_BPS,
			 n = DEFAULT_N,
			 m = DEFAULT_M,
			 l = DEFAULT_L;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS;
	bool verbose = false,
		 info = false;

	CommandLine cmd;

	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("nSrcNodes", "NOF source nodes in topology", nSrcNodes);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("channelDelay", "delay of all channels [ms]", channelDelayTmp);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("l", "NOF meas. vectors after spatial compression, rows of Z", l);
	cmd.AddValue("dir", "Directory to put I/O files to", dir);

	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);
	/*********  Logging  **********/
	if (verbose)
	{
		LogComponentEnableAll(LOG_LEVEL_ERROR);
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsClusterApp", LOG_LEVEL_FUNCTION);
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
	CsNodeContainer cluster;

	NS_LOG_INFO("Creating cluster...");
	CsClusterSimpleHelper clusterHelper;

	clusterHelper.SetCompression(n, m, l);
	clusterHelper.SetChannelAttribute("Delay", TimeValue(channelDelay));
	clusterHelper.SetSrcDeviceAttribute("DataRate", DataRateValue(dataRate));
	clusterHelper.SetClusterDeviceAttribute("DataRate", DataRateValue(dataRate));

	double txProb = TXPROB_MODIFIER * l / nSrcNodes;
	clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));

	Ptr<Compressor<double>> comp = CreateObject<Compressor<double>>();
	comp->TraceConnectWithoutContext("Complete", MakeCallback(&compressCb));
	clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));

	cluster = clusterHelper.Create(0, nSrcNodes, dir + "/x");
	ApplicationContainer clusterApps = clusterHelper.GetFirstApp(cluster);

	//add trace sources to apps
	std::string confPath = "/NodeList/*/ApplicationList/0/$CsSrcApp/"; //for all nodes add a tx callback
	Config::ConnectWithoutContext(confPath + "Tx", MakeCallback(&transmittingCb));
	confPath = "/NodeList/0/ApplicationList/0/$CsClusterApp/"; //for cluster node add a rx callback
	Config::ConnectWithoutContext(confPath + "Rx", MakeCallback(&receiveCb));

	//sink node
	CsNode sink;
	/*********  create netdevices and channels  **********/
	NS_LOG_INFO("Connect to sink...");

	Ptr<MySimpleChannel> channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));
	Ptr<MySimpleNetDevice> devA = CreateObject<MySimpleNetDevice>(),
						   devB = CreateObject<MySimpleNetDevice>();

	devA->SetAttribute("DataRate", DataRateValue(dataRate));
	devB->SetAttribute("DataRate", DataRateValue(dataRate));
	// channel->Add(devA);
	// channel->Add(devB);

	Ptr<CsNode> clusterNode = cluster.Get(0);
	clusterNode->AddTxDevice(devA);
	sink.AddDevice(devB);

	devA->SetNode(clusterNode);
	devA->SetChannel(channel);
	devB->SetNode(&sink);
	devB->SetChannel(channel);
	// MySimpleNetDeviceHelper hSrcToCluster, hRelayToSink;
	// Ptr<RateErrorModel> errModSrc_ptr, errModRelay_ptr;

	// hSrcToCluster.SetDeviceAttribute("DataRate", DataRateValue(dataRate));
	// // hSrcToCluster.SetDeviceAttribute("ReceiveErrorModel", PointerValue(errModSrc_ptr));
	// hSrcToCluster.SetChannelAttribute("Delay", TimeValue(channelDelay));
	// tmp.Add(cluster.Get(0));
	// tmp.Add(srcNodes.Get(0));
	// hSrcToCluster.Install(tmp);
	/*********  Install the applications  **********/

	NS_LOG_INFO("Adding Applications...");

	SimpleSinkApp sinkApp(nSrcNodes, cout);
	sinkApp.TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp.Setup(&sink);

	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	clusterApps.Start(Seconds(0.));
	//	Simulator::Stop(Seconds(30));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}