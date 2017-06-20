/**
* \file somple-sink-app.cc
*
* \author Tobias Waurick
* \date 01.06.17
*
*/

#include "simple-sink-app.h"

NS_LOG_COMPONENT_DEFINE("SimpleSinkApp");
NS_OBJECT_ENSURE_REGISTERED(SimpleSinkApp);

TypeId SimpleSinkApp::GetTypeId(void)
{
	static TypeId tid = TypeId("SimpleSinkApp")
							.SetParent<Application>()
							.SetGroupName("SimpleNetwork")
							.AddTraceSource("Rx", "A new packet is received",
											MakeTraceSourceAccessor(&SimpleSinkApp::m_rxTrace),
											"ns3::Packet::TracedCallback");
	return tid;
}

SimpleSinkApp::SimpleSinkApp(uint32_t nSrcNodes, std::ostream &os) : m_nRxDevices(0),
																	 m_nSrcNodes(nSrcNodes),
																	 m_isSetup(false),
																	 m_os(os)
{
	NS_LOG_FUNCTION(this << nSrcNodes);
}

SimpleSinkApp::~SimpleSinkApp()
{
}

void SimpleSinkApp::Setup(Ptr<Node> node)
{
	NS_LOG_FUNCTION(this << node);

	//set device rx callback
	Ptr<NetDevice> device;
	m_node = node;
	m_nRxDevices = m_node->GetNDevices();

	for (uint32_t i = 0; i < m_nRxDevices; i++)
	{
		device = m_node->GetDevice(i);
		device->SetReceiveCallback(MakeCallback(&SimpleSinkApp::Receive, this));
	}
	m_isSetup = true;
}

bool SimpleSinkApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	NS_LOG_FUNCTION(this << dev << p);
	NS_ASSERT(m_isSetup);
	(void)idUnused;
	(void)adrUnused;

	//call trace source
	m_rxTrace(p);
	
	SimpleHeader header;
	p->PeekHeader(header);

	uint8_t nodeId = header.GetPId();
	uint32_t dataSize = header.GetDataSize();
	uint8_t data[dataSize];
	header.GetData(data, dataSize);

	uint32_t nDoubles = dataSize / sizeof(double);
	double values[nDoubles];

	for (uint32_t i = 0; i < nDoubles; i++)
	{
		doubleToBytes conv;
		memcpy(conv.bytes, data + i * sizeof(double), sizeof(double));
		values[i] = conv.dbl;
	}

	PrintOut(nodeId, values, nDoubles);
	return true;
}

void SimpleSinkApp::PrintOut(uint8_t nodeId, double *doubles, uint32_t size)
{
	NS_LOG_FUNCTION(this << nodeId << doubles << size);
	m_os << Simulator::Now() << " - Sink Received from Node "
		 << static_cast<int>(nodeId) << " the following data:\n";
	for (uint32_t i = 0; i < size; i++)
	{
		m_os << *(doubles + i) << " ";
	}
	m_os << std::endl;
}