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

#define NS_LOG_APPEND_CONTEXT                                            \
  do                                                                     \
    {                                                                    \
      std::clog << " [ CellId " << GetCellId() << ", bwpId "             \
                << GetBwpId () << "] ";                                  \
    }                                                                    \
  while (false);

#include "mmwave-enb-mac.h"
#include "mmwave-phy-mac-common.h"
#include "mmwave-mac-pdu-header.h"
#include "mmwave-mac-sched-sap.h"
#include "mmwave-mac-scheduler.h"
#include "mmwave-control-messages.h"
#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/log.h>
#include <ns3/spectrum-model.h>
#include <algorithm>
#include "beam-id.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("MmWaveEnbMac");

NS_OBJECT_ENSURE_REGISTERED (MmWaveEnbMac);



// //////////////////////////////////////
// member SAP forwarders
// //////////////////////////////////////


class MmWaveEnbMacMemberEnbCmacSapProvider : public LteEnbCmacSapProvider
{
public:
  MmWaveEnbMacMemberEnbCmacSapProvider (MmWaveEnbMac* mac);

  // inherited from LteEnbCmacSapProvider
  virtual void ConfigureMac (uint16_t ulBandwidth, uint16_t dlBandwidth);
  virtual void AddUe (uint16_t rnti);
  virtual void RemoveUe (uint16_t rnti);
  virtual void AddLc (LcInfo lcinfo, LteMacSapUser* msu);
  virtual void ReconfigureLc (LcInfo lcinfo);
  virtual void ReleaseLc (uint16_t rnti, uint8_t lcid);
  virtual void UeUpdateConfigurationReq (UeConfig params);
  virtual RachConfig GetRachConfig ();
  virtual AllocateNcRaPreambleReturnValue AllocateNcRaPreamble (uint16_t rnti);
private:
  MmWaveEnbMac* m_mac;
};


