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
	static const std::string SEQSTREAMNAME; /**< name base of DataStream storing packets sequence numbers*/

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
	/**
	* \brief structure class to determine if a new measurement sequence was received and how many zeros have to be padded to 
	*		 in case of packet loss.
	*
	*/
	struct SeqChecker
	{
	  public:
		SeqChecker(){};

		/**
		* \brief adds a cluster to the SeqChecker
		*
		* \param cluster pointer to cluster to add
		*
		*/
		void AddCluster(const Ptr<CsCluster> cluster)
		{
			DiffInfo diff;
			diff.lastSeq = 0;
			diff.l = cluster->GetCompression(CsCluster::E_COMPR_DIMS::l);
			diff.m = cluster->GetCompression(CsCluster::E_COMPR_DIMS::m);
			diff.packDiff = 0;
			diff.seqDiff = 0;
			diff.seqStream = Create<DataStream<double>>(SEQSTREAMNAME);
			diff.seqStream->CreateBuffer(1, cluster->GetCompression(CsCluster::E_COMPR_DIMS::l));
			cluster->AddStream(diff.seqStream);
			m_diffMap.emplace(cluster->GetClusterId(), diff);
		}

		/**
		* \brief adds new packet sequence
		*
		* \param clusterId ID of the clustere
		* \param seq	   new sequence
		*
		* \return true if enough packets were received to complete a measurement sequence
		*/
		bool AddNewSeq(CsHeader::T_IdField clusterId, CsHeader::T_SeqField seq)
		{
			bool ret = false;
			DiffInfo &info = m_diffMap.at(clusterId);
			int32_t diff = seq - info.lastSeq;

			if (info.packDiff == info.l) // from last run
			{
				info.packDiff = 0;
			}

			if (diff < 0) // unnoticed new sequence or overflow?
			{
				if ((diff + info.l) > 0) // in this case we most likely missed a sequence
				{
					diff = seq;
					info.packDiff = 0;
					info.lastSeq = seq;
					ret = true;
				}
				else //overflow
					diff += std::numeric_limits<CsHeader::T_SeqField>::max();
			}

			info.packDiff += diff;
			info.seqDiff = diff;

			info.lastSeq = seq;
			if (info.packDiff == info.l)
			{
				info.seqStream->CreateBuffer(1, info.l); // add new buffer
				ret = true;
			}

			(*(info.seqStream->End() - 1))->WriteNext(seq); // write seq to last buffer in data stream

			return ret;
		}

		/**
		* \brief gets the number of zeros to pad
		*
		* If there are missing sequences we have to write zero rows to Z.
		* This function determines how many 0s have to be padded.
		*
		* \param clusterId id of the cluster
		*
		* \return number of zeros to pad
		*/
		uint32_t GetNZeros(CsHeader::T_IdField clusterId) const
		{
			DiffInfo diff = m_diffMap.at(clusterId);
			if (diff.seqDiff == 0)
				return 0;
			else
				return (diff.seqDiff - 1) * diff.m;
		}

	  private:
		struct DiffInfo
		{
			CsHeader::T_SeqField lastSeq;	  /**< last package sequence number*/
			uint32_t l;						   /**< NOF packets per measurement sequence*/
			uint32_t m;						   /**< NOF samples per packet*/
			uint32_t packDiff,				   /**< difference of packets to full measurement sequence*/
				seqDiff;					   /**< difference to last sequence*/
			Ptr<DataStream<double>> seqStream; /**< stream to write sequence numbers to */
		};

		std::map<CsHeader::T_IdField, DiffInfo> m_diffMap; /**< map with Diff Info ordered by cluster id*/
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
	void ReconstructNext();

	/**
	* \brief buffers packets data
	*
	* If diff > 1, diff*m zeros will append in front (missing rows of Z_k)
	*
	* \param p	Pointer to Packet
	* \param nZeros NOF zeros to pad
	*
	*/
	void BufferPacketData(Ptr<const Packet> p, uint32_t nZeros);

	/**
	* \brief prepares  for reconstructing  a new measurement sequence	
	*
	* This method resets all input buffers.
	*
	*/
	void StartNewSeq();

	Ptr<CsNode> m_node; /**< aggretated sink node*/

	Ptr<Reconstructor> m_reconst; /**<  reconstructor*/

	CsHeader::T_SeqField m_seqCount; /**< measurment sequence counter*/
	uint32_t m_recAttempt;			 /**< reconstruction attempt of current measurement sequence*/
	SeqChecker m_seqCheck;			 /**< checking for new Sequence*/

	bool m_isSetup;			/**< was setup called ?*/
	Time m_timeout;			/**< Packet inter-send time*/
	EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/

	TracedCallback<Ptr<const Packet>> m_rxTrace;				  /**< received a packet*/
	TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/

	uint32_t m_minPackets,											   /**< minmum NOF packets to start reconstructing*/
		m_rxPacketsSeq;												   /**< NOF received packets for this measurement sequence*/
	const static uint32_t N_SRCNODES_MAX = CsHeader::MAX_SRCNODES + 1; /**< maximum NOF source nodes, +1 since cluster is also source!*/
};

#endif //CS_SINK_APP_H
