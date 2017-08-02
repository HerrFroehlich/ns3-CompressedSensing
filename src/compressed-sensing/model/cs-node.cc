/**
* \file cs-node.cc
*
* \author Tobias Waurick
* \date 15.07.17
*
*/
#include "cs-node.h"
#include "ns3/log.h"
// #include "ns3/node-list.h"
NS_LOG_COMPONENT_DEFINE("CsNode");
NS_OBJECT_ENSURE_REGISTERED(CsNode);

TypeId
CsNode::GetTypeId(void)
{
	static TypeId tid = TypeId("CsNode")
							.SetParent<Node>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsNode>()
							.AddAttribute("Seed", "Individual seed for PRN generators for this node",
										  UintegerValue(1),
										  MakeUintegerAccessor(&CsNode::SetSeed, &CsNode::GetSeed),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("Type", "Type of this node",
										  TypeId::ATTR_GET,
										  EnumValue(NodeType::NONE),
										  MakeEnumAccessor(&CsNode::m_type),
										  MakeEnumChecker(NodeType::NONE, "None",
														  NodeType::SOURCE, "Source",
														  NodeType::CLUSTER, "Cluster",
														  NodeType::SINK, "Sink"));
	return tid;
}

CsNode::CsNode() : m_type(CsNode::NodeType::NONE), m_seed(1), m_clusterId(0), m_nodeId(0)
{
	NS_LOG_FUNCTION(this);
	SetGroupName("Node" + std::to_string(m_nodeId));
}

CsNode::CsNode(CsNode::NodeType type) : m_type(type), m_seed(1), m_clusterId(0), m_nodeId(0)
{
	NS_LOG_FUNCTION(this << type);
	if(type == NodeType::CLUSTER)
		m_nodeId = CsHeader::CLUSTER_NODEID;
	SetGroupName("Node" + std::to_string(m_nodeId));

}

CsNode::CsNode(CsNode::NodeType type, uint32_t systemId) : Node(systemId), m_type(type), m_seed(1), m_clusterId(0), m_nodeId(0)
{
	NS_LOG_FUNCTION(this << type << systemId);
	if(type == NodeType::CLUSTER)
		m_nodeId = CsHeader::CLUSTER_NODEID;
	SetGroupName("Node" + std::to_string(m_nodeId));
}
uint32_t
CsNode::AddDevice(Ptr<NetDevice> device)
{
	NS_LOG_FUNCTION(this << device);
	uint32_t id = Node::AddDevice(device);
	m_isTxDevice.push_back(id);
	m_isRxDevice.push_back(id);
	return id;
}

uint32_t
CsNode::AddTxDevice(Ptr<NetDevice> device)
{
	NS_LOG_FUNCTION(this << device);
	uint32_t id = Node::AddDevice(device);
	m_isTxDevice.push_back(id);
	return id;
}

uint32_t
CsNode::AddRxDevice(Ptr<NetDevice> device)
{
	NS_LOG_FUNCTION(this << device);
	uint32_t id = Node::AddDevice(device);
	m_isRxDevice.push_back(id);
	return id;
}

uint32_t
CsNode::GetNTxDevices() const
{
	NS_LOG_FUNCTION(this);
	return m_isTxDevice.size();
}

uint32_t
CsNode::GetNRxDevices() const
{
	NS_LOG_FUNCTION(this);
	return m_isRxDevice.size();
}
NetDeviceContainer
CsNode::GetTxDevices() const
{
	NS_LOG_FUNCTION(this);
	NetDeviceContainer devices;
	for (auto it = m_isTxDevice.begin(); it != m_isTxDevice.end(); it++)
	{
		devices.Add(GetDevice(*it));
	}
	return devices;
}

NetDeviceContainer
CsNode::GetRxDevices() const
{
	NS_LOG_FUNCTION(this);
	NetDeviceContainer devices;
	for (auto it = m_isRxDevice.begin(); it != m_isRxDevice.end(); it++)
	{
		devices.Add(GetDevice(*it));
	}
	return devices;
}

void CsNode::SetSeed(uint32_t seed)
{
	NS_LOG_FUNCTION(this << seed);
	m_seed = seed;
}

uint32_t
CsNode::GetSeed() const
{
	NS_LOG_FUNCTION(this);
	return m_seed;
}

CsNode::NodeType
CsNode::GetNodeType() const
{
	NS_LOG_FUNCTION(this);
	return m_type;
}

void CsNode::SetNodeId(CsHeader::T_IdField id)
{
	NS_ASSERT_MSG(!IsCluster() || id == CsHeader::CLUSTER_NODEID, "Cluster node must have fixed CLUSTER_NODEID!");
	m_nodeId = id;
	SetGroupName("Node" + std::to_string(id));
}

void CsNode::SetClusterId(CsHeader::T_IdField id)
{
	m_clusterId = id;
}

CsHeader::T_IdField CsNode::GetNodeId()
{
	return m_nodeId;
}

CsHeader::T_IdField CsNode::GetClusterId()
{
	return m_clusterId;
}

bool CsNode::IsSource()
{
	return (m_type == NodeType::SOURCE);
}

bool CsNode::IsCluster()
{
	return (m_type == NodeType::CLUSTER);
}

bool CsNode::IsSink()
{
	return (m_type == NodeType::SINK);
}