MmWaveEnbMacMemberEnbCmacSapProvider::MmWaveEnbMacMemberEnbCmacSapProvider (MmWaveEnbMac* mac)
  : m_mac (mac)
{
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::ConfigureMac (uint16_t ulBandwidth, uint16_t dlBandwidth)
{
  m_mac->DoConfigureMac (ulBandwidth, dlBandwidth);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::AddUe (uint16_t rnti)
{
  m_mac->DoAddUe (rnti);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::RemoveUe (uint16_t rnti)
{
  m_mac->DoRemoveUe (rnti);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::AddLc (LcInfo lcinfo, LteMacSapUser* msu)
{
  m_mac->DoAddLc (lcinfo, msu);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::ReconfigureLc (LcInfo lcinfo)
{
  m_mac->DoReconfigureLc (lcinfo);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::ReleaseLc (uint16_t rnti, uint8_t lcid)
{
  m_mac->DoReleaseLc (rnti, lcid);
}

void
MmWaveEnbMacMemberEnbCmacSapProvider::UeUpdateConfigurationReq (UeConfig params)
{
  m_mac->UeUpdateConfigurationReq (params);
}

LteEnbCmacSapProvider::RachConfig
MmWaveEnbMacMemberEnbCmacSapProvider::GetRachConfig ()
{
  return m_mac->DoGetRachConfig ();
}

LteEnbCmacSapProvider::AllocateNcRaPreambleReturnValue
MmWaveEnbMacMemberEnbCmacSapProvider::AllocateNcRaPreamble (uint16_t rnti)
{
  return m_mac->DoAllocateNcRaPreamble (rnti);
}



// SAP interface between ENB PHY AND MAC
// PHY is provider and MAC is user of its service following OSI model.
// However, PHY may request some information from MAC.
class MmWaveMacEnbMemberPhySapUser : public MmWaveEnbPhySapUser
{
public:
  MmWaveMacEnbMemberPhySapUser (MmWaveEnbMac* mac);

  virtual void ReceivePhyPdu (Ptr<Packet> p) override;

  virtual void ReceiveControlMessage (Ptr<MmWaveControlMessage> msg) override;

  virtual void SlotDlIndication (const SfnSf &, LteNrTddSlotType) override;

  virtual void SlotUlIndication (const SfnSf &, LteNrTddSlotType) override;

  virtual void SetCurrentSfn (const SfnSf &) override;

  virtual void UlCqiReport (MmWaveMacSchedSapProvider::SchedUlCqiInfoReqParameters cqi) override;

  virtual void ReceiveRachPreamble (uint32_t raId) override;

  virtual void UlHarqFeedback (UlHarqInfo params) override;

  virtual void BeamChangeReport (BeamId beamId, uint8_t rnti) override;

  virtual uint32_t GetNumRbPerRbg () const override;

  virtual std::shared_ptr<DciInfoElementTdma> GetDlCtrlDci () const override;
  virtual std::shared_ptr<DciInfoElementTdma> GetUlCtrlDci () const override;

private:
  MmWaveEnbMac* m_mac;
};

MmWaveMacEnbMemberPhySapUser::MmWaveMacEnbMemberPhySapUser (MmWaveEnbMac* mac)
  : m_mac (mac)
{

}

void
MmWaveMacEnbMemberPhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
  m_mac->DoReceivePhyPdu (p);
}

void
MmWaveMacEnbMemberPhySapUser::ReceiveControlMessage (Ptr<MmWaveControlMessage> msg)
{
  m_mac->DoReceiveControlMessage (msg);
}

void
MmWaveMacEnbMemberPhySapUser::SlotDlIndication (const SfnSf &sfn, LteNrTddSlotType type)
{
  m_mac->DoSlotDlIndication (sfn, type);
}

void
MmWaveMacEnbMemberPhySapUser::SlotUlIndication (const SfnSf &sfn, LteNrTddSlotType type)
{
  m_mac->DoSlotUlIndication (sfn, type);
}

void
MmWaveMacEnbMemberPhySapUser::SetCurrentSfn (const SfnSf &sfn)
{
  m_mac->SetCurrentSfn (sfn);
}

void
MmWaveMacEnbMemberPhySapUser::UlCqiReport (MmWaveMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi)
{
  m_mac->DoUlCqiReport (ulcqi);
}

void
MmWaveMacEnbMemberPhySapUser::ReceiveRachPreamble (uint32_t raId)
{
  m_mac->ReceiveRachPreamble (raId);
}

void
MmWaveMacEnbMemberPhySapUser::UlHarqFeedback (UlHarqInfo params)
{
  m_mac->DoUlHarqFeedback (params);
}

void
MmWaveMacEnbMemberPhySapUser::BeamChangeReport (BeamId beamId, uint8_t rnti)
{
  m_mac->BeamChangeReport (beamId, rnti);
}

uint32_t
MmWaveMacEnbMemberPhySapUser::GetNumRbPerRbg () const
{
  return m_mac->GetNumRbPerRbg();
}

std::shared_ptr<DciInfoElementTdma>
MmWaveMacEnbMemberPhySapUser::GetDlCtrlDci() const
{
  return m_mac->GetDlCtrlDci ();
}

std::shared_ptr<DciInfoElementTdma>
MmWaveMacEnbMemberPhySapUser::GetUlCtrlDci() const
{
  return m_mac->GetUlCtrlDci ();
}


// MAC Sched

class MmWaveMacMemberMacSchedSapUser : public MmWaveMacSchedSapUser
{
public:
  MmWaveMacMemberMacSchedSapUser (MmWaveEnbMac* mac);
  virtual void SchedConfigInd (const struct SchedConfigIndParameters& params) override;
  virtual Ptr<const SpectrumModel> GetSpectrumModel () const override;
  virtual uint32_t GetNumRbPerRbg () const override;
  virtual uint8_t GetNumHarqProcess () const override;
  virtual uint16_t GetBwpId () const override;
  virtual uint16_t GetCellId () const override;
  virtual uint32_t GetSymbolsPerSlot () const override;
  virtual Time GetSlotPeriod () const override;
private:
  MmWaveEnbMac* m_mac;
};

MmWaveMacMemberMacSchedSapUser::MmWaveMacMemberMacSchedSapUser (MmWaveEnbMac* mac)
  : m_mac (mac)
{
  //  Some blank spaces
}

void
MmWaveMacMemberMacSchedSapUser::SchedConfigInd (const struct SchedConfigIndParameters& params)
{
  m_mac->DoSchedConfigIndication (params);
}

Ptr<const SpectrumModel>
MmWaveMacMemberMacSchedSapUser::GetSpectrumModel () const
{
  return m_mac->m_phySapProvider->GetSpectrumModel (); //  MAC forwards the call from scheduler to PHY; i.e. this function connects two providers of MAC: scheduler and PHY
}

uint32_t
MmWaveMacMemberMacSchedSapUser::GetNumRbPerRbg () const
{
  return m_mac->GetNumRbPerRbg ();
}

uint8_t
MmWaveMacMemberMacSchedSapUser::GetNumHarqProcess () const
{
  return m_mac->GetNumHarqProcess();
}

uint16_t
MmWaveMacMemberMacSchedSapUser::GetBwpId() const
{
  return m_mac->GetBwpId ();
}

uint16_t
MmWaveMacMemberMacSchedSapUser::GetCellId() const
{
  return m_mac->GetCellId ();
}

uint32_t
MmWaveMacMemberMacSchedSapUser::GetSymbolsPerSlot() const
{
  return m_mac->m_phySapProvider->GetSymbolsPerSlot ();
}

Time
MmWaveMacMemberMacSchedSapUser::GetSlotPeriod() const
{
  return m_mac->m_phySapProvider->GetSlotPeriod ();
}

class MmWaveMacMemberMacCschedSapUser : public MmWaveMacCschedSapUser
{
public:
  MmWaveMacMemberMacCschedSapUser (MmWaveEnbMac* mac);

  virtual void CschedCellConfigCnf (const struct MmWaveMacCschedSapUser::CschedCellConfigCnfParameters& params);
  virtual void CschedUeConfigCnf (const struct MmWaveMacCschedSapUser::CschedUeConfigCnfParameters& params);
  virtual void CschedLcConfigCnf (const struct MmWaveMacCschedSapUser::CschedLcConfigCnfParameters& params);
  virtual void CschedLcReleaseCnf (const struct MmWaveMacCschedSapUser::CschedLcReleaseCnfParameters& params);
  virtual void CschedUeReleaseCnf (const struct MmWaveMacCschedSapUser::CschedUeReleaseCnfParameters& params);
  virtual void CschedUeConfigUpdateInd (const struct MmWaveMacCschedSapUser::CschedUeConfigUpdateIndParameters& params);
  virtual void CschedCellConfigUpdateInd (const struct MmWaveMacCschedSapUser::CschedCellConfigUpdateIndParameters& params);

private:
  MmWaveEnbMac* m_mac;
};


MmWaveMacMemberMacCschedSapUser::MmWaveMacMemberMacCschedSapUser (MmWaveEnbMac* mac)
  : m_mac (mac)
{
}

void
MmWaveMacMemberMacCschedSapUser::CschedCellConfigCnf (const struct CschedCellConfigCnfParameters& params)
{
  m_mac->DoCschedCellConfigCnf (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedUeConfigCnf (const struct CschedUeConfigCnfParameters& params)
{
  m_mac->DoCschedUeConfigCnf (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedLcConfigCnf (const struct CschedLcConfigCnfParameters& params)
{
  m_mac->DoCschedLcConfigCnf (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedLcReleaseCnf (const struct CschedLcReleaseCnfParameters& params)
{
  m_mac->DoCschedLcReleaseCnf (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedUeReleaseCnf (const struct CschedUeReleaseCnfParameters& params)
{
  m_mac->DoCschedUeReleaseCnf (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedUeConfigUpdateInd (const struct CschedUeConfigUpdateIndParameters& params)
{
  m_mac->DoCschedUeConfigUpdateInd (params);
}

void
MmWaveMacMemberMacCschedSapUser::CschedCellConfigUpdateInd (const struct CschedCellConfigUpdateIndParameters& params)
{
  m_mac->DoCschedCellConfigUpdateInd (params);
}

TypeId
MmWaveEnbMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveEnbMac")
    .SetParent<Object> ()
    .AddConstructor<MmWaveEnbMac> ()
    .AddAttribute ("NumRbPerRbg",
                   "Number of resource blocks per resource block group.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&MmWaveEnbMac::SetNumRbPerRbg,
                                         &MmWaveEnbMac::GetNumRbPerRbg),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumHarqProcess",
                   "Number of concurrent stop-and-wait Hybrid ARQ processes per user",
                    UintegerValue (20),
                    MakeUintegerAccessor (&MmWaveEnbMac::SetNumHarqProcess,
                                          &MmWaveEnbMac::GetNumHarqProcess),
                    MakeUintegerChecker<uint8_t> ())
    .AddTraceSource ("DlScheduling",
                     "Information regarding DL scheduling.",
                     MakeTraceSourceAccessor (&MmWaveEnbMac::m_dlScheduling),
                     "ns3::LteEnbMac::DlSchedulingTracedCallback")
    .AddTraceSource ("SrReq",
                     "Information regarding received scheduling request.",
                     MakeTraceSourceAccessor (&MmWaveEnbMac::m_srCallback),
                     "ns3::MmWaveEnbMac::SrTracedCallback")
    .AddTraceSource ("EnbMacRxedCtrlMsgsTrace",
                     "Enb MAC Rxed Control Messages Traces.",
                     MakeTraceSourceAccessor (&MmWaveEnbMac::m_macRxedCtrlMsgsTrace),
                     "ns3::MmWaveMacRxTrace::RxedEnbMacCtrlMsgsTracedCallback")
    .AddTraceSource ("EnbMacTxedCtrlMsgsTrace",
                     "Enb MAC Txed Control Messages Traces.",
                     MakeTraceSourceAccessor (&MmWaveEnbMac::m_macTxedCtrlMsgsTrace),
                     "ns3::MmWaveMacRxTrace::TxedEnbMacCtrlMsgsTracedCallback")
    .AddTraceSource ("DlHarqFeedback",
                     "Harq feedback.",
                      MakeTraceSourceAccessor (&MmWaveEnbMac::m_dlHarqFeedback),
                     "ns3::MmWaveEnbMac::DlHarqFeedbackTracedCallback")
  ;
  return tid;
}

MmWaveEnbMac::MmWaveEnbMac (void) : Object ()
{
  NS_LOG_FUNCTION (this);
  m_cmacSapProvider = new MmWaveEnbMacMemberEnbCmacSapProvider (this);
  m_macSapProvider = new EnbMacMemberLteMacSapProvider<MmWaveEnbMac> (this);
  m_phySapUser = new MmWaveMacEnbMemberPhySapUser (this);
  m_macSchedSapUser = new MmWaveMacMemberMacSchedSapUser (this);
  m_macCschedSapUser = new MmWaveMacMemberMacCschedSapUser (this);
  m_ccmMacSapProvider = new MemberLteCcmMacSapProvider<MmWaveEnbMac> (this);
}

MmWaveEnbMac::~MmWaveEnbMac (void)
{
  NS_LOG_FUNCTION (this);
  m_dlCqiReceived.clear ();
  m_ulCqiReceived.clear ();
  m_ulCeReceived.clear ();
  m_miDlHarqProcessesPackets.clear ();
  delete m_macSapProvider;
  delete m_cmacSapProvider;
  delete m_macSchedSapUser;
  delete m_macCschedSapUser;
  delete m_phySapUser;
}

void
MmWaveEnbMac::SetNumRbPerRbg (uint32_t rbgSize)
{
  NS_ABORT_MSG_IF (m_numRbPerRbg !=-1, "This attribute can not be reconfigured");
  m_numRbPerRbg = rbgSize;
}

uint32_t
MmWaveEnbMac::GetNumRbPerRbg (void) const
{
  return m_numRbPerRbg;
}

/**
 * \brief Sets the number of HARQ processes
 * \param numHarqProcesses the maximum number of harq processes
 */
void
MmWaveEnbMac::SetNumHarqProcess (uint8_t numHarqProcess)
{
  m_numHarqProcess = numHarqProcess;
}

/**
 * \return number of HARQ processes
 */
uint8_t
MmWaveEnbMac::GetNumHarqProcess () const
{
  return m_numHarqProcess;
}

void
MmWaveEnbMac::ReceiveRachPreamble (uint32_t raId)
{
  Ptr<MmWaveRachPreambleMessage> rachMsg = Create<MmWaveRachPreambleMessage> ();
  rachMsg->SetSourceBwp (GetBwpId ());
  m_macRxedCtrlMsgsTrace (m_currentSlot, raId, GetBwpId (), rachMsg);

  ++m_receivedRachPreambleCount[raId];
}

LteMacSapProvider*
MmWaveEnbMac::GetMacSapProvider (void)
{
  return m_macSapProvider;
}

LteEnbCmacSapProvider*
MmWaveEnbMac::GetEnbCmacSapProvider (void)
{
  return m_cmacSapProvider;
}
void
MmWaveEnbMac::SetEnbCmacSapUser (LteEnbCmacSapUser* s)
{
  m_cmacSapUser = s;
}

void
MmWaveEnbMac::SetLteCcmMacSapUser (LteCcmMacSapUser* s)
{
  m_ccmMacSapUser = s;
}


LteCcmMacSapProvider*
MmWaveEnbMac::GetLteCcmMacSapProvider ()
{
  NS_LOG_FUNCTION (this);
  return m_ccmMacSapProvider;
}

void
MmWaveEnbMac::SetCurrentSfn (const SfnSf &sfnSf)
{
  NS_LOG_FUNCTION (this);
  m_currentSlot = sfnSf;
}

void
MmWaveEnbMac::DoSlotDlIndication (const SfnSf &sfnSf, LteNrTddSlotType type)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Perform things on DL, slot on the air: " << sfnSf);

  // --- DOWNLINK ---
  // Send Dl-CQI info to the scheduler    if(m_dlCqiReceived.size () > 0)
  {
    MmWaveMacSchedSapProvider::SchedDlCqiInfoReqParameters dlCqiInfoReq;
    dlCqiInfoReq.m_sfnsf = sfnSf;

    dlCqiInfoReq.m_cqiList.insert (dlCqiInfoReq.m_cqiList.begin (), m_dlCqiReceived.begin (), m_dlCqiReceived.end ());
    m_dlCqiReceived.erase (m_dlCqiReceived.begin (), m_dlCqiReceived.end ());

    m_macSchedSapProvider->SchedDlCqiInfoReq (dlCqiInfoReq);

    for (const auto & v : dlCqiInfoReq.m_cqiList)
      {
        Ptr<MmWaveDlCqiMessage> msg = Create<MmWaveDlCqiMessage> ();
        msg->SetDlCqi (v);
        m_macRxedCtrlMsgsTrace (m_currentSlot, v.m_rnti, GetBwpId (), msg);
      }
  }

  if (!m_receivedRachPreambleCount.empty ())
    {
      // process received RACH preambles and notify the scheduler
      MmWaveMacSchedSapProvider::SchedDlRachInfoReqParameters rachInfoReqParams;

      for (auto it = m_receivedRachPreambleCount.begin ();
           it != m_receivedRachPreambleCount.end ();
           ++it)
        {
          uint16_t rnti = m_cmacSapUser->AllocateTemporaryCellRnti ();

          NS_LOG_INFO ("Informing MAC scheduler of the RACH preamble for " <<
                       static_cast<uint16_t> (it->first) << " in slot " << sfnSf);
          RachListElement_s rachLe;
          rachLe.m_rnti = rnti;
          rachLe.m_estimatedSize = 144; // to be confirmed
          rachInfoReqParams.m_rachList.emplace_back (rachLe);

          m_rapIdRntiMap.insert (std::make_pair (rnti, it->first));
        }
      m_receivedRachPreambleCount.clear ();
      m_macSchedSapProvider->SchedDlRachInfoReq (rachInfoReqParams);
    }

  MmWaveMacSchedSapProvider::SchedDlTriggerReqParameters dlParams;

  dlParams.m_slotType = type;
  dlParams.m_snfSf = sfnSf;

  // Forward DL HARQ feedbacks collected during last subframe TTI
  if (m_dlHarqInfoReceived.size () > 0)
    {
      dlParams.m_dlHarqInfoList = m_dlHarqInfoReceived;
      // empty local buffer
      m_dlHarqInfoReceived.clear ();

      for (const auto & v : dlParams.m_dlHarqInfoList)
        {
          Ptr<MmWaveDlHarqFeedbackMessage> msg = Create <MmWaveDlHarqFeedbackMessage> ();
          msg->SetDlHarqFeedback (v);
          m_macRxedCtrlMsgsTrace (m_currentSlot, v.m_rnti, GetBwpId (), msg);
        }
    }

  {
    for (const auto & ue : m_rlcAttached)
      {
        MmWaveMacCschedSapProvider::CschedUeConfigReqParameters params;
        params.m_rnti = ue.first;
        params.m_beamId = m_phySapProvider->GetBeamId (ue.first);
        params.m_transmissionMode = 0;   // set to default value (SISO) for avoiding random initialization (valgrind error)
        m_macCschedSapProvider->CschedUeConfigReq (params);
      }
  }

  m_macSchedSapProvider->SchedDlTriggerReq (dlParams);
}

void
MmWaveEnbMac::DoSlotUlIndication (const SfnSf &sfnSf, LteNrTddSlotType type)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Perform things on UL, slot on the air: " << sfnSf);

  // --- UPLINK ---
  // Send UL-CQI info to the scheduler
  for (uint16_t i = 0; i < m_ulCqiReceived.size (); i++)
    {
      //m_ulCqiReceived.at (i).m_sfnSf = ((0x3FF & frameNum) << 16) | ((0xFF & subframeNum) << 8) | (0xFF & varTtiNum);
      m_macSchedSapProvider->SchedUlCqiInfoReq (m_ulCqiReceived.at (i));
    }
  m_ulCqiReceived.clear ();

  // Send SR info to the scheduler
  {
    MmWaveMacSchedSapProvider::SchedUlSrInfoReqParameters params;
    params.m_snfSf = m_currentSlot;
    params.m_srList.insert (params.m_srList.begin(), m_srRntiList.begin (), m_srRntiList.end ());
    m_srRntiList.clear();

    m_macSchedSapProvider->SchedUlSrInfoReq (params);

    for (const auto & v : params.m_srList)
      {
        Ptr<MmWaveSRMessage> msg =  Create<MmWaveSRMessage> ();
        msg->SetRNTI (v);
        m_macRxedCtrlMsgsTrace (m_currentSlot, v, GetBwpId (), msg);
      }
  }

  // Send UL BSR reports to the scheduler
  if (m_ulCeReceived.size () > 0)
    {
      MmWaveMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters ulMacReq;
      ulMacReq.m_sfnSf = sfnSf;
      ulMacReq.m_macCeList.insert (ulMacReq.m_macCeList.begin (), m_ulCeReceived.begin (), m_ulCeReceived.end ());
      m_ulCeReceived.erase (m_ulCeReceived.begin (), m_ulCeReceived.end ());
      m_macSchedSapProvider->SchedUlMacCtrlInfoReq (ulMacReq);

      for (const auto & v : ulMacReq.m_macCeList)
        {
          Ptr<MmWaveBsrMessage> msg = Create<MmWaveBsrMessage> ();
          msg->SetBsr (v);
          m_macRxedCtrlMsgsTrace (m_currentSlot, v.m_rnti, GetBwpId (), msg);
        }
    }

  MmWaveMacSchedSapProvider::SchedUlTriggerReqParameters ulParams;

  ulParams.m_snfSf = sfnSf;
  ulParams.m_slotType = type;

  // Forward UL HARQ feebacks collected during last TTI
  if (m_ulHarqInfoReceived.size () > 0)
    {
      ulParams.m_ulHarqInfoList = m_ulHarqInfoReceived;
      // empty local buffer
      m_ulHarqInfoReceived.clear ();
    }

  m_macSchedSapProvider->SchedUlTriggerReq (ulParams);

}

void
MmWaveEnbMac::SetForwardUpCallback (Callback <void, Ptr<Packet> > cb)
{
  m_forwardUpCallback = cb;
}

void
MmWaveEnbMac::ReceiveBsrMessage  (MacCeElement bsr)
{
  NS_LOG_FUNCTION (this);
  // in order to use existing SAP interfaces we need to convert MacCeElement to MacCeListElement_s

  MacCeListElement_s mcle;
  mcle.m_rnti = bsr.m_rnti;
  mcle.m_macCeValue.m_bufferStatus = bsr.m_macCeValue.m_bufferStatus;
  mcle.m_macCeValue.m_crnti = bsr.m_macCeValue.m_crnti;
  mcle.m_macCeValue.m_phr = bsr.m_macCeValue.m_phr;
  mcle.m_macCeValue.m_bufferStatus = bsr.m_macCeValue.m_bufferStatus;

  if (bsr.m_macCeType == MacCeElement::BSR)
    {
      mcle.m_macCeType = MacCeListElement_s::BSR;
    }
  else if (bsr.m_macCeType == MacCeElement::CRNTI)
    {
      mcle.m_macCeType = MacCeListElement_s::CRNTI;
    }
  else if (bsr.m_macCeType == MacCeElement::PHR)
    {
      mcle.m_macCeType = MacCeListElement_s::PHR;
    }

  m_ccmMacSapUser->UlReceiveMacCe (mcle, GetBwpId ());
}

void
MmWaveEnbMac::DoReportMacCeToScheduler (MacCeListElement_s bsr)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (this << " bsr Size " << (uint16_t) m_ulCeReceived.size ());
  uint32_t size = 0;

  //send to LteCcmMacSapUser
  //convert MacCeListElement_s to MacCeElement

  MacCeElement mce;
  mce.m_rnti = bsr.m_rnti;
  mce.m_macCeValue.m_bufferStatus = bsr.m_macCeValue.m_bufferStatus;
  mce.m_macCeValue.m_crnti = bsr.m_macCeValue.m_crnti;
  mce.m_macCeValue.m_phr = bsr.m_macCeValue.m_phr;
  mce.m_macCeValue.m_bufferStatus = bsr.m_macCeValue.m_bufferStatus;

  if (bsr.m_macCeType == MacCeListElement_s::BSR)
    {
      mce.m_macCeType = MacCeElement::BSR;
    }
  else if (bsr.m_macCeType == MacCeListElement_s::CRNTI)
    {
      mce.m_macCeType = MacCeElement::CRNTI;
    }
  else if (bsr.m_macCeType == MacCeListElement_s::PHR)
    {
      mce.m_macCeType = MacCeElement::PHR;
    }

  for (const auto & v : bsr.m_macCeValue.m_bufferStatus)
    {
      size += v;
    }

  m_ulCeReceived.push_back (mce);   // this to called when LteUlCcmSapProvider::ReportMacCeToScheduler is called
  NS_LOG_DEBUG (" Reported by UE " << static_cast<uint32_t> (bsr.m_macCeValue.m_crnti) <<
                " size " << size << " bsr vector ize after push_back " <<
                static_cast<uint32_t> (m_ulCeReceived.size ()));
}

void
MmWaveEnbMac::DoReportSrToScheduler (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  m_srRntiList.push_back (rnti);
  m_srCallback (GetBwpId (), rnti);
}

void
MmWaveEnbMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  LteRadioBearerTag tag;
  p->RemovePacketTag (tag);
  uint16_t rnti = tag.GetRnti ();
  MmWaveMacPduHeader macHeader;
  p->RemoveHeader (macHeader);
  std::map <uint16_t, std::map<uint8_t, LteMacSapUser*> >::iterator rntiIt = m_rlcAttached.find (rnti);
  NS_ASSERT_MSG (rntiIt != m_rlcAttached.end (), "could not find RNTI" << rnti);
  std::vector<MacSubheader> macSubheaders = macHeader.GetSubheaders ();
  uint32_t currPos = 0;
  for (unsigned ipdu = 0; ipdu < macSubheaders.size (); ipdu++)
    {
      if (macSubheaders[ipdu].m_size == 0)
        {
          continue;
        }
      std::map<uint8_t, LteMacSapUser*>::iterator lcidIt = rntiIt->second.find (macSubheaders[ipdu].m_lcid);
      NS_ASSERT_MSG (lcidIt != rntiIt->second.end (), "could not find LCID" << macSubheaders[ipdu].m_lcid);
      Ptr<Packet> rlcPdu;
      if ((p->GetSize () - currPos) < (uint32_t)macSubheaders[ipdu].m_size)
        {
          NS_LOG_ERROR ("Packet size less than specified in MAC header (actual= " \
                        << p->GetSize () << " header= " << (uint32_t)macSubheaders[ipdu].m_size << ")" );
        }
      else if ((p->GetSize () - currPos) > (uint32_t)macSubheaders[ipdu].m_size)
        {
          NS_LOG_DEBUG ("Fragmenting MAC PDU (packet size greater than specified in MAC header (actual= " \
                        << p->GetSize () << " header= " << (uint32_t)macSubheaders[ipdu].m_size << ")" );
          rlcPdu = p->CreateFragment (currPos, macSubheaders[ipdu].m_size);
          currPos += macSubheaders[ipdu].m_size;
          (*lcidIt).second->ReceivePdu (LteMacSapUser::ReceivePduParameters (rlcPdu, rnti, macSubheaders[ipdu].m_lcid));
        }
      else
        {
          rlcPdu = p->CreateFragment (currPos, p->GetSize () - currPos);
          currPos = p->GetSize ();
          (*lcidIt).second->ReceivePdu (LteMacSapUser::ReceivePduParameters (rlcPdu, rnti, macSubheaders[ipdu].m_lcid));
        }
      NS_LOG_DEBUG ("Enb Mac Rx Packet, Rnti:" << rnti << " lcid:" << macSubheaders[ipdu].m_lcid << " size:" << macSubheaders[ipdu].m_size);
    }
}

MmWaveEnbPhySapUser*
MmWaveEnbMac::GetPhySapUser ()
{
  return m_phySapUser;
}

void
MmWaveEnbMac::SetPhySapProvider (MmWavePhySapProvider* ptr)
{
  m_phySapProvider = ptr;
}

MmWaveMacSchedSapUser*
MmWaveEnbMac::GetMmWaveMacSchedSapUser ()
{
  return m_macSchedSapUser;
}

void
MmWaveEnbMac::SetMmWaveMacSchedSapProvider (MmWaveMacSchedSapProvider* ptr)
{
  m_macSchedSapProvider = ptr;
}

MmWaveMacCschedSapUser*
MmWaveEnbMac::GetMmWaveMacCschedSapUser ()
{
  return m_macCschedSapUser;
}

void
MmWaveEnbMac::SetMmWaveMacCschedSapProvider (MmWaveMacCschedSapProvider* ptr)
{
  m_macCschedSapProvider = ptr;
}

void
MmWaveEnbMac::DoUlCqiReport (MmWaveMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi)
{
  if (ulcqi.m_ulCqi.m_type == UlCqiInfo::PUSCH)
    {
      NS_LOG_DEBUG (this << " eNB rxed an PUSCH UL-CQI");
    }
  else if (ulcqi.m_ulCqi.m_type == UlCqiInfo::SRS)
    {
      NS_LOG_DEBUG (this << " eNB rxed an SRS UL-CQI");
    }
  NS_LOG_INFO ("*** UL CQI report SINR " << LteFfConverter::fpS11dot3toDouble (ulcqi.m_ulCqi.m_sinr[0]) <<
               " slot: " << m_currentSlot);

  // NS_ASSERT (ulcqi.m_sfnSf.m_varTtiNum != 0); Now UL data can be the first TTI..
  m_ulCqiReceived.push_back (ulcqi);
}

void
MmWaveEnbMac::DoReceiveControlMessage  (Ptr<MmWaveControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  switch (msg->GetMessageType ())
    {
    case (MmWaveControlMessage::SR):
      {
        // Report it to the CCM. Then he will call the right MAC
        Ptr<MmWaveSRMessage> sr = DynamicCast<MmWaveSRMessage> (msg);
        m_ccmMacSapUser->UlReceiveSr (sr->GetRNTI (), GetBwpId ());
        break;
      }
    case (MmWaveControlMessage::DL_CQI):
      {
        Ptr<MmWaveDlCqiMessage> cqi = DynamicCast<MmWaveDlCqiMessage> (msg);
        DlCqiInfo cqiElement = cqi->GetDlCqi ();
        NS_ASSERT (cqiElement.m_rnti != 0);
        m_dlCqiReceived.push_back (cqiElement);
        break;
      }
    case (MmWaveControlMessage::BSR):
      {
        Ptr<MmWaveBsrMessage> bsr = DynamicCast<MmWaveBsrMessage> (msg);
        ReceiveBsrMessage (bsr->GetBsr ());
        break;
      }
    case (MmWaveControlMessage::DL_HARQ):
      {
        Ptr<MmWaveDlHarqFeedbackMessage> dlharq = DynamicCast<MmWaveDlHarqFeedbackMessage> (msg);
        DoDlHarqFeedback (dlharq->GetDlHarqFeedback ());
        break;
      }
    default:
      NS_LOG_INFO ("Control message not supported/expected");
    }

}

void
MmWaveEnbMac::DoUlHarqFeedback (UlHarqInfo params)
{
  NS_LOG_FUNCTION (this);
  m_ulHarqInfoReceived.push_back (params);
}

void
MmWaveEnbMac::DoDlHarqFeedback (DlHarqInfo params)
{
  NS_LOG_FUNCTION (this);
  // Update HARQ buffer
  std::map <uint16_t, MmWaveDlHarqProcessesBuffer_t>::iterator it =  m_miDlHarqProcessesPackets.find (params.m_rnti);
  NS_ASSERT (it != m_miDlHarqProcessesPackets.end ());

  if (params.m_harqStatus == DlHarqInfo::ACK)
    {
      // discard buffer
      Ptr<PacketBurst> emptyBuf = CreateObject <PacketBurst> ();
      (*it).second.at (params.m_harqProcessId).m_pktBurst = emptyBuf;
      NS_LOG_DEBUG (this << " HARQ-ACK UE " << params.m_rnti << " harqId " << (uint16_t)params.m_harqProcessId);
    }
  else if (params.m_harqStatus == DlHarqInfo::NACK)
    {
      NS_LOG_DEBUG (this << " HARQ-NACK UE " << params.m_rnti << " harqId " << (uint16_t)params.m_harqProcessId);
    }
  else
    {
      NS_FATAL_ERROR (" HARQ functionality not implemented");
    }

  /* trace for HARQ feedback*/
  m_dlHarqFeedback (params);

  m_dlHarqInfoReceived.push_back (params);
}

void
MmWaveEnbMac::DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params)
{
  NS_LOG_FUNCTION (this);
  MmWaveMacSchedSapProvider::SchedDlRlcBufferReqParameters schedParams;
  schedParams.m_logicalChannelIdentity = params.lcid;
  schedParams.m_rlcRetransmissionHolDelay = params.retxQueueHolDelay;
  schedParams.m_rlcRetransmissionQueueSize = params.retxQueueSize;
  schedParams.m_rlcStatusPduSize = params.statusPduSize;
  schedParams.m_rlcTransmissionQueueHolDelay = params.txQueueHolDelay;
  schedParams.m_rlcTransmissionQueueSize = params.txQueueSize;
  schedParams.m_rnti = params.rnti;

  m_macSchedSapProvider->SchedDlRlcBufferReq (schedParams);
}

// forwarded from LteMacSapProvider
void
MmWaveEnbMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  params.componentCarrierId = GetBwpId ();
  // TB UID passed back along with RLC data as HARQ process ID
  uint32_t tbMapKey = ((params.rnti & 0xFFFF) << 8) | (params.harqProcessId & 0xFF);
  std::map<uint32_t, struct MacPduInfo>::iterator it = m_macPduMap.find (tbMapKey);
  if (it == m_macPduMap.end ())
    {
      NS_FATAL_ERROR ("No MAC PDU storage element found for this TB UID/RNTI");
    }
  else
    {
      if (it->second.m_pdu == 0)
        {
          it->second.m_pdu = params.pdu;
        }
      else
        {
          it->second.m_pdu->AddAtEnd (params.pdu);   // append to MAC PDU
        }

      MacSubheader subheader (params.lcid, params.pdu->GetSize ());
      it->second.m_macHeader.AddSubheader (subheader);   // add RLC PDU sub-header into MAC header
      it->second.m_numRlcPdu++;
    }
}

void
MmWaveEnbMac::DoSchedConfigIndication (MmWaveMacSchedSapUser::SchedConfigIndParameters ind)
{
  NS_ASSERT (ind.m_sfnSf.GetNumerology () == m_currentSlot.GetNumerology ());
  std::sort (ind.m_slotAllocInfo.m_varTtiAllocInfo.begin (), ind.m_slotAllocInfo.m_varTtiAllocInfo.end ());

  NS_LOG_DEBUG ("Received from scheduler a new allocation: " << ind.m_slotAllocInfo);

  m_phySapProvider->SetSlotAllocInfo (ind.m_slotAllocInfo);

  // Random Access procedure: send RARs
  Ptr<MmWaveRarMessage> rarMsg = Create<MmWaveRarMessage> ();
  uint16_t raRnti = 1; // NO!! 38.321-5.1.3
  rarMsg->SetRaRnti (raRnti);
  rarMsg->SetSourceBwp (GetBwpId ());
  for (const auto & rarAllocation : ind.m_buildRarList)
    {
      std::map <uint8_t, uint32_t>::iterator itRapId = m_rapIdRntiMap.find (rarAllocation.m_rnti);
      if (itRapId == m_rapIdRntiMap.end ())
        {
          NS_FATAL_ERROR ("Unable to find rapId of RNTI " << rarAllocation.m_rnti);
        }
      MmWaveRarMessage::Rar rar;
      rar.rapId = itRapId->second;
      rar.rarPayload = rarAllocation;
      rarMsg->AddRar (rar);
      NS_LOG_INFO ("In slot " << m_currentSlot <<
                   " send to PHY the RAR message for RNTI " <<
                   rarAllocation.m_rnti << " rapId " << itRapId->second);
      m_macTxedCtrlMsgsTrace (m_currentSlot, rarAllocation.m_rnti, GetBwpId (), rarMsg);
    }

  if (ind.m_buildRarList.size () > 0)
    {
      m_phySapProvider->SendControlMessage (rarMsg);
      m_rapIdRntiMap.clear ();
    }

  for (unsigned islot = 0; islot < ind.m_slotAllocInfo.m_varTtiAllocInfo.size (); islot++)
    {
      VarTtiAllocInfo &varTtiAllocInfo = ind.m_slotAllocInfo.m_varTtiAllocInfo[islot];
      if (varTtiAllocInfo.m_dci->m_type != DciInfoElementTdma::CTRL
          && varTtiAllocInfo.m_dci->m_format == DciInfoElementTdma::DL)
        {
          uint16_t rnti = varTtiAllocInfo.m_dci->m_rnti;
          std::map <uint16_t, std::map<uint8_t, LteMacSapUser*> >::iterator rntiIt = m_rlcAttached.find (rnti);
          if (rntiIt == m_rlcAttached.end ())
            {
              NS_FATAL_ERROR ("Scheduled UE " << rntiIt->first << " not attached");
            }
          else
            {

              // Call RLC entities to generate RLC PDUs
              auto dciElem = varTtiAllocInfo.m_dci;
              uint8_t tbUid = dciElem->m_harqProcess;

              // update Harq Processes
              if (dciElem->m_ndi == 1)
                {
                  NS_ASSERT (dciElem->m_format == DciInfoElementTdma::DL);
                  std::vector<RlcPduInfo> &rlcPduInfo = varTtiAllocInfo.m_rlcPduInfo;
                  NS_ASSERT (rlcPduInfo.size () > 0);
                  MacPduInfo macPduInfo (ind.m_sfnSf, rlcPduInfo.size (), *dciElem.get ());
                  // insert into MAC PDU map
                  uint32_t tbMapKey = ((rnti & 0xFFFF) << 8) | (tbUid & 0xFF);
                  std::pair <std::map<uint32_t, struct MacPduInfo>::iterator, bool> mapRet =
                    m_macPduMap.insert (std::pair<uint32_t, struct MacPduInfo> (tbMapKey, macPduInfo));
                  if (!mapRet.second)
                    {
                      NS_FATAL_ERROR ("MAC PDU map element exists");
                    }

                  // new data -> force emptying correspondent harq pkt buffer
                  std::map <uint16_t, MmWaveDlHarqProcessesBuffer_t>::iterator harqIt = m_miDlHarqProcessesPackets.find (rnti);
                  NS_ASSERT (harqIt != m_miDlHarqProcessesPackets.end ());
                  Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
                  harqIt->second.at (tbUid).m_pktBurst = pb;
                  harqIt->second.at (tbUid).m_lcidList.clear ();

                  std::map<uint32_t, struct MacPduInfo>::iterator pduMapIt = mapRet.first;
                  pduMapIt->second.m_numRlcPdu = 0;
                  for (unsigned int ipdu = 0; ipdu < rlcPduInfo.size (); ipdu++)
                    {
                      NS_ASSERT_MSG (rntiIt != m_rlcAttached.end (), "could not find RNTI" << rnti);
                      std::map<uint8_t, LteMacSapUser*>::iterator lcidIt = rntiIt->second.find (rlcPduInfo[ipdu].m_lcid);
                      NS_ASSERT_MSG (lcidIt != rntiIt->second.end (), "could not find LCID" << rlcPduInfo[ipdu].m_lcid);
                      NS_LOG_DEBUG ("Notifying RLC of TX opportunity for TB " << (unsigned int)tbUid << " PDU num " << ipdu << " size " << (unsigned int) rlcPduInfo[ipdu].m_size);
                      MacSubheader subheader (rlcPduInfo[ipdu].m_lcid, rlcPduInfo[ipdu].m_size);

                      // The MAC and RLC already consider 2 bytes for the header.
                      // that's a repetition, and prevent transmitting very small
                      // portions.
                      //(*lcidIt).second->NotifyTxOpportunity ((rlcPduInfo[ipdu].m_size)-subheader.GetSize (), 0, tbUid, GetBwpId (), rnti, rlcPduInfo[ipdu].m_lcid);

                      (*lcidIt).second->NotifyTxOpportunity (LteMacSapUser::TxOpportunityParameters ((rlcPduInfo[ipdu].m_size), 0, tbUid, GetBwpId (), rnti, rlcPduInfo[ipdu].m_lcid));
                      harqIt->second.at (tbUid).m_lcidList.push_back (rlcPduInfo[ipdu].m_lcid);
                    }

                  if (pduMapIt->second.m_numRlcPdu == 0)
                    {
                      MacSubheader subheader (3, 0);    // add subheader for empty packet
                      pduMapIt->second.m_macHeader.AddSubheader (subheader);
                    }
                  pduMapIt->second.m_pdu->AddHeader (pduMapIt->second.m_macHeader);

                  MmWaveMacPduHeader hdrTst;
                  pduMapIt->second.m_pdu->PeekHeader (hdrTst);

                  NS_ASSERT (pduMapIt->second.m_pdu->GetSize () > 0);
                  LteRadioBearerTag bearerTag (rnti, pduMapIt->second.m_size, 0);
                  pduMapIt->second.m_pdu->AddPacketTag (bearerTag);
                  NS_LOG_DEBUG ("eNB sending MAC pdu size " << pduMapIt->second.m_pdu->GetSize ());
                  for (unsigned i = 0; i < pduMapIt->second.m_macHeader.GetSubheaders ().size (); i++)
                    {
                      NS_LOG_DEBUG ("Subheader " << i << " size " << pduMapIt->second.m_macHeader.GetSubheaders ().at (i).m_size);
                    }
                  NS_LOG_DEBUG ("Total MAC PDU size " << pduMapIt->second.m_pdu->GetSize ());
                  harqIt->second.at (tbUid).m_pktBurst->AddPacket (pduMapIt->second.m_pdu);

                  m_phySapProvider->SendMacPdu (pduMapIt->second.m_pdu);
                  m_macPduMap.erase (pduMapIt);    // delete map entry

                  m_dlScheduling (ind.m_sfnSf.GetFrame (), ind.m_sfnSf.GetSubframe (), ind.m_sfnSf.GetSlot (),
                                  dciElem->m_tbSize, dciElem->m_mcs, dciElem->m_rnti, GetBwpId ());
                }
              else
                {
                  NS_LOG_INFO ("DL retransmission");
                  if (dciElem->m_tbSize > 0)
                    {
                      std::map <uint16_t, MmWaveDlHarqProcessesBuffer_t>::iterator it = m_miDlHarqProcessesPackets.find (rnti);
                      NS_ASSERT (it != m_miDlHarqProcessesPackets.end ());
                      Ptr<PacketBurst> pb = it->second.at (tbUid).m_pktBurst;
                      for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
                        {
                          Ptr<Packet> pkt = (*j)->Copy ();
                          MmWaveMacPduTag tag;         // update PDU tag for retransmission
                          if (!pkt->RemovePacketTag (tag))
                            {
                              NS_FATAL_ERROR ("No MAC PDU tag");
                            }
                          tag.SetSfn (ind.m_sfnSf);
                          tag.SetSymStart (dciElem->m_symStart);
                          tag.SetNumSym (dciElem->m_numSym);
                          pkt->AddPacketTag (tag);
                          m_phySapProvider->SendMacPdu (pkt);
                        }
                    }
                }
            }
        }
    }
}

// ////////////////////////////////////////////
// CMAC SAP
// ////////////////////////////////////////////

void
MmWaveEnbMac::DoConfigureMac (uint16_t ulBandwidth, uint16_t dlBandwidth)
{
  NS_LOG_FUNCTION (this);

  // The bandwidth arrived in Hz. We need to know it in number of RB, and then
  // consider how many RB are inside a single RBG.
  uint16_t bw_in_rbg = m_phySapProvider->GetRbNum () / GetNumRbPerRbg ();
  m_bandwidthInRbg = bw_in_rbg;

  NS_LOG_DEBUG ("Mac configured. Attributes:" << std::endl <<
                "\t NumRbPerRbg: " << m_numRbPerRbg << std::endl <<
                "\t NumHarqProcess: " << +m_numHarqProcess << std::endl <<
                "Physical properties: " << std::endl <<
                "\t Bandwidth provided: " << ulBandwidth * 1000 * 100 << " Hz" << std::endl <<
                "\t that corresponds to " << bw_in_rbg << " RBG, as we have " <<
                m_phySapProvider->GetRbNum () << " RB and " << GetNumRbPerRbg () <<
                " RB per RBG");

  MmWaveMacCschedSapProvider::CschedCellConfigReqParameters params;

  params.m_ulBandwidth = m_bandwidthInRbg;
  params.m_dlBandwidth = m_bandwidthInRbg;

  m_macCschedSapProvider->CschedCellConfigReq (params);
}

void
MmWaveEnbMac::BeamChangeReport (BeamId beamId, uint8_t rnti)
{
  MmWaveMacCschedSapProvider::CschedUeConfigReqParameters params;
  params.m_rnti = rnti;
  params.m_beamId = beamId;
  params.m_transmissionMode = 0;   // set to default value (SISO) for avoiding random initialization (valgrind error)
  m_macCschedSapProvider->CschedUeConfigReq (params);
}

uint16_t
MmWaveEnbMac::GetBwpId () const
{
  if (m_phySapProvider)
    {
      return m_phySapProvider->GetBwpId ();
    }
  else
    {
      return UINT16_MAX;
    }

}

uint16_t
MmWaveEnbMac::GetCellId () const
{
  if (m_phySapProvider)
    {
      return m_phySapProvider->GetCellId ();
    }
  else
    {
      return UINT16_MAX;
    }

}

std::shared_ptr<DciInfoElementTdma>
MmWaveEnbMac::GetDlCtrlDci () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_bandwidthInRbg > 0);
  std::vector<uint8_t> rbgBitmask (m_bandwidthInRbg , 1);

  // Fix: Insert number of dl ctrl symbols
  return std::make_shared<DciInfoElementTdma> (0, 1, DciInfoElementTdma::DL, DciInfoElementTdma::CTRL, rbgBitmask);
}

std::shared_ptr<DciInfoElementTdma>
MmWaveEnbMac::GetUlCtrlDci () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_bandwidthInRbg > 0);
  std::vector<uint8_t> rbgBitmask (m_bandwidthInRbg , 1);

  // Fix: Insert number of ul ctrl symbols
  return std::make_shared<DciInfoElementTdma> (0, 1, DciInfoElementTdma::UL, DciInfoElementTdma::CTRL, rbgBitmask);
}

void
MmWaveEnbMac::DoAddUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " rnti=" << rnti);
  std::map<uint8_t, LteMacSapUser*> empty;
  std::pair <std::map <uint16_t, std::map<uint8_t, LteMacSapUser*> >::iterator, bool>
  ret = m_rlcAttached.insert (std::pair <uint16_t,  std::map<uint8_t, LteMacSapUser*> >
                                (rnti, empty));
  NS_ASSERT_MSG (ret.second, "element already present, RNTI already existed");

  MmWaveMacCschedSapProvider::CschedUeConfigReqParameters params;
  params.m_rnti = rnti;
  params.m_beamId = m_phySapProvider->GetBeamId (rnti);
  params.m_transmissionMode = 0;   // set to default value (SISO) for avoiding random initialization (valgrind error)
  m_macCschedSapProvider->CschedUeConfigReq (params);

  // Create DL transmission HARQ buffers
  MmWaveDlHarqProcessesBuffer_t buf;
  uint16_t harqNum = GetNumHarqProcess ();
  buf.resize (harqNum);
  for (uint8_t i = 0; i < harqNum; i++)
    {
      Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      buf.at (i).m_pktBurst = pb;
    }
  m_miDlHarqProcessesPackets.insert (std::pair <uint16_t, MmWaveDlHarqProcessesBuffer_t> (rnti, buf));

}

void
MmWaveEnbMac::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " rnti=" << rnti);
  MmWaveMacCschedSapProvider::CschedUeReleaseReqParameters params;
  params.m_rnti = rnti;
  m_macCschedSapProvider->CschedUeReleaseReq (params);
  m_miDlHarqProcessesPackets.erase (rnti);
  m_rlcAttached.erase (rnti);
}

