#ifndef EQEMU_HOUSING_OWNERSHIP_TRANSFER_SERVICE_H
#define EQEMU_HOUSING_OWNERSHIP_TRANSFER_SERVICE_H
#include "ownership_transfer_wire.h"
#include "live_item_service.h"
#include "current_estate_access.h"
namespace Housing::OwnershipTransfer {
struct Recipient {AuthenticatedPurchaser actor;uint32_t runtime_entity_id=0;std::string name;};
struct Request {std::string key;LiveItems::Lease lease;Recipient recipient;Route source;std::array<float,3> position{};std::array<uint8_t,EstateWire::PurchaseSize> wire{};};
struct Source {EstateId estate_id=0;uint32_t native_plot_id=0,definition_id=0,reviewed_personal_limit=0;uint64_t catalog_revision=0,acl_revision=0;Route route;RegionWire::Region region;std::string catalog_key,definitions_sha256,regions_sha256,address,contents_sha256;};
struct Receipt {Request request;Source source;uint32_t old_owner=0,new_owner=0;uint64_t ownership_revision=0,acl_revision=0;int64_t retained_copper=0;bool replayed=false;};
// Separate from inventory fence inheritance. Implement using actual Client
// position/current targeted PC, no callback/event/network work under the fence.
class Authority {public:virtual ~Authority()=default;virtual bool TransferFrameMatches(const Request&,const Source&)const noexcept=0;
// Caller owns the transfer transaction and genuine inventory fence. Validate
// the complete current scene; this callback must not mutate or pump events.
virtual bool TransferContentsReviewedUnderTransaction(Database&,const Request&,const Source&,LiveItems::InventorySessionFence&)const{return false;}};
enum class Error {None,Disabled,InvalidArgument,Unauthorized,Unavailable,UnheldFence,StaleLease,PendingApplication,Conflict,NotFound,UnsupportedEscrow,TooManyPlots,DatabaseFailure,CommitUncertain,NotPrimaryOwner,UnreviewedLimit,NoTradeHouseOrCrate,UnsupportedContent};
template<class T>struct Result {T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
class Service {public:explicit Service(std::unique_ptr<Database>,bool enabled=false,uint32_t reviewed_personal_limit=0);// Inspect acquires current authority locks but writes no ownership/ACL/receipt.
Result<Receipt> Inspect(const Request&,LiveItems::InventorySessionFence&,Authority&);Result<Receipt> Transfer(const Request&,LiveItems::InventorySessionFence&,Authority&);Result<Receipt> Resolve(const Request&,LiveItems::InventorySessionFence&);private:std::unique_ptr<Database>db_;bool enabled_;uint32_t reviewed_personal_limit_;std::mutex mutex_;};
}
#endif
