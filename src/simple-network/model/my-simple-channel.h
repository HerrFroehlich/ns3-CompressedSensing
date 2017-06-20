/**
* \file MySimpleChannel.h
*
* \author Tobias Waurick
* \date 14.05.2017
*
*/
#ifndef MY_SIMPLE_CHANNEL_H
#define MY_SIMPLE_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "my-simple-net-device.h"

#define NOF_DEVICES 2

using namespace ns3;
/**
* \class MySimpleChannel
*
* \brief simple bidirectional point to point channel with delay
*
* general concept idea taken from point-to-point-channel
*
* \author Tobias Waurick
* \date 14.05.17
*/
class MySimpleChannel  : public Channel 
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId(void);
  /**
  *  \brief create the simple channel
  *
  *  by default delay is set to zero
  */
  MySimpleChannel();

  /**
   * Attached a net device to the channel.
   *
   * \param device the device to attach to the channel
   */ 
  virtual void Add (Ptr<MySimpleNetDevice> device);

  /**
   * \brief Transmit a packet over this channel
   * \param p Packet to transmit
   * \param txTime Transmit time to apply
   * \param sender sending net device
   *
   */
  virtual void TransmitStart (Ptr<Packet> p, Time txTime, Ptr<MySimpleNetDevice> sender);  
  
  /**
   * \brief Get number of devices on this channel
   * \returns number of devices on this channel
   */
  virtual uint32_t GetNDevices (void) const;

 /**
   * \brief Get NetDevice corresponding to index i on this channel
   * \param i Index number of the device requested
   * \returns Ptr to NetDevice requested
   */
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

private:

  Ptr<MySimpleNetDevice>  m_devices [NOF_DEVICES]; //!< devices connected by the channel
  Time m_delay; /**< delay of the channel*/
  uint32_t m_nDevices; /**< currently connected devices*/

};



#endif // MY_SIMPLE_CHANNEL_H 