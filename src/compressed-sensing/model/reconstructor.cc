/**
* \file reconstructor.cc
*
* \author Tobias Waurick
* \date 07.06.17
*
*/

#include "reconstructor.h"
#include "assert.h"
NS_LOG_COMPONENT_DEFINE("Reconstructor");
NS_OBJECT_ENSURE_REGISTERED(Reconstructor);

TypeId Reconstructor::GetTypeId(void)
{
	static TypeId tid = TypeId(" Reconstructor")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Reconstructor::m_ranMat),
										  MakePointerChecker<RandomMatrix>());
	return tid;
}

Reconstructor::Reconstructor() : m_nMeasDef(0),
								 m_mMaxDef(0),
								 m_vecLenDef(0)
{
}

void Reconstructor::AddSrcNode(uint8_t nodeId, uint32_t seed)
{
	AddSrcNode(nodeId, seed, m_nMeasDef, m_mMaxDef, m_vecLenDef);
}

void Reconstructor::AddSrcNode(uint8_t nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nodeId << seed << nMeas << mMax << vecLen);
	NS_ASSERT_MSG(!m_nodeInBufMap.count(nodeId), "Node was already added!");
	NS_ASSERT_MSG(nMeas && mMax && vecLen, "Dimensions must be >0 !");

	//create input input buffer
	Ptr<T_NodeBuffer> nodeBufIn = ns3::CreateObject<T_NodeBuffer>(mMax, vecLen);
	Ptr<T_NodeBuffer> nodeBufOut = ns3::CreateObject<T_NodeBuffer>(nMeas, vecLen);
	m_nodeInBufMap.at(nodeId) = nodeBufIn;

	m_nodeOutBufMap.at(nodeId) = nodeBufOut;

	T_NodeInfo info = {seed, nMeas, 0, vecLen, mMax};
	m_nodeInfoMap.at(nodeId) = info;
	m_nNodes++;
}

void Reconstructor::SetBufferDim(uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << vecLen);
	m_nMeasDef = nMeas;
	m_mMaxDef = mMax;
	m_vecLenDef = vecLen;
}

uint32_t Reconstructor::WriteData(uint8_t nodeId, double *buffer, uint32_t bufSize)
{
	NS_LOG_FUNCTION(this << nodeId << buffer << bufSize);
	NS_ASSERT_MSG(m_nodeInBufMap.count(nodeId), "Node ID unknown!");

	Ptr<T_NodeBuffer> nodeBuf = m_nodeInBufMap.at(nodeId);
	uint32_t space = nodeBuf->WriteData(buffer, bufSize);

	T_NodeInfo &info = m_nodeInfoMap.at(nodeId);
	info.m = nodeBuf->GetWrRow();

	return space;
}

void Reconstructor::ReadRecData(uint8_t nodeId, double *buffer, uint32_t bufSize) const
{
	NS_LOG_FUNCTION(this << nodeId << buffer << bufSize);
	NS_ASSERT_MSG(m_nodeOutBufMap.count(nodeId), "Node ID unknown!");

	Ptr<T_NodeBuffer> nodeBuf = m_nodeOutBufMap.at(nodeId);
	nodeBuf->ReadBuf(buffer, bufSize);
}

uint32_t Reconstructor::GetNofMeas(uint8_t nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);
	NS_ASSERT_MSG(m_nodeOutBufMap.count(nodeId), "Node ID unknown!");

	Ptr<T_NodeBuffer> nodeBuf = m_nodeOutBufMap.at(nodeId);
	return nodeBuf->GetWrElem();
}

Mat<double> Reconstructor::GetBufMat(uint8_t nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);
	NS_ASSERT_MSG(m_nodeInBufMap.count(nodeId), "Node ID unknown!");

	Ptr<T_NodeBuffer> nodeBuf = m_nodeInBufMap[nodeId];
	return nodeBuf->ReadAll();
}

Mat<double> Reconstructor::GetSensMat(uint8_t nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);
	NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");

	T_NodeInfo info = m_nodeInfoMap.at(nodeId);
	m_ranMat->SetSize(info.m, info.nMeas);
	m_ranMat->Generate(info.seed);
	return *m_ranMat;
}