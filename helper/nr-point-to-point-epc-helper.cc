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
*/

#include <ns3/nr-point-to-point-epc-helper.h>
#include <ns3/log.h>
#include <ns3/object.h>
#include <ns3/node-container.h>
#include <ns3/net-device-container.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/mmwave-enb-net-device.h>
#include <ns3/epc-x2.h>
#include <ns3/mmwave-ue-net-device.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/epc-ue-nas.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrPointToPointEpcHelper");

NS_OBJECT_ENSURE_REGISTERED (NrPointToPointEpcHelper);

NrPointToPointEpcHelper::NrPointToPointEpcHelper () : PointToPointEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

NrPointToPointEpcHelper::~NrPointToPointEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
NrPointToPointEpcHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrPointToPointEpcHelper")
    .SetParent<PointToPointEpcHelper> ()
    .SetGroupName ("nr")
    .AddConstructor<NrPointToPointEpcHelper> ()
  ;
  return tid;
}

void
NrPointToPointEpcHelper::DoAddX2Interface (const Ptr<EpcX2> &enb1X2, const Ptr<NetDevice> &enb1LteDev,
                                           const Ipv4Address &enb1X2Address,
                                           const Ptr<EpcX2> &enb2X2, const Ptr<NetDevice> &enb2LteDev,
                                           const Ipv4Address &enb2X2Address) const
{
  NS_LOG_FUNCTION (this);

  Ptr<MmWaveEnbNetDevice> enb1LteDevice = enb1LteDev->GetObject<MmWaveEnbNetDevice> ();
  Ptr<MmWaveEnbNetDevice> enb2LteDevice = enb2LteDev->GetObject<MmWaveEnbNetDevice> ();
  uint16_t enb1CellId = enb1LteDevice->GetCellId ();
  uint16_t enb2CellId = enb2LteDevice->GetCellId ();

  NS_ABORT_IF (enb1LteDevice == nullptr);
  NS_ABORT_IF (enb2LteDevice == nullptr);

  NS_LOG_LOGIC ("LteEnbNetDevice #1 = " << enb1LteDev << " - CellId = " << enb1CellId);
  NS_LOG_LOGIC ("LteEnbNetDevice #2 = " << enb2LteDev << " - CellId = " << enb2CellId);

  enb1X2->AddX2Interface (enb1CellId, enb1X2Address, enb2CellId, enb2X2Address);
  enb2X2->AddX2Interface (enb2CellId, enb2X2Address, enb1CellId, enb1X2Address);

  enb1LteDevice->GetRrc ()->AddX2Neighbour (enb2CellId);
  enb2LteDevice->GetRrc ()->AddX2Neighbour (enb1CellId);
}

void
NrPointToPointEpcHelper::DoActivateEpsBearerForUe (const Ptr<NetDevice> &ueDevice,
                                                   const Ptr<EpcTft> &tft,
                                                   const EpsBearer &bearer) const
{
  Ptr<MmWaveUeNetDevice> ueLteDevice = ueDevice->GetObject<MmWaveUeNetDevice> ();
  if (ueLteDevice)
    {
      Simulator::ScheduleNow (&EpcUeNas::ActivateEpsBearer, ueLteDevice->GetNas (), bearer, tft);
    }
  else
    {
      NS_FATAL_ERROR ("What kind of device are you trying to pass to the NR helper?");
    }
}


} // namespace ns3