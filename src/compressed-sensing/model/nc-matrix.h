/**
* \file row-data-buffer.h
*
* \author Tobias Waurick
* \date 31.08.17
*
*/

/**
* \ingroup util
* \class NcMatrix
*
* \brief a Matrix with varying number of fixed sized rows, representing the network coding coefficients used for each packet
*
* The in and output type of the matrix operations is always a double.
*
* \tparam T type of data stored
*/

#ifndef NC_MATRIX_H
#define NC_MATRIX_H

#include "ns3/core-module.h"
#include <KL1pInclude.h>
#include <KSciInclude.h>
#include "assert.h"
using namespace std;
using namespace kl1p;

template <typename T>
class NcMatrix : public ns3::Object, public TOperator<double>
{
  public:
	NcMatrix() : TOperator<double>(0, 0) {}
	/**
	* \brief create an NcMatrix with a given row length
	*
	* \param len row length
	*
	*/
	NcMatrix(uint32_t len) : TOperator<double>(0, len) {}

	/**
	* \brief move constructor
	*
	* \param other other NcMatrix to move
	*
	*/
	NcMatrix(NcMatrix &&other) noexcept : TOperator<double>(other.m(), other.n()), m_rows(other.m_rows)
	{
		for (auto row_ptr : other.m_rows)
		{
			row_ptr = nullptr;
		}
	}

	/**
	* \brief copy constructor
	*
	* \param other other NcMatrix to copy
	*
	*/
	NcMatrix(const NcMatrix &other) : TOperator<double>(other.m(), other.n())
	{
		m_rows.reserve(other.m_rows.size());
		for (const auto &row_ptr : other.m_rows)
		{
			m_rows.push_back(new vector<T>(*row_ptr));
		}
	}

	/**
	* \brief move assignment operator
	*
	* \param other other NcMatrix to move
	*
	*/
	NcMatrix &operator=(NcMatrix &&other) noexcept
	{
		for (auto &row_ptr : m_rows)
		{
			delete row_ptr;
			row_ptr = nullptr;
		}
		m_rows = other.m_rows;

		for (auto row_ptr : other.m_rows)
		{
			row_ptr = nullptr;
		}
		Resize(other.m(), other.n());

		return *this;
	}

	/**
	* \brief copy assignment operator
	*
	* \param other other NcMatrix to copy
	*
	*/
	NcMatrix &operator=(const NcMatrix &other)
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
			m_rows.push_back(new vector<T>(*row_ptr));
		}

		Resize(other.m(), other.n());

		return *this;
	}

	~NcMatrix()
	{
		for (auto row_ptr : m_rows)
		{
			delete row_ptr;
			row_ptr = nullptr;
		}
	}

	/**
	* \brief writes a row by copying from a vector
	*
	* Asserts that the length of the vector equals the row length
	*
	* \param row vector containing row data
	*
	*/
	void WriteRow(const vector<T> &row)
	{
		NS_ASSERT_MSG(row.size() == n(), "Vector has incorrect size!");
		m_rows.push_back(new vector<T>(row));
		Resize(m() + 1, n());
	}

	/**
	* \brief writes a row by copying from a buffer
	*
	* Asserts that the length of the buffer equals the row length
	*
	* \param buf buffer containing row data
	* \para bufLen length of buffer
	*
	*/
	void WriteRow(const T *buffer, uint32_t bufLen)
	{
		NS_ASSERT_MSG(bufLen == n(), "Buffer has incorrect size!");
		m_rows.push_back(new vector<T>(buffer, buffer + bufLen));
		Resize(m() + 1, n());
	}

	/**
	* \brief Gets the NOF rows written
	*
	* \return NOF rows written
	*/
	// uint32_t GetNRows() const
	// {
	// 	return m_rows.size();
	// }

	/**
	* \brief Gets the length of a row
	*
	* \return length of a row
	*/
	// void GetRowLen() const
	// {
	// 	return m();
	// }
	/**
	* \brief Sets the length of a row
	*
	* \param len new length of a row
	*/
	void SetRowLen(uint32_t len)
	{
		if (m() > 0)
			Reset();
		Resize(0, len);
	}

	/**
	* \brief resets the buffer 
	*/
	void Reset()
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
	virtual void apply(const arma::Col<double> &in, arma::Col<double> &out)
	{
		ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->n());
		out.set_size(this->m());
		//iterate over rows
		for (uint32_t i = 0; i < m(); i++)
		{
			out(i) = 0;
			vector<T> row = *m_rows.at(i);
			//iterate over row
			for (uint32_t j = 0; j < n(); j++)
			{
				out(i) += row.at(j) * in(j); //implicit casting to double
			}
		}
	}

	virtual void applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out)
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
				vector<T> row = *m_rows.at(j);
				out(i) += row.at(i) * in(j); //implicit casting to double
			}
		}
	}

	virtual void column(klab::UInt32 i, arma::Col<double> &out)
	{
		ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->n());
		out.set_size(m());
		//iterate over rows
		for (uint32_t j = 0; j < m(); j++)
		{
			vector<T> row = *m_rows.at(j);
			out(j) = row.at(i); //implicit casting to double
		}
	}

	virtual void columnAdjoint(klab::UInt32 i, arma::Col<double> &out)
	{
		ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->m());
		out.set_size(n());
		vector<T> row = *m_rows.at(i);
		//iterate over row
		for (uint32_t j = 0; j < n(); j++)
		{
			out(j) = row.at(j); //implicit casting to double
		}
	}

	virtual void toMatrix(arma::Mat<double> &out)
	{
		out.set_size(this->m(), this->n());

		//iterate over rows
		for (uint32_t i = 0; i < m(); i++)
		{
			vector<T> row = *m_rows.at(i);
			//iterate over row
			for (uint32_t j = 0; j < n(); j++)
			{
				out(i, j) = row.at(j); //implicit casting to double
			}
		}
	}

	virtual void toMatrixAdjoint(arma::Mat<double> &out)
	{
		out.set_size(this->n(), this->m());

		//iterate over rows
		for (uint32_t i = 0; i < m(); i++)
		{
			vector<T> row = *m_rows.at(i);
			//iterate over row
			for (uint32_t j = 0; j < n(); j++)
			{
				out(j, i) = row.at(j); //implicit casting to double
			}
		}
	}

  private:
	/**
	* \brief resizes the operator
	*
	* \param m NOF rows
	* \param n NOF column
	*/
	void Resize(uint32_t m, uint32_t n)
	{
		TOperator<double>::resize(m, n);
	}

	vector<vector<T> *> m_rows;
};

#endif //NC_MATRIX_H