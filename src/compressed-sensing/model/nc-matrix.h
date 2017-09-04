/**
* \file nc-matrix.h
*
* \author Tobias Waurick
* \date 31.08.17
*
*/

#ifndef NC_MATRIX_H
#define NC_MATRIX_H

#include "ns3/core-module.h"
#include <KL1pInclude.h>
#include <KSciInclude.h>
#include "assert.h"
using namespace std;
using namespace kl1p;
/**
* \ingroup util
* \class NcMatrix
*
* \brief a Matrix with varying number of fixed sized rows, representing the network coding coefficients used for each packet
*
* The in and output type of the matrix operations is always a double.
*
*/
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
	NcMatrix(NcMatrix &&other) noexcept;
	;

	/**
	* \brief copy constructor
	*
	* \param other other NcMatrix to copy
	*
	*/
	NcMatrix(const NcMatrix &other);

	/**
	* \brief move assignment operator
	*
	* \param other other NcMatrix to move
	*
	*/
	NcMatrix &operator=(NcMatrix &&other) noexcept;

	/**
	* \brief copy assignment operator
	*
	* \param other other NcMatrix to copy
	*
	*/
	NcMatrix &operator=(const NcMatrix &other);

	~NcMatrix();

	/**
	* \brief writes a row by copying from a vector
	*
	* Asserts that the length of the vector equals the row length
	*
	* \param row vector containing row data
	*
	*/
	void WriteRow(const vector<double> &row);

	/**
	* \brief writes a row by copying from a buffer
	*
	* Asserts that the length of the buffer equals the row length
	*
	* \param buf buffer containing row data
	* \para bufLen length of buffer
	*
	*/
	void WriteRow(const double *buffer, uint32_t bufLen);

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
	void SetRowLen(uint32_t len);

	/**
	* \brief resets the buffer 
	*/
	void Reset();

	//inherited from TOperator<double>
	virtual void apply(const arma::Col<double> &in, arma::Col<double> &out);
	virtual void applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out);
	virtual void column(klab::UInt32 i, arma::Col<double> &out);
	virtual void columnAdjoint(klab::UInt32 i, arma::Col<double> &out);
	virtual void toMatrix(arma::Mat<double> &out);
	virtual void toMatrixAdjoint(arma::Mat<double> &out);

  private:
	/**
	* \brief resizes the operator
	*
	* \param m NOF rows
	* \param n NOF column
	*/
	void Resize(uint32_t m, uint32_t n);

	vector<vector<double> *> m_rows;
};

#endif //NC_MATRIX_H