/**
* \file Simple-src-app.cc	
*
* \author Tobias Waurick
* \date 18.05.2017	
*
* description
*/
#include "ns3/log.h"
#include <iostream>

#include <fstream>
#include "simple-src-app.h"

NS_LOG_COMPONENT_DEFINE("SimpleSrcApp");
NS_OBJECT_ENSURE_REGISTERED(SimpleSrcApp);

TypeId SimpleSrcApp::GetTypeId(void)
{
	static TypeId tid = TypeId("SimpleSrcApp")
							.SetParent<Application>()
							.SetGroupName("SimpleNetwork")
							.AddConstructor<SimpleSrcApp>()
							.AddAttribute("Interval",
										  "The time to wait between packets",
										  TimeValue(MilliSeconds(1)),
										  MakeTimeAccessor(&SimpleSrcApp::m_interval),
										  MakeTimeChecker())
							.AddAttribute("RelayDelay",
										  "The time to wait to send a received packet",
										  TimeValue(MilliSeconds(1)),
										  MakeTimeAccessor(&SimpleSrcApp::m_relayDelay),
										  MakeTimeChecker())
							.AddAttribute("PacketSize", "Size of outbound packets in bytes",
										  UintegerValue(2),
										  MakeUintegerAccessor(&SimpleSrcApp::m_packetSize),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("NofPackets", "Number of Packets to send",
										  UintegerValue(1),
										  MakeUintegerAccessor(&SimpleSrcApp::m_nPackets),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("RanVar", "The random variable attached to create package data.",
										  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
										  MakePointerAccessor(&SimpleSrcApp::m_ranvar),
										  MakePointerChecker<RandomVariableStream>())
							.AddAttribute("RanStartMs", "The random variable to set the starting time in ms of Tx.",
										  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=0.0]"),
										  MakePointerAccessor(&SimpleSrcApp::m_ranStartMs),
										  MakePointerChecker<RandomVariableStream>())
							.AddTraceSource("Tx", "A new packet is sent",
											MakeTraceSourceAccessor(&SimpleSrcApp::m_txTrace),
											"ns3::Packet::TracedCallback")
							.AddTraceSource("Rx", "A new packet is received",
											MakeTraceSourceAccessor(&SimpleSrcApp::m_rxTrace),
											"ns3::Packet::TracedCallback");
	return tid;
}

SimpleSrcApp::SimpleSrcApp() : m_dataSize(0),
							   m_sent(0),
							   m_sendEvent(EventId()),
							   m_relayEvent(EventId()),
							   m_running(false),
							   m_isSetup(false),
							   m_isRelay(false),
							   m_relayDelay(Seconds(0.0))
{
	NS_LOG_FUNCTION(this);
}

SimpleSrcApp::~SimpleSrcApp()
{
	NS_LOG_FUNCTION(this);

	m_dataSize = 0;
}
void SimpleSrcApp::Setup(uint8_t nodeId, Ptr<Node> node)
{
	NS_LOG_FUNCTION(this << nodeId << node);

	SetupPriv(nodeId, node);

	m_ranvar = CreateObject<UniformRandomVariable>();
	m_sent = 0;

	//check if data size is a multiple of a double
	uint32_t residuum = m_dataSize % sizeof(double);
	if (residuum)
	{
		m_dataSize -= residuum; //remove residuum
		NS_LOG_WARN("SimpleSrcApp of Node " << m_node->GetId() 
		<< " :Size of data payload not fitting. It was resized to:" << m_dataSize);
	}

	uint32_t nofBytes = m_dataSize * m_nPackets;
	if (nofBytes)
	{
		uint32_t nofDoubles = nofBytes / sizeof(double); //doubles to create for selected data size
		m_byteBuf.Resize(nofBytes);

		NS_LOG_UNCOND("Source node " << m_node->GetId() << " is going to send the following data:");
		doubleToBytes conv;
		for (uint32_t i = 0; i < nofDoubles; i++)
		{
			conv.dbl = m_ranvar->GetValue();
			m_byteBuf.WriteNext(conv.bytes, (uint32_t)sizeof(double));
			nofBytes -= sizeof(double); //remaining bytes
			NS_LOG_UNCOND(conv.dbl);
		}
	}
	// // if there are still bytes to send, send a truncated double
	// if (nofBytes)
	// {
	// 	NS_LOG_INFO(m_node->GetId() << ": A truncated double will be send!");
	// 	conv.dbl = m_ranvar->GetValue();
	// 	m_byteBuf.WriteNext(conv.bytes, nofBytes);
	// }
}
void SimpleSrcApp::Setup(uint8_t nodeId, Ptr<Node> node, std::string filename)
{
	NS_LOG_FUNCTION(this << node << filename);

	SetupPriv(nodeId, node);

	std::ifstream fid;
	fid.open(filename);
	NS_ASSERT_MSG(fid.is_open(), "No such file.");

	// m_nPackets = nPackets;
}

void SimpleSrcApp::StartApplication()
{
	NS_LOG_FUNCTION(this);
	NS_ASSERT(m_isSetup);

	m_running = true;
	if (m_nPackets > 0)
	{
		ScheduleTx(MilliSeconds(m_ranStartMs->GetInteger()));
	}
}
void SimpleSrcApp::StopApplication()
{
	if (m_isRelay)
		m_relayDevice->SetReceiveCallback(
			MakeNullCallback<bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address &>());
	Simulator::Cancel(m_sendEvent);
	Simulator::Cancel(m_relayEvent);
}

void SimpleSrcApp::SetupRelay(uint32_t deviceIdx)
{
	NS_LOG_FUNCTION(this << deviceIdx);
	NS_ASSERT_MSG(deviceIdx < m_nDevices, "Not enough devices on this node");

	Ptr<NetDevice> device = m_node->GetDevice(deviceIdx);
	device->SetReceiveCallback(MakeCallback(&SimpleSrcApp::Receive, this));
	m_relayDevice = device;

	for (uint32_t i = 0; i < m_nTxDevices; i++)
	{
		if (m_isTxDevice[i] == deviceIdx)
		{
			m_isTxDevice.erase(m_isTxDevice.begin() + i); //remove from tx device idx list
			break;
		}
	}
	m_nTxDevices = m_isTxDevice.size();

	m_isRelay = true;
}

void SimpleSrcApp::SetupPriv(uint8_t nodeId, Ptr<Node> node)
{
	NS_LOG_FUNCTION(this << nodeId << node);
	NS_ASSERT(!m_isSetup);

	m_node = node;
	m_nodeId = nodeId;
	m_nDevices = node->GetNDevices();
	NS_ASSERT_MSG(m_nDevices > 0, "No net devices on this node!");

	m_nTxDevices = m_nDevices;
	m_isTxDevice = std::vector<uint32_t>(m_nTxDevices);

	//set net device indexes for transmitting (initial: all)
	for (uint32_t i = 0; i < m_nTxDevices; i++)
	{
		m_isTxDevice[i] = i;
	}

	NS_ASSERT_MSG(m_packetSize, "Packet size must be at least 2 byte to send data (1 byte header)!");
	m_dataSize = m_packetSize - 1; //-1: 8bit id in header

	m_isSetup = true;
}

void SimpleSrcApp::ScheduleTx(Time dt)
{
	NS_LOG_FUNCTION(this << dt);

	Ptr<Packet> p;
	uint8_t data[m_dataSize];

	// Get data from buffer
	m_byteBuf.ReadNext(data, m_dataSize);
	//create the packet
	p = Create<Packet>();
	SimpleHeader header;
	header.SetPId(m_nodeId);
	header.SetData(data, m_dataSize);
	p->AddHeader(header);
	//send
	m_sendEvent = Simulator::Schedule(dt, &SimpleSrcApp::SendPacket, this, p);
}

void SimpleSrcApp::SendPacket(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);

	SendToAll(p);
	// new tx?
	m_sent++;
	if (m_sent < m_nPackets)
	{
		ScheduleTx(m_interval);
	}
}

bool SimpleSrcApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p);
	NS_ASSERT(m_isRelay);
	(void)idUnused;
	(void)adrUnused;
	//call trace
	NS_LOG_INFO(m_node->GetId() << " received");
	m_rxTrace(p);
	//relay via tx device
	Ptr<Packet> relayPacket = Create<Packet>(*p);
	m_relayEvent = Simulator::Schedule(m_relayDelay, &SimpleSrcApp::SendToAll, this, relayPacket);

	return true;
}

void SimpleSrcApp::SendToAll(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	//	NS_ASSERT(m_sendEvent.IsExpired() && m_relayEvent.IsExpired());

	Ptr<NetDevice> device;

	//call trace source
	NS_LOG_INFO(m_node->GetId() << " is about to send");
	m_txTrace(p);

	for (uint32_t i = 0; i < m_nTxDevices; i++)
	{
		device = m_node->GetDevice(m_isTxDevice[i]);
		//send assuming MySimpleNetDevice thus invalid Address
		device->Send(p, Address(), 0);
	}
}