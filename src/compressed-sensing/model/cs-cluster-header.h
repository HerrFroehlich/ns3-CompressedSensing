/**
* \file cs-cluster-header.h
*
* \author Tobias Waurick
* \date 17.08.17
*
*/

#ifndef CS_CLUSTER_HEADER_H
#define CS_CLUSTER_HEADER_H

#include "cs-header.h"
#include "ns3/random-variable-stream.h"

#define BYTE_NVAL 256			  /**< NOF values represented by one byte*/
#define BYTE_NVAL_DIV_BYTE_LEN 32 /**< BYTE_NVAL divided by BYTE_LEN*/
/**
* \ingroup csNet
* \class CsClusterHeader
*
* \brief an extension of the CsHeader for cluster head nodes 
*
* Before using the CsHeader for cluster heads the static Setup function must be called, which is asserted in some methods. 
*
* For a cluster node the header is extended by the following fields:
* - L*256 bit SrcInfo : indicates which source nodes were compressed, 1bit describing one node, the LSB bit representing the nodeID 0\n
* -   8 bit	NcCnt	: counter, storing the number of recombinations of the packet
* -   x bit NcInfo  : information on the process of the network coding coefficients during recombinations in the network.\n
* 					  The size of the network coding information field varies depending on the NOF clusters and their spatial compression, it has to be set explicitly.
*					  Each value in this field corresponds to a row in the spatially compressed \f$Z_k\f$ of each cluster head, so the field size (NOF values) can be computed as
*					  \f$ \sum_{k=0}^L l_k\f$, where \f$L\f$ is the number of clusters and \f$l_k\f$ the spatial compression dimension (NOF rows of \f$Z_k\f$)
*					  of each cluster head. 
* The type of the coefficients is settable, so far supported are: 
* - Normal distributed : double data type, 64 bit per coefficient
* - 1/-1 bernoulli distributed : 2 bit per coefficient, 00 as 0, 01 as 1, 10 as -1
* To have an simple API the coefficients are always returned/set as double values, but they their bit representation is true in a packet.
*/
class CsClusterHeader : public CsHeader
{
  public:
	//sizes
	const static T_IdField CLUSTER_NODEID = 0;										/**< the node id of a cluster node*/
	const static uint32_t SRCINFO_BITLEN = sizeof(T_IdField) * BYTE_NVAL;			/**< length of source info field of one cluster in bit*/
	const static uint32_t SRCINFO_LEN = sizeof(T_IdField) * BYTE_NVAL_DIV_BYTE_LEN; /**< length of source info field of one cluster in byte*/
	const static uint32_t COEFF_NORM_LEN = sizeof(double);							/**< length of a coefficent when using a normal distribution in byte*/
	const static uint32_t COEFF_NORM_BITLEN = sizeof(double) * BYTE_LEN;			/**< length of a coefficent when using a normal distribution in bit*/
	const static uint32_t COEFF_BERN_BITLEN = 2;									/**< length of a coefficent when using a bernoulli distribution in bit*/
	const static uint32_t COEFF_BERN_PER_BYTE = 4;									/**< how many coefficents when using a bernoulli distribution fit in one byte*/
																					//fields

	//fields
	enum E_NcCoeffType /**< enum stating which type of values is used for network coding*/
	{
		NORMAL, /**< using normal distributed coefficients*/
		BERN,	/**< using bernoulli distributed(-1/+1) coefficients*/
		UNI     /**< using uniform (0...1) distributed coefficients*/
	};
	//typedef NcCoeffGenerator::NcCoeff::E_Type E_NcCoeffType;
	//typedef NcCoeffGenerator::NcCoeff T_NcInfoFieldValue;		  /**< type of the values in the network coding information field*/
	typedef std::vector<double> T_NcInfoField;			/**< type representation of the network coding information field for getting and setting*/
	typedef uint8_t T_NcCountField;						/**< type of the recombination counter*/
	typedef std::bitset<SRCINFO_BITLEN> T_SrcInfoField; /**< type of the source information field for each cluster*/

	/**
	* \ingroup csNet
	* \class NcCoeffGenerator
	*
	* \brief Generates network coding coefficients as doubles from a RandomVariableStream
	*
	* The coefficients are drawn sequentially from a RandomVariableStream. The stream number is assigned automatically.
	* So far it is possible to generate the coefficients \f$ \beta(t) \f$ with the following distributions: \n
	* - Normal distribution : \f$ \beta(t)  \sim \mathcal{N}(0,1) \f$
	* - Bernoulli distribution : \f$ p(\beta(t) = 1) = p(\beta(t) = -1) = 0.5 \f$
	*
	*/ class NcCoeffGenerator
	{
	  public:
		NcCoeffGenerator();

		/**
		* \brief generates one network coding coefficients represented as a double
		*
		* \return network coding coefficients
		*/
		double Generate() const;

		/**
		* \brief generates multiple  network coding coefficients represented as doubles
		*
		* \param n NOF coefficients tp generate
		*
		* \return network coding coefficients
		*/
		std::vector<double> Generate(uint32_t n) const;

	  private:
		/**
		* \brief sets the type of nc coefficients for all generators
		*
		* \param type type of the network coefficients
		*
		*/
		static void SetType(CsClusterHeader::E_NcCoeffType);

		static CsClusterHeader::E_NcCoeffType m_coeffType;
		Ptr<RandomVariableStream> m_ranvar;
		friend CsClusterHeader;
	};

	CsClusterHeader();

	/**
	* \brief adds information on source nodes used in cluster compression from a bitset
	*
	* \param set bitset to set
	* \param clusterId ID of cluster
	*
	*/
	void SetSrcInfo(const T_SrcInfoField &set, uint32_t clusterId);

