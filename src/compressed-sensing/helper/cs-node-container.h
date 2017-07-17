/**
* \file cs-node-container.h
*
* \author Tobias Waurick
* \date 16.07.17
*
*/

#ifndef CSNODE_CONTAINER_H
#define CSNODE_CONTAINER_H	
#include "ns3/cs-node.h"
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

	typedef uint32_t (*SeedCreator)(uint32_t, CsHeader::T_IdField); /**< signature for a function with which seeds are created for a given node number*/

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
   * CsNodes are of the given type are created. Seed and IDs are not set!
   *
   * \param type type of CsNodes to create
   * \param n The number of Nodes to create
   */
	void Create(CsNode::NodeType type, uint32_t n);

	/**
   * \brief Create n nodes with specified systemId for distributed simulations 
   * and append pointers to them to the end of this CsNodeContainer.
   *
   * CsNodes are of the given type are created. Seed and IDs are not set!
   * Adds the ability to specify systemId for distributed simulations.
   *
   * \param type type of CsNodes to create
   * \param n The number of CsNodes to create
   * \param systemId The system id or rank associated with the nodes
   */
	void Create(CsNode::NodeType type, uint32_t n, uint32_t systemId);

	/**
   * \brief Creates a Cluster with source n nodes and append pointers to them to the end of this 
   * CsNodeContainer.
   *
   * The cluster node will be stored first. The node ids are set incrementally by one.
   * A seed for the nodes is calculated via the default/ the given optional creator function.
   *
   * \param id cluster id 
   * \param n The number of Nodes to create
   * \param seeder function with SeedCreator signature, which determines the seed of each node
   */
	void CreateCluster(CsHeader::T_IdField id, uint32_t n, SeedCreator seeder = 0);
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
  *
	* the seed simply equals the given number + id + 1	
	*
	* \param number current node number	
	* \param id cluster id
	*
	* \return seed to associate with node
	*/
	uint32_t DefaultSeedCreator(uint32_t number, CsHeader::T_IdField id);

	std::vector<Ptr<CsNode>> m_nodes; //!< Nodes smart pointers
};


#endif  //CSNODE_CONTAINER_H	