void
MmWaveEnbMac::DoAddLc (LteEnbCmacSapProvider::LcInfo lcinfo, LteMacSapUser* msu)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_FUNCTION (this);

  std::map <LteFlowId_t, LteMacSapUser* >::iterator it;

  LteFlowId_t flow (lcinfo.rnti, lcinfo.lcId);

  std::map <uint16_t, std::map<uint8_t, LteMacSapUser*> >::iterator rntiIt = m_rlcAttached.find (lcinfo.rnti);
  NS_ASSERT_MSG (rntiIt != m_rlcAttached.end (), "RNTI not found");
  std::map<uint8_t, LteMacSapUser*>::iterator lcidIt = rntiIt->second.find (lcinfo.lcId);
  if (lcidIt == rntiIt->second.end ())
    {
      rntiIt->second.insert (std::pair<uint8_t, LteMacSapUser*> (lcinfo.lcId, msu));
    }
  else
    {
      NS_LOG_ERROR ("LC already exists");
    }

  // CCCH (LCID 0) is pre-configured
  // see FF LTE MAC Scheduler
  // Interface Specification v1.11,
  // 4.3.4 logicalChannelConfigListElement
  if (lcinfo.lcId != 0)
    {
      struct MmWaveMacCschedSapProvider::CschedLcConfigReqParameters params;
      params.m_rnti = lcinfo.rnti;
      params.m_reconfigureFlag = false;

      struct LogicalChannelConfigListElement_s lccle;
      lccle.m_logicalChannelIdentity = lcinfo.lcId;
      lccle.m_logicalChannelGroup = lcinfo.lcGroup;
      lccle.m_direction = LogicalChannelConfigListElement_s::DIR_BOTH;
      lccle.m_qosBearerType = lcinfo.isGbr ? LogicalChannelConfigListElement_s::QBT_GBR : LogicalChannelConfigListElement_s::QBT_NON_GBR;
      lccle.m_qci = lcinfo.qci;
      lccle.m_eRabMaximulBitrateUl = lcinfo.mbrUl;
      lccle.m_eRabMaximulBitrateDl = lcinfo.mbrDl;
      lccle.m_eRabGuaranteedBitrateUl = lcinfo.gbrUl;
      lccle.m_eRabGuaranteedBitrateDl = lcinfo.gbrDl;
      params.m_logicalChannelConfigList.push_back (lccle);

      m_macCschedSapProvider->CschedLcConfigReq (params);
    }
}

