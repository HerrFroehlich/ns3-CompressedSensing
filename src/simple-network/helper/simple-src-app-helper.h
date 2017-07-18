/**
* \file simple-src-app-helper.h
*
* \author Tobias Waurick
* \date 23.05.17
*
*/
#ifndef SIMPLE_SRC_APP_HELPER_H
#define SIMPLE_SRC_APP_HELPER_H

#include "ns3/attribute.h"
#include "ns3/ptr.h"
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/simple-src-app.h"
using namespace ns3;

class SimpleSrcAppHelper
{
public:
  static TypeId GetTypeId(void);
  /**
* \ingroup simpleN
	* \brief create SimpleSrcAppHelper to install easily SimpleSrcApp t multiple nodes
	*
  */
  SimpleSrcAppHelper();
  virtual ~SimpleSrcAppHelper();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute(std::string name, const AttributeValue &value);

  /**
   * Create a SimpleSrcApp on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install(Ptr<Node> node) const;

  /**
   * Create a SimpleSrcApp on specified nodes in container
   *
   * \param nodeName The node container on which to create the application.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install(NodeContainer c) const;

  /**
   * Create a SimpleSrcApp on specified nodes in container and connects a packet Tx trace
   *
   * \param nodeName The node container on which to create the application.
   * \param cb       Packet::TracedCallback pointer to the tx callback function
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install(NodeContainer c, Packet::TracedCallback cb) const;

  /**
   * Create a SimpleSrcApp with relay functionality on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   * \param relayDevIdx index for the relay device to use 
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer InstallRelay(Ptr<Node> node, const std::vector<uint32_t> &relayDevIdx) const;

  /**
   * Create a SimpleSrcApp with relay functionality on specified nodes in container
   *
   * \param nodeName    The node container on which to create the application
   * \param relayDevIdx vector with indices for the relay devices to use  
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer InstallRelay(NodeContainer c, const std::vector<uint32_t> &relayDevIdx) const;

  /**
   * Create a SimpleSrcApp with relay functionality on specified nodes in container
   * Attaches a RX and TX callback function
   *
   * \param nodeName    The node container on which to create the application
   * \param relayDevIdx vector with indices for the relay devices to use 
   * \param txCb        Packet::TracedCallback pointer to the TX callback function
   * \param rxCb        Packet::TracedCallback pointer to the RX callback function
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer InstallRelay(NodeContainer c, const std::vector<uint32_t> &relayDevIdx, 
                                    Packet::TracedCallback txCb, Packet::TracedCallback rxCb) const;

  /**
  * \brief gets the current 8bit application node ID
  * 
  *
  * \return uint8_t nodeId
  */
  uint8_t GetNodeId(void) const;

  /**
  * \brief sets the current 8bit application node ID
  *
  * \param nodeId application node ID
  *
  */
  void SetNodeId(uint8_t nodeId);

private:
  /**
   * Install an SimpleSrcApp on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an SimpleSrcApp will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<SimpleSrcApp> InstallPriv(Ptr<Node> node) const;
  /**
   * Install an SimpleSrcApp on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an SimpleSrcApp will be installed.
   * \param relayDevIdx vector with indices for the relay devices to use 
   * \returns Ptr to the application installed.
   */
  Ptr<SimpleSrcApp> InstallPrivRelay(Ptr<Node> node,
                                       const std::vector<uint32_t> relayDevIdx) const;

  /**
  * \brief connects a trace source with a callback in  an application
  *
  * \param app        Ptr to a SimpleSrcApp
  * \param traceSrc   string describing the trace source
  * \param cbPtr      a Packet::TracedCallback pointer of function to connect
  *
  */
  void ConnectTraceSource(Ptr<Application> app, std::string traceSrc,
                           Packet::TracedCallback cbPtr) const;

  /**
  * \brief connects a trace source with a callback for all nodes in a container
  *
  * \param c          application container
  * \param traceSrc   string describing the trace source
  * \param cbPtr      a Packet::TracedCallback pointer of function to connect
  *
  */
  void ConnectTraceSource(ApplicationContainer c, std::string traceSrc, Packet::TracedCallback cbPtr) const;

  ObjectFactory m_factory;  //!< Object factory.
  uint32_t m_deviceIdx;     /**< Index of netdevice*/
  mutable uint8_t m_nodeId; /**< current node ID, inkrements with each installed node*/

private:
};

#endif //SIMPLE_SRC_APP_HELPER_H