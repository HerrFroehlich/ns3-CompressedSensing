#include "cs-cluster.h"
#include "assert.h"

TypeId CsCluster::GetTypeId()
{

	static TypeId tid = TypeId("CsCluster")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing");
	return tid;
}

CsCluster::CsCluster(Ptr<CsNode> cluster) : m_clusterNode(cluster), m_n(0), m_m(0), m_l(0)
{
	uint32_t seed = DefaultSeedCreator(0, GetClusterId());

	m_clusterNode->SetSeed(seed);
}

CsCluster::CsCluster(Ptr<CsNode> cluster, const CsNodeContainer &srcNodes) : m_clusterNode(cluster), m_srcNodes(srcNodes),
																			 m_n(0), m_m(0), m_l(0)
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
}

void CsCluster::SetClusterNode(Ptr<CsNode> node)
{
	NS_ASSERT_MSG(node, "Not a valid cluster node!"); //null pointer check
	m_clusterNode = node;

	SetGroupName("Cluster" + std::to_string(node->GetClusterId()));
}

Ptr<CsNode> CsCluster::GetClusterNode() const
{
	return m_clusterNode;
}

void CsCluster::AddSrc(Ptr<CsNode> node, SeedCreator seeder)
{
	NS_ASSERT_MSG(m_srcNodes.GetN() + 1 <= CsHeader::MAX_SRCNODES, "Too many aggregated source nodes!");

	uint32_t seed;
	if (seeder)
		seed = seeder(m_srcNodes.GetN() + 1, GetClusterId());
	else
		seed = DefaultSeedCreator(m_srcNodes.GetN() + 1, GetClusterId());

	node->SetSeed(seed);
	node->SetClusterId(GetClusterId());
	node->SetNodeId(m_srcNodes.GetN() + 1);

	m_srcNodes.Add(node);
}

void CsCluster::AddSrc(const CsNodeContainer &nodes, SeedCreator seeder)
{
	uint32_t nNodesBefore = m_srcNodes.GetN();
	NS_ASSERT_MSG(nNodesBefore + nodes.GetN() <= CsHeader::MAX_SRCNODES, "Too many aggregated source nodes!");
	m_srcNodes.Add(nodes);

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

CsNodeContainer::Iterator CsCluster::SrcBegin() const
{
	return m_srcNodes.Begin();
}

CsNodeContainer::Iterator CsCluster::SrcEnd() const
{
	return m_srcNodes.End();
}

ApplicationContainer CsCluster::GetApps() const
{

	ApplicationContainer apps;
	uint32_t nApps;

	//get cluster apps
	nApps = m_clusterNode->GetNApplications();
	for (uint32_t i = 0; i < nApps; i++)
	{
		apps.Add(m_clusterNode->GetApplication(i));
	}

	for (auto it = m_srcNodes.Begin(); it != m_srcNodes.End(); it++)
	{
		nApps = (*it)->GetNApplications();
		for (uint32_t i = 0; i < nApps; i++)
		{
			apps.Add((*it)->GetApplication(i));
		}
	}
	return apps;
}

uint32_t CsCluster::DefaultSeedCreator(uint32_t number, CsHeader::T_IdField id)
{
	return number + 1 + id;
}

void CsCluster::SetCompression(uint32_t n, uint32_t m, uint32_t l)
{
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