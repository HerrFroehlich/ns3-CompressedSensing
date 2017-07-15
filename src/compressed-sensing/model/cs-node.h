/**
* \file cs-node.h
*
* \author Tobias Waurick
* \date 14.07.17
*
*/

#ifndef CS_NODE_H
#define CS_NODE_H
#include <vector>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/net-device-container.h"
using namespace ns3;
/**
* \ingroup util
* \class CsNode
*
* \brief extended ns3::Node with various new properties
*
* description:TODO
*
*/
class CsNode : public Node
{
  public:
	static TypeId GetTypeId(void);

	enum NodeType /**< type of node*/
	{
		NONE,
		SOURCE,
		CLUSTER,
		SINK
	};

	/**
	* \brief Create new node with no specific type
	* 
	*/
	CsNode();

	/**
	* \brief Create new node with given type
	* 
	* \param type CsNode::NodeType of this node
	*/
	CsNode(CsNode::NodeType type);

	/**
	* \brief Create new node with given type and specific system id
	* 
	* \param type CsNode::NodeType of this node
	* \param systemId	a unique integer used for parallel simulations. 
	*/
	CsNode(CsNode::NodeType type, uint32_t systemId);

	/**
	* \brief adds a NetDevice for receiving and transmitting
	* shadows Node::AddDevice
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	virtual uint32_t AddDevice(Ptr<NetDevice> device);

	/**
	* \brief adds a NetDevice exclusively used for transmitting
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	uint32_t AddTxDevice(Ptr<NetDevice> device);

	/**
	* \brief adds a NetDevice exclusively used for receiving
	*
	* \param device NetDevice to associate to this node
	*
	* \return the index of the NetDevice into the Node's list of NetDevice
	*/
	uint32_t AddRxDevice(Ptr<NetDevice> device);

	/**
	* \brief get all associated tx devices
	*
	*
	* \return NetDeviceContainer with associated tx devices
	*/
	NetDeviceContainer GetTxDevices() const;

	/**
	* \brief get all associated rx devices
	*
	* \return NetDeviceContainer with associated rx devices
	*/
	NetDeviceContainer GetRxDevices() const;

	/**
	* \brief get NOF associated tx devices
	*
	* \return NOF devices
	*/
	uint32_t GetNTxDevices() const;

	/**
	* \brief get NOF associated rx devices
	*
	* \return NOF devices
	*/
	uint32_t GetNRxDevices() const;
	/**
	* \brief Sets the node's seed
	*
	* \param seed seed to use
	*/
	void SetSeed(uint32_t seed);
	/**
	* \brief Gets the node's seed
	*
	* \return used seed
	*/
	uint32_t GetSeed() const;

	CsNode::NodeType GetNodeType() const;

  private:
	NodeType m_type;
	uint32_t m_seed;
	std::vector<uint32_t> m_isTxDevice, m_isRxDevice; /**< tx/rx device indices*/
};

/**
* \ingroup util
* \class CsNodeContainer
*
* \brief container for multiple CsNode
*
* The code was copied from ns3::NodeContainer and only slightly altered!
*
*/
class CsNodeContainer
{
  public:
	/// CsNode container iterator
	typedef std::vector<Ptr<CsNode>>::const_iterator Iterator;

	typedef uint32_t (*SeedCreator)(uint32_t); /**< signature for a function with which seeds are created for a given node number*/

	/**
   * Create an empty CsNodeContainer.
   */
	CsNodeContainer();

	/**
   * Create a CsNodeContainer with exactly one node which has been previously
   * instantiated.  The single CsNode is specified by a smart pointer.
   *
   * \param node The Ptr<CsNode> to add to the container.
   */
	CsNodeContainer(Ptr<CsNode> node);

	/**
   * Create a CsNodeContainer with exactly one node which has been previously 
   * instantiated and assigned a name using the Object Name Service.  This 
   * CsNode is then specified by its assigned name. 
   *
   * \param nodeName The name of the CsNode Object to add to the container.
   */
	//   CsNodeContainer (std::string nodeName);

