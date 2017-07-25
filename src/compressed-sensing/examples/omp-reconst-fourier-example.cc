/**
* \file omp-reconst-example.cc

* \author Tobias Waurick
* \date 26.06.17
*
* An CS example where random sparse (in the frequency domain) data of several nodes is reconstructed
*/
#include "ns3/omp-reconstructor.h"
#include <iostream>
#include <stdint.h>
#include <sys/stat.h>

using namespace std;
using namespace arma;
NS_LOG_COMPONENT_DEFINE("OMP_ReconstExample");
void CreateGaussianSignal(uint32_t size, uint32_t sparsity, double mean, double sigma, arma::Col<double> &out);
void AddNoise(arma::Col<double> &out, double nVar);
void WriteToCSVFile(const arma::Col<klab::DoubleReal> &signal, const std::string &filePath);
void PrintStats(int64_t time, uint32_t iter);

int main(int argc, char *argv[])
{
	cout << "================================================== " << endl
		 << "==========  OMP RECONSTRUCTOR EXAMPLE   ==========" << endl
		 << "==================================================" << endl;
	/**================================================== *
 	* ==========  COMMAND LINE ARGUMENTS  ========== *
 	* ================================================== */
	CommandLine cmd;
	bool verbose = false, cont = false, write = false;
	uint32_t seed = 2,
			 n = 256,
			 mMin = 0;
	uint32_t nNodes = 1, ranType = 0, iter = 100;
	double alpha = 0.5, rho = 0.1, tol = 1e-3, nVar = 0, snr = INFINITY;

	cmd.AddValue("verbose", "enable verbose messages", verbose);
	cmd.AddValue("cont", "Adding data and reconstructing continously", cont);
	cmd.AddValue("mMin", "Minimum NOF sampes to start reconstructing continously", mMin);
	cmd.AddValue("seed", "initial seed. The Nodes have the following seeds: Node 0: seed, Node 1: seed+1, Node 2: seed+2 ....", seed);
	cmd.AddValue("n", "Size of the original signal x0", n);
	cmd.AddValue("nNodes", "NOF nodes [1...255] ", nNodes);
	cmd.AddValue("alpha", "Ratio of the cs-measurements [0...1]", alpha);
	cmd.AddValue("rho", "Ratio of the sparrsity of the signal x0 [0...1]", rho);
	cmd.AddValue("tol", "Tolerance of solution", tol);
	cmd.AddValue("snr", "SNR in dB", snr);
	cmd.AddValue("ranType", "Type of random sensing matrix: 0->Gaussian, 1-> Identity, 2 -> Bernoulli", ranType);
	cmd.AddValue("iter", "Maximum NOF iterations", iter);
	cmd.AddValue("write", "Write x,y and the sensing matrix to csv file in directory ./fout (must exist before)", write);
	cmd.Parse(argc, argv);

	/**================================================== *
 	* ==========  			INIT			  ========== *
 	* ================================================== */
	klab::KRandom::Instance().setSeed(seed);
	LogComponentEnable("OMP_ReconstExample", LOG_LEVEL_WARN);
	if (verbose)
	{
		LogComponentEnable("OMP_TempReconstructor", LOG_LEVEL_FUNCTION);
		LogComponentEnable("Reconstructor", LOG_LEVEL_FUNCTION);
	}
	if (seed == 0)
	{
		NS_LOG_WARN("Seed can't be 0, setting it to 1");
		seed = 1;
	}
	if (nNodes == 0)
	{
		NS_LOG_WARN("nNodes can't be 0, setting it to 1");
		nNodes = 1;
	}
	if (ranType > 2)
	{
		NS_LOG_WARN("ranType not supported, setting it to 1");
		ranType = 1;
	}
	if (alpha > 1)
	{
		NS_LOG_WARN("alpha > 1, setting it to 1");
		alpha = 1.0;
	}
	if (rho > 1)
	{
		NS_LOG_WARN("rho > 1, setting it to 1");
		rho = 1.0;
	}
	if (snr != INFINITY)
	{
		nVar = pow(10.0, (-snr / 20));
	}

	uint32_t m = alpha * n, k = rho * n;
	if (cont && ((mMin == 0) || (mMin > m)))
	{
		NS_LOG_WARN("mMin set to m/2");
		mMin = m / 2;
	}
	else if (!cont)
	{
		mMin = m;
	}

	vector<vec> xVals(nNodes), xValsN(nNodes);
	vector<cx_vec> yVals(nNodes);
	Ptr<OMP_ReconstructorTemp<cx_double>> omp = CreateObject<OMP_ReconstructorTemp<cx_double>>();
	Ptr<RandomMatrix> sensMat;
	string ranName;
	switch (ranType)
	{
	case 0:
		ranName = "Gaussian";
		sensMat = CreateObject<GaussianRandomMatrix>(m, n);
		break;
	case 1:
		ranName = "Identity";
		sensMat = CreateObject<IdentRandomMatrix>(m, n);
		break;
	case 2:
		ranName = "Bernoulli";
		sensMat = CreateObject<BernRandomMatrix>(m, n);
		break;
	}

	Ptr<TransMatrix<cx_double>> transMat = CreateObject<FourierTransMatrix<cx_double>>(n);

	omp->SetAttribute("Tolerance", DoubleValue(tol));
	omp->Setup(n, m, k, tol);
	omp->SetRanMat(sensMat);
	omp->SetAttribute("TransMatrix", PointerValue(transMat));
	omp->TraceConnectWithoutContext("RecComplete", MakeCallback(&PrintStats));

	for (uint32_t i = 0; i < nNodes; i++)
	{
		vec x0;
		cx_vec y, cx0(n);
		klab::TSmartPointer<kl1p::TOperator<cx_double>> sOp, tOp, op;

		CreateGaussianSignal(n, k, 0.0, 1.0, x0); // Create randomly the original signal x0.
		xVals.at(i) = x0;
		if (nVar > 0)
		{
			AddNoise(x0, nVar);
		}
		xValsN.at(i) = x0;

		for (klab::UInt32 i = 0; i < x0.n_rows; ++i)
			cx0[i] = std::complex<klab::DoubleReal>(x0[i], 0.0);

		sensMat->Generate(seed + i);
		sOp = klab::TSmartPointer<kl1p::TOperator<cx_double>>(*sensMat);
		tOp = transMat->Clone();
		op = sOp * tOp;
		op->apply(cx0, y);

		if (write)
		{
			cx_mat A;
			op->toMatrix(A);
			A.save("./fOut/mat" + to_string(i), csv_ascii);
			mat(*sensMat).save("./fOut/sens", csv_ascii);
			y.save("./fOut/y" + to_string(i), csv_ascii);
		}
		yVals.at(i) = y;
		omp->AddSrcNode(i, seed + i);
		omp->WriteData(i, y.memptr(), mMin);
	}
	// Display signal informations.
	cout << "==============================" << endl;
	cout << "N=" << n << " (signal size)" << endl;
	cout << "M=" << m << "=" << setprecision(5) << (alpha * 100.0) << "% (number of measurements)" << endl;
	cout << "K=" << k << "=" << setprecision(5) << (rho * 100.0) << "% (signal sparsity)" << endl;
	cout << "Using Random " << ranName << " Matrix" << endl;
	cout << "==============================" << endl;
	/**================================================== *
 	* ==========  			RECONSTRUCT		  ========== *
 	* ================================================== */
	if (cont && (mMin < m))
	{
		for (uint32_t i = 0; i < nNodes; i++)
		{
			cout << "--Reconstruction of Node " << i << " :" << endl;
			cx_vec y = yVals.at(i);
			for (uint32_t j = mMin; j < m; j++)
			{
				omp->WriteData(i, y.memptr() + j, 1);
				cout << "---Attempt with " << j + 1 << " Samples " << endl;
				vector<cx_double> data;
				omp->Reconstruct(i, k, iter);
				data = omp->ReadRecData(i);
				cx_vec cx(data.data(), n, false, false);
				vec x = real(cx);
				// cout << arma::join_rows(x, xVals.at(i));

				//SNR : 20*log(sqrt|x|²/sqrt|x-xR|²)
				cout << "SNR: " << setprecision(5) << klab::SNR(x, xVals.at(i)) << endl;
				if (write)
				{
					mat xCmp = arma::join_rows(xVals.at(i), xValsN.at(i));
					xCmp = arma::join_rows(xCmp, x);
					xCmp.save("./fOut/x" + to_string(i) + "_" + to_string(j + 1), csv_ascii);
				}
			}
		}
	}
	else
	{

		for (uint32_t i = 0; i < nNodes; i++)
		{
			vector<cx_double> data;
			cout << "--Reconstruction of Node " << i << " :" << endl;
			omp->Reconstruct(i, k, iter);
			data = omp->ReadRecData(i);
			cx_vec cx(data.data(), n, false, false);
			vec x = real(cx);
			// cout << arma::join_rows(x, xVals.at(i));
			cout << "SNR: " << setprecision(5) << klab::SNR(x, xVals.at(i)) << endl;
			if (write)
			{
				mat xCmp = arma::join_rows(xVals.at(i), xValsN.at(i));
				xCmp = arma::join_rows(xCmp, x);
				xCmp.save("./fOut/x" + to_string(i), csv_ascii);
			}
		}
	}
}

