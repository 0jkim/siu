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
                << GetBwpId () << ", rnti " << m_rnti << "] ";           \
    }                                                                    \
  while (false);

#include "nr-ue-mac.h"
//#include "nr-ue-phy.h"
#include <ns3/log.h>
#include <ns3/boolean.h>
#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/random-variable-stream.h>
#include "nr-phy-sap.h"
#include "nr-control-messages.h"
#include "nr-mac-header-vs.h"
#include "nr-mac-short-bsr-ce.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrUeMac");
NS_OBJECT_ENSURE_REGISTERED (NrUeMac);

uint8_t NrUeMac::g_raPreambleId = 0;

///////////////////////////////////////////////////////////
// SAP forwarders
///////////////////////////////////////////////////////////


class UeMemberNrUeCmacSapProvider : public LteUeCmacSapProvider
{
public:
  UeMemberNrUeCmacSapProvider (NrUeMac* mac);

  // inherited from LteUeCmacSapProvider
  virtual void ConfigureRach (RachConfig rc);
  virtual void StartContentionBasedRandomAccessProcedure ();
  virtual void StartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask);
  virtual void AddLc (uint8_t lcId, LteUeCmacSapProvider::LogicalChannelConfig lcConfig, LteMacSapUser* msu);
  virtual void RemoveLc (uint8_t lcId);
  virtual void Reset ();
  virtual void SetRnti (uint16_t rnti);
  virtual void NotifyConnectionSuccessful ();
  virtual void SetImsi (uint64_t imsi);

private:
  NrUeMac* m_mac;
};


UeMemberNrUeCmacSapProvider::UeMemberNrUeCmacSapProvider (NrUeMac* mac)
  : m_mac (mac)
{
}

void
UeMemberNrUeCmacSapProvider::ConfigureRach (RachConfig rc)
{
  m_mac->DoConfigureRach (rc);
}

void
UeMemberNrUeCmacSapProvider::StartContentionBasedRandomAccessProcedure ()
{
  m_mac->DoStartContentionBasedRandomAccessProcedure ();
}

void
UeMemberNrUeCmacSapProvider::StartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask)
{
  m_mac->DoStartNonContentionBasedRandomAccessProcedure (rnti, preambleId, prachMask);
}


void
UeMemberNrUeCmacSapProvider::AddLc (uint8_t lcId, LogicalChannelConfig lcConfig, LteMacSapUser* msu)
{
  m_mac->AddLc (lcId, lcConfig, msu);
}

void
UeMemberNrUeCmacSapProvider::RemoveLc (uint8_t lcid)
{
  m_mac->DoRemoveLc (lcid);
}

void
UeMemberNrUeCmacSapProvider::Reset ()
{
  m_mac->DoReset ();
}

void
UeMemberNrUeCmacSapProvider::SetRnti (uint16_t rnti)
{
  m_mac->SetRnti (rnti);
}

void
UeMemberNrUeCmacSapProvider::NotifyConnectionSuccessful ()
{
  m_mac->DoNotifyConnectionSuccessful ();
}

void
UeMemberNrUeCmacSapProvider::SetImsi (uint64_t imsi)
 {
   m_mac->DoSetImsi (imsi);
 }

class UeMemberNrMacSapProvider : public LteMacSapProvider
{
public:
  UeMemberNrMacSapProvider (NrUeMac* mac);

  // inherited from LteMacSapProvider
  virtual void TransmitPdu (TransmitPduParameters params);
  virtual void ReportBufferStatus (ReportBufferStatusParameters params);

private:
  NrUeMac* m_mac;
};


UeMemberNrMacSapProvider::UeMemberNrMacSapProvider (NrUeMac* mac)
  : m_mac (mac)
{
}

void
UeMemberNrMacSapProvider::TransmitPdu (TransmitPduParameters params)
{
  m_mac->DoTransmitPdu (params);
}

void
UeMemberNrMacSapProvider::ReportBufferStatus (ReportBufferStatusParameters params)
{
  m_mac->DoReportBufferStatus (params);
}

class NrUePhySapUser;

class MacUeMemberPhySapUser : public NrUePhySapUser
{
public:
  MacUeMemberPhySapUser (NrUeMac* mac);

  virtual void ReceivePhyPdu (Ptr<Packet> p) override;

  virtual void ReceiveControlMessage (Ptr<NrControlMessage> msg) override;

  virtual void SlotIndication (SfnSf sfn) override;

  //virtual void NotifyHarqDeliveryFailure (uint8_t harqId);

  virtual uint8_t GetNumHarqProcess () const override;

  //Configured Grant
  virtual bool SlotIndication_configuredGrant (SfnSf sfn) override;

private:
  NrUeMac* m_mac;
};

MacUeMemberPhySapUser::MacUeMemberPhySapUser (NrUeMac* mac)
  : m_mac (mac)
{

}
void
MacUeMemberPhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
  m_mac->DoReceivePhyPdu (p);
}

void
MacUeMemberPhySapUser::ReceiveControlMessage (Ptr<NrControlMessage> msg)
{
  m_mac->DoReceiveControlMessage (msg);
}

void
MacUeMemberPhySapUser::SlotIndication (SfnSf sfn)
{
  m_mac->DoSlotIndication (sfn);
}

uint8_t
MacUeMemberPhySapUser::GetNumHarqProcess () const
{
  return m_mac->GetNumHarqProcess();
}

//-----------------------------------------------------------------------

