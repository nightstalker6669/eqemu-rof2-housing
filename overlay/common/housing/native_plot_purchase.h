#ifndef EQEMU_HOUSING_NATIVE_PLOT_PURCHASE_H
#define EQEMU_HOUSING_NATIVE_PLOT_PURCHASE_H
#include "live_escrow_service.h"
#include "estate_wire.h"
#include "catalog_service.h"
#include <optional>
namespace Housing::NativePlotPurchase {
// Server-only reviewed guild domain, never decoded as authority from45be.
struct GuildOwner {uint32_t guild_id=0,leader_character_id=0;uint8_t permission_mask=0;uint32_t reviewed_minimum_members=0;};
struct Price {uint32_t component=0;int64_t copper=0;};
// Authenticated inspection result. No field in this structure is decoded as
// authority from45be. Full catalog hashes fence every displayed component.
struct Offer {uint32_t native_plot_id=0,plot_id=0,neighborhood_id=0,definition_id=0;uint64_t catalog_revision=0,policy_revision=0;std::string catalog_key,definitions_sha256,regions_sha256;Route source;uint32_t max_owned_plots=0;bool carried_copper=false;std::vector<Price> prices;/* Historical field name retained for receipt compatibility: reviewed paid upkeep IDs may also be admitted. */std::vector<uint32_t> zero_upkeep_components;std::optional<GuildOwner> guild;};
struct Request {std::string request_key;LiveItems::Lease lease;Offer offer;std::array<uint8_t,EstateWire::PurchaseSize> wire{};LiveEscrow::Currency expected;};
struct Receipt {Request request;InventorySessions::Lease reservation;EstateId estate_id=0;int64_t charged_copper=0;LiveEscrow::Currency after;bool requires_application=true,replayed=false;};
enum class Error {None,Disabled,InvalidArgument,UnheldFence,Unauthorized,NotFound,StaleLease,PendingApplication,SnapshotMismatch,Conflict,InsufficientFunds,Overflow,DatabaseFailure,CommitUncertain,UnsupportedFunding,Unavailable,TooManyPlots};
template<class T>struct Result {T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
class Fence:public LiveEscrow::CoinSessionFence {public:virtual bool GuildExpansionAllowed()const noexcept{return false;}virtual bool InspectedOfferMatches(const Offer&)const noexcept=0;virtual bool PurchaseApplied(const Receipt&)const noexcept=0;};
std::vector<uint8_t> OfferBytes(const Offer&);
// Fence the exact bootstrap queued with this inspection, not a newer SQL quote.
bool MatchesPublished(const Offer&,const Catalog::Export&);
class Service {public:explicit Service(std::unique_ptr<Database>,bool enabled=false);Result<Offer> Inspect(const LiveItems::Lease&,uint32_t native_plot,Route actual_source,Fence&);Result<Receipt> Buy(const Request&,Fence&);Result<Receipt> Resolve(const Request&,Fence&);Result<bool> AcknowledgeApplied(const Request&,Fence&);
 // Only the existing certified-writer-death reconciler may call this inside
 // its owned TX after exact process/history/custody linkage. Does not COMMIT or
 // claim dead-process memory applied. Returns canonical outcome audit bytes.
 static Result<std::pair<std::string,std::vector<uint8_t>>> ReconcileRestartUnderTransaction(Database&,const InventorySessions::Lease&,const LiveItems::Lease&);
private:std::unique_ptr<Database> db_;bool enabled_;std::mutex mutex_;};
}
#endif
