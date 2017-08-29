
#define DEFAULT_NOF_SRCNODES 256
#define DEFAULT_CHANNELDELAY_MS 1
#define DEFAULT_DRATE_BPS 1000000 // 1Mbitps
#define DEFAULT_N 512
#define DEFAULT_M 64
#define DEFAULT_L 64
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_K 5
// #define DEFAULT_FILEOUT "./IOdata/dataOut.mat"
#define DEFAULT_SRCMAT_NAME "X"
#define CLUSTER_ID 0
#define DEFAULT_TOL 1e-3

#define RXPROB_MIN 0.05
#define RXPROB_MAX 1
#define RXPROB_STEPS 38

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/cs-sink-app.h"
#include "ns3/reconstructor.h"
#include "ns3/mat-file-handler.h"

using namespace ns3;
using namespace std;

MatFileHandler matHandler_glob;

NS_LOG_COMPONENT_DEFINE("rxProbSweep");

static void
receiveCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	cout << "\n"
		 << Simulator::Now() << " Node " << Simulator::GetContext() << " Received:";
	p->Print(cout);
	cout << endl;
}

static void
transmittingCb(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(p);

	cout << "\n"
		 << Simulator::Now() << " Node " << Simulator::GetContext() << " Sends:";
	p->Print(cout);
	cout << endl;
}

static void
tempRecCb(int64_t time, uint32_t iter)
{
	cout << "Reconstructed temporally in " << time << " ms with " << iter << " iterations"
		 << "\n";
}

