/**
* \file simple-header.cc
*
* \author Tobias Waurick
* \date 22.05.17
*
*/
#include "simple-header.h"
#include "ns3/log.h"
#include <iomanip>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleHeader");

NS_OBJECT_ENSURE_REGISTERED(SimpleHeader);

TypeId
SimpleHeader::GetTypeId()
{
  static TypeId tid = TypeId("SimpleHeader")
                          .SetParent<Header>()
                          .SetGroupName("SimpleNetwork")
                          .AddConstructor<SimpleHeader>();
  return tid;
}

TypeId
SimpleHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

SimpleHeader::SimpleHeader() : m_id(0),
                               m_dataSize(0),
                               m_data(0)
{
}

SimpleHeader::~SimpleHeader()
{
  m_id = 0;
  m_dataSize = 0;
  delete[] m_data;
}

uint8_t
SimpleHeader::GetPId() const
{
  return m_id;
}

void SimpleHeader::SetPId(uint8_t id)
{
  m_id = id;
}

uint32_t
SimpleHeader::SetData(uint8_t *buffer, uint32_t bufferSize)
{
  if (bufferSize != m_dataSize)
  {
    delete[] m_data;
    m_dataSize = bufferSize;
    m_data = new uint8_t[m_dataSize];
  }
  std::memcpy(m_data, buffer, bufferSize);

  return GetSerializedSize();
}

uint32_t
SimpleHeader::GetData(uint8_t *buffer, uint32_t bufferSize) const
{
  NS_ASSERT(bufferSize <= m_dataSize);
  std::memcpy(buffer, m_data, bufferSize);

  return GetSerializedSize();
}


uint32_t
SimpleHeader::GetDataSize() const
{
  return m_dataSize;
}

uint32_t
SimpleHeader::GetSerializedSize() const
{
  return (m_hSize + m_dataSize);
}

uint32_t
SimpleHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_id = i.ReadU8();
  if (!m_dataSize)
  {
    m_dataSize = i.GetRemainingSize();
    m_data = new uint8_t[m_dataSize];
  }
  i.Read(m_data, m_dataSize);
  return GetSerializedSize();
}

void SimpleHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_id);
  if (m_dataSize)
    i.Write(m_data, m_dataSize);
}

void SimpleHeader::Print(std::ostream &os) const
{
  os << "ID: " << static_cast<int>(m_id) << "\t";
  os << "Data: ";
  for (uint32_t i = 0; i < m_dataSize; i++)
  {
    os << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(m_data[i]) << " ";
  }
  os << std::dec << std::endl;
}