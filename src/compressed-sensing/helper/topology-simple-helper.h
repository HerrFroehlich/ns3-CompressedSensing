/**
* \file topology-helper.h
*
* \author Tobias Waurick
* \date 28.08.17
*
*/
#ifndef TOPOLOGY_SIMPLE_HELPER_H
#define TOPOLOGY_SIMPLE_HELPER_H

#include <vector>
#include <armadillo>
#include "ns3/core-module.h"
#include "ns3/cs-cluster.h"
using namespace ns3;
using namespace arma;
/**
* \ingroup util
* \class TopologySimpleHelper
*
* \brief a helper class to set up a given topology easily
*
* This class can be used to easily create links between cluster heads to each other and the sink.
* To describe the connections between the clusters a square matrix(Links) is used, for the connections to the sink a vector.
* It is possible to create link with or without an rate error model for package loss. 
* For the first case, the value in matrix at (i, j) corresponds to the packet error rate between i and j (similiar for the sink link vector).
* For the later case,a one in the matrix at i,j states a connection between cluster head i to cluster head j(similiar for the sink link vector); 
* As default connections are made with a MySimpleChannel connected to MySimpleNetDevice instances with an DropTailQueue.
*
*/
class TopologySimpleHelper : public Object
{
  public:
	/**
	* \brief a NxN matrix describing the connections among N cluster heads and also the links between those clusters to the sink 
	*
	* When used with booleans,a one in the matrix at i,j states a connection between cluster i to cluster j. 
	* When used with a double, the value corresponds to the packet receive probability (= 1 - error rate) between i and j.
	* Same applies for the links to the sink.
	*
	* \tparam T type of matrix entry(uint8_t (aka boolean) or double)
	*/
	template <typename T>
	class Links
	{
	public:
		/**
		* \brief creates an NxN matrix filled with zeros
		*
		* \param n NOF rows/cols of the matrix
		*/
		Links(uint32_t n);

		/**
		* \brief creates from an asserted square matrix for the cluster-cluster links and a vector for the the cluster-sink connection
		*
		* Calls SetAllLinks.
		*
		* \param clLinks cluster-cluster connection matrix
		* \param sLinks cluster-sink connection vector
		*/
		Links(const Mat<T> &clLinks, const std::vector<T> &sLinks);

		/**
		* \brief sets all links from an arma::Mat for the cluster-cluster links and a vector for the the cluster-sink connection
		*
		* It is assert that the size of the matrix is as well NxN andvector for the the sink connections has N elements.
		*
		* \param clLinks cluster-cluster connection matrix
		* \param sLinks cluster-sink connection vector
		*
		*/
		void SetAllLinks(const Mat<T> &clLinks, const std::vector<T> &sLinks);

		/**
		* \brief sets the connection from cluster head i to j
		*
		* \param i source cluster
		* \param j receiving cluster
		* \param val value of matrix entry
		*/
		void SetClLink(uint32_t i, uint32_t j, T val = 1);

		/**
		* \brief sets links from cluster head i to all other clusters heads
		*
		* The size of the vector containing the values
		* must equal to N, else this method will fail with an error.
		*
		* \param i source cluster
		* \param values vector with values of matrix entries
		*/
		void SetClLink(uint32_t i, const std::vector<T> &values);

		/**
		* \brief sets the connection from cluster head i to the sink
		*
		* \param i source cluster
		* \param val value of matrix entry
		*/
		void SetSinkLink(uint32_t i, T val = 1);

		/**
		* \brief sets links from all cluster heads to the sink
		*
		* The size of the vector containing the values
		* must equal to N, else this method will fail with an error.
		*
		* \param links links to sink
		*/
		void SetSinkLink(const std::vector<T> &links);

		/**
		* \brief gets the connection from i to j
		*
		* \param i source cluster
		* \param j receiving cluster
		* \return connection from i to j
		*/
		T GetClLink(uint32_t i, uint32_t j) const;

		/**
		* \brief gets the connection from i to the  sink
		*
		* \param i source cluster
		* \return connection from i to sink
		*/
		T GetSinkLink(uint32_t i) const;

		/**
		* \brief gets the size of the connection matrix
		*
		* \return size of connection matrix
		*/
		uint32_t GetSize() const
		{
			return m_len;
		}

	  private:
		uint32_t m_len;
		Mat<T> m_links;
		std::vector<T> m_sinkLinks;
	};

	typedef Links<uint8_t> LinksBool;
	typedef Links<double> LinksDouble;

	TopologySimpleHelper();

	/**
	* \brief creates a topology with links from cluster to cluster given by a matrix and from cluster to sink by a vector
	*
	* Adds net devices and channels to the cluster and the sink with help from a link matrix/vector.
	* The size of the cluster vector must equal to the length of the  link matrix/vector.
	*
	* \param clusters vector with pointers to CsCluster instances
	* \param sink pointer to sink node
	* \param clLinks link matrix between clusters (and also sink)
	*
	*/
	void Create(const std::vector<Ptr<CsCluster>> &clusters, Ptr<CsNode> sink, const LinksBool &clLinks) const;

	/**
	* \brief creates a topology with links from cluster to cluster given by a matrix and from cluster to sink by a vector with an rate error model.
	*
	* Adds net devices and channels to the cluster and the sink  with help from a link matrix/vector.
	* For each receiving net device a rate error model for package losses is added, with an receive probability (= 1 - error rate) depending  on the corresponding value. 
	* The size of the cluster vector must equal to the length of the  link matrix/vector.
	* in the link matrix/vector. 
	*
	* \param clusters vector with pointers to CsCluster instances
	* \param sink pointer to sink node
	* \param clLinks link matrix between clusters (and also sink)
	*
	*/
	void Create(const std::vector<Ptr<CsCluster>> &clusters, Ptr<CsNode> sink, const LinksDouble &clLinks) const;

	/**
	* Each net device must have a queue to pass packets through.
	* This method allows one to set the type of the queue that is automatically
	* created when the topology is created.
	*
	* \param type the type of queue
	* \param n1 the name of the attribute to set on the queue
	* \param v1 the value of the attribute to set on the queue
	* \param n2 the name of the attribute to set on the queue
	* \param v2 the value of the attribute to set on the queue
	* \param n3 the name of the attribute to set on the queue
	* \param v3 the value of the attribute to set on the queue
	* \param n4 the name of the attribute to set on the queue
	* \param v4 the value of the attribute to set on the queue
	*
   */
	void SetQueue(std::string type,
				  std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue(),
				  std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue(),
				  std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue(),
				  std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue());

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	*/
	void SetDeviceAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	*/
	void SetChannelAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	*/
	void SetQueueAttribute(std::string n1, const AttributeValue &v1);

  private:
	/**
	* \brief connects two CsNode instances
	*
	* \param nodeTx transmitting node
	* \param nodeRx receiving node
	* \param errRate error rate of the link
	*
	*/
	void DoConnect(Ptr<CsNode> nodeTx, Ptr<CsNode> nodeRx, double errRate = 0.0) const;

	ObjectFactory m_queueFactory;   //!< Queue factory
	ObjectFactory m_deviceFactory;  //!< NetDevice factory
	ObjectFactory m_channelFactory; //!< Channel factory
};

#endif //TOPOLOGY_SIMPLE_HELPER_H