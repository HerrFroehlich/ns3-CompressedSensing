/**
* \file three-cs-cluster-example
*
* \author Tobias Waurick
* \date 23.08.17
*		
*		c1-----|	
* c0-----|	   S
* 		c2-----|
*/

#define DEFAULT_NOF_SRCNODES 85
#define DEFAULT_CHANNELDELAY_MS 1
#define DEFAULT_DRATE_BPS 0 // 1Mbitps
#define DEFAULT_N 512
#define DEFAULT_M 64
#define DEFAULT_L 32
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_K 5
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
#include "ns3/topology-simple-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("ThreeCsCluster");

static uint32_t tTemp_glob = 0, tSpat_glob = 0, nErrorRec_glob = 0, nTx_glob = 0, nTxCl_glob = 0;
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
	nTx_glob++;
}

static void
transmittingCbCl(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);
	nTxCl_glob++;
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
			 solver = 0,
			 n = DEFAULT_N,
			 m = DEFAULT_M,
			 l0 = DEFAULT_L,
			 l1 = DEFAULT_L,
			 l2 = DEFAULT_L,
			 nc0 = DEFAULT_L,
			 nc1 = DEFAULT_L,
			 nc2 = DEFAULT_L,
			 k = DEFAULT_K,
			 ks = DEFAULT_K,
			 maxIter = DEFAULT_ITER,
			 minP = 0;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   err02 = 0.0,
		   err01 = 0.0,
		   tol = DEFAULT_TOL,
		   noiseVar = 0.0,
		   mu = TXPROB_MODIFIER_DEFAULT;
	bool noprecode = false,
		 calcSnr = false,
		 nonc = false,
		 ncBern = false,
		 notemp = false,
		 bernSpat = false,
		 identSpat = false;

	std::string matInPath = DEFAULT_FILE,
				matOutPath = "",
				srcMatrixName = DEFAULT_SRCMAT_NAME;

	uint64_t seed = 1;

	CommandLine cmd;

	cmd.AddValue("bern", "Bernoulli random matrix when compressing spatially?", bernSpat);
	cmd.AddValue("ident", "Identity random matrix when compressing spatially?", identSpat);
	cmd.AddValue("channelDelay", "delay of all channels in ms", channelDelayTmp);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("err02", "Probability of between cluster head 0 and 2 (uniform rate error model)", err02);
	cmd.AddValue("err01", "Probability of between cluster head 1 and 2 (uniform rate error model)", err01);
	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("iter", "Maximum NOF iterations for solver", maxIter);
	cmd.AddValue("k", "sparsity of original source measurements (needed when using OMP temporally)", k);
	cmd.AddValue("ks", "sparsity of the colums of Y (needed when using OMP spatially)", ks);
	cmd.AddValue("l0", "NOF meas. vectors after spatial compression, rows of Z of cluster 0", l0);
	cmd.AddValue("l1", "NOF meas. vectors after spatial compression, rows of Z of cluster 1", l1);
	cmd.AddValue("l2", "NOF meas. vectors after spatial compression, rows of Z of cluster 1", l2);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("minP", "Minimum NOF packets at sink to start reconstruction", minP);
	cmd.AddValue("mu", "Tx probability modifier", mu);
	cmd.AddValue("nc0", "NOF network coded packets per link in each inverval at cluster head 0", nc0);
	cmd.AddValue("nc1", "NOF network coded packets per link in each inverval at cluster head 1", nc1);
	cmd.AddValue("nc2", "NOF network coded packets per link in each inverval at cluster head 2", nc2);
	cmd.AddValue("ncBern", "Use bernoulli nc coefficients?", ncBern);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("nNodes", "NOF nodes per cluster", nNodes);
	cmd.AddValue("noise", "Variance of noise added artificially", noiseVar);
	cmd.AddValue("nonc", "Disable network coding?", nonc);
	cmd.AddValue("notemp", "Disable temporal reconstruction?", notemp);
	cmd.AddValue("noprecode", "Disable spatial precoding?", noprecode);
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

	if ((l0 > nNodes) || (l1 > nNodes) || (l2 > nNodes))
	{
		cout << "l must be <= nNodes!" << endl;
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
	MatFileHandler matHandler;
	matHandler.OpenExisting(matInPath);
	DataStream<double> sourceData = matHandler.ReadMat<double>(srcMatrixName);
	uint32_t nMeasSeq = sourceData.GetMaxSize() / n;
	if (sourceData.GetN() < nNodes * 3)
		cout << "The input matrix " << srcMatrixName << "does not have enough columns for " << to_string(3 * nNodes) << " Nodes" << endl;

	/*********  setup CsHeader  **********/

	NS_LOG_INFO("Setting up...");

	// std::vector<uint32_t> lk(1, l);
	std::vector<uint32_t> lk;
	lk.push_back(l0);
	lk.push_back(l1);
	lk.push_back(l2);
	if (ncBern)
		CsClusterHeader::Setup(lk, CsClusterHeader::E_NcCoeffType::BERN);
	else
		CsClusterHeader::Setup(lk);

	/*********  set up clusters  **********/
	vector<Ptr<CsCluster>> clusters(3);

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
	if (identSpat)
		comp->SetRanMat(CreateObject<IdentRandomMatrix>());
	else if (bernSpat)
		comp->SetRanMat(CreateObject<BernRandomMatrix>());

	clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));

	//noise
	clusterHelper.SetSrcAppAttribute("NoiseVar", DoubleValue(noiseVar));
	clusterHelper.SetClusterAppAttribute("NoiseVar", DoubleValue(noiseVar));

	//create cluster 0

	if (nonc) // switch off nc if selected or unncessary (nc0 == l0)
		clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(false));
	else
	{
		clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(true));
		clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(nc0));
	}

	if (!noprecode)
	{
		double txProb = mu * (l0 - 1) / (nNodes - 1);
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}

	clusterHelper.SetCompression(n, m, l0);
	Ptr<CsCluster> cluster0 = clusterHelper.Create(CLUSTER_ID, nNodes, sourceData); // will remove streams from source data
	ApplicationContainer clusterApps = cluster0->GetApps();
	clusters.at(0) = cluster0;
	//create cluster 1

	clusterHelper.SetClusterAppAttribute("NcIntervalDelay", TimeValue(MilliSeconds(20) + channelDelay));

	if (nonc) // switch off nc if selected or unncessary (nc1 == l1)
	{
		clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(false));
		clusterHelper.SetClusterAppAttribute("NcShuffle", BooleanValue(true));
	}
	else
	{
		clusterHelper.SetClusterAppAttribute("NcShuffle", BooleanValue(false));
		clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(true));
		clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(nc1));
	}

	if (!noprecode)
	{
		double txProb = mu * (l1 - 1) / (nNodes - 1);
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}
	clusterHelper.SetCompression(n, m, l1);

	Ptr<CsCluster> cluster1 = clusterHelper.Create(CLUSTER_ID + 1, nNodes, sourceData); // will remove streams from source data
	clusterApps.Add(cluster1->GetApps());
	clusters.at(1) = cluster1;

	//create cluster 2

	clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(nc2));
	if (!noprecode)
	{
		double txProb = mu * (l2 - 1) / (nNodes - 1);
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));
	}
	clusterHelper.SetCompression(n, m, l2);
	Ptr<CsCluster> cluster2 = clusterHelper.Create(CLUSTER_ID + 2, nNodes, sourceData); // will remove streams from source data
	clusterApps.Add(cluster2->GetApps());
	clusters.at(2) = cluster2;

	NS_LOG_INFO("Connecting...");

	/*********  CONNECT  **********/
	Ptr<CsNode> sink = CreateObject<CsNode>(CsNode::NodeType::SINK);
	TopologySimpleHelper topHelper;

	if (err02 > 0.0 || err01 > 0.0)
	{
		TopologySimpleHelper::LinksDouble links(3);
		links.SetClLink(0, 2, 1 - err02);
		links.SetClLink(0, 1, 1 - err01);
		links.SetSinkLink(2, 1);
		links.SetSinkLink(1, 1);
		topHelper.Create(clusters, sink, links);
	}
	else
	{
		TopologySimpleHelper::LinksBool links(3);
		links.SetClLink(0, 1);
		links.SetClLink(0, 2);
		links.SetSinkLink(2);
		links.SetSinkLink(1);
		topHelper.Create(clusters, sink, links);
	}

	//adding sink app
	NS_LOG_INFO("Adding Applications...");

	Ptr<CsSinkApp> sinkApp = CreateObject<CsSinkApp>();
	sink->AddApplication(sinkApp);

	Ptr<Reconstructor> rec = CreateObject<Reconstructor>();
	Ptr<TransMatrix> transMat = CreateObject<DcTransMatrix>();
	Ptr<RandomMatrix> ranMat = CreateObject<IdentRandomMatrix>();
	rec->SetAttribute("RecMatTemp", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	if (identSpat)
		ranMat = CreateObject<IdentRandomMatrix>();
	else if (bernSpat)
		ranMat = CreateObject<BernRandomMatrix>();
	else
		ranMat = CreateObject<GaussianRandomMatrix>();

	rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	//rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat)));

	if (calcSnr)
		rec->SetAttribute("CalcSnr", BooleanValue(true));

	if (nonc)
		rec->SetAttribute("NoNC", BooleanValue(true));

	if (notemp)
		rec->SetAttribute("NoRecTemp", BooleanValue(true));

	sinkApp->SetAttribute("Reconst", PointerValue(rec));

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
	//add trace sources to apps
	std::string confPath = "/NodeList/*/ApplicationList/0/$CsSrcApp/"; //for all nodes add a tx callback
	Config::ConnectWithoutContext(confPath + "Tx", MakeCallback(&transmittingCb));
	confPath = "/NodeList/*/ApplicationList/0/$CsClusterApp/"; //for cluster node add a rx callback
	Config::ConnectWithoutContext(confPath + "Rx", MakeCallback(&receiveCb));
	Config::ConnectWithoutContext(confPath + "Tx", MakeCallback(&transmittingCbCl));

	sinkApp->SetAttribute("MinPackets", UintegerValue(minP));
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

	if (!matOutPath.empty())
		matHandler.Open(matOutPath);

	matHandler.WriteCluster(*cluster0);
	matHandler.WriteCluster(*cluster1);
	matHandler.WriteCluster(*cluster2);
	matHandler.WriteValue<double>("nNodesUsed", nNodes);
	matHandler.WriteValue<double>("n", n);
	matHandler.WriteValue<double>("m", m);
	matHandler.WriteValue<double>("l0", l0);
	matHandler.WriteValue<double>("l1", l1);
	matHandler.WriteValue<double>("l2", l2);
	matHandler.WriteValue<double>("nc0", nc0);
	matHandler.WriteValue<double>("nc1", nc1);
	matHandler.WriteValue<double>("nc2", nc2);
	matHandler.WriteValue<double>("totalTimeTemp", tTemp_glob);
	matHandler.WriteValue<double>("totalTimeSpat", tSpat_glob);
	matHandler.WriteValue<double>("nErrorRec", nErrorRec_glob);
	matHandler.WriteValue<double>("nTx", nTx_glob);
	matHandler.WriteValue<double>("nTxCl", nTxCl_glob);
	matHandler.WriteValue<double>("err01", err01);
	matHandler.WriteValue<double>("err02", err02);

	matHandler.WriteValue<double>("nMeasSeq", nMeasSeq);

	return 0;
}