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
* Since the cluster head itself can be a source node this class inherits from CsSrcApp.
* BEFORE running the the application it has to be as well setup with a valid CsNode and an data input SerialDataBuffer instance.  
* The cluster head compresses the data form the SerialDataBuffer temporally during fixed measurment intervals.
* Instead of broadcasting the result the cluster head waits for incoming packets with temporally
* compressed data from other source nodes (\f$ Y_{jc} \in {\rm I\!R}^{m \times 1} \f$)in the cluster. 
* Packets with an outdated sequence number, incorrect size, or from source nodes of a different cluster are dropped.
* Upon receiving a new measurement sequence number or after
* a certain timeout intervall when no new packets are received from the source nodes, the cluster head compresses spatially:
* \f$Z_{c} = \Phi_c \hat{Y} = \Phi_c B_c Y = \Phi_c B_c
* \begin{bmatrix} Y_{1c}^T\\
*                 Y_{2c}^T\\
*                 \cdots\\
*                 Y_{jc}^T\\
* \end{bmatrix}, Z_c \in {\rm I\!R}^{l \times m}, \Phi_c \in {\rm I\!R}^{l \times N_c}, Y \in {\rm I\!R}^{N_c \times m} \f$ \n
* where:\n
* - \f$l\f$ is the NOF measurement vectors after compression
* - \f$m\f$ the NOF samples in a temporally compressed measurement vector
* - \f$N_c\f$ NOF source nodes in the cluster (including the cluster head)
* - \f$\Phi_c\f$ the spatial compression matrix of the cluster head
* - \f$ B_c\f$ is a diagonal matrix stating from which source nodes the cluster head has received valid data.
* - \f$ Y \f$ the  temporally compressed data from all source nodes in the cluster
* - \f$ \hat{Y} = B_c Y \f$ the actually received temporally compressed data from source nodes in the cluster
* To know which row of which \f$ Z_c\f$ is sended a corresponding row from a identity matrix is set as the 
* initial NC coefficients (see CsClusterHeader::SetNcInfoNew).
* If network coding (NC) is disabled the simply broadcasts packets containing the rows from \f$Z_c\f$, thus sending \f$l\f$ packets
* per measurement sequence. In contrary when NC is enabled, as it is per default, the cluster head recombines its own data
* with incoming data from other cluster heads periodically in NC intervalls (Be sure to set the NC interval time lower than
* the time of a measurement sequence, so that at least once NC is performed for a measurement sequence).
* In each NC interval \f$t\f$ during a measurment sequence the cluster head creates \f$P\f$ packets to be broadcasted:  
 \f$ h_{p,tx}^t = \sum_{\hat{p}=1}^{\hat{P}}\beta_(\hat{p})^t h_{\hat{p},rx}^{t-1} + \sum_{i=1}^l \alpha(i)^t Z_{c, i}^R  \f$\n
* where:\n
* - \f$ p \in (1\cdots P)\f$ denotes the coded packets to be send
* - \f$ \hat{p} \in (1\cdots \hat{P})\f$ denotes received coded packet (with the same measurement sequence)
* - \f$h_{p,tx}^t\f$ is the data vector of new packet \f$p\f$ to be send 
* - \f$h_{\hat{p},rx}^{t-1}\f$ is the data vector from a received packet \f$ \hat{p}\f$ from the previous interval
* - \f$\beta_(\hat{p})^t, \alpha(i)^t\f$ are NC coefficients, drawn with help of the CsClusterHeader::NcCoeffGenerator
* Finally the NC coded packets are broadcasted.
* Since the NC intervals would run endlessly, one can set the NOF intervals with no packets from other cluster heads,
* to stop the NC intervals after that.
*/
class CsClusterApp : public CsSrcApp
{
public:
  static const std::string NRX_SRC_STREAMNAME; /**< name of DataStream storing NOF packets fom sources received per sequence*/
  static const std::string NRX_CL_STREAMNAME; /**< name of DataStream storing NOF packets fom other clusters received per sequence*/
  
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
  bool m_spatComprEnable;                                             /**< Enable Spatial Compression?*/
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
  bool m_ncEnable,                           /**< Enable network coding?*/
      m_shuffle;                             /**< shuffle buffered packets?*/

  //Internal
  bool m_running,
      m_isSetup;
  Time m_timeout;         /**< time to wait for new source data of same sequence*/
  EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/
  uint32_t m_nPktRxSeq_src, /**< NOF packets received in a measurment sequence from sources*/
      m_nPktRxSeq_cl;      /**< NOF packets received in a measurment sequence from other clusters*/
  Ptr<DataStream<double>> m_rxCnt_src_stream, m_rxCnt_cl_stream;

  //Traces
  TracedCallback<Ptr<const Packet>> m_rxTrace;                  /**< received a packet*/
  TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/
  //TracedCallback<CsHeader::T_IdField> m_compressFailTrace;      /**< trace when compression failed*/

  const static uint32_t MAX_N_SRCNODES = CsHeader::MAX_SRCNODES + 1; /**< maximum NOF source nodes, +1 since cluster is also source!*/
};

#endif //CS_CLUSTERAPP_H