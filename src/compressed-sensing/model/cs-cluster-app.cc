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
							.AddAttribute("ComprSpat", "Spatial Compressor",
										  PointerValue(CreateObject<Compressor<double>>()),
										  MakePointerAccessor(&CsClusterApp::m_comp),
										  MakePointerChecker<Compressor<double>>())
							.AddAttribute("l", "NOF of measurement vectors after spatial compression",
										  UintegerValue(64),
										  MakeUintegerAccessor(&CsClusterApp::m_l),
										  MakeUintegerChecker<uint32_t>())
							.AddTraceSource("RxDrop",
											"Trace source indicating a packet has been dropped by the device during reception",
											MakeTraceSourceAccessor(&CsClusterApp::m_rxDropTrace),
											"CsClusterApp::RxDropCallback")
							.AddTraceSource("ComprFail",
											"Trace source when spatial compression has failed"
											"by the device during reception",
											MakeTraceSourceAccessor(&CsClusterApp::m_compressFailTrace),
											"CsClusterApp::CompressFailCallback");
	return tid;
}

CsClusterApp::CsClusterApp() : m_l(0), m_zData()
{
	NS_LOG_FUNCTION(this);
}

CsClusterApp::CsClusterApp(uint32_t n, uint32_t m, uint32_t l) : CsSrcApp(n, m), m_l(l), m_outBufSize(2 * m * l), m_zData(m, l),
																 m_normalize(false), m_running(false), m_isSetup(false)
{
	NS_LOG_FUNCTION(this << n << m << l);
}

void CsClusterApp::Setup(Ptr<CsNode> node, std::string filename)
{
	NS_LOG_FUNCTION(this << node << filename);
	NS_ASSERT_MSG(node->IsCluster(), "Must be a cluster node!");
	/*--------  initialize temporal compressor  --------*/
	CsSrcApp::Setup(node, filename);

	/*--------  Setup receiving netdevices  --------*/
	NetDeviceContainer devices = node->GetRxDevices();
	for (auto it = devices.Begin(); it != devices.End(); it++)
	{
		(*it)->SetReceiveCallback(MakeCallback(&CsClusterApp::Receive, this));
	}

	m_outBufSize = 2 * m_m * m_l;

	m_isSetup = true;
}

void CsClusterApp::SetSpatialCompressor(Ptr<Compressor<double>> comp)
{
	NS_LOG_FUNCTION(this << comp);

	m_comp = comp;
	// m_comp->Setup(m_node->GetSeed(), m_l, m_m, m_normalize);
}

void CsClusterApp::SetSpatialCompressor(Ptr<Compressor<double>> comp, uint32_t l, bool norm)
{
	NS_LOG_FUNCTION(this << comp << l << norm);

	m_comp = comp;
	SetSpatialCompressDim(l);
	m_normalize = norm;

	// m_comp->Setup(m_node->GetSeed(), m_l, m_m, m_normalize);
}

void CsClusterApp::SetSpatialCompressDim(uint32_t l)
{
	NS_LOG_FUNCTION(this << l);

	m_l = l;

	m_zData.Resize(m_l, m_m);
	// m_yR.Resize(m);
	// m_comp->Setup(m_node->GetSeed(), m_l, m_m, m_normalize);
}
void CsClusterApp::StartApplication()
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");
	m_running = true;
}

void CsClusterApp::StopApplication()
{
	NS_LOG_FUNCTION(this);

	CsSrcApp::StopApplication();
	m_running = false;
}

bool CsClusterApp::CompressNext()
{
	NS_LOG_FUNCTION(this);

	//prepare source data
	CsSrcApp::CompressNext();

	m_srcDataBuffer.WriteData(m_yR.GetMem(), m_m, m_node->GetNodeId());
	m_srcDataBuffer.SortByMeta();

	//setup compressor & compress
	uint32_t nMeas = m_srcDataBuffer.GetWrRow();
	if (nMeas < m_l)
	{
		m_compressFailTrace(m_clusterId);
		return false;
	}

	uint32_t zBufSize = m_zData.nElem();
	double *zData = new double[zBufSize]; //temporal buffer on heap will be moved to m_zData

	m_comp->Setup(m_node->GetSeed(), nMeas, m_l, m_m, m_normalize);
	m_comp->CompressSparse(m_srcDataBuffer.ReadAll(),
						   m_srcDataBuffer.ReadAllMeta(),
						   zData, m_l * m_m);
	m_zData.WrMove(zData, zBufSize);
	return true;
}

