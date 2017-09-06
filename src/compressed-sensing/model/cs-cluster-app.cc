/**
* \file cs-cluster-app.cc
*
* \author Tobias Waurick
* \date 15.07.17
*
*/
#include "cs-cluster-app.h"

NS_LOG_COMPONENT_DEFINE("CsClusterApp");
NS_OBJECT_ENSURE_REGISTERED(CsClusterApp);

TypeId
CsClusterApp::GetTypeId(void)
{
	static TypeId tid = TypeId("CsClusterApp")
							.SetParent<CsSrcApp>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsClusterApp>()
							.AddAttribute("TimeOut",
										  "The time to wait for new source data",
										  TimeValue(MilliSeconds(100)),
										  MakeTimeAccessor(&CsClusterApp::m_timeout),
										  MakeTimeChecker(Seconds(0)))
							.AddAttribute("ComprSpat", "Spatial Compressor",
										  PointerValue(CreateObject<Compressor>()),
										  MakePointerAccessor(&CsClusterApp::SetSpatialCompressor, &CsClusterApp::GetSpatialCompressor),
										  MakePointerChecker<Compressor>())
							.AddAttribute("l", "NOF of measurement vectors after spatial compression",
										  UintegerValue(64),
										  MakeUintegerAccessor(&CsClusterApp::m_l),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("nNodes", "NOF source nodes (including cluster node)",
										  UintegerValue(MAX_N_SRCNODES),
										  MakeUintegerAccessor(&CsClusterApp::m_nNodes),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("NcInterval", "Network coding interval",
										  TimeValue(MilliSeconds(200)),
										  MakeTimeAccessor(&CsClusterApp::m_ncInterval),
										  MakeTimeChecker(Seconds(0)))
							.AddAttribute("NcIntervalTimeOut",
										  "NOF network coding intervals with no packages to stop the network coding intervals (0 for no time out)",
										  UintegerValue(10),
										  MakeUintegerAccessor(&CsClusterApp::m_ncTimeOut),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("NcIntervalDelay", "Initial Network coding interval delay",
										  TimeValue(MilliSeconds(10)),
										  MakeTimeAccessor(&CsClusterApp::m_ncIntervalDelay),
										  MakeTimeChecker())
							.AddAttribute("NcEnable", "Enable Network coding?",
										  BooleanValue(true),
										  MakeBooleanAccessor(&CsClusterApp::m_ncEnable),
										  MakeBooleanChecker())
							.AddAttribute("NcShuffle", "Don't do network coding, but shiffle buffered packets?",
										  BooleanValue(false),
										  MakeBooleanAccessor(&CsClusterApp::m_shuffle),
										  MakeBooleanChecker())
							.AddAttribute("NcMax", "Network coding: maximum NOF recombinations",
										  UintegerValue(10),
										  MakeUintegerAccessor(&CsClusterApp::m_ncMaxRecomb),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("NcPktPerLink", "Network coding: NOF coded packet per link at each interval",
										  UintegerValue(1),
										  MakeUintegerAccessor(&CsClusterApp::m_ncPktPLink),
										  MakeUintegerChecker<uint32_t>())
							.AddTraceSource("Rx", "A new packet is received",
											MakeTraceSourceAccessor(&CsClusterApp::m_rxTrace),
											"ns3::Packet::TracedCallback")
							.AddTraceSource("RxDrop",
											"Trace source indicating a packet has been dropped by the device during reception",
											MakeTraceSourceAccessor(&CsClusterApp::m_rxDropTrace),
											"CsClusterApp::RxDropCallback")
		// .AddTraceSource("ComprFail",
		// 				"Trace source when spatial compression has failed"
		// 				"by the device during reception",
		// 				MakeTraceSourceAccessor(&CsClusterApp::m_compressFailTrace),
		// 				"CsClusterApp::CompressFailCallback")
		;
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterApp::CsClusterApp() : m_l(0), m_zData(), m_ncMaxRecomb(0), m_ncPktPLink(0),
							   m_ncTimeOut(0), m_ncTimeOutCnt(0), m_ncEvent(EventId()),
							   m_ncEnable(true), m_shuffle(false),
							   m_running(false), m_isSetup(false), m_timeoutEvent(EventId())
{
	NS_LOG_FUNCTION(this);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::Setup(const Ptr<CsCluster> cluster, Ptr<SerialDataBuffer<double>> input)
{
	NS_LOG_FUNCTION(this << cluster << input);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	/*--------  initialize temporal compressor  --------*/
	CsSrcApp::Setup(cluster->GetClusterHead(), input);

	/*--------  Setup buffers and compressor  --------*/
	m_srcDataBuffer.Resize(m_nNodes, m_m);

	m_seed = cluster->GetClusterSeed();
	if (!m_comp)
		m_comp = CreateObject<Compressor>();
	m_comp->Setup(m_seed, m_nNodes, m_l, m_m);

	m_zData.Resize(m_l, m_m);
	m_isSetup = true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::SetSpatialCompressor(Ptr<Compressor> comp)
{
	NS_LOG_FUNCTION(this << comp);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	if (comp)
	{
		m_comp = CopyObject(comp);
		m_comp->Setup(m_seed, m_nNodes, m_l, m_m);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Ptr<Compressor> CsClusterApp::GetSpatialCompressor() const
{
	NS_LOG_FUNCTION(this);
	return m_comp;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::StartApplication()
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");

	/*--------  Setup receiving netdevices  --------*/
	NetDeviceContainer devices = m_node->GetRxDevices();
	for (auto it = devices.Begin(); it != devices.End(); it++)
	{
		(*it)->SetReceiveCallback(MakeCallback(&CsClusterApp::Receive, this));
	}

	CsSrcApp::StartApplication(); //start measurement cycle
	m_running = true;
	if (m_ncEnable || m_shuffle)
		m_ncEvent = Simulator::Schedule(m_ncInterval + m_ncIntervalDelay, &CsClusterApp::RLNetworkCoding, this, m_ncInterval);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::StopApplication()
{
	NS_LOG_FUNCTION(this);

	CsSrcApp::StopApplication();
	Simulator::Cancel(m_timeoutEvent);
	Simulator::Cancel(m_ncEvent);

	m_running = false;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsClusterApp::CompressNextSpat()
{
	NS_LOG_FUNCTION(this);

	//prepare source data
	if (m_yTemp.GetNWritten() == m_m)
	{
		m_srcDataBuffer.WriteData(m_yTemp.GetMem(), m_m, m_node->GetNodeId());
		m_yTemp.Clear(); // so we now if it was written to again
	}
	m_srcDataBuffer.SortByMeta();

	uint32_t zBufSize = m_zData.nElem();
	double *zData = new double[zBufSize];

	m_comp->CompressSparse(m_srcDataBuffer.ReadAll(),
						   m_srcDataBuffer.ReadAllMeta(),
						   zData, zBufSize);
	//m_comp->Compress(m_srcDataBuffer.ReadAll(),
	//    zData, m_l * m_m);
	m_zData.Write(zData, zBufSize);

	//write info bit field
	m_srcInfo.reset();
	for (uint32_t i = 0; i < m_srcDataBuffer.GetWrRow(); i++)
	{
		uint32_t idx = m_srcDataBuffer.ReadMeta(i);
		m_srcInfo[idx] = 1;
	}

	delete[] zData;

	return true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::CreateCsClusterPackets()
{
	NS_LOG_FUNCTION(this);

	/*--------  Create packets from that data  --------*/
	uint32_t payloadSize = GetMaxPayloadSizeByte();
	uint32_t packetsNow = m_l;
	std::vector<Ptr<Packet>> pktList;
	pktList.reserve(packetsNow);

	CsClusterHeader header;
	header.SetClusterId(m_clusterId);
	header.SetNodeId(m_nodeId);
	header.SetDataSize(payloadSize);
	header.SetSrcInfo(m_srcInfo, m_clusterId);
	header.SetSeq(m_nextSeq);

	for (uint32_t i = 0; i < packetsNow; i++)
	{
		T_PktData zRowData[m_zData.nCols()];
		m_zData.ReadRow(i, zRowData, m_zData.nCols());
		uint8_t *byte_ptr = reinterpret_cast<uint8_t *>(zRowData);
		Ptr<Packet> p = Create<Packet>(byte_ptr, payloadSize);
		header.SetNcInfoNew(m_clusterId, i);
		p->AddHeader(header);
		pktList.push_back(p);
	}
	if (m_ncEnable || m_shuffle) // write to network coding packet buffer
	{
		m_ncPktBuffer.insert(m_ncPktBuffer.end(), pktList.begin(), pktList.end());
	}
	else // simply broadcast
		WriteBcPacketList(pktList);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t
CsClusterApp::GetMaxPayloadSizeByte() const
{
	return GetMaxPayloadSize() * sizeof(T_PktData);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t
CsClusterApp::GetMaxPayloadSize() const
{
	return m_zData.nCols();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::RLNetworkCoding(Time dt)
{
	NS_LOG_FUNCTION(this);

	if (m_ncPktBuffer.size())
		m_ncTimeOutCnt = 0; //reset time out counter
	else
		m_ncTimeOutCnt++;

	//if we have still packets to combine
	while (m_ncPktBuffer.size())
	{
		/*--------  get packets of same measurement sequence  --------*/
		CsClusterHeader header;
		std::vector<Ptr<Packet>> pSameSeq;
		Ptr<Packet> pkt = m_ncPktBuffer.front();
		m_ncPktBuffer.erase(m_ncPktBuffer.begin());
		pSameSeq.push_back(pkt);

		// get sequence from first packet
		pkt->PeekHeader(header);
		CsHeader::T_SeqField seqNow = header.GetSeq();
		//compare to sequences of other headers
		for (auto it = m_ncPktBuffer.begin(); it != m_ncPktBuffer.end();)
		{
			(*it)->PeekHeader(header);
			CsHeader::T_SeqField seq = header.GetSeq();
			if (seq == seqNow)
			{
				pSameSeq.push_back(*it);
				it = m_ncPktBuffer.erase(it);
			}
			else
				it++;
		}

		// /*--------  create packets for each link/tx device  --------*/
		// NetDeviceContainer devices = m_node->GetTxDevices();
		// for (auto it = devices.Begin(); it != devices.End(); it++)
		// {
		// 	Time dtPkt = MilliSeconds(0);
		// 	for (uint32_t i = 0; i < m_ncPktPLink; i++)
		// 	{
		// 		//send assuming MySimpleNetDevice, where Address is not needed
		// 		Ptr<Packet> p = DoRLNC(pSameSeq, seqNow);
		// 		m_txTrace(p);
		// 		Simulator::Schedule(dtPkt, &CsClusterApp::Send, this, p, *it);
		// 		dtPkt += GetPktInterval();
		// 	}

		/*--------  create packets for each link to broadcast  --------*/
		std::vector<Ptr<Packet>> packets;
		if (m_shuffle) //only shuffle their order
		{
			uint32_t nPkt = pSameSeq.size();
			packets.reserve(nPkt);
			for (size_t i = 0; i < nPkt / 2; i++)
			{
				packets.push_back(pSameSeq.at(i));
				packets.push_back(pSameSeq.at(nPkt / 2 + i));
			}
			if (nPkt % 2 != 0) //odd
				packets.push_back(pSameSeq.at(nPkt-1));
		}
		else //do NC
		{
			packets.reserve(m_ncPktPLink);
			for (uint32_t i = 0; i < m_ncPktPLink; i++)
			{
				Ptr<Packet> p = DoRLNC(pSameSeq, seqNow);
				packets.push_back(p);
			}
		}
		WriteBcPacketList(packets);
	}

	//schedule next nc intervall if time out wasn't reached or time out was disabled (m_ncTimeOut == 0)
	if (m_ncTimeOut == 0 || m_ncTimeOutCnt < m_ncTimeOut)
		m_ncEvent = Simulator::Schedule(dt, &CsClusterApp::RLNetworkCoding, this, dt);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsClusterApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p << idUnused << adrUnused);
	(void)idUnused;
	(void)adrUnused;

	bool success = false;

	//call receive trace
	m_rxTrace(p);

	CsHeader header;
	CsHeader::T_IdField nodeId,
		clusterId;

	p->PeekHeader(header);
	nodeId = header.GetNodeId();
	clusterId = header.GetClusterId();

	if (clusterId == m_clusterId && nodeId != CsClusterHeader::CLUSTER_NODEID) //we receive from a source node of this cluster
		success = ReceiveSrc(p);
	else if (clusterId != m_clusterId && nodeId == CsClusterHeader::CLUSTER_NODEID) // we receive from a different cluster node
		success = ReceiveCluster(p);
	else //  we receive from a source node of a different cluster
		m_rxDropTrace(p, E_DropCause::SRC_NOT_IN_CLUSTER);

	return success;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsClusterApp::ReceiveSrc(const Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION(this << packet);
	//cancel old time out event and start a new
	Simulator::Cancel(m_timeoutEvent);
	m_timeoutEvent = Simulator::Schedule(m_timeout, &CsClusterApp::StartNewSeq, this, m_nextSeq + 1);

	CsHeader header;
	CsHeader::T_IdField nodeId;
	CsHeader::T_SeqField seq;
	CsHeader::T_SizeField size;

	Ptr<Packet> p = packet->Copy(); //COW

	p->RemoveHeader(header);

	seq = header.GetSeq();
	if (seq > m_nextSeq) // check if new sequence
		StartNewSeq(seq);
	else if (seq < m_nextSeq) // check if old sequence
	{
		m_rxDropTrace(packet, E_DropCause::EXPIRED_SEQ);
		return false;
	}

	size = header.GetDataSize() / sizeof(T_PktData);
	if (size != m_m) //check if has correct size
	{
		m_rxDropTrace(packet, E_DropCause::SIZE_MISMATCH);
		return false;
	}

	T_PktData *data = new T_PktData[GetMaxPayloadSize()];
	uint32_t nBytes = CsSrcApp::GetMaxPayloadSizeByte();
	p->CopyData(reinterpret_cast<uint8_t *>(data), nBytes);

	nodeId = header.GetNodeId();
	m_srcDataBuffer.WriteData(data, size, nodeId);

	delete[] data;
	return true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsClusterApp::ReceiveCluster(const Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	CsClusterHeader header;
	p->PeekHeader(header);
	if (header.GetNcCount() >= m_ncMaxRecomb) //maximum NOF recombination reached
	{
		m_rxDropTrace(p, E_DropCause::NC_MAXRECOMB);
		return false;
	}
	else if (header.GetDataSize() != GetMaxPayloadSizeByte()) // different size m athan this application
	{
		m_rxDropTrace(p, E_DropCause::SIZE_MISMATCH);
		return false;
	}

	if (m_ncEnable || m_shuffle) //write to nc packet buffer
		m_ncPktBuffer.push_back(p->Copy());
	else //broadcast
		WriteBcPacketList(p->Copy());
	return true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::StartNewSeq(CsHeader::T_SeqField seq)
{
	NS_LOG_FUNCTION(this << seq);
	if (CompressNextSpat())
	{
		CreateCsClusterPackets();
	}
	m_srcDataBuffer.Reset();
	m_nextSeq = seq;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Ptr<Packet> CsClusterApp::DoRLNC(const std::vector<Ptr<Packet>> &pktList, CsClusterHeader::T_SeqField seq)
{
	NS_LOG_FUNCTION(this << &pktList);
	//get coefficients
	std::vector<double> coeffs = m_ncGen.Generate(pktList.size());

	//calculate NC packet

	uint32_t nBytes = GetMaxPayloadSizeByte(),
			 nValues = GetMaxPayloadSize();
	T_PktData dataBuf[nValues] = {0.0};

	//new header
	CsClusterHeader headerNew;
	CsClusterHeader::T_NcInfoField ncInfo(CsClusterHeader::GetNcInfoSize(), 0.0); // empty nc info field
	CsClusterHeader::T_NcCountField ncCountMax = 0;

	for (const auto &pkt : pktList)
	{
		//get header information and packet data
		CsClusterHeader h;
		Ptr<Packet> p = pkt->Copy();
		p->RemoveHeader(h);
		CsClusterHeader::T_NcInfoField pktNcInfo = h.GetNcInfo();

		if (h.GetNcCount() > ncCountMax) // set ncCount to maximum of all packets
			ncCountMax = h.GetNcCount();

		for (CsHeader::T_IdField id = 0; id < CsClusterHeader::GetMaxClusters(); id++) //preserving SrcInfo
		{
			if (h.IsSrcInfoSet(id))
			{
				CsClusterHeader::T_SrcInfoField bitset = h.GetSrcInfo(id);
				headerNew.SetSrcInfo(bitset, id);
			}
		}
		T_PktData pktData[nValues];
		p->CopyData(reinterpret_cast<uint8_t *>(pktData), nBytes);

		//multiply each data value with the coefficient;
		double c = coeffs.back();
		coeffs.pop_back();

		for (uint32_t i = 0; i < GetMaxPayloadSize(); i++)
		{
			dataBuf[i] += pktData[i] * c;
		}

		for (uint32_t i = 0; i < CsClusterHeader::GetNcInfoSize(); i++)
		{
			ncInfo.at(i) += pktNcInfo.at(i) * c;
		}
	}
	//create new packet and add header
	Ptr<Packet> ncPkt = Create<Packet>(reinterpret_cast<const uint8_t *>(dataBuf), nBytes);
	headerNew.SetClusterId(m_clusterId);
	headerNew.SetNodeId(m_nodeId);
	headerNew.SetDataSize(nBytes);
	headerNew.SetSeq(seq);
	headerNew.SetNcInfo(ncInfo);
	headerNew.SetNcCount(ncCountMax);
	ncPkt->AddHeader(headerNew);

	return ncPkt;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterApp::CreateCsPackets()
{
}