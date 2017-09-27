/**
* \file cs-cluster-header.cc
*
* \author Tobias Waurick
* \date 18.08.17
*
*/

#include "cs-cluster-header.h"
#include <iomanip> //setprecision
#include "assert.h"
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(CsClusterHeader);
NS_LOG_COMPONENT_DEFINE("CsClusterHeader");

bool CsClusterHeader::m_isSetup = false;
uint32_t CsClusterHeader::m_ncInfoSize = 0;
uint32_t CsClusterHeader::m_maxClusters = 0;
std::vector<uint32_t> CsClusterHeader::m_lk;
CsClusterHeader::E_NcCoeffType CsClusterHeader::m_coeffType = CsClusterHeader::E_NcCoeffType::NORMAL;
CsClusterHeader::E_NcCoeffType CsClusterHeader::NcCoeffGenerator::m_coeffType = CsClusterHeader::E_NcCoeffType::NORMAL;

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::CsClusterHeader() : m_ncCount(0), m_srcInfo(m_maxClusters), m_ncInfo(m_ncInfoSize)
{
	CsHeader::SetNodeId(CLUSTER_NODEID);
	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetSrcInfo(const T_SrcInfoField &set, uint32_t clusterId)
{
	NS_ASSERT_MSG(clusterId < m_maxClusters, "Non-valid cluster ID!");
	m_srcInfo.at(clusterId) = set;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::T_SrcInfoField CsClusterHeader::GetSrcInfo(uint32_t clusterId) const
{
	NS_ASSERT_MSG(clusterId < m_maxClusters, "Non-valid cluster ID!");
	return m_srcInfo.at(clusterId);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

bool CsClusterHeader::IsSrcInfoSet(uint32_t clusterId) const
{
	NS_ASSERT_MSG(clusterId < m_maxClusters, "Non-valid cluster ID!");
	return m_srcInfo.at(clusterId).any();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::Setup(const std::vector<uint32_t> &lk, E_NcCoeffType cType)
{
	m_lk = lk;
	m_ncInfoSize = 0;
	for (auto l : lk)
	{
		m_ncInfoSize += l;
	}
	m_maxClusters = m_lk.size();

	m_coeffType = cType;
	NcCoeffGenerator::SetType(cType);

	m_isSetup = true;
}

/*-----------------------------------------------------------------------------------------------------------------------*/
CsClusterHeader::E_NcCoeffType CsClusterHeader::GetNcCoeffType()
{
	return m_coeffType;
}
/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsClusterHeader::GetMaxClusters()
{
	return m_maxClusters;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsClusterHeader::GetNcInfoSize()
{
	return m_ncInfoSize;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetNcInfoNew(T_IdField clusterId, uint32_t i)
{
	NS_ASSERT_MSG(i < m_lk.at(clusterId), "row index i larger than compression l!");

	T_NcInfoField ncInfo(m_ncInfoSize, 0.0);

	uint32_t idx = i;
	for (uint32_t k = 0; k < clusterId; k++)
	{
		idx += m_lk.at(k);
	}

	ncInfo.at(idx) = 1.0;
	SetNcInfo(ncInfo);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetNcInfo(const T_NcInfoField &vec)
{
	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");
	NS_ASSERT_MSG(vec.size() == m_ncInfoSize, "Vector has incorrect size!");

	m_ncInfo = vec;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::T_NcInfoField CsClusterHeader::GetNcInfo() const
{
	NS_ASSERT_MSG(m_isSetup, "Run Setup first!");

	return m_ncInfo;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetNcCount(uint32_t cnt)
{
	m_ncCount = cnt;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::T_NcCountField CsClusterHeader::GetNcCount() const
{
	return m_ncCount;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetNodeId(T_IdField ignored)
{
	(void)ignored;
	CsHeader::SetNodeId(CLUSTER_NODEID);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId CsClusterHeader::GetTypeId()
{
	static TypeId tid = TypeId("CsClusterHeader")
							.SetParent<CsHeader>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<CsClusterHeader>();
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

TypeId CsClusterHeader::GetInstanceTypeId(void) const
{
	return GetTypeId();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsClusterHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator buf = start;

	CsHeader::DoDeserialize(buf);

	//source info
	for (auto &it : m_srcInfo)
	{
		for (uint32_t j = 0; j < SRCINFO_LEN; j++)
		{
			uint8_t byte = buf.ReadU8();
			for (size_t i = 0; i < BYTE_LEN; i++)
			{
				it[i + j * BYTE_LEN] = (1 << i) & byte;
			}
		}
	}
	//nc count
	m_ncCount = buf.ReadU8();

	//nc info
	if (m_coeffType == E_NcCoeffType::NORMAL) //8 byte per coefficient
	{
		uint32_t nBytes = m_ncInfoSize * COEFF_NORM_LEN;
		buf.Read(reinterpret_cast<uint8_t *>(m_ncInfo.data()), nBytes);
	}
	else if (m_coeffType == E_NcCoeffType::BERN) //2bit  per coefficient
	{
		uint32_t nBytes = 1 + (m_ncInfoSize * COEFF_BERN_BITLEN - 1) / BYTE_LEN; //round up

		//iterate over bytes
		for (uint32_t j = 0; j < nBytes - 1; j++)
		{
			uint8_t byte = buf.ReadU8();
			//iterate over one byte beginning at LSB
			for (uint32_t i = 0; i < COEFF_BERN_PER_BYTE; i++)
			{
				double d = ReadBernCoeffFromByte(byte, i);
				m_ncInfo.at(j * COEFF_BERN_PER_BYTE + COEFF_BERN_PER_BYTE - i - 1) = d; //write back as double
			}
		}

		//last byte might contain less than COEFF_BERN_PER_BYTE coefficients
		uint32_t remain = m_ncInfoSize - COEFF_BERN_PER_BYTE * (nBytes - 1);
		if (remain)
		{
			uint8_t byte = buf.ReadU8();
			//iterate over one byte
			for (uint8_t i = 0; i < remain; i++)
			{
				double d = ReadBernCoeffFromByte(byte, COEFF_BERN_PER_BYTE - i - 1);
				m_ncInfo.at((nBytes - 1) * COEFF_BERN_PER_BYTE + i) = d;
			}
			buf.WriteU8(byte);
		}
	}

	return GetSerializedSize();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsClusterHeader::GetSerializedSize() const
{
	uint32_t size = CsHeader::GetSerializedSize();

	size += m_maxClusters * SRCINFO_LEN + sizeof(T_NcCountField);

	if (m_coeffType == E_NcCoeffType::NORMAL) //8 byte per coefficient
	{
		size += m_ncInfoSize * COEFF_NORM_LEN;
	}
	else if (m_coeffType == E_NcCoeffType::BERN) //2bit  per coefficient
	{
		uint32_t nBytes = 1 + (m_ncInfoSize * COEFF_BERN_BITLEN - 1) / BYTE_LEN; //round up
		size += nBytes;
	}

	return size;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::Print(std::ostream &os) const
{

	CsHeader::Print(os);
	// os << "Node Info: " << m_srcInfo << std::endl;
	// os << "NC Count: " << static_cast<int>(m_ncCount) << std::endl;
	// os << "NC Info: "  << std::endl;

	os << "Node Info: ";
	for (const auto &it : m_srcInfo)
	{
		os << it;
	}
	os << std::endl;

	os << "NC Count: " << static_cast<int>(m_ncCount) << std::endl;
	os << "NC Info: " << std::endl;

	uint32_t prec = 2;
	if (m_coeffType == E_NcCoeffType::BERN) //2bit  per coefficient
		prec = 0;
	os << std::fixed << std::setprecision(prec);
	
	uint32_t j = 0;
	auto lk = m_lk.begin();
	for (uint32_t i = 0; i < m_ncInfoSize; i++)
	{
		os << m_ncInfo.at(i) << " ";

		//print a line end for each cluster head
		if (++j >= *lk)
		{
			j = 0;
			lk++;
			os << "|" << std::endl;
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator buf = start;

	CsHeader::DoSerialize(buf);

	//source info

	for (const auto &it : m_srcInfo)
	{
		for (uint32_t j = 0; j < SRCINFO_LEN; j++)
		{
			uint8_t byte = 0;
			for (size_t i = 0; i < BYTE_LEN; i++)
			{
				byte |= (it[i + j * BYTE_LEN] << i);
			}
			buf.WriteU8(byte);
		}
	}

	//nc count
	buf.WriteU8(m_ncCount);

	//nc info

	if (m_coeffType == E_NcCoeffType::NORMAL) //8 byte per coefficient
	{
		uint32_t nBytes = m_ncInfoSize * COEFF_NORM_LEN;
		buf.Write(reinterpret_cast<const uint8_t *>(m_ncInfo.data()), nBytes);
	}
	else if (m_coeffType == E_NcCoeffType::BERN) //2bit  per coefficient
	{

		uint32_t nBytes = 1 + (m_ncInfoSize * COEFF_BERN_BITLEN - 1) / BYTE_LEN; //round up

		//iterate over bytes
		for (uint32_t j = 0; j < nBytes - 1; j++)
		{
			uint8_t byte = 0;
			//iterate over one byte
			for (uint8_t i = 0; i < COEFF_BERN_PER_BYTE; i++)
			{
				double d = m_ncInfo.at(j * COEFF_BERN_PER_BYTE + i);
				byte = WriteBernCoeffToByte(byte, d, i);
			}
			buf.WriteU8(byte);
		}

		//last byte might contain less than COEFF_BERN_PER_BYTE coefficients
		uint32_t remain = m_ncInfoSize - COEFF_BERN_PER_BYTE * (nBytes - 1);
		if (remain)
		{
			uint8_t byte = 0;
			//iterate over one byte
			for (uint8_t i = 0; i < remain; i++)
			{
				double d = m_ncInfo.at((nBytes - 1) * COEFF_BERN_PER_BYTE + i);
				byte = WriteBernCoeffToByte(byte, d, COEFF_BERN_PER_BYTE-1 - i);
			}
			buf.WriteU8(byte);
		}
	}
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint8_t CsClusterHeader::WriteBernCoeffToByte(uint8_t byte, double val, uint8_t pos) const
{

	uint8_t c;
	if (val == 0)
		c = E_COEFF_BERN_VAL::ZERO;
	else if (val > 0)
		c = E_COEFF_BERN_VAL::PLUS_ONE;
	else if (val < 0)
		c = E_COEFF_BERN_VAL::MINUS_ONE;
	else
	{
		NS_LOG_ERROR("Invalid coefficient!");
		c = E_COEFF_BERN_VAL::INVALID;
	}
	byte |= c << (COEFF_BERN_BITLEN * pos);
	return byte;
}
/*-----------------------------------------------------------------------------------------------------------------------*/

double CsClusterHeader::ReadBernCoeffFromByte(uint8_t byte, uint8_t pos) const
{
	uint8_t c = byte & ((1 << COEFF_BERN_BITLEN * pos) | (1 << (COEFF_BERN_BITLEN * pos + 1)));
	c = c >> COEFF_BERN_BITLEN*pos;

	double d;
	switch (c)
	{
	case E_COEFF_BERN_VAL::ZERO:
		d = 0.0;
		break;
	case E_COEFF_BERN_VAL::PLUS_ONE:
		d = 1.0;
		break;
	case E_COEFF_BERN_VAL::MINUS_ONE:
		d = -1.0;
		break;
	default:
		NS_LOG_ERROR("Invalid coefficient!");
		d = 0.0;
		break;
	}
	return d;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::NcCoeffGenerator::NcCoeffGenerator()
{
	if (m_coeffType == CsClusterHeader::E_NcCoeffType::NORMAL)
		m_ranvar = CreateObject<NormalRandomVariable>();
	else if (m_coeffType == CsClusterHeader::E_NcCoeffType::BERN)
		m_ranvar = CreateObject<UniformRandomVariable>();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::NcCoeffGenerator::SetType(CsClusterHeader::E_NcCoeffType type)
{
	m_coeffType = type;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

double CsClusterHeader::NcCoeffGenerator::Generate() const
{
	double c = 0.0;
	if (m_coeffType == CsClusterHeader::E_NcCoeffType::NORMAL)
	{
		c = m_ranvar->GetValue();
	}
	else if (m_coeffType == CsClusterHeader::E_NcCoeffType::BERN)
	{
		double p = m_ranvar->GetValue();
		if (p > 0.5)
			c = 1.0;
		else
			c = -1.0;
	}
	return c;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

std::vector<double> CsClusterHeader::NcCoeffGenerator::Generate(uint32_t n) const
{
	std::vector<double> cv;
	cv.reserve(n);
	for (uint32_t i = 0; i < n; i++)
	{
		double c = Generate();

		cv.push_back(c);
	}
	return cv;
}