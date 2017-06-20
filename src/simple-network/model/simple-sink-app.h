/**
* \file simple-sink-app.h
*
* \author Tobias Waurick
* \date 01.06.17
*
*/

#ifndef SIMPLE_SINK_APP_H
#define SIMPLE_SINK_APP_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/simple-header.h"
#include <iostream>

/**
* \class SimpleSinkApp
*
* \brief a SimpleSinkApp  which receives data of selected Type from several source nodes
*
* The sink App receives data (double values) on all net devices aggregated to the node.
* The received data is printed out. Truncated double values will be dropped.
*
* \author Tobias Waurick
* \date 01.06.17
*/
class SimpleSinkApp : public Application
{
  public:
	static TypeId GetTypeId(void);
	/**
* \brief constructs a SimpleSinkApp
*
* \param nSrcNodes NOF src nodes from which sink is receiving data
* \param os	  outputstream
*
* \return returnDesc
*/
	SimpleSinkApp(uint32_t nSrcNodes, std::ostream &os);
	virtual ~SimpleSinkApp();

	/**
* \brief setups the app
*
* \param node Pointer to a node
*
*/
	void Setup(Ptr<Node>);

  private:
	/**
  * \brief receive callback for rx devices
  *
  * \param dev pointer to the net device
  * \param p   received packet
  * \param idUnused   protocol id UNUSED
  * \param adrUnused  sender address UNUSED
  *
  */
	bool Receive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused);

	/**
	* \brief prints out an array of doubles received from a node
	*
	* \param nodeId	ID of src node
	* \param doubles pointer to an double array
	* \param size	size of double array		
	*
	*/
	void PrintOut(uint8_t nodeId, double *doubles, uint32_t size);

	union doubleToBytes /**< union to create bytes from a double*/
	{
		double dbl;
		uint8_t bytes[sizeof(double)];
	};
	uint32_t m_nRxDevices,
		m_nSrcNodes;  /**< NOF Src nodes*/
	Ptr<Node> m_node; /**< aggregated node*/
	bool m_isSetup;
	std::ostream &m_os; /**< output stream*/
	TracedCallback<Ptr<const Packet>> m_rxTrace;
};
#endif //SIMPLE_SINK_APP_H