static void
spatRecCb(int64_t time, uint32_t iter)
{
	cout << "Reconstructed spatially in " << time << " ms with " << iter << " iterations"
		 << "\n";
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
			 ks = DEFAULT_K;
	double channelDelayTmp = DEFAULT_CHANNELDELAY_MS,
		   noiseVar = 0.0;
	std::string matFilePath = DEFAULT_FILE,
				// matFilePathOut = DEFAULT_FILEOUT,
		srcMatrixName = DEFAULT_SRCMAT_NAME;

	CommandLine cmd;

	cmd.AddValue("channelDelay", "delay of all channels in ms", channelDelayTmp);
	cmd.AddValue("dataRate", "data rate [mbps]", dataRate);
	cmd.AddValue("file", "path to mat file to read from", matFilePath);
	cmd.AddValue("k", "sparsity of original source measurements (needed when using OMP temporally)", k);
	cmd.AddValue("ks", "sparsity of the colums of Y (needed when using OMP spatially)", ks);
	cmd.AddValue("l", "NOF meas. vectors after spatial compression, rows of Z", l);
	cmd.AddValue("m", "NOF samples after temporal compression, size of Y_i", m);
	cmd.AddValue("n", "NOF samples to compress temporally, size of X_i", n);
	cmd.AddValue("noise", "Variance of noise added artificially", noiseVar);
	cmd.AddValue("nNodes", "NOF source nodes in topology", nNodes);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);
	cmd.AddValue("MATfile", "name of the Matlab file", matFilePath);

	cmd.Parse(argc, argv);

	Time channelDelay = MilliSeconds(channelDelayTmp);

	if (l > nNodes)
	{
		cout << "l must be <= nNodes!" << endl;
		return 1;
	}

	/*********  Logging  **********/

	LogComponentEnableAll(LOG_LEVEL_WARN);

	LogComponentEnable("rxProbSweep", LOG_LEVEL_INFO);
	LogComponentEnable("CsSrcApp", LOG_LEVEL_INFO);
	LogComponentEnable("CsClusterApp", LOG_LEVEL_INFO);
	LogComponentEnable("CsSinkApp", LOG_LEVEL_INFO);
	LogComponentEnable("MatFileHandler", LOG_LEVEL_INFO);
	Packet::EnablePrinting();

	//uint32_t nMeasSeq;
	Col<double> meanSnr(RXPROB_STEPS);
	Col<double> varSnr(RXPROB_STEPS);
	for (uint32_t step = 0; step < RXPROB_STEPS; step++)
	{
		NS_LOG_INFO("STEP " << step);

		/*********  read matlab file  **********/
		NS_LOG_INFO("Reading mat file...");
		matHandler_glob.Open(matFilePath);
		DataStream<double> sourceData = matHandler_glob.ReadMat<double>(srcMatrixName);
		//nMeasSeq = sourceData.GetMaxSize() / n;

		/*********  setup CsHeader  **********/

		NS_LOG_INFO("Setting up...");

		// std::vector<uint32_t> lk(1, l);
		std::vector<uint32_t> lk;
		lk.push_back(l);
		CsClusterHeader::SetupCl(lk);

		/*********  create cluster  **********/
		NS_LOG_INFO("Creating cluster...");
		CsClusterSimpleHelper clusterHelper;

		clusterHelper.SetCompression(n, m, l);
		//delay & data rate
		clusterHelper.SetChannelAttribute("Delay", TimeValue(channelDelay));
		clusterHelper.SetSrcDeviceAttribute("DataRate", DataRateValue(dataRate));
		clusterHelper.SetClusterDeviceAttribute("DataRate", DataRateValue(dataRate));
		double txProb = RXPROB_MIN + (step + 1) * (RXPROB_MAX - RXPROB_MIN) / RXPROB_STEPS;
		if (txProb <= 1 && txProb >= 0)
			clusterHelper.SetSrcAppAttribute("TxProb", DoubleValue(txProb));

		// temporal compressor
		Ptr<CompressorTemp> comprTemp = CreateObject<CompressorTemp>();
		Ptr<RandomMatrix> ident = CreateObject<IdentRandomMatrix>();
		comprTemp->SetAttribute("RanMatrix", PointerValue(ident));
		clusterHelper.SetSrcAppAttribute("ComprTemp", PointerValue(comprTemp));
		clusterHelper.SetClusterAppAttribute("ComprTemp", PointerValue(comprTemp));

		//spatial compressor
		Ptr<Compressor> comp = CreateObject<Compressor>();
		clusterHelper.SetClusterAppAttribute("ComprSpat", PointerValue(comp));

		//noise
		clusterHelper.SetSrcAppAttribute("NoiseVar", DoubleValue(noiseVar));

		//disable network coding
		clusterHelper.SetClusterAppAttribute("NcEnable", BooleanValue(false));

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
		ranMat = CreateObject<GaussianRandomMatrix>();
		rec->SetAttribute("CalcSnr", BooleanValue(true));
		rec->SetAttribute("RecMatSpat", PointerValue(Create<RecMatrix>(ranMat, transMat)));
		sinkApp->SetAttribute("Reconst", PointerValue(rec));

		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm_OMP/k", UintegerValue(k));
		Config::Set("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm_OMP/k", UintegerValue(ks));
		// recTemp->TraceConnectWithoutContext("RecError", MakeCallback(&recErrorCb));

		sinkApp->SetAttribute("MinPackets", UintegerValue(l));

		sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&receiveCb));
		sinkApp->AddCluster(cluster);
		sinkApp->Setup(sink);
		//setting calbacks
		Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoSpat/$CsAlgorithm/RecComplete", MakeCallback(&spatRecCb));
		Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$CsSinkApp/Reconst/AlgoTemp/$CsAlgorithm/RecComplete", MakeCallback(&tempRecCb));
		/*********  Running the Simulation  **********/

		NS_LOG_INFO("Running Simulation...");
		clusterApps.Start(Seconds(0.));
		Simulator::Run();
		Simulator::Destroy();

		//snr mean
		uint32_t i = 0;
		for (auto it = cluster->StreamBegin(); it != cluster->StreamEnd(); it++)
		{
			Ptr<SerialDataBuffer<double>> buf = (*it)->PeekBuffer(0);
			double snr = buf->ReadNext();
			double delta = snr - meanSnr(step);
			double snrMean = meanSnr(step) + delta / ++i;
			meanSnr(step) = snrMean;

			double delta2 = snr - snrMean;
			varSnr(step) = delta * delta2;
		}
		varSnr(step) /= i;
	}

	/*********  Writing output **********/
	matHandler_glob.WriteMat<double>("meanSnr", meanSnr);
	matHandler_glob.WriteMat<double>("varSnr", varSnr);
	matHandler_glob.WriteValue<double>("nNodesUsed", nNodes);
	matHandler_glob.WriteValue<double>("noiseVar", noiseVar);
	matHandler_glob.WriteValue<double>("n", n);
	matHandler_glob.WriteValue<double>("m", m);
	matHandler_glob.WriteValue<double>("l", l);
	matHandler_glob.WriteValue<double>("attempts", l);

	return 0;
}