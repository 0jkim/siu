/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
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
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *                Sourjya Dutta <sdutta@nyu.edu>
 *                Russell Ford <russell.ford@nyu.edu>
 *                Menglei Zhang <menglei@nyu.edu>
 *                Biljana Bojovic <bbojovic@cttc.es>
 */

#define NS_LOG_APPEND_CONTEXT                                            \
  do                                                                     \
    {                                                                    \
      if (m_phyMacConfig)                                                \
        {                                                                \
          std::clog << " [ccId "                                         \
                    << static_cast<uint32_t> (m_phyMacConfig->GetCcId ())\
                    << "] ";                                             \
        }                                                                \
    }                                                                    \
  while (false);

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include "mmwave-ue-phy.h"
#include "mmwave-ue-net-device.h"
#include "mmwave-spectrum-value-helper.h"
#include <ns3/pointer.h>
#include <ns3/node.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveUePhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveUePhy);

MmWaveUePhy::MmWaveUePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

MmWaveUePhy::MmWaveUePhy (Ptr<MmWaveSpectrumPhy> dlPhy, Ptr<MmWaveSpectrumPhy> ulPhy,
                          const Ptr<Node> &n)
  : MmWavePhy (dlPhy, ulPhy),
  m_rnti (0)
{
  NS_LOG_FUNCTION (this);
  m_wbCqiLast = Simulator::Now ();
  m_ueCphySapProvider = new MemberLteUeCphySapProvider<MmWaveUePhy> (this);
  Simulator::ScheduleWithContext (n->GetId (), MilliSeconds (0),
                                  &MmWaveUePhy::StartSlot, this, 0, 0, 0);
}

MmWaveUePhy::~MmWaveUePhy ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MmWaveUePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveUePhy")
    .SetParent<MmWavePhy> ()
    .AddConstructor<MmWaveUePhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&MmWaveUePhy::SetTxPower,
                                       &MmWaveUePhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0.\" "
                  "In this model, we consider T0 = 290K.",
                   DoubleValue (5.0), // mmwave code from NYU and UniPd assumed in the code the value of 5dB, thats why we configure the default value to that
                   MakeDoubleAccessor (&MmWavePhy::SetNoiseFigure,
                   &MmWavePhy::GetNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("DlSpectrumPhy",
                   "The downlink MmWaveSpectrumPhy associated to this MmWavePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&MmWaveUePhy::GetDlSpectrumPhy),
                   MakePointerChecker <MmWaveSpectrumPhy> ())
    .AddAttribute ("UlSpectrumPhy",
                   "The uplink MmWaveSpectrumPhy associated to this MmWavePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&MmWaveUePhy::GetUlSpectrumPhy),
                   MakePointerChecker <MmWaveSpectrumPhy> ())
    .AddAttribute ("AntennaArrayType",
                    "AntennaArray of this UE phy. There are two types of antenna array available: "
                    "a) AntennaArrayModel which is using isotropic antenna elements, and "
                    "b) AntennaArray3gppModel which is using directional 3gpp antenna elements."
                    "Another important parameters to specify is the number of antenna elements by "
                    "dimension.",
                    TypeIdValue(ns3::AntennaArrayModel::GetTypeId()),
                    MakeTypeIdAccessor (&MmWavePhy::SetAntennaArrayType,
                                        &MmWavePhy::GetAntennaArrayType),
                    MakeTypeIdChecker())
    .AddAttribute ("AntennaNumDim1",
                   "Size of the first dimension of the antenna sector/panel expressed in number of antenna elements",
                   UintegerValue (2),
                   MakeUintegerAccessor (&MmWavePhy::SetAntennaNumDim1,
                                         &MmWavePhy::GetAntennaNumDim1),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("AntennaNumDim2",
                   "Size of the second dimension of the antenna sector/panel expressed in number of antenna elements",
                   UintegerValue (4),
                   MakeUintegerAccessor (&MmWavePhy::SetAntennaNumDim2,
                                         &MmWavePhy::GetAntennaNumDim2),
                   MakeUintegerChecker<uint8_t> ())
    .AddTraceSource ("ReportCurrentCellRsrpSinr",
                     "RSRP and SINR statistics.",
                     MakeTraceSourceAccessor (&MmWaveUePhy::m_reportCurrentCellRsrpSinrTrace),
                     "ns3::CurrentCellRsrpSinr::TracedCallback")
    .AddTraceSource ("ReportUplinkTbSize",
                     "Report allocated uplink TB size for trace.",
                     MakeTraceSourceAccessor (&MmWaveUePhy::m_reportUlTbSize),
                     "ns3::UlTbSize::TracedCallback")
    .AddTraceSource ("ReportDownlinkTbSize",
                     "Report allocated downlink TB size for trace.",
                     MakeTraceSourceAccessor (&MmWaveUePhy::m_reportDlTbSize),
                     "ns3::DlTbSize::TracedCallback");
  return tid;
}

void
MmWaveUePhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  SfnSf sfnf = SfnSf (0, 0, 0, 0);
  std::vector<uint8_t> rbgBitmask (m_phyMacConfig->GetBandwidthInRbg (), 1);


  for (unsigned i = 0; i < m_phyMacConfig->GetSlotsPerSubframe (); i++)
    {
      SlotAllocInfo sai = SlotAllocInfo (sfnf);
      DciInfoElementTdma dlDci (0, 1, rbgBitmask);
      DciInfoElementTdma ulDci (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, rbgBitmask);
      VarTtiAllocInfo dlCtrlSlot (VarTtiAllocInfo::DL,
                                  VarTtiAllocInfo::CTRL,
                                  std::make_shared<DciInfoElementTdma> (0, 1, rbgBitmask));
      VarTtiAllocInfo ulCtrlSlot (VarTtiAllocInfo::UL,
                                  VarTtiAllocInfo::CTRL,
                                  std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, rbgBitmask));
      sai.m_varTtiAllocInfo.push_back (dlCtrlSlot);
      sai.m_varTtiAllocInfo.push_back (ulCtrlSlot);
      SetSlotAllocInfo (sai);

      sfnf = sfnf.IncreaseNoOfSlots (m_phyMacConfig->GetSlotsPerSubframe (),
                                     m_phyMacConfig->GetSubframesPerFrame ());
    }

  m_slotPeriod = m_phyMacConfig->GetSlotPeriod ();

  MmWavePhy::InstallAntenna ();
  NS_ASSERT_MSG (GetAntennaArray(), "Error in initialization of the AntennaModel object");
  Ptr<AntennaArray3gppModel> antenna3gpp = DynamicCast<AntennaArray3gppModel> (GetAntennaArray());

  if (antenna3gpp)
    {
      antenna3gpp->SetIsUe(true);
    }

  m_downlinkSpectrumPhy->SetAntenna (GetAntennaArray());
  m_uplinkSpectrumPhy->SetAntenna (GetAntennaArray());

  NS_LOG_INFO ("UE antenna array initialised:"<<(unsigned)GetAntennaArray()->GetAntennaNumDim1() <<
                         ", "<< (unsigned)GetAntennaArray()->GetAntennaNumDim2());

}

