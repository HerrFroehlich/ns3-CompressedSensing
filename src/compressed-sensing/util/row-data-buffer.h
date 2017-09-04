/**
* \file row-data-buffer.h
*
* \author Tobias Waurick
* \date 31.08.17
*
*/

/**
* \ingroup util
* \class RowDataBuffer
*
* \brief a Matrix with varying number of fixed sized rows
*
*/

#ifndef ROW_DATA_BUFFER_H
#define ROW_DATA_BUFFER_H

#include "ns3/core-module.h"
#include <KL1pInclude.h>
#include <KSciInclude.h>
#include "assert.h"
using namespace std;
using namespace kl1p;

template <typename T>
class RowDataBuffer : public ns3::Object
{
  public:
	RowDataBuffer() : m_len(0) {}
	/**
	* \brief create an RowDataBuffer with a given row length
	*
	* \param len row length
	*
	*/
	RowDataBuffer(uint32_t len) : m_len(len) {}

	/**
	* \brief move constructor
	*
	* \param other other RowDataBuffer to move
	*
	*/
	RowDataBuffer(RowDataBuffer &&other) noexcept : m_len(other.m_len), m_rows(other.m_rows)
	{
		for (auto row_ptr : other.m_rows)
		{
			row_ptr = nullptr;
		}
	}

	/**
	* \brief copy constructor
	*
	* \param other other RowDataBuffer to copy
	*
	*/
	RowDataBuffer(const RowDataBuffer &other) : m_len(other.m_len)
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
	* \param other other RowDataBuffer to move
	*
	*/
	RowDataBuffer &operator=(RowDataBuffer &&other) noexcept
	{
		for (auto &row_ptr : m_rows)
		{
			delete row_ptr;
			row_ptr = nullptr;
		}
		m_len = other.m_len;
		m_rows = other.m_rows;

		for (auto row_ptr : other.m_rows)
		{
			row_ptr = nullptr;
		}

		return *this;
	}

	/**
	* \brief copy assignment operator
	*
	* \param other other RowDataBuffer to copy
	*
	*/
	RowDataBuffer &operator=(const RowDataBuffer &other)
	{
		for (auto &row_ptr : m_rows)
		{
			delete row_ptr;
			row_ptr = nullptr;
		}

		m_len = other.m_len;
		m_rows.resize(other.m_rows.size());
		m_rows.clear();
		for (const auto &row_ptr : other.m_rows)
		{
			m_rows.push_back(new vector<T>(*row_ptr));
		}

		return *this;
	}

	~RowDataBuffer()
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
		NS_ASSERT_MSG(row.size() == m_len, "Vector has incorrect size!");
		m_rows.push_back(new vector<T>(row));
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
		NS_ASSERT_MSG(bufLen == m_len, "Buffer has incorrect size!");
		m_rows.push_back(new vector<T>(buffer, buffer + bufLen));
	}

	/**
	* \brief Gets the NOF rows written
	*
	* \return NOF rows written
	*/
	uint32_t GetNRows() const
	{
		return m_rows.size();
	}

	/**
	* \brief Gets the length of a row
	*
	* \return length of a row
	*/
	uint32_t GetRowLen() const
	{
		return m_len;
	}

	/**
	* \brief Sets the length of a row
	*
	* \param len new length of a row
	*/
	void SetRowLen(uint32_t len)
	{
		if (GetNRows() > 0)
			Reset();
		m_len = len;
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
	}

	/**
	* \brief gets a column
	*
	* Asserts that the index is in the correct range.
	*
	* \param i column index
	* \param out arma::Col to copy to
	*/
	void GetCol(uint32_t i, arma::Col<double> &out)
	{
		NS_ASSERT_MSG(i >= this->m_len, "Index out of bounds!");
		out.set_size(GetNRows());
		//iterate over rows
		for (uint32_t j = 0; j < GetNRows(); j++)
		{
			vector<T> row = *m_rows.at(j);
			out(j) = row.at(i); //implicit casting to double
		}
	}

	/**
	* \brief gets the whole matrix
	*
	* \param out arma::Mat to copy to
	*
	*/
	void GetMatrix(arma::Mat<double> &out)
	{
		out.set_size(this->GetNRows(), this->m_len);

		//iterate over rows
		for (uint32_t i = 0; i < GetNRows(); i++)
		{
			vector<T> row = *m_rows.at(i);
			//iterate over row
			for (uint32_t j = 0; j < m_len; j++)
			{
				out(i, j) = row.at(j); //implicit casting to double
			}
		}
	}

  private:
	uint32_t m_len; /**< length of rows*/
	vector<vector<T> *> m_rows;
};

#endif //ROW_DATA_BUFFER_H