/**
* \file compressor.h
*
* \author Tobias Waurick
* \date 11.07.17
*
*/

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "ns3/core-module.h"
#include <KL1pInclude.h>
#include "ns3/node-data-buffer.h"
#include "random-matrix.h"
#include "transform-matrix.h"
using namespace ns3;

/**
* \ingroup compsens
 * \defgroup compr Compressors
 *
 * Classes to compress data via random matrices and transfromations
 */

/**
* \ingroup compr
* \class Compressor	
*
* \brief Compresses measurement vectors X into a lower dimensional space Y.
*
* The seed for the random matrix, length of X and Y  and the size of a single measurement vector are set in a Setup method,
* which should be called before initiating the compression via Compress.
* In addition to compressing dense matrices, it is also possible to compress sparse matrices, where some rows are zero ENTIRELY.
*
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* Compressor<double>\n
* Compressor<cx_double>\n
*
* \tparam T type of internal data (either double or arma::cx_double)
*
* \author Tobias Waurick
* \date 11.07.17
*/
template <typename T>
class Compressor : public Object
{
  public:
	typedef void (*CompleteCallback)(arma::Mat<T>, arma::Mat<T>); /**< callback signature: complete a compression*/

	static TypeId GetTypeId(void);
	Compressor();

	/**
	* \brief creates a compressor and sets the dimension for X and Y 
	*
	* \param n 		NOF original measurment vectors (X)
	* \param m  	NOF  measurments vectors when compreesed
	* \param vecLen	length of each measurement vector (e.g. 1 for temporal reconstruction, x for spatial reconstruction)
	*
	*/
	Compressor(uint32_t n, uint32_t m, uint32_t vecLen);

	/**
	* \brief sets the dimension for X and Y 
	*
	* \param n 		NOF original measurment vectors (X)
	* \param m  	NOF  measurments vectors when compreesed
	* \param vecLen	length of each measurement vector (e.g. 1 for temporal reconstruction, x for spatial reconstruction)
	* \param norm	normalize random matrix to 1/sqrt(m)?
	*
	*/
	void Setup(uint32_t seed, uint32_t n, uint32_t m, uint32_t vecLen, bool norm = false);

	/**
	* \brief compresses data from input buffer and writes it to the output buffer
	* If X is a matrix, column-wise order in the input buffer is assumed. In this case the output buffer is also written column by column.
	*
	* \param bufferIn pointer to input buffer 
	* \param bufLenIn length of input buffer (must be vecLen*n)
	* \param bufferOut pointer to output buffer
	* \param bufLenOut length of output buffer (must be vecLen*m)
	*
	*/
	void Compress(const T *bufferIn, uint32_t bufLenIn, T *bufferOut, uint32_t bufLenOut) const;

	/**
	* \brief compresses data from input matrix and writes it to the output buffer
	*
	* The output buffer is written column by column.
	*
	* \param matIn input matrix
	* \param bufferOut pointer to output buffer
	* \param bufLenOut length of output buffer (must be vecLen*m)
	*
	*/
	void Compress(const arma::Mat<T> &matIn, T *bufferOut, uint32_t bufLenOut) const;

	/**
	* \brief compresses data from input matrix with sparse rows and writes it to the output buffer
	*
	* The sparse SxV matrix is represented by 2 parameters: 
	* - its non-zero-row data
	* - a Sx1 vector of indices giving the actual row index of each data row in the sparse matrix.
	* Note that S<N and that V == the length of a measurement vector.
	* The output buffer is written column by column.
	*
	* \param data data of sparse matrix WITHOUT sparse rows
	* \param idx  vector containing row indices
	* \param bufferOut pointer to output buffer
	* \param bufLenOut length of output buffer (must be vecLen*m)
	*
	*/
	template <typename TI>
	void CompressSparse(const arma::Mat<T> &data, const arma::Col<TI> &idx, T *bufferOut, uint32_t bufLenOut) const;

	/**
	* \brief sets the seed used for the random matrix and regenerates it
	*
	* \param seed seed to use
	* \param norm normalize random matrix to 1/sqrt(m)?
	*
	*/
	void SetSeed(uint32_t seed, bool norm = false);

	/**
	* \brief sets the internal random matrix object
	*
	* \param ranMat_ptr pointer to a RandomMatrix object
	*
	*/
	void SetRanMat(Ptr<RandomMatrix> ranMat_ptr);

	/**
	* \brief sets the internal transformation object
	*
	* \param transMat_ptr pointer to a TransMatrix object
	*
	*/
	void SetTransMat(Ptr<TransMatrix<T>> transMat_ptr);

  private:
	uint32_t
		m_seed,										/**< seed to create random matrix from*/
		m_m,										/**< length of Y*/
		m_n,										/**< length of X*/
		m_vecLen;									/**< size of one measurement vector*/
	uint64_t m_bufLenIn,							/**< input buffer length */
		m_bufLenOut;								/**< output buffer length */
	bool m_normalize;								/**< normalize random matrix?*/
	klab::TSmartPointer<RandomMatrix> m_ranMat;		/**< Random matrix form from which sensing matrix is constructed*/
	klab::TSmartPointer<TransMatrix<T>> m_transMat; /**< Transformation matrix form from which sensing matrix is constructed*/
	TracedCallback<arma::Mat<T>, arma::Mat<T>> m_completeCb;	/**< callback when compression is completed, 1.matrix: input, 2.matrix result*/
};

/**
* \ingroup compr
* \class CompressorTemp
*
* \brief compresses measurements X into a lower dimensional space Y (temporal coding)
*
* Similiar to Compressor<T>, but here the length of a measurement vector is fixed to 1;
*
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* CompressorTemp<double>\n
* CompressorTemp<cx_double>\n
*
* \tparam T type of internal data (either double or arma::cx_double)
*
* \author Tobias Waurick
* \date 12.07.17
*/
template <typename T>
class CompressorTemp : public Compressor<T>
{
  public:
	static TypeId GetTypeId(void);
	CompressorTemp();

	/**
	* \brief creates a compressor and sets the dimension for X and Y 
	*
	* \param n 		NOF original measurment vectors (X)
	* \param m  	NOF  measurments vectors when compreesed
*
	*/
	CompressorTemp(uint32_t n, uint32_t m);

	/**
	* \brief sets the dimension for X and Y 
	*
	* \param n 		NOF original measurment vectors (X)
	* \param m  	NOF  measurments vectors when compreesed
	* \param norm	normalize random matrix to 1/sqrt(m)?
	*
	*/
	void Setup(uint32_t seed, uint32_t n, uint32_t m, bool norm = false);

  private:
	const static uint32_t VECLEN = 1;
};

#endif //COMPRESSOR_H