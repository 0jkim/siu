/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef SRC_MMWAVE_MODEL_MMWAVE_NET_DEVICE_H_
#define SRC_MMWAVE_MODEL_MMWAVE_NET_DEVICE_H_

#include <ns3/net-device.h>
#include "mmwave-phy.h"

namespace ns3 {

class Node;
class Packet;

/**
 * \ingroup ue
 * \ingroup gnb
 * \brief The MmWaveNetDevice class
 *
 * This is the base class for MmWaveUeNetDevice and MmWaveEnbNetDevice.
 */
class MmWaveNetDevice : public NetDevice
{
public:
  /**
   * \brief GetTypeId
   * \return the object type id
   */
  static TypeId GetTypeId (void);

  /**
   * \brief MmWaveNetDevice
   */
  MmWaveNetDevice (void);
  /**
   * \brief ~MmWaveNetDevice
   */
  virtual ~MmWaveNetDevice (void);

  // inherit
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsBridge (void) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual void SetReceiveCallback (ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  void Receive (Ptr<Packet> p);

protected:
  virtual void DoDispose (void);

  NetDevice::ReceiveCallback m_rxCallback;
  virtual bool DoSend (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) = 0;

private:
  Mac48Address m_macaddress;
  Ptr<Node> m_node;
  mutable uint16_t m_mtu;
  bool m_linkUp;
  uint32_t m_ifIndex;

};

}

#endif /* SRC_MMWAVE_MODEL_MMWAVE_NET_DEVICE_H_ */