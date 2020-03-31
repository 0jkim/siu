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
#include "mmwave-mac-scheduler-ofdma-mr.h"
#include "mmwave-mac-scheduler-ue-info-mr.h"
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveMacSchedulerOfdmaMR");
NS_OBJECT_ENSURE_REGISTERED (MmWaveMacSchedulerOfdmaMR);

TypeId
MmWaveMacSchedulerOfdmaMR::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveMacSchedulerOfdmaMR")
    .SetParent<MmWaveMacSchedulerOfdmaRR> ()
    .AddConstructor<MmWaveMacSchedulerOfdmaMR> ()
  ;
  return tid;
}

MmWaveMacSchedulerOfdmaMR::MmWaveMacSchedulerOfdmaMR () : MmWaveMacSchedulerOfdmaRR ()
{

}

std::shared_ptr<MmWaveMacSchedulerUeInfo>
MmWaveMacSchedulerOfdmaMR::CreateUeRepresentation (const MmWaveMacCschedSapProvider::CschedUeConfigReqParameters &params) const
{
  NS_LOG_FUNCTION (this);
  return std::make_shared <MmWaveMacSchedulerUeInfoMR> (params.m_rnti, params.m_beamId,
                                                        std::bind (&MmWaveMacSchedulerOfdmaMR::GetNumRbPerRbg, this));
}

std::function<bool(const MmWaveMacSchedulerNs3::UePtrAndBufferReq &lhs,
                   const MmWaveMacSchedulerNs3::UePtrAndBufferReq &rhs )>
MmWaveMacSchedulerOfdmaMR::GetUeCompareDlFn () const
{
  return MmWaveMacSchedulerUeInfoMR::CompareUeWeightsDl;
}

std::function<bool (const MmWaveMacSchedulerNs3::UePtrAndBufferReq &lhs,
                    const MmWaveMacSchedulerNs3::UePtrAndBufferReq &rhs)>
MmWaveMacSchedulerOfdmaMR::GetUeCompareUlFn () const
{
  return MmWaveMacSchedulerUeInfoMR::CompareUeWeightsUl;
}

} // namespace ns3