TypeId
NrUeMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrUeMac")
    .SetParent<Object> ()
    .AddConstructor<NrUeMac> ()
    .AddAttribute ("NumHarqProcess",
                   "Number of concurrent stop-and-wait Hybrid ARQ processes per user",
                    UintegerValue (20),
                    MakeUintegerAccessor (&NrUeMac::SetNumHarqProcess,
                                          &NrUeMac::GetNumHarqProcess),
                    MakeUintegerChecker<uint8_t> ())
    .AddTraceSource ("UeMacRxedCtrlMsgsTrace",
                     "Ue MAC Control Messages Traces.",
                     MakeTraceSourceAccessor (&NrUeMac::m_macRxedCtrlMsgsTrace),
                     "ns3::NrMacRxTrace::RxedUeMacCtrlMsgsTracedCallback")
    .AddTraceSource ("UeMacTxedCtrlMsgsTrace",
                     "Ue MAC Control Messages Traces.",
                     MakeTraceSourceAccessor (&NrUeMac::m_macTxedCtrlMsgsTrace),
                     "ns3::NrMacRxTrace::TxedUeMacCtrlMsgsTracedCallback")
    // Configured Grant
    .AddAttribute ("CG",
                  "Activate configured grant scheduling for UL periodic transmissions",
                  BooleanValue (true),
                  MakeBooleanAccessor (&NrUeMac::SetCG,
                                       &NrUeMac::GetCG),
                  MakeBooleanChecker ())
    .AddAttribute ("ConfigurationTime",
                  "Time required to configure UE with configured grant signal",
                   UintegerValue (30),
                   MakeUintegerAccessor (&NrUeMac::SetConfigurationTime,
                                         &NrUeMac::GetConfigurationTime),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("CGPeriod",
                  "The periodicity of configured grant transmissions",
                   UintegerValue (10),
                   MakeUintegerAccessor (&NrUeMac::SetCGPeriod,
                                         &NrUeMac::GetCGPeriod),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}

NrUeMac::NrUeMac (void) : Object ()
{
  NS_LOG_FUNCTION (this);
  m_cmacSapProvider = new UeMemberNrUeCmacSapProvider (this);
  m_macSapProvider = new UeMemberNrMacSapProvider (this);
  m_phySapUser = new MacUeMemberPhySapUser (this);
  m_raPreambleUniformVariable = CreateObject<UniformRandomVariable> ();
}

NrUeMac::~NrUeMac (void)
{
}

void
NrUeMac::DoDispose ()
{
  m_miUlHarqProcessesPacket.clear ();
  m_miUlHarqProcessesPacketTimer.clear ();
  m_ulBsrReceived.clear ();
  m_lcInfoMap.clear ();
  m_raPreambleUniformVariable = nullptr;
  delete m_macSapProvider;
  delete m_cmacSapProvider;
  delete m_phySapUser;
}

void
NrUeMac::SetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  m_rnti = rnti;
}

void
NrUeMac::DoNotifyConnectionSuccessful ()
{
  NS_LOG_FUNCTION (this);
  m_phySapProvider->NotifyConnectionSuccessful ();
}

void
NrUeMac::DoSetImsi (uint64_t imsi)
{
  NS_LOG_FUNCTION (this);
  m_imsi = imsi;
}

uint16_t
NrUeMac::GetBwpId () const
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
NrUeMac::GetCellId () const
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

uint32_t
NrUeMac::GetTotalBufSize () const
{
  uint32_t ret = 0;
  for (auto it = m_ulBsrReceived.cbegin (); it != m_ulBsrReceived.cend (); ++it)
    {
      ret += ((*it).second.txQueueSize + (*it).second.retxQueueSize + (*it).second.statusPduSize);
    }
  return ret;
}

/**
 * \brief Sets the number of HARQ processes
 * \param numHarqProcesses the maximum number of harq processes
 */
void
NrUeMac::SetNumHarqProcess (uint8_t numHarqProcess)
{
  m_numHarqProcess = numHarqProcess;

  m_miUlHarqProcessesPacket.resize (GetNumHarqProcess ());
  for (uint8_t i = 0; i < m_miUlHarqProcessesPacket.size (); i++)
    {
      if (m_miUlHarqProcessesPacket.at (i).m_pktBurst == nullptr)
        {
          Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
          m_miUlHarqProcessesPacket.at (i).m_pktBurst = pb;
        }
    }
  m_miUlHarqProcessesPacketTimer.resize (GetNumHarqProcess (), 0);
}

/**
 * \return number of HARQ processes
 */
uint8_t
NrUeMac::GetNumHarqProcess () const
{
  return m_numHarqProcess;
}

// forwarded from MAC SAP
void
NrUeMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_ulDci->m_harqProcess == params.harqProcessId);

  m_miUlHarqProcessesPacket.at (params.harqProcessId).m_lcidList.push_back (params.lcid);

  NrMacHeaderVs header;
  header.SetLcId (params.lcid);
  header.SetSize (params.pdu->GetSize ());

  params.pdu->AddHeader (header);

  LteRadioBearerTag bearerTag (params.rnti, params.lcid, params.layer);
  params.pdu->AddPacketTag (bearerTag);

  m_miUlHarqProcessesPacket.at (params.harqProcessId).m_pktBurst->AddPacket (params.pdu);
  m_miUlHarqProcessesPacketTimer.at (params.harqProcessId) = GetNumHarqProcess();

  m_ulDciTotalUsed += params.pdu->GetSize ();

  NS_ASSERT_MSG (m_ulDciTotalUsed <= m_ulDci->m_tbSize.at (0), "We used more data than the DCI allowed us.");

  m_phySapProvider->SendMacPdu (params.pdu, m_ulDciSfnsf, m_ulDci->m_symStart, params.layer);
}

void
NrUeMac::DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (params.lcid));

  auto it = m_ulBsrReceived.find (params.lcid);

  NS_LOG_INFO ("Received BSR for LC Id" << static_cast<uint32_t>(params.lcid));

  if (it != m_ulBsrReceived.end ())
    {
      // update entry
      (*it).second = params;
    }
  else
    {
      it = m_ulBsrReceived.insert (std::make_pair (params.lcid, params)).first;
    }

  // The state machine for CG and dynamic grant is different, so when a packet is
  // received in the UE layer of the upper layers, depending on which scheduling
  // we are using, it will move to one state or another
  if (m_cgScheduling)
    {
      if (m_srState_configuredGrant == INACTIVE_CG)
        {
          NS_LOG_INFO ("INACTIVE -> TO_SEND, bufSize " << GetTotalBufSize ());
          m_srState_configuredGrant = TO_SEND_TrafficInfo;
          if (params.rnti == 4)// Esto lo teno que cambiar, va a ser variable pero por ahroa
              // solo para probar
          {
            m_traffDeadlineTime =  MicroSeconds(200); //80
          }
          else
          {
            m_traffDeadlineTime = MicroSeconds(150); //60
          }
          m_traffStartTime = Simulator::Now();
          m_trafficPeriodicity = params.periodicity;
        }

      if (m_srState_configuredGrant == ACTIVE_CG)
        {
          NS_LOG_INFO ("CONFIGURED GRANT, bufSize " << GetTotalBufSize ());
          m_srState_configuredGrant = SCH_CG_DATA; //If we have a new packet, we are not going to call to SR.
        }
    }
  else
    {
      if (m_srState == INACTIVE)
        {
          NS_LOG_INFO ("INACTIVE -> TO_SEND, bufSize " << GetTotalBufSize ());
          m_srState = TO_SEND;
        }
    }
}


