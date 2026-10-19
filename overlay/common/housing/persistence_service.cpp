#include "persistence_service.h"
#include "../database.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <set>
#include <map>
#include <utility>

namespace Housing {
namespace {
struct Failure { Error error; };

MySQLRequestResult Checked(Database& db, const std::string& sql)
{
	// A reconnect/retry inside a transaction could silently lose its earlier work.
	auto result = db.QueryDatabase(sql, false);
	if (!result.Success()) throw Failure{Error::DatabaseFailure};
	return result;
}

class Transaction {
public:
	explicit Transaction(Database& db) : db_(db) { Checked(db_, "START TRANSACTION"); }
	~Transaction() {
		if (active_) {
			try { db_.QueryDatabase(std::string("ROLLBACK"), false); } catch (...) {}
		}
	}
	void Commit() {
		// Any exception after attempting COMMIT can follow a server-side commit.
		// A caller must resolve its durable key rather than refund or replay blindly.
		try {
			auto result = db_.QueryDatabase(std::string("COMMIT"), false);
			if (!result.Success()) throw Failure{Error::CommitUncertain};
			active_ = false;
		} catch (...) { throw Failure{Error::CommitUncertain}; }
	}
private:
	Database& db_;
	bool active_ = true;
};

template<typename T, typename Function> Result<T> Read(Function f)
{
	try { Result<T> result; result.value = f(); return result; }
	catch (const Failure& failure) { return Result<T>::Failure(failure.error); }
	catch (const std::exception&) { return Result<T>::Failure(Error::DatabaseFailure); }
}

template<typename T, typename Function> Result<T> Mutate(Database& db, Function f)
{
	return Read<T>([&]() {
		Transaction transaction(db);
		// All affected tables must participate in rollback, including legacy instances.
		auto engines = Checked(db, "SELECT COUNT(*) FROM information_schema.tables WHERE "
			"table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
			"('housing_instance_allocator','housing_neighborhoods','housing_plot_definitions',"
			"'housing_estates','instance_list')");
		if (engines.RowCount() != 1 || std::stoul(engines.begin()[0]) != 5)
			throw Failure{Error::DatabaseFailure};
		auto value = f();
		transaction.Commit();
		return value;
	});
}

bool ValidKey(const std::string& key)
{
	if (key.empty() || key.size() > 96) return false;
	for (unsigned char c : key) {
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == ':')) return false;
	}
	return true;
}

bool ValidLabel(const std::string& label)
{
	return !label.empty() && label.size() <= 128 && label.find('\0') == std::string::npos;
}

std::string Quote(Database& db, const std::string& s) { return "'" + db.Escape(s) + "'"; }
const std::string neighborhood_columns = "id,request_key,name,zone_id,zone_version,instance_id";
const std::string plot_columns = "id,neighborhood_id,plot_number,label";
const std::string estate_columns = "id,request_key,plot_id,owner_kind,owner_id";

Neighborhood NeighborhoodRow(MySQLRequestRow& row)
{
	return {static_cast<uint32_t>(std::stoul(row[0])), row[1], row[2],
		static_cast<uint32_t>(std::stoul(row[3])), static_cast<uint8_t>(std::stoul(row[4])),
		static_cast<uint16_t>(std::stoul(row[5]))};
}
PlotDefinition PlotRow(MySQLRequestRow& row)
{
	return {static_cast<uint32_t>(std::stoul(row[0])), static_cast<uint32_t>(std::stoul(row[1])),
		static_cast<uint32_t>(std::stoul(row[2])), row[3]};
}
Estate EstateRow(MySQLRequestRow& row)
{
	return {static_cast<uint32_t>(std::stoul(row[0])), row[1],
		static_cast<uint32_t>(std::stoul(row[2])), static_cast<OwnerKind>(std::stoul(row[3])),
		static_cast<uint32_t>(std::stoul(row[4]))};
}

uint16_t AllocatePermanentInstance(Database& db, const Options& options,
	uint32_t zone_id, uint8_t zone_version)
{
	// Serializes housing allocators across world processes; the legacy primary key
	// independently protects against another non-housing allocator racing this one.
	auto lock = Checked(db, "SELECT last_instance_id FROM housing_instance_allocator WHERE id=1 FOR UPDATE");
	if (lock.RowCount() != 1) throw Failure{Error::DatabaseFailure};
	auto used = Checked(db, "SELECT COALESCE(MAX(id),0) FROM instance_list");
	if (used.RowCount() != 1) throw Failure{Error::DatabaseFailure};
	// Never recycle a housing instance identity: expired legacy holes can retain
	// orphaned instance-scoped state. Exhaustion is explicit, not silent reuse.
	const uint32_t last_used = static_cast<uint32_t>(std::stoul(used.begin()[0]));
	const uint32_t high_water = static_cast<uint32_t>(std::stoul(lock.begin()[0]));
	if (last_used >= options.maximum_instance_id || high_water >= options.maximum_instance_id)
		throw Failure{Error::InstanceCapacity};
	const uint32_t candidate = std::max({last_used, high_water,
		static_cast<uint32_t>(options.reserved_instance_ids)}) + 1;
	if (candidate > options.maximum_instance_id) throw Failure{Error::InstanceCapacity};
	Checked(db, "INSERT INTO instance_list "
		"(id,zone,version,is_global,start_time,duration,expire_at,never_expires,notes) VALUES (" +
		std::to_string(candidate) + "," + std::to_string(zone_id) + "," +
		std::to_string(zone_version) + ",0,UNIX_TIMESTAMP(),0,0,1,'Housing neighborhood')");
	Checked(db, "UPDATE housing_instance_allocator SET last_instance_id=" +
		std::to_string(candidate) + " WHERE id=1");
	return static_cast<uint16_t>(candidate);
}
}

