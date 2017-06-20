/**
* \file reconstructor
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

using namespace ns3;
using namespace arma;

/**
* \class Reconstructor
*
* \brief A base class templtae for compressed sensing reconstructors using KL1p
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
	Reconstructor();

	/**
	* \brief starts the reconstruction for one node
	*
	* \param nodeId ID of node to reconstruct
	* 
	*/
	virtual void Reconstruct(uint8_t nodeId) = 0;

	/**
	* \brief starts the reconstruction for all nodes
	*
	*/
	virtual void ReconstructAll() = 0;

	/**
	* \brief Adds a source node whose measurement data shall be reconstructed, default sizes are used for the buffer
	*
	* \param nodeId 8bit node Id
	* \param seed 	Seed used for constructing the sensing matrix of each node
	*
	*/
	void AddSrcNode(uint8_t nodeId, uint32_t seed);

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
	void AddSrcNode(uint8_t nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax, uint32_t vecLen);

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
	* \brief writes from data buffer to a this NodeDataBuffer, should be used in subclasses own write function
	*
	* \param nodeId	8bit-ID of the node
	* \param buffer pointer to data
	* \param bufSize  size of data buffer	
	*
	*/

	uint32_t WriteData(uint8_t nodeId, double *buffer, uint32_t bufSize);

	/**
	* \brief reads reconstructed data to  a buffer, should be used in subclasses own read function
	*
	* \param nodeId	8bit-ID of the node
	* \param buffer pointer to a buffer
	* \param bufSize  size of data buffer, must be == nMeas (see SetBufferDim)
	*
	* \return remaining buffer size
	*/
	void ReadRecData(uint8_t nodeId, double *buffer, uint32_t bufSize) const;

	/**
	* \brief gets the NOF buffered measurement vectors for a node
	*
	* \param nodeId 8bitNodeId
	*
	* \return NOF  measurement vectors
	*/
	uint32_t GetNofMeas(uint8_t nodeId) const;

	/**
	* \brief gets the buffered data of a node as a matrix
	*
	* \param nodeId node ID
	*
	* \return Mat<double> the buffered data (const reference)
	*/
	const Mat<double> &GetBufMat(uint8_t nodeId);

	/**
	* \brief gets the sparse sensing matrix constructed out of the meta data for the given node ID
	*
	* \param nodeId node ID
	*
	* \return sparse sensing matrix
	*/
	virtual Mat<double> GetSensMat(uint8_t nodeId);

  private:
	typedef NodeDataBuffer<double> T_NodeBuffer;

	uint32_t m_nNodes;									 /**< NOF nodes from which we are gathering data*/
	std::map<uint8_t, Ptr<T_NodeBuffer>> m_nodeInBufMap, /**< input data of each node*/
		m_nodeOutBufMap;								 /**< output data of each node after reconstruction*/
	std::map<uint8_t, uint32_t> m_nodeSeedMap;			 /**< seed to construct the sensing matrix for each node*/
	uint32_t m_nMeasDef, m_mMaxDef, m_vecLenDef;		 /**< default buffer dimensions*/
};

#endif //RECONSTRUCTOR_H