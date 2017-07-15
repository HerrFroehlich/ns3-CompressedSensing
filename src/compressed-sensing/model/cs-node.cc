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

CsNode::CsNode() : m_type(CsNode::NodeType::NONE), m_seed(1)
{
	NS_LOG_FUNCTION(this);
}

CsNode::CsNode(CsNode::NodeType type) : m_type(type), m_seed(1)
{
	NS_LOG_FUNCTION(this << type);
}

CsNode::CsNode(CsNode::NodeType type, uint32_t systemId) :  Node(systemId), m_type(type), m_seed(1)
{
	NS_LOG_FUNCTION(this << type << systemId);
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

/*--------  CsNodeContainer  --------*/

CsNodeContainer::CsNodeContainer()
{
}

CsNodeContainer::CsNodeContainer(Ptr<CsNode> node)
{
	m_nodes.push_back(node);
}
// CsNodeContainer::CsNodeContainer (std::string nodeName)
// {
//   Ptr<CsNode> node = Names::Find<CsNode> (nodeName);
//   m_nodes.push_back (node);
// }
CsNodeContainer::CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b)
{
	Add(a);
	Add(b);
}
CsNodeContainer::CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b,
								 const CsNodeContainer &c)
{
	Add(a);
	Add(b);
	Add(c);
}
CsNodeContainer::CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b,
								 const CsNodeContainer &c, const CsNodeContainer &d)
{
	Add(a);
	Add(b);
	Add(c);
	Add(d);
}

CsNodeContainer::CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b,
								 const CsNodeContainer &c, const CsNodeContainer &d,
								 const CsNodeContainer &e)
{
	Add(a);
	Add(b);
	Add(c);
	Add(d);
	Add(e);
}

CsNodeContainer::Iterator
CsNodeContainer::Begin(void) const
{
	return m_nodes.begin();
}
CsNodeContainer::Iterator
CsNodeContainer::End(void) const
{
	return m_nodes.end();
}

uint32_t
CsNodeContainer::GetN(void) const
{
	return m_nodes.size();
}
Ptr<CsNode>
CsNodeContainer::Get(uint32_t i) const
{
	return m_nodes[i];
}
void CsNodeContainer::Create(CsNode::NodeType type, uint32_t n, SeedCreator seeder)
{
	for (uint32_t i = 0; i < n; i++)
	{
		Ptr<CsNode> node = CreateObject<CsNode>(type);

		uint32_t seed;
		if (seeder)
			seed = seeder(i);
		else
			seed = DefaultSeedCreator(i);
		node->SetSeed(seed);

		m_nodes.push_back(node);
	}
}
void CsNodeContainer::Create(CsNode::NodeType type, uint32_t n, uint32_t systemId, SeedCreator seeder)
{
	for (uint32_t i = 0; i < n; i++)
	{
		Ptr<CsNode> node = CreateObject<CsNode>(type, systemId);

		uint32_t seed;
		if (seeder)
			seed = seeder(i);
		else
			seed = DefaultSeedCreator(i);
		node->SetSeed(seed);

		m_nodes.push_back(node);
	}
}
void CsNodeContainer::Add(CsNodeContainer other)
{
	for (Iterator i = other.Begin(); i != other.End(); i++)
	{
		m_nodes.push_back(*i);
	}
}
void CsNodeContainer::Add(Ptr<CsNode> node)
{
	m_nodes.push_back(node);
}
void CsNodeContainer::Add(std::string nodeName)
{
	Ptr<CsNode> node = Names::Find<CsNode>(nodeName);
	m_nodes.push_back(node);
}

// CsNodeContainer
// CsNodeContainer::GetGlobal(void)
// {
// 	CsNodeContainer c;
// 	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
// 	{
// 		c.Add(*i);
// 	}
// 	return c;
// }

uint32_t CsNodeContainer::DefaultSeedCreator(uint32_t number)
{
	return number+1;
}