Result<uint16_t> AllocatePermanentInstanceUnderTransaction(Database& db,const Options& options,uint32_t zone,uint8_t version)
{
    try {
        if(!options.enabled||!zone||zone==UINT32_MAX||options.reserved_instance_ids>=options.maximum_instance_id||options.maximum_instance_id>32000)return Result<uint16_t>::Failure(Error::InvalidArgument);
        auto tx=Checked(db,"SELECT @@in_transaction");
        if(tx.RowCount()!=1||std::stoul(tx.begin()[0])!=1)return Result<uint16_t>::Failure(Error::InvalidArgument);
        auto engines=Checked(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_instance_allocator','instance_list')");
        if(engines.RowCount()!=1||std::stoul(engines.begin()[0])!=2)return Result<uint16_t>::Failure(Error::DatabaseFailure);
        const auto id=AllocatePermanentInstance(db,options,zone,version);
        Checked(db,"UPDATE instance_list SET notes='Housing house interior' WHERE id="+std::to_string(id));
        return {id,Error::None};
    }catch(const Failure& failure){return Result<uint16_t>::Failure(failure.error);}catch(...){return Result<uint16_t>::Failure(Error::DatabaseFailure);}
}
PersistenceService::PersistenceService(std::unique_ptr<Database> database, Options options)
	: database_(std::move(database)), options_(options) {}
PersistenceService::PersistenceService(std::unique_ptr<Database> database, Options options, IsolatedPurchaseFixture)
	: database_(std::move(database)), options_(options), isolated_purchase_fixture_(true) {}
PersistenceService::~PersistenceService() = default;

bool PersistenceService::Ready() const
{
	return database_ && database_->GetStatus() == DBcore::Connected &&
		options_.reserved_instance_ids < options_.maximum_instance_id &&
		options_.maximum_instance_id <= 32000;
}

Result<Neighborhood> PersistenceService::CreateNeighborhood(const std::string& request_key,
	const std::string& name, uint32_t zone_id, uint8_t zone_version)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<Neighborhood>::Failure(Error::Disabled);
	if (!ValidKey(request_key) || !ValidLabel(name) || !zone_id)
		return Result<Neighborhood>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<Neighborhood>::Failure(Error::DatabaseFailure);
	return Mutate<Neighborhood>(*database_, [&]() {
		// Lock before checking the request key, so simultaneous identical requests replay.
		auto lock = Checked(*database_, "SELECT id FROM housing_instance_allocator WHERE id=1 FOR UPDATE");
		if (lock.RowCount() != 1) throw Failure{Error::DatabaseFailure};
		auto existing = Checked(*database_, "SELECT " + neighborhood_columns +
			" FROM housing_neighborhoods WHERE request_key=" + Quote(*database_, request_key) + " FOR UPDATE");
		if (existing.RowCount()) {
			auto value = NeighborhoodRow(existing.begin());
			if (value.name != name || value.zone_id != zone_id || value.zone_version != zone_version)
				throw Failure{Error::Conflict};
			return value;
		}
		const auto instance_id = AllocatePermanentInstance(*database_, options_, zone_id, zone_version);
		auto inserted = Checked(*database_, "INSERT INTO housing_neighborhoods "
			"(request_key,name,zone_id,zone_version,instance_id) VALUES (" + Quote(*database_, request_key) +
			"," + Quote(*database_, name) + "," + std::to_string(zone_id) + "," +
			std::to_string(zone_version) + "," + std::to_string(instance_id) + ")");
		if (!inserted.LastInsertedID()) throw Failure{Error::DatabaseFailure};
		return Neighborhood{inserted.LastInsertedID(), request_key, name, zone_id, zone_version, instance_id};
	});
}

Result<PlotDefinition> PersistenceService::DefinePlot(NeighborhoodId neighborhood_id,
	uint32_t verified_plot_number, const std::string& verified_label)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<PlotDefinition>::Failure(Error::Disabled);
	if (!neighborhood_id || !verified_plot_number || !ValidLabel(verified_label))
		return Result<PlotDefinition>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<PlotDefinition>::Failure(Error::DatabaseFailure);
	return Mutate<PlotDefinition>(*database_, [&]() {
		auto parent = Checked(*database_, "SELECT id FROM housing_neighborhoods WHERE id=" +
			std::to_string(neighborhood_id) + " FOR UPDATE");
		if (!parent.RowCount()) throw Failure{Error::NotFound};
		auto existing = Checked(*database_, "SELECT " + plot_columns + " FROM housing_plot_definitions "
			"WHERE neighborhood_id=" + std::to_string(neighborhood_id) + " AND plot_number=" +
			std::to_string(verified_plot_number) + " FOR UPDATE");
		if (existing.RowCount()) {
			auto value = PlotRow(existing.begin());
			if (value.label != verified_label) throw Failure{Error::Conflict};
			return value;
		}
		auto inserted = Checked(*database_, "INSERT INTO housing_plot_definitions "
			"(neighborhood_id,plot_number,label) VALUES (" + std::to_string(neighborhood_id) +
			"," + std::to_string(verified_plot_number) + "," + Quote(*database_, verified_label) + ")");
		if (!inserted.LastInsertedID()) throw Failure{Error::DatabaseFailure};
		return PlotDefinition{inserted.LastInsertedID(), neighborhood_id, verified_plot_number, verified_label};
	});
}

