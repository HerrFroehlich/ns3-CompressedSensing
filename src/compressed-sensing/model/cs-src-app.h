/**
* \file cs-src-app.h
*
* \author Tobias Waurick
* \date 12.07.17
*
*/
#ifndef CS_SRCAPP_H
#define CS_SRCAPP_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "cs-header.h"
#include "compressor.h"
#include "ns3/serial-buffer.h"
#include "cs-node.h"

using namespace ns3;

/**
* \ingroup compsens
* \defgroup csApps Applications
*
* Applications for Source, Cluster and Sink nodes
*/

/**
* \ingroup csApps
* \class CsSrcApp
*
* \brief A source app to compress data from a file temporally and transmitting it
*
* When running the setup this application reads data from a file specified by a file path and stores it locally.\n
* Upon starting the application the n data samples are compressed to m with the help of the Compressor class and a given seed.
* Then packets are formed containing the CsHeader and m samples as payload (so package loss won't corrupt a data vector).
* Finally the application transmits them using no protocol at all (-> assuming use of classes from the simple-network module) 
* with a certain time gap between each packet.
*/
class CsSrcApp : public Application
{
  public:
	typedef double T_PktData;						   /**< package data type*/
	const std::string STREAMNAME = "ComprTemp"; /**< name of DataStream storing temporal compression results*/

	static TypeId GetTypeId(void);

	/**
	* \brief create an CsSrcApp with default values
	*/
	CsSrcApp();

	/**
	* \brief create an CsSrcApp
	*
	* \param n length of original measurement vector
	* \param m length of compressed vector
	*/
	CsSrcApp(uint32_t n, uint32_t m);

	/**
	* \brief setups the application to send packets with data from a file.
	*
	* This function has to be called BEFORE starting the application. 
	*
	* \param node CsNode to aggregate application to
	* \param input SerialDataBuffer<double> with input data for the node
	*
	*/
	virtual void Setup(Ptr<CsNode> node, Ptr<SerialDataBuffer<double>> input);

	/**
	* \brief Sets the used temporal compressor.
	*
	*  It is setuped with the previously defined/default seed and sizes 
	* \param comp  pointer to compressor
	*/
	void SetTempCompressor(Ptr<CompressorTemp<double>> comp);

	/**
	* \brief sets the used temporal compressor
	*
	*  It is setuped via the given seed and sizes
	*
	* \param comp  pointer to compressor
	* \param seed seed to use
	* \param n length of original measurement vector
	* \param m length of compressed vector
	* \param norm normalize random matrix by 1/sqrt(m)?
	*/
	// void SetTempCompressor(Ptr<CompressorTemp<double>> comp, uint32_t n, uint32_t m, bool norm = false);

	/**
	* \brief sets the compression given by n and m
	*
	* \param n length of original measurement vector
	* \param m length of compressed vector
	*/
	void SetTempCompressDim(uint32_t n, uint32_t m);

	/**
	* \brief sets the seed used for generating the random sensing matrix
	*
	* \param seed seed to use
	* \param norm normalize random matrix by 1/sqrt(m)?
	*
	*/
	// void SetSeed(uint32_t seed, bool norm = false);

	/**
	* \brief sets the transmission probability for sending
	*
	* \param p probability to send
	*
	*/
	void SetTxProb(double p);

	//inherited from Application
	virtual void StartApplication();
	virtual void StopApplication();

  protected:
	/**
	* \brief send a packet via all tx devices
	*
	* \param p packet to send
	*/
	void SendToAll(Ptr<Packet> p);

	/**
	* \brief tries to compress the next Y and with that  to 
	* The base class does temporal compression here. 
	*
	* \return true when a new Y could be compressed
	*/
	virtual bool CompressNext();

	/**
	* \brief create new packets with a CsHeader and payload
	*/
	virtual void CreateCsPackets();

	/**
	* \brief writes to the TX packet list 
	* The last package in the vector will be send first
	*
	* \param pktList vector containing packets (Ptr<Packet>), which will be transmitted
	*
	*/
	void WriteTxPacketList(const std::vector<Ptr<Packet>> &pktList);

	/**
	* \brief gets the maximum payload size
	* CsSrcApp will packets will have a fixed sized of m*size(T_PktData), so that one compressed measurement fits in one packet.
	* So this function provides the option for subclasses to calculate alternative sizes (e.g splitting Y over several packets).
	* 
	* \param pktSize maximum packet size
	*
	* \return calculated packetSize
	*/
	virtual uint32_t GetMaxPayloadSize();

	/**
	* \brief checks if has queued packets
	*	
	* \return true if there are packets queued
	*/
	bool HasPackets();

	/**
	* \brief schedules a transmit event for the simulator
	*
	* \param dt Time to next transmit event   
	*
	*/
	void ScheduleTx(Time dt);

	/**
	* \brief check if application is already sending packets
	*
	* \return true when packets are send
	*/
	bool IsSending();

	SerialDataBuffer<double> m_yR; /**< buffers for  compressed real meas. vector */
	CsHeader::T_IdField m_nodeId, m_clusterId;
	CsHeader::T_SeqField m_nextSeq; /**< next sequence!*/

	uint32_t
		m_seed, /**< seed used for generating the random sensing matrix*/
		m_n,	/**< length of an original measurement vector*/
		m_m,	/**< length of compressed measurment vector*/
		// m_nDevices,   /**< number of net devices*/
		// m_nTxDevices, /**< number of net devices*/
		//	m_packetSize, /**< Packet size in byte*/
		// m_nMeas,	  /**< NOF measurements to send*/
		//m_nPackets,   /**< NOF packets to send*/
		m_sent;			/**< NOF packets already sent*/
	Ptr<CsNode> m_node; /**< aggretated node*/
  private:
	/**
	* \brief sends a packet with compressed source data  via all devices in TX-device list
	* if there are packets remaining in m_txPackets this will call another ScheduleTx
	*
	* \param p packet to send
	*
	*/
	void SendPacket(Ptr<Packet> p);

	double m_txProb; /**< propability to send a packet*/

	bool m_normalize, /**< normalize random matrix by 1/sqrt(m)?*/
		m_running,
		m_isSetup;

	Ptr<SerialDataBuffer<double>> m_fdata; /**< data from file*/
	Ptr<CompressorTemp<double>> m_compR;   /**< compressor for real*/
	Ptr<RandomVariableStream> m_ranTx;	 /**< random variable stream, to determine when to send*/
	// std::vector<uint32_t> m_isTxDevice;   /**< determine if device is used for sending */
	std::vector<Ptr<Packet>> m_txPackets;  /**< packets to send next*/
	Ptr<DataStream<double>> m_stream; /**< data stream storing compression results*/

	Time m_interval; /**< Packet inter-send time*/
	EventId m_sendEvent;
	TracedCallback<Ptr<const Packet>> m_txTrace, m_dropTrace; /**< callback to call when sending/ packet is dropped*/
};

#endif //CS_SRCAPP_H