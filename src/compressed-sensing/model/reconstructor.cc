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
	static TypeId tid = TypeId("Reconstructor")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Reconstructor::m_ranMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of the transformation in which the solution is sparse",
										  PointerValue(),
										  MakePointerAccessor(&Reconstructor::m_transMat),
										  MakePointerChecker<RandomMatrix>());
	return tid;
}

Reconstructor::Reconstructor() : m_nMeasDef(0),
								 m_mMaxDef(0),
								 m_vecLenDef(0),
								 m_nodeIdNow(0),
								 m_infoNow()
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

uint32_t Reconstructor::WriteData(T_NodeIdTag nodeId, const double *buffer, const uint32_t &bufSize)
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

Mat<cx_double> Reconstructor::GetBufMat(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	const T_NodeInfo &info = CheckOutInfo(nodeId);

	return conv_to<cx_mat>::from(info.inBufPtr->ReadAll());
}

void Reconstructor::WriteRecBuf(T_NodeIdTag nodeId, const Mat<double> &mat)
{
	NS_LOG_FUNCTION(this << nodeId << mat);

	T_NodeInfo info = CheckOutInfo(nodeId);
	info.outBufPtr->WriteAll(mat);
}

kl1p::TMatrixOperator<cx_double> Reconstructor::GetMatOp(T_NodeIdTag nodeId, bool norm)
{
	NS_LOG_FUNCTION(this << nodeId << norm);

	Mat<cx_double> ret;
	T_NodeInfo info = CheckOutInfo(nodeId);
	Mat<double> Phi = GetSensMat(info.seed, info.m, info.nMeas, norm);
	if (m_transMat)
	{
		Mat<cx_double> Psi = GetTransMat(info.nMeas);
		ret = Phi * Psi;
	}
	else
	{
		ret = Mat<cx_double>(Phi, zeros<Mat<double>>(info.m, info.nMeas));
	}

	return kl1p::TMatrixOperator<cx_double>(ret);
}

Mat<cx_double> Reconstructor::GetTransMat(uint32_t n)
{
	NS_LOG_FUNCTION(this << n);
	return Mat<cx_double>();
}

Mat<double> Reconstructor::GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm)
{
	NS_LOG_FUNCTION(this << seed << m << n << norm);

	m_ranMat->SetSize(m, n);
	m_ranMat->Generate(seed);
	if (norm)
		m_ranMat->Normalize();
	return *m_ranMat;
}

Reconstructor::T_NodeInfo &Reconstructor::CheckOutInfo(T_NodeIdTag nodeId)
{
	return const_cast<T_NodeInfo &>(static_cast<const Reconstructor&> (*this).CheckOutInfo(nodeId));
}
const Reconstructor::T_NodeInfo &Reconstructor::CheckOutInfo(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo &retInfo = *m_infoNow;
	if ((nodeId != nodeId) || !m_infoNow) // check out new when new nodeId or nothing checked out before
	{
		NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");
		retInfo = m_nodeInfoMap.at(nodeId);
		m_infoNow = &retInfo;
	}
	return retInfo;
}