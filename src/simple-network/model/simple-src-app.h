/**
* \file simple-src-app.h
*
* \author Tobias Waurick
* \date 16.05.17
*
* 
*/
#ifndef SIMPLE_SRC_APP_H
#define SIMPLE_SRC_APP_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "simple-header.h"
#include <string>

/**
* \class DataBuffer
*
* \brief a general buffer class to store and read data
*		
*
* \tparam T	the type of stored data
*
* \author Tobias Waurick
* \date 30.05.17
*/
template <typename T>
class DataBuffer
{
  public:
	/**
	* \brief creates an empty DataBuffer
	*
	*/
	DataBuffer();

	/**
	* \brief creates a DataBuffer with a certain size
	*
	* \param size size of the buff
	*
	*/
	DataBuffer(uint32_t size);

	/**
	* \brief destroy the Biffer
	*
	*/
	~DataBuffer();

	/**
	* \brief reads next value this DataBuffer
	*
	* \return next value in DataBuffer
	*/
	T ReadNext();

	/**
	* \brief reads multiple next values in a buffer given by a pointer
	*
	*\param DataBuffer	a DataBuffer pointer of the stored type
	*\param bufSize	size of that DataBuffer
	*/
	void ReadNext(T *DataBuffer, uint32_t bufSize);

	/**
	* \brief writes a single value into this DataBuffer
	*
	* \param data data of underlaying type to store
	*/
	void WriteNext(T data);

	/**
	* \brief writes multiple values at the end of this DataBuffer
	*
	*\param DataBuffer	a DataBuffer pointer of the stored type
	*\param bufSize	size of that DataBuffer
	*/
	void WriteNext(T *DataBuffer, uint32_t bufSize);

	/**
	* \brief deletes and resizes the data buffer
	*
	* \param size size of new buffer
	*
	*/
	void Resize(uint32_t size);

  private:
	T *m_data_ptr;		 /**< stored data*/
	uint32_t m_dataSize, /**< size of stored data*/
		m_wrIdx,		 /**< current write index*/
		m_rdIdx;		 /**< current read index*/
};


template <typename T>
DataBuffer<T>::DataBuffer() : m_data_ptr(0),
						   m_dataSize(0),
						   m_wrIdx(0),
						   m_rdIdx(0)
{}

template <typename T>
DataBuffer<T>::DataBuffer(uint32_t dataSize) : m_dataSize(dataSize),
											m_wrIdx(0),
											m_rdIdx(0)
{
	m_data_ptr = new T[dataSize];
}

template <typename T>
DataBuffer<T>::~DataBuffer()
{
	delete[] m_data_ptr;
	m_data_ptr = 0;
	m_dataSize = 0;
}

template <typename T>
T DataBuffer<T>::ReadNext()
{
	NS_ASSERT_MSG(m_rdIdx <= m_wrIdx, "Read index > write index : Trying to read outside of written area!");

	return m_data_ptr[m_rdIdx++];
}

template <typename T>
void DataBuffer<T>::ReadNext(T *DataBuffer, uint32_t bufSize)
{
	NS_ASSERT_MSG((m_rdIdx + bufSize) <= m_wrIdx,
				  "Read index > write index : Trying to read outside of written area!");

  memcpy(DataBuffer, m_data_ptr + m_rdIdx, bufSize);
  m_rdIdx += bufSize;
}

template <typename T>
void DataBuffer<T>::WriteNext(T data)
{
	NS_ASSERT_MSG(m_wrIdx < m_dataSize, "Write index > data size : Trying to write outside of allocated area!");
	m_data_ptr[m_wrIdx++] = data;
}

template <typename T>
void DataBuffer<T>::WriteNext(T *buffer, uint32_t bufSize)
{
	NS_ASSERT_MSG((m_wrIdx + bufSize - 1) < m_dataSize,
				  "Write index > data size : Trying to write outside of allocated area!");
	memcpy(m_data_ptr+m_wrIdx, buffer, bufSize);
	m_wrIdx += bufSize;
}

template <typename T>
void DataBuffer<T>::Resize(uint32_t size)
{
	delete[] m_data_ptr;
	m_dataSize = size;
	m_wrIdx = 0;
	m_rdIdx = 0;
	m_data_ptr = new T[m_dataSize];
}


