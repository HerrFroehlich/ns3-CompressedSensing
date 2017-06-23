/**
* \file random-matrix.cc
*
* \author Tobias Waurick
* \date 21.06.17
*
*/

#include "random-matrix.h"

/*********  Random Matrix  **********/
NS_OBJECT_ENSURE_REGISTERED(RandomMatrix);

TypeId RandomMatrix::GetTypeId(void)
{
	static TypeId tid = TypeId("RandomMatrix")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("Stream", "RNG stream number",
										  IntegerValue(0),
										  MakeIntegerAccessor(&RandomMatrix::m_stream),
										  MakeIntegerChecker<int64_t>());
	return tid;
}

RandomMatrix::RandomMatrix() : m_prevSeed(0), m_mat()
{
}

RandomMatrix::RandomMatrix(uint32_t m, uint32_t n) : m_prevSeed(0), m_mat(m, n)
{
}

void RandomMatrix::SetSize(uint32_t m, uint32_t n)
{
	if ((m != nRows()) && (n != nCols()))
	{
		m_mat.set_size(m, n);
		Generate(m_prevSeed);
	}
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

void RandomMatrix::Normalize()
{
	m_mat = (1 / sqrt(nRows())) * m_mat;
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
IdentRandomMatrix::IdentRandomMatrix()
{
}

IdentRandomMatrix::IdentRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
}

void IdentRandomMatrix::Generate(uint32_t seed)
{
	if (seed != m_prevSeed) // only regenerate if nescessary
	{
		uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
		RngSeedManager::SetSeed(seed);
		uint32_t n = nCols(),
				 m = nRows();
		arma::mat ident = arma::eye<arma::mat>(n, n);
		Ptr<T_RanVar> ranvar = CreateObject<T_RanVar>();
		ranvar->SetAttribute("Min", DoubleValue(0));
		ranvar->SetAttribute("Max", DoubleValue(n - 1));
		ranvar->SetStream(m_stream);

		if (n > 1)
		{
			//Fisher-Yates-Shuffle
			for (uint32_t i = 0; i < n - 2; i++)
			{
				ranvar->SetAttribute("Min", DoubleValue(i));
				uint32_t j = ranvar->GetInteger();
				ident.swap_rows(i, j);
			}
			m_mat = ident.rows(0, m - 1);
		}
		else if (n == 1)
		{
			m_mat = 1;
		}

		RngSeedManager::SetSeed(seedOld);
	}
}

/*********  GaussianRandomMatrix  **********/
GaussianRandomMatrix::GaussianRandomMatrix()
{
}

GaussianRandomMatrix::GaussianRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n), m_mean(0), m_var(1)
{
}
GaussianRandomMatrix::GaussianRandomMatrix(double mean, double var, uint32_t m, uint32_t n) : RandomMatrix(m, n), m_mean(mean), m_var(var)
{
}

void GaussianRandomMatrix::Generate(uint32_t seed)
{
	if (seed != m_prevSeed) // only regenerate if nescessary
	{
		uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
		RngSeedManager::SetSeed(seed);
		Ptr<T_RanVar> ranvar = CreateObject<T_RanVar>();
		ranvar->SetAttribute("Mean", DoubleValue(m_mean));
		ranvar->SetAttribute("Variance", DoubleValue(m_var));
		ranvar->SetStream(m_stream);
		uint32_t n = nCols(),
				 m = nRows();
		for (uint32_t i = 0; i < m; i++)
		{
			for (uint32_t j = 0; j < n; j++)
			{
				m_mat(i, j) = ranvar->GetValue();
			}
		}

		RngSeedManager::SetSeed(seedOld);
	}
}

/*********  BernRandomMatrix  **********/
BernRandomMatrix::BernRandomMatrix()
{
}

BernRandomMatrix::BernRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
}

void BernRandomMatrix::Generate(uint32_t seed)
{
	if (seed != m_prevSeed) // only regenerate if nescessary
	{
		uint32_t seedOld = RngSeedManager::GetSeed(); // save back old seed
		RngSeedManager::SetSeed(seed);

		Ptr<T_RanVar> ranvar = CreateObject<T_RanVar>();
		ranvar->SetAttribute("Min", DoubleValue(0));
		ranvar->SetAttribute("Max", DoubleValue(1));
		ranvar->SetStream(m_stream);

		uint32_t n = nCols(),
				 m = nRows();
		for (uint32_t i = 0; i < m; i++)
		{
			for (uint32_t j = 0; j < n; j++) // use inverse transform method here
			{
				double p = ranvar->GetValue();
				if (p < m_bernP)
				{
					m_mat(i, j) = 1;
				}
				else
				{
					m_mat(i, j) = -1;
				}
			}
		}

		RngSeedManager::SetSeed(seedOld);
	}
}