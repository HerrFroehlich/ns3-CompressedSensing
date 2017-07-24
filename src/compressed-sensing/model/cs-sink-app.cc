/**
* \file cs-sink-app.cc
*
* \author Tobias Waurick
* \date 21.07.2017
*
*/

#include "cs-sink-app.h"
#include "omp-reconstructor.h"
#include "ns3/log.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("CsSinkApp");
NS_OBJECT_ENSURE_REGISTERED(CsSinkApp);

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
							.AddAttribute("RecSpat", "Spatial reconstructor",
										  PointerValue(CreateObject<OMP_Reconstructor<double>>()),
										  MakePointerAccessor(&CsSinkApp::m_recSpat),
										  MakePointerChecker<Reconstructor<double>>())
							.AddAttribute("RecTemp", "Temporal reconstructor",
										  PointerValue(CreateObject<OMP_ReconstructorTemp<double>>()),
										  MakePointerAccessor(&CsSinkApp::m_recTemp),
										  MakePointerChecker<Reconstructor<double>>())
							.AddAttribute("FileNameBase", "Base filename for output files",
										  StringValue("xRec"),
										  MakeStringAccessor(&CsSinkApp::m_filebase),
										  MakeStringChecker())
							.AddTraceSource("Rx", "A new packet is received",
											MakeTraceSourceAccessor(&CsSinkApp::m_rxTrace),
											"ns3::Packet::TracedCallback")
							.AddTraceSource("RxDrop",
											"Trace source indicating a packet has been dropped by the device during reception",
											MakeTraceSourceAccessor(&CsSinkApp::m_rxDropTrace),
											"CsSinkApp::RxDropCallback");
	return tid;
}

CsSinkApp::CsSinkApp() : m_isSetup(false),
						 m_timeoutEvent(EventId())
{
	NS_LOG_FUNCTION(this);
}

void CsSinkApp::Setup(Ptr<CsNode> node, std::string dir)
{
	NS_LOG_FUNCTION(this << node << dir);
	m_node = node;
	m_dir = dir;

	/*--------  Setup receiving netdevices  --------*/
	NetDeviceContainer devices = node->GetRxDevices();
	for (auto it = devices.Begin(); it != devices.End(); it++)
	{
		(*it)->SetReceiveCallback(MakeCallback(&CsSinkApp::Receive, this));
	}
	m_isSetup = true;
}

void CsSinkApp::AddCluster(const CsNodeContainer &cluster, uint32_t n, uint32_t m, uint32_t l)
{
	NS_LOG_FUNCTION(this << &cluster << n << m << l);

	NS_ASSERT_MSG(cluster.GetN(), "Not enough nodes in this cluster!");
	NS_ASSERT_MSG(m_recSpat && m_recTemp, "Non-valid reconstructors! Have you added them or called CreateObject?");

	Ptr<CsNode> clusterNode = cluster.Get(0);
	NS_ASSERT_MSG(clusterNode->IsCluster(), "Must be a cluster node!");

	CsHeader::T_IdField clusterId = clusterNode->GetClusterId();
	NS_ASSERT_MSG(!m_recSpat->HasNode(clusterId), "Cluster ID already added! ");

	uint32_t seed = clusterNode->GetSeed();
	m_recSpat->AddSrcNode(clusterId, seed, N_SRCNODES_MAX, l, m);

	// add unique temporal reconstructor
	Ptr<Reconstructor<double>> recTemp = m_recTemp->Clone();
	recTemp->SetBufferDim(n, m, 1);
	for (auto it = cluster.Begin() + 1; it != cluster.End(); it++)
	{
		uint32_t seed = (*it)->GetSeed();
		CsHeader::T_IdField id = (*it)->GetNodeId();
		recTemp->AddSrcNode(id, seed);
	}
	m_recTempMap[clusterId] = recTemp;

	//add sequence checker
	// SeqChecker check(l);
	m_seqCheckMap.emplace (clusterId, SeqChecker(l));
}

bool CsSinkApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p);
	NS_ASSERT(m_isSetup);
	(void)idUnused;
	(void)adrUnused;

	//call receive trace
	m_rxTrace(p);

	//cancel old time out event and start a new
	Simulator::Cancel(m_timeoutEvent);
	m_timeoutEvent = Simulator::Schedule(m_timeout, &CsSinkApp::ReconstructNext, this);

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
	else if (!m_recSpat->HasNode(clusterId))
	{
		m_rxDropTrace(p, E_DropCause::UNKNOWN_CLUSTER);
		return false;
	}

	CsHeader::T_SeqField seq = header.GetSeq();

	SeqChecker &check = m_seqCheckMap.at(clusterId);
	bool newSeq = check.AddNewSeq(seq);
	(void)newSeq;
	// TODO: actually a new Reconstruction should take place if we have a new sequence for all cluster nodes
		if (newSeq)
		{
	ReconstructNext();
		}

	BufferPacket(p, check.GetSeqDiff());

	return true;
}

void CsSinkApp::BufferPacket(Ptr<const Packet> packet, uint32_t diff)
{
	NS_LOG_FUNCTION(this << packet);

	CsHeader header;
	CsHeader::T_IdField id;

	Ptr<Packet> p = packet->Copy(); //COW

	p->RemoveHeader(header);

	id = header.GetClusterId();
	uint32_t size = header.GetDataSize() / sizeof(double);

	double *data = new double[size];
	p->CopyData(reinterpret_cast<uint8_t *>(data), size * sizeof(double));

	if (diff > 1) // we have to fill with zeros if we have a difference greater one
	{
		std::vector<uint32_t> dim = m_recSpat->GetNodeDim(id);
		uint32_t m = dim.at(2);

		double *zeroFill = new double[m * diff](); //zero initialized
		m_recSpat->WriteData(id, zeroFill, m * diff);
		delete[] zeroFill;
	}

	m_recSpat->WriteData(id, data, size);
	delete[] data;
}

void CsSinkApp::ReconstructNext()
{
	NS_LOG_FUNCTION(this);

	//spatial 
	m_recSpat->ReconstructAll();
	//temporal
	for (auto it = m_recSpat->IdBegin(); it != m_recSpat->IdEnd(); it++)// iterate over cluster ids
	{
		CsHeader::T_IdField id = *it;
		Ptr<Reconstructor<double>> recTemp = m_recTempMap.at(id);
		recTemp->ReconstructAll();
	}
}

void CsSinkApp::WriteOutputFiles()
{
}
