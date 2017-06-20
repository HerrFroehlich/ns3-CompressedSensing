/**
* \file simple-device-channel-example.cc
*
* \author Tobias Waurick
* \date 21.05.17
*
* A simple example to demonstrate the simple net device and channel
*/
#include "ns3/my-simple-channel.h"
#include "ns3/core-module.h"
#include "ns3/my-simple-net-device-helper.h"
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SimpleNetDeviceChannelExample");

static bool
pReceive(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t idUnused, const Address &adrUnused)
{
	cout << Simulator::Now()<< ": Node " <<dev->GetNode()->GetId() 
		 << " received " << p->ToString() ;
	cout << endl;
	return true;
}

int main(int argc, char *argv[])
{
  bool verbose = false;

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.Parse(argc, argv);

  if (verbose)
    {
	LogComponentEnable("MySimpleNetDevice", LOG_LEVEL_FUNCTION);
	LogComponentEnable("MySimpleChannel", LOG_LEVEL_FUNCTION);
	}

	uint8_t data = 5;
	Packet::EnablePrinting();
	Ptr<Packet> p = Create<Packet>(&data, 1);
	Ptr<MySimpleChannel>
		channel = CreateObject<MySimpleChannel>();
	Ptr<MySimpleNetDevice> devA = CreateObject<MySimpleNetDevice>(),
						   devB = CreateObject<MySimpleNetDevice>();

	NodeContainer nodes;
	nodes.Create(2);
	channel->Add(devA);
	channel->Add(devB);
	devA->SetNode(nodes.Get(0));
	devA->SetReceiveCallback(MakeCallback(&pReceive));
	devB->SetNode(nodes.Get(2));
	devB->SetReceiveCallback(MakeCallback(&pReceive));
	channel->TransmitStart(p, Seconds(1), devA);
	channel->TransmitStart(p, Seconds(1), devB);

	//now with helper
	MySimpleNetDeviceHelper hlpr;
	Ptr<NetDevice> device;
	hlpr.SetChannelAttribute("Delay", TimeValue(Seconds(5)));
	hlpr.SetDeviceAttribute("DataRate", DataRateValue(100));
	hlpr.Install(nodes);
	device = nodes.Get(0)->GetDevice(0);
	device->SetReceiveCallback(MakeCallback(&pReceive));
	Callback<bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&> cb =
		MakeCallback(&pReceive);
	device = nodes.Get(1)->GetDevice(0);
	device->SetReceiveCallback(cb);
	device = nodes.Get(0)->GetDevice(0);
	device->Send(p, Address(), 0);

	Simulator::Run();
	Simulator::Destroy();
}