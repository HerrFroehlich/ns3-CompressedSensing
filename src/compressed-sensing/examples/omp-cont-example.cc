/**
* \file omp-cont-example.cc
*
* \author Tobias Waurick
* \date 28.06.2017
*
* A simple example for OMP reconstruction. Data is reconstructed of y. Size of y is increased from [m/2...m] = [alpha*n/2...alpha*n];
*/
#include "compressed-sensing-example.h"
#include "ns3/core-module.h"
using namespace ns3;
using namespace kl1p;

int main(int argc, char *argv[])
{
	kl1p::RunExample();

	std::cout << "Start of KL1p compressed-sensing example." << std::endl;
	std::cout << "Try to determine a sparse vector x " << std::endl;
	std::cout << "from an underdetermined set of linear measurements y=A*x, " << std::endl;
	std::cout << "where A is a random gaussian i.i.d sensing matrix." << std::endl;

	klab::UInt32 n = 256;		  // Size of the original signal x0.
	klab::DoubleReal alpha = 0.5; // Ratio of the cs-measurements.
	klab::DoubleReal rho = 0.1;   // Ratio of the sparsity of the signal x0.
	klab::UInt64 seed = 0;		  // Seed used for random number generation (0 if regenerate random numbers on each launch).
	bool bWrite = false;		  // Write signals to files ?
	CommandLine cmd;

	cmd.AddValue("seed", "initial seed", seed);
	cmd.AddValue("n", "Size of the original signal x0", n);
	cmd.AddValue("alpha", "Ratio of the cs-measurements [0...1]", alpha);
	cmd.AddValue("rho", "Ratio of the sparsity of the signal x0 [0...1]", rho);
	cmd.AddValue("write", "Write x,y and the sensing matrix to an Ascii file", bWrite);
	cmd.Parse(argc, argv);
	// Initialize random seed if needed.
	if (seed > 0)
		klab::KRandom::Instance().setSeed(seed);

	klab::UInt32 m = klab::UInt32(alpha * n); // Number of cs-measurements.
	klab::UInt32 k = klab::UInt32(rho * n);   // Sparsity of the signal x0 (number of non-zero elements).
	// Display signal informations.
	std::cout << "==============================" << std::endl;
	std::cout << "N=" << n << " (signal size)" << std::endl;
	std::cout << "M=" << m << "=" << std::setprecision(5) << (alpha * 100.0) << "% (number of measurements)" << std::endl;
	std::cout << "K=" << k << "=" << std::setprecision(5) << (rho * 100.0) << "% (signal sparsity)" << std::endl;
	std::cout << "Random Seed=" << klab::KRandom::Instance().seed() << std::endl;
	std::cout << "==============================" << std::endl;

	arma::Col<klab::DoubleReal> x0;					// Original signal x0 of size n.
	kl1p::CreateGaussianSignal(n, k, 0.0, 1.0, x0); // Create randomly the original signal x0.

	if (bWrite)
		kl1p::WriteToCSVFile(x0, "OriginalSignal.csv"); // Write x0 to a file.

	// Create random gaussian i.i.d matrix A of size (m,n).
	klab::TSmartPointer<kl1p::TOperator<klab::DoubleReal>> A = new kl1p::TNormalRandomMatrixOperator<klab::DoubleReal>(m, n, 0.0, 1.0);
	A = new kl1p::TScalingOperator<klab::DoubleReal>(A, 1.0 / klab::Sqrt(klab::DoubleReal(m))); // Pseudo-normalization of the matrix (required for AMP and EMBP solvers).

	// Perform cs-measurements of size m.
	arma::Col<klab::DoubleReal> y;
	A->apply(x0, y);

	klab::DoubleReal tolerance = 1e-3; // Tolerance of the solution.
	arma::Col<klab::DoubleReal> x;	 // Will contain the solution of the reconstruction.

	klab::KTimer timer;

	for (int32_t i = m/2; i >= 0; i--)
	{
		std::cout << "------------------------------" << std::endl;
		std::cout << "[OMP] Start." << m-i<< std::endl;
		timer.start();
		kl1p::TOMPSolver<klab::DoubleReal> omp(tolerance);
		arma::mat Amat;
		A->toMatrix(Amat);
		klab::TSmartPointer<kl1p::TOperator<klab::DoubleReal>> Aptr = new kl1p::TMatrixOperator<double>(Amat.rows(0, m - i - 1));
		omp.solve(y.rows(0, m - i - 1), Aptr, k, x);
		timer.stop();
		std::cout << "[OMP] Done - SNR=" << std::setprecision(5) << klab::SNR(x, x0) << " - "
				  << "Time=" << klab::UInt32(timer.durationInMilliseconds()) << "ms"
				  << " - "
				  << "Iterations=" << omp.iterations() << std::endl;
		if (bWrite)
			kl1p::WriteToCSVFile(x, "OMP-Signal.csv"); // Write solution to a file.

	}
		return 0;
}

// ---------------------------------------------------------------------------------------------------- //

void kl1p::CreateGaussianSignal(klab::UInt32 size, klab::UInt32 sparsity, klab::DoubleReal mean, klab::DoubleReal sigma, arma::Col<klab::DoubleReal> &out)
{
	out.set_size(size);
	out.fill(0.0);

	std::vector<klab::TArrayElement<klab::DoubleReal>> indices;
	for (klab::UInt32 i = 0; i < size; ++i)
		indices.push_back(klab::TArrayElement<klab::DoubleReal>(i, klab::KRandom::Instance().generateDoubleReal(0.0, 1.0)));

	std::partial_sort(indices.begin(), indices.begin() + klab::Min(size, sparsity), indices.end(), std::greater<klab::TArrayElement<klab::DoubleReal>>());

	for (klab::UInt32 i = 0; i < sparsity; ++i)
	{
		klab::DoubleReal u1 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);
		klab::DoubleReal u2 = klab::KRandom::Instance().generateDoubleReal(0.0, 1.0);

		klab::DoubleReal sign = klab::KRandom::Instance().generateBool() ? -1.0 : 1.0;
		out[indices[i].i()] = sign * ((klab::Sqrt(-2.0 * klab::Log(u1)) * klab::Cos(2.0 * klab::PI * u2)) * sigma + mean);
	}
}

// ---------------------------------------------------------------------------------------------------- //

void kl1p::WriteToCSVFile(const arma::Col<klab::DoubleReal> &signal, const std::string &filePath)
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

// ---------------------------------------------------------------------------------------------------- //

void kl1p::RunExample()
{
}

// ---------------------------------------------------------------------------------------------------- //
