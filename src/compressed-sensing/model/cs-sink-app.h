/**
* \file cs-sink-app.h
*
* \author Tobias Waurick
* \date 21.07.2017
*
*/

#ifndef CS_SINK_APP_H
#define CS_SINK_APP_H

#include <string>
#include <limits>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "cs-cluster-header.h"
#include "ns3/cs-cluster.h"
#include "reconstructor.h"
#include "ns3/data-stream.h"

using namespace ns3;

/**
* \ingroup csApps
* \class CsSinkApp
*
* \brief A sink app receiving from several clusters, reconstructing spatially and temporally
*
* description
*
*/
class CsSinkApp : public Application
{
  public:
	static const std::string NRX_STREAMNAME; /**< name of DataStream storing NOF packets received per sequence*/

	enum E_DropCause
	{
		NOT_A_CLUSTER,  /**< received data from non cluster node  */
		EXPIRED_SEQ,	/**< received old sequence number*/
		UNKNOWN_CLUSTER /**< received from cluster which was not registered with AddCluster*/
	};
	typedef void (*RxDropCallback)(Ptr<const Packet>, E_DropCause); /**< callback signature:  dropping a received packet*/
	static TypeId GetTypeId(void);

	/**
	* \brief create an CsSinkApp with default values
	*/
	CsSinkApp();
	/**

	* \brief setups the application to receive packets and writing to a  directory.
	*
	* This function has to be called BEFORE starting the application. 
	*
	* \param node CsNode to aggregate application to
	* \param dir directory to write files to
	*
	*/
	void Setup(Ptr<CsNode> node);

	/**
	* \brief Adds a cluster to receive from with given compression
	*
	* Asserts that the cluster with the given ID was added only once.
	*
	* \param cluster CsCluster with nodes
	*
	*/
	void AddCluster(Ptr<CsCluster> cluster);

	//inherited from Application
	// virtual void StartApplication();
	// virtual void StopApplication();
  private:
	/**
	* \brief action on net device receive
	*
	* The received data will be stored and processed
	*
	* \param dev pointer to the net device
	* \param p   received packet
	* \param idUnused   protocol id UNUSED
	* \param adrUnused  sender address UNUSED
	*
	*/
	bool Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused);

	/**
	* \brief starts reconstruction of next sequence
	*
	*/
	void ReconstructNext();

	/**
	* \brief buffers packets datas
	*
	* \param p	Pointer to Packet
	*
	*/
	void BufferPacketData(Ptr<const Packet> p);

	/**
	* \brief prepares  for reconstructing  a new measurement sequence	
	*
	* This method resets all input buffers of the Reconstructor.
	*
	* \param seqDiff difference in sequence numbers
	*
	*/
	void StartNewSeq(uint32_t seqDiff);

	Ptr<CsNode> m_node; /**< aggretated sink node*/

	Ptr<Reconstructor> m_reconst; /**<  reconstructor*/

	CsHeader::T_SeqField m_seqCount; /**< measurment sequence counter*/
	uint32_t m_recAttempt;			 /**< reconstruction attempt of current measurement sequence*/

	bool m_isSetup;			/**< was setup called ?*/
	Time m_timeout;			/**< Packet inter-send time*/
	EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/

	TracedCallback<Ptr<const Packet>> m_rxTrace;				  /**< received a packet*/
	TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/

	uint32_t m_minPackets, /**< minmum NOF packets to start reconstructing*/
		m_rxPacketsSeq;	/**< NOF received packets for this measurement sequence*/
	Ptr<DataStream<double>> m_rxCnt_stream;
};

#endif //CS_SINK_APP_H
