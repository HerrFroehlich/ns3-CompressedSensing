/**
* \file topology-helper.cc
*
* \author Tobias Waurick
* \date 28.08.17
*
*/
#include "topology-simple-helper.h"
#include "ns3/queue.h"
#include "ns3/my-simple-net-device.h"
#include "ns3/my-simple-channel.h"
#include "ns3/error-model.h"

template <typename T>
TopologySimpleHelper::Links<T>::Links(uint32_t n) : m_len(n), m_links(zeros<Mat<T>>(n, n)), m_sinkLinks(n, 0)
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
TopologySimpleHelper::Links<T>::Links(const Mat<T> &clLinks, const std::vector<T> &sLinks) : m_len(clLinks.n_rows)
{
	SetAllLinks(clLinks, sLinks);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void TopologySimpleHelper::Links<T>::SetClLink(uint32_t i, uint32_t j, T val)
{
	NS_ASSERT_MSG(i < m_len && j < m_len, "Indices out of bounds!");
	m_links(i, j) = val;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void TopologySimpleHelper::Links<T>::SetClLink(uint32_t i, const std::vector<T> &values)
{
	NS_ASSERT_MSG(i < m_len, "Index out of bounds!");
	NS_ASSERT_MSG(values.size() == m_len, "Value vector has incorrect size!");

	for (uint32_t j = 0; j < m_len; j++)
	{
		SetClLink(i, j, values.at(j));
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void TopologySimpleHelper::Links<T>::SetSinkLink(uint32_t i, T val)
{
	NS_ASSERT_MSG(i < m_len, "Indices out of bounds!");
	m_sinkLinks.at(i) = val;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void TopologySimpleHelper::Links<T>::SetSinkLink(const std::vector<T> &links)
{
	NS_ASSERT_MSG(links.size() == m_len, "Link vector has incorrect size!");
	m_sinkLinks = links;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void TopologySimpleHelper::Links<T>::SetAllLinks(const Mat<T> &clLinks, const std::vector<T> &sLinks)
{
	NS_ASSERT_MSG(clLinks.n_rows == m_len && clLinks.n_cols == m_len, "Not a sqare matrix!");
	NS_ASSERT_MSG(sLinks.size() == m_len, "Not enough elements in vector!");
	m_links = clLinks;
	m_sinkLinks = sLinks;
	m_len = clLinks.n_rows;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
T TopologySimpleHelper::Links<T>::GetClLink(uint32_t i, uint32_t j) const
{
	return m_links(i, j);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
T TopologySimpleHelper::Links<T>::GetSinkLink(uint32_t i) const
{
	return m_sinkLinks.at(i);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

template class TopologySimpleHelper::Links<uint8_t>;
template class TopologySimpleHelper::Links<double>;

/*-----------------------------------------------------------------------------------------------------------------------*/

TopologySimpleHelper::TopologySimpleHelper()
{
	m_queueFactory.SetTypeId("ns3::DropTailQueue");
	m_deviceFactory.SetTypeId("MySimpleNetDevice");
	m_channelFactory.SetTypeId("MySimpleChannel");
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::Create(const std::vector<Ptr<CsCluster>> &clusters, Ptr<CsNode> sink, const LinksBool &clLinks) const
{
	NS_ASSERT_MSG(clusters.size() == clLinks.GetSize(), "Dimensions of link matrix/vector and NOF clusters not matching!");
	NS_ASSERT_MSG(sink->IsSink() && sink, "Invalid sink node!");

	uint32_t nCl = clLinks.GetSize();
	//connect clusters
	for (uint32_t i = 0; i < nCl; i++)
	{
		for (uint32_t j = 0; j < nCl; j++)
		{
			if (clLinks.GetClLink(i, j) && j != i) //if there is a link and we have differing clusters
			{
				Ptr<CsCluster> clusterA = clusters.at(i),
							   clusterB = clusters.at(j);
				NS_ASSERT_MSG(clusterA && clusterB, "Invalid cluster pointer!"); //nullpointer check
				Ptr<CsNode> nodeA = clusterA->GetClusterHead();
				Ptr<CsNode> nodeB = clusterB->GetClusterHead();

				DoConnect(nodeA, nodeB, 0.0);
			}
		}
	}

	//connect clusters to sink
	for (uint32_t i = 0; i < nCl; i++)
	{
		if (clLinks.GetSinkLink(i))
		{
			Ptr<CsNode> nodeA = clusters.at(i)->GetClusterHead();

			DoConnect(nodeA, sink, 0.0);
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::Create(const std::vector<Ptr<CsCluster>> &clusters, Ptr<CsNode> sink, const LinksDouble &clLinks) const
{
	NS_ASSERT_MSG(clusters.size() == clLinks.GetSize(), "Dimensions of link matrix/vector and NOF clusters not matching!");

	uint32_t nCl = clLinks.GetSize();
	//connect clusters
	for (uint32_t i = 0; i < nCl; i++)
	{
		for (uint32_t j = 0; j < nCl; j++)
		{
			if (j != i) //if there is a link and we have differing clusters
			{
				Ptr<CsCluster> clusterA = clusters.at(i),
							   clusterB = clusters.at(j);
				NS_ASSERT_MSG(clusterA && clusterB, "Invalid cluster pointer!"); //nullpointer check
				Ptr<CsNode> nodeA = clusterA->GetClusterHead();
				Ptr<CsNode> nodeB = clusterB->GetClusterHead();

				DoConnect(nodeA, nodeB, 1-clLinks.GetClLink(i, j));
			}
		}
	}

	//connect clusters to sink
	for (uint32_t i = 0; i < nCl; i++)
	{
		Ptr<CsNode> nodeA = clusters.at(i)->GetClusterHead();
		DoConnect(nodeA, sink, 1-clLinks.GetSinkLink(i));
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::SetQueue(std::string type,
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

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::SetDeviceAttribute(std::string n1, const AttributeValue &v1)
{
	m_deviceFactory.Set(n1, v1);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::SetChannelAttribute(std::string n1, const AttributeValue &v1)
{
	m_channelFactory.Set(n1, v1);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::SetQueueAttribute(std::string n1, const AttributeValue &v1)
{
	m_queueFactory.Set(n1, v1);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void TopologySimpleHelper::DoConnect(Ptr<CsNode> nodeA, Ptr<CsNode> nodeB, double errRate) const
{
	if (errRate < 1.0) // if error rate equals 1, we don't need a connection
	{
		Ptr<MySimpleChannel> channel = m_channelFactory.Create<MySimpleChannel>();
		Ptr<Queue> queue = m_queueFactory.Create<Queue>();

		Ptr<MySimpleNetDevice> deviceA = m_deviceFactory.Create<MySimpleNetDevice>(),
							   deviceB = m_deviceFactory.Create<MySimpleNetDevice>();
		if (errRate > 0.0) // if error rate equals 0, we don't need to attach an error model
		{
			Ptr<RateErrorModel> errModel = CreateObject<RateErrorModel>();
			errModel->SetRate(errRate);
			errModel->SetUnit(RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
			deviceA->SetAttribute("ReceiveErrorModel", PointerValue(errModel));
			deviceB->SetAttribute("ReceiveErrorModel", PointerValue(errModel));
		}

		deviceA->SetChannel(channel);
		deviceA->SetNode(nodeA);
		deviceA->SetQueue(queue);
		deviceB->SetChannel(channel);
		deviceB->SetNode(nodeB);
		deviceB->SetQueue(queue);

		nodeA->AddTxDevice(deviceA);
		nodeB->AddRxDevice(deviceB);
	}
}