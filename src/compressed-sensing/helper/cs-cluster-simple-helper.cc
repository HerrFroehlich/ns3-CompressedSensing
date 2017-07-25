#include "cs-cluster-simple-helper.h"
#include "assert.h"

CsClusterSimpleHelper::CsClusterSimpleHelper() : m_ranDelay(false), m_ranRate(false), m_seeder(0)
{
	m_queueFactory.SetTypeId("ns3::DropTailQueue");
	m_srcDeviceFactory.SetTypeId("MySimpleNetDevice");
	m_clusterDeviceFactory.SetTypeId("MySimpleNetDevice");
	m_channelFactory.SetTypeId("MySimpleChannel");
	m_srcAppFactory.SetTypeId("CsSrcApp");
	m_clusterAppFactory.SetTypeId("CsClusterApp");
}

void CsClusterSimpleHelper::SetQueue(std::string type,
									 std::string n1, const AttributeValue &v1,
									 std::string n2, const AttributeValue &v2,
									 std::string n3, const AttributeValue &v3,
									 std::string n4, const AttributeValue &v4)
{
	m_queueFactory.SetTypeId(type);
	m_queueFactory.Set(n1, v1);
	m_queueFactory.Set(n2, v2);
	m_queueFactory.Set(n3, v3);
	m_queueFactory.Set(n4, v4);
}

void CsClusterSimpleHelper::SetSrcDeviceAttribute(std::string n1, const AttributeValue &v1)
{
	m_srcDeviceFactory.Set(n1, v1);
}

void CsClusterSimpleHelper::SetClusterDeviceAttribute(std::string n1, const AttributeValue &v1)
{
	m_clusterDeviceFactory.Set(n1, v1);
}

void CsClusterSimpleHelper::SetSrcAppAttribute(std::string n1, const AttributeValue &v1)
{
	m_srcAppFactory.Set(n1, v1);
}

void CsClusterSimpleHelper::SetClusterAppAttribute(std::string n1, const AttributeValue &v1)
{
	m_clusterAppFactory.Set(n1, v1);
}

void CsClusterSimpleHelper::SetChannelAttribute(std::string n1, const AttributeValue &v1)
{
	m_channelFactory.Set(n1, v1);
}

void CsClusterSimpleHelper::SetNodeSeeder(CsNodeContainer::SeedCreator seeder)
{
	m_seeder = seeder;
}

CsNodeContainer
CsClusterSimpleHelper::Create(CsHeader::T_IdField id, uint32_t n, std::string fileBaseName)
{
	NS_ASSERT_MSG(n <= CsHeader::MAX_SRCNODES, "Too many source nodes!");

	/*--------  Create Nodes  --------*/
	CsNodeContainer nodes;
	nodes.CreateCluster(id, n, m_seeder);

	Ptr<CsNode> cluster = nodes.Get(0);
	for (uint32_t i = 1; i <= n; i++)
	{
		Ptr<CsNode> src = nodes.Get(i);

		/*--------  Create Channel  --------*/
		if (m_ranDelay)
		{
			double delay = m_gaussRan.GetValue(m_delayMean, m_delayVar);
			if (delay < 0)
				delay = 0;
			m_channelFactory.Set("Delay", TimeValue(Time(delay)));
		}
		Ptr<MySimpleChannel> channel = m_channelFactory.Create<MySimpleChannel>();

		Ptr<Queue> queue = m_queueFactory.Create<Queue>();

		/*--------  Create Devices  --------*/
		if (m_ranRate)
		{
			double rate = m_gaussRan.GetValue(m_rateMean, m_rateVar);
			if (rate < 0)
				rate = 0;
			m_srcDeviceFactory.Set("DataRate", DataRateValue(DataRate(uint64_t(rate))));
		}
		Ptr<MySimpleNetDevice> srcdevice = m_srcDeviceFactory.Create<MySimpleNetDevice>();
		srcdevice->SetChannel(channel);
		srcdevice->SetNode(src);
		srcdevice->SetQueue(queue);

		Ptr<MySimpleNetDevice> clusterdevice = m_clusterDeviceFactory.Create<MySimpleNetDevice>();
		clusterdevice->SetChannel(channel);
		clusterdevice->SetNode(cluster);
		clusterdevice->SetQueue(queue);

		src->AddTxDevice(srcdevice);
		cluster->AddRxDevice(clusterdevice);

		/*--------  Create Source Applications  --------*/
		Ptr<CsSrcApp> srcApp = m_srcAppFactory.Create<CsSrcApp>();
		std::string filename = fileBaseName + std::to_string(i);
		srcApp->Setup(src, filename);
		src->AddApplication(srcApp);
	}

	/*--------  Create Cluster Application  --------*/
	Ptr<CsClusterApp> app = m_clusterAppFactory.Create<CsClusterApp>();
	std::string filename = fileBaseName + std::to_string(CsHeader::CLUSTER_NODEID);
	app->Setup(cluster, filename);
	cluster->AddApplication(app);
	return nodes;
}

void CsClusterSimpleHelper::SetRandomDelay(Time mean, Time var)
{
	m_ranDelay = true;
	m_delayMean = mean.GetDouble();
	m_delayVar = var.GetDouble();
}

void CsClusterSimpleHelper::SetRandomDataRate(DataRate mean, DataRate var)
{
	m_ranRate = true;
	m_rateMean = mean.GetBitRate();
	m_rateVar = var.GetBitRate();
}

void CsClusterSimpleHelper::SetCompression(uint32_t n, uint32_t m, uint32_t l)
{
	m_srcAppFactory.Set("n", UintegerValue(n));
	m_srcAppFactory.Set("m", UintegerValue(m));
	m_clusterAppFactory.Set("l", UintegerValue(l));
	m_clusterAppFactory.Set("m", UintegerValue(m));
}

void CsClusterSimpleHelper::NormalizeToM()
{
	m_srcAppFactory.Set("Norm",BooleanValue(true));
	m_clusterAppFactory.Set("Norm",BooleanValue(true));
	m_clusterAppFactory.Set("NormSpat",BooleanValue(true));
}

ApplicationContainer CsClusterSimpleHelper::GetFirstApp(CsNodeContainer nodes)
{
	ApplicationContainer apps;

	for (auto it = nodes.Begin(); it != nodes.End(); it++)
	{
		NS_ASSERT_MSG((*it)->GetNApplications() > 0, "Node has no applications!");
		apps.Add((*it)->GetApplication(0));
	}
	return apps;
}
