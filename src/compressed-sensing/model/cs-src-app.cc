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

TypeId
CsSrcApp::GetTypeId(void)
{
	static TypeId tid = TypeId("CsSrcApp")
							.SetParent<Application>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsSrcApp>()
							.AddAttribute("Interval",
										  "The time to wait between packets",
										  TimeValue(MilliSeconds(1)),
										  MakeTimeAccessor(&CsSrcApp::m_interval),
										  MakeTimeChecker())
							.AddAttribute("PacketSize", "Maximum size of outbound packets' payload in bytes",
										  UintegerValue(8),
										  MakeUintegerAccessor(&CsSrcApp::m_packetSize),
										  MakeUintegerChecker<uint32_t>(1))
							.AddAttribute("m", "Length of original measurement vector",
										  UintegerValue(128),
										  MakeUintegerAccessor(&CsSrcApp::m_m),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("n", "Length of compressed vector",
										  UintegerValue(256),
										  MakeUintegerAccessor(&CsSrcApp::m_n),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("TxProb", "Probability to send",
										  DoubleValue(1.0),
										  MakeDoubleAccessor(&CsSrcApp::m_txProb),
										  MakeDoubleChecker<double>(0.0, 1.0))
							.AddAttribute("RanVar", "The random variable attached to create package data.",
										  TypeId::ATTR_GET,
										  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
										  MakePointerAccessor(&CsSrcApp::m_ranTx),
										  MakePointerChecker<RandomVariableStream>())
							.AddTraceSource("Tx", "A new packet is sent",
											MakeTraceSourceAccessor(&CsSrcApp::m_txTrace),
											"ns3::Packet::TracedCallback");
	return tid;
}

CsSrcApp::CsSrcApp() : m_nodeId(0), m_clusterId(0), m_seed(0),
					   m_n(0), m_m(0), m_nDevices(0), m_nTxDevices(0),
					   m_nMeas(0), m_nPackets(0), m_sent(0),
					   m_normalize(false),
					   m_running(false),
					   m_isSetup(false),
					   m_sendEvent(EventId())

{
	NS_LOG_FUNCTION(this);
}

CsSrcApp::CsSrcApp(uint32_t seed, uint32_t n, uint32_t m) : m_nodeId(0), m_clusterId(0), m_seed(seed),
															m_n(n), m_m(m), m_nDevices(0), m_nTxDevices(0),
															m_nMeas(0), m_nPackets(0), m_sent(0),
															m_normalize(false),
															m_running(false),
															m_isSetup(false),
															m_yR(m),
															m_sendEvent(EventId())
{
	NS_LOG_FUNCTION(this << seed << n << m);
}

void CsSrcApp::Setup(Ptr<Node> node, CsHeader::T_IdField nodeId, CsHeader::T_IdField clusterId, std::string filename)
{
	using namespace std;
	NS_LOG_FUNCTION(this << node << nodeId << clusterId << filename);
	NS_ASSERT(!m_isSetup);

	m_node = node;
	m_nodeId = nodeId;
	m_clusterId = clusterId;

	/*--------  read data from file  --------*/

	ifstream ifs(filename, ifstream::in | ifstream::binary);
	NS_ASSERT_MSG(ifs.is_open(), "Cannot open file!");

	uint32_t flen;
	double *buffer;
	uint32_t bufSize;

	ifs.seekg(0, ifs.end);
	flen = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	bufSize = flen / sizeof(double);
	buffer = new double[bufSize];

	ifs.read(reinterpret_cast<char *>(buffer), flen);
	if (!ifs)
		NS_LOG_WARN("Only " << ifs.gcount() << " bytes could be read!");
	ifs.close();

	//write to serial buffer
	// m_fdata.Resize(flen);
	m_fdata.MoveMem(buffer, flen);

	// delete[] buffer;
	// buffer = 0;

	/*--------  calculating NOF packets needed  --------*/

	m_nPackets = 1 + (bufSize - 1) / m_packetSize; // ceil

	/*--------  get tx devices from node  --------*/
	m_nDevices = node->GetNDevices();
	NS_ASSERT_MSG(m_nDevices > 0, "No net devices on this node!");
	m_isTxDevice = std::vector<uint32_t>(m_nDevices);

	//set net device indexes for transmitting (initial: all)
	m_nTxDevices = m_nDevices;
	for (uint32_t i = 0; i < m_nTxDevices; i++)
	{
		m_isTxDevice[i] = i;
	}

	m_isSetup = true;
}

void CsSrcApp::SetTempCompressor(Ptr<CompressorTemp<double>> comp)
{

	NS_LOG_FUNCTION(this << comp);
	m_compR = comp;
	m_compR->Setup(m_seed, m_n, m_m, m_normalize);
}

void CsSrcApp::SetTempCompressor(Ptr<CompressorTemp<double>> comp, uint32_t seed, uint32_t n, uint32_t m, bool norm)
{
	NS_LOG_FUNCTION(this << comp << seed << n << m << norm);
	m_compR = comp;
	m_seed = seed;
	m_n = n;
	m_m = m;
	m_normalize = norm;

	m_compR->Setup(m_seed, m_n, m_m, m_normalize);
}

void CsSrcApp::SetTempCompressDim(uint32_t n, uint32_t m)
{
	NS_LOG_FUNCTION(this << n << m);
	m_n = n;
	m_m = m;

	m_yR.Resize(m);
	m_compR->Setup(m_seed, m_n, m_m, m_normalize);
}

void CsSrcApp::SetSeed(uint32_t seed, bool norm)
{
	NS_LOG_FUNCTION(this << seed << norm);
	m_compR->SetSeed(seed, norm);
}

void CsSrcApp::SetTxProb(double p)
{
	NS_LOG_FUNCTION(this << p);
	m_txProb = p;
}

void CsSrcApp::StartApplication()
{
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");

	m_running = true;
	if (m_nPackets > 0)
	{
		ScheduleTx(MilliSeconds(0.0));
	}
}

void CsSrcApp::StopApplication()
{
	Simulator::Cancel(m_sendEvent);
}

Ptr<Packet> CsSrcApp::CreateCsPackets()
{
	return Ptr<Packet>();
}

void CsSrcApp::CompressNext()
{
	uint32_t remain = m_fdata.GetRemaining();
	double *yData = new double[m_m];

	if (remain < m_n) // this actually shouldn't happen if nPackets is calculated right, but just to be sure
	{
		NS_LOG_WARN("Not enough samples left in file, sending zeros!");
		std::fill(yData, yData + m_m, 0);
	}
	else
	{
		double xData[m_n];
		m_fdata.ReadNext(xData, m_n);
		m_compR->Compress(xData, m_n, yData, m_m);
	}

	m_yR.MoveMem(yData, m_m);
}

void CsSrcApp::WriteTxPacketList(std::vector<Ptr<Packet>> &pktList)
{
	m_txPackets.insert(m_txPackets.end(), pktList.begin(), pktList.end());
}

void CsSrcApp::SendPacket(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);

	//call trace source
	NS_LOG_INFO(m_node->GetId() << " is about to send");
	m_txTrace(p);

	Ptr<NetDevice> device;
	for (uint32_t i = 0; i < m_nTxDevices; i++)
	{
		device = m_node->GetDevice(m_isTxDevice[i]);
		//send assuming MySimpleNetDevice thus invalid Address
		device->Send(p, Address(), 0);
	}
	// new tx?
	m_sent++;
	if (m_sent < m_nPackets)
	{
		ScheduleTx(m_interval);
	}
}

void CsSrcApp::ScheduleTx(Time dt)
{
	NS_LOG_FUNCTION(this << dt);
	Ptr<Packet> pkt = CreateCsPacket();
	//schedule send
	m_sendEvent = Simulator::Schedule(dt, &CsSrcApp::SendPacket, this, pkt);
}
