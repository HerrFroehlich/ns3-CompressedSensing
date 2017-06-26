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
#include "./utils/node-data-buffer.h"
#include "random-matrix.h"
#include "transform-matrix.h"

using namespace ns3;
using namespace arma;

/**
* \class Reconstructor
*
* \brief A base class template for compressed sensing reconstructors using KL1p
*
* This class is used as a base class for different CS reconstructors (using KL1p).
* The base takes care of storing data (doubles) for indivudal nodes. Aside a Seed  to caonstruct a sensing matrix);
*
* \author Tobias Waurick
* \date 03.06.17
*/

class Reconstructor : public ns3::Object
{
  public:
	typedef uint8_t T_NodeIdTag; /**< type of node ID*/
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
	virtual int64_t ReconstructAll() = 0;

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
	uint32_t WriteData(T_NodeIdTag nodeId, const double *buffer, const uint32_t &bufSize);

	/**
	* \brief reads reconstructed data matrix and returns a vector (matrix ordered column by column)
	*
	* \param nodeId	8bit-ID of the node
	*
	* \return vector of doubles containing the reconstructed data matrix
	*/
	std::vector<double> ReadRecData(T_NodeIdTag nodeId) const;

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
	* \brief resets a node's in- and output buffer
	*
	* \param nodeId ID of node
	*
	*/
	void ResetBuf(T_NodeIdTag nodeId);

	/**
	* \brief gets the NOF buffered measurement vectors for a node
	*
	* \param nodeId 8bitNodeId
	*
	* \return NOF  measurement vectors
	*/
	uint32_t GetNofMeas(T_NodeIdTag nodeId) const;

	/**
	* \brief gets the buffered data of a node as a matrix
	*
	* \param nodeId node ID
	*
	* \return Mat<cx_double> the buffered data  (with complex entries)
	*/
	Mat<cx_double> GetBufMat(T_NodeIdTag nodeId) const;

	/**
	* \brief write matrix to reconstruction buffer
	*
	* \param nodeId node ID
	* \param mat matrix to be written
	*
	*/
	void WriteRecBuf(T_NodeIdTag nodeId, const Mat<double>& mat);

	/**
	* \brief gets the matrix operator used for reconstructing, which is created by multiplying the sensing matrix with a transformation
	*
	* \param nodeId node ID
	* \param norm	Normalize sensing matrix?
	*
	* \return returnDesc
	*/
	kl1p::TMatrixOperator<cx_double> GetMatOp(T_NodeIdTag nodeId, bool norm = false);

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

	uint32_t m_nMeasDef, m_mMaxDef, m_vecLenDef; /**< default buffer dimensions*/
  private:
	typedef NodeDataBuffer<double> T_NodeBuffer;
	class T_NodeInfo : public Object /**< Class containing info on each node (seed, matrix sizes, buffer Ptrs)*/
	{
	  public:
		T_NodeInfo() : seed(0), nMeas(0), m(0), vectLen(0), mMax(0) {}
		T_NodeInfo(uint32_t s, uint32_t n, uint32_t m, uint32_t vl,
				   uint32_t mMax, Ptr<T_NodeBuffer> in, Ptr<T_NodeBuffer> out) : seed(s), nMeas(n),
																				 m(m), vectLen(vl),
																				 mMax(mMax), inBufPtr(in), outBufPtr(out)
		{
		}
		uint32_t seed;				// seed of random sensing matrix
		uint32_t nMeas;				// original number of measurements to reconstruct
		uint32_t m;					// current compressed data vectors
		uint32_t vectLen;			// data vector length
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
	* \return sparse sensing matrix
	*/
	Mat<double> GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm);

	/**
	* \brief Gets the NxN transformation matrix		
	* 
	* n	size of matrix
	*
	* \return the transformation matrix	
	*/
	Mat<cx_double> GetTransMat(uint32_t n);

	/**
	* \brief checks out node info to avoid redundent look ups		
	*
	* \param nodeId	node ID	
	*
	* \return reference to current info structure
	*/
	T_NodeInfo &CheckOutInfo(T_NodeIdTag nodeId);
	const T_NodeInfo &CheckOutInfo(T_NodeIdTag nodeId) const;

	uint32_t m_nNodes;								 /**< NOF nodes from which we are gathering data*/
	std::map<T_NodeIdTag, T_NodeInfo> m_nodeInfoMap; /**< map for node info<>node ID*/
	Ptr<RandomMatrix> m_ranMat;						 /**< Random matrix form from which sensing matrix is constructed*/
	Ptr<TransMatrix> m_transMat;					 /**< Transformation matrix form from which sensing matrix is constructed*/
	mutable T_NodeIdTag m_nodeIdNow;				 /**< current nodeId*/
	mutable Ptr<T_NodeInfo> m_infoNow;				 /**< current selected info structure*/
};

#endif //RECONSTRUCTOR_H