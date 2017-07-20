/**
* \file MySimpleChannel.cc
*
* \author Tobias Waurick
* \date	14.05.17
*
*/
#include "my-simple-channel.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MySimpleChannel");

NS_OBJECT_ENSURE_REGISTERED (MySimpleChannel);

TypeId MySimpleChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("MySimpleChannel")
    .SetParent<Channel> ()
    .SetGroupName ("SimpleNetwork")
    .AddConstructor<MySimpleChannel> ()
    .AddAttribute ("Delay", "Propagation delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&MySimpleChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}
MySimpleChannel::MySimpleChannel()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_nDevices(0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

 void MySimpleChannel::Add (Ptr<MySimpleNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT(m_nDevices < NOF_DEVICES);
  m_devices[m_nDevices] = device;
  m_nDevices++;
}

  void MySimpleChannel::TransmitStart (Ptr<Packet> p, Time txTime, Ptr<MySimpleNetDevice> sender)
  {
  NS_LOG_FUNCTION (this << p << sender);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT(m_nDevices == NOF_DEVICES);

  uint32_t idx = sender == m_devices[0] ? 1 : 0;

  // schedule simulator event
  Simulator::ScheduleWithContext (m_devices[idx]->GetNode ()->GetId (),
                                  txTime + m_delay, &MySimpleNetDevice::Receive,
                                  m_devices[idx], p);//set context and receive event towards rx node
  }

uint32_t MySimpleChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<NetDevice> MySimpleChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < NOF_DEVICES);
  return m_devices[i];
}