void
MmWaveUePhy::DoDispose (void)
{
  delete m_ueCphySapProvider;
  MmWavePhy::DoDispose ();
}

void
MmWaveUePhy::SetUeCphySapUser (LteUeCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_ueCphySapUser = s;
}

LteUeCphySapProvider*
MmWaveUePhy::GetUeCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_ueCphySapProvider);
}

void
MmWaveUePhy::SetTxPower (double pow)
{
  m_txPower = pow;
}
double
MmWaveUePhy::GetTxPower () const
{
  return m_txPower;
}

void
MmWaveUePhy::SetNoiseFigure (double pf)
{
  m_noiseFigure = pf;
}

double
MmWaveUePhy::GetNoiseFigure () const
{
  return m_noiseFigure;
}

void
MmWaveUePhy::DoSetSubChannels ()
{

}

void
MmWaveUePhy::SetSubChannelsForReception (std::vector <int> mask)
{

}

std::vector <int>
MmWaveUePhy::GetSubChannelsForReception (void)
{
  std::vector <int> vec;

  return vec;
}

void
MmWaveUePhy::SetSubChannelsForTransmission (std::vector <int> mask)
{
  Ptr<SpectrumValue> txPsd = GetTxPowerSpectralDensity (mask);
  NS_ASSERT (txPsd);
  m_downlinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}

std::vector <int>
MmWaveUePhy::GetSubChannelsForTransmission (void)
{
  std::vector <int> vec;

  return vec;
}

void
MmWaveUePhy::DoSendControlMessage (Ptr<MmWaveControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  SetControlMessage (msg);
}


void
MmWaveUePhy::RegisterToEnb (uint16_t cellId, Ptr<MmWavePhyMacCommon> config)
{
  m_cellId = cellId;
  //TBD how to assign bandwitdh and earfcn
  m_phyMacConfig = config;

  Ptr<SpectrumValue> noisePsd = GetNoisePowerSpectralDensity ();
  m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
  m_downlinkSpectrumPhy->GetSpectrumChannel ()->AddRx (m_downlinkSpectrumPhy);
  m_downlinkSpectrumPhy->SetCellId (m_cellId);

  GetAntennaArray()->SetSpectrumModel (m_downlinkSpectrumPhy->GetRxSpectrumModel());
}