void
NrUeMac::SendReportBufferStatus (const SfnSf &dataSfn, uint8_t symStart)
{
  NS_LOG_FUNCTION (this);

  if (m_rnti == 0)
    {
      NS_LOG_INFO ("MAC not initialized, BSR deferred");
      return;
    }

  if (m_ulBsrReceived.size () == 0)
    {
      NS_LOG_INFO ("No BSR report to transmit");
      return;
    }
  MacCeElement bsr = MacCeElement ();
  bsr.m_rnti = m_rnti;
  bsr.m_macCeType = MacCeElement::BSR;

  // BSR is reported for each LCG
  std::unordered_map <uint8_t, LteMacSapProvider::ReportBufferStatusParameters>::iterator it;
  std::vector<uint32_t> queue (4, 0);   // one value per each of the 4 LCGs, initialized to 0
  for (it = m_ulBsrReceived.begin (); it != m_ulBsrReceived.end (); it++)
    {
      uint8_t lcid = it->first;
      std::unordered_map <uint8_t, LcInfo>::iterator lcInfoMapIt;
      lcInfoMapIt = m_lcInfoMap.find (lcid);
      NS_ASSERT (lcInfoMapIt !=  m_lcInfoMap.end ());
      NS_ASSERT_MSG ((lcid != 0) || (((*it).second.txQueueSize == 0)
                                     && ((*it).second.retxQueueSize == 0)
                                     && ((*it).second.statusPduSize == 0)),
                     "BSR should not be used for LCID 0");
      uint8_t lcg = lcInfoMapIt->second.lcConfig.logicalChannelGroup;
      queue.at (lcg) += ((*it).second.txQueueSize + (*it).second.retxQueueSize + (*it).second.statusPduSize);
    }

  NS_LOG_INFO ("Sending BSR with this info for the LCG: " << queue.at (0) << " " <<
               queue.at (1) << " " << queue.at(2) << " " << queue.at(3));
  // FF API says that all 4 LCGs are always present
  bsr.m_macCeValue.m_bufferStatus.push_back (NrMacShortBsrCe::FromBytesToLevel (queue.at (0)));
  bsr.m_macCeValue.m_bufferStatus.push_back (NrMacShortBsrCe::FromBytesToLevel (queue.at (1)));
  bsr.m_macCeValue.m_bufferStatus.push_back (NrMacShortBsrCe::FromBytesToLevel (queue.at (2)));
  bsr.m_macCeValue.m_bufferStatus.push_back (NrMacShortBsrCe::FromBytesToLevel (queue.at (3)));

  // create the message. It is used only for tracing, but we don't send it...
  Ptr<NrBsrMessage> msg = Create<NrBsrMessage> ();
  msg->SetSourceBwp (GetBwpId ());
  msg->SetBsr (bsr);

  m_macTxedCtrlMsgsTrace (m_currentSlot, GetCellId (), bsr.m_rnti, GetBwpId (), msg);

  // Here we send the real SHORT_BSR, as a subpdu.
  Ptr<Packet> p = Create<Packet> ();

  // Please note that the levels are defined from the standard. In this case,
  // we have 5 bit available, so use such standard levels. In the future,
  // when LONG BSR will be implemented, this have to change.
  NrMacShortBsrCe header;
  header.m_bufferSizeLevel_0 = NrMacShortBsrCe::FromBytesToLevel (queue.at (0));
  header.m_bufferSizeLevel_1 = NrMacShortBsrCe::FromBytesToLevel (queue.at (1));
  header.m_bufferSizeLevel_2 = NrMacShortBsrCe::FromBytesToLevel (queue.at (2));
  header.m_bufferSizeLevel_3 = NrMacShortBsrCe::FromBytesToLevel (queue.at (3));

  p->AddHeader (header);

  LteRadioBearerTag bearerTag (m_rnti, NrMacHeaderFsUl::SHORT_BSR, 0);
  p->AddPacketTag (bearerTag);

  m_ulDciTotalUsed += p->GetSize ();
  NS_ASSERT_MSG (m_ulDciTotalUsed <= m_ulDci->m_tbSize.at (0), "We used more data than the DCI allowed us.");

  //MIMO is not supported for UL yet.
  //Therefore, there will be only
  //one stream with stream Id 0.
  uint8_t streamId = 0;

  m_phySapProvider->SendMacPdu (p, dataSfn, symStart, streamId);
}

void
NrUeMac::SetUeCmacSapUser (LteUeCmacSapUser* s)
{
  m_cmacSapUser = s;
}

LteUeCmacSapProvider*
NrUeMac::GetUeCmacSapProvider (void)
{
  return m_cmacSapProvider;
}

void
NrUeMac::RefreshHarqProcessesPacketBuffer (void)
{
  NS_LOG_FUNCTION (this);

  for (uint16_t i = 0; i < m_miUlHarqProcessesPacketTimer.size (); i++)
    {
      if (m_miUlHarqProcessesPacketTimer.at (i) == 0 && m_miUlHarqProcessesPacket.at (i).m_pktBurst)
        {
          if (m_miUlHarqProcessesPacket.at (i).m_pktBurst->GetSize () > 0)
            {
              // timer expired: drop packets in buffer for this process
              NS_LOG_INFO ("HARQ Proc Id " << i << " packets buffer expired");
              Ptr<PacketBurst> emptyPb = CreateObject <PacketBurst> ();
              m_miUlHarqProcessesPacket.at (i).m_pktBurst = emptyPb;
              m_miUlHarqProcessesPacket.at (i).m_lcidList.clear ();
            }
        }
      else
        {
          //m_miUlHarqProcessesPacketTimer.at (i)--;  // ignore HARQ timeout
        }
    }
}

