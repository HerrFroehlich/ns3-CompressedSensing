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
NS_OBJECT_ENSURE_REGISTERED(RecMatrix);

const std::string Reconstructor::STREAMNAME = "RecSeq";

/*-----------------------------------------------------------------------------------------------------------------------*/

Reconstructor::Reconstructor() : m_runNmb(0),  m_nClusters(0)
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

void Reconstructor::SetRecMatSpat(Ptr<RecMatrix> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_ranMatSpat = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatSpat = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::SetRecMatTemp(Ptr<RecMatrix> recMat)
{
	NS_ASSERT_MSG(recMat->ranMatrix, "Needs a valid random matrix!");

	m_ranMatTemp = recMat->ranMatrix->Clone();
	if (recMat->transMatrix)
		m_transMatTemp = recMat->transMatrix->Clone();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::WriteStream(Ptr<DataStream<double>> stream, const Mat<double> &mat)
{
	stream->CreateBuffer(mat.memptr(), mat.n_elem);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::WriteRecSpat(const ClusterInfo &info, const Mat<double> &mat)
{
	if (m_transMatSpat.isValid()) // since the reconstruction only gives the indices of the transform we have to apply it again!
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
		WriteStream(info.clStream, res);
	}
	else
	{
		info.spatRecBuf->Write(mat);
		WriteStream(info.clStream, mat);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::WriteRecTemp(Ptr<DataStream<double>> stream, const Mat<double> &mat)
{
	if (m_transMatTemp.isValid()) // since the reconstruction only gives the indices of the transform we have to apply it again!
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
		WriteStream(stream, res);
	}
	else
		WriteStream(stream, mat);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructSpat()
{
	NS_LOG_FUNCTION(this);
	/*for now we reconstructe each cluster separetly,
	* later we will do that jointly*/
	for (auto const &entry : m_clusterInfoMap)
	{
		ClusterInfo info = entry.second;

		klab::TSmartPointer<kl1p::TOperator<double>> A = GetASpat(info);
		Mat<double> Z = info.inBuf->ReadAll();

		Mat<double> Y = m_algoSpat->Run(Z, A);

		WriteRecSpat(info, Y);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructTemp(const Reconstructor::ClusterInfo &info)
{
	NS_LOG_FUNCTION(this << &info);

	Ptr<CsCluster> cluster = info.cluster;
	const Mat<double> &Y = info.spatRecBuf->Read();

	std::vector<uint32_t> seeds = cluster->GetSeeds();
	for (uint32_t i = 0; i < info.nNodes; i++)
	{
		uint32_t seed = seeds.at(i);
		klab::TSmartPointer<kl1p::TOperator<double>> A = GetATemp(seed, info.m, info.n);

		Mat<double> yi = Y.row(i).t();
		Mat<double> xi = m_algoTemp->Run(yi, A);

		Ptr<DataStream<double>> stream = info.streams.at(i);
		WriteRecTemp(stream, xi);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructAll()
{
	NS_LOG_FUNCTION(this);

	ReconstructSpat();

	for (auto const &entry : m_clusterInfoMap)
	{
		ReconstructTemp(entry.second);
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
										  PointerValue(Create<RecMatrix, Ptr<RandomMatrix>>(CreateObject<GaussianRandomMatrix>())),
										  MakePointerAccessor((&Reconstructor::SetRecMatSpat)),
										  MakePointerChecker<RecMatrix>())
							.AddAttribute("RecMatTemp", "RecMatrix  for temporal reconstruction",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(Create<RecMatrix, Ptr<RandomMatrix>>(CreateObject<IdentRandomMatrix>())),
										  MakePointerAccessor((&Reconstructor::SetRecMatTemp)),
										  MakePointerChecker<RecMatrix>());
	return tid;
}