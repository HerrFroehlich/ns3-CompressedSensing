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

const std::string CsSinkApp::SEQSTREAMNAME = "PacketSeq";

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
	m_isSetup = true;
	//StartNewSeq();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::AddCluster(Ptr<CsCluster> cluster)
{
	NS_LOG_FUNCTION(this << &cluster);

	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	NS_ASSERT_MSG(cluster->GetNSrc(), "Not enough source nodes in this cluster!");

	NS_ASSERT_MSG(m_reconst, "Non-valid reconstructor! Have you added it or called CreateObject?");

	m_reconst->AddCluster(cluster);

	Ptr<CsNode> clusterNode = cluster->GetClusterNode();

	//add sequence checker
	m_seqCheck.AddCluster(cluster);
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
	CsHeader::T_IdField nodeId,
		clusterId;

	p->PeekHeader(header);
	nodeId = header.GetNodeId();
	clusterId = header.GetClusterId();

	//check if we received from a cluster and if we know it
	if (nodeId != CsHeader::CLUSTER_NODEID) // we do not receive from a different cluster node
	{
		m_rxDropTrace(p, E_DropCause::NOT_A_CLUSTER);
		return false;
	}
	// else if (!m_recSpat->HasNode(clusterId))
	// {
	// 	m_rxDropTrace(p, E_DropCause::UNKNOWN_CLUSTER);
	// 	return false;
	// }

	CsHeader::T_SeqField seq = header.GetSeq();

	bool newSeq = m_seqCheck.AddNewSeq(clusterId, seq);

	/*TODO: actually a new Reconstruction should take place if we have a new sequence for all cluster nodes!
	 * -> new SeqChecker, buffer packets of new Seq
	 */

	if (newSeq)
		StartNewSeq();

	BufferPacketData(p, m_seqCheck.GetNZeros(clusterId));
	if (++m_rxPacketsSeq >= m_minPackets)
		ReconstructNext();
	return true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsSinkApp::BufferPacketData(Ptr<const Packet> packet, uint32_t nZeros)
{
	NS_LOG_FUNCTION(this << packet);

	CsHeader header;
	CsHeader::T_IdField id;

	Ptr<Packet> p = packet->Copy(); //COW

	p->RemoveHeader(header);

	id = header.GetClusterId();
	uint32_t size = header.GetDataSize() / sizeof(double);

	double *data = new double[size]();
	p->CopyData(reinterpret_cast<uint8_t *>(data), size * sizeof(double));

	if (nZeros > 0) // we have to fill with zeros
	{
		double *zeroFill = new double[nZeros](); //zero initialized
		m_reconst->WriteData(id, zeroFill, nZeros);
		delete[] zeroFill;
	}

	m_reconst->WriteData(id, data, size);
	delete[] data;

	//set precoding
	CsHeader::T_SrcInfo bitset = header.GetSrcInfo();

	std::vector<bool> precode;
	precode.reserve(CsHeader::SRCINFO_BITLEN);
	for (uint32_t i = 0; i < CsHeader::SRCINFO_BITLEN; i++)
	{
		precode.push_back(bitset[i]);
	}
	m_reconst->SetPrecodeEntries(id, precode);
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

void CsSinkApp::StartNewSeq()
{
	m_rxPacketsSeq = 0;
	m_recAttempt = 0;
	m_reconst->Reset(++m_seqCount);
}