Result<Estate> PersistenceService::ProvisionEstate(const std::string& request_key, PlotId plot_id,
	OwnerKind owner_kind, uint32_t owner_id)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<Estate>::Failure(Error::Disabled);
	if (!ValidKey(request_key) || !plot_id || !owner_id ||
		(owner_kind != OwnerKind::Character && owner_kind != OwnerKind::Account && owner_kind != OwnerKind::Guild))
		return Result<Estate>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<Estate>::Failure(Error::DatabaseFailure);
	return Mutate<Estate>(*database_, [&]() {
		if (owner_kind == OwnerKind::Guild) {
			// Serialize administrative provisioning with guild dissolution too.
			// Existence must be checked under the same transaction as the insert.
			auto engine = Checked(*database_, "SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='guilds' AND table_type='BASE TABLE'");
			if (engine.RowCount() != 1 || !engine.begin()[0] || std::string(engine.begin()[0]) != "InnoDB")
				throw Failure{Error::DatabaseFailure};
			auto guild = Checked(*database_, "SELECT id FROM guilds WHERE id=" + std::to_string(owner_id) + " FOR UPDATE");
			if (guild.RowCount() != 1) throw Failure{Error::NotFound};
		}
		auto plot = Checked(*database_, "SELECT id FROM housing_plot_definitions WHERE id=" +
			std::to_string(plot_id) + " FOR UPDATE");
		if (!plot.RowCount()) throw Failure{Error::NotFound};
		auto existing = Checked(*database_, "SELECT " + estate_columns +
			" FROM housing_estates WHERE request_key=" + Quote(*database_, request_key) + " FOR UPDATE");
		if (existing.RowCount()) {
			auto value = EstateRow(existing.begin());
			if (value.plot_id != plot_id || value.owner_kind != owner_kind || value.owner_id != owner_id)
				throw Failure{Error::Conflict};
			return value;
		}
		auto occupied = Checked(*database_, "SELECT id FROM housing_current_estates WHERE plot_id=" +
			std::to_string(plot_id) + " FOR UPDATE");
		if (occupied.RowCount()) throw Failure{Error::Conflict};
		auto inserted = Checked(*database_, "INSERT INTO housing_estates (request_key,plot_id,owner_kind,owner_id) VALUES (" +
			Quote(*database_, request_key) + "," + std::to_string(plot_id) + "," +
			std::to_string(static_cast<uint8_t>(owner_kind)) + "," + std::to_string(owner_id) + ")");
		if (!inserted.LastInsertedID()) throw Failure{Error::DatabaseFailure};
		return Estate{inserted.LastInsertedID(), request_key, plot_id, owner_kind, owner_id};
	});
}

namespace {
void VerifyIsolatedPurchaseDatabase(Database& db)
{
	auto name = Checked(db, "SELECT DATABASE()");
	if (name.RowCount() != 1 || !name.begin()[0]) throw Failure{Error::UnsupportedFunding};
	const std::string value = name.begin()[0];
	if (value != "peq_housing_test" && value.rfind("peq_housing_test_", 0) != 0)
		throw Failure{Error::UnsupportedFunding};
}
void VerifyPurchaseTables(Database& db)
{
	auto engines = Checked(db, "SELECT COUNT(*) FROM information_schema.tables WHERE "
		"table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
		"('housing_purchase_offers','housing_purchase_requests','character_data','character_currency','account')");
	if (engines.RowCount() != 1 || std::stoul(engines.begin()[0]) != 5)
		throw Failure{Error::DatabaseFailure};
}
const std::string offer_columns = "id,plot_id,revision,cost_scheme_id,cost_component_id,"
	"currency_type_id,currency_subtype_id,copper_price,funding_source,enabled,fixture_only";
PersonalPlotOffer OfferRow(MySQLRequestRow& row)
{
	return {static_cast<uint32_t>(std::stoul(row[0])), static_cast<uint32_t>(std::stoul(row[1])),
		static_cast<uint32_t>(std::stoul(row[2])), static_cast<uint32_t>(std::stoul(row[3])),
		static_cast<uint32_t>(std::stoul(row[4])), static_cast<uint32_t>(std::stoul(row[5])),
		static_cast<uint32_t>(std::stoul(row[6])), std::stoll(row[7]),
		static_cast<FundingSource>(std::stoul(row[8])), std::stoul(row[9]) != 0, std::stoul(row[10]) != 0};
}
bool IsFixtureFunding(const PersonalPlotOffer& offer)
{
	return offer.fixture_only && offer.funding_source == FundingSource::IsolatedCarriedCopper &&
		offer.currency_type_id == IsolatedCopperCurrencyType &&
		offer.currency_subtype_id == IsolatedCopperCurrencySubtype;
}
bool IsSyntheticPurchaseActorName(const std::string& name)
{
	// Reserved harness identity: PurchaseFixture + twelve timestamp digits + actor digit.
	// This convention and missing login binding identify accounts created by the harness.
	const std::string prefix = "PurchaseFixture";
	if (name.size() != prefix.size() + 13 || name.compare(0, prefix.size(), prefix) != 0) return false;
	return std::all_of(name.begin() + prefix.size(), name.end(), [](unsigned char c) { return c >= '0' && c <= '9'; });
}
void VerifyLockedPlotRoute(Database& db, PlotId plot_id)
{
	auto plot = Checked(db, "SELECT neighborhood_id FROM housing_plot_definitions WHERE id=" +
		std::to_string(plot_id) + " FOR UPDATE");
	if (plot.RowCount() != 1) throw Failure{Error::NotFound};
	auto neighborhood = Checked(db, "SELECT zone_id,zone_version,instance_id FROM housing_neighborhoods WHERE id=" +
		std::string(plot.begin()[0]) + " FOR UPDATE");
	if (neighborhood.RowCount() != 1) throw Failure{Error::InvalidRoute};
	auto n = neighborhood.begin();
	const auto instance_id = std::stoul(n[2]);
	if (!instance_id || instance_id > 32000) throw Failure{Error::InvalidRoute};
	auto instance = Checked(db, "SELECT zone,version,never_expires FROM instance_list WHERE id=" +
		std::to_string(instance_id) + " FOR UPDATE");
	if (instance.RowCount() != 1 || std::stoul(instance.begin()[0]) != std::stoul(n[0]) ||
		std::stoul(instance.begin()[1]) != std::stoul(n[1]) || std::stoul(instance.begin()[2]) != 1)
		throw Failure{Error::InvalidRoute};
}
}