void
MmWaveEnbMac::DoReconfigureLc (LteEnbCmacSapProvider::LcInfo lcinfo)
{
  NS_FATAL_ERROR ("not implemented");
}

void
MmWaveEnbMac::DoReleaseLc (uint16_t rnti, uint8_t lcid)
{
  //Find user based on rnti and then erase lcid stored against the same
  std::map <uint16_t, std::map<uint8_t, LteMacSapUser*> >::iterator rntiIt = m_rlcAttached.find (rnti);
  rntiIt->second.erase (lcid);

  struct MmWaveMacCschedSapProvider::CschedLcReleaseReqParameters params;
  params.m_rnti = rnti;
  params.m_logicalChannelIdentity.push_back (lcid);
  m_macCschedSapProvider->CschedLcReleaseReq (params);
}

void
MmWaveEnbMac::UeUpdateConfigurationReq (LteEnbCmacSapProvider::UeConfig params)
{
  NS_LOG_FUNCTION (this);
  // propagates to scheduler
  MmWaveMacCschedSapProvider::CschedUeConfigReqParameters req;
  req.m_rnti = params.m_rnti;
  req.m_transmissionMode = params.m_transmissionMode;
  req.m_beamId = m_phySapProvider->GetBeamId (params.m_rnti);
  req.m_reconfigureFlag = true;
  m_macCschedSapProvider->CschedUeConfigReq (req);
}