void
NrUeMac::DoSlotIndication (const SfnSf &sfn)
{
  NS_LOG_FUNCTION (this);
  m_currentSlot = sfn;
  NS_LOG_INFO ("Slot " << m_currentSlot);

  RefreshHarqProcessesPacketBuffer ();

  if (m_srState == TO_SEND)
    {
      NS_LOG_INFO ("Sending SR to PHY in slot " << sfn);
      SendSR ();
      m_srState = ACTIVE;
    }

  // Feedback missing
}

void
NrUeMac::SendSR () const
{
  NS_LOG_FUNCTION (this);

  if (m_rnti == 0)
    {
      NS_LOG_INFO ("MAC not initialized, SR deferred");
      return;
    }

  // create the SR to send to the gNB
  Ptr<NrSRMessage> msg = Create<NrSRMessage> ();
  msg->SetSourceBwp (GetBwpId ());
  msg->SetRNTI (m_rnti);

  m_macTxedCtrlMsgsTrace (m_currentSlot, GetCellId (), m_rnti, GetBwpId (), msg);
  m_phySapProvider->SendControlMessage (msg);
}

void
NrUeMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);

  LteRadioBearerTag tag;
  p->RemovePacketTag (tag);

  if (tag.GetRnti() != m_rnti) // Packet is for another user
    {
      return;
    }

  NrMacHeaderVs header;
  p->RemoveHeader (header);

  LteMacSapUser::ReceivePduParameters rxParams;
  rxParams.p = p;
  rxParams.rnti = m_rnti;
  rxParams.lcid = header.GetLcId ();

  auto it = m_lcInfoMap.find (header.GetLcId());

  // p can be empty. Well, right now no, but when someone will add CE in downlink,
  // then p can be empty.
  if (rxParams.p->GetSize () > 0)
    {
      it->second.macSapUser->ReceivePdu (rxParams);
    }
}

void
NrUeMac::RecvRaResponse (BuildRarListElement_s raResponse)
{
  NS_LOG_FUNCTION (this);
  m_waitingForRaResponse = false;
  m_rnti = raResponse.m_rnti;
  m_cmacSapUser->SetTemporaryCellRnti (m_rnti);
  m_cmacSapUser->NotifyRandomAccessSuccessful ();
}

void
NrUeMac::ProcessUlDci (const Ptr<NrUlDciMessage> &dciMsg)
{
  NS_LOG_FUNCTION (this);

  SfnSf dataSfn = m_currentSlot;
  dataSfn.Add (dciMsg->GetKDelay ());

  // Saving the data we need in DoTransmitPdu
  m_ulDciSfnsf = dataSfn;
  m_ulDciTotalUsed = 0;
  m_ulDci = dciMsg->GetDciInfoElement ();

  // When the UE receives the CG signal it stores this information in the MAC layer
  // It represents the TO_RECEIVE_CG state
  if (m_cgScheduling)
    {
      m_dciGranted_stored[cg_slot_counter_DCI] = std::make_shared <DciInfoElementTdma> (m_ulDci->m_rnti,
                                                                                 m_ulDci->m_format,
                                                                                 m_ulDci->m_symStart,
                                                                                 m_ulDci->m_numSym,
                                                                                 m_ulDci->m_mcs,
                                                                                 m_ulDci->m_tbSize,
                                                                                 m_ulDci->m_ndi,
                                                                                 m_ulDci->m_rv,
                                                                                 m_ulDci->m_type,
                                                                                 m_ulDci -> m_bwpIndex,
                                                                                 m_ulDci->m_harqProcess,
                                                                                 m_ulDci->m_rbgBitmask,
                                                                                 m_ulDci->m_tpc);

      uint8_t streamId = 0;
      SfnSf dataSfn_cg = m_currentSlot;
      uint8_t number_slots_for_processing_configurationPeriod = 5;
      uint8_t numberOfSlot_insideOneSubframe = pow(2,(m_currentSlot.GetNumerology ()));
      m_configurationTime = GetConfigurationTime();
      uint8_t number_slots_configuration = (GetConfigurationTime()*numberOfSlot_insideOneSubframe)-number_slots_for_processing_configurationPeriod;
      dataSfn_cg.Add(number_slots_configuration);
      uint64_t key = dataSfn_cg.GetEncForStreamWithSymStart (streamId, m_dciGranted_stored[cg_slot_counter_DCI]->m_symStart);

      auto it_1 = m_dci_cg_map.find (key);

      if (it_1 == m_dci_cg_map.end ())
        {
          it_1 = m_dci_cg_map.insert (std::make_pair (key, m_dciGranted_stored[cg_slot_counter_DCI])).first;
          cg_slot_counter_DCI = cg_slot_counter_DCI +1;
        }
    }

  m_macRxedCtrlMsgsTrace (m_currentSlot, GetCellId (), m_rnti, GetBwpId (), dciMsg);

  NS_LOG_INFO ("UL DCI received, transmit data in slot " << dataSfn <<
               " Harq Process " << +m_ulDci->m_harqProcess <<
               " TBS " << m_ulDci->m_tbSize.at (0) << " total queue " << GetTotalBufSize ());

  if (m_ulDci->m_ndi.at (0) == 0)
    {
      // This method will retransmit the data saved in the harq buffer
      TransmitRetx ();

      // This method will transmit a new BSR.
      SendReportBufferStatus (dataSfn, m_ulDci->m_symStart);
    }
  else if (m_ulDci->m_ndi.at (0) == 1)
    {
      SendNewData ();

      NS_LOG_INFO ("After sending NewData, bufSize " << GetTotalBufSize ());

      // Send a new BSR. SendNewData() already took into account the size of
      // the BSR.
      SendReportBufferStatus (dataSfn, m_ulDci->m_symStart);

      NS_LOG_INFO ("UL DCI processing done, sent to PHY a total of " << m_ulDciTotalUsed <<
                   " B out of " << m_ulDci->m_tbSize.at (0) << " allocated bytes ");

      if (GetTotalBufSize () == 0)
        {
          // Sets the correct state depending on the type of scheduling: cg or dynamic
          if (m_cgScheduling)
            {
              m_srState_configuredGrant = ACTIVE_CG;
              NS_LOG_INFO ("TO_RECEIVE_CG -> ACTIVE_CG, bufSize " << GetTotalBufSize ());
            }
          else
            {
              m_srState = INACTIVE;
              NS_LOG_INFO ("ACTIVE -> INACTIVE, bufSize " << GetTotalBufSize ());
            }
          // the UE may have been scheduled, but we didn't use a single byte
          // of the allocation. So send an empty PDU. This happens because the
          // byte reporting in the BSR is not accurate, due to RLC and/or
          // BSR quantization.
          if (m_ulDciTotalUsed == 0)
            {
              NS_LOG_WARN ("No byte used for this UL-DCI, sending empty PDU");

              LteMacSapProvider::TransmitPduParameters txParams;

              txParams.pdu = Create<Packet> ();
              txParams.lcid = 3;
              txParams.rnti = m_rnti;
              txParams.layer = 0;
              txParams.harqProcessId = m_ulDci->m_harqProcess;
              txParams.componentCarrierId = GetBwpId ();

              DoTransmitPdu (txParams);
            }
        }
    }
}

