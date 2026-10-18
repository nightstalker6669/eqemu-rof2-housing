#ifndef EQEMU_HOUSING_UPKEEP_CLOCK_H
#define EQEMU_HOUSING_UPKEEP_CLOCK_H
#include <cstdint>
#include <limits>
#include <optional>
namespace Housing::UpkeepClock {
// Explicit compatibility policy: distribute indivisible copper using the UTC
// hour phase. Every 24 consecutive assessments sum to the published daily price.
// This rounding phase is not asserted to be recovered original server behavior.
// Sub-copper hourly cohorts remain unsupported by the existing delinquency
// ledger: a zero charge must not silently clear an earlier expiration marker.
inline std::optional<uint64_t> Hourly(uint64_t daily,uint32_t due,bool reviewed){
 if(daily>uint64_t(std::numeric_limits<int64_t>::max()))return {};
 if(daily%24==0)return daily/24;
 if(!reviewed||daily<24||!due)return {};
 const uint64_t phase=(due/3600)%24,remainder=daily%24;
 return daily/24+(remainder*(phase+1))/24-(remainder*phase)/24;
}
struct Cursor {uint32_t due=0,estate=0;};
inline bool After(const Cursor& cursor,uint32_t due,uint32_t estate){return due>cursor.due||(due==cursor.due&&estate>cursor.estate);}
}
#endif
