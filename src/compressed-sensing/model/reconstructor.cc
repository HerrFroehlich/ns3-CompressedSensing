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
										  MakePointerAccessor(&Reconstructor::SetRecMatSpat),
										  MakePointerChecker<RecMatrix>())
							.AddAttribute("RecMatTemp", "RecMatrix  for temporal reconstruction",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(Create<RecMatrix, Ptr<RandomMatrix>>(CreateObject<IdentRandomMatrix>())),
										  MakePointerAccessor(&Reconstructor::SetRecMatTemp),
										  MakePointerChecker<RecMatrix>())
							.AddAttribute("CalcSnr", "Calculate the SNR instead of saving reconstructed measurement vectors?",
										  BooleanValue(false),
										  MakeBooleanAccessor(&Reconstructor::m_calcSnr),
										  MakeBooleanChecker())
							.AddAttribute("NoRecTemp", "Switch off temporal reconstruction?",
										  BooleanValue(false),
										  MakeBooleanAccessor(&Reconstructor::m_noRecTemp),
										  MakeBooleanChecker())
							.AddAttribute("NoNC", "Switch off pseudo normalization of the NC matrix?",
										  BooleanValue(false),
										  MakeBooleanAccessor(&Reconstructor::m_noNc),
										  MakeBooleanChecker())
							.AddAttribute("JointTransform", "Use a joint transformation?",
										  BooleanValue(true),
										  MakeBooleanAccessor(&Reconstructor::m_jointTrans),
										  MakeBooleanChecker());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Reconstructor::Reconstructor() : m_seq(0), m_calcSnr(false), m_nClusters(0),
								 m_ncMatrix(new NcMatrix(CsClusterHeader::GetNcInfoSize())),
								 m_jointTrans(true)
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

	//adjusting Input buffer
	uint32_t cols = m_inBuf.GetRowLen(),
			 colsNow = cluster->GetCompression(CsCluster::E_COMPR_DIMS::m);
	if (cols < colsNow)
		m_inBuf.SetRowLen(colsNow);

	m_nClusters++;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::WriteData(const double *buffer, const uint32_t bufSize,
							  const CsClusterHeader::T_NcInfoField &ncCoeff)
{
	NS_LOG_FUNCTION(this << buffer << bufSize << &ncCoeff);
	uint32_t rowLen = m_inBuf.GetRowLen();
	NS_ASSERT_MSG(!(bufSize > rowLen), "Buffer is larger than a row of U!");
	if (bufSize < rowLen)
	{
		NS_LOG_WARN("Incomplete row, filling with zeros!");
		double buf[rowLen];
		std::copy(buffer, buffer + bufSize, buf);
		for (uint32_t i = 0; i < rowLen - bufSize; i++)
			*(buf + bufSize + i) = 0.0;
		m_inBuf.WriteRow(buf, rowLen);
	}
	else
		m_inBuf.WriteRow(buffer, bufSize);

	m_ncMatrix->WriteRow(ncCoeff);
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

void Reconstructor::Reset(uint32_t seq)
{
	NS_LOG_FUNCTION(this);
	m_seq = seq;
	for (auto &entry : m_clusterInfoMap)
	{
		ClusterInfo &info = entry.second;
		info.AddNewStreams(m_seq);
		m_inBuf.Reset();
		m_ncMatrix->Reset();
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

	//get phi
	m_ranMatSpat->SetSize(info.l, info.nNodes, info.clSeed);
	klab::TSmartPointer<kl1p::TOperator<double>> Phi = m_ranMatSpat->Clone();

	//get B
	klab::TSmartPointer<kl1p::TOperator<double>> B = info.precode;

	//Get psi if valied and unique transformation
	if (m_transMatSpat.isValid() && !m_jointTrans)
	{
		m_transMatSpat->SetSize(info.nNodes);
		klab::TSmartPointer<kl1p::TOperator<double>> Psi = m_transMatSpat->Clone();
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
	if (!m_jointTrans && m_transMatSpat.isValid()) // since the reconstruction only gives the indices of the transform we have to apply it again!
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

		if (m_calcSnr)
			CalcSnr(info.clStream, GetY0(info), res);
		else
			WriteStream(info.clStream, res);
	}
	else
	{
		info.spatRecBuf->Write(mat);
		if (m_calcSnr)
			CalcSnr(info.clStream, GetY0(info), mat);
		else
			WriteStream(info.clStream, mat);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::WriteRecTemp(Ptr<DataStream<double>> stream, const Col<double> &vec, Ptr<DataStream<double>> streamX)
{
	if (m_transMatTemp.isValid()) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		Col<double> res;
		res.set_size(vec.n_rows);
		m_transMatTemp->SetSize(vec.n_rows);

		m_transMatTemp->apply(vec, res);

		if (m_calcSnr)
			CalcSnr(stream, GetX0(streamX, vec.n_rows), res);
		else
			WriteStream(stream, res);
	}
	else
	{
		if (m_calcSnr)
			CalcSnr(stream, GetX0(streamX, vec.n_rows), vec);
		else
			WriteStream(stream, vec);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructSpat()
{
	NS_LOG_FUNCTION(this);

	//get N
	klab::TSmartPointer<kl1p::TOperator<double>> N = m_ncMatrix;
	if (!m_noNc)
		N = new kl1p::TScalingOperator<double>(N, 1.0 / klab::Sqrt(N->m()));

	//get operator array
	kl1p::TBlockDiagonalOperator<double>::TOperatorArray blockA;
	blockA.reserve(m_nClusters);
	for (auto const &entry : m_clusterInfoMap)
	{
		ClusterInfo info = entry.second;
		blockA.push_back(GetASpat(info));
	}

	//sensing block matrix A
	klab::TSmartPointer<TOperator<double>> A = new TBlockDiagonalOperator<double>(blockA);
	NS_ASSERT_MSG(N->n() == A->m(), "NC matrix and sensing block matrix are not matching sizes! Have you added all clusters?");

	//stored input data
	Mat<double> U;
	m_inBuf.GetMatrix(U);

	if (!m_noNc)
		U /= klab::Sqrt(N->m()); //since we scaled N before

	Mat<double> Y;
	klab::TSmartPointer<kl1p::TOperator<double>> H;
	if (m_jointTrans && m_transMatSpat.isValid()) // reconstruct jointly with joint transformation
	{
		klab::TSmartPointer<kl1p::TOperator<double>> Psi;
		m_transMatSpat->SetSize(A->n());
		Psi = m_transMatSpat;

		//calculate the sensing matrix
		Mat<double> Hmat; 
		H = N * A * Psi;
		H->toMatrix(Hmat);
		H = new kl1p::TMatrixOperator<double>(Hmat);

		Y = m_algoSpat->Run(U, H);
	}
	else // reconstruct jointly with single transformation, A will already contain them
	{
		Mat<double> Hmat;
		H = N * A;
		H->toMatrix(Hmat);
		H = new kl1p::TMatrixOperator<double>(Hmat);
	}
		Y = m_algoSpat->Run(U, H);

	uint32_t idxL = 0;
	if (m_jointTrans && m_transMatSpat.isValid()) // since the reconstruction only gives the indices of the transform we have to apply it again!
	{
		for (uint32_t i = 0; i < Y.n_cols; i++)
		{
			Col<double> col;
			m_transMatSpat->apply(Y.col(i), col);
			Y.col(i) = col;
		}
	}

	for (auto const &entry : m_clusterInfoMap)
	{
		const ClusterInfo &info = entry.second;

		uint32_t idxU = idxL + info.nNodes - 1;
		WriteRecSpat(info, Y.rows(idxL, idxU));
		idxL += info.nNodes;
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

		Col<double> yi = Y.row(i).t();
		Col<double> xi = m_algoTemp->Run(yi, A); //legit conversion to Col since when input a Col only

		Ptr<DataStream<double>> stream = info.streams.at(i);
		Ptr<DataStream<double>> streamX = info.streamsXin.at(i);
		WriteRecTemp(stream, xi, streamX);
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::ReconstructAll()
{
	NS_LOG_FUNCTION(this);

	ReconstructSpat();
	if (!m_noRecTemp)
	{
		for (auto const &entry : m_clusterInfoMap)
		{
			ReconstructTemp(entry.second);
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void Reconstructor::CalcSnr(Ptr<DataStream<double>> stream, const Mat<double> &x0, const Mat<double> &xr)
{
	double snr = klab::SNR(x0, xr);
	stream->CreateBuffer(&snr, 1);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Mat<double> Reconstructor::GetY0(const ClusterInfo &info)
{
	uint32_t nNodes = info.nNodes,
			 m = info.m;
	double *data = new double[nNodes * m];

	uint32_t writeIdx = 0;
	for (auto it = info.cluster->Begin(); it != info.cluster->End(); it++)
	{
		Ptr<DataStream<double>> stream = (*it)->GetStreamByName(CsNode::STREAMNAME_COMPR);

		NS_ASSERT_MSG(stream->GetN(), "Stream has no buffers left!");

		double dataRow[m] = {0.0};
		Ptr<SerialDataBuffer<double>> buffer = stream->PeekBuffer(m_seq); // Get according to run number

		buffer->Read(0, dataRow, m);

		std::copy(dataRow, dataRow + m, data + writeIdx);

		writeIdx += m;
	}

	Mat<double> Yt(data, m, nNodes); //transposed version since we need yi row by row, not column by column
	delete[] data;
	return Yt.t();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Col<double> Reconstructor::GetX0(Ptr<DataStream<double>> stream, uint32_t n)
{
	NS_ASSERT_MSG(stream->GetN(), "Stream has no buffers left!");

	Ptr<SerialDataBuffer<double>> buf = stream->PeekBuffer(m_seq); // Get according to run number
	Col<double> x(buf->GetMem(), n);

	return x;
}