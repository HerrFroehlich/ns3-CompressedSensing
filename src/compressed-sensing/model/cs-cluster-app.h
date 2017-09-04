/**
* \file cs-cluster-app.h
*
* \author Tobias Waurick
* \date 14.07.17
*
*/

#ifndef CS_CLUSTERAPP_H
#define CS_CLUSTERAPP_H

#include "cs-src-app.h"
#include "ns3/mat-buffer.h"
#include "ns3/node-data-buffer-meta.h"
#include "cs-cluster-header.h"
#include "cs-cluster.h"

/**
* \ingroup csApps
* \class CsClusterApp
*
* \brief Application for a cluster node
*
* description:TODO
*
*/
class CsClusterApp : public CsSrcApp
{
public:
  enum E_DropCause
  {
    SIZE_MISMATCH,      /**< size of received data is not matching with expected size  */
    EXPIRED_SEQ,        /**< received old sequence number*/
    SRC_NOT_IN_CLUSTER, /**< received data from source node which is not in this cluster*/
    NC_MAXRECOMB        /**< packet reached max NOF recombinations/relays*/
  };
  typedef void (*RxDropCallback)(Ptr<const Packet>, E_DropCause); /**< callback signature:  dropping a received packet*/
  //typedef void (*CompressFailCallback)(CsHeader::T_IdField);      /**< callback signature:  compression failed*/

  static TypeId GetTypeId(void);

  /**
	* \brief create an CsClusterApp with default values
	*/
  CsClusterApp();

  /**
	* \brief setups the application
  *
	* MUST be called before starting the application
  * Uses the compressions dimensions spatially and temporally setup for the cluster head.
  *
	* \param cluster CsCluster to aggregate application to
	* \param input SerialDataBuffer<double> with input data for the node
	*
	*/
  virtual void Setup(const Ptr<CsCluster> cluster, Ptr<SerialDataBuffer<double>> input);

  /**
	* \brief sets the used spatial compressor
  *
	* \param comp  pointer to compressor
	*/
  void SetSpatialCompressor(Ptr<Compressor> comp);

  /**
	* \brief gets the used spatial compressor
  *
	* \return  pointer to compressor
	*/
  Ptr<Compressor> GetSpatialCompressor() const;

  //inherited from Application
  virtual void StartApplication();
  virtual void StopApplication();

protected:
  //inherited from CsSrcApp
  virtual void CreateCsPackets();

  virtual uint32_t GetMaxPayloadSizeByte() const;
  virtual uint32_t GetMaxPayloadSize() const;

private:
  /**
  * \brief compresses the next Z spatially 
  *
  * Adds the nodes own temporally compressed data if existent.
  * Sets the internal source info field depending on which source ndoes where used during compression,
  * which is needed for the CsClusterHeader header. 
  *
  * \return true when compression was successfull
  */
  bool CompressNextSpat();

  /**
  * \brief create new packets with a CsClusterHeader and payload to send to other cluster nodes/sink
  *
  * If NC is enabled the packcets are written to the NC packet buffered.
  * Else the packets are scheduled for broadcast.
  *
	*/
  void CreateCsClusterPackets();

  /**
  * \brief merge this clusters data with the data from others using RLNC
  *
  * Repeats the network coding task every dt, until cancelled (application stops).
  *
	* \param dt network coding interval 
  *
  */
  void RLNetworkCoding(Time dt);

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
  * \brief action when source data was received on a NetDevice
  *
  * \param p   received packet
  *
  */
  bool ReceiveSrc(const Ptr<const Packet> p);

  /**
  * \brief action when data from another  cluster was received on a NetDevice
  *
  * \param p   received packet
  *
  */
  bool ReceiveCluster(const Ptr<const Packet> p);

  /**
  * \brief start a new sequence
  *
  * \param seq new sequence 
  *
  */
  void StartNewSeq(CsHeader::T_SeqField seq);

  /**
  * \brief combines packets lineary with random coefficients (RLNC)
  *
  * Coefficients are drawn from a from a normal distribution and normated to sum up to 1
  *
  * \param pktList  vector containing packets for RLNC
  * \param seq      sequence number of new packet
  *
  * \return pointer to new packet
  */
  Ptr<Packet> DoRLNC(const std::vector<Ptr<Packet>> &pktList, CsClusterHeader::T_SeqField seq);

  //Spatial compression
  uint32_t m_l,                                                       /**< NOF of spatial and temporal compressed vectors*/
      m_nNodes,                                                       /**< NOF nodes in Cluster*/
      m_seed;                                                         /**< seed used for generating the temporal random sensing matrix*/
  CsHeader::T_SeqField m_nextPackSeq;                                 /**< sequence number of next packet*/
  Ptr<Compressor> m_comp;                                             /**<spatial compressor*/
  MatBuffer<T_PktData> m_zData;                                       /**< buffer containg spatially compressed data*/
  NodeDataBufferMeta<T_PktData, CsHeader::T_IdField> m_srcDataBuffer; /**< NodeDataBuffer with meta data for incoming source node data*/
  CsClusterHeader::T_SrcInfoField m_srcInfo;

  //Network coding
  //Ptr<RandomVariableStream> m_ranNc;      /**< random variable stream, to determine when to send*/
  CsClusterHeader::NcCoeffGenerator m_ncGen; /**< generator for network coding coefficients*/
  std::vector<Ptr<Packet>> m_ncPktBuffer;    /**< packet buffer for network coding*/
  uint32_t m_ncMaxRecomb,                    /**< maximum network coding recombinations*/
      m_ncPktPLink,                          /**< NOF packets per link at each interval*/
      m_ncTimeOut,                           /**< NOF of intervals with no packages to timeout*/
      m_ncTimeOutCnt;                        /**< counter of nc intervals with no packages*/
  Time m_ncInterval,                         /**< network coding interval*/
      m_ncIntervalDelay;                     /**< Initial delay of network coding interval*/
  EventId m_ncEvent;                         /**< event for doing network coding*/
  bool m_ncEnable;                           /**< Enable network coding?*/

  //Internal
  bool m_running,
      m_isSetup;
  Time m_timeout;         /**< time to wait for new source data of same sequence*/
  EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/
  EventId m_txEvent;      /**< transmission schedule event*/

  //Traces
  TracedCallback<Ptr<const Packet>> m_rxTrace;                  /**< received a packet*/
  TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/
  //TracedCallback<CsHeader::T_IdField> m_compressFailTrace;      /**< trace when compression failed*/

  const static uint32_t MAX_N_SRCNODES = CsHeader::MAX_SRCNODES + 1; /**< maximum NOF source nodes, +1 since cluster is also source!*/
};

#endif //CS_CLUSTERAPP_H