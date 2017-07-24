/**
* \file spatial-precoding-matrix.h
*
* \author Tobias Waurick
* \date 24.07.17
*
*/

/**
* \ingroup compsens
* \class SpatialPrecodingMatrix
*
* \brief a MxM matrix containingwith where some diagonal entries are one and the rest zero, representing the nodes sending during the spatial precoding
*
* 
*
*/

#ifndef SPATIAL_PRECODING_MATRIX_H
#define SPATIAL_PRECODING_MATRIX_H

#include "ns3/core-module.h"
#include <armadillo>
#include <KL1pInclude.h>
#include <KSciInclude.h>
#include <vector>
#include "ns3/template-registration.h"

using namespace ns3;
using namespace kl1p;

template <typename T>
class SpatialPrecodingMatrix : public Object, public kl1p::TOperator<T>
{
  public:
	static TypeId GetTypeId();

	/**
	* \brief creates an empty precoding matrix
	*
	*/
	SpatialPrecodingMatrix();

	/**
	* \brief creates an  precoding matrix of size MxM with ones on the diagonal
	*
	* \param n NOF rows /colums of the matrix
	*
	*/
	SpatialPrecodingMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix, if bigger sets new entries to true
	*
	* \param n NOF rows /colums of the matrix
	*
	*/
	void SetSize(uint32_t n);

	/**
	* \brief gets the size of the matrix
	*
	* \returns size of matrix
	*
	*/
	uint32_t  GetSize();

	/**
	* \brief Sets one diagonal entry
	*
	* \param idx index of diagonal entry to set zero
	* \param val value of entry, either one(true) or zero(false)
	*
	*/
	void SetEntry(uint32_t idx, bool val = false);

	/**
	* \brief sets the whole diagonal
	*
	* \param vec boolean vector representing the diagonal entries, must have M entries
	*
	*/
	void SetDiag(std::vector<bool> vec);

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<T> &in, arma::Col<T> &out);
	virtual void applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out);
	virtual void column(klab::UInt32 i, arma::Col<T> &out);
	virtual void columnAdjoint(klab::UInt32 i, arma::Col<T> &out);
	virtual void toMatrix(arma::Mat<T> &out);
	virtual void toMatrixAdjoint(arma::Mat<T> &out);


  private:
	uint32_t m_n;
	std::vector<bool> m_diag;
};
#endif //SPATIAL_PRECODING_MATRIX_H