/**
* \class SimpleSrcApp
*
* \brief simple node application transmitting without transport protocols
*
* This simple application assumes a simple net device and a simple p2p channel.
* Data is generated randomly  and is stored as a IEEE754 double value. Therefor payload size is assumed to be a multible of sizeof(double)
* A packet consist of this data and an 8bit node id.
* No transport protocol is used and a packet is send directly via all net devices aggregated to a node,
* except for net devices wich are set for receiving via their device idx.
* The hereby received packages are relayed via the TX net devices.
* The source nodes may start sending at a random start time if desired;
* All devices should be installed before installing the application!
*
* \author Tobias Waurick
* \date 22.05.17
*/
class SimpleSrcApp : public Application
{
public:
  static TypeId GetTypeId(void);
  SimpleSrcApp();
  virtual ~SimpleSrcApp();
  /**
  * \brief setups the application to send packets with random data,all aggregated devices are considered Tx devices  
  *
  * \param 8bit node id
  * \param node node to aggregate application to
  */
  void Setup(uint8_t nodeId, Ptr<Node> node);
  /**
  * \brief setups the application to send packets with data from a file, ignores nofPackets Attribute,
  *  all aggregated devices are considered Tx devices  
  *
  * \param 8bit node id
  * \param node node to aggregate application to
  * \param fileName name of file to read from
  *
  */
  void Setup(uint8_t nodeId, Ptr<Node> node, std::string fileName);
  void SetRandomVariable(Ptr<RandomVariableStream> ranvar);
  /**
  * \brief setups the application to relay received packets (using indexed netdevice)
  *         Does not check if net devices since setup call hav ebben installed! 
  *
  * \param deviceIdx   index of net device,aggregated to node, which is used for rx&relaying
  *
  */
  void SetupRelay(uint32_t deviceIdx);

  virtual void StartApplication(void);
  virtual void StopApplication(void);
  /**
  * \brief action on net device receive if setup
  *
  * \param dev pointer to the net device
  * \param p   received packet
  * \param idUnused   protocol id UNUSED
  * \param adrUnused  sender address UNUSED
  *
  */
  bool Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused);
  

private:
  void SetupPriv(uint8_t nodeId, Ptr<Node> node);
  /**
  * \brief schedules a transmit event for the simulator
  *
  * \param dt Time to next transmit event   
  *
  */
  void ScheduleTx(Time dt);

  /**
  * \brief Sends a packet over all tx net devices
  *
  * \param p Ptr to packet to send
  *
  */
  void SendPacket(Ptr<Packet> p);

  /**
  * \brief sends a  packet via all tx devices
  *
  * \param p Ptr to Packet to send
  */
  void SendToAll(Ptr<Packet> p);

  union doubleToBytes /**< union to create bytes from a double*/
  {
    double dbl;
    uint8_t bytes[sizeof(double)];
  };
  //Ptr<NetDevice> m_device;
  Ptr<Node> m_node;    /**< aggretated node*/
  uint32_t m_nDevices, /**< number of net devices*/
      m_nTxDevices,    /**< number of Tx net devices*/
      m_packetSize,    /**< Packet size in byte*/
      m_dataSize,      /**< size of data field*/
      m_nMeas,         /**< NOF measurements to send*/
      m_nPackets,      /**< NOF packets to send*/
      m_sent;          /**< NOF packets already sent*/
  Time m_interval;     /**< Packet inter-send time*/
  EventId m_sendEvent, m_relayEvent;
  bool m_running,
      m_isSetup;
  uint8_t m_nodeId;
  //relay attributes
  bool m_isRelay;
  Time m_relayDelay;
  Ptr<Packet> m_relayPacket;
  Ptr<NetDevice> m_relayDevice;
  Callback<bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address &> m_cb;
  TracedCallback<Ptr<const Packet>> m_txTrace;
  TracedCallback<Ptr<const Packet>> m_rxTrace;
  Ptr<RandomVariableStream> m_ranvar, m_ranStartMs;
  DataBuffer<uint8_t> m_byteBuf;
  std::vector<uint32_t> m_isTxDevice; /**< determine if device is used for sending */
};
#endif //SIMPLE_SRC_APP_H