void CreateGaussianSignal(uint32_t size, uint32_t sparsity, double mean, double sigma, arma::Col<double> &out)
{
	out.set_size(size);
	out.fill(0.0);

	vector<klab::TArrayElement<double>> indices;
	for (klab::UInt32 i = 0; i < size; ++i)
		indices.push_back(klab::TArrayElement<double>(i, klab::KRandom::Instance().generateDoubleReal(0.0, 1.0)));

	partial_sort(indices.begin(), indices.begin() + klab::Min(size, sparsity), indices.end(), greater<klab::TArrayElement<double>>());

	for (klab::UInt32 i = 0; i < sparsity; ++i)
	{
		double u1 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);
		double u2 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);

		double sign = klab::KRandom::Instance().generateBool() ? -1.0 : 1.0;
		out[indices[i].i()] = sign * ((klab::Sqrt(-2.0 * klab::Log(u1)) * klab::Cos(2.0 * klab::PI * u2)) * sigma + mean);
	}
}

void AddNoise(arma::Col<double> &out, double nVar)
{
	uint32_t size = out.n_rows;
	for (uint32_t i = 0; i < size; i++)
	{
		double u3 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);
		double u4 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);
		double noise = (klab::Sqrt(-2.0 * klab::Log(u3)) * klab::Cos(2.0 * klab::PI * u4)) * nVar;
		out[i] += noise;
	}
}

void WriteToCSVFile(const arma::Col<klab::DoubleReal> &signal, const std::string &filePath)
{
	std::ofstream of(filePath.c_str());
	if (of.is_open())
	{
		for (klab::UInt32 i = 0; i < signal.n_rows; ++i)
			of << i << ";" << signal[i] << std::endl;

		of.close();
	}
	else
	{
		std::cout << "ERROR! Unable to open file \"" << filePath << "\" !" << std::endl;
	}
}

void PrintStats(int64_t time, uint32_t iter)
{
	cout << "Time needed: " << time << "ms, Iterations " << iter << endl;
}