Ptr<MmWaveSpectrumPhy>
MmWaveUePhy::GetDlSpectrumPhy () const
{
  return m_downlinkSpectrumPhy;
}

Ptr<MmWaveSpectrumPhy>
MmWaveUePhy::GetUlSpectrumPhy () const
{
  return m_uplinkSpectrumPhy;
}

void
MmWaveUePhy::ReceiveControlMessageList (std::list<Ptr<MmWaveControlMessage> > msgList)
{
  NS_LOG_FUNCTION (this);
  bool dlUpdated = false;
  bool ulUpdated = false;

  SfnSf ulSfnSf = m_currSlotAllocInfo.m_sfnSf.CalculateUplinkSlot (m_phyMacConfig->GetUlSchedDelay (),
                                                                   m_phyMacConfig->GetSlotsPerSubframe (),
                                                                   m_phyMacConfig->GetSubframesPerFrame ());

  for (const auto &msg : msgList)
    {
      if (msg->GetMessageType () == MmWaveControlMessage::DCI_TDMA)
        {
          NS_ASSERT_MSG (m_varTtiNum == 0, "UE" << m_rnti << " got DCI on slot != 0");
          Ptr<MmWaveTdmaDciMessage> dciMsg = DynamicCast<MmWaveTdmaDciMessage> (msg);
          auto dciInfoElem = dciMsg->GetDciInfoElement ();
          SfnSf dciSfn = dciMsg->GetSfnSf ();

          if (dciSfn.m_frameNum != m_frameNum || dciSfn.m_subframeNum != m_subframeNum)
            {
              NS_FATAL_ERROR ("DCI intended for different subframe (dci= "
                              << dciSfn.m_frameNum << " "
                              << dciSfn.m_subframeNum << ", actual= "
                              << m_frameNum << " " << m_subframeNum);
            }

          if (dciInfoElem->m_rnti != m_rnti)
            {
              continue;   // DCI not for me
            }

          if (dciInfoElem->m_format == DciInfoElementTdma::DL)   // set downlink slot schedule for current slot
            {
              NS_LOG_DEBUG ("UE" << m_rnti << " DL-DCI received for slot " << dciSfn <<
                            " symStart " << static_cast<uint32_t> (dciInfoElem->m_symStart) <<
                            " numSym " << static_cast<uint32_t> (dciInfoElem->m_numSym) <<
                            " tbs " << dciInfoElem->m_tbSize <<
                            " harqId " << static_cast<uint32_t> (dciInfoElem->m_harqProcess));

              VarTtiAllocInfo varTtiInfo (VarTtiAllocInfo::DL, VarTtiAllocInfo::DATA, dciInfoElem);
              m_currSlotAllocInfo.m_varTtiAllocInfo.push_back (varTtiInfo);
              dlUpdated = true;
            }
          else if (dciInfoElem->m_format == DciInfoElementTdma::UL)   // set downlink slot schedule for t+Tul_sched slot
            {

              NS_LOG_DEBUG ("UE" << m_rnti <<
                            " UL-DCI received for slot " << ulSfnSf <<
                            " symStart " << static_cast<uint32_t> (dciInfoElem->m_symStart) <<
                            " numSym " << static_cast<uint32_t> (dciInfoElem->m_numSym) <<
                            " tbs " << dciInfoElem->m_tbSize <<
                            " harqId " << static_cast<uint32_t> (dciInfoElem->m_harqProcess));

              VarTtiAllocInfo varTtiInfo (VarTtiAllocInfo::UL, VarTtiAllocInfo::DATA, dciInfoElem);

              if (m_phyMacConfig->GetUlSchedDelay () == 0)
                {
                  m_currSlotAllocInfo.m_varTtiAllocInfo.push_back (varTtiInfo);
                  ulUpdated = true;
                }
              else
                {
                  if (SlotExists (ulSfnSf))
                    {
                      auto & ulSlot = PeekSlotAllocInfo (ulSfnSf);
                      ulSlot.m_varTtiAllocInfo.push_back (varTtiInfo);
                    }
                  else
                    {
                      std::vector<uint8_t> rbgBitmask (m_phyMacConfig->GetBandwidthInRbg (), 1);
                      SlotAllocInfo slotAllocInfo = SlotAllocInfo (ulSfnSf);
                      DciInfoElementTdma dciDl (0, 1, rbgBitmask);
                      DciInfoElementTdma dciUl (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, rbgBitmask);
                      VarTtiAllocInfo dlCtrlSlot (VarTtiAllocInfo::DL,
                                                  VarTtiAllocInfo::CTRL,
                                                  std::make_shared<DciInfoElementTdma> (0, 1, rbgBitmask));
                      VarTtiAllocInfo ulCtrlSlot (VarTtiAllocInfo::UL,
                                                  VarTtiAllocInfo::CTRL,
                                                  std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, rbgBitmask));
                      slotAllocInfo.m_varTtiAllocInfo.push_front (dlCtrlSlot);
                      slotAllocInfo.m_varTtiAllocInfo.push_back (varTtiInfo);
                      slotAllocInfo.m_varTtiAllocInfo.push_back (ulCtrlSlot);
                      SetSlotAllocInfo (slotAllocInfo);
                    }
                  ulUpdated = true;
                }
            }

          m_phySapUser->ReceiveControlMessage (msg);
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::MIB)
        {
          NS_LOG_INFO ("received MIB");
          NS_ASSERT (m_cellId > 0);
          Ptr<MmWaveMibMessage> msg2 = DynamicCast<MmWaveMibMessage> (msg);
          m_ueCphySapUser->RecvMasterInformationBlock (m_cellId, msg2->GetMib ());
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::SIB1)
        {
          NS_ASSERT (m_cellId > 0);
          Ptr<MmWaveSib1Message> msg2 = DynamicCast<MmWaveSib1Message> (msg);
          m_ueCphySapUser->RecvSystemInformationBlockType1 (m_cellId, msg2->GetSib1 ());
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::RAR)
        {
          NS_LOG_INFO ("received RAR");
          NS_ASSERT (m_cellId > 0);

          Ptr<MmWaveRarMessage> rarMsg = DynamicCast<MmWaveRarMessage> (msg);

          for (std::list<MmWaveRarMessage::Rar>::const_iterator it = rarMsg->RarListBegin ();
               it != rarMsg->RarListEnd ();
               ++it)
            {
              if (it->rapId == m_raPreambleId)
                {
                  m_phySapUser->ReceiveControlMessage (rarMsg);
                }
            }
        }
      else
        {
          m_phySapUser->ReceiveControlMessage (msg);
        }
    }

  if (dlUpdated)
    {
      std::sort (m_currSlotAllocInfo.m_varTtiAllocInfo.begin (), m_currSlotAllocInfo.m_varTtiAllocInfo.end ());
    }

  if (ulUpdated)
    {
      if (m_phyMacConfig->GetUlSchedDelay () == 0)
        {
          std::sort (m_currSlotAllocInfo.m_varTtiAllocInfo.begin (), m_currSlotAllocInfo.m_varTtiAllocInfo.end ());
        }
      else if (m_phyMacConfig->GetUlSchedDelay () > 0 && SlotExists (ulSfnSf))
        {
          auto & ulSlot = PeekSlotAllocInfo (ulSfnSf);
          std::sort (ulSlot.m_varTtiAllocInfo.begin (), ulSlot.m_varTtiAllocInfo.end ());
        }
    }
}

