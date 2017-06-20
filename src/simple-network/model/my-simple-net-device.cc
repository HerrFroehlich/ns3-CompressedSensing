/**
* \file my-simple-net-device.cc
*
* \author Tobias Waurick
* \date 15.05.17
*
* 
*/
#include "my-simple-net-device.h"
#include "my-simple-channel.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

NS_LOG_COMPONENT_DEFINE ("MySimpleNetDevice");

NS_OBJECT_ENSURE_REGISTERED (MySimpleNetDevice);

TypeId MySimpleNetDevice::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::MySimpleNetDevice")
							.SetParent<NetDevice>()
							.SetGroupName("SimpleNetwork")
							.AddConstructor<MySimpleNetDevice>()
							.AddAttribute("ReceiveErrorModel",
										  "The receiver error model used to simulate packet loss",
										  PointerValue(),
										  MakePointerAccessor(&MySimpleNetDevice::m_receiveErrorModel),
										  MakePointerChecker<ErrorModel>())
							.AddAttribute("TxQueue",
										  "A queue to use as the transmit queue in the device.",
										  StringValue("ns3::DropTailQueue"),
										  MakePointerAccessor(&MySimpleNetDevice::m_queue),
										  MakePointerChecker<Queue>())
							.AddAttribute("DataRate",
										  "The default data rate for point to point links. Zero means infinite",
										  DataRateValue(DataRate("0b/s")),
										  MakeDataRateAccessor(&MySimpleNetDevice::m_bps),
										  MakeDataRateChecker())
							.AddTraceSource("PhyRxDrop",
											"Trace source indicating a packet has been dropped "
											"by the device during reception",
											MakeTraceSourceAccessor(&MySimpleNetDevice::m_phyRxDropTrace),
											"ns3::Packet::TracedCallback");
	return tid;
}

MySimpleNetDevice::MySimpleNetDevice() : 
	m_channel (0),
    m_node (0),
    m_mtu (0xffff),
    m_ifIndex (0),
    m_linkUp (false)
{
	NS_LOG_FUNCTION(this);
}

bool
MySimpleNetDevice::SimpleTransmit(Ptr<Packet> p)
{
 NS_LOG_FUNCTION (this << p);
//   if (p->GetSize () > GetMtu ())
//     {
//       return false;
//     }
  Ptr<Packet> packet = p->Copy ();

  if (m_queue->Enqueue (Create<QueueItem> (p)))
    {
      if (m_queue->GetNPackets () == 1 && !TransmitCompleteEvent.IsRunning ())
        {
          p = m_queue->Dequeue ()->GetPacket ();
          Time txTime = Time (0);
          if (m_bps > DataRate (0))
            {
              txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
            }
          m_channel->TransmitStart (p, txTime, this);
          TransmitCompleteEvent = Simulator::Schedule (txTime, &MySimpleNetDevice::TransmitComplete, this);
        }
     return true;
	}
	return false;
}

void MySimpleNetDevice::Receive(Ptr<Packet> packet)
{
	NS_LOG_FUNCTION (this << packet);

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      m_phyRxDropTrace (packet);
    }
	else
	{
   // if(!m_rxCallback.IsNull())
      m_rxCallback(this, packet, 0, Address());
  }

return;
}
void 
MySimpleNetDevice::SetChannel (Ptr<MySimpleChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  m_channel = channel;
  m_channel->Add (this);
  m_linkUp = true;
}
Ptr<Channel> 
MySimpleNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
MySimpleNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void 
MySimpleNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t 
MySimpleNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
void
MySimpleNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = address;
}
Address 
MySimpleNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool
MySimpleNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t 
MySimpleNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool 
MySimpleNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}
void 
MySimpleNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
	return;
}
bool 
MySimpleNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Address
MySimpleNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Address (); //create invalid addres
}
bool 
MySimpleNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
Address 
MySimpleNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Address ();
}
Address 
MySimpleNetDevice::GetMulticast (Ipv6Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Address ();
}
bool 
MySimpleNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool 
MySimpleNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool 
MySimpleNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SimpleTransmit(packet);
}

bool
MySimpleNetDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{
	  return SimpleTransmit(p);
}


void
MySimpleNetDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);

  if (m_queue->GetNPackets () == 0)
    {
      return;
    }

  Ptr<Packet> packet = m_queue->Dequeue ()->GetPacket ();
  SimpleTransmit(packet);

  if (m_queue->GetNPackets ())
    {
      Time txTime = Time (0);
      if (m_bps > DataRate (0))
        {
          txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
        }
      TransmitCompleteEvent = Simulator::Schedule (txTime, &MySimpleNetDevice::TransmitComplete, this);
    }

  return;
}

Ptr<Node> 
MySimpleNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void 
MySimpleNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool 
MySimpleNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
void 
MySimpleNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void 
MySimpleNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_receiveErrorModel = 0;
  m_queue->DequeueAll ();
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  NetDevice::DoDispose ();
}


void 
MySimpleNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool 
MySimpleNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Ptr<Queue>
MySimpleNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
MySimpleNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}