void
NrUeMac::TransmitRetx ()
{
  NS_LOG_FUNCTION (this);

  Ptr<PacketBurst> pb = m_miUlHarqProcessesPacket.at (m_ulDci->m_harqProcess).m_pktBurst;

  if (pb == nullptr)
    {
      NS_LOG_WARN ("The previous transmission did not contain any new data; "
                   "probably it was BSR only. To not send an old BSR to the scheduler, "
                   "we don't send anything back in this allocation. Eventually, "
                   "the Harq timer at gnb will expire, and soon this allocation will be forgotten.");
      return;
    }

  NS_LOG_DEBUG ("UE MAC RETX HARQ " << + m_ulDci->m_harqProcess);

  NS_ASSERT (pb->GetNPackets() > 0);

  for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
    {
      Ptr<Packet> pkt = (*j)->Copy ();
      LteRadioBearerTag bearerTag;
      if (!pkt->PeekPacketTag (bearerTag))
        {
          NS_FATAL_ERROR ("No radio bearer tag");
        }
      //MIMO is not supported for UL yet.
      //Therefore, there will be only
      //one stream with stream Id 0.
      uint8_t streamId = 0;
      m_phySapProvider->SendMacPdu (pkt, m_ulDciSfnsf, m_ulDci->m_symStart, streamId);
    }

  m_miUlHarqProcessesPacketTimer.at (m_ulDci->m_harqProcess) = GetNumHarqProcess();
}

void
NrUeMac::SendRetxData (uint32_t usefulTbs, uint32_t activeLcsRetx)
{
  NS_LOG_FUNCTION (this);

  if (activeLcsRetx == 0)
    {
      return;
    }

  uint32_t bytesPerLcId = usefulTbs / activeLcsRetx;

  for (auto & itBsr : m_ulBsrReceived)
    {
      auto &bsr = itBsr.second;

      if (m_ulDciTotalUsed + bytesPerLcId <= usefulTbs)
        {
          LteMacSapUser::TxOpportunityParameters txParams;
          txParams.lcid = bsr.lcid;
          txParams.rnti = m_rnti;
          txParams.bytes = bytesPerLcId;
          txParams.layer = 0;
          txParams.harqId = m_ulDci->m_harqProcess;
          txParams.componentCarrierId = GetBwpId ();

          NS_LOG_INFO ("Notifying RLC of LCID " << +bsr.lcid << " of a TxOpp "
                       "of " << bytesPerLcId << " B for a RETX PDU");

          m_lcInfoMap.at (bsr.lcid).macSapUser->NotifyTxOpportunity (txParams);
          // After this call, m_ulDciTotalUsed has been updated with the
          // correct amount of bytes... but it is up to us in updating the BSR
          // value, substracting the amount of bytes transmitted

          // We need to use std::min here because bytesPerLcId can be
          // greater than bsr.txQueueSize because scheduler can assign
          // more bytes than needed due to how TB size is computed.
          bsr.retxQueueSize -= std::min (bytesPerLcId, bsr.retxQueueSize);
        }
      else
        {
          NS_LOG_DEBUG ("Something wrong with the calculation of overhead."
                        "Active LCS Retx: " << activeLcsRetx << " assigned to this: " <<
                        bytesPerLcId << ", with TBS of " << m_ulDci->m_tbSize.at (0) <<
                        " usefulTbs " << usefulTbs << " and total used " << m_ulDciTotalUsed);
        }
    }
}

void
NrUeMac::SendTxData(uint32_t usefulTbs, uint32_t activeTx)
{
  NS_LOG_FUNCTION (this);

  if (activeTx == 0)
    {
      return;
    }

  uint32_t bytesPerLcId = usefulTbs / activeTx;

  for (auto & itBsr : m_ulBsrReceived)
    {
      auto &bsr = itBsr.second;

      if (m_ulDciTotalUsed + bytesPerLcId <= usefulTbs)
        {
          LteMacSapUser::TxOpportunityParameters txParams;
          txParams.lcid = bsr.lcid;
          txParams.rnti = m_rnti;
          txParams.bytes = bytesPerLcId;
          txParams.layer = 0;
          txParams.harqId = m_ulDci->m_harqProcess;
          txParams.componentCarrierId = GetBwpId ();

          NS_LOG_INFO ("Notifying RLC of LCID " << +bsr.lcid << " of a TxOpp "
                       "of " << bytesPerLcId << " B for a TX PDU");

          m_lcInfoMap.at (bsr.lcid).macSapUser->NotifyTxOpportunity (txParams);
          // After this call, m_ulDciTotalUsed has been updated with the
          // correct amount of bytes... but it is up to us in updating the BSR
          // value, substracting the amount of bytes transmitted

          // We need to use std::min here because bytesPerLcId can be
          // greater than bsr.txQueueSize because scheduler can assign
          // more bytes than needed due to how TB size is computed.
         // bsr.txQueueSize -= std::min (bytesPerLcId, bsr.txQueueSize);
          bsr.txQueueSize -= std::min (bytesPerLcId-2, bsr.txQueueSize);

        }
      else
        {
          NS_LOG_DEBUG ("Something wrong with the calculation of overhead."
                        "Active LCS Retx: " << activeTx << " assigned to this: " <<
                        bytesPerLcId << ", with TBS of " << m_ulDci->m_tbSize.at (0) <<
                        " usefulTbs " << usefulTbs << " and total used " << m_ulDciTotalUsed);
        }
    }
}