LteEnbCmacSapProvider::RachConfig
MmWaveEnbMac::DoGetRachConfig ()
{
  //UEs in NR does not choose RACH preambles randomly, therefore,
  //it does not rely on the following parameters. However, the
  //recent change in LteUeRrc class introduced an assert to
  //check the correct value of connEstFailCount parameter.
  //Thus, we need to assign dummy but correct values to
  //avoid this assert in LteUeRrc class.
  LteEnbCmacSapProvider::RachConfig rc;
  rc.numberOfRaPreambles = 52;
  rc.preambleTransMax = 50;
  rc.raResponseWindowSize = 3;
  rc.connEstFailCount = 1;
  return rc;
}

LteEnbCmacSapProvider::AllocateNcRaPreambleReturnValue
MmWaveEnbMac::DoAllocateNcRaPreamble (uint16_t rnti)
{
  return LteEnbCmacSapProvider::AllocateNcRaPreambleReturnValue ();
}

// ////////////////////////////////////////////
// CSCHED SAP
// ////////////////////////////////////////////


void
MmWaveEnbMac::DoCschedCellConfigCnf (MmWaveMacCschedSapUser::CschedCellConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveEnbMac::DoCschedUeConfigCnf (MmWaveMacCschedSapUser::CschedUeConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveEnbMac::DoCschedLcConfigCnf (MmWaveMacCschedSapUser::CschedLcConfigCnfParameters params)
{
  NS_LOG_FUNCTION (this);
  // Call the CSCHED primitive
  // m_cschedSap->LcConfigCompleted();
}

void
MmWaveEnbMac::DoCschedLcReleaseCnf (MmWaveMacCschedSapUser::CschedLcReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveEnbMac::DoCschedUeReleaseCnf (MmWaveMacCschedSapUser::CschedUeReleaseCnfParameters params)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveEnbMac::DoCschedUeConfigUpdateInd (MmWaveMacCschedSapUser::CschedUeConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
  // propagates to RRC
  LteEnbCmacSapUser::UeConfig ueConfigUpdate;
  ueConfigUpdate.m_rnti = params.m_rnti;
  ueConfigUpdate.m_transmissionMode = params.m_transmissionMode;
  m_cmacSapUser->RrcConfigurationUpdateInd (ueConfigUpdate);
}

void
MmWaveEnbMac::DoCschedCellConfigUpdateInd (MmWaveMacCschedSapUser::CschedCellConfigUpdateIndParameters params)
{
  NS_LOG_FUNCTION (this);
}

}