Result<PurchaseReceipt> PersistenceService::PurchasePersonalPlot(const PersonalPlotPurchase&)
{
	std::lock_guard<std::mutex> guard(mutex_);
	return Result<PurchaseReceipt>::Failure(options_.enabled ? Error::UnsupportedFunding : Error::Disabled);
}

Result<PersonalPlotOffer> PersistenceService::ConfigureIsolatedPersonalPlotOffer(const PersonalPlotOffer& offer)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<PersonalPlotOffer>::Failure(Error::Disabled);
	if (!isolated_purchase_fixture_ || !IsFixtureFunding(offer))
		return Result<PersonalPlotOffer>::Failure(Error::UnsupportedFunding);
	if (!offer.plot_id || !offer.revision || !offer.cost_scheme_id || !offer.cost_component_id ||
		offer.copper_price <= 0 || !offer.enabled) return Result<PersonalPlotOffer>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<PersonalPlotOffer>::Failure(Error::DatabaseFailure);
	return Mutate<PersonalPlotOffer>(*database_, [&]() {
		VerifyIsolatedPurchaseDatabase(*database_);
		VerifyPurchaseTables(*database_);
		VerifyLockedPlotRoute(*database_, offer.plot_id);
		auto existing = Checked(*database_, "SELECT " + offer_columns +
			" FROM housing_purchase_offers WHERE plot_id=" + std::to_string(offer.plot_id) + " FOR UPDATE");
		if (existing.RowCount()) {
			auto value = OfferRow(existing.begin());
			if ((offer.id && value.id != offer.id) || value.revision != offer.revision ||
				value.cost_scheme_id != offer.cost_scheme_id || value.cost_component_id != offer.cost_component_id ||
				value.currency_type_id != offer.currency_type_id || value.currency_subtype_id != offer.currency_subtype_id ||
				value.copper_price != offer.copper_price || value.funding_source != offer.funding_source ||
				value.enabled != offer.enabled || value.fixture_only != offer.fixture_only) throw Failure{Error::Conflict};
			return value;
		}
		if (offer.id) throw Failure{Error::NotFound};
		auto inserted = Checked(*database_, "INSERT INTO housing_purchase_offers "
			"(plot_id,revision,cost_scheme_id,cost_component_id,currency_type_id,currency_subtype_id,"
			"copper_price,funding_source,enabled,fixture_only) VALUES (" + std::to_string(offer.plot_id) +
			"," + std::to_string(offer.revision) + "," + std::to_string(offer.cost_scheme_id) + "," +
			std::to_string(offer.cost_component_id) + "," + std::to_string(offer.currency_type_id) + "," +
			std::to_string(offer.currency_subtype_id) + "," + std::to_string(offer.copper_price) + ",1,1,1)");
		if (inserted.RowsAffected() != 1 || !inserted.LastInsertedID()) throw Failure{Error::DatabaseFailure};
		auto value = offer; value.id = inserted.LastInsertedID(); return value;
	});
}

