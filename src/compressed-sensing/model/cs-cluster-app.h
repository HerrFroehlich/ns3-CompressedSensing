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
#include "node-data-buffer.h"

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
  static TypeId GetTypeId(void);

  /**
	* \brief create an CsClusterApp with default values
	*/
  CsClusterApp();

  /**
	* \brief create an CsClusterApp
	*
	* \param n length of original temporal measurement vector (source)
	* \param m1 length of compressed temporal vector (source)
  * \param m2 NOF of spatial and temporal compressed vectors
	*/
  CsSrcApp(uint32_t n, uint32_t m1, uint32_t m2);

  /**
	* \brief setups the application
	* MUST be called before starting the application
	*
	* \param clusterId ID for this cluster node
	* \param rxDevIdx vector containing indices of devices wich only receive
	*
	* \return returnDesc
	*/
  Setup(Ptr<CsNode> node, T_IdField clusterId, uint32_t nSrcNodes, std::string filename);

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
  * The received data will be stored 
  *
  * \param dev pointer to the net device
  * \param p   received packet
  * \param idUnused   protocol id UNUSED
  * \param adrUnused  sender address UNUSED
  *
  */
  bool Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused);

  uint32_t m_m1,                  /**< length of compressed temporal vector (source)*/
      m_m2;                       /**< NOF of spatial and temporal compressed vectors*/
  Ptr<Compressor<double>> m_comp; /**< compressor*/

  SerialDataBuffer<double> outBuf;                            /**< buffer containing output data*/
  std::map<T_NodeIdTag, NodeDataBuffer<double>> m_srcDataMap, /**< NodeDataBuffer for incoming source node data*/
      m_clusterDataMap;                                       /**< NodeDataBuffer for  incoming cluster  node data*/
  bool m_isSetup;
};

#endif //CS_CLUSTERAPP_H