void
NrUeMac::SendNewData ()
{
  NS_LOG_FUNCTION (this);
  // New transmission -> empty pkt buffer queue (for deleting eventual pkts not acked )
  Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
  m_miUlHarqProcessesPacket.at (m_ulDci->m_harqProcess).m_pktBurst = pb;
  m_miUlHarqProcessesPacket.at (m_ulDci->m_harqProcess).m_lcidList.clear ();
  NS_LOG_INFO ("Reset HARQP " << +m_ulDci->m_harqProcess);

  // Sending the status data has no boundary: let's try to send the ACK as
  // soon as possible, filling the TBS, if necessary.
  SendNewStatusData ();

  // Let's count how many LC we have, that are waiting with some data
  uint16_t activeLcsRetx = 0;
  uint16_t activeLcsTx = 0;
  uint32_t totRetx = 0;
  uint32_t totTx = 0;
  for (const auto & itBsr : m_ulBsrReceived)
    {
      totRetx += itBsr.second.retxQueueSize;
      totTx += itBsr.second.txQueueSize;

      if (itBsr.second.retxQueueSize > 0)
        {
          activeLcsRetx++;
        }
      if (itBsr.second.txQueueSize > 0)
        {
          activeLcsTx++;
        }
    }

  // Of the TBS we received in the DCI, one part is gone for the status pdu,
  // where we didn't check much as it is the most important data, that has to go
  // out. For the rest that we have left, we can use only a part of it because of
  // the overhead of the SHORT_BSR, which is 5 bytes.
  NS_ASSERT_MSG (m_ulDciTotalUsed + 5 <= m_ulDci->m_tbSize.at (0),
                 "The StatusPDU used " << m_ulDciTotalUsed << " B, we don't have any for the SHORT_BSR.");
  uint32_t usefulTbs = m_ulDci->m_tbSize.at (0) - m_ulDciTotalUsed - 5;

  // Now, we have 3 bytes of overhead for each subPDU. Let's try to serve all
  // the queues with some RETX data.
  if (activeLcsRetx * 3 > usefulTbs)
    {
      NS_LOG_DEBUG ("The overhead for transmitting retx data is greater than the space for transmitting it."
                    "Ignore the TBS of " << usefulTbs << " B.");
    }
  else
    {
      usefulTbs -= activeLcsRetx * 3;
      SendRetxData (usefulTbs, activeLcsRetx);
    }

  // Now we have to update our useful TBS for the next transmission.
  // Remember that m_ulDciTotalUsed keep count of data and overhead that we
  // used till now.
  NS_ASSERT_MSG (m_ulDciTotalUsed + 5 <= m_ulDci->m_tbSize.at (0),
                 "The StatusPDU sending required all space, we don't have any for the SHORT_BSR.");
  usefulTbs = m_ulDci->m_tbSize.at (0) - m_ulDciTotalUsed - 5; // Update the usefulTbs.

  // The last part is for the queues with some non-RETX data. If there is no space left,
  // then nothing.
  if (activeLcsTx * 3 > usefulTbs)
    {
      NS_LOG_DEBUG ("The overhead for transmitting new data is greater than the space for transmitting it."
                    "Ignore the TBS of " << usefulTbs << " B.");
    }
  else
    {
      usefulTbs -= activeLcsTx * 3;
      SendTxData (usefulTbs, activeLcsTx);
    }

  // If we did not used the packet burst, explicitly signal it to the HARQ
  // retx, if any.
  if (m_ulDciTotalUsed == 0)
    {
      m_miUlHarqProcessesPacket.at (m_ulDci->m_harqProcess).m_pktBurst = nullptr;
      m_miUlHarqProcessesPacket.at (m_ulDci->m_harqProcess).m_lcidList.clear ();
    }
}


void
NrUeMac::SendNewStatusData()
{
  NS_LOG_FUNCTION (this);

  bool hasStatusPdu = false;
  bool sentOneStatusPdu = false;

  for (auto & bsrIt : m_ulBsrReceived)
    {
      auto & bsr = bsrIt.second;

      if (bsr.statusPduSize > 0)
        {
          hasStatusPdu = true;

          // Check if we have room to transmit the statusPdu
          if (m_ulDciTotalUsed + bsr.statusPduSize <= m_ulDci->m_tbSize.at (0))
            {
              LteMacSapUser::TxOpportunityParameters txParams;
              txParams.lcid = bsr.lcid;
              txParams.rnti = m_rnti;
              txParams.bytes = bsr.statusPduSize;
              txParams.layer = 0;
              txParams.harqId = m_ulDci->m_harqProcess;
              txParams.componentCarrierId = GetBwpId ();

              NS_LOG_INFO ("Notifying RLC of LCID " << +bsr.lcid << " of a TxOpp "
                           "of " << bsr.statusPduSize << " B for a status PDU");

              m_lcInfoMap.at (bsr.lcid).macSapUser->NotifyTxOpportunity (txParams);
              // After this call, m_ulDciTotalUsed has been updated with the
              // correct amount of bytes... but it is up to us in updating the BSR
              // value, substracting the amount of bytes transmitted
              bsr.statusPduSize = 0;
              sentOneStatusPdu = true;
            }
          else
            {
              NS_LOG_INFO ("Cannot send StatusPdu of " << bsr.statusPduSize <<
                           " B, we already used all the TBS");
            }
        }
    }

  NS_ABORT_MSG_IF (hasStatusPdu && !sentOneStatusPdu,
                   "The TBS of size " << m_ulDci->m_tbSize.at (0) << " doesn't allow us "
                   "to send one status PDU...");
}