Result<PurchaseReceipt> PersistenceService::PurchaseIsolatedPersonalPlot(const PersonalPlotPurchase& request)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<PurchaseReceipt>::Failure(Error::Disabled);
	if (!isolated_purchase_fixture_) return Result<PurchaseReceipt>::Failure(Error::UnsupportedFunding);
	if (!ValidKey(request.request_key) || !request.plot_id || !request.offer_id || !request.expected_revision ||
		!request.purchaser.character_id || !request.purchaser.account_id)
		return Result<PurchaseReceipt>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<PurchaseReceipt>::Failure(Error::DatabaseFailure);
	return Mutate<PurchaseReceipt>(*database_, [&]() {
		VerifyIsolatedPurchaseDatabase(*database_);
		VerifyPurchaseTables(*database_);
		// The request row serializes identical keys across independent service connections.
		// Pending rows roll back with every unsuccessful purchase; no separate pending commit.
		const auto key = Quote(*database_, request.request_key);
		Checked(*database_, "INSERT INTO housing_purchase_requests "
			"(request_key,character_id,account_id,plot_id,offer_id,offer_revision) VALUES (" + key + "," +
			std::to_string(request.purchaser.character_id) + "," + std::to_string(request.purchaser.account_id) + "," +
			std::to_string(request.plot_id) + "," + std::to_string(request.offer_id) + "," +
			std::to_string(request.expected_revision) + ") ON DUPLICATE KEY UPDATE request_key=request_key");
		auto records = Checked(*database_, "SELECT character_id,account_id,plot_id,offer_id,offer_revision,"
			"estate_id,charged_copper,remaining_carried_copper,completed FROM housing_purchase_requests "
			"WHERE request_key=" + key + " FOR UPDATE");
		if (records.RowCount() != 1) throw Failure{Error::DatabaseFailure};
		auto record = records.begin();
		if (std::stoul(record[0]) != request.purchaser.character_id || std::stoul(record[1]) != request.purchaser.account_id)
			throw Failure{Error::Unauthorized};
		if (std::stoul(record[2]) != request.plot_id || std::stoul(record[3]) != request.offer_id ||
			std::stoul(record[4]) != request.expected_revision) throw Failure{Error::Conflict};
		auto character = Checked(*database_, "SELECT account_id,ingame,deleted_at,name,gm FROM character_data WHERE id=" +
			std::to_string(request.purchaser.character_id) + " FOR UPDATE");
		if (character.RowCount() != 1 || std::stoul(character.begin()[0]) != request.purchaser.account_id ||
			character.begin()[2]) throw Failure{Error::Unauthorized};
		auto account = Checked(*database_, "SELECT name,status,lsaccount_id FROM account WHERE id=" +
			std::to_string(request.purchaser.account_id) + " FOR UPDATE");
		// ingame is NOT an authoritative presence flag. Only unloginable, synthetic
		// harness accounts/characters may use this adapter; ordinary clone actors fail closed.
		if (account.RowCount() != 1 || !IsSyntheticPurchaseActorName(character.begin()[3]) ||
			std::string(character.begin()[3]) != account.begin()[0] || std::stoul(character.begin()[4]) != 0 ||
			std::stoul(account.begin()[1]) != 0 || account.begin()[2]) throw Failure{Error::Unauthorized};
		// Extra heuristic only; harness lifecycle must keep synthetic actors out of runtime.
		if (std::stoul(character.begin()[1]) != 0) throw Failure{Error::UnsupportedFunding};
		if (std::stoul(record[8]) == 1) {
			auto estates = Checked(*database_, "SELECT " + estate_columns + " FROM housing_estates WHERE id=" +
				std::string(record[5]) + " FOR UPDATE");
			if (estates.RowCount() != 1) throw Failure{Error::Conflict};
			auto estate = EstateRow(estates.begin());
			if (estate.request_key != request.request_key || estate.plot_id != request.plot_id ||
				estate.owner_kind != OwnerKind::Character || estate.owner_id != request.purchaser.character_id)
				throw Failure{Error::Conflict};
			return PurchaseReceipt{estate, request.offer_id, request.expected_revision,
				std::stoll(record[6]), std::stoll(record[7]), true};
		}
		VerifyLockedPlotRoute(*database_, request.plot_id);
		auto offers = Checked(*database_, "SELECT " + offer_columns + " FROM housing_purchase_offers WHERE id=" +
			std::to_string(request.offer_id) + " FOR UPDATE");
		if (offers.RowCount() != 1) throw Failure{Error::NotFound};
		auto offer = OfferRow(offers.begin());
		if (offer.plot_id != request.plot_id || offer.revision != request.expected_revision || !offer.enabled)
			throw Failure{Error::StaleOffer};
		if (!IsFixtureFunding(offer)) throw Failure{Error::UnsupportedFunding};
		if (offer.copper_price <= 0 || !offer.cost_scheme_id || !offer.cost_component_id)
			throw Failure{Error::InvalidArgument};
		auto occupied = Checked(*database_, "SELECT id FROM housing_current_estates WHERE plot_id=" +
			std::to_string(request.plot_id) + " OR request_key=" + key + " FOR UPDATE");
		if (occupied.RowCount()) throw Failure{Error::Conflict};
		auto currency = Checked(*database_, "SELECT platinum,gold,silver,copper FROM character_currency WHERE id=" +
			std::to_string(request.purchaser.character_id) + " FOR UPDATE");
		if (currency.RowCount() != 1) throw Failure{Error::UnsupportedFunding};
		auto money = currency.begin();
		int64_t total = 0; const int64_t multipliers[] = {1000, 100, 10, 1};
		for (int i = 0; i < 4; ++i) {
			const auto denomination = std::stoull(money[i]);
			if (denomination > std::numeric_limits<uint32_t>::max()) throw Failure{Error::UnsupportedFunding};
			total += static_cast<int64_t>(denomination) * multipliers[i];
		}
		if (total < offer.copper_price) throw Failure{Error::InsufficientFunds};
		const auto remaining = total - offer.copper_price;
		if (remaining / 1000 > std::numeric_limits<uint32_t>::max()) throw Failure{Error::UnsupportedFunding};
		auto charged = Checked(*database_, "UPDATE character_currency SET platinum=" + std::to_string(remaining / 1000) +
			",gold=" + std::to_string(remaining / 100 % 10) + ",silver=" + std::to_string(remaining / 10 % 10) +
			",copper=" + std::to_string(remaining % 10) + " WHERE id=" + std::to_string(request.purchaser.character_id));
		if (charged.RowsAffected() != 1) throw Failure{Error::DatabaseFailure};
		auto inserted = Checked(*database_, "INSERT INTO housing_estates (request_key,plot_id,owner_kind,owner_id) VALUES (" +
			key + "," + std::to_string(request.plot_id) + ",1," + std::to_string(request.purchaser.character_id) + ")");
		if (inserted.RowsAffected() != 1 || !inserted.LastInsertedID()) throw Failure{Error::DatabaseFailure};
		Estate estate{inserted.LastInsertedID(), request.request_key, request.plot_id, OwnerKind::Character,
			request.purchaser.character_id};
		auto completed = Checked(*database_, "UPDATE housing_purchase_requests SET estate_id=" + std::to_string(estate.id) +
			",charged_copper=" + std::to_string(offer.copper_price) + ",remaining_carried_copper=" +
			std::to_string(remaining) + ",completed=1 WHERE request_key=" + key + " AND completed=0");
		if (completed.RowsAffected() != 1) throw Failure{Error::DatabaseFailure};
		return PurchaseReceipt{estate, offer.id, offer.revision, offer.copper_price, remaining, false};
	});
}

