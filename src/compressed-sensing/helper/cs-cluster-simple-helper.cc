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

void CsClusterSimpleHelper::SetNodeSeeder(CsCluster::SeedCreator seeder)
{
	m_seeder = seeder;
}

CsCluster CsClusterSimpleHelper::Create(CsHeader::T_IdField id, uint32_t nSrc, DataStream<double> &stream)
{
	NS_ASSERT_MSG(nSrc <= CsHeader::MAX_SRCNODES, "Too many source nodes!");
	NS_ASSERT_MSG(nSrc < stream.GetN(), "Not enough stream buffers in this DataStream!");

	/*--------  Create Nodes  --------*/
	Ptr<CsNode> clusterNode = CreateObject<CsNode>(CsNode::NodeType::CLUSTER);
	CsNodeContainer srcNodes;
	srcNodes.Create(CsNode::NodeType::SOURCE, nSrc);
	CsCluster cluster(clusterNode, srcNodes);
	// srcNodes.CreateCluster(id, nSrc, m_seeder);

	Ptr<SerialDataBuffer<double>> bufCluster = stream.GetBuffer(CsHeader::CLUSTER_NODEID);//CLUSTER_NODEID=0

	for (uint32_t i = 0; i < nSrc; i++)
	{
		Ptr<CsNode> src = srcNodes.Get(i);

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
		clusterdevice->SetNode(clusterNode);
		clusterdevice->SetQueue(queue);

		src->AddTxDevice(srcdevice);
		clusterNode->AddRxDevice(clusterdevice);

		/*--------  Create Source Applications  --------*/
		Ptr<CsSrcApp> srcApp = m_srcAppFactory.Create<CsSrcApp>();
		Ptr<SerialDataBuffer<double>> buf = stream.GetBuffer(0); //always remove the first buffer
		srcApp->Setup(src, buf);
		src->AddApplication(srcApp);
	}

	/*--------  Create Cluster Application  --------*/
	Ptr<CsClusterApp> app = m_clusterAppFactory.Create<CsClusterApp>();
	app->Setup(clusterNode, bufCluster); 
	clusterNode->AddApplication(app);


	/*--------  Create CsCluster  --------*/
	UintegerValue n, m, l;
	app->GetAttribute("n", n);
	app->GetAttribute("m", m);
	app->GetAttribute("l", l);
	cluster.SetCompression(n.Get(), m.Get(), l.Get());
	return cluster;
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
	m_clusterAppFactory.Set("n", UintegerValue(n));
	m_clusterAppFactory.Set("l", UintegerValue(l));
	m_clusterAppFactory.Set("m", UintegerValue(m));
}

void CsClusterSimpleHelper::NormalizeToM()
{
	m_srcAppFactory.Set("Norm", BooleanValue(true));
	m_clusterAppFactory.Set("Norm", BooleanValue(true));
	m_clusterAppFactory.Set("NormSpat", BooleanValue(true));
}

// ApplicationContainer CsClusterSimpleHelper::GetFirstApp(CsNodeContainer nodes)
// {
// 	ApplicationContainer apps;

// 	for (auto it = nodes.Begin(); it != nodes.End(); it++)
// 	{
// 		NS_ASSERT_MSG((*it)->GetNApplications() > 0, "Node has no applications!");
// 		apps.Add((*it)->GetApplication(0));
// 	}
// 	return apps;
// }