void
NrUeMac::DoReceiveControlMessage  (Ptr<NrControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  switch (msg->GetMessageType ())
    {
    case (NrControlMessage::UL_DCI):
      {
        ProcessUlDci (DynamicCast<NrUlDciMessage> (msg));
        break;
      }
    case (NrControlMessage::RAR):
      {
        NS_LOG_INFO ("Received RAR in slot " << m_currentSlot);

        m_macRxedCtrlMsgsTrace (m_currentSlot, GetCellId (), m_rnti, GetBwpId (), msg);

        if (m_waitingForRaResponse == true)
          {
            Ptr<NrRarMessage> rarMsg = DynamicCast<NrRarMessage> (msg);
            NS_LOG_LOGIC ("got RAR with RA-RNTI " << +rarMsg->GetRaRnti () <<
                          ", expecting " << +m_raRnti);
            for (auto it = rarMsg->RarListBegin (); it != rarMsg->RarListEnd (); ++it)
              {
                if (it->rapId == m_raPreambleId)
                  {
                    RecvRaResponse (it->rarPayload);
                  }
              }
          }
        break;
      }

    default:
      NS_LOG_LOGIC ("Control message not supported/expected");
    }
}

NrUePhySapUser*
NrUeMac::GetPhySapUser ()
{
  return m_phySapUser;
}

void
NrUeMac::SetPhySapProvider (NrPhySapProvider* ptr)
{
  m_phySapProvider = ptr;
}

void
NrUeMac::DoConfigureRach ([[maybe_unused]] LteUeCmacSapProvider::RachConfig rc)
{
  NS_LOG_FUNCTION (this);
}

void
NrUeMac::DoStartContentionBasedRandomAccessProcedure ()
{
  NS_LOG_FUNCTION (this);
  RandomlySelectAndSendRaPreamble ();
}

void
NrUeMac::RandomlySelectAndSendRaPreamble ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (m_currentSlot << " Received System Information, send to PHY the RA preamble");
  SendRaPreamble (true);
}

void
NrUeMac::SendRaPreamble ([[maybe_unused]] bool contention)
{
  NS_LOG_INFO (this);
  //m_raPreambleId = m_raPreambleUniformVariable->GetInteger (0, 64 - 1);
  m_raPreambleId = g_raPreambleId++;
  /*raRnti should be subframeNo -1 */
  m_raRnti = 1;

  Ptr<NrRachPreambleMessage> rachMsg = Create<NrRachPreambleMessage> ();
  rachMsg->SetSourceBwp (GetBwpId ());
  m_macTxedCtrlMsgsTrace (m_currentSlot, GetCellId (), m_rnti, GetBwpId (), rachMsg);

  m_phySapProvider->SendRachPreamble (m_raPreambleId, m_raRnti);
}

void
NrUeMac::DoStartNonContentionBasedRandomAccessProcedure (uint16_t rnti, [[maybe_unused]] uint8_t preambleId, uint8_t prachMask)
{
  NS_LOG_FUNCTION (this << " rnti" << rnti);
  NS_ASSERT_MSG (prachMask == 0, "requested PRACH MASK = " << (uint32_t) prachMask << ", but only PRACH MASK = 0 is supported");
  m_rnti = rnti;
}

void
NrUeMac::AddLc (uint8_t lcId,  LteUeCmacSapProvider::LogicalChannelConfig lcConfig, LteMacSapUser* msu)
{
  NS_LOG_FUNCTION (this << " lcId" << (uint32_t) lcId);
  NS_ASSERT_MSG (m_lcInfoMap.find (lcId) == m_lcInfoMap.end (), "cannot add channel because LCID " << lcId << " is already present");

  LcInfo lcInfo;
  lcInfo.lcConfig = lcConfig;
  lcInfo.macSapUser = msu;
  m_lcInfoMap[lcId] = lcInfo;
}

void
NrUeMac::DoRemoveLc (uint8_t lcId)
{
  NS_LOG_FUNCTION (this << " lcId" << lcId);
}

LteMacSapProvider*
NrUeMac::GetUeMacSapProvider (void)
{
  return m_macSapProvider;
}

void
NrUeMac::DoReset ()
{
  NS_LOG_FUNCTION (this);
}
//////////////////////////////////////////////

int64_t
NrUeMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_raPreambleUniformVariable->SetStream (stream);
  return 1;
}

//Configured Grant

bool  MacUeMemberPhySapUser::SlotIndication_configuredGrant (SfnSf sfn)
{
  return m_mac->DoSlotIndication_configuredGrant(sfn);
}

