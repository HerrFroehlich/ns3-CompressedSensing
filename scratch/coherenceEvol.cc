
#include "ns3/core-module.h"
#include "ns3/random-matrix.h"
#include "ns3/transform-matrix.h"
#include "ns3/nc-matrix.h"
#include "ns3/cs-cluster-header.h"
#include "ns3/mat-file-handler.h"

#define nNodes 85
#define l 32
#define nPkt 96
#define nRun 1
#define file "./IOdata/coEvol.mat"
#define ncType CsClusterHeader::E_NcCoeffType::BERN

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
	GaussianRandomMatrix phi1(l, nNodes), phi2(l, nNodes), phi3(l, nNodes);
	phi1.Generate(1);
	phi1.SetAttribute("Stream", IntegerValue(1));
	phi2.Generate(1);
	phi1.SetAttribute("Stream", IntegerValue(2));
	phi3.Generate(1);
	phi1.SetAttribute("Stream", IntegerValue(3));

	//get operator array
	kl1p::TBlockDiagonalOperator<double>::TOperatorArray blockA;
	blockA.reserve(3);
	blockA.push_back(phi1.Clone());
	blockA.push_back(phi2.Clone());
	blockA.push_back(phi3.Clone());
	klab::TSmartPointer<TOperator<double>> A = new TBlockDiagonalOperator<double>(blockA);
	A = new kl1p::TScalingOperator<double>(A, 1.0 / klab::Sqrt(A->m()));

	klab::TSmartPointer<NcMatrix> N = new NcMatrix(3 * l);

	std::vector<uint32_t> ls(3, l);
	CsClusterHeader::Setup(ls, ncType);

	CsClusterHeader::NcCoeffGenerator ncGen;

	arma::Mat<double> coh(nRun, nPkt);

	for (uint32_t run = 0; run < nRun; run++)
	{
		for (uint32_t i = 0; i < nPkt; i++)
		{
			//std::vector<double> coeffs = ncGen.Generate(3 * l);
			std::vector<double> coeffs(3 * l, 0.0);
			coeffs.at(i) = 1.0;
			N->WriteRow(coeffs);
			klab::TSmartPointer<TOperator<double>> Nnorm = new kl1p::TScalingOperator<double>(N, 1.0 / klab::Sqrt(N->m()));
			coh(run, i) = calcMaxCorr(Nnorm * A);
		}
		N->Reset();
	}

	MatFileHandler matHandler;

	matHandler.Open(file);
	matHandler.WriteMat("Coherence", coh);
	return 0;
}