Result<std::vector<Neighborhood>> PersistenceService::ListNeighborhoods()
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<std::vector<Neighborhood>>::Failure(Error::Disabled);
	if (!Ready()) return Result<std::vector<Neighborhood>>::Failure(Error::DatabaseFailure);
	return Read<std::vector<Neighborhood>>([&]() {
		auto rows = Checked(*database_, "SELECT " + neighborhood_columns + " FROM housing_neighborhoods ORDER BY id");
		std::vector<Neighborhood> values;
		for (auto row = rows.begin(); row.operator!=(rows.end()); ++row) values.push_back(NeighborhoodRow(row));
		return values;
	});
}

Result<Neighborhood> PersistenceService::GetNeighborhood(NeighborhoodId id)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<Neighborhood>::Failure(Error::Disabled);
	if (!id) return Result<Neighborhood>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<Neighborhood>::Failure(Error::DatabaseFailure);
	return Read<Neighborhood>([&]() {
		auto rows = Checked(*database_, "SELECT " + neighborhood_columns + " FROM housing_neighborhoods WHERE id=" + std::to_string(id));
		if (!rows.RowCount()) throw Failure{Error::NotFound};
		return NeighborhoodRow(rows.begin());
	});
}

Result<PlotDefinition> PersistenceService::GetPlot(PlotId id)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<PlotDefinition>::Failure(Error::Disabled);
	if (!id) return Result<PlotDefinition>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<PlotDefinition>::Failure(Error::DatabaseFailure);
	return Read<PlotDefinition>([&]() {
		auto rows = Checked(*database_, "SELECT " + plot_columns + " FROM housing_plot_definitions WHERE id=" + std::to_string(id));
		if (!rows.RowCount()) throw Failure{Error::NotFound};
		return PlotRow(rows.begin());
	});
}

Result<Estate> PersistenceService::GetEstate(EstateId id)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<Estate>::Failure(Error::Disabled);
	if (!id) return Result<Estate>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<Estate>::Failure(Error::DatabaseFailure);
	return Read<Estate>([&]() {
		auto rows = Checked(*database_, "SELECT " + estate_columns + " FROM housing_estates WHERE id=" + std::to_string(id));
		if (!rows.RowCount()) throw Failure{Error::NotFound};
		return EstateRow(rows.begin());
	});
}

Result<Route> PersistenceService::ResolveNeighborhoodRoute(NeighborhoodId id)
{
	std::lock_guard<std::mutex> guard(mutex_);
	if (!options_.enabled) return Result<Route>::Failure(Error::Disabled);
	if (!id) return Result<Route>::Failure(Error::InvalidArgument);
	if (!Ready()) return Result<Route>::Failure(Error::DatabaseFailure);
	return Read<Route>([&]() {
		auto rows = Checked(*database_, "SELECT n.zone_id,n.zone_version,n.instance_id,i.zone,i.version,i.never_expires "
			"FROM housing_neighborhoods n LEFT JOIN instance_list i ON i.id=n.instance_id WHERE n.id=" + std::to_string(id));
		if (!rows.RowCount()) throw Failure{Error::NotFound};
		auto row = rows.begin();
		if (!row[3] || !row[4] || !row[5]) throw Failure{Error::InvalidRoute};
		const auto zone_id = std::stoul(row[0]);
		const auto version = std::stoul(row[1]);
		const auto instance = std::stoul(row[2]);
		if (!instance || instance > 32000 || zone_id != std::stoul(row[3]) ||
			version != std::stoul(row[4]) || std::stoul(row[5]) != 1) throw Failure{Error::InvalidRoute};
		return Route{static_cast<uint32_t>(zone_id), static_cast<uint8_t>(version), static_cast<uint16_t>(instance)};
	});
}

