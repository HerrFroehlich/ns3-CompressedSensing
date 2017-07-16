/**
* \file cs-node.h
*
* \author Tobias Waurick
* \date 14.07.17
*
*/

#ifndef CS_NODE_H
#define CS_NODE_H
#include <vector>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/net-device-container.h"
#include "cs-header.h"
using namespace ns3;
/**
* \ingroup util
* \class CsNode
*
* \brief extended ns3::Node with various new properties
*
* description:TODO
*
*/
class CsNode : public Node
{
  public:
	static TypeId GetTypeId(void);

	enum NodeType /**< type of node*/
	{
		NONE,
		SOURCE,
		CLUSTER,
		SINK
	};

	/**
	* \brief Create new node with no specific type
	* 
	*/
	CsNode();

	/**
	* \brief Create new node with given type
	* 
	* \param type CsNode::NodeType of this node
	*/
	CsNode(CsNode::NodeType type);

	/**
	* \brief Create new node with given type and specific system id
	* 
	* \param type CsNode::NodeType of this node
	* \param systemId	a unique integer used for parallel simulations. 
	*/
	CsNode(CsNode::NodeType type, uint32_t systemId);

	/**
	* \brief adds a NetDevice for receiving and transmitting
	* shadows Node::AddDevice
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	virtual uint32_t AddDevice(Ptr<NetDevice> device);

	/**
	* \brief adds a NetDevice exclusively used for transmitting
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	uint32_t AddTxDevice(Ptr<NetDevice> device);

	/**
	* \brief adds a NetDevice exclusively used for receiving
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	uint32_t AddRxDevice(Ptr<NetDevice> device);

	/**
	* \brief get all associated tx devices
	*
	*
	* \return NetDeviceContainer with associated tx devices
	*/
	NetDeviceContainer GetTxDevices() const;

	/**
	* \brief get all associated rx devices
	*
	* \return NetDeviceContainer with associated rx devices
	*/
	NetDeviceContainer GetRxDevices() const;

	/**
	* \brief get NOF associated tx devices
	*
	* \return NOF devices
	*/
	uint32_t GetNTxDevices() const;

	/**
	* \brief get NOF associated rx devices
	*
	* \return NOF devices
	*/
	uint32_t GetNRxDevices() const;
	/**
	* \brief Sets the node's seed
	*
	* \param seed seed to use
	*/
	void SetSeed(uint32_t seed);
	/**
	* \brief Gets the node's seed
	*
	* \return used seed
	*/
	uint32_t GetSeed() const;

	CsNode::NodeType GetNodeType() const;

  /**
  * \brief sets the nodeId
  *
  * \param id ID to use
  *
  */
  void SetNodeId(CsHeader::T_IdField id);

  /**
  * \brief sets the clusterId
  *
  * \param id ID to use
  *
  */
  void SetClusterId(CsHeader::T_IdField id);

  /**
  * \brief gets the current node ID
  *
  * \return ID used
  *
  */
  CsHeader::T_IdField GetNodeId();

  /**
  * \brief gets the current cluster ID
  *
  * \return ID used
  *
  */
  CsHeader::T_IdField GetClusterId();

private:
	NodeType m_type;
  uint32_t m_seed;
  CsHeader::T_IdField m_clusterId, m_nodeId;
  std::vector<uint32_t> m_isTxDevice, m_isRxDevice; /**< tx/rx device indices*/
};

#endif //CS_NODE_H