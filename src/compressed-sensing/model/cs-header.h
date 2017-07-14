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
#include "ns3/header.h"
#include "ns3/buffer.h"
using namespace ns3;
/**
* \ingroup compsens
* \class CsHeader
*
* \brief header for a clustered sensor network performing in-network compressions and recovery via compressed sensing techniques
*
* The header consist of 4 fields :
*                   48bit=6byte
* 8bit ClusterID: the identification number of the current cluster\n 
* 8bit NodeID: the identification number of the current source node, 0 is received for the cluster head\n 
* 16bit SEQ: current measurment sequence number\n 
* 16bit SIZE: size of sended data\n 
* Totaling 48bit=6byte
*/
class CsHeader : public Header
{
  public:
	typedef uint8_t T_IdField;	/**< ID field type*/
	typedef uint16_t T_SizeField; /**< size field type*/
	typedef uint16_t T_SeqField;  /**< sequence field type*/

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

//inherited from class Header
	static TypeId GetTypeId();
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;
  private:
	static const uint32_t m_hSize = 6; /**< Header size in Byte*/
	T_IdField m_clusterId,			   /**< 8bit cluster ID*/
		m_nodeId;					   /**< 8bit node ID*/
	T_SeqField m_seq;				   /**< sequence number*/
	T_SizeField m_dataSize;			   /**< payload size in byte*/
};

#endif //CS_HEADER_H