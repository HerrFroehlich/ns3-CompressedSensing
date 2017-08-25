#include "cs-cluster.h"
#include "assert.h"

TypeId CsCluster::GetTypeId()
{

	static TypeId tid = TypeId("CsCluster")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing");
	return tid;
}

CsCluster::CsCluster(Ptr<CsNode> cluster) : m_clusterNode(cluster), m_seed(1), m_n(0), m_m(0), m_l(0), m_isFrozen(false)
{
	uint32_t seed = DefaultSeedCreator(0, GetClusterId());

	m_clusterNode->SetSeed(seed);
	SetGroupName("Cluster" + std::to_string(GetClusterId()));

	m_allNodes.Add(m_clusterNode);
}

CsCluster::CsCluster(Ptr<CsNode> cluster, const CsNodeContainer &srcNodes) : m_clusterNode(cluster), m_srcNodes(srcNodes),
																			 m_seed(1), m_n(0), m_m(0), m_l(0), m_isFrozen(false)
{
	uint32_t nNodes = 0, seed = DefaultSeedCreator(nNodes, GetClusterId());

	m_clusterNode->SetSeed(seed);

	for (auto it = m_srcNodes.Begin(); it != m_srcNodes.End(); it++)
	{
		seed = DefaultSeedCreator(++nNodes, GetClusterId());
		(*it)->SetSeed(seed);

		(*it)->SetClusterId(GetClusterId());
		(*it)->SetNodeId(nNodes);
	}
	SetGroupName("Cluster" + std::to_string(GetClusterId()));

	m_allNodes.Add(m_clusterNode);
	m_allNodes.Add(m_srcNodes);
}

void CsCluster::SetClusterHead(Ptr<CsNode> node)
{
	NS_ASSERT_MSG(node, "Not a valid cluster node!"); //null pointer check
	NS_ASSERT_MSG(!m_isFrozen, "Cluster is frozen!");
	m_clusterNode = node;

	SetGroupName("Cluster" + std::to_string(node->GetClusterId()));

	m_allNodes = CsNodeContainer(m_clusterNode);
	m_allNodes.Add(m_srcNodes);
}

Ptr<CsNode> CsCluster::GetClusterHead() const
{
	return m_clusterNode;
}

void CsCluster::AddSrc(Ptr<CsNode> node, SeedCreator seeder)
{
	NS_ASSERT_MSG(m_srcNodes.GetN() + 1 <= CsHeader::MAX_SRCNODES, "Too many aggregated source nodes!");
	NS_ASSERT_MSG(!m_isFrozen, "Cluster is frozen!");

	uint32_t seed;
	if (seeder)
		seed = seeder(m_srcNodes.GetN() + 1, GetClusterId());
	else
		seed = DefaultSeedCreator(m_srcNodes.GetN() + 1, GetClusterId());

	node->SetSeed(seed);
	node->SetClusterId(GetClusterId());
	node->SetNodeId(m_srcNodes.GetN() + 1);

	m_srcNodes.Add(node);
	m_allNodes.Add(node);
}

void CsCluster::AddSrc(const CsNodeContainer &nodes, SeedCreator seeder)
{
	NS_ASSERT_MSG(!m_isFrozen, "Cluster is frozen!");
	uint32_t nNodesBefore = m_srcNodes.GetN();
	NS_ASSERT_MSG(nNodesBefore + nodes.GetN() <= CsHeader::MAX_SRCNODES, "Too many aggregated source nodes!");
	m_srcNodes.Add(nodes);
	m_allNodes.Add(nodes);

	// set cluster ID and seed
	uint32_t seed;
	uint32_t nNodes = nNodesBefore;
	for (auto it = m_srcNodes.Begin() + nNodesBefore; it != m_srcNodes.End(); it++)
	{
		if (seeder)
			seed = seeder(++nNodes, GetClusterId());
		else
			seed = DefaultSeedCreator(++nNodes, GetClusterId());
		(*it)->SetSeed(seed);

		(*it)->SetClusterId(GetClusterId());
		(*it)->SetNodeId(nNodes);
	}
}

Ptr<CsNode> CsCluster::GetSrc(uint32_t idx) const
{
	return m_srcNodes.Get(idx);
}

uint32_t CsCluster::GetNSrc() const
{
	return m_srcNodes.GetN();
}

uint32_t CsCluster::GetN() const
{
	return m_allNodes.GetN();
}

CsNodeContainer::Iterator CsCluster::SrcBegin() const
{
	return m_srcNodes.Begin();
}

CsNodeContainer::Iterator CsCluster::SrcEnd() const
{
	return m_srcNodes.End();
}

CsNodeContainer::Iterator CsCluster::Begin() const
{
	return m_allNodes.Begin();
}

CsNodeContainer::Iterator CsCluster::End() const
{
	return m_allNodes.End();
}

ApplicationContainer CsCluster::GetApps() const
{

	ApplicationContainer apps;

	for (auto it = m_allNodes.Begin(); it != m_allNodes.End(); it++)
	{

		uint32_t nApps = (*it)->GetNApplications();
		for (uint32_t i = 0; i < nApps; i++)
		{
			apps.Add((*it)->GetApplication(i));
		}
	}
	return apps;
}

uint32_t CsCluster::DefaultSeedCreator(uint32_t number, CsHeader::T_IdField id)
{
	(void)number;
	(void)id;
	return CsClusterHeader::GetMaxClusters() + 1;
}

void CsCluster::SetCompression(uint32_t n, uint32_t m, uint32_t l)
{
	NS_ASSERT_MSG(!m_isFrozen, "Cluster is frozen!");
	m_n = n;
	m_m = m;
	m_l = l;
}

std::vector<uint32_t> CsCluster::GetCompression() const
{
	std::vector<uint32_t> param(3);
	param.at(0) = m_n;
	param.at(1) = m_m;
	param.at(2) = m_l;
	return param;
}
uint32_t CsCluster::GetCompression(CsCluster::E_COMPR_DIMS dim) const
{
	uint32_t ret = 0;
	switch (dim)
	{
	case E_COMPR_DIMS::n:
		ret = m_n;
		break;
	case E_COMPR_DIMS::m:
		ret = m_m;
		break;
	case E_COMPR_DIMS::l:
		ret = m_l;
		break;
	default:
		break;
	}
	return ret;
}

void CsCluster::SetClusterSeed(uint32_t seed)
{
	m_seed = seed;
}

uint32_t CsCluster::GetClusterSeed() const
{
	return m_seed;
}

std::vector<uint32_t> CsCluster::GetSeeds() const
{
	std::vector<uint32_t> seeds;
	seeds.reserve(GetN());
	for (auto it = Begin(); it != End(); it++)
	{
		seeds.push_back((*it)->GetSeed());
	}
	return seeds;
}

void CsCluster::Freeze()
{
	m_isFrozen = true;
}

bool CsCluster::IsFrozen()
{
	return m_isFrozen;
}