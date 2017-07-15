/**
* \file cs-cluster-app.cc
*
* \author Tobias Waurick
* \date 15.07.17
*
*/
#include "cs-cluster-app.h"

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
										  MakePointerChecker<Compressor<double>>());
	return tid;
}

CsClusterApp::CsClusterApp()
{
}

CsClusterApp::CsSrcApp(uint32_t n, uint32_t m1, uint32_t m2)
{
}


CsClusterApp::Setup(Ptr<CsNode> node, T_IdField clusterId, uint32_t nSrcNodes, std::string filename)
{
}


void
CsClusterApp::StartApplication()
{
}


void
CsClusterApp::StopApplication()
{
}


bool
CsClusterApp::CompressNext()
{
}


void
CsClusterApp::CreateCsPackets()
{
}


uint32_t
CsClusterApp::GetMaxPayloadSize()
{
}


void
CsClusterApp::DoNetworkCoding()
{
}


bool
CsClusterApp::Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
}