namespace {
void VerifyAclTables(Database& db)
{
    auto rows = Checked(db, "SELECT COUNT(*) FROM information_schema.tables WHERE "
        "table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
        "('housing_estates','housing_acl_state','housing_acl_groups','housing_acl_members',"
        "'character_data','account')");
    if (rows.RowCount() != 1 || std::stoul(rows.begin()[0]) != 6)
        throw Failure{Error::DatabaseFailure};
}
uint32_t AuthenticatePersonalAclActor(Database& db, EstateId estate_id, AuthenticatedPurchaser actor)
{
    // Estate lock serializes initial revision0 writes and ownership changes too.
    auto estates = Checked(db, "SELECT owner_kind,owner_id FROM housing_current_estates WHERE id=" +
        std::to_string(estate_id) + " FOR UPDATE");
    if (estates.RowCount() != 1) throw Failure{Error::NotFound};
    if (std::stoul(estates.begin()[0]) != static_cast<uint32_t>(OwnerKind::Character))
        throw Failure{Error::Unauthorized};
    const auto owner_id = static_cast<uint32_t>(std::stoul(estates.begin()[1]));
    auto character = Checked(db, "SELECT account_id,deleted_at FROM character_data WHERE id=" +
        std::to_string(actor.character_id) + " FOR UPDATE");
    if (character.RowCount() != 1 || character.begin()[1] ||
        std::stoul(character.begin()[0]) != actor.account_id) throw Failure{Error::Unauthorized};
    auto account = Checked(db, "SELECT id FROM account WHERE id=" +
        std::to_string(actor.account_id) + " FOR UPDATE");
    if (account.RowCount() != 1) throw Failure{Error::Unauthorized};
    return owner_id;
}
PersonalEstateAcl LoadAcl(Database& db, EstateId estate_id, bool include_members = true)
{
    PersonalEstateAcl value; value.estate_id = estate_id;
    auto state = Checked(db, "SELECT revision FROM housing_acl_state WHERE estate_id=" +
        std::to_string(estate_id) + " FOR UPDATE");
    if (!state.RowCount()) return value;
    if (state.RowCount() != 1) throw Failure{Error::DatabaseFailure};
    value.revision = std::stoull(state.begin()[0]);
    if (!value.revision) throw Failure{Error::DatabaseFailure};
    auto groups = Checked(db, "SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id=" +
        std::to_string(estate_id) + " ORDER BY group_id");
    if (groups.RowCount() != 7) throw Failure{Error::DatabaseFailure};
    uint32_t expected = 0;
    for (auto row = groups.begin(); row.operator!=(groups.end()); ++row) {
        const auto group = std::stoul(row[0]); const auto mask = std::stoul(row[1]);
        if (group != expected++ || (mask & ~PersonalAclKnownCapabilities) || (group == 6 && mask))
            throw Failure{Error::DatabaseFailure};
        value.masks[group] = static_cast<uint32_t>(mask);
    }
    auto members = Checked(db, "SELECT m.character_id,m.group_id,c.name,c.deleted_at,c.account_id,a.id "
        "FROM housing_acl_members m LEFT JOIN character_data c ON c.id=m.character_id "
        "LEFT JOIN account a ON a.id=c.account_id WHERE m.estate_id=" +
        std::to_string(estate_id) + " ORDER BY m.character_id");
    for (auto row = members.begin(); row.operator!=(members.end()); ++row) {
        if (std::stoul(row[1]) > 5 ||
            (include_members && (!row[2] || row[3] || !row[4] || !row[5])))
            throw Failure{Error::DatabaseFailure};
        value.members.push_back({static_cast<uint32_t>(std::stoul(row[0])),
            static_cast<uint8_t>(std::stoul(row[1])), row[2] ? row[2] : ""});
    }
    return value;
}
struct AclAuthority { uint8_t rank; uint32_t capabilities; };
AclAuthority ResolveAclAuthority(const PersonalEstateAcl& current, uint32_t actor_id, uint32_t owner_id)
{
    // Rank and capabilities come only from the locked stored snapshot, never input.
    if (actor_id == owner_id) return {6, PersonalAclKnownCapabilities};
    uint8_t rank = 0;
    for (const auto& member : current.members)
        if (member.character_id == actor_id) { rank = member.group_id; break; }
    const auto capabilities = current.masks[rank];
    if (!(capabilities & (1u << 12))) throw Failure{Error::Unauthorized};
    return {rank, capabilities};
}
void AuthorizeAclDiff(const PersonalEstateAcl& current, const PersonalAclReplacement& proposed,
    uint32_t owner_id, AclAuthority actor)
{
    if (proposed.masks[6]) throw Failure{Error::InvalidArgument};
    for (uint8_t group = 0; group < 6; ++group) {
        const uint32_t changed_bits = current.masks[group] ^ proposed.masks[group];
        if (changed_bits && actor.rank != 6 &&
            (group >= actor.rank || (changed_bits & ~actor.capabilities)))
            throw Failure{Error::Unauthorized};
    }
    std::map<uint32_t, uint8_t> old_members, new_members;
    for (const auto& member : current.members) old_members.emplace(member.character_id, member.group_id);
    for (const auto& member : proposed.members) {
        if (member.character_id == owner_id) throw Failure{Error::InvalidArgument};
        new_members.emplace(member.character_id, member.group_id);
    }
    for (const auto& member : new_members) {
        auto old = old_members.find(member.first);
        if (old == old_members.end()) {
            if (member.second > actor.rank) throw Failure{Error::Unauthorized};
        } else if (old->second != member.second &&
            (old->second >= actor.rank || member.second > actor.rank)) {
            throw Failure{Error::Unauthorized};
        }
    }
    for (const auto& member : old_members)
        if (!new_members.count(member.first) && member.second >= actor.rank)
            throw Failure{Error::Unauthorized};
}
}

