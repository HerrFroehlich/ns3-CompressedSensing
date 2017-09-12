#define DEFAULT_K 5
#define DEFAULT_FILE "./IOdata/data.mat"
#define DEFAULT_SRCMAT_NAME "X"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/cs-cluster-simple-helper.h"
#include "ns3/cs-sink-app.h"
#include "ns3/reconstructor.h"
#include "ns3/mat-file-handler.h"
#include "ns3/topology-simple-helper.h"

using namespace ns3;
using namespace std;

/*-------------------------  MAIN  ------------------------------*/

int main(int argc, char *argv[])
{
	/*********  Command line arguments  **********/

	uint32_t k = DEFAULT_K;
	std::string matFilePath = DEFAULT_FILE,
				srcMatrixName = DEFAULT_SRCMAT_NAME;
	bool bpSolv = false,
		 ampSolv = false,
		 bernSpat = false,
		 identSpat = false;
	CommandLine cmd;

	cmd.AddValue("amp", "AMP as solver?", ampSolv);
	cmd.AddValue("bp", "Basis Pursuit as solver?", bpSolv);
	cmd.AddValue("bern", "Bernoulli random matrix when compressing spatially?", bernSpat);
	cmd.AddValue("ident", "Identity random matrix when compressing spatially?", identSpat);
	cmd.AddValue("MATsrc", "name of the matrix in the mat file containing the data for the source nodes", srcMatrixName);
	cmd.AddValue("MATfile", "name of the Matlab file", matFilePath);

	cmd.Parse(argc, argv);

	/*********  read matlab file  **********/
	MatFileHandler matHandler;

	matHandler.Open(matFilePath);
	arma::Mat<double> x;
	matHandler.ReadMat<double>(srcMatrixName, x);

	uint32_t m = x.n_rows,
			 n = x.n_cols;

	Ptr<TransMatrix> transMat = CreateObject<DcTransMatrix>(m);
	Ptr<RandomMatrix> ranMat;
	//setup reconstructor
	if (identSpat)
		ranMat = CreateObject<IdentRandomMatrix>(m, m);
	else if (bernSpat)
		ranMat = CreateObject<BernRandomMatrix>(m, m);
	else
		ranMat = CreateObject<GaussianRandomMatrix>(m, m);

	Ptr<CsAlgorithm> algo;
	if (bpSolv)
	{
		algo = CreateObject<CsAlgorithm_BP>();
	}
	else if (ampSolv)
	{
		algo = CreateObject<CsAlgorithm_AMP>();
	}
	else
	{
		algo = CreateObject<CsAlgorithm_OMP>();
		algo->SetAttribute("k", UintegerValue(k));
	}

	// compress
	arma::Mat<double> y(m, n);
	for (uint32_t i = 0; i < n; i++)
	{
		Col<double> yVec(m);
		ranMat->apply(x.col(i), yVec);
		y.col(i) = yVec;
	}

	//reconstruct
	arma::vec snr(m);
	for (uint32_t i = 0; i < m; i++)
	{
		ranMat->SetSize(i+1, m);
		klab::TSmartPointer<kl1p::TOperator<double>> Phi = ranMat->Clone();
		klab::TSmartPointer<kl1p::TOperator<double>> Psi = transMat->Clone();
		arma::Mat<double> theta = algo->Run(y.rows(0, i), Phi * Psi);
		arma::Mat<double> xrec(m, n);
		for (uint32_t i = 0; i < n; i++)
		{
			Col<double> xVec(m);
			transMat->apply(theta.col(i), xVec);
			xrec.col(i) = xVec;
		}
		snr(i) = klab::SNR(x, xrec);
		cout << "SNR with " << i+1 << "rows: " << snr(i) << endl;
	}

	matHandler.WriteMat<double>("SNR", snr);
	matHandler.WriteValue<double>("len", m);
	matHandler.WriteValue<double>("nRep", n);

	return 0;
}