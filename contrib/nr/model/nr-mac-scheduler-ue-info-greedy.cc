#include "nr-mac-scheduler-ue-info-greedy.h"
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrMacSchedulerUeInfoGreedy");

void
NrMacSchedulerUeInfoGreedy::UpdateAoi (uint32_t currentAoi)
{
  NS_LOG_FUNCTION (this << currentAoi);
  m_aoi = currentAoi;
}

void
NrMacSchedulerUeInfoGreedy::IncrementNiceCount (uint32_t WMA)
{
  NS_LOG_FUNCTION (this);
  m_niceCount = WMA;
}

uint32_t
NrMacSchedulerUeInfoGreedy::GetAoi () const
{
  NS_LOG_FUNCTION (this);
  return m_aoi;
}

uint32_t
NrMacSchedulerUeInfoGreedy::GetNiceCount () const
{
  NS_LOG_FUNCTION (this);
  return m_niceCount;
}

void
NrMacSchedulerUeInfoGreedy::UpdateMetric (uint32_t currentAoi, uint32_t newNiceCount)
{
  NS_LOG_FUNCTION (this << currentAoi << newNiceCount);
  m_aoi = currentAoi;
  m_niceCount = newNiceCount;
  m_metric = m_aoiWeight * m_aoi + m_niceWeight * m_niceCount;
}

double
NrMacSchedulerUeInfoGreedy::CalculateMetric () const
{
  return m_metric;
}

bool
NrMacSchedulerUeInfoGreedy::CompareUeWeightsDl (const NrMacSchedulerNs3::UePtrAndBufferReq &lue,
                                                   const NrMacSchedulerNs3::UePtrAndBufferReq &rue)
{
  auto luePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoGreedy> (lue.first);
  auto ruePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoGreedy> (rue.first);

  if (!luePtr || !ruePtr)
    {
      NS_ABORT_MSG ("Failed to cast UePtr to NrMacSchedulerUeInfoGreedy");
    }

  double lMetric = luePtr->CalculateMetric ();
  double rMetric = ruePtr->CalculateMetric ();

  // Higher metric gets priority
  return (lMetric > rMetric);
}

bool
NrMacSchedulerUeInfoGreedy::CompareUeWeightsUl (const NrMacSchedulerNs3::UePtrAndBufferReq &lue,
                                                   const NrMacSchedulerNs3::UePtrAndBufferReq &rue)
{
  auto luePtr = dynamic_cast<NrMacSchedulerUeInfoGreedy *> (lue.first.get ());
  auto ruePtr = dynamic_cast<NrMacSchedulerUeInfoGreedy *> (rue.first.get ());

  double lMetric = 0.5 * luePtr->GetAoi () + 0.5 * luePtr->GetNiceCount ();
  double rMetric = 0.5 * ruePtr->GetAoi () + 0.5 * ruePtr->GetNiceCount ();

  // Higher metric gets priority
  return (lMetric > rMetric);
}
} // namespace ns3