Result<PersonalEstateAcl> PersistenceService::ReadPersonalEstateAcl(EstateId estate_id,
    AuthenticatedPurchaser actor)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!options_.enabled) return Result<PersonalEstateAcl>::Failure(Error::Disabled);
    if (!estate_id || !actor.character_id || !actor.account_id)
        return Result<PersonalEstateAcl>::Failure(Error::InvalidArgument);
    if (!Ready()) return Result<PersonalEstateAcl>::Failure(Error::DatabaseFailure);
    return Mutate<PersonalEstateAcl>(*database_, [&]() {
        VerifyAclTables(*database_);
        const auto owner_id = AuthenticatePersonalAclActor(*database_, estate_id, actor);
        auto current = LoadAcl(*database_, estate_id);
        ResolveAclAuthority(current, actor.character_id, owner_id);
        return current;
    });
}

Result<PersonalEstateAcl> PersistenceService::ReplacePersonalEstateAcl(const PersonalAclReplacement& input)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!options_.enabled) return Result<PersonalEstateAcl>::Failure(Error::Disabled);
    if (!input.estate_id || !input.actor.character_id || !input.actor.account_id ||
        input.expected_revision == std::numeric_limits<uint64_t>::max() || input.members.size() > 4096)
        return Result<PersonalEstateAcl>::Failure(Error::InvalidArgument);
    if (input.masks[6]) return Result<PersonalEstateAcl>::Failure(Error::InvalidArgument);
    for (auto mask : input.masks) if (mask & ~PersonalAclKnownCapabilities)
        return Result<PersonalEstateAcl>::Failure(Error::InvalidArgument);
    std::set<uint32_t> unique;
    for (const auto& member : input.members) {
        if (!member.character_id || member.group_id > 5 ||
            !unique.insert(member.character_id).second)
            return Result<PersonalEstateAcl>::Failure(Error::InvalidArgument);
    }
    if (!Ready()) return Result<PersonalEstateAcl>::Failure(Error::DatabaseFailure);
    return Mutate<PersonalEstateAcl>(*database_, [&]() {
        VerifyAclTables(*database_);
        const auto owner_id = AuthenticatePersonalAclActor(*database_, input.estate_id, input.actor);
        const auto current = LoadAcl(*database_, input.estate_id, false);
        const auto authority = ResolveAclAuthority(current, input.actor.character_id, owner_id);
        if (current.revision != input.expected_revision) throw Failure{Error::Conflict};
        // Validate and lock targets in stable ID order before changing existing state.
        for (auto id : unique) {
            auto target = Checked(*database_, "SELECT c.deleted_at,a.id FROM character_data c "
                "LEFT JOIN account a ON a.id=c.account_id WHERE c.id=" + std::to_string(id) + " FOR UPDATE");
            if (target.RowCount() != 1 || target.begin()[0] || !target.begin()[1])
                throw Failure{Error::InvalidArgument};
        }
        AuthorizeAclDiff(current, input, owner_id, authority);
        const auto estate = std::to_string(input.estate_id);
        if (!current.revision) {
            Checked(*database_, "INSERT INTO housing_acl_state (estate_id,revision) VALUES (" + estate + ",1)");
        } else {
            auto changed = Checked(*database_, "UPDATE housing_acl_state SET revision=revision+1 WHERE estate_id=" +
                estate + " AND revision=" + std::to_string(input.expected_revision));
            if (changed.RowsAffected() != 1) throw Failure{Error::Conflict};
        }
        Checked(*database_, "DELETE FROM housing_acl_groups WHERE estate_id=" + estate);
        Checked(*database_, "DELETE FROM housing_acl_members WHERE estate_id=" + estate);
        for (uint32_t i = 0; i < 7; ++i)
            Checked(*database_, "INSERT INTO housing_acl_groups (estate_id,group_id,capabilities) VALUES (" +
                estate + "," + std::to_string(i) + "," + std::to_string(input.masks[i]) + ")");
        for (const auto& member : input.members)
            Checked(*database_, "INSERT INTO housing_acl_members (estate_id,character_id,group_id) VALUES (" +
                estate + "," + std::to_string(member.character_id) + "," + std::to_string(member.group_id) + ")");
        return LoadAcl(*database_, input.estate_id);
    });
}
}
