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

template <typename T>
TypeId Reconstructor<T>::GetTypeId(void)
{
	std::string name = TypeNameGet<Reconstructor<T>>();
	static TypeId tid = TypeId(("ns3::Reconstructor<" + name + ">").c_str())
							.SetParent<Object>()
							//.AddConstructor<Reconstructor>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  TypeId::ATTR_SET,
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Reconstructor::SetRanMat),
										  //  MakePointerAccessor(&Reconstructor::m_ranMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET,
										  PointerValue(),
										  MakePointerAccessor(&Reconstructor::SetTransMat),
										  MakePointerChecker<TransMatrix<T>>())
							// .AddAttribute("nMeas", "Default NOF measurement vectors to reconstruct",
							// 			  UintegerValue(0),
							// 			  MakeUintegerAccessor(&Reconstructor::m_nMeasDef),
							// 			  MakeUintegerChecker<uint32_t>())
							// .AddAttribute("mMax", "Default maximum NOF measurement vectors used for reconstruction",
							// 			  UintegerValue(0),
							// 			  MakeUintegerAccessor(&Reconstructor::m_mMaxDef),
							// 			  MakeUintegerChecker<uint32_t>())
							// .AddAttribute("vecLen", "Default length of each measurement vector",
							// 			  UintegerValue(0),
							// 			  MakeUintegerAccessor(&Reconstructor::m_vecLenDef),
							// 			  MakeUintegerChecker<uint32_t>())
							.AddTraceSource("RecComplete", "Callback when Reconstuction completed",
											MakeTraceSourceAccessor(&Reconstructor::m_completeCb),
											"Reconstructor::CompleteTracedCallback")
							.AddTraceSource("RecError", "Callback when Reconstuction failed with an error",
											MakeTraceSourceAccessor(&Reconstructor::m_completeCb),
											"Reconstructor::ErrorTracedCallback");
	return tid;
}

template <typename T>
Reconstructor<T>::Reconstructor() : m_nMeasDef(0),
									m_mMaxDef(0),
									m_vecLenDef(0),
									m_nodeIdNow(0),
									m_nodeIdNowConst(0)

{
}

template <typename T>
int64_t Reconstructor<T>::ReconstructAll()
{
	int64_t time = 0;
	for (auto const &entry : m_nodeInfoMap)
	{
		time += Reconstruct(entry.first);
	}
	return time;
}

template <typename T>
void Reconstructor<T>::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed)
{
	AddSrcNode(nodeId, seed, m_nMeasDef, m_mMaxDef, m_vecLenDef);
}

template <typename T>
void Reconstructor<T>::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nodeId << seed << nMeas << mMax << vecLen);
	NS_ASSERT_MSG(!m_nodeInfoMap.count(nodeId), "Node was already added!");
	NS_ASSERT_MSG(nMeas && mMax && vecLen, "Dimensions must be >0 !");

	//create input input buffer
	Ptr<T_NodeBuffer> nodeBufIn = ns3::CreateObject<T_NodeBuffer>(mMax, vecLen);
	Ptr<T_NodeBuffer> nodeBufOut = ns3::CreateObject<T_NodeBuffer>(nMeas, vecLen);

	T_NodeInfo info(seed, nMeas, 0, vecLen, mMax, nodeBufIn, nodeBufOut);
	m_nodeInfoMap[nodeId] = info;
	m_nNodes++;
}

template <typename T>
void Reconstructor<T>::SetBufferDim(uint32_t nMeas, uint32_t mMax, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << vecLen);
	m_nMeasDef = nMeas;
	m_mMaxDef = mMax;
	m_vecLenDef = vecLen;
}

template <typename T>
uint32_t Reconstructor<T>::WriteData(T_NodeIdTag nodeId, const T *buffer, const uint32_t bufSize)
{
	NS_LOG_FUNCTION(this << nodeId << buffer << bufSize);

	T_NodeInfo &info = CheckOutInfo(nodeId);
	Ptr<T_NodeBuffer> nodeBuf = info.inBufPtr;
	uint32_t space = nodeBuf->WriteData(buffer, bufSize);
	info.m = nodeBuf->GetWrRow();

	return space;
}

template <typename T>
std::vector<T> Reconstructor<T>::ReadRecData(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	const T_NodeInfo &info = CheckOutInfo(nodeId);
	NS_ASSERT_MSG(!info.outBufPtr->IsEmpty(), "No reconstructed data, run Reconstruct() first!");

	uint32_t bufSize = info.nMeas * info.vecLen;
	std::vector<T> retVec(bufSize);
	info.outBufPtr->ReadBuf(retVec.data(), bufSize);

	return retVec;
	
}

template <typename T>
void Reconstructor<T>::SetRanMat(Ptr<RandomMatrix> ranMat_ptr)
{
	m_ranMat = ranMat_ptr->Clone();
}

template <typename T>
void Reconstructor<T>::SetTransMat(Ptr<TransMatrix<T>> transMat_ptr)
{
	m_transMat = transMat_ptr->Clone();
}

