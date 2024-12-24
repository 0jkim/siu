#ifndef NR_MAC_SCHEDULER_UE_INFO_GREEDY_H
#define NR_MAC_SCHEDULER_UE_INFO_GREEDY_H

#include "nr-mac-scheduler-ue-info.h"
#include "nr-mac-scheduler-ns3.h"
#include <memory>

namespace ns3 {

class NrMacSchedulerUeInfoGreedy : public NrMacSchedulerUeInfo
{
public:
  /**
   * \brief NrMacSchedulerUeInfoAoiGreedy constructor
   * \param rnti RNTI of the UE
   * \param beamConfId BeamConfId of the UE
   * \param fn A function that tells how many RB per RBG
   * \param aoiWeight Weight for AoI in the metric calculation
   * \param niceWeight Weight for Nice Count in the metric calculation
   */
  NrMacSchedulerUeInfoGreedy (uint16_t rnti, BeamConfId beamConfId, const GetRbPerRbgFn &fn)
      : NrMacSchedulerUeInfo (rnti, beamConfId, fn),
        m_aoi (1),
        m_niceCount (1),
        m_aoiWeight (0.5),
        m_niceWeight (0.5),
        m_metric (0.0)
  {
  }

  /**
   * \brief Update the AoI value for the UE
   * \param currentAoi The current AoI value
   */
  void UpdateAoi (uint32_t currentAoi);

  /**
   * \brief Increment the Nice Count for the UE
   */
  void IncrementNiceCount (uint32_t WMA);

  /**
   * \brief Get the current AoI value
   * \return The current AoI value
   */
  uint32_t GetAoi () const;

  /**
   * \brief Get the current Nice Count value
   * \return The current Nice Count value
   */
  uint32_t GetNiceCount () const;

  /**
   * \brief Update the metric used for scheduling
   * \param currentAoi The current AoI value
   * \param newNiceCount The new Nice Count value
   */
  void UpdateMetric (uint32_t currentAoi, uint32_t newNiceCount);

  /**
   * \brief Calculate the metric for scheduling
   * \return The calculated metric value
   */
  double CalculateMetric () const;

  static bool CompareUeWeightsDl (const NrMacSchedulerNs3::UePtrAndBufferReq &lue,
                                  const NrMacSchedulerNs3::UePtrAndBufferReq &rue);
  static bool CompareUeWeightsUl (const NrMacSchedulerNs3::UePtrAndBufferReq &lue,
                                  const NrMacSchedulerNs3::UePtrAndBufferReq &rue);

  uint32_t m_aoi; //!< Age of Information value
  uint32_t m_niceCount; //!< Nice Count value
  float m_aoiWeight = 0.5; //!< Weight for AoI in the metric calculation
  float m_niceWeight = 0.5; //!< Weight for Nice Count in the metric calculation
  double m_metric; //!< Cached metric value for scheduling
};

} // namespace ns3

#endif /* NR_MAC_SCHEDULER_UE_INFO_GREEDY_H */
