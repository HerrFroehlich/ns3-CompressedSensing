/**
* \file omp-reconst-example.cc

* \author Tobias Waurick
* \date 26.06.17
*
* An CS example where random sparse data of several nodes is reconstructed
*/
#include "ns3/omp-reconstructor.h"
#include <iostream>
#include <stdint.h>
#include <sys/stat.h>
#include "ns3/compressor.h"

using namespace std;
using namespace arma;
NS_LOG_COMPONENT_DEFINE("OMP_ReconstExample");
void CreateGaussianSignal(uint32_t size, uint32_t sparsity, double mean, double sigma, arma::Col<double> &out);
void CreateGaussianSignal(uint32_t m, uint32_t n, uint32_t sparsity, double mean, double sigma, arma::mat &out);
void AddNoise(arma::mat &out, double nVar);
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
	uint32_t seed = 1,
			 n = 256,
			 mMin = 0;
	uint32_t len = 1, nNodes = 1, ranType = 0, iter = 100;
	double alpha = 0.5, rho = 0.1, tol = 1e-3, nVar = 0, snr = INFINITY;

	cmd.AddValue("verbose", "enable verbose messages", verbose);
	cmd.AddValue("cont", "Adding data and reconstructing continously", cont);
	cmd.AddValue("mMin", "Minimum NOF sampes to start reconstructing continously", mMin);
	cmd.AddValue("seed", "initial seed. The Nodes have the following seeds: Node 0: seed, Node 1: seed+1, Node 2: seed+2 ....", seed);
	cmd.AddValue("n", "Size of the original signal x0", n);
	cmd.AddValue("len", "length of each measurement vector", len);
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

	vector<arma::mat> xVals(nNodes), yVals(nNodes);

	Ptr<OMP_Reconstructor<double>> omp = CreateObject<OMP_Reconstructor<double>>();
	Ptr<Compressor<double>> comp = CreateObject<Compressor<double>>();
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

	// Setting up reconstructor
	omp->SetAttribute("Tolerance", DoubleValue(tol));
	omp->Setup(n, m, len, k, tol);
	omp->SetRanMat(sensMat);
	//	omp->SetAttribute("RanMatrix", PointerValue(sensMat)); //<- also possible this way
	omp->TraceConnectWithoutContext("RecComplete", MakeCallback(&PrintStats));

	// Setting up compressor
	comp->SetRanMat(sensMat);
	//	comp->SetAttribute("RanMatrix", PointerValue(sensMat)); //<- also possible this way

	for (uint32_t i = 0; i < nNodes; i++)
	{
		arma::mat x0(n, len), xN(n, len), y(m, len);

		CreateGaussianSignal(n, len, k, 0.0, 1.0, x0); // Create randomly the original signal x0.
		xVals.at(i) = x0;
		xN = x0;
		if (nVar > 0)
		{
			AddNoise(xN, nVar);
		}

		comp->Setup(seed + i, n, m, len);
		comp->Compress(xN.memptr(), n*len, y.memptr(), m*len); //we need the memptr stuff only here, later we won't work with arma::Mat's anymore!
		if (write)
		{
			mat A = *(sensMat);
			A.save("./fOut/A" + to_string(i), csv_ascii);
			y.save("./fOut/y" + to_string(i), csv_ascii);
			xN.save("./fOut/xN" + to_string(i), csv_ascii);
			x0.save("./fOut/x" + to_string(i), csv_ascii);
		}
		//we need to transpose y since the buffer is written row wise (which is needed for a real tx/rx szenario)
		//but armadillo stores the matrix data column by column!
		y = y.t();
		yVals.at(i) = y;
		omp->AddSrcNode(i, seed + i);
		omp->WriteData(i, y.memptr(), mMin * len);
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
			mat y = yVals.at(i);
			for (uint32_t j = mMin; j < m; j++)
			{
				omp->WriteData(i, y.memptr() + (j * len), len);
				cout << "---Attempt with " << j + 1 << " Samples " << endl;
				vector<double> data;
				omp->Reconstruct(i, k, iter);
				data = omp->ReadRecData(i);
				mat x(data.data(), n, len, false, false);
				// cout << arma::join_rows(x, xVals.at(i));

				//SNR : 20*log(sqrt|x|²/sqrt|x-xR|²)
				cout << "SNR: " << setprecision(5) << klab::SNR(x, xVals.at(i)) << endl;
				if (write)
				{
					x.save("./fOut/xR" + to_string(i) + "_" + to_string(j + 1), csv_ascii);
				}
			}
		}
	}
	else
	{

		for (uint32_t i = 0; i < nNodes; i++)
		{
			vector<double> data;
			cout << "--Reconstruction of Node " << i << " :" << endl;
			omp->Reconstruct(i, k, iter);
			data = omp->ReadRecData(i);
			mat x(data.data(), n, len, false, false);
			// cout << arma::join_rows(x, xVals.at(i));
			cout << "SNR: " << setprecision(5) << klab::SNR(x, xVals.at(i)) << endl;
			if (write)
			{
				x.save("./fOut/xR" + to_string(i), csv_ascii);
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

void CreateGaussianSignal(uint32_t m, uint32_t n, uint32_t sparsity, double mean, double sigma, arma::mat &out)
{
	out.set_size(m, n);

	vector<klab::TArrayElement<double>> indices;
	for (uint32_t i = 0; i < n; ++i)
	{
		arma::vec x;
		CreateGaussianSignal(m, sparsity, mean, sigma, x);
		out.col(i) = x;
	}
}

void AddNoise(arma::mat &out, double nVar)
{
	uint32_t size = out.n_elem;
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