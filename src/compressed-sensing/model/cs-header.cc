/**
* \file cs-header.cc
*
* \author Tobias Waurick
* \date 07.07.17
*
*/

#include <iostream>
#include "cs-header.h"
#include "ns3/log.h"
#include "assert.h"

NS_OBJECT_ENSURE_REGISTERED(CsHeader);

CsHeader::CsHeader() : m_clusterId(0),
                       m_nodeId(0),
                       m_seq(0),
                       m_dataSize(0)
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::SetClusterId(CsHeader::T_IdField clusterId)
{
  m_clusterId = clusterId;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::SetNodeId(CsHeader::T_IdField nodeId)
{
  m_nodeId = nodeId;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::SetSeq(CsHeader::T_SeqField seq)
{
  m_seq = seq;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::SetDataSize(CsHeader::T_SizeField size)
{
  m_dataSize = size;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsHeader::T_IdField CsHeader::GetClusterId()
{
  return m_clusterId;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsHeader::CsHeader::T_IdField CsHeader::GetNodeId()
{
  return m_nodeId;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsHeader::T_SeqField CsHeader::GetSeq()
{
  return m_seq;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsHeader::T_SizeField CsHeader::GetDataSize()
{
  return m_dataSize;
}


/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId CsHeader::GetTypeId()
{
  static TypeId tid = TypeId("CsHeader")
                          .SetParent<Header>()
                          .SetGroupName("CompressedSensing")
                          .AddConstructor<CsHeader>();
  return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId CsHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsHeader::Deserialize(Buffer::Iterator start)
{
  return DoDeserialize(start);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsHeader::DoDeserialize(Buffer::Iterator &start)
{
  m_clusterId = start.ReadU8();
  m_nodeId = start.ReadU8();
  m_seq = start.ReadNtohU16();
  m_dataSize = start.ReadNtohU16();

  return GetSerializedSize();
}
/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsHeader::GetSerializedSize() const
{
  uint32_t size = m_hSizeSrc;

  return size;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::Print(std::ostream &os) const
{
  os << "Cluster ID: " << static_cast<int>(m_clusterId) << "\t";
  os << "Node ID: " << static_cast<int>(m_nodeId) << "\t";
  os << "SEQ: " << m_seq << "\t";
  os << "SIZE: " << m_dataSize << std::endl;

}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::Serialize(Buffer::Iterator start) const
{
  DoSerialize(start);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsHeader::DoSerialize(Buffer::Iterator &start) const
{
  start.WriteU8(m_clusterId);
  start.WriteU8(m_nodeId);
  start.WriteHtonU16(m_seq);
  start.WriteHtonU16(m_dataSize);
}
