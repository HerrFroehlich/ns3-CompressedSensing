/**
* \file nc-matrix.h
*
* \author Tobias Waurick
* \date 31.08.17
*
*/
#include "nc-matrix.h"

NcMatrix::NcMatrix(NcMatrix &&other) noexcept : TOperator<double>(other.m(), other.n()), m_rows(other.m_rows)
{
	for (auto &row_ptr : other.m_rows)
	{
		row_ptr = nullptr;
	}
}

NcMatrix::NcMatrix(const NcMatrix &other) : TOperator<double>(other.m(), other.n())
{
	m_rows.reserve(other.m_rows.size());
	for (const auto &row_ptr : other.m_rows)
	{
		m_rows.push_back(new vector<double>(*row_ptr));
	}
}

NcMatrix &NcMatrix::operator=(NcMatrix &&other) noexcept
{
	for (auto &row_ptr : m_rows)
	{
		delete row_ptr;
		row_ptr = nullptr;
	}
	m_rows = other.m_rows;

	for (auto &row_ptr : other.m_rows)
	{
		row_ptr = nullptr;
	}
	Resize(other.m(), other.n());

	return *this;
}

NcMatrix &NcMatrix::operator=(const NcMatrix &other)
{
	for (auto &row_ptr : m_rows)
	{
		delete row_ptr;
		row_ptr = nullptr;
	}

	m_rows.resize(other.m_rows.size());
	m_rows.clear();
	for (const auto &row_ptr : other.m_rows)
	{
		m_rows.push_back(new vector<double>(*row_ptr));
	}

	Resize(other.m(), other.n());

	return *this;
}

NcMatrix::~NcMatrix()
{
	for (auto row_ptr : m_rows)
	{
		delete row_ptr;
		row_ptr = nullptr;
	}
}

void NcMatrix::WriteRow(const vector<double> &row)
{
	NS_ASSERT_MSG(row.size() == n(), "Vector has incorrect size!");
	m_rows.push_back(new vector<double>(row));
	Resize(m() + 1, n());
}

void NcMatrix::WriteRow(const double *buffer, uint32_t bufLen)
{
	NS_ASSERT_MSG(bufLen == n(), "Buffer has incorrect size!");
	m_rows.push_back(new vector<double>(buffer, buffer + bufLen));
	Resize(m() + 1, n());
}


void NcMatrix::SetRowLen(uint32_t len)
{
	if (m() > 0)
		Reset();
	Resize(0, len);
}

void NcMatrix::Reset()
{
	for (auto &row_ptr : m_rows)
	{
		delete row_ptr;
		row_ptr = nullptr;
	}
	m_rows.clear();
	Resize(0, n());
}

//inherited from TOperator<double>
void NcMatrix::apply(const arma::Col<double> &in, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->n());
	out.set_size(this->m());
	//iterate over rows
	for (uint32_t i = 0; i < m(); i++)
	{
		out(i) = 0;
		vector<double> row = *m_rows.at(i);
		//iterate over row
		for (uint32_t j = 0; j < n(); j++)
		{
			out(i) += row.at(j) * in(j); //implicit casting to double
		}
	}
}

void NcMatrix::applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->m());
	out.set_size(this->n());
	//iterate over column
	for (uint32_t i = 0; i < n(); i++)
	{
		out(i) = 0;
		//iterate over rows
		for (uint32_t j = 0; j < m(); j++)
		{
			vector<double> row = *m_rows.at(j);
			out(i) += row.at(i) * in(j); //implicit casting to double
		}
	}
}

void NcMatrix::column(klab::UInt32 i, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->n());
	out.set_size(m());
	//iterate over rows
	for (uint32_t j = 0; j < m(); j++)
	{
		vector<double> row = *m_rows.at(j);
		out(j) = row.at(i); //implicit casting to double
	}
}

void NcMatrix::columnAdjoint(klab::UInt32 i, arma::Col<double> &out)
{
	ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->m());
	out.set_size(n());
	vector<double> row = *m_rows.at(i);
	//iterate over row
	for (uint32_t j = 0; j < n(); j++)
	{
		out(j) = row.at(j); //implicit casting to double
	}
}

void NcMatrix::toMatrix(arma::Mat<double> &out)
{
	out.set_size(this->m(), this->n());

	//iterate over rows
	for (uint32_t i = 0; i < m(); i++)
	{
		vector<double> row = *m_rows.at(i);
		//iterate over row
		for (uint32_t j = 0; j < n(); j++)
		{
			out(i, j) = row.at(j); //implicit casting to double
		}
	}
}

void NcMatrix::toMatrixAdjoint(arma::Mat<double> &out)
{
	out.set_size(this->n(), this->m());

	//iterate over rows
	for (uint32_t i = 0; i < m(); i++)
	{
		vector<double> row = *m_rows.at(i);
		//iterate over row
		for (uint32_t j = 0; j < n(); j++)
		{
			out(j, i) = row.at(j); //implicit casting to double
		}
	}
}

void NcMatrix::Resize(uint32_t m, uint32_t n)
{
	TOperator<double>::resize(m, n);
}
