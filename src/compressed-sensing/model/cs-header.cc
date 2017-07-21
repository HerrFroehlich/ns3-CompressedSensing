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
                       m_dataSize(0),
                       m_srcInfo()
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

void CsHeader::SetSrcInfo(const std::bitset<CsHeader::SRCINFO_BITLEN> &set)
{
  NS_ASSERT_MSG(m_nodeId == CLUSTER_NODEID, "Must be a cluster node!");
  m_srcInfo = set;
}

std::bitset<CsHeader::SRCINFO_BITLEN> CsHeader::GetSrcInfo() const
{
  NS_ASSERT_MSG(m_nodeId == CLUSTER_NODEID, "Must be a cluster node!");
  return m_srcInfo;
}

TypeId CsHeader::GetTypeId()
{
  static TypeId tid = TypeId("CsHeader")
                          .SetParent<Header>()
                          .SetGroupName("CompressedSensing")
                          .AddConstructor<CsHeader>();
  return tid;
}

TypeId CsHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

uint32_t CsHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_clusterId = i.ReadU8();
  m_nodeId = i.ReadU8();
  m_seq = i.ReadNtohU16();
  m_dataSize = i.ReadNtohU16();

  if (m_nodeId == CLUSTER_NODEID)
  {
    for (uint32_t j = 0; j < SRCINFO_LEN; j++)
    {
      uint8_t byte = i.ReadU8();
      for (size_t i = 0; i < BYTE_LEN; i++)
      {
        m_srcInfo[i + j * BYTE_LEN] = (1 << i) & byte;
      }
    }
  }

  return GetSerializedSize();
}

uint32_t CsHeader::GetSerializedSize() const
{
  uint32_t size = m_hSizeSrc;
  if (m_nodeId == CLUSTER_NODEID)
    size = m_hSizeCluster;

  return size;
}

void CsHeader::Print(std::ostream &os) const
{
  os << "Cluster ID: " << static_cast<int>(m_clusterId) << "\t";
  os << "Node ID: " << static_cast<int>(m_nodeId) << "\t";
  os << "SEQ: " << m_seq << "\t";
  os << "SIZE: " << m_dataSize << std::endl;

  if (m_nodeId == CLUSTER_NODEID)
  {
    os << "Node Info: " << m_srcInfo << std::endl;
  }
}

void CsHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_clusterId);
  i.WriteU8(m_nodeId);
  i.WriteHtonU16(m_seq);
  i.WriteHtonU16(m_dataSize);

  if (m_nodeId == CLUSTER_NODEID)
  {
    for (uint32_t j = 0; j < SRCINFO_LEN; j++)
    {
      uint8_t byte = 0;
      for (size_t i = 0; i < BYTE_LEN; i++)
      {
        byte |= (m_srcInfo[i + j * BYTE_LEN] << i);
      }
      i.WriteU8(byte);
    }
  }
}
