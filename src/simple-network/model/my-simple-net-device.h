/**
* \file my-simple-net-device.h
*
* \author Tobias Waurick
* \date 14.05.17
*
* 
*/
#ifndef MY_SIMPLE_NETDEVICE
#define MY_SIMPLE_NETDEVICE

#include "ns3/node.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/queue.h"
#include "ns3/error-model.h"
#include "ns3/event-id.h"
using namespace ns3;

class MySimpleChannel;
/**
* \ingroup simpleN
* \class MySimpleNetDevice
*
* \brief simple net device model
*
* most implementation ideas taken from point-to-point-net-device
*
* \author Tobias Waurick
* \date 15.05.17
*/
class MySimpleNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId(void);

  MySimpleNetDevice();
/**
* \brief transmit a packet via the net device
*
* \param p packet to send
*
* \return status of transmisson
*/
  bool SimpleTransmit(Ptr<Packet> p);
  /**
   * Set the Data Rate used for transmission of packets.  The data rate is
   * set in the Attach () method from the corresponding field in the channel
   * to which the device is attached.  It can be overridden using this method.
   *
   * \param bps the data rate at which this object operates
   */
  void SetDataRate(DataRate bps);
  /**
   * Receive a packet from a connected MySimpleChannel.
   *
   * \param[in] p Ptr to the received packet.
   */
  void Receive(Ptr<Packet> p);                   /**
   * Attach a channel to this net device.  This will be the 
   * channel the net device sends on
   * 
   * \param channel channel to assign to this net device
   *
   */
  void SetChannel(Ptr<MySimpleChannel> channel); /**
   * Attach a receive ErrorModel to the SimpleNetDevice.
   *
   * The SimpleNetDevice may optionally include an ErrorModel in
   * the packet receive chain.
   *
   * \see ErrorModel
   * \param em Ptr to the ErrorModel.
   */
  void SetReceiveErrorModel(Ptr<ErrorModel> em); /**
   * Attach a queue to the SimpleNetDevice.
   *
   * \param queue Ptr to the new queue.
   */
  void SetQueue(Ptr<Queue> queue);

  /**
   * Get a copy of the attached Queue.
   *
   * \returns Ptr to the queue.
   */
  Ptr<Queue> GetQueue(void) const;

  // inherited from NetDevice base class.
  virtual void SetIfIndex(const uint32_t index);
  virtual uint32_t GetIfIndex(void) const;
  virtual Ptr<Channel> GetChannel(void) const;
  virtual void SetAddress(Address address);
  virtual Address GetAddress(void) const;
  virtual bool SetMtu(const uint16_t mtu);
  virtual uint16_t GetMtu(void) const;
  virtual bool IsLinkUp(void) const;
  virtual void AddLinkChangeCallback(Callback<void> callback);
  virtual bool IsBroadcast(void) const;
  virtual Address GetBroadcast(void) const;
  virtual bool IsMulticast(void) const;
  virtual Address GetMulticast(Ipv4Address multicastGroup) const;
  virtual Address GetMulticast(Ipv6Address multicastGroup) const;
  virtual bool IsPointToPoint(void) const;
  virtual bool IsBridge(void) const;
  virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom(Ptr<Packet> packet, const Address &source, const Address &dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode(void) const;
  virtual void SetNode(Ptr<Node> node);
  virtual bool NeedsArp(void) const;
  virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback(PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom(void) const;

protected:
  virtual void DoDispose(void);

private:
  Ptr<MySimpleChannel> m_channel;                      //!< the channel the device is connected to
  NetDevice::ReceiveCallback m_rxCallback;             //!< Receive callback
  NetDevice::PromiscReceiveCallback m_promiscCallback; //!< Promiscuous receive callback
  Ptr<Node> m_node;                                    //!< Node this netDevice is associated to
  uint16_t m_mtu;                                      //!< MTU, unsused
  uint32_t m_ifIndex;                                  //!< Interface index
  Ptr<ErrorModel> m_receiveErrorModel;                 //!< Receive error model.

  /**
   * The trace source fired when the phy layer drops a packet it has received
   * due to the error model being active.  Although SimpleNetDevice doesn't 
   * really have a Phy model, we choose this trace source name for alignment
   * with other trace sources.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;

  /**
   * The TransmitComplete method is used internally to finish the process
   * of sending a packet out on the channel.
   */
  void TransmitComplete(void);

  bool m_linkUp; //!< Flag indicating whether or not the link is up

  Address m_address; //!< address, unused

  Ptr<Queue> m_queue;            //!< The Queue for outgoing packets.
  DataRate m_bps;                //!< The device nominal Data rate. Zero means infinite
  EventId TransmitCompleteEvent; //!< the Tx Complete event
};

#endif //MY_SIMPLE_NETDEVICE
