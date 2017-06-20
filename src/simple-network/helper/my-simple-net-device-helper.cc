
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/boolean.h"

#include "ns3/trace-helper.h"
#include "my-simple-net-device-helper.h"

NS_LOG_COMPONENT_DEFINE ("MySimpleNetDeviceHelper");

MySimpleNetDeviceHelper::MySimpleNetDeviceHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::MySimpleNetDevice");
  m_channelFactory.SetTypeId ("ns3::MySimpleChannel");
}

void 
MySimpleNetDeviceHelper::SetQueue (std::string type,
                                 std::string n1, const AttributeValue &v1,
                                 std::string n2, const AttributeValue &v2,
                                 std::string n3, const AttributeValue &v3,
                                 std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void
MySimpleNetDeviceHelper::SetChannel (std::string type,
                                   std::string n1, const AttributeValue &v1,
                                   std::string n2, const AttributeValue &v2,
                                   std::string n3, const AttributeValue &v3,
                                   std::string n4, const AttributeValue &v4)
{
  m_channelFactory.SetTypeId (type);
  m_channelFactory.Set (n1, v1);
  m_channelFactory.Set (n2, v2);
  m_channelFactory.Set (n3, v3);
  m_channelFactory.Set (n4, v4);
}

void
MySimpleNetDeviceHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void
MySimpleNetDeviceHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}


NetDeviceContainer
MySimpleNetDeviceHelper::Install (Ptr<Node> node) const
{
  Ptr<MySimpleChannel> channel = m_channelFactory.Create<MySimpleChannel> ();
  return Install (node, channel);
}

NetDeviceContainer
MySimpleNetDeviceHelper::Install (Ptr<Node> node, Ptr<MySimpleChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer 
MySimpleNetDeviceHelper::Install (const NodeContainer &c) const
{
  Ptr<MySimpleChannel> channel = m_channelFactory.Create<MySimpleChannel> ();

  return Install (c, channel);
}

NetDeviceContainer 
MySimpleNetDeviceHelper::Install (const NodeContainer &c, Ptr<MySimpleChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

Ptr<NetDevice>
MySimpleNetDeviceHelper::InstallPriv (Ptr<Node> node, Ptr<MySimpleChannel> channel) const
{
  Ptr<MySimpleNetDevice> device = m_deviceFactory.Create<MySimpleNetDevice> ();
  node->AddDevice (device);
  device->SetNode (node);
  device->SetChannel (channel);
  Ptr<Queue> queue = m_queueFactory.Create<Queue> ();
  device->SetQueue (queue);
  return device;
}