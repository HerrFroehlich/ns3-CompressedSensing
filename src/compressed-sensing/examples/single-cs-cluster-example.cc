/*
* Default solver is OMP
*/
#define DEFAULT_NOF_SRCNODES 100
#define DEFAULT_CHANNELDELAY_MS 1
#define DEFAULT_DRATE_BPS 1000000 // 1Mbitps
#define DEFAULT_N 512
#define DEFAULT_M 64
#define DEFAULT_L 96
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_K 5
// #define DEFAULT_FILEOUT "./IOdata/dataOut.mat"
#define DEFAULT_SRCMAT_NAME "X"
#define CLUSTER_ID 0
#define DEFAULT_TOL 1e-3
#define DEFAULT_ITER 1000

#define TXPROB_MODIFIER_DEFAULT 1.0

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/cs-sink-app.h"
#include "ns3/reconstructor.h"
#include "ns3/mat-file-handler.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SingleCsCluster");

uint32_t nErrorRec_glob = 0;
vector<int64_t> tSpat_glob, tTemp_glob;
vector<uint32_t> iterSpat_glob, iterTemp_glob;
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
	tTemp_glob.push_back(time);
	iterTemp_glob.push_back(iter);
}

static void
spatRecCb(int64_t time, uint32_t iter)
{
	if (info || verbose)
		cout << "Reconstructed spatially in " << time << " ms with " << iter << " iterations"
			 << "\n";
	tSpat_glob.push_back(time);
	iterSpat_glob.push_back(iter);
}
static void
recErrorCb(const klab::KException &e)
{
	tSpat_glob.push_back(-1);
	iterSpat_glob.push_back(0);
	if (info || verbose)
		cout << "Reconstruction failed with error " << e.what();
	nErrorRec_glob++;
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
			 l = DEFAULT_L,
			 k = DEFAULT_K,
			 ks = DEFAULT_K,
			 solver = 0,
			 maxIter = DEFAULT_ITER,
			 minP = 0;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   rateErr = 0.0,
		   tol = DEFAULT_TOL,
		   noiseVar = 0.0,
		   mu = TXPROB_MODIFIER_DEFAULT;
	bool noprecode = false,
		 calcSnr = false,
		 bernSpat = false,
		 identSpat = false,
		 notemp = false,
		 nc = false,
		 onlyprecode = false;
	std::string matInPath = DEFAULT_FILE,
				matOutPath = "",
				srcMatrixName = DEFAULT_SRCMAT_NAME;

	uint64_t seed = 1;

	CommandLine cmd;

	cmd.AddValue("bern", "Bernoulli random matrix when compressing spatially?", bernSpat);
	cmd.AddValue("ident", "Identity random matrix when compressing spatially?", identSpat);
	cmd.AddValue("channelDelay", "delay of all channels in ms", channelDelayTmp);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("iter", "Maximum NOF iterations for solver", maxIter);
	cmd.AddValue("k", "sparsity of original source measurements", k);
	cmd.AddValue("ks", "sparsity of the colums of Y", ks);
	cmd.AddValue("l", "NOF meas. vectors after spatial compression, rows of Z", l);
	cmd.AddValue("mu", "Tx probability modifier", mu);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("minP", "Minimum NOF packets at sink to start reconstruction", minP);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("nc", "Enable network coding recombinations of clusterheads?", nc);
	cmd.AddValue("nNodes", "NOF source nodes in topology", nNodes);
	cmd.AddValue("noprecode", "Disable spatial precoding?", noprecode);
	cmd.AddValue("notemp", "Disable temporal reconstruction?", notemp);
	cmd.AddValue("noise", "Variance of noise added artificially", noiseVar);
	cmd.AddValue("onlyprecode", "Do only spatial precoding? Switches off NC completly at cluster heads. ", onlyprecode);
	cmd.AddValue("rateErr", "Probability of uniform rate error model", rateErr);
	cmd.AddValue("seed", "Global seed for random streams > 0 (except random matrices)", seed);
	cmd.AddValue("snr", "calculate snr directly, reconstructed signals won't be output", calcSnr);
	cmd.AddValue("solver", "Solvers: 0=OMP | 1=BP | 2=AMP | 3=CoSaMP | 4=ROMP | 5=SP | 6=SL0 | 7=EMBP", solver);
	cmd.AddValue("tol", "Tolerance for solvers", tol);
	cmd.AddValue("verbose", "Verbose Mode", verbose);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);
	cmd.AddValue("MATin", "path to the matlab file with extension", matInPath);
	cmd.AddValue("MATout", "name of the Matlab output file (if empty = input file). Directory must exist!", matOutPath);

	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);

	if (l > nNodes)
	{
		cout << "l must be <= nNodes!" << endl;
		return 1;
	}

	if (onlyprecode && noprecode)
	{
		cout << "Can't disable precoding  and do only precoding!" << endl;
		return 1;
	}

	if (seed == 0)
	{
		cout << "Seed must be > 0" << endl;
		return 1;
	}
	else //set seed
		RngSeedManager::SetSeed(seed);
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
	MatFileHandler matHandler;
	matHandler.OpenExisting(matInPath);
	DataStream<double> sourceData = matHandler.ReadMat<double>(srcMatrixName);
	uint32_t nMeasSeq = sourceData.GetMaxSize() / n;

	//uint32_t k = matHandler.ReadValue<double>(kName); // casting double  to uint32_t
	//matHandler.Open(matInPathOut);				   // open output file
	/*********  setup CsHeader  **********/

	NS_LOG_INFO("Setting up...");

	// std::vector<uint32_t> lk(1, l);
	std::vector<uint32_t> lc;
	if (onlyprecode) // in this case l = N
		lc.push_back(nNodes);
	else
		lc.push_back(l);
	CsClusterHeader::Setup(lc);

	/*********  create cluster  **********/
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

	if (!noprecode)
	{
		double txProb = mu * (l - 1) / ((nNodes - 1) * (1 - rateErr));
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}

	//noise
	clusterHelper.SetSrcAppAttribute("NoiseVar", DoubleValue(noiseVar));

	//network coding
	clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(nc));
	clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(l));

	//spatial compressor
	if (onlyprecode)
		clusterHelper.SetClusterAppAttribute("ComprSpatEnable", BooleanValue(false));
	else
	{
		Ptr<Compressor> comp = CreateObject<Compressor>();
		comp->TraceConnectWithoutContext("Complete", MakeCallback(&compressCb));
		if (identSpat)
			comp->SetRanMat(CreateObject<IdentRandomMatrix>());
		else if (bernSpat)
			comp->SetRanMat(CreateObject<BernRandomMatrix>());

		clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));
	}

	if (onlyprecode) // in this case l = N
		clusterHelper.SetCompression(n, m, nNodes);
	else
		clusterHelper.SetCompression(n, m, l);

	//error model
	Ptr<RateErrorModel> errModel = CreateObject<RateErrorModel>();
	if (rateErr > 0.0)
	{
		errModel->SetRate(rateErr);
		errModel->SetUnit(RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
		// errModel->AssignStreams(0);
		clusterHelper.SetSrcDeviceAttribute("ReceiveErrorModel", PointerValue(errModel));
		clusterHelper.SetClusterDeviceAttribute("ReceiveErrorModel", PointerValue(errModel));
	}

	//create
	Ptr<CsCluster> cluster = clusterHelper.Create(CLUSTER_ID, nNodes, sourceData);
	ApplicationContainer clusterApps = cluster->GetApps();

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
		clusterNode = cluster->GetClusterHead();
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

	rec->SetAttribute("NoNC", BooleanValue(true));

	if (identSpat || onlyprecode)
		ranMat = CreateObject<IdentRandomMatrix>();
	else if (bernSpat)
		ranMat = CreateObject<BernRandomMatrix>();
	else
		ranMat = CreateObject<GaussianRandomMatrix>();

	rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	sinkApp->SetAttribute("Reconst", PointerValue(rec));

	if (calcSnr)
		rec->SetAttribute("CalcSnr", BooleanValue(true));

	if (notemp)
		rec->SetAttribute("NoRecTemp", BooleanValue(true));

	switch (solver)
	{
	case 0: // OMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_OMP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_OMP/k", UintegerValue(ks));
		break;
	case 1: // BP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_BP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_BP>()));
		break;
	case 2: // AMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_AMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_AMP>()));
		break;
	case 3: // CoSaMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_CoSaMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_CoSaMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_CoSaMP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_CoSaMP/k", UintegerValue(ks));
		break;
	case 4: //ROMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_ROMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_ROMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_ROMP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_ROMP/k", UintegerValue(ks));
		break;
	case 5: //SP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_SP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_SP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_SP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_SP/k", UintegerValue(ks));
		break;
	case 6: // SL0
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_SL0>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_SL0>()));
		break;
	case 7: // EMBP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_EMBP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp", PointerValue(CreateObject<CsAlgorithm_EMBP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_EMBP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_EMBP/k", UintegerValue(ks));
		break;

	default:
		cout << "Invalid solver!";
		return 1;
		break;
	}

	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/MaxIter", UintegerValue(maxIter));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/MaxIter", UintegerValue(maxIter));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/Tolerance", DoubleValue(tol));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/Tolerance", DoubleValue(tol));
	// recTemp->TraceConnectWithoutContext("RecError", MakeCallback(&recErrorCb));

	if (minP >= l)
		sinkApp->SetAttribute("MinPackets", UintegerValue(l * (1 - rateErr)));
	else
		sinkApp->SetAttribute("MinPackets", UintegerValue(minP));

	sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp->AddCluster(cluster);
	sinkApp->Setup(sink);
	//setting calbacks
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecComplete", MakeCallback(&spatRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecComplete", MakeCallback(&tempRecCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecError", MakeCallback(&recErrorCb));
	Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$MySimpleNetDevice/PhyRxDrop", MakeCallback(&packetDrop));
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	clusterApps.Start(Seconds(0.));
	Simulator::Run();
	Simulator::Destroy();

	/*********  Writing output **********/
	if (calcSnr) // remove in/output streams of the cluster head/ source nodes to write less
	{
		for (auto it = cluster->Begin(); it != cluster->End(); it++)
		{
			(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
			(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
		}
	}

	if (!matOutPath.empty())
		matHandler.Open(matOutPath);

	matHandler.WriteCluster(*cluster);
	matHandler.WriteValue<double>("nNodesUsed", nNodes);
	matHandler.WriteValue<double>("n", n);
	matHandler.WriteValue<double>("m", m);
	matHandler.WriteValue<double>("l", l);
	matHandler.WriteValue<bool>("precode", !noprecode);
	matHandler.WriteValue<double>("rateErr", rateErr);
	matHandler.WriteValue<double>("noiseVar", noiseVar);
	matHandler.WriteVector<int64_t>("totalTimeTemp", tTemp_glob);
	matHandler.WriteVector<int64_t>("totalTimeSpat", tSpat_glob);
	matHandler.WriteVector<uint32_t>("totalIterTemp", iterTemp_glob);
	matHandler.WriteVector<uint32_t>("totalIterSpat", iterSpat_glob);
	matHandler.WriteValue<double>("nErrorRec", nErrorRec_glob);
	if (minP >= l)
		matHandler.WriteValue<double>("attempts", 1);
	else
		matHandler.WriteValue<double>("attempts", l - minP + 1);
	matHandler.WriteValue<double>("nMeasSeq", nMeasSeq);

	return 0;
}