void
MmWaveUePhy::QueueUlTbAlloc (TbAllocInfo m)
{
  NS_LOG_FUNCTION (this);
  //  NS_LOG_DEBUG ("UL TB Info Elem queue size == " << m_ulTbAllocQueue.size ());
  m_ulTbAllocQueue.at (m_phyMacConfig->GetUlSchedDelay () - 1).push_back (m);
}

std::list<TbAllocInfo>
MmWaveUePhy::DequeueUlTbAlloc (void)
{
  NS_LOG_FUNCTION (this);

  if (m_ulTbAllocQueue.empty ())
    {
      std::list<TbAllocInfo> emptylist;
      return (emptylist);
    }

  if (m_ulTbAllocQueue.at (0).size () > 0)
    {
      std::list<TbAllocInfo> ret = m_ulTbAllocQueue.at (0);
      m_ulTbAllocQueue.erase (m_ulTbAllocQueue.begin ());
      std::list<TbAllocInfo> l;
      m_ulTbAllocQueue.push_back (l);
      return (ret);
    }
  else
    {
      m_ulTbAllocQueue.erase (m_ulTbAllocQueue.begin ());
      std::list<TbAllocInfo> l;
      m_ulTbAllocQueue.push_back (l);
      std::list<TbAllocInfo> emptylist;
      return (emptylist);
    }
}

