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

NS_OBJECT_ENSURE_REGISTERED(CsClusterHeader);

bool CsClusterHeader::m_isSetup = false;
uint32_t CsClusterHeader::m_ncInfoSize = 0;
uint32_t CsClusterHeader::m_maxClusters = 0;
std::vector<uint32_t> CsClusterHeader::m_lk;

CsClusterHeader::CsClusterHeader() : m_ncCount(0), m_srcInfo(m_maxClusters), m_ncInfo(m_ncInfoSize)
{
	CsHeader::SetNodeId(CLUSTER_NODEID);
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

void CsClusterHeader::SetupCl(const std::vector<uint32_t> &lk)
{
	m_lk = lk;
	m_ncInfoSize = 0;
	for (auto l : lk)
	{
		m_ncInfoSize += l;
	}
	m_maxClusters = m_lk.size();

	m_isSetup = true;
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

	std::vector<double> ncInfo(m_ncInfoSize, 0.0);

	uint32_t idx = i;
	for (uint32_t k = 0; k < clusterId; k++)
	{
		idx += m_lk.at(k);
	}

	ncInfo.at(idx) = 1.0;
	SetNcInfo(ncInfo);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsClusterHeader::SetNcInfo(CsClusterHeader::T_NcInfoField vec)
{
	NS_ASSERT_MSG(m_isSetup, "Run SetupCl first!");
	NS_ASSERT_MSG(vec.size() == m_ncInfoSize, "Vector has incorrect size!");

	m_ncInfo = vec;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsClusterHeader::T_NcInfoField CsClusterHeader::GetNcInfo() const
{
	NS_ASSERT_MSG(m_isSetup, "Run SetupCl first!");

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
	Buffer::Iterator i = start;

	CsHeader::DoDeserialize(i);

	//source info
	for (auto &it : m_srcInfo)
	{
		for (uint32_t j = 0; j < SRCINFO_LEN; j++)
		{
			uint8_t byte = i.ReadU8();
			for (size_t i = 0; i < BYTE_LEN; i++)
			{
				it[i + j * BYTE_LEN] = (1 << i) & byte;
			}
		}
	}
	//nc count
	m_ncCount = i.ReadU8();

	//nc info
	uint32_t nBytes = m_ncInfoSize * sizeof(T_NcInfoFieldValue);
	i.Read(reinterpret_cast<uint8_t *>(m_ncInfo.data()), nBytes);

	return GetSerializedSize();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsClusterHeader::GetSerializedSize() const
{
	uint32_t size = CsHeader::GetSerializedSize();

	size += SRCINFO_LEN + sizeof(T_NcCountField) + m_ncInfoSize * sizeof(T_NcInfoFieldValue);

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

	os << std::fixed << std::setprecision(2);
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
	Buffer::Iterator i = start;

	CsHeader::DoSerialize(i);

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
			i.WriteU8(byte);
		}
	}

	//nc count
	i.WriteU8(m_ncCount);

	//nc info
	uint32_t nBytes = m_ncInfoSize * sizeof(T_NcInfoFieldValue);
	i.Write(reinterpret_cast<const uint8_t *>(m_ncInfo.data()), nBytes);
}
