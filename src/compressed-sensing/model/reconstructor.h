/**
* \file reconstructor.h
*
* \author Tobias Waurick
* \date 03.06.2017
*
*/

#ifndef RECONSTRUCTOR_H
#define RECONSTRUCTOR_H

#include "ns3/core-module.h"
#include <armadillo>
#include <KL1pInclude.h>
#include <stdint.h>
#include <map>
#include "ns3/node-data-buffer.h"
#include "random-matrix.h"
#include "transform-matrix.h"
#include "cs-header.h"

using namespace ns3;
using namespace arma;
typedef std::complex<double> cx_double;
typedef CsHeader::T_IdField T_NodeIdTag;
/**
* \ingroup compsens
 * \defgroup rec Reconstructors
 *
 * Various classes for reconstructing spatial dependent, compressed data with compressed sensing algorithms
 */
/**
* \ingroup rec
* \class Reconstructor
*
* \brief A base class template for compressed sensing reconstructors using KL1p
*
* This class is used as a base class for different CS reconstructors (using KL1p).
* The base takes care of storing data for multiple nodes. Their individual random sensing matrix is recreated from used seed, which is also stored.
* Additionally a transformation matrix if needed can be aggregated.
*
* \tparam T type of internal data (either double or arma::cx_double)
*
* \author Tobias Waurick
* \date 03.06.17
*/
template <typename T = double>
class Reconstructor : public ns3::Object
{
  public:																 /**< type of node ID*/
	typedef void (*CompleteTracedCallback)(int64_t time, uint32_t iter); /**< callback signature when completed a reconstruction*/
	typedef void (*ErrorTracedCallback)(klab::KException e);			 /**< callback signature when reconstruction fails*/
	static TypeId GetTypeId(void);
	Reconstructor();

	/**
	* \brief starts the reconstruction for one node
	*
	* \param nodeId ID of node to reconstruct
	* 
	* \return time in ms needed for reconstruction
	*/
	virtual int64_t Reconstruct(T_NodeIdTag nodeId) = 0;

	/**
	* \brief starts the reconstruction for all nodes
	*
	* \return time in ms needed for reconstruction
	*/
	int64_t ReconstructAll();

	/**
	* \brief Adds a source node whose measurement data shall be reconstructed, default sizes are used for the buffer
	*
	* \param nodeId 8bit node Id
	* \param seed 	Seed used for constructing the sensing matrix of each node
	*
	*/
	void AddSrcNode(T_NodeIdTag nodeId, uint32_t seed);

	/**
	* \brief writes from data buffer to a this NodeDataBuffer
	*
	* \param nodeId	8bit-ID of the node
	* \param buffer pointer to data
	* \param bufSize  size of data buffer	
	*
	*/
	uint32_t WriteData(T_NodeIdTag nodeId, const T *buffer, const uint32_t bufSize);

	/**
	* \brief reads reconstructed data matrix and returns a vector (matrix ordered column by column)
	*
	* \param nodeId	8bit-ID of the node
	*
	* \return vector of T containing the reconstructed data matrix
	*/
	std::vector<T> ReadRecData(T_NodeIdTag nodeId) const;

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
	* \param trans Mat_ptr pointer to a TransMatrix object
	*
	*/
	void SetTransMat(Ptr<TransMatrix<T>> transMat_ptr);

  protected:
	/**
	* \brief sets the default dimension for new in and output buffers, should be used in a Setup member function
	*
	* \param nMeas 		original length of/(NOF) each original measurment (vectors)
	* \param mMax  		maximum length of/(NOF)  measurments (vectors) used for reconstructing
	* \param vecLen	    length of each measurement vector (e.g. 1 for temporal reconstruction, x for spatial reconstruction)
	*
	*/
	void SetBufferDim(uint32_t nMeas, uint32_t mMax, uint32_t vecLen);

	/**
	* \brief resets a node's in buffer if it's full
	*
	* \param nodeId ID of node
	*
	*/
	void ResetFullBuf(T_NodeIdTag nodeId);

	/**
	* \brief gets the NOF buffered measurement vectors for a node
	*
	* \param nodeId 8bitNodeId
	*
	* \return NOF  measurement vectors
	*/
	uint32_t GetNofMeas(T_NodeIdTag nodeId) const;

	/**
	* \brief gets the length of a measurement vectors for a node
	*
	* \param nodeId 8bitNodeId
	*
	* \return size of measurement vectors
	*/
	uint32_t GetVecLen(T_NodeIdTag nodeId) const;

	/**
	* \brief gets the NOF measurement vectors to be reconstructed for a node
	*
	* \param nodeId 8bitNodeId
	*
	* \return NOF  measurement vectors (out)
	*/
	uint32_t GetN(T_NodeIdTag nodeId) const;

	/**
	* \brief gets the NOF nodes added
	*
	*
	* \return NOF  node
	*/
	uint32_t GetNofNodes() const;

