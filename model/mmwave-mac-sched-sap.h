/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
#ifndef MMWAVEMACSCHEDSAPPROVIDER_H
#define MMWAVEMACSCHEDSAPPROVIDER_H

#include "mmwave-phy-mac-common.h"
#include "mmwave-control-messages.h"

namespace ns3 {

class SpectrumModel;

class MmWaveMacSchedSapProvider
{
public:
  MmWaveMacSchedSapProvider () = default;
  MmWaveMacSchedSapProvider (const MmWaveMacSchedSapProvider &o) = delete;

  virtual ~MmWaveMacSchedSapProvider () = default;

  /**
   * \brief RLC buffer status.
   */
  struct SchedDlRlcBufferReqParameters
  {
    uint16_t  m_rnti;                                    //!< The RNTI identifying the UE.
    uint8_t   m_logicalChannelIdentity;                  //!< The logical channel ID, range: 0..10
    uint32_t  m_rlcTransmissionQueueSize;                //!< The current size of the new transmission queue in byte.
    uint16_t  m_rlcTransmissionQueueHolDelay;            //!< Head of line delay of new transmissions in ms.
    uint32_t  m_rlcRetransmissionQueueSize;              //!< The current size of the retransmission queue in byte.
    uint16_t  m_rlcRetransmissionHolDelay;               //!< Head of line delay of retransmissions in ms.
    uint16_t  m_rlcStatusPduSize;                        //!< The current size of the pending STATUS message in byte.
  };

  struct SchedDlCqiInfoReqParameters
  {
    SfnSf m_sfnsf;
    std::vector <struct DlCqiInfo> m_cqiList;
  };

  struct SchedUlMacCtrlInfoReqParameters
  {
    SfnSf  m_sfnSf;
    std::vector <struct MacCeElement> m_macCeList;
  };

  struct SchedUlCqiInfoReqParameters
  {
    SfnSf  m_sfnSf;
    uint8_t m_symStart;
    struct UlCqiInfo m_ulCqi;
  };

  /**
   * \brief UL HARQ information to be used when scheduling UL data.
   */
  struct SchedUlTriggerReqParameters
  {
    SfnSf m_snfSf;
    std::vector <struct UlHarqInfo> m_ulHarqInfoList;
    LteNrTddSlotType m_slotType {F}; //!< Indicate the type of slot requested
  };

  /**
   * \brief DL HARQ information to be used when scheduling UL data.
   */
  struct SchedDlTriggerReqParameters
  {
    SfnSf m_snfSf;
    std::vector <struct DlHarqInfo> m_dlHarqInfoList;
    LteNrTddSlotType m_slotType {F}; //!< Indicate the type of slot requested
  };

  /**
   * \brief SR received from MAC, to pass to schedulers
   *
   * Scheduling request information.
   *
   * http://www.eurecom.fr/~kaltenbe/fapi-2.0/structSchedUlSrInfoReqParameters.html
   */
  struct SchedUlSrInfoReqParameters
  {
    SfnSf m_snfSf;                                 //!< SnfSf in which the sr where received
    std::vector<uint16_t> m_srList;                //!< List of RNTI which asked for a SR
  };

  /**
   * Parameters of the SCHED_DL_RACH_INFO_REQ primitive.
   * See section 4.2.5 for a detailed description of the parameters.
   */
  struct SchedDlRachInfoReqParameters
  {
    uint16_t  m_sfnSf; //!< sfn SF
    std::vector <struct RachListElement_s> m_rachList; //!< RACH list

    std::vector <struct VendorSpecificListElement_s> m_vendorSpecificList; ///< vendor specific list
  };

  virtual void SchedDlRlcBufferReq (const struct SchedDlRlcBufferReqParameters& params) = 0;

  virtual void SchedDlCqiInfoReq (const SchedDlCqiInfoReqParameters& params) = 0;

  /**
   * \brief Starts the DL MAC scheduler for this subframe.
   * \param params      DL HARQ information
   */
  virtual void SchedDlTriggerReq (const struct SchedDlTriggerReqParameters& params) = 0;

  virtual void SchedUlCqiInfoReq (const struct SchedUlCqiInfoReqParameters& params) = 0;

  /**
   * \brief Starts the UL MAC scheduler for this subframe.
   * \param params      UL HARQ information
   */
  virtual void SchedUlTriggerReq (const struct SchedUlTriggerReqParameters& params) = 0;

  /**
   * \brief Provides scheduling request reception information to the scheduler.
   * \param params Scheduling request information.
   */
  virtual void SchedUlSrInfoReq (const SchedUlSrInfoReqParameters &params) = 0;

  virtual void SchedUlMacCtrlInfoReq (const struct SchedUlMacCtrlInfoReqParameters& params) = 0;

  virtual void SchedSetMcs (uint32_t mcs) = 0;

  /**
   * \brief SCHED_DL_RACH_INFO_REQ
   *
   * \param params SchedDlRachInfoReqParameters
   */
  virtual void SchedDlRachInfoReq (const SchedDlRachInfoReqParameters& params) = 0;

private:
};

class MmWaveMacSchedSapUser
{
public:
  virtual ~MmWaveMacSchedSapUser ();

  struct SchedConfigIndParameters
  {
    SchedConfigIndParameters (const SfnSf sfnSf)
      : m_sfnSf (sfnSf),
        m_slotAllocInfo (sfnSf)
    {
    }
    const SfnSf m_sfnSf;
    SlotAllocInfo m_slotAllocInfo;
    std::vector <BuildRarListElement_s> m_buildRarList; ///< build rar list
  };

  virtual void SchedConfigInd (const struct SchedConfigIndParameters& params) = 0;

  virtual Ptr<const SpectrumModel> GetSpectrumModel () const = 0;

  virtual uint32_t GetNumRbPerRbg () const = 0;

  virtual uint8_t GetNumHarqProcess () const = 0;

  virtual uint16_t GetBwpId () const = 0;

  virtual uint16_t GetCellId () const = 0;

  virtual uint32_t GetSymbolsPerSlot () const = 0;

  virtual Time GetSlotPeriod () const = 0;

};

std::ostream & operator<< (std::ostream & os, MmWaveMacSchedSapProvider::SchedDlRlcBufferReqParameters const & p);

} // namespace ns3

#endif /* MMWAVEMACSCHEDSAPPROVIDER_H */