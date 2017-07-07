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

NS_OBJECT_ENSURE_REGISTERED(CsHeader);

CsHeader::CsHeader() : m_clusterId(0),
					   m_nodeId(0),
					   m_seq(0),
					   m_dataSize(0)
{
}

void CsHeader::SetClusterId(CsHeader::T_IdField clusterId)
{
	m_clusterId = clusterId;
}

void CsHeader::SetNodeId(CsHeader::T_IdField nodeId)
{
	m_nodeId = nodeId;
}

void CsHeader::SetSeq(CsHeader::T_SeqField seq)
{
	m_seq = seq;
}

void CsHeader::SetDataSize(CsHeader::T_SizeField size)
{
	m_dataSize = size;
}

CsHeader::T_IdField CsHeader::GetClusterId()
{
	return m_clusterId;
}

CsHeader::CsHeader::T_IdField CsHeader::GetNodeId()
{
	return m_nodeId;
}

CsHeader::T_SeqField CsHeader::GetSeq()
{
	return m_seq;
}

CsHeader::T_SizeField CsHeader::GetDataSize()
{
	return m_dataSize;
}

TypeId CsHeader::GetTypeId()
{
  static TypeId tid = TypeId("CsHeader")
                          .SetParent<Header>()
                          .SetGroupName("CompressedSensing")
                          .AddConstructor<CsHeader>();
  return tid;
}

TypeId CsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t CsHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_clusterId = i.ReadU8();
  m_nodeId = i.ReadU8();
  m_seq = i.ReadNtohU16();
  m_dataSize = i.ReadNtohU16();
  return GetSerializedSize();
}

uint32_t CsHeader::GetSerializedSize() const
{
  return m_hSize;
}

void CsHeader::Print(std::ostream &os) const
{
  os << "cluster ID: " << static_cast<int>(m_clusterId) << "\t";
  os << "node ID: " << static_cast<int>(m_nodeId) << "\t";
  os << "SEQ: " << m_seq << "\t";
  os << "SIZE: " << m_dataSize << "\t";
}

void CsHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_clusterId);
  i.WriteU8(m_nodeId);
  i.WriteHtonU16(m_seq);
  i.WriteHtonU16(m_dataSize);
}