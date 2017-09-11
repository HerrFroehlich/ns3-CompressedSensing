/**
* \file cs-sink-app.cc
*
* \author Tobias Waurick
* \date 21.07.2017
*
*/

#include "cs-sink-app.h"
#include "ns3/log.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("CsSinkApp");
NS_OBJECT_ENSURE_REGISTERED(CsSinkApp);

const std::string CsSinkApp::NRX_STREAMNAME = "nPktRx";

/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId CsSinkApp::GetTypeId(void)
{
	static TypeId tid = TypeId("CsSinkApp")
							.SetParent<Application>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsSinkApp>()
							.AddAttribute("TimeOut",
										  "The time to wait for new data",
										  TimeValue(Seconds(10)),
										  MakeTimeAccessor(&CsSinkApp::m_timeout),
										  MakeTimeChecker(Seconds(0)))
							.AddAttribute("Reconst", "Reconstructor",
										  PointerValue(CreateObject<Reconstructor>()),
										  MakePointerAccessor(&CsSinkApp::m_reconst),
										  MakePointerChecker<Reconstructor>())
							.AddAttribute("MinPackets", "Minmum NOF received Packets to start reconstructing",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsSinkApp::m_minPackets),
										  MakeUintegerChecker<uint32_t>())
							.AddTraceSource("Rx", "A new packet is received",
											MakeTraceSourceAccessor(&CsSinkApp::m_rxTrace),
											"ns3::Packet::TracedCallback")
							.AddTraceSource("RxDrop",
											"Trace source indicating a packet has been dropped by the device during reception",
											MakeTraceSourceAccessor(&CsSinkApp::m_rxDropTrace),
											"CsSinkApp::RxDropCallback");
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsSinkApp::CsSinkApp() : m_seqCount(0),
						 m_recAttempt(0),
						 m_isSetup(false),
						 m_timeoutEvent(EventId()),
						 m_minPackets(0),
						 m_rxPacketsSeq(0)

{
	NS_LOG_FUNCTION(this);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::Setup(Ptr<CsNode> node)
{
	NS_LOG_FUNCTION(this << node);
	m_node = node;

	/*--------  Setup receiving netdevices  --------*/
	NetDeviceContainer devices = node->GetRxDevices();
	for (auto it = devices.Begin(); it != devices.End(); it++)
	{
		(*it)->SetReceiveCallback(MakeCallback(&CsSinkApp::Receive, this));
	}
	m_rxCnt_stream = Create<DataStream<double>>(NRX_STREAMNAME);
	m_node->AddStream(m_rxCnt_stream);

	m_isSetup = true;
	//StartNewSeq();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::AddCluster(Ptr<CsCluster> cluster)
{
	NS_LOG_FUNCTION(this << &cluster);

	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	NS_ASSERT_MSG(m_reconst, "Non-valid reconstructor! Have you added it or called CreateObject?");
	NS_ABORT_MSG_IF(!cluster->GetNSrc(), "Not enough source nodes in this cluster!");

	m_reconst->AddCluster(cluster);

	Ptr<CsNode> clusterNode = cluster->GetClusterHead();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsSinkApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p);
	NS_ASSERT(m_isSetup);
	(void)idUnused;
	(void)adrUnused;

	//call receive trace
	m_rxTrace(p);

	//cancel old time out event and start a new
	// Simulator::Cancel(m_timeoutEvent);
	// m_timeoutEvent = Simulator::Schedule(m_timeout, &CsSinkApp::ReconstructNext, this);

	CsHeader header;
	CsHeader::T_IdField nodeId;
	CsHeader::T_SeqField seq;

	p->PeekHeader(header);
	nodeId = header.GetNodeId();

	//check if we received from a cluster and if we know it
	if (nodeId != CsClusterHeader::CLUSTER_NODEID) // we do not receive from a different cluster node
	{
		m_rxDropTrace(p, E_DropCause::NOT_A_CLUSTER);
		return false;
	}
	// else if (!m_recSpat->HasNode(clusterId))
	// {
	// 	m_rxDropTrace(p, E_DropCause::UNKNOWN_CLUSTER);
	// 	return false;
	// }

	//check sequence
	seq = header.GetSeq();
	if (seq > m_seqCount)
		StartNewSeq(seq - m_seqCount);
	else if (seq < m_seqCount)
	{
		m_rxDropTrace(p, E_DropCause::EXPIRED_SEQ);
		return false;
	}

	BufferPacketData(p);
	if (++m_rxPacketsSeq >= m_minPackets)
		ReconstructNext();
	return true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::BufferPacketData(Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION(this << packet);

	CsClusterHeader header;

	Ptr<Packet> p = packet->Copy(); //COW

	p->RemoveHeader(header);

	uint32_t size = header.GetDataSize() / sizeof(double);

	//write data
	double *data = new double[size]();
	p->CopyData(reinterpret_cast<uint8_t *>(data), size * sizeof(double));
	m_reconst->WriteData(data, size, header.GetNcInfo());
	delete[] data;

	//set precoding
	// CsClusterHeader::T_SrcInfoField bitset = header.GetSrcInfo();

	// std::vector<bool> precode;
	// precode.reserve(CsClusterHeader::SRCINFO_BITLEN);
	// for (uint32_t i = 0; i < CsClusterHeader::SRCINFO_BITLEN; i++)
	// {
	// 	precode.push_back(bitset[i]);
	// }
	// m_reconst->SetPrecodeEntries(header.GetClusterId(), precode);

	for (CsHeader::T_IdField id = 0; id < CsClusterHeader::GetMaxClusters(); id++)
	{
		if (header.IsSrcInfoSet(id))
		{
			CsClusterHeader::T_SrcInfoField bitset = header.GetSrcInfo(id);

			std::vector<bool> precode;
			precode.reserve(CsClusterHeader::SRCINFO_BITLEN);
			for (uint32_t i = 0; i < CsClusterHeader::SRCINFO_BITLEN; i++)
			{
				precode.push_back(bitset[i]);
			}
			m_reconst->SetPrecodeEntries(id, precode);
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::ReconstructNext()
{
	NS_LOG_FUNCTION(this);
	NS_LOG_INFO("Reconstructing measurement seqence " << m_seqCount << ", " << m_recAttempt + 1 << ". attempt");

	m_reconst->ReconstructAll();

	m_recAttempt++;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::StartNewSeq(uint32_t seqDiff)
{
	vector<double> rxCnt(1, double(m_rxPacketsSeq));
	m_rxCnt_stream->CreateBuffer(rxCnt);

	m_rxPacketsSeq = 0;
	m_recAttempt = 0;
	m_seqCount += seqDiff;
	m_reconst->Reset(m_seqCount);
}
