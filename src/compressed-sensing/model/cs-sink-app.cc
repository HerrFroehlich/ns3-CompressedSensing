/**
* \file cs-sink-app.cc
*
* \author Tobias Waurick
* \date 21.07.2017
*
*/

#include "cs-sink-app.h"
#include "omp-reconstructor.h"
#include "bp-reconstructor.h"
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
							// .AddAttribute("FileNameBase", "Base filename for output files",
							// 			  StringValue("xRec"),
							// 			  MakeStringAccessor(&CsSinkApp::m_filebase),
							// 			  MakeStringChecker())
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

CsSinkApp::CsSinkApp() : m_seqCount(0),
						 m_recAttempt(0),
						 m_isSetup(false),
						 m_timeoutEvent(EventId()),
						 m_minPackets(0),
						 m_rxPackets(0)

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
	StartNewSeq();
}

void CsSinkApp::AddCluster(Ptr<CsCluster> cluster)
{
	NS_LOG_FUNCTION(this << &cluster);

	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	NS_ASSERT_MSG(cluster->GetNSrc(), "Not enough source nodes in this cluster!");
	NS_ASSERT_MSG(m_recSpat && m_recTemp, "Non-valid reconstructors! Have you added them or called CreateObject?");

	std::vector<uint32_t> param = cluster->GetCompression();
	uint32_t n = param.at(0),
			 m = param.at(1),
			 l = param.at(2);

	Ptr<CsNode> clusterNode = cluster->GetClusterNode();

	CsHeader::T_IdField clusterId = clusterNode->GetClusterId();
	NS_ASSERT_MSG(!m_recSpat->HasNode(clusterId), "Cluster ID already added! ");

	uint32_t seed = clusterNode->GetSeed();
	m_recSpat->AddSrcNode(clusterId, seed, cluster->GetNSrc() + 1, l, m);

	// add unique temporal reconstructor
	Ptr<Reconstructor<double>> recTemp = m_recTemp->Clone();
	recTemp->SetBufferDim(n, m, 1);
	;
	for (auto it = cluster->Begin(); it != cluster->End(); it++)
	{
		uint32_t seed = (*it)->GetSeed();
		CsHeader::T_IdField id = (*it)->GetNodeId();
		recTemp->AddSrcNode(id, seed);
	}
	m_recTempMap.emplace(clusterId, recTemp);

	m_clusters.emplace(clusterId, cluster);

	//add sequence checker
	// SeqChecker check(l);
	m_seqCheckMap.emplace(clusterId, SeqChecker(l));
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
	else if (!m_recSpat->HasNode(clusterId))
	{
		m_rxDropTrace(p, E_DropCause::UNKNOWN_CLUSTER);
		return false;
	}

	CsHeader::T_SeqField seq = header.GetSeq();

	SeqChecker &check = m_seqCheckMap.at(clusterId);
	bool newSeq = check.AddNewSeq(seq);

	// TODO: actually a new Reconstruction should take place if we have a new sequence for all cluster nodes
	// if (newSeq)
	// {
	// 	return false;
	// 	//ReconstructNext(newSeq);
	// }

	BufferPacketData(p, check.GetSeqDiff());
	if(++m_rxPackets >= m_minPackets)
		ReconstructNext(newSeq);
	return true;
}

void CsSinkApp::BufferPacketData(Ptr<const Packet> packet, uint32_t diff)
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

	if (diff > 1) // we have to fill with zeros if we have a difference greater one
	{
		std::vector<uint32_t> dim = m_recSpat->GetBufferDim(id);
		uint32_t m = dim.at(2);

		double *zeroFill = new double[m * diff](); //zero initialized
		m_recSpat->WriteData(id, zeroFill, m * diff);
		delete[] zeroFill;
	}

	m_recSpat->WriteData(id, data, size);
	delete[] data;

	//set precoding
	CsHeader::T_SrcInfo bitset = header.GetSrcInfo();

	std::vector<bool> precode;
	precode.reserve(CsHeader::SRCINFO_BITLEN);
	for (uint32_t i = 0; i < CsHeader::SRCINFO_BITLEN; i++)
	{
		precode.push_back(bitset[i]);
	}
	m_recSpat->SetPrecodeEntries(id, precode);
}

