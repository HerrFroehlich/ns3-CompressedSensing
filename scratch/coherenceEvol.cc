
#include "ns3/core-module.h"
#include "ns3/random-matrix.h"
#include "ns3/transform-matrix.h"
#include "ns3/nc-matrix.h"
#include "ns3/cs-cluster-header.h"
#include "ns3/mat-file-handler.h"

#define NNODES 85
#define L 32
#define NPKT 96
#define NRUN 20
#define FILE "./IOdata/coEvol.mat"

double calcMaxCorr(klab::TSmartPointer<TOperator<double>> A)
{
	double max = 0;
	for (uint32_t i = 0; i < A->n(); i++)
	{
		arma::vec coli;
		A->column(i, coli);
		for (uint32_t j = i + 1; i < A->n(); i++)
		{
			arma::vec colj;
			A->column(j, colj);
			arma::vec res = arma::abs(arma::cor(coli, colj));
			if (res(0) > max)
				max = res(0);
		}
	}
	return max;
}

int main(int argc, char *argv[])
{
	bool bern = false, ident = false, nonc = false, ncBern = false;
	std::string matFilePath = FILE;

	CommandLine cmd;
	cmd.AddValue("bern", "Bernoulli random matrix when compressing spatially?", bern);
	cmd.AddValue("ident", "Identity random matrix when compressing spatially?", ident);
	cmd.AddValue("nonc", "Disable network coding?", nonc);
	cmd.AddValue("ncBern", "Use bernoulli nc coefficients?", ncBern);
	cmd.AddValue("MATfile", "name of the Matlab file", matFilePath);
	cmd.Parse(argc, argv);


	ns3::Ptr<RandomMatrix> phi1, phi2, phi3;

	if (ident)
	{
		phi1 = CreateObject<IdentRandomMatrix>(L, NNODES);
		phi2 = CreateObject<IdentRandomMatrix>(L, NNODES);
		phi3 = CreateObject<IdentRandomMatrix>(L, NNODES);
	}
	else if (bern)
	{
		phi1 = CreateObject<BernRandomMatrix>(L, NNODES);
		phi2 = CreateObject<BernRandomMatrix>(L, NNODES);
		phi3 = CreateObject<BernRandomMatrix>(L, NNODES);
	}
	else
	{
		phi1 = CreateObject<GaussianRandomMatrix>(L, NNODES);
		phi2 = CreateObject<GaussianRandomMatrix>(L, NNODES);
		phi3 = CreateObject<GaussianRandomMatrix>(L, NNODES);
	}
	phi1->Generate(1);
	phi2->Generate(2);
	phi3->Generate(3);

	//get operator array
	kl1p::TBlockDiagonalOperator<double>::TOperatorArray blockA;
	blockA.reserve(3);
	blockA.push_back(phi1->Clone());
	blockA.push_back(phi2->Clone());
	blockA.push_back(phi3->Clone());
	klab::TSmartPointer<TOperator<double>> A = new TBlockDiagonalOperator<double>(blockA);
	A = new kl1p::TScalingOperator<double>(A, 1.0 / klab::Sqrt(A->m()));

	klab::TSmartPointer<NcMatrix> N = new NcMatrix(3 * L);
	klab::TSmartPointer<TransMatrix> Psi = new DcTransMatrix(3*NNODES);

	std::vector<uint32_t> ls(3, L);
	if (ncBern)
		CsClusterHeader::Setup(ls, CsClusterHeader::E_NcCoeffType::BERN);
	else
		CsClusterHeader::Setup(ls);

	CsClusterHeader::NcCoeffGenerator ncGen;

	arma::Mat<double> coh(NRUN, NPKT);

	for (uint32_t run = 0; run < NRUN; run++)
	{
		std::vector<int> mix(NPKT);
		if (nonc)
		{
			std::iota(std::begin(mix), std::end(mix), 0);
			random_shuffle(std::begin(mix), std::end(mix));
		}

		for (uint32_t i = 0; i < NPKT; i++)
		{
			std::vector<double> coeffs(3 * L, 0.0);
			if (nonc)
			{
				coeffs.at(mix.at(i)) = 1.0;
			}
			else
				coeffs = ncGen.Generate(3 * L);

			N->WriteRow(coeffs);
			klab::TSmartPointer<TOperator<double>> Nnorm = new kl1p::TScalingOperator<double>(N, 1.0 / klab::Sqrt(N->m()));
			coh(run, i) = calcMaxCorr(Nnorm * A *  klab::TSmartPointer<TOperator<double>>(Psi));
		}
		N->Reset();
	}

	MatFileHandler matHandler;

	matHandler.Open(matFilePath);
	matHandler.WriteMat("Coherence", coh);
	return 0;
}