	/**
	* \brief gets the buffered data of a node as a matrix
	*
	* \param nodeId node ID
	*
	* \return Mat<T> the buffered data
	*/
	Mat<T> GetBufMat(T_NodeIdTag nodeId) const;

	/**
	* \brief write matrix to reconstruction buffer
	*
	* \param nodeId node ID
	* \param mat matrix to be written
	*
	*/
	void WriteRecBuf(T_NodeIdTag nodeId, const Mat<T> &mat);

	/**
	* \brief gets the operator used for reconstructing, which is created by multiplying the sensing matrix with a transformation
	*
	* \param nodeId node ID
	* \param norm	Normalize sensing matrix?
	*
	* \return multiplication operator
	// */
	klab::TSmartPointer<kl1p::TOperator<T>> GetOp(T_NodeIdTag nodeId, bool norm = false);
	// auto GetMatOp(T_NodeIdTag nodeId, bool norm = false);

	/**
	* \brief Adds a source node whose measurement data shall be reconstructed
	*
	* \param nodeId		8bit node Id
	* \param seed 		Seed used for constructing the sensing matrix of each node
	* \param nMeas 		original length of/(NOF) each original measurment (vectors)
	* \param mMax  		maximum length of/(NOF)  measurments (vectors) used for reconstructing
	* \param vecLen	    length of each measurement vector (e.g. 1 for temporal reconstruction, x for spatial reconstruction)
	*
	*/
	void AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax, uint32_t vecLen);

	uint32_t m_nMeasDef, m_mMaxDef, m_vecLenDef;	/**< default buffer dimensions*/
	TracedCallback<int64_t, uint32_t> m_completeCb; /**< callback when completed reconstruction, returning time+iterations needed*/
	TracedCallback<klab::KException> m_errorCb;		/**< callback when reconstruction fails, returning KExeception type*/

  private:
	typedef NodeDataBuffer<T> T_NodeBuffer;				 /**< NodeDataBuffer with elements of type T*/
	class T_NodeInfo : public SimpleRefCount<T_NodeInfo> /**< Class containing info on each node (seed, matrix sizes, buffer Ptrs)*/
	{
	  public:
		T_NodeInfo() : seed(0), nMeas(0), m(0), vecLen(0), mMax(0) {}
		T_NodeInfo(uint32_t s, uint32_t n, uint32_t m, uint32_t vl,
				   uint32_t mMax, Ptr<T_NodeBuffer> in, Ptr<T_NodeBuffer> out) : seed(s), nMeas(n),
																				 m(m), vecLen(vl),
																				 mMax(mMax), inBufPtr(in), outBufPtr(out)
		{
		}
		uint32_t seed;				// seed of random sensing matrix
		uint32_t nMeas;				// original number of measurements to reconstruct
		uint32_t m;					// current compressed data vectors
		uint32_t vecLen;			// data vector length
		uint32_t mMax;				// maximum NOF compressed data vectors
		Ptr<T_NodeBuffer> inBufPtr, /**< input data of each node*/
			outBufPtr;				/**< output data of each node after reconstruction*/
	};

	/**
	* \brief gets the sparse sensing MXN matrix constructed out of the meta data for the given node ID
	*
	* \param seed
	* \param m NOF rows
  	* \param n NOF columns
	* \param norm	Normalize sensing matrix?
	*
	* \return sparse sensing matrix, may be NULL
	*/
	klab::TSmartPointer<kl1p::TOperator<T>> GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm);

	/**
	* \brief Gets the NxN transformation matrix		
	* 
	* n	size of matrix
	*
	* \return the transformation matrix	, may be NULL
	*/
	klab::TSmartPointer<kl1p::TOperator<T>> GetTransMat(uint32_t n);

	/**
	* \brief checks out node info to avoid redundent look ups		
	*
	* \param nodeId	node ID	
	*
	* \return reference to current info structure
	*/
	T_NodeInfo &CheckOutInfo(T_NodeIdTag nodeId);
	const T_NodeInfo &CheckOutInfo(T_NodeIdTag nodeId) const;

	uint32_t m_nNodes;									/**< NOF nodes from which we are gathering data*/
	std::map<T_NodeIdTag, T_NodeInfo> m_nodeInfoMap;	/**< map for node info<>node ID*/
	klab::TSmartPointer<RandomMatrix> m_ranMat;			/**< Random matrix form from which sensing matrix is constructed*/
	klab::TSmartPointer<TransMatrix<T>> m_transMat; /**< Transformation matrix form from which sensing matrix is constructed*/
	// klab::TSmartPointer<kl1p::TMultiplicationOperator<T, T> m_multOp; /**< multiplication operator*/
	mutable T_NodeIdTag m_nodeIdNow, m_nodeIdNowConst; /**< current nodeId*/
	mutable Ptr<const T_NodeInfo> m_infoNowConst;	  /**< current nodeId for const call*/
	mutable Ptr<T_NodeInfo> m_infoNow;				   /**< current selected info structure*/
};

#endif //RECONSTRUCTOR_H