template <typename T>
void Reconstructor<T>::ResetFullBuf(T_NodeIdTag nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo &info = CheckOutInfo(nodeId);
	if (info.m == info.mMax)
		info.inBufPtr->Reset();
}

template <typename T>
uint32_t Reconstructor<T>::GetNofMeas(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo info = CheckOutInfo(nodeId);
	return info.m;
}

template <typename T>
uint32_t Reconstructor<T>::GetVecLen(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo info = CheckOutInfo(nodeId);
	return info.vecLen;
}

template <typename T>
uint32_t Reconstructor<T>::GetN(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	T_NodeInfo info = CheckOutInfo(nodeId);
	return info.nMeas;
}

template <typename T>
uint32_t Reconstructor<T>::GetNofNodes() const
{
	NS_LOG_FUNCTION(this);
	return m_nNodes;
}

template <typename T>
Mat<T> Reconstructor<T>::GetBufMat(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);

	const T_NodeInfo &info = CheckOutInfo(nodeId);

	return info.inBufPtr->ReadAll();
}

template <typename T>
void Reconstructor<T>::WriteRecBuf(T_NodeIdTag nodeId, const Mat<T> &mat)
{
	NS_LOG_FUNCTION(this << nodeId << mat);
	
	T_NodeInfo info = CheckOutInfo(nodeId);

	info.outBufPtr->WriteAll(mat);
}

template <typename T>
klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor<T>::GetOp(T_NodeIdTag nodeId, bool norm)
{
	NS_LOG_FUNCTION(this << nodeId << norm);
	T_NodeInfo info = CheckOutInfo(nodeId);
	// Mat<T> Phi = conv_to<Mat<T>>::from(GetSensMat(info.seed, info.m, info.nMeas, norm));
	// if (m_transMat)
	// {
	// 	Mat<T> Psi = GetTransMat(info.nMeas);
	// 	Phi = Phi * Psi;
	// }
	// Phi.save("rmat" + std::to_string(nodeId), arma_ascii);
	// return kl1p::TMatrixOperator<T>(Phi);
	// return  * GetTransMat(n);.
	klab::TSmartPointer<kl1p::TOperator<double>> sensMat_ptr = GetSensMat(info.seed, info.m, info.nMeas, norm);
	return sensMat_ptr * m_transMat;
}

template <typename T>
klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor<T>::GetTransMat(uint32_t n)
{
	NS_LOG_FUNCTION(this << n);
<<<<<<< HEAD
	return m_transMat;
}

template <typename T>
klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor<T>::GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm)
=======
	if (m_transMat.isValid())
		m_transMat->SetSize(n);
	return m_transMat;
}

template <>
klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor<double>::GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm)
>>>>>>> 2ad0564... Cleanup
{
	NS_LOG_FUNCTION(this << seed << m << n << norm);
	if (m_ranMat.isValid())
	{
		m_ranMat->SetSize(m, n, false);
		m_ranMat->Generate(seed,true);
		if (norm)
			m_ranMat->NormalizeToM();
	}
	return m_ranMat;
}

template <typename T>
klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor<T>::GetSensMat(uint32_t seed, uint32_t m, uint32_t n, bool norm)
{
	if (m_ranMat.isValid())
	{
		m_ranMat->SetSize(m, n, false);
		m_ranMat->Generate(seed,true);
		if (norm)
			m_ranMat->NormalizeToM();
	}
	return klab::TSmartPointer<kl1p::TOperator<T>>(*m_ranMat);
}

template <typename T>
typename Reconstructor<T>::T_NodeInfo &Reconstructor<T>::CheckOutInfo(T_NodeIdTag nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);
	typedef std::map<T_NodeIdTag, T_NodeInfo> T_InfoMap;

	if ((nodeId != m_nodeIdNow) || !m_infoNow) // check out new when new nodeId or nothing checked out before
	{
		NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");
		typename T_InfoMap::iterator it = m_nodeInfoMap.find(nodeId);
		m_infoNow = &((*it).second);
		m_nodeIdNow = nodeId;
	}
	return *(m_infoNow);
}

template <typename T>
const typename Reconstructor<T>::T_NodeInfo &Reconstructor<T>::CheckOutInfo(T_NodeIdTag nodeId) const
{
	NS_LOG_FUNCTION(this << nodeId);
	typedef std::map<T_NodeIdTag, T_NodeInfo> T_InfoMap;

	if ((nodeId != m_nodeIdNowConst) || !m_infoNowConst) // check out new when new nodeId or nothing checked out before
	{
		NS_ASSERT_MSG(m_nodeInfoMap.count(nodeId), "Node ID unknown!");
		typename T_InfoMap::const_iterator it = m_nodeInfoMap.find(nodeId);
		m_infoNowConst = &((*it).second);
		m_nodeIdNowConst = nodeId;
	}
	return *(m_infoNowConst);
}

template class Reconstructor<double>;
template class Reconstructor<cx_double>;
