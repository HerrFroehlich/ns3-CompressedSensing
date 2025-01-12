/**
* \file cs-cluster.h
*
* \author Tobias Waurick
* \date 31.07.17
*
*/

#ifndef CS_CLUSTER_H
#define CS_CLUSTER_H
#include "cs-node.h"
#include "ns3/cs-node-container.h"
#include "ns3/data-stream.h"
#include "ns3/application-container.h"
#include "transform-matrix.h"
#include "random-matrix.h"
#include "cs-cluster-header.h"
using namespace ns3;

/**
* \ingroup csNet
* \class CsCluster
*
* \brief Class which describes a cluster in the network
*
* The CsCluster stores pointers to the cluster head and source nodes (CsNode).
* When creating a new cluster or adding new nodes the individual seed (from which the random sensing matrix is drawn) 
* is chosen via the default seed creator or an optional seeder function (with signature SeedCreator) for each node.
* Utility functions, such as iterators, are provided to access the nodes and their IDs/seeds easily.
* Additionally the compression dimensions n, m and l are saved, where:\n
* - n is the original NOF samples gathered of each source node each measurement intervall 
* - m is the NOF measurements after the temporal compression at each source node
* - l is the NOF measurment vectors after the spatial compression at the cluster head
* It is possible to freeze the cluster, which denies adding nodes and changing the compression dimensions.
* Inheriting from DataStreamContainer<double> one can add DataStream<double> instances to this class.  
*/
class CsCluster : public Object, public DataStreamContainer<double>
{
  public:
	typedef uint32_t (*SeedCreator)(uint32_t, CsHeader::T_IdField); /**< signature for a function with which seeds are created for a given node number*/
	enum E_COMPR_DIMS												/**< compression dimensions*/
	{
		n, /**< number of samples in  measurement vector per sequence for source nodes*/
		m, /**< number of samples in  measurement vector after temporal compression for source nodes*/
		l  /**< number of measurement vectors after spatial compression for cluster head node*/
	};

	static TypeId GetTypeId();
	/**
	* \brief creates an empty cluster
	*
	* \param cluster pointer to cluster node
	*/
	CsCluster(Ptr<CsNode> cluster);

	/**
	* \brief creates an cluster with cluster and source nodes
	*
	* The individual seeds are set with the default SeedCreator.
	* The node IDs for the sources are set incrementally
	*
	* \param cluster pointer to cluster node
	* \param srcNodes CsNodeContainer with source nodes to add
	*/
	CsCluster(Ptr<CsNode> cluster, const CsNodeContainer &srcNodes);

	/**
	* \brief sets the cluster head node
	*
	* Also set the DataStreamContainer groupName to "Clusterx", where x is the cluster ID.
	* Fails with an error if the cluster is frozen.
	*
	* \param node pointer to cluster node
	*
	*/
	void SetClusterHead(Ptr<CsNode> node);

	/**
	* \brief gets the cluster head node
	*
	* \return pointer to cluster node
	*/
	Ptr<CsNode> GetClusterHead() const;

	/**
	* \brief Gets the cluster ID
	*
	* \return cluster ID
	*/
	inline CsHeader::T_IdField GetClusterId()
	{
		return m_clusterNode->GetClusterId();
	};

	/**
	* \brief adds source nodes to internal container
	*
	* Asserts that the number of source nodes does not exceed CsHeader::MAX_SRCNODES.	
	* Fails with an error if the cluster is frozen.
	*
	* \param node pointer to source nodes to add	
    * \param seeder function with SeedCreator signature, which determines the seed of each node
	*
	*/
	void AddSrc(Ptr<CsNode> node, SeedCreator seeder = 0);

	/**
	* \brief adds source nodes to internal container
	*
	* Asserts that the number of source nodes does not exceed CsHeader::MAX_SRCNODES.
	* Fails with an error if the cluster is frozen.
	*
	* \param nodes CsNodeContainer to add
   	* \param seeder function with SeedCreator signature, which determines the seed of each node
	*
	*/
	void AddSrc(const CsNodeContainer &nodes, SeedCreator seeder = 0);

	/**
	* \brief gets source node with given index
	*
	* \param idx index of source node
	*
	* \return pointer to cluster node
	*/
	Ptr<CsNode> GetSrc(uint32_t idx) const;

	/**
	* \brief gets the number of source nodes attached
	*
	* \return NOF source nodes
	*/
	uint32_t GetNSrc() const;

	/**
	* \brief gets the number of nodes attached
	*
	* \return NOF source nodes
	*/
	uint32_t GetN() const;

	/**
	* \brief iterator to the source nodes beginning
	*
	* \return iterator to the beginning
	*/
	CsNodeContainer::Iterator SrcBegin() const;

	/**
	* \brief iterator to the source nodes ending
	*
	* \return iterator to the ending
	*/
	CsNodeContainer::Iterator SrcEnd() const;

	/**
	* \brief iterator to the all nodes beginning
	*
	* \return iterator to the beginning
	*/
	CsNodeContainer::Iterator Begin() const;

	/**
	* \brief iterator to the all nodes ending
	*
	* \return iterator to the ending
	*/
	CsNodeContainer::Iterator End() const;

	/**
	* \brief gets all applications of every node
	*
	*
	* \return ApplicationContainer with associated applications
	*/
	ApplicationContainer GetApps() const;

	/**
	* \brief Stores the compression parameters for this cluster
	*
	* Fails with an error if the cluster is frozen.
	*
	* \param n length of original measurement vector (source nodes)
	* \param m length of temporally compressed vector (source nodes)
	* \param l NOF compressed vectors(cluster node)
	*
	*/
	void SetCompression(uint32_t n, uint32_t m, uint32_t l);

	/**
	* \brief gets the stored compression parameters as vector 
	*
	* \return vector with  stored compression parameters: [n m l]
	*/
	std::vector<uint32_t> GetCompression() const;

	/**
	* \brief gets a compression parameters
	*
	* \return dim  compression parameters defined in E_COMPR_DIMS
	*/
	uint32_t GetCompression(E_COMPR_DIMS dim) const;

	/**
	* \brief sets the seed of the cluster head node used for spatial compression
	*
	* \param seed seed of the cluster head node used for spatial compression
	*/
	void SetClusterSeed(uint32_t seed);

	/**
	* \brief gets the seed of the cluster head node used for spatial compression
	*
	* \return seed of the cluster head node used for spatial compression
	*/
	uint32_t GetClusterSeed() const;

	/**
	* \brief gets seeds of all nodes used for temporal compression
	*
	* \return vector with seeds of all nodes
	*/
	std::vector<uint32_t> GetSeeds() const;

	/**
	* \brief freezes the cluster and denies adding nodes and changing the compression dimensions
	*/
	void Freeze();

	/**
	* \brief checks whether the cluster is frozen
	*
	* \return true if it is frozen
	*/
	bool IsFrozen();

  private:
	/**
	* \brief default seed creator function
  	*
	* The default creator ignores number and id and simply returns the constant value of
	* CsClusterHeader::GetMaxClusters() + 1
	*
	* \param number current node number	
	* \param id cluster id
	*
	* \return seed to associate with node
	*/
	uint32_t DefaultSeedCreator(uint32_t number, CsHeader::T_IdField id);

	Ptr<CsNode> m_clusterNode;
	CsNodeContainer m_srcNodes, m_allNodes;
	uint32_t m_seed; /**< seed of cluster head for spatial compression*/
	uint32_t m_n, m_m, m_l;
	bool m_isFrozen;
};

#endif //CS_CLUSTER_H