/**
* \file three-cs-cluster-example
*
* \author Tobias Waurick
* \date 23.08.17
*
* c0-----|
* 		c2----s
* c1-----|		
*/

#define DEFAULT_NOF_SRCNODES 100
#define DEFAULT_CHANNELDELAY_MS 0
#define DEFAULT_DRATE_BPS 0 // 1Mbitps
#define DEFAULT_N 256
#define DEFAULT_M 64
#define DEFAULT_L 64
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_K 20
#define DEFAULT_SRCMAT_NAME "X"
#define CLUSTER_ID 0
#define DEFAULT_TOL 1e-3

#define TXPROB_MODIFIER 1.5

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/cs-sink-app.h"
#include "ns3/reconstructor.h"
#include "ns3/mat-file-handler.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("ThreeCsCluster");

MatFileHandler matHandler_glob;
uint32_t tTemp_glob = 0, tSpat_glob = 0, nErrorRec_glob = 0, nErrorComp_glob = 0;
bool verbose = false,
	 info = false;

static void
compressCb(arma::Mat<double> matIn, arma::Mat<double> matOut)
{
	NS_LOG_FUNCTION(&matIn << &matOut);
	if (info || verbose)
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " compressed.";
	matOut.save("IOdata/comp", csv_ascii);
}

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	if (info || verbose)
	{
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " Received:";
		p->Print(cout);
		cout << endl;
	}
}

static void
transmittingCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	if (info || verbose)
	{
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " Sends:";
		p->Print(cout);
		cout << endl;
	}
}

static void
tempRecCb(int64_t time, uint32_t iter)
{
	if (info || verbose)
		cout << "Reconstructed temporally in " << time << " ms with " << iter << " iterations"
			 << "\n";
	tTemp_glob += time;
}

static void
spatRecCb(int64_t time, uint32_t iter)
{
	if (info || verbose)
		cout << "Reconstructed spatially in " << time << " ms with " << iter << " iterations"
			 << "\n";
	tSpat_glob += time;
}
static void
recErrorCb(const klab::KException &e)
{
	if (info || verbose)
		cout << "Reconstruction failed with error " << e.what();
	nErrorRec_glob++;
}

static void
comprFailSpat(CsHeader::T_IdField id)
{
	if (info || verbose)
		cout << "Spatial compression failed within cluster " << static_cast<int>(id) << endl;
	nErrorComp_glob++;
}

static void
packetDrop(Ptr<const Packet> packet)
{
	if (info || verbose)
	{
		CsHeader header;
		CsHeader::T_IdField nodeId,
			clusterId;
		CsHeader::T_SeqField seq;

		packet->PeekHeader(header);
		nodeId = header.GetNodeId();
		clusterId = header.GetClusterId();
		seq = header.GetSeq();

		cout << "Packet of Node " << static_cast<int>(nodeId) << " in cluster " << static_cast<int>(clusterId) << " with SEQ " << static_cast<int>(seq)
			 << " was dropped on physical layer!" << endl;
	}
}

/*-------------------------  MAIN  ------------------------------*/