void CsClusterApp::CreateCsPackets() //TODO
{
	NS_LOG_FUNCTION(this);
	/*--------  Create packets from that data  --------*/
	std::vector<Ptr<Packet>> pktList;

	uint32_t payloadSize = GetMaxPayloadSize();
	uint32_t packetsNow = 1 + (m_outBufSize / payloadSize);

	CsHeader header;
	header.SetClusterId(m_clusterId);
	header.SetNodeId(m_nodeId);
	header.SetSeq(m_nextSeq);
	header.SetDataSize(payloadSize);

	const uint8_t *byte_ptr = reinterpret_cast<const uint8_t *>(m_outBuf.GetMem());
	uint32_t remain = m_outBufSize;
	for (uint32_t i = 0; i < packetsNow - 1; i++) //last packet might be truncated
	{
		remain -= payloadSize;
		Ptr<Packet> p = Create<Packet>(byte_ptr + i * payloadSize, payloadSize);
		pktList.push_back(p);
	}
	//last package
	Ptr<Packet> p = Create<Packet>(byte_ptr + (packetsNow - 1) * payloadSize, remain);
	pktList.push_back(p);

	WriteTxPacketList(pktList);
}

uint32_t
CsClusterApp::GetMaxPayloadSize() //TODO: for now mxl, might be too large in future, send row by row?pw
{
	return 2 * m_m * m_l * sizeof(double);
}

void CsClusterApp::DoNetworkCoding() //TODO: actual NC
{
	NS_LOG_FUNCTION(this);

	arma::Col<CsHeader::T_IdField> ids;
	arma::Mat<double> tmp(2 * m_l, m_m);

	tmp.rows(0, m_l - 1) = m_zData.Read();

	for (auto it = ids.begin(); it != ids.end(); it++) //create unity rows
	{
		uint32_t i = *it;
		tmp(m_l + i, i) = 1.0;
	}
	tmp = tmp.t(); // since we send row by row

	m_outBuf.WriteNext(tmp.memptr(), 2 * m_l * m_m);
}

bool CsClusterApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p << idUnused << adrUnused);
	(void)idUnused;
	(void)adrUnused;

	bool success = false;
	CsHeader header;
	CsHeader::T_IdField nodeId,
		clusterId;

	p->PeekHeader(header);
	nodeId = header.GetNodeId();
	clusterId = header.GetClusterId();

	if (clusterId == m_clusterId) //we receive from a source node of this cluster
		success = ReceiveSrc(p);
	else if (nodeId == CsHeader::CLUSTER_NODEID) // we receive from a different cluster nide
		success = ReceiveCluster(p);
	else //  we receive from a source node of a different cluster
		m_rxDropTrace(p, E_DropCause::SRC_NOT_IN_CLUSTER);

	return success;
}

bool CsClusterApp::ReceiveSrc(const Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION(this << packet);

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

	size = header.GetDataSize();
	if (size != m_m) //check if has correct size
	{
		m_rxDropTrace(packet, E_DropCause::SIZE_MISMATCH);
		return false;
	}

	double* data = new double[m_m];
	uint32_t nBytes = CsSrcApp::GetMaxPayloadSize();
	p->CopyData(reinterpret_cast<uint8_t *>(data), nBytes);

	nodeId = header.GetNodeId();
	m_srcDataBuffer.WriteData(data, size, nodeId);

	delete[] data;
	return true;
}

bool CsClusterApp::ReceiveCluster(const Ptr<const Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	return true;
}

void CsClusterApp::StartNewSeq(CsHeader::T_SeqField seq)
{
	NS_LOG_FUNCTION(this << seq);
	if (CompressNext())
	{
		DoNetworkCoding();
		CreateCsPackets();
		if (!IsSending())
		{
			ScheduleTx(MilliSeconds(0.0));
		}
	}
	m_nextSeq = seq;
}