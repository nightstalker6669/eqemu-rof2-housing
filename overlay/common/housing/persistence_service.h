#ifndef EQEMU_HOUSING_PERSISTENCE_SERVICE_H
#define EQEMU_HOUSING_PERSISTENCE_SERVICE_H

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Database;

namespace Housing {
// Domain identifiers are independent of protocol values and emulator instance IDs.
using NeighborhoodId = uint32_t;
using PlotId = uint32_t;
using EstateId = uint32_t;

enum class Error {
	None, Disabled, InvalidArgument, NotFound, Conflict, InstanceCapacity,
	DatabaseFailure, CommitUncertain, InvalidRoute, Unauthorized, InsufficientFunds,
	UnsupportedFunding, StaleOffer
};

template<typename T> struct Result {
	T value{};
	Error error = Error::None;
	explicit operator bool() const { return error == Error::None; }
	static Result Failure(Error e) { Result r; r.error = e; return r; }
};

struct Options {
	bool enabled = false;
	uint16_t reserved_instance_ids = 100; // Supply the current Instances:ReservedInstances rule.
	uint16_t maximum_instance_id = 32000; // Existing emulator allocator ceiling.
};
// Reuses the housing monotonic allocator in a caller-owned checked transaction.
// No commit/rollback; failure requires the parent to roll back all its writes.
Result<uint16_t> AllocatePermanentInstanceUnderTransaction(Database&,const Options&,
    uint32_t zone_id,uint8_t zone_version);

struct Neighborhood {
	NeighborhoodId id = 0;
	std::string request_key;
	std::string name;
	uint32_t zone_id = 0;
	uint8_t zone_version = 0;
	uint16_t instance_id = 0;
};

struct PlotDefinition {
	PlotId id = 0;
	NeighborhoodId neighborhood_id = 0;
	uint32_t plot_number = 0;
	std::string label; // Explicit content identifier; never generated from assumed addresses.
};

// Internal subject types, not guessed May 2013 wire enums or access-control policy.
enum class OwnerKind : uint8_t { Character = 1, Account = 2, Guild = 3 };
struct Estate {
	EstateId id = 0;
	std::string request_key;
	PlotId plot_id = 0;
	OwnerKind owner_kind = OwnerKind::Character;
	uint32_t owner_id = 0;
};

struct Route {
	uint32_t zone_id = 0;
	uint8_t zone_version = 0;
	uint16_t instance_id = 0;
};

// This foundation deliberately has no production funding adapter. The fixture
// adapter is restricted to isolated databases and unloginable synthetic harness
// actors. DB ingame is not an authoritative presence flag; the harness owns lifecycle.
struct IsolatedPurchaseFixture {};
enum class FundingSource : uint8_t { Unsupported = 0, IsolatedCarriedCopper = 1 };
inline constexpr uint32_t IsolatedCopperCurrencyType = 0xfffffff0;
inline constexpr uint32_t IsolatedCopperCurrencySubtype = 0xfffffff0;
struct PersonalPlotOffer {
	uint32_t id = 0;
	PlotId plot_id = 0;
	uint32_t revision = 0;
	uint32_t cost_scheme_id = 0;
	uint32_t cost_component_id = 0;
	uint32_t currency_type_id = 0;
	uint32_t currency_subtype_id = 0;
	int64_t copper_price = 0;
	FundingSource funding_source = FundingSource::Unsupported;
	bool enabled = false;
	bool fixture_only = true;
};
struct AuthenticatedPurchaser {
	uint32_t character_id = 0;
	uint32_t account_id = 0; // Supplied by trusted server session, never raw client fields.
};
struct PersonalPlotPurchase {
	std::string request_key;
	AuthenticatedPurchaser purchaser;
	PlotId plot_id = 0;
	uint32_t offer_id = 0;
	uint32_t expected_revision = 0;
};
struct PurchaseReceipt {
	Estate estate;
	uint32_t offer_id = 0;
	uint32_t offer_revision = 0;
	int64_t charged_copper = 0;
	int64_t remaining_carried_copper = 0; // Historical balance at this commit.
	bool replayed = false;
};

// Exact-build native defaults; capability indices 0..12 only. Owner identity
// comes from the estate row, never a named membership claiming group6.
inline constexpr uint32_t PersonalAclKnownCapabilities = 0x1fff;
inline constexpr std::array<uint32_t, 7> PersonalAclDefaultMasks =
    {0x3, 0x3, 0x803, 0xe03, 0x1f87, 0x1fff, 0};
struct PersonalAclMember {
    uint32_t character_id = 0;
    uint8_t group_id = 0; // 0..5; Owner6 cannot be assigned.
};
struct PersonalAclNamedMember {
    uint32_t character_id = 0;
    uint8_t group_id = 0;
    std::string canonical_name; // Database-derived; input names are never accepted.
};
struct PersonalEstateAcl {
    EstateId estate_id = 0;
    uint64_t revision = 0;
    std::array<uint32_t, 7> masks = PersonalAclDefaultMasks;
    std::vector<PersonalAclNamedMember> members;
};
struct PersonalAclReplacement {
    EstateId estate_id = 0;
    AuthenticatedPurchaser actor; // Server-authenticated session identity.
    uint64_t expected_revision = 0;
    std::array<uint32_t, 7> masks = PersonalAclDefaultMasks;
    std::vector<PersonalAclMember> members;
};

class PersistenceService {
public:
	// Transfer a newly connected, EXCLUSIVE Database connection. Do not use SetMySQL
	// to alias a live connection: its query-level lock cannot isolate a transaction.
	// World owns this service. ACL methods verify ownership against a trusted actor;
	// routing/provisioning callers still authorize their own administrative actions.
	explicit PersistenceService(std::unique_ptr<Database> database, Options options = {});
	PersistenceService(std::unique_ptr<Database> database, Options options, IsolatedPurchaseFixture);
	~PersistenceService();
	PersistenceService(const PersistenceService&) = delete;
	PersistenceService& operator=(const PersistenceService&) = delete;

