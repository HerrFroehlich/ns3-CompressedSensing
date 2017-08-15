/**
* \file Reconstructor.cc
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
NS_OBJECT_ENSURE_REGISTERED(RecMatrixReal);
NS_OBJECT_ENSURE_REGISTERED(RecMatrixCx);

const std::string Reconstructor::STREAMNAME = "RecSeq";

/*-----------------------------------------------------------------------------------------------------------------------*/

Reconstructor::Reconstructor() : m_runNmb(0), m_cxTransTemp(false), m_cxTransSpat(false), m_nClusters(0)
{

	NS_LOG_FUNCTION(this);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::AddCluster(Ptr<CsCluster> cluster)
{
	NS_LOG_FUNCTION(this << cluster);

	uint32_t id = cluster->GetClusterId();
	NS_ASSERT_MSG(!m_clusterInfoMap.count(id), "Cluster with that ID was already added!");

	m_clusterInfoMap.emplace(id, cluster);

	m_nClusters++;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t Reconstructor::WriteData(CsHeader::T_IdField clusterId, const double *buffer, const uint32_t bufSize)
{
	NS_LOG_FUNCTION(this << clusterId << buffer << bufSize);

	ClusterInfo info = m_clusterInfoMap.at(clusterId);
	Ptr<T_InBuffer> nodeBuf = info.inBuf;
	uint32_t space = nodeBuf->WriteData(buffer, bufSize);

	return space;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t Reconstructor::WriteData(CsHeader::T_IdField clusterId, const std::vector<double> &vec)
{
	NS_LOG_FUNCTION(this << clusterId << &vec);

	return WriteData(clusterId, vec.data(), vec.size());
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetPrecodeEntries(CsHeader::T_IdField clusterId, const std::vector<bool> &entries)
{
	NS_LOG_FUNCTION(this << clusterId << &entries);

	ClusterInfo info = m_clusterInfoMap.at(clusterId);
	std::vector<bool> relevant(entries.begin(), entries.begin() + info.nNodes);
	info.precode->SetDiag(relevant);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::Reset()
{
	NS_LOG_FUNCTION(this);
	m_runNmb++;
	for (auto &entry : m_clusterInfoMap)
	{
		ClusterInfo &info = entry.second;
		info.AddNewStreams(m_runNmb);
		info.inBuf->Reset();
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetAlgorithmTemp(Ptr<CsAlgorithm> algo)
{
	NS_LOG_FUNCTION(this << algo);
	NS_ASSERT_MSG(algo, "Invalid CsAlgorithm!"); //null pointer check
	m_algoTemp = algo;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetAlgorithmSpat(Ptr<CsAlgorithm> algo)
{
	NS_LOG_FUNCTION(this << algo);
	NS_ASSERT_MSG(algo, "Invalid CsAlgorithm!"); //null pointer check
	m_algoSpat = algo;
}
/*-----------------------------------------------------------------------------------------------------------------------*/

Ptr<CsAlgorithm> Reconstructor::GetAlgorithmTemp() const
{
	return m_algoTemp;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Ptr<CsAlgorithm> Reconstructor::GetAlgorithmSpat() const
{
	return m_algoSpat;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetRecMatSpat(Ptr<RecMatrixReal> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_cxTransSpat = false;

	m_ranMatSpat = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatSpat = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetRecMatSpat(Ptr<RecMatrixCx> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_cxTransSpat = true;

	m_ranMatSpat = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatSpatCx = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetRecMatTemp(Ptr<RecMatrixReal> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_cxTransTemp = false;

	m_ranMatTemp = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatTemp = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetRecMatTemp(Ptr<RecMatrixCx> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_cxTransTemp = true;

	m_ranMatTemp = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatTempCx = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor::Reconstructor::GetASpat(const Reconstructor::ClusterInfo &info)
*/
template <>
klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor::GetASpat(const Reconstructor::ClusterInfo &info)
{
	NS_LOG_FUNCTION(this << &info);

	uint32_t nMeas = info.inBuf->GetWrRow();
	//get phi
	m_ranMatSpat->SetSize(nMeas, info.nNodes, info.clSeed);
	klab::TSmartPointer<kl1p::TOperator<double>> Phi = m_ranMatSpat;

	//get B
	klab::TSmartPointer<kl1p::TOperator<double>> B = info.precode;

	//Get psi if valied
	if (m_transMatSpat.isValid())
	{
		m_transMatSpat->SetSize(info.nNodes);
		klab::TSmartPointer<kl1p::TOperator<double>> Psi = m_transMatSpat;
		return Phi * B * Psi;
	}
	return Phi * B;
}
//template klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor::GetASpat(const Reconstructor::ClusterInfo &);

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor::Reconstructor::GetASpat(const Reconstructor::ClusterInfo &info)
*/
template <>
klab::TSmartPointer<kl1p::TOperator<cx_double>> Reconstructor::GetASpat(const Reconstructor::ClusterInfo &info)
{
	NS_LOG_FUNCTION(this << &info);

	//get phi
	m_ranMatSpat->SetSize(info.l, info.nNodes, info.clSeed);
	klab::TSmartPointer<kl1p::TOperator<cx_double>> Phi = klab::TSmartPointer<kl1p::TOperator<cx_double>>(*m_ranMatSpat);

	//get B
	klab::TSmartPointer<kl1p::TOperator<cx_double>> B = klab::TSmartPointer<kl1p::TOperator<cx_double>>(*info.precode);

	//Get psi if valied
	if (m_transMatSpat.isValid())
	{
		m_transMatSpatCx->SetSize(info.nNodes);
		klab::TSmartPointer<kl1p::TOperator<cx_double>> Psi = m_transMatSpatCx;
		return Phi * B * Psi;
	}
	return Phi * B;
}
//template klab::TSmartPointer<kl1p::TOperator<cx_double>> Reconstructor::GetASpat(const Reconstructor::ClusterInfo &);

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor::Reconstructor::GetATemp(const Reconstructor::ClusterInfo &info)
*/
template <>
klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor::GetATemp(uint32_t seed, uint32_t m, uint32_t n)
{
	NS_LOG_FUNCTION(this << seed << m << n);

	//get phi
	m_ranMatTemp->SetSize(m, n, seed);
	klab::TSmartPointer<kl1p::TOperator<double>> Phi = m_ranMatTemp;

	//Get psi if valied
	if (m_transMatTemp.isValid())
	{
		m_transMatTemp->SetSize(n);
		klab::TSmartPointer<kl1p::TOperator<double>> Psi = m_transMatTemp;
		return Phi * Psi;
	}
	return Phi;
}
//template klab::TSmartPointer<kl1p::TOperator<double>> Reconstructor::GetATemp(uint32_t seed, uint32_t m, uint32_t n);

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc klab::TSmartPointer<kl1p::TOperator<T>> Reconstructor::Reconstructor::GetATemp(const Reconstructor::ClusterInfo &info)
*/
template <>
klab::TSmartPointer<kl1p::TOperator<cx_double>> Reconstructor::GetATemp(uint32_t seed, uint32_t m, uint32_t n)
{
	NS_LOG_FUNCTION(this << seed << m << n);

	//get phi
	m_ranMatTemp->SetSize(m, n, seed);
	klab::TSmartPointer<kl1p::TOperator<cx_double>> Phi = klab::TSmartPointer<kl1p::TOperator<cx_double>>(*m_ranMatTemp);

	//Get psi if valid
	if (m_transMatTemp.isValid())
	{
		m_transMatTemp->SetSize(n);
		klab::TSmartPointer<kl1p::TOperator<cx_double>> Psi = m_transMatTempCx;
		return Phi * Psi;
	}
	return Phi;
}
//template klab::TSmartPointer<kl1p::TOperator<cx_double>> Reconstructor::GetATemp(uint32_t seed, uint32_t m, uint32_t n);

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc Reconstructor::WriteStream(Ptr<DataStream<double>> stream, const Mat<T> &mat)
*/
template <>
void Reconstructor::WriteStream(Ptr<DataStream<double>> stream, const Mat<double> &mat)
{
	stream->CreateBuffer(mat.memptr(), mat.n_elem);
}
//template void Reconstructor::WriteStream(Ptr<DataStream<double>>, const Mat<double> &mat);

/**
* \private
* \copydoc Reconstructor::WriteStream(Ptr<DataStream<double>> stream, const Mat<T> &mat)
*/
template <>
void Reconstructor::WriteStream(Ptr<DataStream<double>> stream, const Mat<cx_double> &mat)
{
	Mat<double> matReal = real(mat);
	stream->CreateBuffer(matReal.memptr(), matReal.n_elem);
}
//template void Reconstructor::WriteStream(Ptr<DataStream<double>>, const Mat<cx_double> &mat);
/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc Reconstructor::WriteRecSpat(const ClusterInfo &, const Mat<T> &)
*/
template <>
void Reconstructor::WriteRecSpat(const ClusterInfo &info, const Mat<double> &mat)
{
	if (m_transMatSpat.isValid() && !m_cxTransSpat) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		m_transMatSpat->SetSize(info.nNodes);
		Mat<double> res;
		res.set_size(mat.n_rows, mat.n_cols);

		for (size_t i = 0; i < mat.n_cols; i++)
		{
			Col<double> xVec;
			m_transMatSpat->apply(mat.col(i), xVec);
			res.col(i) = xVec;
		}

		info.spatRecBuf->Write(res);
		WriteStream<double>(info.clStream, res);
	}
	else
	{
		info.spatRecBuf->Write(mat);
		WriteStream<double>(info.clStream, mat);
	}
}
//template void Reconstructor::WriteRecSpat(const ClusterInfo &, const Mat<double> &);

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc Reconstructor::WriteRecSpat(const ClusterInfo &, const Mat<T> &)
*/
template <>
void Reconstructor::WriteRecSpat(const ClusterInfo &info, const Mat<cx_double> &mat)
{

	if (m_transMatSpat.isValid() && m_cxTransSpat) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		m_transMatSpatCx->SetSize(info.nNodes);
		Mat<cx_double> res;
		res.set_size(mat.n_rows, mat.n_cols);

		for (size_t i = 0; i < mat.n_cols; i++)
		{
			Col<cx_double> xVec;
			m_transMatSpatCx->apply(mat.col(i), xVec);
			res.col(i) = xVec;
		}

		info.spatRecBuf->Write(real(res));
		WriteStream<cx_double>(info.clStream, res);
	}
	else
	{
		info.spatRecBuf->Write(real(mat));
		WriteStream<cx_double>(info.clStream, mat);
	}
}
//template void Reconstructor::WriteRecSpat(const ClusterInfo &, const Mat<cx_double> &); /**< \copydoc template < T> Reconstructor::WriteRecSpat(const ClusterInfo &, const Mat<T> &)*/

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc Reconstructor::WriteRecTemp(Ptr<DataStream<double>>, const Mat<T> &)
*/
template <>
void Reconstructor::WriteRecTemp(Ptr<DataStream<double>> stream, const Mat<double> &mat)
{
	if (m_transMatTemp.isValid() && !m_cxTransTemp) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		Mat<double> res;
		res.set_size(mat.n_rows, mat.n_cols);
		m_transMatTemp->SetSize(mat.n_rows);

		for (size_t i = 0; i < mat.n_cols; i++)
		{
			Col<double> xVec;
			m_transMatTemp->apply(mat.col(i), xVec);
			res.col(i) = xVec;
		}
		WriteStream<double>(stream, res);
	}
	else
		WriteStream<double>(stream, mat);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

/**
* \private
* \copydoc Reconstructor::WriteRecTemp(Ptr<DataStream<double>>, const Mat<T> &)
*/
template <>
void Reconstructor::WriteRecTemp(Ptr<DataStream<double>> stream, const Mat<cx_double> &mat)
{
	if (m_transMatTempCx.isValid() && m_cxTransTemp) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		Mat<cx_double> res;
		res.set_size(mat.n_rows, mat.n_cols);
		m_transMatTempCx->SetSize(mat.n_rows);

		for (size_t i = 0; i < mat.n_cols; i++)
		{
			Col<cx_double> xVec;
			m_transMatTempCx->apply(mat.col(i), xVec);
			res.col(i) = xVec;
		}
		WriteStream<cx_double>(stream, res);
	}
	else
		WriteStream<cx_double>(stream, mat);
}
/*-----------------------------------------------------------------------------------------------------------------------*/

template <typename T>
void Reconstructor::ReconstructSpat()
{
	NS_LOG_FUNCTION(this);
	/*for now we reconstructe each cluster separetly,
	* later we will do that jointly*/
	for (auto const &entry : m_clusterInfoMap)
	{
		ClusterInfo info = entry.second;

		klab::TSmartPointer<kl1p::TOperator<T>> A = GetASpat<T>(info);
		Mat<double> Z = info.inBuf->ReadAll();
		
		Mat<T> Y = m_algoSpat->Run(Z, A);

		WriteRecSpat<T>(info, Y);
	}
}
template void Reconstructor::Reconstructor::ReconstructSpat();			  /**< \copydoc Reconstructor:ReconstructSpat()*/
template void Reconstructor::Reconstructor::ReconstructSpat<cx_double>(); /**< \copydoc Reconstructor:ReconstructSpat()*/

/*-----------------------------------------------------------------------------------------------------------------------*/
/**
* \private
* \copydoc template<T> Reconstructor::ReconstructTemp
*/
template <typename T>
void Reconstructor::ReconstructTemp(const Reconstructor::ClusterInfo &info)
{
	NS_LOG_FUNCTION(this << &info);

	Ptr<CsCluster> cluster = info.cluster;
	const Mat<double> &Y = info.spatRecBuf->Read();

	std::vector<uint32_t> seeds = cluster->GetSeeds();
	for (uint32_t i = 0; i < info.nNodes; i++)
	{
		uint32_t seed = seeds.at(i);
		klab::TSmartPointer<kl1p::TOperator<T>> A = GetATemp<T>(seed, info.m, info.n);

		Mat<double> yi = Y.row(i).t();
		Mat<T> xi = m_algoTemp->Run(yi, A);

		Ptr<DataStream<double>> stream = info.streams.at(i);
		WriteRecTemp(stream, xi);
	}
}
template void Reconstructor::Reconstructor::ReconstructTemp(const Reconstructor::ClusterInfo &);
template void Reconstructor::ReconstructTemp<cx_double>(const Reconstructor::ClusterInfo &);

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructAll()
{
	NS_LOG_FUNCTION(this);

	if (m_cxTransSpat)
		ReconstructSpat<cx_double>();
	else
		ReconstructSpat();

	if (m_cxTransTemp)
	{
		for (auto const &entry : m_clusterInfoMap)
		{
			ReconstructTemp<cx_double>(entry.second);
		}
	}
	else
	{
		for (auto const &entry : m_clusterInfoMap)
		{
			ReconstructTemp(entry.second);
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId Reconstructor::GetTypeId(void)
{
	// void (*setRealSpat)(const RecMatrix<double> &) = &Reconstructor::SetRecMatSpat;
	// void (*setCxSpat)(const RecMatrixCx &) = &Reconstructor::SetRecMatSpat;
	static TypeId tid = TypeId("Reconstructor")
							.SetParent<Object>()
							.AddConstructor<Reconstructor>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("AlgoTemp", "The CsAlgorithm used to reconstruct temporally.",
										  PointerValue(CreateObject<CsAlgorithm_OMP>()),
										  MakePointerAccessor(&Reconstructor::SetAlgorithmTemp, &Reconstructor::GetAlgorithmTemp),
										  MakePointerChecker<CsAlgorithm>())
							.AddAttribute("AlgoSpat", "The CsAlgorithm used to reconstruct spatially",
										  PointerValue(CreateObject<CsAlgorithm_OMP>()),
										  MakePointerAccessor(&Reconstructor::SetAlgorithmSpat, &Reconstructor::GetAlgorithmSpat),
										  MakePointerChecker<CsAlgorithm>())
							.AddAttribute("RecMatSpat", "RecMatrix  for spatial reconstruction",
										  PointerValue(Create<RecMatrixReal, Ptr<RandomMatrix>>(CreateObject<GaussianRandomMatrix>())),
										  MakePointerAccessor(static_cast<void (Reconstructor::*)(Ptr<RecMatrixReal>)>(&Reconstructor::SetRecMatSpat)),
										  MakePointerChecker<RecMatrixReal>())
							.AddAttribute("RecMatTemp", "RecMatrix  for temporal reconstruction",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(Create<RecMatrixReal, Ptr<RandomMatrix>>(CreateObject<IdentRandomMatrix>())),
										  MakePointerAccessor(static_cast<void (Reconstructor::*)(Ptr<RecMatrixReal>)>(&Reconstructor::SetRecMatTemp)),
										  MakePointerChecker<RecMatrixReal>())
							.AddAttribute("RecMatSpatCx", "Complex RecMatrix  for spatial reconstruction",
										  TypeId::ATTR_SET,
										  PointerValue(Create<RecMatrixCx>(CreateObject<GaussianRandomMatrix>())),
										  MakePointerAccessor(static_cast<void (Reconstructor::*)(Ptr<RecMatrixCx>)>(&Reconstructor::SetRecMatSpat)),
										  MakePointerChecker<RecMatrixCx>())
							.AddAttribute("RecMatTempCx", "Complex RecMatrix  for temporal reconstruction",
										  TypeId::ATTR_SET,
										  PointerValue(Create<RecMatrixCx>(CreateObject<IdentRandomMatrix>())),
										  MakePointerAccessor(static_cast<void (Reconstructor::*)(Ptr<RecMatrixCx>)>(&Reconstructor::SetRecMatTemp)),
										  MakePointerChecker<RecMatrixCx>());
	return tid;
}