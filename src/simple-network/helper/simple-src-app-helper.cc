/**
* \file simple-src-app-helper.cc
*
* \author Tobias Waurick
* \date 23.05.17
*
*/

#include "simple-src-app-helper.h"

SimpleSrcAppHelper::SimpleSrcAppHelper() : m_nodeId(0)
{
	m_factory.SetTypeId(SimpleSrcApp::GetTypeId());
}

SimpleSrcAppHelper::~SimpleSrcAppHelper()
{
}

void SimpleSrcAppHelper::SetAttribute(
	std::string name,
	const AttributeValue &value)
{
	m_factory.Set(name, value);
}

ApplicationContainer
SimpleSrcAppHelper::Install(Ptr<Node> node) const
{
	return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
SimpleSrcAppHelper::Install(NodeContainer c) const
{
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
	{
		apps.Add(InstallPriv(*i));
	}

	return apps;
}

ApplicationContainer
SimpleSrcAppHelper::Install(NodeContainer c, Packet::TracedCallback cb) const
{
	ApplicationContainer apps;
	apps = Install(c);
	ConnectTraceSource(apps, "Tx", cb);

	return apps;
}

ApplicationContainer
SimpleSrcAppHelper::InstallRelay(Ptr<Node> node, const std::vector<uint32_t> &relayDevIdx) const
{
	return ApplicationContainer(InstallPrivRelay(node, relayDevIdx));
}

ApplicationContainer
SimpleSrcAppHelper::InstallRelay(NodeContainer c, const std::vector<uint32_t> &relayDevIdx) const
{
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
	{
		apps.Add(InstallPrivRelay(*i, relayDevIdx));
	}

	return apps;
}

ApplicationContainer
SimpleSrcAppHelper::InstallRelay(NodeContainer c, const std::vector<uint32_t> &relayDevIdx,
								 Packet::TracedCallback txCb, Packet::TracedCallback rxCb) const
{
	ApplicationContainer apps;
	apps = InstallRelay(c, relayDevIdx);
	ConnectTraceSource(apps, "Tx", txCb);
	ConnectTraceSource(apps, "Rx", rxCb);

	return apps;
}

uint8_t
SimpleSrcAppHelper::GetNodeId() const
{
	return m_nodeId;
}

void SimpleSrcAppHelper::SetNodeId(uint8_t nodeId)
{
	m_nodeId = nodeId;
}

Ptr<SimpleSrcApp>
SimpleSrcAppHelper::InstallPriv(Ptr<Node> node) const
{
	Ptr<SimpleSrcApp> app = m_factory.Create<SimpleSrcApp>();
	app->Setup(m_nodeId, node);
	node->AddApplication(app);
	m_nodeId++;
	return app;
}

Ptr<SimpleSrcApp>
SimpleSrcAppHelper::InstallPrivRelay(Ptr<Node> node, const std::vector<uint32_t> relayDevIdx) const
{
	NS_ASSERT_MSG(relayDevIdx.size() > 0, "No device indices specified");
	Ptr<SimpleSrcApp> app = m_factory.Create<SimpleSrcApp>();

	app = InstallPriv(node);
	for (uint32_t i = 0; i < relayDevIdx.size(); i++)
	{
		app->SetupRelay(relayDevIdx[i]);
	}
	return app;
}

void SimpleSrcAppHelper::ConnectTraceSource(Ptr<Application> app, std::string traceSrc,
											Packet::TracedCallback cbPtr) const
{
	app->TraceConnectWithoutContext(traceSrc, MakeCallback(cbPtr));
}

void SimpleSrcAppHelper::ConnectTraceSource(ApplicationContainer c, std::string traceSrc,
											Packet::TracedCallback cbPtr) const
{
	for (ApplicationContainer::Iterator i = c.Begin(); i != c.End(); ++i)
	{
		ConnectTraceSource(*i, traceSrc, cbPtr);
	}
}