	// Stable request keys make retries recover the same resource after uncertain commits.
	// Caller must validate zone assets/content before provisioning; no zone is guessed.
	Result<Neighborhood> CreateNeighborhood(const std::string& request_key,
		const std::string& name, uint32_t zone_id, uint8_t zone_version);
	Result<PlotDefinition> DefinePlot(NeighborhoodId neighborhood_id,
		uint32_t verified_plot_number, const std::string& verified_label);
	// Administrative ownership scaffolding, NOT a player purchase/escrow operation.
	// Does not allocate an interior. Interior identity belongs to a house, not a plot.
	Result<Estate> ProvisionEstate(const std::string& request_key, PlotId plot_id,
		OwnerKind owner_kind, uint32_t owner_id);
	// Always fails closed until a production funding/catalog adapter is implemented.
	Result<PurchaseReceipt> PurchasePersonalPlot(const PersonalPlotPurchase& request);
	// Explicit fixture-only operations; do not wire these into native handlers.
	Result<PersonalPlotOffer> ConfigureIsolatedPersonalPlotOffer(const PersonalPlotOffer& offer);
	Result<PurchaseReceipt> PurchaseIsolatedPersonalPlot(const PersonalPlotPurchase& request);

	// Personal estate Manage administration follows exact-build stored group hierarchy.
	// Guild/account-owned estates and privileged bypass are not implemented.
	Result<PersonalEstateAcl> ReadPersonalEstateAcl(EstateId estate_id,
		AuthenticatedPurchaser actor);
	Result<PersonalEstateAcl> ReplacePersonalEstateAcl(const PersonalAclReplacement& replacement);

	Result<std::vector<Neighborhood>> ListNeighborhoods();
	Result<Neighborhood> GetNeighborhood(NeighborhoodId id);
	Result<PlotDefinition> GetPlot(PlotId id);
	// Historical generation lookup, including retired ownership. This is not
	// current ownership/entry/ACL authority; those readers use housing_current_estates.
	Result<Estate> GetEstate(EstateId id);
	// Checks that explicit backing instance exists, remains permanent, and matches metadata.
	// Caller still authorizes entry/membership before using the route.
	Result<Route> ResolveNeighborhoodRoute(NeighborhoodId id);

private:
	bool Ready() const;
	std::unique_ptr<Database> database_;
	Options options_;
	std::mutex mutex_;
	bool isolated_purchase_fixture_ = false;
};
}
#endif
