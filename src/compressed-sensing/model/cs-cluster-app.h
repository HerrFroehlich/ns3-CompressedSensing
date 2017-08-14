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
    SIZE_MISMATCH,     /**< size of received data is not matching with expected size  */
    EXPIRED_SEQ,       /**< received old sequence number*/
    SRC_NOT_IN_CLUSTER /**< received data from source node which is not in this cluster*/
  };
  typedef void (*RxDropCallback)(Ptr<const Packet>, E_DropCause); /**< callback signature:  dropping a received packet*/
  typedef void (*CompressFailCallback)(CsHeader::T_IdField);      /**< callback signature:  compression failed*/

  static TypeId GetTypeId(void);

  /**
	* \brief create an CsClusterApp with default values
	*/
  CsClusterApp();

  /**
	* \brief create an CsClusterApp
	*
	* \param n length of original temporal measurement vector (source)
	* \param m length of compressed temporal vector (source)
  * \param m2 NOF of spatial and temporal compressed vectors
	*/
  CsClusterApp(uint32_t n, uint32_t m, uint32_t m2);

  /**
	* \brief setups the application
  *
	* MUST be called before starting the application
	*
	* \param node Csnode to aggregate application to
	* \param input SerialDataBuffer<double> with input data for the node
	*
	*/
  virtual void Setup(Ptr<CsNode> node, Ptr<SerialDataBuffer<double>> input);

  /**
	* \brief sets the used spatial compressor
  *
	* \param comp  pointer to compressor
	*/
  void SetSpatialCompressor(Ptr<Compressor<double>> comp);

  /**
	* \brief gets the used spatial compressor
  *
	* \return  pointer to compressor
	*/
  Ptr<Compressor<double>> GetSpatialCompressor() const;

  /**
	* \brief sets the used spatial compressor
  *
	*  The NOF measurements used for compression may differ from sequence to sequence, as the source nodes transmit randomly. 
  *  This means the compressor's "n"  does not need to be given as parameter.
  *  Therefore the compressor will be setup later during compression.
	*
	* \param comp  pointer to compressor
	* \param m2 NOF compressed vectors
	* \param norm normalize random matrix by 1/sqrt(m)?
	*/
  // void SetSpatialCompressor(Ptr<Compressor<double>> comp, uint32_t l, bool norm = false);

  /**
	* \brief sets the compression given by l
  *
	*  The NOF measurements used for compression may differ from sequence to sequence, as the source nodes transmit randomly. 
  *
  * \param nNodes NOF nodes in cluster
	* \param l NOF compressed vectors
	*/
  void SetSpatialCompressDim(uint32_t nNodes, uint32_t l);

  //inherited from Application
  virtual void StartApplication();
  virtual void StopApplication();

protected:
  //inherited from CsSrcApp
  virtual bool CompressNext();
  virtual void CreateCsPackets();
  virtual uint32_t GetMaxPayloadSize();

private:
  /**
  * \brief merge this clusters data with the data from others using RLNC
  *
  */
  void DoNetworkCoding();

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

  uint32_t m_l,     /**< NOF of spatial and temporal compressed vectors*/
      m_nNodes,     /**< NOF nodes in Cluster*/
      m_outBufSize; /**< size of output buffer*/

  CsHeader::T_SeqField m_nextPackSeq; /**< sequence number of next packet*/
  Ptr<Compressor<double>> m_comp;     /**< compressor*/

  SerialDataBuffer<double> m_outBuf;                               /**< buffer containing output data*/
  MatBuffer<double> m_zData;                                       /**< buffer containg spatially compressed data*/
  NodeDataBufferMeta<double, CsHeader::T_IdField> m_srcDataBuffer; /**< NodeDataBuffer with meta data for incoming source node data*/
  //m_clusterDataMap;                                                 /**< NodeDataBuffer for  incoming cluster  node data*/
  std::bitset<CsHeader::SRCINFO_BITLEN> m_srcInfo;

  bool m_running,
      m_isSetup;

  Time m_timeout;         /**< Packet inter-send time*/
  EventId m_timeoutEvent; /**< timeout event when waiting for new source data*/

  TracedCallback<Ptr<const Packet>> m_rxTrace;                  /**< received a packet*/
  TracedCallback<Ptr<const Packet>, E_DropCause> m_rxDropTrace; /**< callback:  dropping a received packet*/
  TracedCallback<CsHeader::T_IdField> m_compressFailTrace;      /**< trace when compression failed*/

  const static uint32_t MAX_N_SRCNODES = CsHeader::MAX_SRCNODES + 1; /**< maximum NOF source nodes, +1 since cluster is also source!*/
};

#endif //CS_CLUSTERAPP_H