	/**
	* \brief gets the information on source nodes used in cluster compression
	*
	* \param clusterId ID of cluster
	*
	* \return bit set containing the infoset
	*
	*/
	T_SrcInfoField GetSrcInfo(uint32_t clusterId) const;

	/**
	* \brief checks if source information was set for the ID
	*
	* source information is considered set, if at least one bit is set to 1
	*
	* \return true  if source information was set for the ID
	*
	*/
	bool IsSrcInfoSet(uint32_t clusterId) const;

	/**
	* \brief setups the CsHeader by calculating and setting the size of the network coding information field and the NOF clusters
	*
	* Since the size of the network coding information field varies depending on the
	* NOF clusters and their spatial compression, it has to be set explicitly.
	* The field size can be computed as \f$ \sum{k=0}^L l_k\f$,
	* where \f$L\f$ is the number of clusters and \f$l_k\f$ the
	* spatial compression dimension (NOF rows of \f$Z_k\f$) of each cluster head.
	* \warning This method should be called globally before using the CsHeader.
	*
	* \param lk vector with the spatial compression dimension of each cluster head, NOF of clusters is deduced from vector length
	* \param cType type of NC coefficients to use
	*
	*/
	static void Setup(const std::vector<uint32_t> &lk, E_NcCoeffType cType = E_NcCoeffType::NORMAL);

	/**
	* \brief gets the type of nc coefficients for all generators
	*
	* \param type type of the network coefficients
	*
	*/
	static E_NcCoeffType GetNcCoeffType();

	/**
	* \brief gets the maximum NOF clusters
	*
	* \return maximum NOF clusters  
	*/
	static uint32_t GetMaxClusters();

	/**
	* \brief Gets the size of the network coding information field
	*
	* \return size of the network coding information field
	*/
	static uint32_t GetNcInfoSize();

	/**
	* \brief sets network coding information field for a new packet
	*
	* Each value in this field corresponds to a row in the spatially compressed \f$Z_k\f$ of each cluster head.
	* Thus for a fresh new created packet the vector equals the \f$sum_{j=0}^{j<k}l_k + i \f$, where
	* \f$l_k\f$ are the spatial compression dimension, k is the current cluster head (= clusterId) and i
	* the row of \f$Z_k\f$ beeing transmitted.
	* Asserts that SetupCl was called, and that i < l. 
	*
	* \param clusterId 	ID of the cluster
	* \param i			index of \f$Z_k\f$ row, which is transmitted			
	*
	*/
	void SetNcInfoNew(T_IdField clusterId, uint32_t i);

	/**
	* \brief sets the network coding information field from a vector
	*
	* If the vector does not match the size of network coding information field an error is thrown.
	*
	* \param vec vector containing values of the network coding information field
	*
	*/
	void SetNcInfo(const T_NcInfoField &vec);

	/**
	* \brief gets the the network coding information field
	*
	*
	* \return vector containing values of the network coding information field
	*/
	T_NcInfoField GetNcInfo() const;

	/**
	* \brief sets the network coding recombination counter
	*
	* \param cnt new count
	*
	*/
	void SetNcCount(uint32_t cnt);

	/**
	* \brief gets the value of the network coding recombination counter field
	*
	* \return value of the network coding recombination counter field
	*/
	T_NcCountField GetNcCount() const;

	//inherited from class CsHeader

	/**
	* \brief sets the node ID to CLUSTER_NODEID
	*
	* A cluster head has a static node ID of CLUSTER_NODEID, so it can be differed from the common source nodes.
	* Calling this method will ignore the input parameter and will set the constant CLUSTER_NODEID instead
	*
	* \param ignored ignored parameter
	*
	*/
	virtual void SetNodeId(T_IdField ignored);

	//inherited from class Header
	static TypeId GetTypeId();
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;

  private:
	//const variables
	enum E_COEFF_BERN_VAL /**< interpretation of the 2bits when using a bernoulli distribution*/
	{
		ZERO = 0,	  /**< 00: 0*/
		PLUS_ONE = 1,  /**< 01: 1*/
		MINUS_ONE = 2, /**< 10: -1*/
		INVALID = 3,   /**< 11: Invalid*/
	};

	/**
	* \brief writes a bernoulli coefficient to a byte
	*
	* \param byte byte to write to
	* \param val value to write
	* \param pos position in byte(0..3)
	*
	* \return byte with added coefficient
	*/
	uint8_t WriteBernCoeffToByte(uint8_t byte, double val, uint8_t pos) const;

	/**
	* \brief reads a bernoulli from to a byte
	*
	* \param byte byte to write to
	* \param pos position in byte(0..3)
	*
	* \return corresponding double coefficient
	*/
	double ReadBernCoeffFromByte(uint8_t byte, uint8_t pos) const;

	//static variables
	static bool m_isSetup;
	static uint32_t m_ncInfoSize;	  /**< size of the network coding information field (NOF values)*/
	static uint32_t m_maxClusters;	 /**< maximum NOF clusters*/
	static std::vector<uint32_t> m_lk; /**< vector with the spatial compression dimension of each cluster head*/
	static E_NcCoeffType m_coeffType;

	//variables
	T_NcCountField m_ncCount; /**< network combination counter*/
	//T_SrcInfoField m_srcInfo; /**< info about source nodes*/
	std::vector<T_SrcInfoField> m_srcInfo; /**< info about source nodes of each cluster*/
	T_NcInfoField m_ncInfo;				   /**< network coding information field*/
};

#endif //CS_CLUSTER_HEADER_H