	/**
   * Create a node container which is a concatenation of two input
   * NodeContainers.
   *
   * \param a The first CsNodeContainer
   * \param b The second CsNodeContainer
   *
   * \note A frequently seen idiom that uses these constructors involves the
   * implicit conversion by constructor of Ptr<CsNode>.  When used, two 
   * Ptr<CsNode> will be passed to this constructor instead of CsNodeContainer&.
   * C++ will notice the implicit conversion path that goes through the 
   * CsNodeContainer (Ptr<CsNode> node) constructor above.  Using this conversion
   * one may provide optionally provide arguments of Ptr<CsNode> to these 
   * constructors.
   */
	CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b);

	/**
   * Create a node container which is a concatenation of three input
   * NodeContainers.
   *
   * \param a The first CsNodeContainer
   * \param b The second CsNodeContainer
   * \param c The third CsNodeContainer
   *
   * \note A frequently seen idiom that uses these constructors involves the
   * implicit conversion by constructor of Ptr<CsNode>.  When used, two 
   * Ptr<CsNode> will be passed to this constructor instead of CsNodeContainer&.
   * C++ will notice the implicit conversion path that goes through the 
   * CsNodeContainer (Ptr<CsNode> node) constructor above.  Using this conversion
   * one may provide optionally provide arguments of Ptr<CsNode> to these 
   * constructors.
   */
	CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b, const CsNodeContainer &c);

	/**
   * Create a node container which is a concatenation of four input
   * NodeContainers.
   *
   * \param a The first CsNodeContainer
   * \param b The second CsNodeContainer
   * \param c The third CsNodeContainer
   * \param d The fourth CsNodeContainer
   *
   * \note A frequently seen idiom that uses these constructors involves the
   * implicit conversion by constructor of Ptr<CsNode>.  When used, two 
   * Ptr<CsNode> will be passed to this constructor instead of CsNodeContainer&.
   * C++ will notice the implicit conversion path that goes through the 
   * CsNodeContainer (Ptr<CsNode> node) constructor above.  Using this conversion
   * one may provide optionally provide arguments of Ptr<CsNode> to these 
   * constructors.
   */
	CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b, const CsNodeContainer &c, const CsNodeContainer &d);

	/**
   * Create a node container which is a concatenation of five input
   * NodeContainers.
   *
   * \param a The first CsNodeContainer
   * \param b The second CsNodeContainer
   * \param c The third CsNodeContainer
   * \param d The fourth CsNodeContainer
   * \param e The fifth CsNodeContainer
   *
   * \note A frequently seen idiom that uses these constructors involves the
   * implicit conversion by constructor of Ptr<CsNode>.  When used, two 
   * Ptr<CsNode> will be passed to this constructor instead of CsNodeContainer&.
   * C++ will notice the implicit conversion path that goes through the 
   * CsNodeContainer (Ptr<CsNode> node) constructor above.  Using this conversion
   * one may provide optionally provide arguments of Ptr<CsNode> to these 
   * constructors.
   */
	CsNodeContainer(const CsNodeContainer &a, const CsNodeContainer &b, const CsNodeContainer &c, const CsNodeContainer &d,
					const CsNodeContainer &e);

	/**
   * \brief Get an iterator which refers to the first CsNode in the 
   * container.
   *
   * Nodes can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the iterator method and is typically used in a 
   * for-loop to run through the Nodes
   *
   * \code
   *   CsNodeContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some CsNode method
   *     }
   * \endcode
   *
   * \returns an iterator which refers to the first CsNode in the container.
   */
	Iterator Begin(void) const;

	/**
   * \brief Get an iterator which indicates past-the-last CsNode in the 
   * container.
   *
   * Nodes can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the iterator method and is typically used in a 
   * for-loop to run through the Nodes
   *
   * \code
   *   CsNodeContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some CsNode method
   *     }
   * \endcode
   *
   * \returns an iterator which indicates an ending condition for a loop.
   */
	Iterator End(void) const;

	/**
   * \brief Get the number of Ptr<CsNode> stored in this container.
   *
   * Nodes can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the direct method and is typically used to
   * define an ending condition in a for-loop that runs through the stored
   * Nodes
   *
   * \code
   *   uint32_t nNodes = container.GetN ();
   *   for (uint32_t i = 0 i < nNodes; ++i)
   *     {
   *       Ptr<CsNode> p = container.Get (i)
   *       i->method ();  // some CsNode method
   *     }
   * \endcode
   *
   * \returns the number of Ptr<CsNode> stored in this container.
   */
	uint32_t GetN(void) const;

	/**
   * \brief Get the Ptr<CsNode> stored in this container at a given
   * index.
   *
   * Nodes can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the direct method and is used to retrieve the
   * indexed Ptr<Appliation>.
   *
   * \code
   *   uint32_t nNodes = container.GetN ();
   *   for (uint32_t i = 0 i < nNodes; ++i)
   *     {
   *       Ptr<CsNode> p = container.Get (i)
   *       i->method ();  // some CsNode method
   *     }
   * \endcode
   *
   * \param i the index of the requested node pointer.
   * \returns the requested node pointer.
   */
	Ptr<CsNode> Get(uint32_t i) const;

	/**
   * \brief Create n nodes and append pointers to them to the end of this 
   * CsNodeContainer.
   *
   * CsNodes are of the given type are create and a seed for them is calculated via the default/ optional creator function
   *
   * \param type type of CsNodes to create
   * \param n The number of Nodes to create
   * \param seeder function with SeedCreator signature, which determines the seed of each node
   */
	void Create(CsNode::NodeType type, uint32_t n, SeedCreator seeder = 0);

	/**
   * \brief Create n nodes with specified systemId for distributed simulations 
   * and append pointers to them to the end of this CsNodeContainer.
   *
   * CsNodes are of the given type are create and a seed for them is calculated via the default/ optional creator function
   * Adds the ability to specify systemId for distributed simulations.
   *
   * \param type type of CsNodes to create
   * \param n The number of CsNodes to create
   * \param systemId The system id or rank associated with this node
   * \param seeder function with SeedCreator signature, which determines the seed of each node
   */
	void Create(CsNode::NodeType type, uint32_t n, uint32_t systemId, SeedCreator seeder = 0);

	/**
   * \brief Append the contents of another CsNodeContainer to the end of
   * this container.
   *
   * \param other The CsNodeContainer to append.
   */
	void Add(CsNodeContainer other);

	/**
   * \brief Append a single Ptr<CsNode> to this container.
   *
   * \param node The Ptr<CsNode> to append.
   */
	void Add(Ptr<CsNode> node);

	/**
   * \brief Append to this container the single Ptr<CsNode> referred to
   * via its object name service registered name.
   *
   * \param nodeName The name of the CsNode Object to add to the container.
   */
	void Add(std::string nodeName);

	/**
   * \brief Create a CsNodeContainer that contains a list of _all_ nodes
   * created through CsNodeContainer::Create() and stored in the 
   * ns3::NodeList.
   *
   * Whenever a CsNode is created, a Ptr<CsNode> is added to a global list of all
   * nodes in the system.  It is sometimes useful to be able to get to all
   * nodes in one place.  This method creates a CsNodeContainer that is 
   * initialized to contain all of the simulation nodes,
   *
   * \returns a NoceContainer which contains a list of all Nodes.
   */
	// static CsNodeContainer GetGlobal(void);

  private:
	/**
	* \brief default seed creator function
	* the seed simply equals the given number	
	*
	* \param number current node number	
	*
	* \return seed to associate with node
	*/
	uint32_t DefaultSeedCreator(uint32_t number);

	std::vector<Ptr<CsNode>> m_nodes; //!< Nodes smart pointers
};

;

#endif //CS_NODE_H