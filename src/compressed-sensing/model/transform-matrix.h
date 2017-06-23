/**
* \file transform-matrix.h
*
* \author Tobias Waurick
* \date 23.06.17
*
*/

#ifndef TRANSFORM_MATRIX_H
#define TRANSFORM_MATRIX_H
#include "ns3/core-module.h"
#include <armadillo>
using namespace arma;

/**
* \class TransMatrix
*
* \brief a base clase template to create transformation matrices
*
* \tparam T data type entries of underlying matrix
*/
class TransMatrix : public ns3::Object
{
  public:
	/**
	* \brief create empty matrix
	*/
	TransMatrix();

	/**
	* \brief create a transformation matrix of size MxN
	*
	* \param n NOF rows/columms
	*
	*/
	TransMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix (and thus changing its entries)
	*
	* \param n NOF rows/columms
	*
	*/
	virtual void SetSize(uint32_t n) = 0;

	/**
	* \brief gets the NOF rows/columms
	*
	* \return NOF rows/columms
	*/
	uint32_t GetSize() const;

	operator Mat<cx_double>() const
	{
		return m_mat;
	};

	friend Mat<cx_double> operator*(const TransMatrix &, const Mat<cx_double> &);

	friend Mat<cx_double> operator*(const Mat<cx_double> &, const TransMatrix &);
	friend std::ostream &operator<<(std::ostream &os, const TransMatrix &obj);

  protected:
	Mat<cx_double> m_mat; /**< underlying matrix*/
};


/**
* \class FourierTransMatrix
*
* \brief matrix inducing a 1D fourier transformation
*
*/
class FourierTransMatrix : public TransMatrix
{
public:
	/**
	* \brief create empty matrix
	*/
	FourierTransMatrix();

	/**
	* \brief create a transformation matrix of size MxN
	*
	* \param n NOF rows/columms
	*
	*/
	FourierTransMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix (and thus changing its entries)
	*
	* \param n NOF rows/columms
	*
	*/
	virtual void SetSize(uint32_t n);
};

#endif //TRANSFORM_MATRIX_H