void
MmWaveUePhy::StartSlot (uint16_t frameNum, uint8_t sfNum, uint16_t slotNum)
{
  m_frameNum = frameNum;
  m_subframeNum = sfNum;
  m_slotNum = slotNum;
  m_lastSlotStart = Simulator::Now ();

  // update the current slot
  m_currSlotAllocInfo = GetSlotAllocInfo (SfnSf (frameNum, sfNum, slotNum, m_varTtiNum));

  if (m_currSlotAllocInfo.m_varTtiAllocInfo.size () == 0)
    {
      NS_ASSERT (m_currSlotAllocInfo.m_numSymAlloc == 0);
      NS_LOG_INFO ("No allocation in this slot, directly go to the end of the slot");
      Simulator::Schedule (m_phyMacConfig->GetSlotPeriod(), &MmWaveUePhy::EndVarTti, this);
      return;
    }

  NS_ASSERT ((m_currSlotAllocInfo.m_sfnSf.m_frameNum == m_frameNum)
             && (m_currSlotAllocInfo.m_sfnSf.m_subframeNum == m_subframeNum
                 && m_currSlotAllocInfo.m_sfnSf.m_slotNum == m_slotNum));

  NS_LOG_INFO ("UE " << m_rnti << " start slot " << m_currSlotAllocInfo.m_sfnSf <<
               " composed by the following allocations, total " << m_currSlotAllocInfo.m_varTtiAllocInfo.size ());
  for (const auto & alloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
    {
      std::string direction, type;
      if (alloc.m_varTtiType == VarTtiAllocInfo::CTRL)
        {
          type = "CTRL";
        }
      else if (alloc.m_varTtiType == VarTtiAllocInfo::CTRL_DATA)
        {
          type = "CTRL_DATA";
        }
      else
        {
          type = "DATA";
        }

      if (alloc.m_tddMode == VarTtiAllocInfo::UL)
        {
          direction = "UL";
        }
      else
        {
          direction = "DL";
        }
      NS_LOG_INFO ("Allocation from sym " << static_cast<uint32_t> (alloc.m_dci->m_symStart) <<
                   " to sym " << static_cast<uint32_t> (alloc.m_dci->m_numSym + alloc.m_dci->m_symStart) <<
                   " direction " << direction << " type " << type);
    }

  auto currentDci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci;
  auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * currentDci->m_symStart;

  Simulator::Schedule (nextVarTtiStart, &MmWaveUePhy::StartVarTti, this);
}


Time
MmWaveUePhy::DlCtrl(const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);

  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * m_phyMacConfig->GetDlCtrlSymbols ();

  NS_LOG_DEBUG ("UE" << m_rnti <<
                " RXing DL CTRL frame for"
                " symbols "  << +dci->m_symStart <<
                "-" << +(dci->m_symStart + dci->m_numSym - 1) <<
                "\t start " << Simulator::Now () <<
                " end " << (Simulator::Now () + varTtiPeriod));
  return varTtiPeriod;
}

Time
MmWaveUePhy::UlCtrl (const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);

  std::vector<int> channelRbs;
  for (uint32_t i = 0; i < m_phyMacConfig->GetBandwidthInRbs (); i++)
    {
      channelRbs.push_back (static_cast<int> (i));
    }

  SetSubChannelsForTransmission (channelRbs);
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * m_phyMacConfig->GetUlCtrlSymbols ();

  std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
  NS_LOG_DEBUG ("UE" << m_rnti << " TXing UL CTRL frame for symbols " <<
                +dci->m_symStart << "-" <<
                +(dci->m_symStart + dci->m_numSym - 1) <<
                "\t start " << Simulator::Now () << " end " <<
                (Simulator::Now () + varTtiPeriod - NanoSeconds (1.0)));
  SendCtrlChannels (ctrlMsg, varTtiPeriod - NanoSeconds (1.0));
  return varTtiPeriod;
}

Time
MmWaveUePhy::DlData (const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);

  m_receptionEnabled = true;
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * dci->m_numSym;

  m_downlinkSpectrumPhy->AddExpectedTb (dci->m_rnti, dci->m_ndi, dci->m_tbSize, dci->m_mcs,
                                        FromRBGBitmaskToRBAssignment (dci->m_rbgBitmask),
                                        dci->m_harqProcess, dci->m_rv, true,
                                        dci->m_symStart, dci->m_numSym);
  m_reportDlTbSize (GetDevice ()->GetObject <MmWaveUeNetDevice> ()->GetImsi (), dci->m_tbSize);
  NS_LOG_DEBUG ("UE" << m_rnti <<
                " RXing DL DATA frame for"
                " symbols "  << +dci->m_symStart <<
                "-" << +(dci->m_symStart + dci->m_numSym - 1) <<
                " num of rbg assigned: " << FromRBGBitmaskToRBAssignment (dci->m_rbgBitmask).size () <<
                "\t start " << Simulator::Now () <<
                " end " << (Simulator::Now () + varTtiPeriod));

  return varTtiPeriod;
}

