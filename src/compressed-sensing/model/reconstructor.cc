/**
* \file reconstructor.cc
*
* \author Tobias Waurick
* \date 07.06.17
*
*/

#include "reconstructor.h"
#include "assert.h"
#include <iostream>
NS_LOG_COMPONENT_DEFINE("Reconstructor");
NS_OBJECT_ENSURE_REGISTERED(Reconstructor);


TypeId Reconstructor::GetTypeId(void)
{
	static TypeId tid = TypeId("Reconstructor")
							.SetParent<Object>()
							//.AddConstructor<Reconstructor>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Reconstructor::m_ranMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of the real transformation in which the solution is sparse",
										  PointerValue(),
										  MakePointerAccessor(&Reconstructor::m_transMat),
										  MakePointerChecker<TransMatrix<double>>())
							.AddTraceSource("RecComplete", "Reconstuction completed",
											MakeTraceSourceAccessor(&Reconstructor::m_completeCb),
											"ns3::Reconstructor::CompleteTracedCallback");
	return tid;
}

Reconstructor::Reconstructor() : m_nMeasDef(0),
								 m_mMaxDef(0),
								 m_vecLenDef(0),
								 m_nodeIdNow(0),
								 m_nodeIdNowConst(0)
{
}

void Reconstructor::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed)
{
	AddSrcNode(nodeId, seed, m_nMeasDef, m_mMaxDef, m_vecLenDef);
}

void Reconstructor::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nodeId << seed << nMeas << mMax << vecLen);
	NS_ASSERT_MSG(!m_nodeInfoMap.count(nodeId), "Node was already added!");
	NS_ASSERT_MSG(nMeas && mMax && vecLen, "Dimensions must be >0 !");

	//create input input buffer
	Ptr<T_NodeBuffer> nodeBufIn = ns3::CreateObject<T_NodeBuffer>(mMax, vecLen);
	Ptr<T_NodeBuffer> nodeBufOut = ns3::CreateObject<T_NodeBuffer>(nMeas, vecLen);

	T_NodeInfo info (seed, nMeas, 0, vecLen, mMax, nodeBufIn, nodeBufOut);
	m_nodeInfoMap[nodeId] = info;
	m_nNodes++;
}

void Reconstructor::SetBufferDim(uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << vecLen);
	m_nMeasDef = nMeas;
	m_mMaxDef = mMax;
	m_vecLenDef = vecLen;
}

uint32_t Reconstructor::WriteData(T_NodeIdTag nodeId, const double *buffer, const uint32_t bufSize)
{
	NS_LOG_FUNCTION(this << nodeId << buffer << bufSize);

	T_NodeInfo &info = CheckOutInfo(nodeId);
	Ptr<T_NodeBuffer> nodeBuf = info.inBufPtr;
	uint32_t space = nodeBuf->WriteData(buffer, bufSize);
	info.m = nodeBuf->GetWrRow();

	return space;
}

std::vector<double> Reconstructor::ReadRecData(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);
	std::vector<double> retVect;

	const T_NodeInfo &info = CheckOutInfo(nodeId);

	uint32_t bufSize = info.nMeas * info.vectLen;
	retVect.reserve(bufSize);
	info.outBufPtr->ReadBuf(retVect.data(), bufSize);
	return retVect;
}

void Reconstructor::SetRanMat(Ptr<RandomMatrix> ranMat_ptr)
{
	m_ranMat = ranMat_ptr;
}

void Reconstructor::SetTransMat(Ptr<TransMatrix<double>> transMat_ptr)
{
	m_transMat = transMat_ptr;
}

void Reconstructor::ResetBuf(T_NodeIdTag nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo &info = CheckOutInfo(nodeId);
	info.inBufPtr->Reset();
	info.outBufPtr->Reset();
}

uint32_t Reconstructor::GetNofMeas(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo info = CheckOutInfo(nodeId);
	return info.m;
}

	uint32_t Reconstructor::GetNofNodes() const
	{
		NS_LOG_FUNCTION(this);
		return m_nNodes;
	}

	Mat<double> Reconstructor::GetBufMat(T_NodeIdTag nodeId) const
	{
		NS_LOG_FUNCTION(this << nodeId);

		const T_NodeInfo &info = CheckOutInfo(nodeId);

		return info.inBufPtr->ReadAll();
}

void Reconstructor::WriteRecBuf(T_NodeIdTag nodeId, const Mat<double> &mat)
{
	NS_LOG_FUNCTION(this << nodeId << mat);
	T_NodeInfo info = CheckOutInfo(nodeId);
	info.outBufPtr->WriteAll(mat);
}

kl1p::TMatrixOperator<double> Reconstructor::GetMatOp(T_NodeIdTag nodeId, bool norm)
{
	NS_LOG_FUNCTION(this << nodeId << norm);
	T_NodeInfo info = CheckOutInfo(nodeId);
	Mat<double> Phi = GetSensMat(info.seed, info.m, info.nMeas, norm);
	if (m_transMat)
	{
		Mat<double> Psi = GetTransMat(info.nMeas);
		Phi = Phi * Psi;
	}
	// Phi.save("rmat" + std::to_string(nodeId), arma_ascii);
	return kl1p::TMatrixOperator<double>(Phi);
}

Mat<double> Reconstructor::GetTransMat(uint32_t n)
{
	NS_LOG_FUNCTION(this << n);
	return Mat<double>();
}

Mat<double> Reconstructor::GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm)
{
	NS_LOG_FUNCTION(this << seed << m << n << norm);
	NS_ASSERT_MSG(m_ranMat, "No sensing Matrix! To create an object use CreateObject!");

	m_ranMat->SetSize(m, n);
	m_ranMat->Generate(seed);
	if (norm)
		m_ranMat->Normalize();
	return *m_ranMat;
}

Reconstructor::T_NodeInfo &Reconstructor::CheckOutInfo(T_NodeIdTag nodeId)
{	
	NS_LOG_FUNCTION(this << nodeId);
	typedef std::map<T_NodeIdTag, T_NodeInfo> T_InfoMap;

	if ((nodeId != m_nodeIdNow) || !m_infoNow) // check out new when new nodeId or nothing checked out before
	{
		NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");
		T_InfoMap::iterator it = m_nodeInfoMap.find(nodeId);
		m_infoNow = &((*it).second);
		m_nodeIdNow = nodeId;
	}
	return *(m_infoNow);
}
const Reconstructor::T_NodeInfo &Reconstructor::CheckOutInfo(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);
	typedef std::map<T_NodeIdTag, T_NodeInfo> T_InfoMap;

	if ((nodeId != m_nodeIdNowConst) || !m_infoNowConst) // check out new when new nodeId or nothing checked out before
	{
		NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");
		T_InfoMap::const_iterator it = m_nodeInfoMap.find(nodeId);
		m_infoNowConst = &((*it).second);
		m_nodeIdNowConst = nodeId;
	}
	return *(m_infoNowConst);
}
