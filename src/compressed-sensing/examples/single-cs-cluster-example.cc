
#define DEFAULT_NOF_SRCNODES 100
#define DEFAULT_CHANNELDELAY_MS 1
#define DEFAULT_DRATE_BPS 1000000 // 1Mbitps
#define DEFAULT_N 256
#define DEFAULT_M 64
#define DEFAULT_L 64
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_K 20
// #define DEFAULT_FILEOUT "./IOdata/dataOut.mat"
#define DEFAULT_SRCMAT_NAME "X"
#define CLUSTER_ID 0
#define DEFAULT_TOL 1e-3

#define TXPROB_MODIFIER 1.1

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/cs-sink-app.h"
#include "ns3/reconstructor.h"
#include "ns3/mat-file-handler.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SingleCsCluster");

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
}

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	if (info || verbose)
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " Received:";
	p->Print(cout);
	cout << endl;
}

static void
transmittingCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	if (info || verbose)
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " Sends:";
	p->Print(cout);
	cout << endl;
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

	uint32_t nSrcNodes = DEFAULT_NOF_SRCNODES,
			 dataRate = DEFAULT_DRATE_BPS,
			 n = DEFAULT_N,
			 m = DEFAULT_M,
			 l = DEFAULT_L,
			 k = DEFAULT_K,
			 ks = DEFAULT_K;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   rateErr = 0.0,
		   tol = 0.0;
	bool seq = false,
		 noprecode = false,
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
	cmd.AddValue("file", "path to mat file to read from", matFilePath);
	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("k", "sparsity of original source measurements (needed when using OMP temporally)", k);
	cmd.AddValue("ks", "sparsity of the colums of Y (needed when using OMP spatially)", ks);
	cmd.AddValue("l", "NOF meas. vectors after spatial compression, rows of Z", l);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("nSrcNodes", "NOF source nodes in topology", nSrcNodes);
	cmd.AddValue("noprecode", "Disable spatial precoding?", noprecode);
	cmd.AddValue("rateErr", "Probability of uniform rate error model", rateErr);
	cmd.AddValue("seq", "Reconstruct sequentially for each received packet", seq);
	cmd.AddValue("snr", "calculate snr directly, reconstructed signals won't be output", calcSnr);
	cmd.AddValue("tol", "Tolerance for solvers", tol);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);

	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);

	if (l > nSrcNodes+1)
	{
		cout << "l must be <= nSrcNodes!" << endl;
		return 1;
	}

	/*********  Logging  **********/
	if (verbose)
	{
		LogComponentEnableAll(LOG_LEVEL_WARN);
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_FUNCTION);
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
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_INFO);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_INFO);
		LogComponentEnable("CsClusterApp", LOG_LEVEL_INFO);
		LogComponentEnable("CsSinkApp", LOG_LEVEL_INFO);
		LogComponentEnable("MatFileHandler", LOG_LEVEL_INFO);
		Packet::EnablePrinting();
	}
	else
	{
		LogComponentEnableAll(LOG_LEVEL_WARN);
		LogComponentEnable("SingleCsCluster", LOG_LEVEL_WARN);
		LogComponentEnable("CsSrcApp", LOG_LEVEL_WARN);
		LogComponentEnable("CsClusterApp", LOG_LEVEL_WARN);
		LogComponentEnable("CsSinkApp", LOG_LEVEL_WARN);
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

	std::vector<uint32_t> lk(1, l);
	CsHeader::SetupCl(lk);

	/*********  create cluster  **********/
	NS_LOG_INFO("Creating cluster...");
	CsClusterSimpleHelper clusterHelper;

	clusterHelper.SetCompression(n, m, l);
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

	if (!noprecode)
	{
		double txProb = TXPROB_MODIFIER * l / (nSrcNodes * (1 - rateErr));
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}

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

	//create
	CsCluster cluster = clusterHelper.Create(CLUSTER_ID, nSrcNodes, sourceData);
	ApplicationContainer clusterApps = cluster.GetApps();

	//add trace sources to apps
	std::string confPath = "/NodeList/*/ApplicationList/0/$CsSrcApp/"; //for all nodes add a tx callback
	Config::ConnectWithoutContext(confPath + "Tx", MakeCallback(&transmittingCb));
	confPath = "/NodeList/0/ApplicationList/0/$CsClusterApp/"; //for cluster node add a rx callback
	Config::ConnectWithoutContext(confPath + "Rx", MakeCallback(&receiveCb));

	//sink node
	Ptr<CsNode> sink = CreateObject<CsNode>();
	/*********  create netdevices and channels  **********/
	NS_LOG_INFO("Connect to sink...");

	Ptr<MySimpleChannel> channel = CreateObject<MySimpleChannel>();
	channel->SetAttribute("Delay", TimeValue(channelDelay));
	Ptr<MySimpleNetDevice> devA = CreateObject<MySimpleNetDevice>(),
						   devB = CreateObject<MySimpleNetDevice>();

	devA->SetAttribute("DataRate", DataRateValue(dataRate));
	devB->SetAttribute("DataRate", DataRateValue(dataRate));
	if (rateErr > 0.0)
		devB->SetAttribute("ReceiveErrorModel", PointerValue(errModel));
	// channel->Add(devA);
	// channel->Add(devB);

	Ptr<CsNode>
		clusterNode = cluster.GetClusterNode();
	clusterNode->AddTxDevice(devA);
	sink->AddDevice(devB);

	devA->SetNode(clusterNode);
	devA->SetChannel(channel);
	devB->SetNode(sink);
	devB->SetChannel(channel);

	//adding sink app
	NS_LOG_INFO("Adding Applications...");

	Ptr<CsSinkApp> sinkApp = CreateObject<CsSinkApp>();
	sink->AddApplication(sinkApp);

	Ptr<Reconstructor> rec = CreateObject<Reconstructor>();
	Ptr<TransMatrix> transMat = CreateObject<DcTransMatrix>();
	Ptr<RandomMatrix> ranMat = CreateObject<IdentRandomMatrix>();
	rec->SetAttribute("RecMatTemp", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	ranMat = CreateObject<GaussianRandomMatrix>();
	if(ampSpat)
		ranMat->NormalizeToM();

	rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	sinkApp->SetAttribute("Reconst", PointerValue(rec));

	if(calcSnr)
		rec->SetAttribute("CalcSnr", BooleanValue(true));

	// Ptr<TransMatrix<cx_double>> transMat = CreateObject<FourierTransMatrix<cx_double>>();
	// Ptr<RandomMatrix> ranMat = CreateObject<IdentRandomMatrix>();
	// rec->SetAttribute("RecMatTempCx", PointerValue(Create<RecMatrixCx>(ranMat, transMat)));
	// ranMat = CreateObject<GaussianRandomMatrix>();
	// if(ampSpat)
	// 	ranMat->NormalizeToM();

	// rec->SetAttribute("RecMatSpatCx", PointerValue(Create<RecMatrixCx>(ranMat, transMat)));
	// sinkApp->SetAttribute("Reconst", PointerValue(rec));

	// Ptr<TransMatrix> transMat = CreateObject<FourierTransMatrix<double>>();
	// Ptr<RandomMatrix> ranMat = CreateObject<IdentRandomMatrix>();
	// rec->SetAttribute("RecMatTemp", PointerValue(Create<RecMatrixReal>(ranMat, transMat)));
	// ranMat = CreateObject<GaussianRandomMatrix>();
	// if(ampSpat)
	// 	ranMat->NormalizeToM();

	// rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrixReal>(ranMat, transMat)));
	// sinkApp->SetAttribute("Reconst", PointerValue(rec));

	if (bpSpat)
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_BP>()));
	else if (ampSpat)
	{
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_AMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSrcApp/$CsClusterApp/ComprSpat/RanMatrix/Norm", BooleanValue(true));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSrcApp/$CsSrcApp/ComprTemp/RanMatrix/Norm", BooleanValue(true));
	}

	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_OMP/k", UintegerValue(k));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_OMP/k", UintegerValue(ks));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/Tolerance", DoubleValue(tol));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/Tolerance", DoubleValue(tol));
	// recTemp->TraceConnectWithoutContext("RecError", MakeCallback(&recErrorCb));

	if (!seq)
		sinkApp->SetAttribute("MinPackets", UintegerValue(l * (1 - rateErr)));
	else
		sinkApp->SetAttribute("MinPackets", UintegerValue(0));

	sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp->AddCluster(&cluster);
	sinkApp->Setup(sink);
	//setting calbacks
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecComplete", MakeCallback(&spatRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecComplete", MakeCallback(&tempRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_OMP/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSrcApp/$CsClusterApp/ComprFail", MakeCallback(&comprFailSpat));
	Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$MySimpleNetDevice/PhyRxDrop", MakeCallback(&packetDrop));
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	clusterApps.Start(Seconds(0.));
	//	Simulator::Stop(Seconds(30));
	Simulator::Run();
	Simulator::Destroy();

	/*********  Writing output **********/
	if(calcSnr) // remove in/output streams of the cluster head/ source nodes to write less
	{
		for (auto it = cluster.Begin(); it != cluster.End(); it++)
		{
			(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
			(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
		}
	}


	matHandler_glob.WriteCluster(cluster);
	matHandler_glob.WriteValue<double>("nNodesUsed", nSrcNodes + 1);
	matHandler_glob.WriteValue<double>("n", n);
	matHandler_glob.WriteValue<double>("m", m);
	matHandler_glob.WriteValue<double>("l", l);
	matHandler_glob.WriteValue<double>("totalTimeTemp", tTemp_glob);
	matHandler_glob.WriteValue<double>("totalTimeSpat", tSpat_glob);
	matHandler_glob.WriteValue<double>("nErrorRec", nErrorRec_glob);
	matHandler_glob.WriteValue<double>("nErrorComp", nErrorComp_glob);
	if (!seq)
		matHandler_glob.WriteValue<double>("attempts", 1);
	else
		matHandler_glob.WriteValue<double>("attempts", l);
	matHandler_glob.WriteValue<double>("nMeasSeq", nMeasSeq);

	return 0;
}