Time
MmWaveUePhy::UlData(const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);
  SetSubChannelsForTransmission (FromRBGBitmaskToRBAssignment (dci->m_rbgBitmask));
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * dci->m_numSym;
  std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
  Ptr<PacketBurst> pktBurst = GetPacketBurst (SfnSf (m_frameNum, m_subframeNum, m_slotNum, dci->m_symStart));
  if (pktBurst && pktBurst->GetNPackets () > 0)
    {
      std::list< Ptr<Packet> > pkts = pktBurst->GetPackets ();
      MmWaveMacPduTag tag;
      pkts.front ()->PeekPacketTag (tag);
      NS_ASSERT ((tag.GetSfn ().m_subframeNum == m_subframeNum) && (tag.GetSfn ().m_varTtiNum == dci->m_symStart));

      LteRadioBearerTag bearerTag;
      if (!pkts.front ()->PeekPacketTag (bearerTag))
        {
          NS_FATAL_ERROR ("No radio bearer tag");
        }
    }
  else
    {
      NS_LOG_DEBUG ("Send an empty PDU .... ");
      // sometimes the UE will be scheduled when no data is queued
      // in this case, send an empty PDU
      MmWaveMacPduTag tag (SfnSf (m_frameNum, m_subframeNum, m_slotNum, dci->m_symStart));
      Ptr<Packet> emptyPdu = Create <Packet> ();
      MmWaveMacPduHeader header;
      MacSubheader subheader (3, 0);    // lcid = 3, size = 0
      header.AddSubheader (subheader);
      emptyPdu->AddHeader (header);
      emptyPdu->AddPacketTag (tag);
      LteRadioBearerTag bearerTag (m_rnti, 3, 0);
      emptyPdu->AddPacketTag (bearerTag);
      pktBurst = CreateObject<PacketBurst> ();
      pktBurst->AddPacket (emptyPdu);
    }
  m_reportUlTbSize (GetDevice ()->GetObject <MmWaveUeNetDevice> ()->GetImsi (), dci->m_tbSize);

  NS_LOG_DEBUG ("UE" << m_rnti <<
                " TXing UL DATA frame for" <<
                " symbols "  << +dci->m_symStart <<
                "-" << +(dci->m_symStart + dci->m_numSym - 1)
                     << "\t start " << Simulator::Now () <<
                " end " << (Simulator::Now () + varTtiPeriod));

  Simulator::Schedule (NanoSeconds (1.0), &MmWaveUePhy::SendDataChannels, this,
                       pktBurst, ctrlMsg, varTtiPeriod - NanoSeconds (2.0), m_varTtiNum);
  return varTtiPeriod;
}

void
MmWaveUePhy::StartVarTti ()
{
  NS_LOG_FUNCTION (this);
  Time varTtiPeriod;
  const VarTtiAllocInfo & currSlot = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum];

  m_currNumSym = currSlot.m_dci->m_numSym;
  m_currTbs = currSlot.m_dci->m_tbSize;
  m_receptionEnabled = false;
  m_prevSlotDir = currSlot.m_tddMode;

  // This is necessary as the MAC will compare the TtiNum, and if matches,
  // will send the BSR
  m_phySapUser->SlotIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, currSlot.m_dci->m_symStart));   // trigger mac

  NS_LOG_DEBUG ("UE " << m_rnti << " frame " << static_cast<uint32_t> (m_frameNum) <<
                " subframe " << static_cast<uint32_t> (m_subframeNum) << " slot " <<
                static_cast<uint32_t> (m_slotNum) << " sym " <<
                static_cast<uint32_t> (currSlot.m_dci->m_symStart));

  if (currSlot.m_varTtiType == VarTtiAllocInfo::CTRL && currSlot.m_tddMode == VarTtiAllocInfo::DL)
    {
      varTtiPeriod = DlCtrl (currSlot.m_dci);
    }
  else if (currSlot.m_varTtiType == VarTtiAllocInfo::CTRL && currSlot.m_tddMode == VarTtiAllocInfo::UL)
    {
      varTtiPeriod = UlCtrl (currSlot.m_dci);
    }
  else if (currSlot.m_varTtiType == VarTtiAllocInfo::DATA && currSlot.m_tddMode == VarTtiAllocInfo::DL)
    {
      varTtiPeriod = DlData (currSlot.m_dci);
    }
  else if (currSlot.m_varTtiType == VarTtiAllocInfo::DATA && currSlot.m_tddMode == VarTtiAllocInfo::UL)
    {
      varTtiPeriod = UlData (currSlot.m_dci);
    }

  Simulator::Schedule (varTtiPeriod, &MmWaveUePhy::EndVarTti, this);
}


