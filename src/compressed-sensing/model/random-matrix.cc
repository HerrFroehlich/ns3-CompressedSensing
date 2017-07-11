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

RandomMatrix::RandomMatrix() : m_prevSeed(1), m_mat()
{
}

RandomMatrix::RandomMatrix(uint32_t m, uint32_t n) : kl1p::TOperator<double>(m, n),
													 m_prevSeed(1),
													 m_mat(m, n)
{
	// m_mat.set_size(m, n);
}

void RandomMatrix::SetSize(uint32_t m, uint32_t n, bool regenerate)
{
	if ((m != nRows()) || (n != nCols()))
	{
		kl1p::TOperator<double>::resize(m, n);
		m_mat.set_size(m, n);
		//m_mat = arma::mat(m, n);
		if (regenerate)
			Generate(m_prevSeed, true);
	}
}

void RandomMatrix::SetSize(uint32_t m, uint32_t n, uint32_t seed)
{
	if ((m != nRows()) || (n != nCols()) || (seed != m_prevSeed))
	{
		kl1p::TOperator<double>::resize(m, n);
		m_mat.set_size(m, n);
		//m_mat = arma::mat(m, n);
		Generate(seed, true);
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

void RandomMatrix::NormalizeToM()
{
	m_mat = (1 / sqrt(nRows())) * m_mat;
}

void RandomMatrix::apply(const arma::Col<double> &in, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->n());
	out = m_mat * in;
}

void RandomMatrix::applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->m());

	out = arma::trans(m_mat) * in;
}

void RandomMatrix::column(klab::UInt32 i, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->n());

	out.set_size(this->m());
	for (klab::UInt32 j = 0; j < out.n_rows; ++j)
		out[j] = m_mat(j, i);
}

void RandomMatrix::columnAdjoint(klab::UInt32 i, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->m());

	out.set_size(this->n());
	for (klab::UInt32 j = 0; j < out.n_rows; ++j)
		out[j] = klab::Conj(m_mat(i, j));
}

void RandomMatrix::toMatrix(arma::Mat<double> &out)
{
	out = m_mat;
}

void RandomMatrix::toMatrixAdjoint(arma::Mat<double> &out)
{
	out = arma::trans(m_mat);
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
	Generate(m_prevSeed, true);
}

void IdentRandomMatrix::Generate(uint32_t seed, bool force)
{
	if ((seed != m_prevSeed) || force) // only regenerate if nescessary or forced to
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
		m_prevSeed = seed;
		RngSeedManager::SetSeed(seedOld);
	}
}

IdentRandomMatrix *IdentRandomMatrix::Clone() const
{
	return new IdentRandomMatrix(*this);
}
/*********  GaussianRandomMatrix  **********/
GaussianRandomMatrix::GaussianRandomMatrix()
{
}

GaussianRandomMatrix::GaussianRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n), m_mean(0), m_var(1)
{
	Generate(m_prevSeed, true);
}
GaussianRandomMatrix::GaussianRandomMatrix(double mean, double var, uint32_t m, uint32_t n) : RandomMatrix(m, n), m_mean(mean), m_var(var)
{
	Generate(m_prevSeed, true);
}

void GaussianRandomMatrix::Generate(uint32_t seed, bool force)
{
	if ((seed != m_prevSeed) || force) // only regenerate if nescessary or forced to
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

		m_prevSeed = seed;
		RngSeedManager::SetSeed(seedOld);
	}
}

GaussianRandomMatrix *GaussianRandomMatrix::Clone() const
{
	return new GaussianRandomMatrix(*this);
}
/*********  BernRandomMatrix  **********/
BernRandomMatrix::BernRandomMatrix()
{
}

BernRandomMatrix::BernRandomMatrix(uint32_t m, uint32_t n) : RandomMatrix(m, n)
{
	Generate(m_prevSeed, true);
}

void BernRandomMatrix::Generate(uint32_t seed, bool force)
{
	if ((seed != m_prevSeed) || force) // only regenerate if nescessary or forced to
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

		m_prevSeed = seed;
		RngSeedManager::SetSeed(seedOld);
	}
}

BernRandomMatrix *BernRandomMatrix::Clone() const
{
	return new BernRandomMatrix(*this);
}