bool
NrUeMac::DoSlotIndication_configuredGrant (const SfnSf &sfn)
{

  NS_LOG_FUNCTION (this);
  m_currentSlot = sfn;
  NS_LOG_INFO ("Slot " << m_currentSlot);

  RefreshHarqProcessesPacketBuffer ();

  if (m_srState_configuredGrant == TO_SEND_TrafficInfo)
    {
      uint32_t bufSr = GetTotalBufSize();
      NS_LOG_INFO ("Sending CGR to PHY in slot " << sfn <<", changes the state"
                   ": TO_SEND_TrafficInfo -> TO_RECEIVE_CG, with bufSr: "<< bufSr);
      if (Simulator::Now() == m_traffStartTime )
      {
          m_traffStartTime = m_traffStartTime - Simulator::Now();
      }
      else
      {
        m_traffStartTime = m_traffStartTime - m_startSlotTime ;
      }
      //m_traffDeadlineTime = m_traffDeadlineTime + m_startSlotTime;
      std::cout << "nanoseconds since epoch StartSlot: "
                                << m_startSlotTime<<
                                "  InitTime: "
                                << m_traffStartTime<<
                                "  DeadlineTime"
                                <<m_traffDeadlineTime<<'\n';
      // UE MAC sends CGR to PHY in order to send to the gNB
      SendTrafficInfo();

      m_srState_configuredGrant = TO_RECEIVE_CG;
      configuredGrant_state = false;
      return configuredGrant_state;
    }
  else if (m_srState_configuredGrant == SCH_CG_DATA)
    {

      if (m_dciGranted_stored[cg_slot_counter_DCI_2] == nullptr)
        {
          // The packet has already been transmitted, we switch to the ACTIVE_CG status,
          // We will be in this state until the following periodic transmission.
          m_srState_configuredGrant = ACTIVE_CG;
          cg_slot_counter_DCI_2 = 0;
        }
      else
        {
          uint8_t streamId = 0;
          auto it = m_dci_cg_map.find (m_currentSlot.GetEncForStreamWithSymStart (streamId, m_dciGranted_stored[cg_slot_counter_DCI_2]->m_symStart));

          if (it == m_dci_cg_map.end ())
            {
              // Do nothing
            }
          else
            {
              // Processes the current packet
              m_ulDciSfnsf = m_currentSlot;
              m_ulDci = it->second;
              m_dci_cg_map.erase (it);
              ProcessULPacket();

              NS_LOG_INFO ("Sending a packet to PHY layer in slot " << m_ulDciSfnsf);

              // Prepare resources for future transmission
              SfnSf dataSfn_cg = m_currentSlot;
              uint8_t numberOfSlot_insideOneSubframe = pow(2,(m_currentSlot.GetNumerology ()));
              m_cgPeriod = GetCGPeriod();
              uint8_t number_slots_configuration = (m_cgPeriod*numberOfSlot_insideOneSubframe);
              dataSfn_cg.Add(number_slots_configuration);
              uint64_t key = dataSfn_cg.GetEncForStreamWithSymStart (streamId, m_dciGranted_stored[cg_slot_counter_DCI_2]->m_symStart);

              auto it_1 = m_dci_cg_map.find (key);

              if (it_1 == m_dci_cg_map.end ())
                {
                  it_1 = m_dci_cg_map.insert (std::make_pair (key, m_dciGranted_stored[cg_slot_counter_DCI_2])).first;
                  cg_slot_counter_DCI_2 = cg_slot_counter_DCI_2 +1;
                }

            }
        }
      //Send the SCH_CG_DATA state to UE-PHY
      configuredGrant_state = true;
      return configuredGrant_state;
    }
  else
    {
      m_startSlotTime = Simulator::Now ();
      configuredGrant_state = false;
      return configuredGrant_state;
    }
  // Feedback missing
}

void
NrUeMac::ProcessULPacket()
{
  NS_LOG_FUNCTION (this);

  // Saving the data we need in DoTransmitPdu
  m_ulDciTotalUsed = 0;


  //m_macRxedCtrlMsgsTrace (m_currentSlot, GetCellId (), m_rnti, GetBwpId (), dciMsg);

  NS_LOG_INFO ("UL DCI received, transmit data in slot " << m_ulDciSfnsf <<
               " Harq Process " << +m_ulDci->m_harqProcess <<
               " TBS " << m_ulDci->m_tbSize.at (0) << " total queue " << GetTotalBufSize ());


  if (m_ulDci->m_ndi.at (0) == 0)
    {
      // This method will retransmit the data saved in the harq buffer
      TransmitRetx ();

      // This method will transmit a new BSR.
      SendReportBufferStatus (m_ulDciSfnsf, m_ulDci->m_symStart);
    }
  else if (m_ulDci->m_ndi.at (0) == 1)
    {
      SendNewData ();

      NS_LOG_INFO ("After sending NewData, bufSize " << GetTotalBufSize ());

      // Send a new BSR. SendNewData() already took into account the size of
      // the BSR.
      SendReportBufferStatus (m_ulDciSfnsf, m_ulDci->m_symStart);
      NS_LOG_INFO ("UL DCI processing done, sent to PHY a total of " << m_ulDciTotalUsed <<
                   " B out of " << m_ulDci->m_tbSize.at (0) << " allocated bytes ");

     if (GetTotalBufSize () == 0)
        {
          // the UE may have been scheduled, but we didn't use a single byte
          // of the allocation. So send an empty PDU. This happens because the
          // byte reporting in the BSR is not accurate, due to RLC and/or
          // BSR quantization.
          if (m_ulDciTotalUsed == 0)
            {
              NS_LOG_WARN ("No byte used for this UL-DCI, sending empty PDU");

              LteMacSapProvider::TransmitPduParameters txParams;

              txParams.pdu = Create<Packet> ();
              txParams.lcid = 3;
              txParams.rnti = m_rnti;
              txParams.layer = 0;
              txParams.harqProcessId = m_ulDci->m_harqProcess;
              txParams.componentCarrierId = GetBwpId ();

              DoTransmitPdu (txParams);
            }
        }
    }
}

uint8_t
NrUeMac::GetConfigurationTime () const
{
  return m_configurationTime;
}

void
NrUeMac::SetConfigurationTime (uint8_t v)
{
  m_configurationTime = v;
}

uint8_t
NrUeMac::GetCGPeriod () const
{
  return m_cgPeriod;
}

void
NrUeMac::SetCGPeriod (uint8_t v)
{
  m_cgPeriod = v;
}

void
NrUeMac::SetCG (bool CGsch)
{
  m_cgScheduling = CGsch;
}

bool
NrUeMac::GetCG () const
{
  return m_cgScheduling;
}

void
NrUeMac::SendTrafficInfo () const
{
  NS_LOG_FUNCTION (this);

  if (m_rnti == 0)
    {
      NS_LOG_INFO ("MAC not initialized, CGR deferred");
      return;
    }

  // create the CGR to send to the gNB
  Ptr<NrCGRMessage> msg = Create<NrCGRMessage> ();
  msg->SetSourceBwp (GetBwpId ());
  msg->SetRNTI (m_rnti);
  msg -> SetBufSize(GetTotalBufSize());
  msg -> SetTrafficP(m_trafficPeriodicity);
  msg -> SetTrafficTimeInit(m_traffStartTime);
  msg -> SetTrafficDeadline(m_traffDeadlineTime);

  for (auto it = m_ulBsrReceived.cbegin (); it != m_ulBsrReceived.cend (); ++it)
    {
      if ((*it).second.rnti == m_rnti)
        {
          uint8_t lcid = (*it).first;
          msg -> SetLCID(lcid);
        }
    }

  m_phySapProvider->SendControlMessage (msg);
}

}