void
MmWaveUePhy::EndVarTti ()
{
  NS_LOG_FUNCTION (this);
  if (m_varTtiNum == m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1)
    {
      // end of slot
      SfnSf retVal = SfnSf (m_frameNum, m_subframeNum, m_slotNum,0).IncreaseNoOfSlots (m_phyMacConfig->GetSlotsPerSubframe (),
                                                                                       m_phyMacConfig->GetSubframesPerFrame ());
      m_varTtiNum = 0;

      if (!SlotExists (retVal))
        {
          // prepare the following slot info
          std::vector<uint8_t> rbgBitmask (m_phyMacConfig->GetBandwidthInRbg (), 1);
          SlotAllocInfo slotAllocInfo = SlotAllocInfo (retVal);
          VarTtiAllocInfo dlCtrlSlot (VarTtiAllocInfo::DL,
                                      VarTtiAllocInfo::CTRL,
                                      std::make_shared<DciInfoElementTdma> (0, 1, rbgBitmask));
          VarTtiAllocInfo ulCtrlSlot (VarTtiAllocInfo::UL,
                                      VarTtiAllocInfo::CTRL,
                                      std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, rbgBitmask));
          slotAllocInfo.m_varTtiAllocInfo.push_front (dlCtrlSlot);
          slotAllocInfo.m_varTtiAllocInfo.push_back (ulCtrlSlot);
          SetSlotAllocInfo (slotAllocInfo);
        }

      Simulator::Schedule (m_lastSlotStart + m_phyMacConfig->GetSlotPeriod () -
                           Simulator::Now (),
                           &MmWaveUePhy::StartSlot,
                           this,
                           retVal.m_frameNum,
                           retVal.m_subframeNum,
                           retVal.m_slotNum);
    }
  else
    {
      m_varTtiNum++;
      Time nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () *
                             m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci->m_symStart;

      Simulator::Schedule (nextVarTtiStart + m_lastSlotStart - Simulator::Now (), &MmWaveUePhy::StartVarTti, this);
    }

  if (m_receptionEnabled)
    {
      m_receptionEnabled = false;
    }
}

void
MmWaveUePhy::PhyDataPacketReceived (Ptr<Packet> p)
{
  Simulator::ScheduleWithContext (m_netDevice->GetNode ()->GetId (),
                                  MicroSeconds (m_phyMacConfig->GetTbDecodeLatency ()),
                                  &MmWaveUePhySapUser::ReceivePhyPdu,
                                  m_phySapUser,
                                  p);
  // m_phySapUser->ReceivePhyPdu (p);
}

void
MmWaveUePhy::SendDataChannels (Ptr<PacketBurst> pb, std::list<Ptr<MmWaveControlMessage> > ctrlMsg, Time duration, uint8_t slotInd)
{
  if (pb->GetNPackets () > 0)
    {
      LteRadioBearerTag tag;
      if (!pb->GetPackets ().front ()->PeekPacketTag (tag))
        {
          NS_FATAL_ERROR ("No radio bearer tag");
        }
    }

  m_downlinkSpectrumPhy->StartTxDataFrames (pb, ctrlMsg, duration, slotInd);
}

void
MmWaveUePhy::SendCtrlChannels (std::list<Ptr<MmWaveControlMessage> > ctrlMsg, Time prd)
{
  m_downlinkSpectrumPhy->StartTxDlControlFrames (ctrlMsg,prd);
}


Ptr<MmWaveDlCqiMessage>
MmWaveUePhy::CreateDlCqiFeedbackMessage (const SpectrumValue& sinr)
{
  if (!m_amc)
    {
      m_amc = CreateObject <NrAmc> (m_phyMacConfig);
    }
  NS_LOG_FUNCTION (this);
  SpectrumValue newSinr = sinr;
  // CREATE DlCqiLteControlMessage
  Ptr<MmWaveDlCqiMessage> msg = Create<MmWaveDlCqiMessage> ();
  DlCqiInfo dlcqi;

  dlcqi.m_rnti = m_rnti;
  dlcqi.m_cqiType = DlCqiInfo::WB;

  std::vector<int> cqi;

  uint8_t mcs;
  dlcqi.m_wbCqi = m_amc->CreateCqiFeedbackWbTdma (newSinr, m_currTbs, mcs);

  msg->SetDlCqi (dlcqi);
  return msg;
}

