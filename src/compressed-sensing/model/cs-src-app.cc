/**
* \file cs-src-app.h
*
* \author Tobias Waurick
* \date 12.07.17
*
*/
#include <iostream>
#include <fstream>
#include <algorithm>
#include "cs-src-app.h"
#include "assert.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("CsSrcApp");
NS_OBJECT_ENSURE_REGISTERED(CsSrcApp);

TypeId
CsSrcApp::GetTypeId(void)
{
	static TypeId tid = TypeId("CsSrcApp")
							.SetParent<Application>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsSrcApp>()
							.AddAttribute("Interval",
										  "The time to wait between packets",
										  TimeValue(MilliSeconds(100)),
										  MakeTimeAccessor(&CsSrcApp::m_interval),
										  MakeTimeChecker(Seconds(0)))
							// .AddAttribute("PacketSize", "Maximum size of outbound packets' payload in bytes",
							// 			  UintegerValue(8),
							// 			  MakeUintegerAccessor(&CsSrcApp::m_packetSize),
							// 			  MakeUintegerChecker<uint32_t>(8))
							.AddAttribute("n", "Length of original measurement vector",
										  UintegerValue(256),
										  MakeUintegerAccessor(&CsSrcApp::m_n),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("m", "Length of compressed vector",
										  UintegerValue(128),
										  MakeUintegerAccessor(&CsSrcApp::m_m),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("TxProb", "Probability to send",
										  DoubleValue(1.0),
										  MakeDoubleAccessor(&CsSrcApp::m_txProb),
										  MakeDoubleChecker<double>(0.0, 1.0))
							.AddAttribute("ComprTemp", "Temporal Compressor",
										  PointerValue(),
										  MakePointerAccessor(&CsSrcApp::SetTempCompressor, &CsSrcApp::GetTempCompressor),
										  MakePointerChecker<CompressorTemp>())
							.AddAttribute("RanTx", "The random variable attached to determine when to send.",
										  TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
										  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
										  MakePointerAccessor(&CsSrcApp::m_ranTx),
										  MakePointerChecker<RandomVariableStream>())
							.AddTraceSource("Tx", "A new packet is sent",
											MakeTraceSourceAccessor(&CsSrcApp::m_txTrace),
											"ns3::Packet::TracedCallback")
							.AddTraceSource("Drop", "A packet is dropped",
											MakeTraceSourceAccessor(&CsSrcApp::m_dropTrace),
											"ns3::Packet::TracedCallback");
	return tid;
}

CsSrcApp::CsSrcApp() : m_yR(0), m_nodeId(0), m_clusterId(0),
					   m_nextSeq(0), m_seed(1),
					   m_n(0), m_m(0),
					   //    m_nDevices(0), m_nTxDevices(0),
					   // m_nMeas(0), // m_nPackets(0),
					   m_sent(0),
					   m_running(false),
					   m_isSetup(false),
					   //    m_txPackets(0),
					   m_sendEvent(EventId())

{
	NS_LOG_FUNCTION(this);
	m_stream = Create<DataStream<double>>(STREAMNAME);
}

CsSrcApp::CsSrcApp(uint32_t n, uint32_t m) : m_yR(m), m_nodeId(0), m_clusterId(0),
											 m_nextSeq(0), m_seed(1),
											 m_n(n), m_m(m),
											 //  m_nDevices(0), m_nTxDevices(0),
											 // m_nMeas(0), // m_nPackets(0),
											 m_sent(0),
											 m_running(false),
											 m_isSetup(false),
											 // m_txPackets(0),
											 m_sendEvent(EventId())
{
	NS_LOG_FUNCTION(this << n << m);
	m_stream = Create<DataStream<double>>(STREAMNAME);
}

void CsSrcApp::Setup(Ptr<CsNode> node, Ptr<SerialDataBuffer<double>> input)
{
	using namespace std;
	NS_LOG_FUNCTION(this << node << input);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");
	NS_ASSERT_MSG(node->IsSource() || node->IsCluster(), "Must be a source or cluster node!");

	m_node = node;
	m_nodeId = node->GetNodeId();
	m_clusterId = node->GetClusterId();

	m_seed = node->GetSeed();

	m_fdata = input;

	//setup compressor
	if (!m_compR)
		m_compR = CreateObject<CompressorTemp>();
	m_compR->Setup(m_seed, m_n, m_m);

	//add stream to node
	m_node->AddStream(m_stream);

	m_isSetup = true;
}

void CsSrcApp::SetTempCompressor(Ptr<CompressorTemp> comp)
{
	NS_LOG_FUNCTION(this << comp);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");

	if (comp)
	{
		m_compR = CopyObject(comp);
		m_compR->Setup(m_seed, m_n, m_m);
	}
}

Ptr<CompressorTemp> CsSrcApp::GetTempCompressor() const
{
	NS_LOG_FUNCTION(this);
	return m_compR;
}

// void CsSrcApp::SetTempCompressor(Ptr<CompressorTemp> comp, uint32_t n, uint32_t m, bool norm)
// {
// 	NS_LOG_FUNCTION(this << comp << n << m << norm);

// 	m_compR = CopyObject(comp);
// 	m_n = n;
// 	m_m = m;
// 	m_normalize = norm;

// 	m_compR->Setup(m_seed, m_n, m_m, m_normalize);
// }

void CsSrcApp::SetTempCompressDim(uint32_t n, uint32_t m)
{
	NS_LOG_FUNCTION(this << n << m);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");

	m_n = n;
	m_m = m;

	// m_yR.Resize(m);

	if (!m_compR)
		m_compR->Setup(m_seed, m_n, m_m);
}

// void CsSrcApp::SetSeed(uint32_t seed, bool norm)
// {
// 	NS_LOG_FUNCTION(this << seed << norm);

// 	m_compR->SetSeed(seed, norm);
// }

void CsSrcApp::SetTxProb(double p)
{
	NS_LOG_FUNCTION(this << p);
	NS_ASSERT_MSG(!m_isSetup, "Setup was already called!");

	m_txProb = p;
}

void CsSrcApp::StartApplication()
{
	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");

	/*--------  create all src packets --------*/
	while (CompressNext())
	{
		CreateCsPackets();
	}

	m_running = true;
	if (HasPackets())
	{
		ScheduleTx(MilliSeconds(0.0));
	}
}

void CsSrcApp::StopApplication()
{
	NS_LOG_FUNCTION(this);

	Simulator::Cancel(m_sendEvent);
	m_running = false;
}

void CsSrcApp::SendToAll(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	NS_ASSERT(m_sendEvent.IsExpired());

	Ptr<NetDevice> device;

	m_txTrace(p);
	NetDeviceContainer devices = m_node->GetTxDevices();
	for (auto it = devices.Begin(); it != devices.End(); it++)
	{
		// device = m_node->GetDevice(m_isTxDevice[i]);
		//send assuming MySimpleNetDevice thus invalid Address
		// device->Send(p, Address(), 0);
		(*it)->Send(p, Address(), 0);
	}
}

bool CsSrcApp::CompressNext()
{
	NS_LOG_FUNCTION(this);

	/*--------  Compress next Y  --------*/

	uint32_t remain = m_fdata->GetRemaining();

	if (!m_compR)
	{
		NS_LOG_ERROR("Src Node" << int(m_nodeId) << " has no valid compressor attached!");
		return false;
	}
	else if (remain < m_n)
	{
		// NS_LOG_WARN("Not enough samples left in file, sending zeros!");
		// std::fill(yData, yData + m_m, 0);
		NS_LOG_INFO("Src Node" << int(m_nodeId) << " has no more samples to compress!");
		return false;
	}

	double xData[m_n] = {0};
	double *yData = new double[m_m];
	m_fdata->ReadNext(xData, m_n);
	m_compR->Compress(xData, m_n, yData, m_m);

	// write to stream
	m_stream->CreateBuffer(yData, m_m);

	m_yR.MoveMem(yData, m_m);

	return true;
}

void CsSrcApp::CreateCsPackets()
{
	NS_LOG_FUNCTION(this);
	/*--------  Create packets from that data  --------*/
	// uint32_t nPacketsNow = 1;
	std::vector<Ptr<Packet>> pktList;

	uint32_t payloadSize = GetMaxPayloadSize();

	CsHeader header;
	header.SetClusterId(m_clusterId);
	header.SetNodeId(m_nodeId);
	header.SetSeq(m_nextSeq);
	header.SetDataSize(payloadSize);
	// double yData[m_m];
	// m_yR.ReadNext(yData, m_m);

	// Ptr<Packet> p = Create<Packet>(reinterpret_cast<uint8_t *>(yData), payloadSize);
	Ptr<Packet> p = Create<Packet>(reinterpret_cast<const uint8_t *>(m_yR.GetMem()), payloadSize);
	p->AddHeader(header);
	pktList.push_back(p);
	WriteTxPacketList(pktList);
	/*--------  Update members  --------*/
	m_nextSeq++;
}

void CsSrcApp::WriteTxPacketList(const std::vector<Ptr<Packet>> &pktList)
{
	// NS_LOG_FUNCTION(this);
	// if (HasPackets())
	// 	m_txPackets.insert(m_txPackets.begin() + pktList.size() - 1, pktList.begin(), pktList.end());
	// else
	// 	m_txPackets = pktList;
	if (HasPackets())
		m_txPackets.insert(m_txPackets.end(), pktList.begin(), pktList.end());
	else
		m_txPackets = pktList;
}

uint32_t CsSrcApp::GetMaxPayloadSize()
{
	NS_LOG_FUNCTION(this);

	return m_m * sizeof(double);
}

bool CsSrcApp::HasPackets()
{
	return !m_txPackets.empty();
}

bool CsSrcApp::IsSending()
{
	return m_sendEvent.IsRunning();
}

void CsSrcApp::SendPacket(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);

	SendToAll(p);
	// new tx?
	m_sent++;
	if (HasPackets())
	{
		ScheduleTx(m_interval);
	}
}

void CsSrcApp::ScheduleTx(Time dt)
{
	NS_LOG_FUNCTION(this << dt);
	NS_ASSERT_MSG(HasPackets(), "No packets to schedule!");
	NS_ASSERT_MSG(m_sendEvent.IsExpired(), "Already sending!");

	Ptr<Packet> pkt = m_txPackets.front();
	m_txPackets.erase(m_txPackets.begin());

	//schedule send
	if (m_ranTx->GetValue() < m_txProb)
		m_sendEvent = Simulator::Schedule(dt, &CsSrcApp::SendPacket, this, pkt);
	else if (HasPackets())
		Simulator::Schedule(dt, &CsSrcApp::ScheduleTx, this, m_interval);
}
