#include "nr-mac-scheduler-ofdma-greedy.h"
#include "ns3/log.h"
#include <algorithm>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaGreedy");
    NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaGreedy);

    TypeId NrMacSchedulerOfdmaGreedy::GetTypeId (void)
    {
        static TypeId tid = TypeId("ns3::NrMacSchedulerOfdmaGreedy").SetParent<NrMacSchedulerOfdma>().AddConstructor<NrMacSchedulerOfdmaGreedy>();
        return tid;
    }

    NrMacSchedulerOfdmaGreedy::NrMacSchedulerOfdmaGreedy():NrMacSchedulerOfdma()
    {
        NS_LOG_FUNCTION(this);
    }
    NrMacSchedulerOfdmaGreedy::~NrMacSchedulerOfdmaGreedy()
    {
        NS_LOG_FUNCTION(this);
    }

    std::shared_ptr<NrMacSchedulerUeInfo>NrMacSchedulerOfdmaGreedy::CreateUeRepresentation(const NrMacCschedSapProvider::CschedUeConfigReqParameters &params) const
    {
        NS_LOG_FUNCTION(this);
        return std::make_shared<NrMacSchedulerUeInfoGreedy>(
            params.m_rnti, params.m_beamConfId,
            std::bind(&NrMacSchedulerOfdmaGreedy::GetNumRbPerRbg, this));
    }

    std::function<bool (const NrMacSchedulerNs3::UePtrAndBufferReq &lhs,const NrMacSchedulerNs3::UePtrAndBufferReq &rhs)>
    NrMacSchedulerOfdmaGreedy::GetUeCompareDlFn () const
    {
        return [] (const NrMacSchedulerNs3::UePtrAndBufferReq &lhs,
                const NrMacSchedulerNs3::UePtrAndBufferReq &rhs) -> bool {
        return std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (lhs.first)->GetAoi () >
            std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (rhs.first)->GetAoi ();};
    }

    std::function<bool (const NrMacSchedulerNs3::UePtrAndBufferReq &lhs,const NrMacSchedulerNs3::UePtrAndBufferReq &rhs)>
    NrMacSchedulerOfdmaGreedy::GetUeCompareUlFn () const
    {
        return NrMacSchedulerUeInfoGreedy::CompareUeWeightsUl;
    }

    void
    NrMacSchedulerOfdmaGreedy::AssignedDlResources (const UePtrAndBufferReq &ue,
                                                    const FTResources &assigned,
                                                    const FTResources &totAssigned) const
    {
    NS_LOG_FUNCTION (this);
    // std::cout << "[aoi-scheduler] : test\n";
    auto ueInfo = std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (ue.first);
    ueInfo->UpdateDlMetric (m_dlAmc);
    }

    void
    NrMacSchedulerOfdmaGreedy::AssignedUlResources (const UePtrAndBufferReq &ue,
                                                    const FTResources &assigned,
                                                    const FTResources &totAssigned) const
    {
    NS_LOG_FUNCTION (this);
    // std::cout << "[ofdma -aoi-greedy]: test\n";
    // Get the UE info object casted to the appropriate type
    auto ueInfo = std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (ue.first);

    // Use the AMC object to update UL metrics
    // Assuming `m_ulAmc` is accessible and correctly initialized in this scheduler
    ueInfo->UpdateUlMetric (m_ulAmc); // Corrected to pass AMC for TB size calculation
    }

    std::function<bool (const std::shared_ptr<NrMacSchedulerUeInfo> &lhs,
                        const std::shared_ptr<NrMacSchedulerUeInfo> &rhs)>
    NrMacSchedulerOfdmaGreedy::GetUePriorityFn () const
    {
        return [] (const std::shared_ptr<NrMacSchedulerUeInfo> &lhs,
                const std::shared_ptr<NrMacSchedulerUeInfo> &rhs) -> bool {
        return std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (lhs)->GetAoi () >
            std::static_pointer_cast<NrMacSchedulerUeInfoGreedy> (rhs)->GetAoi ();
    };
    }

    void
    NrMacSchedulerOfdmaGreedy::PrioritizeUesBasedOnAoi (
        std::vector<std::shared_ptr<NrMacSchedulerUeInfoGreedy>> &ueList) const
    {
    NS_LOG_FUNCTION (this);
    std::sort (ueList.begin (), ueList.end (),
                [] (const std::shared_ptr<NrMacSchedulerUeInfoGreedy> &lhs,
                    const std::shared_ptr<NrMacSchedulerUeInfoGreedy> &rhs) -> bool {
                return lhs->GetAoi () > rhs->GetAoi ();
                });
    }
    void
    NrMacSchedulerOfdmaGreedy::NotAssignedUlResources (const UePtrAndBufferReq &ue,
                                                        const FTResources &notAssigned,
                                                        const FTResources &totalAssigned) const
    {
        NS_LOG_FUNCTION (this);
    }

    void
        NrMacSchedulerOfdmaGreedy::NotAssignedDlResources (const UePtrAndBufferReq &ue,
                                                        const FTResources &notAssigned,
                                                        const FTResources &totalAssigned) const
    {
    NS_LOG_FUNCTION (this);
    }

    void
        NrMacSchedulerOfdmaGreedy::BeforeDlSched (const UePtrAndBufferReq &ue,
                                                const FTResources &assignableInIteration) const
    {
        NS_LOG_FUNCTION (this);
    }

    /**
     * UePtrAndBufferReq에서 UePtr은 NrMacSchedulerUeInfo에서 관리하기 때문에 rnti, AoI, WMA를 가져올 메서드를 구현해야함
     */
    void NrMacSchedulerOfdmaGreedy::BeforeUlSched (const UePtrAndBufferReq &ue, const FTResources &assignableInIteration) const
    {
        NS_LOG_FUNCTION (this);
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoGreedy> (ue.first);
        uint16_t ue_rnti = ue.first->GetRnti ();
        uint32_t ue_aoi = this->GetAge (ue_rnti);
        uint32_t ue_WMA = this->GetWMA(ue_rnti);
        uePtr->UpdateAoi (ue_aoi);
        uePtr->IncrementNiceCount(ue_WMA);
    }
}