void
MmWaveUePhy::GenerateDlCqiReport (const SpectrumValue& sinr)
{
  if (m_ulConfigured && (m_rnti > 0) && m_receptionEnabled)
    {
      if (Simulator::Now () > m_wbCqiLast + m_wbCqiPeriod)
        {
          SpectrumValue newSinr = sinr;
          Ptr<MmWaveDlCqiMessage> msg = CreateDlCqiFeedbackMessage (newSinr);

          if (msg)
            {
              DoSendControlMessage (msg);
            }
          Ptr<MmWaveUeNetDevice> UeRx = DynamicCast<MmWaveUeNetDevice> (GetDevice ());
          m_reportCurrentCellRsrpSinrTrace (UeRx->GetImsi (), newSinr, newSinr);
        }
    }
}

void
MmWaveUePhy::ReceiveLteDlHarqFeedback (DlHarqInfo m)
{
  NS_LOG_FUNCTION (this);
  // generate feedback to eNB and send it through ideal PUCCH
  Ptr<MmWaveDlHarqFeedbackMessage> msg = Create<MmWaveDlHarqFeedbackMessage> ();
  msg->SetDlHarqFeedback (m);
  Simulator::Schedule (MicroSeconds (m_phyMacConfig->GetTbDecodeLatency ()), &MmWaveUePhy::DoSendControlMessage, this, msg);
}

bool
MmWaveUePhy::IsReceptionEnabled ()
{
  return m_receptionEnabled;
}

void
MmWaveUePhy::ResetReception ()
{
  m_receptionEnabled = false;
}

uint16_t
MmWaveUePhy::GetRnti ()
{
  return m_rnti;
}


void
MmWaveUePhy::DoReset ()
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveUePhy::DoStartCellSearch (uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << dlEarfcn);
}

void
MmWaveUePhy::DoSynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << cellId << dlEarfcn);
  DoSynchronizeWithEnb (cellId);
}

void
MmWaveUePhy::DoSetPa (double pa)
{
  NS_LOG_FUNCTION (this << pa);
}

void
MmWaveUePhy::DoSetRsrpFilterCoefficient (uint8_t rsrpFilterCoefficient)
{
  NS_LOG_FUNCTION (this << (uint16_t) (rsrpFilterCoefficient));
}

void
MmWaveUePhy::DoSynchronizeWithEnb (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);
  if (cellId == 0)
    {
      NS_FATAL_ERROR ("Cell ID shall not be zero");
    }

  m_cellId = cellId;
  //TBD how to assign bandwitdh and earfcn
  // we will assign this already in mmwave-helper.cc
  //m_phyMacConfig = config;

  Ptr<SpectrumValue> noisePsd = GetNoisePowerSpectralDensity ();
  m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
  m_downlinkSpectrumPhy->GetSpectrumChannel ()->AddRx (m_downlinkSpectrumPhy);
  m_downlinkSpectrumPhy->SetCellId (m_cellId);
}


void
MmWaveUePhy::SetPhyMacConfig (Ptr<MmWavePhyMacCommon> config)
{
  m_phyMacConfig = config;
}

void
MmWaveUePhy::DoSetDlBandwidth (uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << (uint32_t) dlBandwidth);
}


void
MmWaveUePhy::DoConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth)
{
  NS_LOG_FUNCTION (this << ulEarfcn << ulBandwidth);
  m_ulConfigured = true;
}

void
MmWaveUePhy::DoConfigureReferenceSignalPower (int8_t referenceSignalPower)
{
  NS_LOG_FUNCTION (this << referenceSignalPower);
}

void
MmWaveUePhy::DoSetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  m_rnti = rnti;
}

void
MmWaveUePhy::DoSetTransmissionMode (uint8_t txMode)
{
  NS_LOG_FUNCTION (this << (uint16_t)txMode);
}

void
MmWaveUePhy::DoSetSrsConfigurationIndex (uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << srcCi);
}

void
MmWaveUePhy::SetPhySapUser (MmWaveUePhySapUser* ptr)
{
  m_phySapUser = ptr;
}

void
MmWaveUePhy::SetHarqPhyModule (Ptr<MmWaveHarqPhy> harq)
{
  m_harqPhyModule = harq;
}

}

