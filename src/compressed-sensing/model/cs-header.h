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
* \class CsHeader
*
* \brief header for a clustered sensor network performing in-network compressions and recovery via compressed sensing techniques
*
* The header consist of 4 fields :
* 8bit ClusterID: the identification number of the current cluster\n 
* 8bit NodeID: the identification number of the current source node, 0 is received for the cluster node\n 
* 16bit SEQ: current measurement sequence number\n 
* 16bit SIZE: size of sent data\n 
* Totaling 48bit=6byte\n
*
* For a cluster node the header is extended by the following fields:
* 256 bit SrcInfo : indicates which source nodes were compressed, 1bit describing one node, the LSB bit representing the nodeID 0\n
* Totaling 304bit=38byte\n
*/
class CsHeader : public Header
{
  public:
	typedef uint8_t T_IdField;	/**< ID field type*/
	typedef uint16_t T_SizeField; /**< size field type*/
	typedef uint16_t T_SeqField;  /**< sequence field type*/

	const static T_IdField CLUSTER_NODEID = 0;										/**< the node id of a cluster node*/
	const static uint32_t MAX_SRCNODES = 255;	 									/**< maximum NOF source nodes*/
	const static uint32_t MAX_CLUSTERNODES = 256; 									/**< maximum NOF source nodes*/
	const static uint32_t SRCINFO_LEN = sizeof(T_IdField) * 32;						/**< length of source info field in byte*/
	const static uint32_t SRCINFO_BITLEN = SRCINFO_LEN * BYTE_LEN;					/**< length of source info field in bit*/

	typedef std::bitset<SRCINFO_BITLEN> T_SrcInfo;
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
	void SetNodeId(T_IdField nodeId);

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

	/**
	* \brief adds info of source nodes used in cluster compression from a bitset
	*
	* The method throws an error, if the node id does not equal CLUSTER_NODEID
	*
	* \param set std::bitset to set
	*
	*/
	void SetSrcInfo(const std::bitset<SRCINFO_BITLEN> &set);

	/**
	* \brief gets the info of source nodes used in cluster compression
	*
	* The method throws an error,d if the nodeId does not equal CLUSTER_NODEID
	*
	* \return bit set containing the info
	*
	*/
	std::bitset<SRCINFO_BITLEN> GetSrcInfo() const;

	//inherited from class Header
	static TypeId GetTypeId();
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;

  private:
	static const uint32_t m_hSizeSrc = 6;	  /**< Header size in byte for src*/
	static const uint32_t m_hSizeCluster = 38; /**< Header size in byte for cluster*/
	T_IdField m_clusterId,					   /**< 8bit cluster ID*/
		m_nodeId;							   /**< 8bit node ID*/
	T_SeqField m_seq;						   /**< sequence number*/
	T_SizeField m_dataSize;					   /**< size in byte*/
	std::bitset<SRCINFO_BITLEN> m_srcInfo;	 /**< info about source nodes*/
};

#endif //CS_HEADER_H