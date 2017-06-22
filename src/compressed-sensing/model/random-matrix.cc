/**
* \file random-matrix.cc
*
* \author Tobias Waurick
* \date 21.06.17
*
*/

#include "random-matrix.h"

/*********  Random Matrix  **********/
RandomMatrix::RandomMatrix(uint32_t m, uint32_t n)
{
	m_mat = arma::zeros<arma::mat>(m, n);
}

uint32_t RandomMatrix::nRows() const
{
	return m_mat.n_rows;
}
uint32_t RandomMatrix::nCols() const
{
	return m_mat.n_cols;
}

arma::Col<uint32_t> RandomMatrix::Dim() const
{
	arma::Col<uint32_t> size(2);
	size(0) = m_mat.n_rows;
	size(1) = m_mat.n_cols;
	return size;
}

arma::mat operator*(const RandomMatrix &lvl, const arma::mat &rvl)
{
	return lvl.m_mat * rvl;
}

arma::mat operator*(const arma::mat &lvl, const RandomMatrix &rvl)
{
	return lvl * rvl.m_mat;
}

std::ostream &operator<<(std::ostream &os, const RandomMatrix &obj)
{
	// write obj to stream
	os << obj.m_mat;
	return os;
}

/*********  IdentRandomMatrix  **********/
IdentRandomMatrix::IdentRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
	m_ranvar = CreateObject<UniformRandomVariable>();
	m_ranvar->SetAttribute("Min", DoubleValue(0));
	m_ranvar->SetAttribute("Max", DoubleValue(n - 1));
}

void IdentRandomMatrix::Generate(uint32_t seed)
{
	uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
	RngSeedManager::SetSeed(seed);

	uint32_t n = nCols(),
			 m = nRows();
	arma::mat ident = arma::eye<arma::mat>(n, n);
	//Fisher-Yates-Shuffle
	for (uint32_t i = 0; i < n - 2; i++)
	{
		m_ranvar->SetAttribute("Min", DoubleValue(i));
		uint32_t j = m_ranvar->GetInteger();
		ident.swap_rows(i, j);
	}
	m_mat = ident.rows(0, m - 1);

	RngSeedManager::SetSeed(seedOld);
}

/*********  GaussianRandomMatrix  **********/
GaussianRandomMatrix::GaussianRandomMatrix(double mean, double var, uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
	m_ranvar = CreateObject<NormalRandomVariable>();
	m_ranvar->SetAttribute("Mean", DoubleValue(mean));
	m_ranvar->SetAttribute("Variance", DoubleValue(var));
}

void GaussianRandomMatrix::Generate(uint32_t seed)
{
	uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
	RngSeedManager::SetSeed(seed);

	uint32_t n = nCols(),
			 m = nRows();
	for (uint32_t i = 0; i < m; i++)
	{
		for (uint32_t j = 0; j < n; j++)
		{
			m_mat(i, j) = m_ranvar->GetValue();
		}
	}

	RngSeedManager::SetSeed(seedOld);
}

/*********  BernRandomMatrix  **********/
BernRandomMatrix::BernRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
	m_ranvar = CreateObject<UniformRandomVariable>();
	m_ranvar->SetAttribute("Min", DoubleValue(0));
	m_ranvar->SetAttribute("Max", DoubleValue(1));
}

void BernRandomMatrix::Generate(uint32_t seed)
{
	uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
	RngSeedManager::SetSeed(seed);

	uint32_t n = nCols(),
			 m = nRows();
	double entryVal = sqrt(m);
	for (uint32_t i = 0; i < m; i++)
	{
		for (uint32_t j = 0; j < n; j++) // use inverse transform method here
		{
			double p = m_ranvar->GetValue();
			if(p < m_bernP)
			{
				m_mat(i, j) = entryVal;
			}
			else
			{
				m_mat(i, j) = -entryVal;
				}

		}
	}

	RngSeedManager::SetSeed(seedOld);
}