int main(int argc, char *argv[])
{
	/*********  Command line arguments  **********/

	uint32_t nNodes = DEFAULT_NOF_SRCNODES,
			 dataRate = DEFAULT_DRATE_BPS,
			 n = DEFAULT_N,
			 m = DEFAULT_M,
			 l0 = DEFAULT_L,
			 l1 = DEFAULT_L,
			 l2 = DEFAULT_L,
			 k = DEFAULT_K,
			 ks = DEFAULT_K;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   rateErr = 0.0,
		   tol = 0.0;
	bool noprecode = false,
		 bpSpat = false,
		 ampSpat = false,
		 calcSnr = false;
	std::string matFilePath = DEFAULT_FILE,
				// matFilePathOut = DEFAULT_FILEOUT,
		srcMatrixName = DEFAULT_SRCMAT_NAME;

	CommandLine cmd;

	cmd.AddValue("amp", "AMP when solving spatially?", ampSpat);
	cmd.AddValue("bp", "Basis Pursuit when solving spatially?", bpSpat);
	cmd.AddValue("channelDelay", "delay of all channels in ms", channelDelayTmp);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("k", "sparsity of original source measurements (needed when using OMP temporally)", k);
	cmd.AddValue("ks", "sparsity of the colums of Y (needed when using OMP spatially)", ks);
	cmd.AddValue("l0", "NOF meas. vectors after spatial compression, rows of Z of cluster 0", l0);
	cmd.AddValue("l1", "NOF meas. vectors after spatial compression, rows of Z of cluster 1", l1);
	cmd.AddValue("l2", "NOF meas. vectors after spatial compression, rows of Z of cluster 1", l2);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("nNodes", "NOF nodes per cluster", nNodes);
	cmd.AddValue("noprecode", "Disable spatial precoding?", noprecode);
	cmd.AddValue("rateErr", "Probability of uniform rate error model", rateErr);
	cmd.AddValue("snr", "calculate snr directly, reconstructed signals won't be output", calcSnr);
	cmd.AddValue("tol", "Tolerance for solvers", tol);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);
	cmd.AddValue("MATfile", "name of the Matlab file", matFilePath);


	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);

	if ((l0 > nNodes) || (l1 > nNodes) || (l2 > nNodes))
	{
		cout << "l must be <= nNodes!" << endl;
		return 1;
	}

	/*********  Logging  **********/
	if (verbose)
	{
		LogComponentEnableAll(LOG_LEVEL_WARN);
		LogComponentEnable("ThreeCsCluster", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsClusterApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("CsSinkApp", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleChannel", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_FUNCTION);
		LogComponentEnable("MatFileHandler", LOG_LEVEL_FUNCTION);
		Packet::EnablePrinting();
	}
	else if (info)
	{
		LogComponentEnableAll(LOG_LEVEL_WARN);
		LogComponentEnable("ThreeCsCluster", LOG_LEVEL_INFO);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_INFO);
		LogComponentEnable("CsClusterApp", LOG_LEVEL_INFO);
		LogComponentEnable("CsSinkApp", LOG_LEVEL_INFO);
		LogComponentEnable("MatFileHandler", LOG_LEVEL_INFO);
		Packet::EnablePrinting();
	}
	else
	{
		LogComponentEnableAll(LOG_LEVEL_WARN);
	}

	/*********  read matlab file  **********/
	NS_LOG_INFO("Reading mat file...");
	matHandler_glob.Open(matFilePath);
	DataStream<double> sourceData = matHandler_glob.ReadMat<double>(srcMatrixName);
	uint32_t nMeasSeq = sourceData.GetMaxSize() / n;

	//uint32_t k = matHandler_glob.ReadValue<double>(kName); // casting double  to uint32_t
	//matHandler_glob.Open(matFilePathOut);				   // open output file
	/*********  setup CsHeader  **********/

	NS_LOG_INFO("Setting up...");

	// std::vector<uint32_t> lk(1, l);
	std::vector<uint32_t> lk;
	lk.push_back(l0);
	lk.push_back(l1);
	lk.push_back(l2);
	CsClusterHeader::SetupCl(lk);

	/*********  set up common cluster properties  **********/
	NS_LOG_INFO("Creating cluster...");
	CsClusterSimpleHelper clusterHelper;

	//delay & data rate
	clusterHelper.SetChannelAttribute("Delay", TimeValue(channelDelay));
	clusterHelper.SetSrcDeviceAttribute("DataRate", DataRateValue(dataRate));
	clusterHelper.SetClusterDeviceAttribute("DataRate", DataRateValue(dataRate));
	// temporal compressor
	Ptr<CompressorTemp> comprTemp = CreateObject<CompressorTemp>();
	Ptr<RandomMatrix> ident = CreateObject<IdentRandomMatrix>();
	comprTemp->SetAttribute("RanMatrix", PointerValue(ident));
	clusterHelper.SetSrcAppAttribute("ComprTemp", PointerValue(comprTemp));
	clusterHelper.SetClusterAppAttribute("ComprTemp", PointerValue(comprTemp));
	//spatial compressor
	Ptr<Compressor> comp = CreateObject<Compressor>();
	comp->TraceConnectWithoutContext("Complete", MakeCallback(&compressCb));
	clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));
	//error model
	Ptr<RateErrorModel> errModel = CreateObject<RateErrorModel>();
	if (rateErr > 0.0)
	{
		errModel->SetRate(rateErr);
		errModel->SetUnit(RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
		errModel->AssignStreams(0);
		clusterHelper.SetSrcDeviceAttribute("ReceiveErrorModel", PointerValue(errModel));
		clusterHelper.SetClusterDeviceAttribute("ReceiveErrorModel", PointerValue(errModel));
	}
	
	clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(false)); // switch off nc for inner clusters
	//create cluster 0
	if (!noprecode)
	{
		double txProb = TXPROB_MODIFIER * (l0-1) / ((nNodes-1) * (1 - rateErr));
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}
	//NC
	clusterHelper.SetCompression(n, m, l0);
	Ptr<CsCluster> cluster0 = clusterHelper.Create(CLUSTER_ID, nNodes, sourceData); // will remove streams from source data
	ApplicationContainer clusterApps = cluster0->GetApps();
	//create cluster 1
	if (!noprecode)
	{
		double txProb = TXPROB_MODIFIER * (l1-1) / ((nNodes-1) * (1 - rateErr));
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}
	clusterHelper.SetCompression(n, m, l1);
	Ptr<CsCluster> cluster1 = clusterHelper.Create(CLUSTER_ID + 1, nNodes, sourceData); // will remove streams from source data
	clusterApps.Add(cluster1->GetApps());

	//create cluster 2
	clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(true));
	clusterHelper.SetQueueAttribute("MaxPackets", UintegerValue(l0 + l1 + l2));
	if (!noprecode)
	{
		double txProb = TXPROB_MODIFIER * (l2-1) / ((nNodes-1) * (1 - rateErr));
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}
	clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(l2+l1+l0));
	clusterHelper.SetCompression(n, m, l2);
	Ptr<CsCluster> cluster2 = clusterHelper.Create(CLUSTER_ID + 2, nNodes, sourceData); // will remove streams from source data
	clusterApps.Add(cluster2->GetApps());

	//add trace sources to apps
	std::string confPath = "/NodeList/*/ApplicationList/0/$CsSrcApp/"; //for all nodes add a tx callback
	Config::ConnectWithoutContext(confPath + "Tx", MakeCallback(&transmittingCb));
	confPath = "/NodeList/*/ApplicationList/0/$CsClusterApp/"; //for cluster node add a rx callback
	Config::ConnectWithoutContext(confPath + "Rx", MakeCallback(&receiveCb));

	NS_LOG_INFO("Connecting...");

	/*********  CONNECT  **********/
	Ptr<MySimpleChannel> channel;
	Ptr<MySimpleNetDevice> devTx, devRx;

	/*********  connect clusters  **********/
	//c0->c2
	channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));

	devTx = CreateObject<MySimpleNetDevice>();
	devRx = CreateObject<MySimpleNetDevice>();
	devTx->SetAttribute("DataRate", DataRateValue(dataRate));
	devRx->SetAttribute("DataRate", DataRateValue(dataRate));
	devTx->SetNode(cluster0->GetClusterHead());
	devTx->SetChannel(channel);
	devRx->SetNode(cluster2->GetClusterHead());
	devRx->SetChannel(channel);
	if (rateErr > 0.0)
		devRx->SetAttribute("ReceiveErrorModel", PointerValue(errModel));

	cluster0->GetClusterHead()->AddTxDevice(devTx);
	cluster2->GetClusterHead()->AddRxDevice(devRx);

	//c1->c2
	channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));

	devTx = CreateObject<MySimpleNetDevice>();
	devRx = CreateObject<MySimpleNetDevice>();
	devTx->SetAttribute("DataRate", DataRateValue(dataRate));
	devRx->SetAttribute("DataRate", DataRateValue(dataRate));
	devTx->SetNode(cluster1->GetClusterHead());
	devTx->SetChannel(channel);
	devRx->SetNode(cluster2->GetClusterHead());
	devRx->SetChannel(channel);
	if (rateErr > 0.0)
		devRx->SetAttribute("ReceiveErrorModel", PointerValue(errModel));

	cluster1->GetClusterHead()->AddTxDevice(devTx);
	cluster2->GetClusterHead()->AddRxDevice(devRx);

	/*********  connect sink  **********/
	//c2->s
	Ptr<CsNode> sink = CreateObject<CsNode>();

	channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));

	devTx = CreateObject<MySimpleNetDevice>();
	devRx = CreateObject<MySimpleNetDevice>();
	devTx->SetAttribute("DataRate", DataRateValue(dataRate));
	devRx->SetAttribute("DataRate", DataRateValue(dataRate));
	if (rateErr > 0.0)
		devRx->SetAttribute("ReceiveErrorModel", PointerValue(errModel));

	Ptr<CsNode> clusterNode = cluster2->GetClusterHead();
	clusterNode->AddTxDevice(devTx);
	sink->AddDevice(devRx);

	devTx->SetNode(clusterNode);
	devTx->SetChannel(channel);
	devRx->SetNode(sink);
	devRx->SetChannel(channel);

	//adding sink app
	NS_LOG_INFO("Adding Applications...");

	Ptr<CsSinkApp> sinkApp = CreateObject<CsSinkApp>();
	sink->AddApplication(sinkApp);

	Ptr<Reconstructor> rec = CreateObject<Reconstructor>();
	Ptr<TransMatrix> transMat = CreateObject<DcTransMatrix>();
	Ptr<RandomMatrix> ranMat = CreateObject<IdentRandomMatrix>();
	rec->SetAttribute("RecMatTemp", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	ranMat = CreateObject<GaussianRandomMatrix>();

	rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	//rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat)));
	sinkApp->SetAttribute("Reconst", PointerValue(rec));

	if (calcSnr)
		rec->SetAttribute("CalcSnr", BooleanValue(true));

	if (bpSpat)
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_BP>()));
	else if (ampSpat)
	{
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_AMP>()));
	}

	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_OMP/k", UintegerValue(k));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_OMP/k", UintegerValue(ks)); // times three since we have three clusters
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/Tolerance", DoubleValue(tol));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/Tolerance", DoubleValue(tol));
	// recTemp->TraceConnectWithoutContext("RecError", MakeCallback(&recErrorCb));
	
	sinkApp->SetAttribute("MinPackets", UintegerValue(0));

	sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp->AddCluster(cluster2);
	sinkApp->AddCluster(cluster0);
	sinkApp->AddCluster(cluster1);
	sinkApp->Setup(sink);
	//setting calbacks
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecComplete", MakeCallback(&spatRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecComplete", MakeCallback(&tempRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSrcApp/$CsClusterApp/ComprFail", MakeCallback(&comprFailSpat));
	Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$MySimpleNetDevice/PhyRxDrop", MakeCallback(&packetDrop));

	sinkApp->SetAttribute("MinPackets", UintegerValue(l0+l1+l2));
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	clusterApps.Start(Seconds(0.));
	Simulator::Run();
	Simulator::Destroy();

	/*********  Writing output **********/
	if (calcSnr) // remove in/output streams of the cluster head/ source nodes to write less
	{
		for (auto it = cluster0->Begin(); it != cluster0->End(); it++)
		{
			(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
			(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
		}
		for (auto it = cluster1->Begin(); it != cluster1->End(); it++)
		{
			(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
			(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
		}
		for (auto it = cluster2->Begin(); it != cluster2->End(); it++)
		{
			(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
			(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
		}
	}

	matHandler_glob.WriteCluster(*cluster0);
	matHandler_glob.WriteCluster(*cluster1);
	matHandler_glob.WriteCluster(*cluster2);
	matHandler_glob.WriteValue<double>("nNodesUsed", nNodes);
	matHandler_glob.WriteValue<double>("n", n);
	matHandler_glob.WriteValue<double>("m", m);
	matHandler_glob.WriteValue<double>("l0", l0);
	matHandler_glob.WriteValue<double>("l1", l1);
	matHandler_glob.WriteValue<double>("l2", l2);
	matHandler_glob.WriteValue<double>("totalTimeTemp", tTemp_glob);
	matHandler_glob.WriteValue<double>("totalTimeSpat", tSpat_glob);
	matHandler_glob.WriteValue<double>("nErrorRec", nErrorRec_glob);
	matHandler_glob.WriteValue<double>("nErrorComp", nErrorComp_glob);
	
	matHandler_glob.WriteValue<double>("nMeasSeq", nMeasSeq);

	return 0;
}