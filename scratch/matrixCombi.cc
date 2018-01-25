/*
* Default solver is OMP
* Single Cluster with assymetric errors from source nodes (uniform distributed)
*/
#define DEFAULT_NOF_SRCNODES 256
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

static bool info;

static void receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	if (info)
	{
		cout << "\n"
			 << Simulator::Now() << " Node " << Simulator::GetContext() << " Received:";
		p->Print(cout);
		cout << endl;
	}
}

static void
spatRecCb(int64_t time, uint32_t iter)
{
	if (info)
	{
		cout << "Reconstructed spatially in " << time << " ms with " << iter << " iterations"
			 << "\n";
	}
}
/*-------------------------  MAIN  ------------------------------*/

int main(int argc, char *argv[])
{
	/*********  Command line arguments  **********/

	uint32_t nNodes = DEFAULT_NOF_SRCNODES,
			 n = DEFAULT_N,
			 l = DEFAULT_L,
			 k = DEFAULT_K,
			 ks = DEFAULT_K,
			 solver = 0,
			 maxIter = DEFAULT_ITER,
			 minP = 0,
			 nrx = DEFAULT_L;
	double 
		   tol = DEFAULT_TOL;
	bool bernSpat = false,
		 identSpat = false,
		 ncBern = false,
		 ncUni = false;
	std::string matInPath = DEFAULT_FILE,
				matOutPath = "",
				srcMatrixName = DEFAULT_SRCMAT_NAME;

	uint64_t seed = 1;

	CommandLine cmd;

	cmd.AddValue("bern", "Bernoulli random matrix when compressing spatially?", bernSpat);
	cmd.AddValue("ident", "Identity random matrix when compressing spatially?", identSpat);
	cmd.AddValue("iter", "Maximum NOF iterations for solver", maxIter);
	cmd.AddValue("info", "Enable info messages", info);
	cmd.AddValue("ks", "sparsity of the colums of Y", ks);
	cmd.AddValue("l", "NOF meas. vectors after spatial compression, rows of Z", l);
	cmd.AddValue("minP", "Minimum NOF packets at sink to start reconstruction", minP);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("ncBern", "Use bernoulli nc coefficients?", ncBern);
	cmd.AddValue("ncUni", "Use bernoulli nc coefficients?", ncUni);
	cmd.AddValue("nrx", "NOF packets received from NC", nrx);
	cmd.AddValue("nNodes", "NOF source nodes in topology", nNodes);
	cmd.AddValue("seed", "Global seed for random streams > 0 (except random matrices)", seed);
	cmd.AddValue("solver", "Solvers: 0=OMP | 1=BP | 2=AMP | 3=CoSaMP | 4=ROMP | 5=SP | 6=SL0 | 7=EMBP", solver);
	cmd.AddValue("tol", "Tolerance for solvers", tol);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);
	cmd.AddValue("MATin", "path to the matlab file with extension", matInPath);
	cmd.AddValue("MATout", "name of the Matlab output file (if empty = input file). Directory must exist!", matOutPath);

	cmd.Parse(argc, argv);

	Packet::EnablePrinting();

	if (l > nNodes)
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

	/*********  read matlab file  **********/
	MatFileHandler matHandler;
	matHandler.OpenExisting(matInPath);
	DataStream<double> sourceData = matHandler.ReadMat<double>(srcMatrixName);
	uint32_t nMeasSeq = sourceData.GetMaxSize() / n;
	if (sourceData.GetN() < nNodes)
		cout << "The input matrix " << srcMatrixName << "does not have enough columns for " << to_string(nNodes) << " Nodes" << endl;

	/*********  setup CsHeader  **********/
	// std::vector<uint32_t> lk(1, l);
	std::vector<uint32_t> lc;
	lc.push_back(l);

	if (ncBern)
		CsClusterHeader::Setup(lc, CsClusterHeader::E_NcCoeffType::BERN);
	else if (ncUni)
		CsClusterHeader::Setup(lc, CsClusterHeader::E_NcCoeffType::UNI);
	else
		CsClusterHeader::Setup(lc);

	/*********  create cluster  **********/
	CsClusterSimpleHelper clusterHelper;

	// temporal compressor
	Ptr<CompressorTemp> comprTemp = CreateObject<CompressorTemp>();
	Ptr<RandomMatrix> ident = CreateObject<IdentRandomMatrix>();
	comprTemp->SetAttribute("RanMatrix", PointerValue(ident));
	clusterHelper.SetSrcAppAttribute("ComprTemp", PointerValue(comprTemp));
	clusterHelper.SetClusterAppAttribute("ComprTemp", PointerValue(comprTemp));

	//network coding
	clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(true));
	clusterHelper.SetClusterAppAttribute("NcPktPerLink", UintegerValue(nrx));

	//spatial compressor
	Ptr<Compressor> comp = CreateObject<Compressor>();
	if (identSpat)
		comp->SetRanMat(CreateObject<IdentRandomMatrix>());
	else if (bernSpat)
		comp->SetRanMat(CreateObject<BernRandomMatrix>());

	clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));

	clusterHelper.SetCompression(n, n, l); //don't do temporal compression here

	//create
	Ptr<CsCluster> cluster = clusterHelper.Create(CLUSTER_ID, nNodes, sourceData);
	ApplicationContainer clusterApps = cluster->GetApps();

	//error model
	//uint32_t headNodeId = cluster->GetClusterHead()->GetNodeId();

	//sink node
	Ptr<CsNode> sink = CreateObject<CsNode>();
	/*********  create netdevices and channels  **********/
	NS_LOG_INFO("Connect to sink...");

	Ptr<MySimpleChannel> channel = CreateObject<MySimpleChannel>();
	Ptr<MySimpleNetDevice> devA = CreateObject<MySimpleNetDevice>(),
						   devB = CreateObject<MySimpleNetDevice>();

	Ptr<CsNode> clusterNode = cluster->GetClusterHead();
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

	if (identSpat)
		ranMat = CreateObject<IdentRandomMatrix>();
	else if (bernSpat)
		ranMat = CreateObject<BernRandomMatrix>();
	else
		ranMat = CreateObject<GaussianRandomMatrix>();

	rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
	sinkApp->SetAttribute("Reconst", PointerValue(rec));

	rec->SetAttribute("CalcSnr", BooleanValue(true));

	rec->SetAttribute("NoRecTemp", BooleanValue(true));

	switch (solver)
	{
	case 0: // OMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_OMP/k", UintegerValue(k));
		break;
	case 1: // BP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_BP>()));
		break;
	case 2: // AMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_AMP>()));
		break;
	case 3: // CoSaMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_CoSaMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_CoSaMP/k", UintegerValue(ks));
		break;
	case 4: //ROMP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_ROMP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_ROMP/k", UintegerValue(ks));
		break;
	case 5: //SP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_SP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_SP/k", UintegerValue(ks));
		break;
	case 6: // SL0
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_SL0>()));
		break;
	case 7: // EMBP
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat", PointerValue(CreateObject<CsAlgorithm_EMBP>()));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_EMBP/k", UintegerValue(ks));
		break;

	default:
		cout << "Invalid solver!";
		return 1;
		break;
	}

	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/MaxIter", UintegerValue(maxIter));
	Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/Tolerance", DoubleValue(tol));
	// recTemp->TraceConnectWithoutContext("RecError", MakeCallback(&recErrorCb));

	if (minP >= l)
		sinkApp->SetAttribute("MinPackets", UintegerValue(l));
	else
		sinkApp->SetAttribute("MinPackets", UintegerValue(minP));

	sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
	sinkApp->AddCluster(cluster);
	sinkApp->Setup(sink);
	//setting calbacks
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecComplete", MakeCallback(&spatRecCb));
	/*********  Running the Simulation  **********/

	NS_LOG_INFO("Starting Simulation...");
	clusterApps.Start(Seconds(0.));
	Simulator::Run();
	Simulator::Destroy();

	/*********  Writing output **********/
	for (auto it = cluster->Begin(); it != cluster->End(); it++)
	{
		(*it)->RmStreamByName(CsNode::STREAMNAME_UNCOMPR);
		(*it)->RmStreamByName(CsNode::STREAMNAME_COMPR);
	}

	if (!matOutPath.empty())
		matHandler.Open(matOutPath);

	matHandler.WriteCluster(*cluster);
	matHandler.WriteValue<double>("nNodesUsed", nNodes);
	matHandler.WriteValue<double>("n", n);
	matHandler.WriteValue<double>("nrx", nrx);
	matHandler.WriteValue<double>("l", l);
	if (minP >= l)
		matHandler.WriteValue<double>("attempts", 1);
	else
		matHandler.WriteValue<double>("attempts", l - minP);
	matHandler.WriteValue<double>("nMeasSeq", nMeasSeq);

	return 0;
}