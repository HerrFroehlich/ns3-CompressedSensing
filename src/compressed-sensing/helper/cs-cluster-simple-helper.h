/**
* \file cs-cluster-simple-helper.h
*
* \author Tobias Waurick
* \date 18.07.17
*	
*/

#ifndef CS_CLUSTER__SIMPLE_HELPER_H
#define CS_CLUSTER__SIMPLE_HELPER_H

#include <string>
#include "ns3/random-variable-stream.h"
#include "ns3/my-simple-net-device-helper.h"
#include "ns3/cs-cluster-app.h"
#include "ns3/application-container.h"
#include "ns3/cs-cluster-header.h"
#include "ns3/data-stream.h"
#include "ns3/cs-cluster.h"
#include "cs-node-container.h"

/**
* \ingroup util
* \class CsClusterSimpleHelper
*
* \brief Helper to create a cluster connected with MySimpleChannel and MySimpleNetDevice, as well adding applications
*
* This helper makes the creation of a cluster much easier. By calling Create a cluster with one cluster and n
* source nodes is initiated. Each source node is connected to the cluster node via a point to point link using 
* MySimpleChannel and MySimpleNetDevices. Hereby N receiving net devices with a DropTailQueue are attached to the the cluster node and to
* every source node one transmitting net device. Finally for the cluster node a CsClusterApp and for the source nodes
* CsSrcApp are setup and associated.
* Beforehand creation attributes of the net devices, channels and application and also the compression dimensions can be set.
* Additionaly it is possible to have the data rates of the source MySimpleNetDevice and the delay of MySimpleChannel drawn
* randomly from a gaussian distribution.
*
*/
class CsClusterSimpleHelper
{
  public:
	CsClusterSimpleHelper();

	/**
	* Set attributes of the DropTailQueue  associated with each
	* SimpleNetDevice created through CsClusterSimpleHelper::Create.
	*
	* \param n1 the name of the attribute to set on the queue
	* \param v1 the value of the attribute to set on the queue
	*
	*/
	void SetQueueAttribute(std::string n1, const AttributeValue &v1);

	/**
	* Each net device must have a channel to pass packets through.
	* This method allows one to set the type of the channel that is automatically
	* created when the device is created and attached to a node.
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
	* Set the type of channel to create and associated to each
	* SimpleNetDevice created through CsClusterSimpleHelper::Create.
	*/
	// void SetChannel(std::string type,
	// 				std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue(),
	// 				std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue(),
	// 				std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue(),
	// 				std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue());

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	* Set these attributes on each tx MySimpleNetDevice created for the source nodes
	* by CsClusterSimpleHelper::Create
	*/
	void SetSrcDeviceAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	* Set these attributes on each rx MySimpleNetDevice created for the cluster node
	* by CsClusterSimpleHelper::Create
	*/
	void SetClusterDeviceAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	* Set these attributes on each CsSrcApp created for the source nodes
	* by CsClusterSimpleHelper::Create
	*/
	void SetSrcAppAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	* Set these attributes on each CsClusterApp created for the source nodes
	* by CsClusterSimpleHelper::Create
	*/
	void SetClusterAppAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \param n1 the name of the attribute to set
	* \param v1 the value of the attribute to set
	*
	* Set these attributes on each MySimpleChannel created
	* by CsClusterSimpleHelper::Create
	*/
	void SetChannelAttribute(std::string n1, const AttributeValue &v1);

	/**
	* \brief sets the function which creates a seed from an index and a clusterid
	*
	*
	* \param seeder function pointer to seed generator function
	*/
	void SetNodeSeeder(CsCluster::SeedCreator seeder);

	/**
	* \brief creates the cluster with the selected attributes
	*
	* The cluster head seeded used for spatial compression is simply set to the value of id+1. 
	* Seeds are selected from a the default seed function of CsNodeContainer or by an optional function set by SetNodeSeeder;
	* The SerialDataBuffer in the DataStream at index 0 will be used for the cluster head node, the one at 1 for the first source node
	* and so on. The used SerialDataBuffer instances are removed from the DataStream!
	*
	*
	* \param id cluster id to set
	* \param nNodes NOF  nodes in cluster to create
	* \param stream DataStream with at least one SerialDataBuffer for each node (n+1)
	*
	* \return created CsCluster
	*/
	Ptr<CsCluster> Create(CsHeader::T_IdField id, uint32_t nNodes, DataStream<double> &stream);

	/**
	* \brief sets the channel delay for the created MySimpleChannels to be random with a mean and variance
	*
	* The delays are chosen from a gaussian distribution. Negativ channel delays are prevented.
	*
	* \param mean mean delay
	* \param var  delay variation
	* 
	*
	*/
	void SetRandomDelay(Time mean, Time var);

	/**
	* \brief sets the data rate for the created MySimpleNetDevices to be random with mean a and variance
	*
	* The delays are chosen from a gaussian distribution. Negativ data rates are prevented.
	*
	* \param mean mean data rate
	* \param var  data rate variation
	* 
	*
	*/
	void SetRandomDataRate(DataRate mean, DataRate var);

	/**
	* \brief sets the compression dimensions for source and cluster nodes
	*
	* Also sets the queue length of the DropTailQueue to l, so that no packages are dropped at the cluster head.
	*
	* \param n length of original measurement vector
	* \param m length of temporally compressed vector
	* \param  l NOF spatially compressed vectors
	*
	*/
	void SetCompression(uint32_t n, uint32_t m, uint32_t l);

	/**
	* \brief Normalizes the random matrices to 1/sqrt(m)
	*/
	void NormalizeToM();

	/**
	* \brief gets the first application of each node in node container
	*
	* Useful for getting the associated applications after calling Create.
	* This method fails, if one node has no applications;
	*
	*
	* \return ApplicationContainer with associated applications
	*/
	//static ApplicationContainer GetFirstApp(CsNodeContainer nodes);

  private:
	ObjectFactory m_queueFactory;		  //!< Queue factory
	ObjectFactory m_srcDeviceFactory;	 //!< NetDevice factory
	ObjectFactory m_clusterDeviceFactory; //!< NetDevice factory
	ObjectFactory m_channelFactory;		  //!< Channel factory
	ObjectFactory m_srcAppFactory;		  //!< Application factory
	ObjectFactory m_clusterAppFactory;	//!< Application factory

	MySimpleNetDeviceHelper m_deviceHelper;
	bool m_ranDelay, m_ranRate;
	double m_delayMean, m_delayVar, m_rateMean, m_rateVar;

	NormalRandomVariable m_gaussRan;
	CsCluster::SeedCreator m_seeder;
};
#endif //CS_CLUSTER__SIMPLE_HELPER_H