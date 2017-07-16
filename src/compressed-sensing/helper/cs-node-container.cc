/**
* \file cs-node-container.cc
*
* \author Tobias Waurick
* \date 16.07.17
*
*/
#include "cs-node-container.h"


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
	return number + 1;
}