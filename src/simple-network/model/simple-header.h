/**
* \file simple-header.h
*
* \author Tobias Waurick
* \date 22.05.17
*
*/
#ifndef SIMPLE_HEADER_H
#define SIMPLE_HEADER_H

#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/header.h"
#include "ns3/type-id.h"
#include "ns3/buffer.h"
using namespace ns3;
/**
* \class SimpleHeader
*
* \brief a simple header containing an 8bit-ID and variable payload
*
*
* \author Tobias Waurick
* \date 22.05.17
*/
class SimpleHeader : public Header
{
  public:
	SimpleHeader();
	virtual ~SimpleHeader();
	uint8_t GetPId() const;  /**< returns 8bit id stored in package*/
	void SetPId(uint8_t id); /**< sets 8bit id stored in package*/

	/**
	* \brief gets the payload data into a buffer
	*
	* \param *buffer byte buffer to write to
	* \param bufferSize size of buffer, must be size of stored data field	
	*
	* \return The serialized header size
	*/
	uint32_t GetData(uint8_t *buffer, uint32_t bufferSize) const;

	/**
	* \brief get the size of stored data
	*
	*
	* \return data field size
	*/
	uint32_t GetDataSize() const;

	/**
	* \brief Sets the payload data from a buffer
	*
	* \param *buffer byte buffer to read from
	* \param bufferSize size of buffer, sets size of stored data field	
	*
	* \return The serialized header size
	*/
	uint32_t SetData(uint8_t *buffer, uint32_t bufferSize);
	//inherited from class Header
	static TypeId GetTypeId();
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;

  private:
	static const uint32_t m_hSize = 1; /**< Header size in Byte*/
	uint8_t m_id;					   /**< 8bit ID*/
	uint32_t m_dataSize;			   /**< payload size in byte*/
	uint8_t *m_data;				   /**< payload data*/
};

#endif //SIMPLE_HEADER_H