/**
* \file cs-header.h
*
* \author Tobias Waurick
* \date 07.07.17
*
*/

#ifndef CS_HEADER_H
#define CS_HEADER_H

#include <stdint.h>
#include <bitset> //binary representation
#include "ns3/header.h"
#include "ns3/buffer.h"
using namespace ns3;

#define BYTE_LEN 8
/**
* \ingroup compsens
* \defgroup csNet Networking
*
* Various classes needed for the networking
*/
/**
* \ingroup csNet
* \class CsHeader
*
* \brief header for a clustered sensor network performing in-network compressions and recovery via compressed sensing techniques
*
*
* The header consist of 4 fields :
* - 8bit ClusterID: the identification number of the current cluster
* - 8bit NodeID: the identification number of the current source node, 0 is received for the cluster node
* - 16bit Seq: current measurement sequence number
* - 16bit Size: size of sent data
* - Totaling 48bit=6byte
* 
*/
class CsHeader : public Header
{
  public:
	//field types
	typedef uint8_t T_IdField;	/**< ID field type*/
	typedef uint16_t T_SizeField; /**< size field type*/
	typedef uint16_t T_SeqField;  /**< sequence field type*/

	// sizes
	const static uint32_t MAX_SRCNODES = 255; /**< maximum NOF source nodes*/
	/**
	* \brief create an empty CsHeader
	*/
	CsHeader();

	/**
	* \brief sets the cluster ID
	*
	* \param clusterId ID to set
	*
	*/
	void SetClusterId(T_IdField clusterId);

	/**
	* \brief sets the node ID
	*
	* \param nodeId ID to set
	*
	*/
	virtual void SetNodeId(T_IdField nodeId);

	/**
	* \brief sets the sequence number
	*
	* \param seq sequence number
	*
	*/
	void SetSeq(T_SeqField seq);

	/**
	* \brief sets the data size field
	*
	* \param size size to set
	*
	*/
	void SetDataSize(T_SizeField size);

	/**
	* \brief gets the cluster ID
	*
	* \return cluster ID
	*
	*/
	T_IdField GetClusterId();

	/**
	* \brief gets the node ID
	*
	* \return node ID
	*
	*/
	T_IdField GetNodeId();

	/**
	* \brief gets the sequence number
	*
	* \return sequence number
	*
	*/
	T_SeqField GetSeq();

	/**
	* \brief gets the data size field
	*
	* \return data size
	*
	*/
	T_SizeField GetDataSize();

	//inherited from class Header
	static TypeId GetTypeId();
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;

  protected:
	/**
	* \brief doing deserialization
	*
	* Method to call when during deserialization
	*
	* \param start beginning of Buffer::Iterator	
	*
	* \return deserialized size
	*/
	uint32_t DoDeserialize(Buffer::Iterator &start);

	/**
	* \brief  doing serialization
	*
	*
	* \param start beginning of Buffer::Iterator
	*
	*/
	void DoSerialize(Buffer::Iterator &start) const;

  private:
	static const uint32_t m_hSizeSrc = 2 * sizeof(T_IdField) + sizeof(T_SeqField) + sizeof(T_SizeField); /**< Header size in byte for src*/
	//variables
	T_IdField m_clusterId,  /**< 8bit cluster ID*/
		m_nodeId;			/**< 8bit node ID*/
	T_SeqField m_seq;		/**< sequence number*/
	T_SizeField m_dataSize; /**< size in byte*/
};

#endif //CS_HEADER_H