void CsSinkApp::ReconstructNext(bool newSeq)
{
	NS_LOG_FUNCTION(this);
	if(newSeq)
		StartNewSeq();

	NS_LOG_INFO("Reconstructing measurement seqence " << m_seqCount << ", " << m_recAttempt+1 << ". attempt");

	//spatial
	NS_LOG_INFO("Reconstructing  spatially");
	m_recSpat->ReconstructAll();
	//temporal
	NS_LOG_INFO("Reconstructing  temporally");
	for (auto it = m_recSpat->IdBegin(); it != m_recSpat->IdEnd(); it++) // iterate over cluster ids
	{
		CsHeader::T_IdField clusterId = *it;
		Ptr<Reconstructor<double>> recTemp = m_recTempMap.at(clusterId);

		//write data to each source node
		std::vector<double> data = m_recSpat->ReadRecDataRow(clusterId);

		//write to data stream
		Ptr<CsCluster> cluster = m_clusters.at(clusterId);
		Ptr<DataStream<double>> lastStream = *(cluster->StreamEnd() - 1); // how do we know this is the stream we need?
		lastStream->CreateBuffer(data);
		//cluster->AddStream(spatStream);

		auto dataIt = data.begin();
		for (auto it = recTemp->IdBegin(); it != recTemp->IdEnd(); it++) // iterate over src ids
		{
			CsHeader::T_IdField srcId = *it;
			std::vector<uint32_t> bufdim = recTemp->GetBufferDim(srcId);
			uint32_t bufSize = bufdim.at(1) * bufdim.at(2);

			std::vector<double> srcData(dataIt, dataIt + bufSize);

			// /*
			// * Solution for non-dense Y: Solver problems with 0 entries:
			// * Add a very low mean!
			// */
			// for (auto it = srcData.begin(); it != srcData.end(); it++)
			// {
			// 	*(it) += 1e-15;
			// }

			recTemp->WriteData(srcId, srcData);
			dataIt += bufSize;
		}

		recTemp->ReconstructAll();

		//write to data stream
		// std::vector<double> dataAll;
		// Ptr<DataStream<double>> lastStream = *(cluster->StreamEnd() - 1);
		// lastStream->CreateBuffer(data);
		//Ptr<DataStream<double>> lastStream;
		Ptr<CsNode> clusterNode = cluster->GetClusterNode();
		CsHeader::T_IdField id = clusterNode->GetNodeId();

		std::vector<double> clusterData = recTemp->ReadRecData(id);
		lastStream = *(clusterNode->StreamEnd() - 1);
		lastStream->CreateBuffer(clusterData);
		for (auto it = cluster->SrcBegin(); it != cluster->SrcEnd(); it++) // iterate over src ids
		{
			Ptr<CsNode> node = *it;
			CsHeader::T_IdField srcId = node->GetNodeId();

			std::vector<double> data = recTemp->ReadRecData(srcId);
			// dataAll.insert(dataAll.end(), data.begin(), data.end());
			// Ptr<SerialDataBuffer<double>> buf = Create<SerialDataBuffer<double>>(data.size());
			// buf->WriteNext(data);
			// lastStream->AddBuffer(buf);
			Ptr<DataStream<double>> lastStream = *(node->StreamEnd() - 1);
			lastStream->CreateBuffer(data);
		}
		// Ptr<SerialDataBuffer<double>> buf = Create<SerialDataBuffer<double>>(dataAll.size());
		// buf->WriteNext(dataAll);
	}
	m_recAttempt++;
}

void CsSinkApp::StartNewSeq()
{
	m_recAttempt = 0;
	//create new data streams
	for (auto it = m_recSpat->IdBegin(); it != m_recSpat->IdEnd(); it++) // iterate over cluster ids
	{
		Ptr<DataStream<double>> stream = Create<DataStream<double>>("RecSeq" + std::to_string(m_seqCount));
		Ptr<CsCluster> cluster = m_clusters.at(*it);
		cluster->AddStream(stream);
		// add new data streams to each node;
		for (auto it = cluster->Begin(); it != cluster->End(); it++)
		{
			(*it)->AddStream(CopyObject(stream));
		}
	}
	m_seqCount++;
}
