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
#include "cs-header.h"
#include "ns3/cs-cluster.h"
#include "reconstructor.h"
#include "ns3/data-stream.h"

using namespace ns3;

/**
* \ingroup csApps
* \class CsSinkApp
*
* \brief A sink app receiving from several clusters, reconstructing spatially and temporally and saves the result to binary files
*
* description
*
*/
class CsSinkApp : public Application
{
  public:
	enum E_DropCause
	{
		NOT_A_CLUSTER,  /**< received data from non cluster node  */
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
	void Setup(Ptr<CsNode> node, std::string dir);

	/**
	* \brief Adds a cluster to receive from with given compression
	*
	* Asserts that the cluster with the given ID was added only once.
	*
	* \param cluster CsCluster with nodes
	*
	*/
	void AddCluster(Ptr<CsCluster> cluster);

	/**
	* \brief set the temporal reconstructor
	*
	* \param rec pointer to the reconstructor
	*
	*/
	// void SetTemporalReconstructor(Ptr<Reconstructor<double>> rec);

	/**
	* \brief set the spatial reconstructor
	*
	* \param rec pointer to the reconstructor
	*
	*/
	// void SetSpatialReconstructor(Ptr<Reconstructor<double>> rec);

	//inherited from Application
	// virtual void StartApplication();
	// virtual void StopApplication();

  private:
	struct SeqChecker /**< check for new measurement sequence, TODO overflow???*/
	{
	  public:
		SeqChecker(uint32_t l) : m_lastSeq(0), m_l(l), m_packDiff(0), m_seqDiff(0) {}

		/**
		* \brief adds new packet sequence
		*
		* \return true if enough packets were received to complete a measurement sequence
		*/
		bool AddNewSeq(CsHeader::T_SeqField seq)
		{
			bool ret = false;
			int32_t diff = seq - m_lastSeq;
			if (diff < 0) //overflow
				diff += std::numeric_limits<CsHeader::T_SeqField>::max();

			m_packDiff += diff;
			m_seqDiff = diff;
			if (m_packDiff == m_l)
			{
				ret = true;
				m_packDiff = 0;
			}

			m_lastSeq = seq;
			return ret;
		}

		CsHeader::T_SeqField GetSeqDiff() /**< get difference to last sequence*/
		{
			return m_seqDiff;
		}

	  private:
		CsHeader::T_SeqField m_lastSeq; /**< last package sequence number*/
		uint32_t m_l;					/**< NOF packets per measurement sequence*/
		uint32_t m_packDiff,			/**< difference of packets to full measurement sequence*/
			m_seqDiff;					/**< difference to last sequence*/
	};

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
	void ReconstructNext(bool newSeq);

	/**
	* \brief buffers packets data
	*
	* If diff > 1, diff*m zeros will append in front (missing rows of Z_k)
	*
	* \param p	Pointer to Packet
	* \param diff difference to last sequence
	*
	*/
	void BufferPacketData(Ptr<const Packet> p, uint32_t diff);

	/**
	* \brief prepares  for reconstructing  a new measurement sequence	
	*
	* This method resets all input buffers.
	*
	*/
	void StartNewSeq();

	Ptr<CsNode> m_node; /**< aggretated sink node*/

	Ptr<Reconstructor> m_reconst; /**<  reconstructor*/

	CsHeader::T_SeqField m_seqCount;						 /**< measurment sequence counter*/
	uint32_t m_recAttempt;									 /**< reconstruction attempt of current measurement sequence*/
	std::map<CsHeader::T_IdField, SeqChecker> m_seqCheckMap; /**< checking for new Sequence*/

	std::map<CsHeader::T_IdField, Ptr<CsCluster>> m_clusters; /**< clusters ordered with id*/

	bool m_isSetup;
	Time m_timeout;			/**< Packet inter-send time*/
	EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/

	std::string m_dir,											  /**< output directory*/
		m_filebase;												  /**< Base filename for output files*/
	TracedCallback<Ptr<const Packet>> m_rxTrace;				  /**< received a packet*/
	TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/

	uint32_t m_minPackets,											   /**< minmum NOF packets to start reconstructing*/
		m_rxPackets;												   /**< NOF received packets*/
	const static uint32_t N_SRCNODES_MAX = CsHeader::MAX_SRCNODES + 1; /**< maximum NOF source nodes, +1 since cluster is also source!*/
};

#endif //CS_SINK_APP_H
