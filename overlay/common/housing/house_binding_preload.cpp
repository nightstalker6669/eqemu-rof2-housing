#include "runtime_activation_db.h"
#include "house_binding_preload.h"
#include "crate_placement_model.h"
#include "house_inventory_transition.h"
#include "inventory_process_recovery.h"
#include "custody_codec.h"
#include "catalog_service.h"
#include "coin_pending_guard.h"
#include "../database.h"
#include "../inventory_profile.h"
#include "../item_instance.h"
#include <algorithm>
#include <map>
#include <set>
namespace Housing::HousePreload {
    namespace {
        using Row=LiveItems::InventoryRow;
        using Bytes=CustodyCodec::Bytes;
        struct Failure {
            Error error;
        };
        [[noreturn]] void Reject(Error e){
            throw Failure{
                e
            };
        }
        MySQLRequestResult Q(Database& db,const std::string& sql){
            auto r=db.QueryDatabase(sql,false);
            if(!r.Success())Reject(Error::DatabaseFailure);
            return r;
        }
        std::string Hex(const Bytes& bytes) {
            static constexpr char alphabet[] = "0123456789abcdef";
            std::string out;
            out.reserve(bytes.size() * 2);
            for (auto byte : bytes) {
                out += alphabet[byte >> 4];
                out += alphabet[byte & 15];
            }
            return out;
        }
        Bytes Unhex(const char* input) {
            if (!input) Reject(Error::DatabaseFailure);
            const std::string text(input);
            if (text.size() % 2 || text.size() > NeighborhoodWire::MaximumPayload * 2) Reject(Error::DatabaseFailure);
            auto nibble = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                Reject(Error::DatabaseFailure);
            };
            Bytes out;
            for (size_t at = 0; at < text.size(); at += 2)
            out.push_back(uint8_t(nibble(text[at]) * 16 + nibble(text[at + 1])));
            return out;
        }
        std::string Literal(const std::string& text) {
            return "x'" + Hex(Bytes(text.begin(), text.end())) + "'";
        }
        uint64_t Number(const char* value, uint64_t max) {
            if (!value || !*value || *value == '-') Reject(Error::DatabaseFailure);
            size_t end = 0;
            auto out = std::stoull(value, &end);
            if (end != std::string(value).size() || out > max) Reject(Error::DatabaseFailure);
            return out;
        }
        const std::string InventoryColumns = "character_id,slot_id,item_id,charges,color,augment_one,augment_two,"
        "augment_three,augment_four,augment_five,augment_six,instnodrop,HEX(custom_data),ornament_icon,"
        "ornament_idfile,ornament_hero_model,guid";
        Row InventoryRow(MySQLRequestRow row) {
            Row out;
            auto number = [&](size_t column, uint64_t max) {
                return row[column] ? Number(row[column], max) : 0;
            };
            auto& v = out.values;
            v.character_id = number(0, UINT32_MAX);
            v.slot_id = number(1, UINT32_MAX);
            v.item_id = number(2, UINT32_MAX);
            v.charges = number(3, UINT16_MAX);
            v.color = number(4, UINT32_MAX);
            v.augment_one = number(5, UINT32_MAX);
            v.augment_two = number(6, UINT32_MAX);
            v.augment_three = number(7, UINT32_MAX);
            v.augment_four = number(8, UINT32_MAX);
            v.augment_five = number(9, UINT32_MAX);
            v.augment_six = number(10, UINT32_MAX);
            v.instnodrop = number(11, UINT8_MAX);
            if (row[12]) {
                auto text = Unhex(row[12]);
                v.custom_data.assign(text.begin(), text.end());
            }
            v.ornament_icon = number(13, UINT32_MAX);
            v.ornament_idfile = number(14, UINT32_MAX);
            if (!row[15]) Reject(Error::DatabaseFailure);
            size_t end = 0;
            auto hero = std::stoll(row[15], &end);
            if (end != std::string(row[15]).size() || hero < INT32_MIN || hero > INT32_MAX) Reject(Error::DatabaseFailure);
            v.ornament_hero_model = int32_t(hero);
            v.guid = number(16, UINT64_MAX);
            if (!row[2]) out.null_fields |= LiveItems::NullItemId;
            if (!row[3]) out.null_fields |= LiveItems::NullCharges;
            if (!row[12]) out.null_fields |= LiveItems::NullCustomData;
            if (!row[16]) out.null_fields |= LiveItems::NullGuid;
            // Non-null fields must never silently become zero on malformed legacy rows.
            for (auto i : {
                0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14
            }) if (!row[i]) Reject(Error::DatabaseFailure);
            if (!CustodyCodec::Valid(out)) Reject(Error::UnsupportedItem);
            return out;
        }
        std::string InventoryWhere(const Row& row) {
            return "character_id=" + std::to_string(row.values.character_id) + " AND slot_id=" + std::to_string(row.values.slot_id);
        }
        void InventoryExact(Database& db, const Row& expected, bool absent = false) {
            auto result = Q(db, "SELECT " + InventoryColumns + " FROM inventory WHERE " + InventoryWhere(expected) + " FOR UPDATE");
            if (absent) {
                if (result.RowCount()) Reject(Error::Conflict);
                return;
            }
            if (result.RowCount() != 1 || CustodyCodec::Encode(InventoryRow(result.begin())) != CustodyCodec::Encode(expected))
            Reject(Error::Conflict);
        }
        bool Key(const std::string& s){
            return !s.empty()&&s.size()<=96;
        }
        bool Slot(uint32_t slot){
            return slot>=23&&slot<=33;
        }
        class Tx {
            Database& db_;
            bool open_=true;
            public:explicit Tx(Database& d):db_(d){
                Q(d,"START TRANSACTION");
            }~Tx(){
                if(open_)try{
                    db_.QueryDatabase(std::string("ROLLBACK"),false);
                }catch(...){
                }
            }void Commit(){
                try{
                    if(!db_.QueryDatabase(std::string("COMMIT"),false).Success())Reject(Error::CommitUncertain);
                    open_=false;
                }catch(...){
                    Reject(Error::CommitUncertain);
                }
            }
        };
        void Schema(Database& db){
            auto r=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_sessions','housing_inventory_session_history','account','character_data','inventory','items','items_evolving_details','character_evolving_items','housing_house_instances','housing_house_locations','housing_house_interiors','housing_house_receipt_effects','housing_live_item_leases','housing_live_item_receipts','housing_catalogs','housing_native_estates','instance_list')");
            if(Number(r.begin()[0],17)!=17)Reject(Error::DatabaseFailure);
            auto columns=Q(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='inventory' ORDER BY ordinal_position");
            auto names=InventoryRepository::Columns();
            if(columns.RowCount()!=names.size())Reject(Error::UnsupportedItem);
            size_t n=0;
            for(auto row=columns.begin();row.operator!=(columns.end());++row)if(names[n++]!=row[0])Reject(Error::UnsupportedItem);
        }
        std::string Predicate(const InventorySessions::Lease& lease){
            return "character_id="+std::to_string(lease.actor.character_id)+" AND account_id="+std::to_string(lease.actor.account_id)+" AND epoch="+std::to_string(lease.epoch)+" AND token=x'"+Hex(Bytes(lease.token.begin(),lease.token.end()))+"' AND process_key="+Literal(lease.owner.process_key)+" AND client_key="+Literal(lease.owner.client_key)+" AND zone_id="+std::to_string(lease.owner.zone_id)+" AND instance_id="+std::to_string(lease.owner.instance_id);
        }
        void Authority(Database& db,const InventorySessions::Lease& lease,SessionFence& fence){
            if(!lease.active||!lease.actor.character_id||!lease.actor.account_id||!lease.epoch||!lease.owner.zone_id||!Key(lease.owner.process_key)||!Key(lease.owner.client_key))Reject(Error::InvalidArgument);
            if(!fence.HeldFor(lease))Reject(Error::UnheldReservation);
            const auto character=std::to_string(lease.actor.character_id);
            auto actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+character+" FOR UPDATE");
            if(actor.RowCount()!=1||Number(actor.begin()[0],UINT32_MAX)!=lease.actor.account_id||actor.begin()[1])Reject(Error::Unauthorized);
            auto account=Q(db,"SELECT id FROM account WHERE id="+std::to_string(lease.actor.account_id)+" FOR UPDATE");
            if(account.RowCount()!=1)Reject(Error::Unauthorized);
            auto optional=Q(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations')");
            auto count=Number(optional.begin()[0],3);
            if(count&&(count!=3||Number(optional.begin()[1],3)!=3))Reject(Error::DatabaseFailure);
            bool registered=false;
            if(count){
                auto process=Q(db,"SELECT reaped FROM housing_inventory_processes WHERE process_key="+Literal(lease.owner.process_key)+" FOR UPDATE");
                if(process.RowCount()>1)Reject(Error::DatabaseFailure);
                if(process.RowCount()){
                    registered=true;
                    if(Number(process.begin()[0],1))Reject(Error::UnheldReservation);
                }
            }
            auto owner=Q(db,"SELECT active FROM housing_inventory_sessions WHERE "+Predicate(lease)+" FOR UPDATE");
            auto history=Q(db,"SELECT released FROM housing_inventory_session_history WHERE "+Predicate(lease)+" FOR UPDATE");
            if(owner.RowCount()!=1||history.RowCount()!=1||Number(owner.begin()[0],1)!=1||Number(history.begin()[0],1)!=0)Reject(Error::UnheldReservation);
            const auto coin=CoinPending::Inspect(db,lease.actor.character_id,true);
            if(coin!=CoinPending::State::Clear)Reject(coin==CoinPending::State::Pending?Error::PendingApplication:Error::DatabaseFailure);
            const auto ordinary_pending=HouseInventory::InspectPending(db,lease.actor.character_id,true);
            if(ordinary_pending!=HouseInventory::PendingState::Clear)
                Reject(ordinary_pending==HouseInventory::PendingState::Pending?Error::PendingApplication:Error::DatabaseFailure);
            auto custody=Q(db,"SELECT account_id,epoch,HEX(session_key),HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+character+" FOR UPDATE");
            if(custody.RowCount()>1)Reject(Error::DatabaseFailure);
            if(custody.RowCount()){
                auto c=custody.begin();
                if(c[5])Reject(Error::PendingApplication);
                if(Number(c[4],1)){
                    if(!count||!registered||Number(c[0],UINT32_MAX)!=lease.actor.account_id)Reject(Error::UnheldReservation);
                    auto link=Q(db,"SELECT inventory_epoch,HEX(inventory_token),HEX(process_key),HEX(client_key),account_id,HEX(custody_session),HEX(custody_token) FROM housing_inventory_custody_links WHERE character_id="+character+" AND custody_epoch="+std::string(c[1])+" FOR UPDATE");
                    if(link.RowCount()!=1)Reject(Error::UnheldReservation);
                    auto l=link.begin();
                    if(Number(l[0],UINT64_MAX)!=lease.epoch||Unhex(l[1])!=Bytes(lease.token.begin(),lease.token.end())||Unhex(l[2])!=Bytes(lease.owner.process_key.begin(),lease.owner.process_key.end())||Unhex(l[3])!=Bytes(lease.owner.client_key.begin(),lease.owner.client_key.end())||Number(l[4],UINT32_MAX)!=lease.actor.account_id||Unhex(l[5])!=Unhex(c[2])||Unhex(l[6])!=Unhex(c[3]))Reject(Error::UnheldReservation);
                }
            }
            auto receipts=Q(db,"SELECT request_key FROM housing_live_item_receipts WHERE character_id="+character+" AND applied=0 FOR UPDATE");
            for(auto row=receipts.begin();row.operator!=(receipts.end());++row){
                if(!row[0]||!InventoryRecovery::ReceiptDeathReconciled(db,lease.actor.character_id,row[0]))Reject(Error::PendingApplication);
            }
            auto pending=Q(db,"SELECT h.house_id FROM housing_house_instances h LEFT JOIN housing_house_locations l ON l.house_id=h.house_id LEFT JOIN housing_live_item_receipts r ON r.request_key=h.pending_request_key WHERE h.pending_request_key IS NOT NULL AND (l.character_id="+character+" OR r.character_id="+character+" OR h.created_by_character_id="+character+") FOR UPDATE");
            if(pending.RowCount())Reject(Error::PendingApplication);
            auto orphan=Q(db,"SELECT h.house_id FROM housing_house_instances h LEFT JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.created_by_character_id="+character+" AND l.house_id IS NULL FOR UPDATE");
            if(orphan.RowCount())Reject(Error::Conflict);
        }
        bool SameInterior(const std::optional<HouseBinding::Interior>& a,const std::optional<HouseBinding::Interior>& b){
            if(a.has_value()!=b.has_value())return false;
            if(!a)return true;
            return a->native_estate_id==b->native_estate_id&&a->definition_id==b->definition_id&&a->zone_id==b->zone_id&&a->instance_id==b->instance_id&&a->zone_version==b->zone_version&&a->catalog_key==b->catalog_key&&a->catalog_revision==b->catalog_revision;
        }
        bool CrateSlot(uint32_t slot){return slot==33||(slot>=23&&slot<=32);}
        std::vector<Capture::CrateEntry> ReadCrates(Database& db,const InventorySessions::Lease& lease,const CrateBinding::ReloadPolicy* policy){
            std::vector<Capture::CrateEntry> out;if(!CrateBinding::Installed(db))return out;
            auto rows=Q(db,"SELECT slot_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+std::to_string(lease.actor.character_id)+" ORDER BY slot_id FOR UPDATE");
            if(rows.RowCount()>11)Reject(Error::UnsupportedItem);
            for(auto row=rows.begin();row.operator!=(rows.end());++row){
                const auto slot=uint32_t(Number(row[0],UINT32_MAX));if(!CrateSlot(slot)||!policy)Reject(Error::UnsupportedItem);
                auto inventory=Q(db,"SELECT "+InventoryColumns+" FROM inventory WHERE character_id="+std::to_string(lease.actor.character_id)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");if(inventory.RowCount()!=1)Reject(Error::Conflict);
                auto source=InventoryRow(inventory.begin());auto carried=CrateBinding::ReadCarriedUnderTransaction(db,source);if(!carried)Reject(Error::Conflict);
                if(source.values.augment_one||source.values.augment_two||source.values.augment_three||source.values.augment_four||source.values.augment_five||source.values.augment_six)Reject(Error::UnsupportedItem);
                auto metadata=Q(db,"SELECT idfile,UNK227,itemclass,bagslots,stackable,UNK226,UNK224,UNK228,UNK229 FROM items WHERE id="+std::to_string(source.values.item_id)+" FOR UPDATE");
                if(metadata.RowCount()!=1||!metadata.begin()[0]||!CrateGeometry::Lookup(source.values.item_id,uint32_t(Number(metadata.begin()[1],UINT32_MAX)),metadata.begin()[0])||Number(metadata.begin()[2],UINT32_MAX)||Number(metadata.begin()[3],UINT32_MAX)||Number(metadata.begin()[4],UINT32_MAX)||Number(metadata.begin()[5],UINT32_MAX))Reject(Error::UnsupportedItem);
                if(policy->ManifestDigest()!=carried.value.content_manifest_sha256||!policy->ReviewedUnderTransaction(db,lease,source,carried.value))Reject(Error::Unauthorized);
                out.push_back({std::move(carried.value),std::move(source),uint32_t(Number(metadata.begin()[6],UINT32_MAX)),uint32_t(Number(metadata.begin()[7],UINT32_MAX)),uint32_t(Number(metadata.begin()[8],UINT32_MAX))});
            }
            return out;
        }
        std::vector<Entry> Read(Database& db,const InventorySessions::Lease& lease,const HouseBinding::HouseTemplatePolicy& policy,const HouseBinding::InteriorRoutePolicy* route){
            const auto character=std::to_string(lease.actor.character_id);
            std::vector<Entry> out;
            auto houses=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,l.slot_id FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE l.character_id="+character+" AND l.location_kind=1 ORDER BY l.slot_id FOR UPDATE");
            if(houses.RowCount()>11)Reject(Error::UnsupportedItem);
            for(auto h=houses.begin();h.operator!=(houses.end());++h){
                Entry entry;
                entry.binding.house_id=Number(h[0],UINT64_MAX);
                entry.binding.revision=Number(h[1],UINT64_MAX);
                entry.binding.item_template_id=Number(h[2],UINT32_MAX);
                auto slot=uint32_t(Number(h[3],UINT32_MAX));
                if(!entry.binding.house_id||!entry.binding.revision||!Slot(slot))Reject(Error::Conflict);
                entry.binding.location={
                    HouseBinding::Location::Kind::Inventory,lease.actor.character_id,slot,0
                };
                auto inventory=Q(db,"SELECT "+InventoryColumns+" FROM inventory WHERE character_id="+character+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");
                if(inventory.RowCount()!=1)Reject(Error::Conflict);
                entry.source=InventoryRow(inventory.begin());
                if(entry.source.values.item_id!=entry.binding.item_template_id||!policy.VerifiedHouse(entry.source)||entry.source.values.augment_one||entry.source.values.augment_two||entry.source.values.augment_three||entry.source.values.augment_four||entry.source.values.augment_five||entry.source.values.augment_six)Reject(Error::UnsupportedItem);
                auto item=Q(db,"SELECT itemclass,bagslots,stackable,UNK227 FROM items WHERE id="+std::to_string(entry.source.values.item_id)+" FOR UPDATE");
                if(item.RowCount()!=1||Number(item.begin()[0],UINT32_MAX)||Number(item.begin()[1],UINT32_MAX)||Number(item.begin()[2],UINT32_MAX))Reject(Error::UnsupportedItem);
                entry.item_definition_id=Number(item.begin()[3],UINT32_MAX);
                if(!entry.item_definition_id||entry.item_definition_id==UINT32_MAX)Reject(Error::UnsupportedItem);
                auto first=EQ::InventoryProfile::CalcSlotId(int16_t(slot),EQ::invbag::SLOT_BEGIN),last=EQ::InventoryProfile::CalcSlotId(int16_t(slot),EQ::invbag::SLOT_END);
                if(first<0||last<first)Reject(Error::UnsupportedItem);
                auto children=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+character+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(last)+" FOR UPDATE");
                if(children.RowCount())Reject(Error::UnsupportedItem);
                auto evolving=Q(db,"SELECT id FROM items_evolving_details WHERE item_id="+std::to_string(entry.source.values.item_id)+" FOR UPDATE");
                auto evolving_actor=Q(db,"SELECT id FROM character_evolving_items WHERE character_id="+character+" AND item_id="+std::to_string(entry.source.values.item_id)+" AND deleted_at IS NULL FOR UPDATE");
                if(evolving.RowCount()||evolving_actor.RowCount())Reject(Error::UnsupportedItem);
                auto interior=Q(db,"SELECT native_estate_id,definition_id,zone_id,instance_id,zone_version,HEX(catalog_key),catalog_revision FROM housing_house_interiors WHERE house_id="+std::to_string(entry.binding.house_id)+" FOR UPDATE");
                if(interior.RowCount()>1)Reject(Error::DatabaseFailure);
                if(interior.RowCount()){
                    auto row=interior.begin();
                    HouseBinding::Interior i;
                    i.native_estate_id=Number(row[0],UINT32_MAX);
                    i.definition_id=Number(row[1],UINT32_MAX);
                    i.zone_id=Number(row[2],UINT32_MAX);
                    i.instance_id=Number(row[3],32000);
                    i.zone_version=Number(row[4],UINT8_MAX);
                    auto key=Unhex(row[5]);
                    i.catalog_key.assign(key.begin(),key.end());
                    i.catalog_revision=Number(row[6],UINT64_MAX);
                    if(!i.native_estate_id||i.native_estate_id==UINT32_MAX||!i.instance_id||!i.catalog_revision||i.definition_id!=entry.item_definition_id)Reject(Error::Conflict);
                    auto native=Q(db,"SELECT definition_id,HEX(catalog_key),active FROM housing_native_estates WHERE native_id="+std::to_string(i.native_estate_id)+" FOR UPDATE");
                    if(native.RowCount()!=1||Number(native.begin()[0],UINT32_MAX)!=i.definition_id||Unhex(native.begin()[1])!=key||Number(native.begin()[2],1)!=1)Reject(Error::Conflict);
                    auto instance=Q(db,"SELECT zone,version,never_expires FROM instance_list WHERE id="+std::to_string(i.instance_id)+" FOR UPDATE");
                    if(instance.RowCount()!=1||Number(instance.begin()[0],UINT32_MAX)!=i.zone_id||Number(instance.begin()[1],UINT8_MAX)!=i.zone_version||Number(instance.begin()[2],1)!=1)Reject(Error::Conflict);
                    auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Literal(i.catalog_key)+" FOR UPDATE");
                    if(catalog.RowCount()!=1||Number(catalog.begin()[0],UINT64_MAX)!=i.catalog_revision||Number(catalog.begin()[2],UINT32_MAX)!=20130510)Reject(Error::Conflict);
                    auto origin=Number(catalog.begin()[1],2);
                    if(origin==2){
                        auto database=Q(db,"SELECT DATABASE()");
                        std::string name=database.begin()[0];
                        if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))Reject(Error::Unauthorized);
                    }else if(origin!=1)Reject(Error::Conflict);
                    Catalog::Bundle decoded;
                    try{
                        decoded=Catalog::DecodePayloads(Unhex(catalog.begin()[3]),Unhex(catalog.begin()[4]));
                    }catch(...){
                        Reject(Error::DatabaseFailure);
                    }const EstateWire::Definition* definition=nullptr;
                    for(const auto& d:decoded.definitions)if(d.id==i.definition_id){
                        if(definition)Reject(Error::DatabaseFailure);
                        definition=&d;
                    }
                    if(!definition||(definition->type!=2&&definition->type!=3)||definition->zone_id!=i.zone_id)Reject(Error::Conflict);
                    if(!route||!route->VerifiedDestination(i,*definition,entry.source.values.item_id))Reject(Error::UnsupportedItem);
                    entry.binding.interior=i;
                }
                out.push_back(std::move(entry));
            }
            return out;
        }
        template<class T,class F>Result<T> Run(Database& db,F body){
            try{
                Tx tx(db);
                Schema(db);
                auto value=body();
                tx.Commit();
                return {
                    std::move(value),Error::None
                };
            }catch(const Failure& e){
                return {
                    {
                    },e.error
                };
            }catch(...){
                return {
                    {
                    },Error::DatabaseFailure
                };
            }
        }
    }
    Result<Loaded> ProjectGuidUpdate(const InventoryRepository::LoadRow& input,int32_t serial){
        try{
            InventoryRepository::LoadRow decoded;
            if(serial<=0||!InventoryRepository::DecodeLoadRow(input.fields,decoded)||!decoded.values.character_id||!decoded.values.item_id)return {{},Error::InvalidArgument};
            LiveItems::InventoryRow source;
            source.values=decoded.values;
            if(!input.fields[2])source.null_fields|=LiveItems::NullItemId;
            if(!input.fields[3])source.null_fields|=LiveItems::NullCharges;
            if(!input.fields[12])source.null_fields|=LiveItems::NullCustomData;
            if(!input.fields[16])source.null_fields|=LiveItems::NullGuid;
            auto typed=source;typed.values=input.values;
            if(!CustodyCodec::Valid(typed)||CustodyCodec::Encode(typed)!=CustodyCodec::Encode(source))return {{},Error::Conflict};
            source.values.guid=uint64_t(serial);
            source.null_fields&=~LiveItems::NullGuid;
            return {{std::move(source),serial},Error::None};
        }catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<std::vector<Loaded>> SelectBoundLoadWitnesses(const Capture& capture,const std::vector<Loaded>& all){
        try{
            if(!capture.reservation.active||!capture.reservation.actor.character_id||!capture.reservation.actor.account_id)
                return {{},Error::InvalidArgument};
            const auto character=capture.reservation.actor.character_id;
            std::map<uint32_t,const Loaded*> by_slot;
            std::set<int32_t> serials;
            for(const auto& row:all){
                if(row.persisted.values.character_id!=character||row.runtime_serial<=0||
                    row.persisted.values.guid!=uint64_t(row.runtime_serial)||(row.persisted.null_fields&LiveItems::NullGuid)||
                    !CustodyCodec::Valid(row.persisted)||!by_slot.emplace(row.persisted.values.slot_id,&row).second||
                    !serials.insert(row.runtime_serial).second)return {{},Error::Conflict};
            }
            std::vector<Loaded> selected;
            std::set<uint32_t> slots;
            for(const auto& entry:capture.entries){
                const auto slot=entry.source.values.slot_id;
                if(!Slot(slot)||entry.source.values.character_id!=character||!entry.binding.house_id||
                    entry.binding.location.kind!=HouseBinding::Location::Kind::Inventory||
                    entry.binding.location.character_id!=character||entry.binding.location.slot_id!=slot||
                    !slots.insert(slot).second)return {{},Error::Conflict};
                const auto found=by_slot.find(slot);
                if(found==by_slot.end())return {{},Error::Conflict};
                auto expected=entry.source;
                expected.values.guid=uint64_t(found->second->runtime_serial);
                expected.null_fields&=~LiveItems::NullGuid;
                if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(found->second->persisted))return {{},Error::Conflict};
                selected.push_back(*found->second);
            }
            for(const auto& entry:capture.crates){
                const auto slot=entry.source.values.slot_id;const auto& b=entry.binding;
                if(!CrateSlot(slot)||entry.source.values.character_id!=character||!b.binding.crate_id||b.binding.custody_id||b.inventory_character_id!=character||b.inventory_slot_id!=slot||!slots.insert(slot).second)return {{},Error::Conflict};
                CrateBinding::EncodeAllocation(b);const auto found=by_slot.find(slot);if(found==by_slot.end())return {{},Error::Conflict};
                auto expected=entry.source;expected.values.guid=uint64_t(found->second->runtime_serial);expected.null_fields&=~LiveItems::NullGuid;
                if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(found->second->persisted))return {{},Error::Conflict};selected.push_back(*found->second);
            }
            return {std::move(selected),Error::None};
        }catch(...){return {{},Error::DatabaseFailure};}
    }
    Service::Service(std::unique_ptr<Database> db,bool enabled):database_(std::move(db)),enabled_(enabled){
    }
    Result<Capture> Service::BeforeLoad(const InventorySessions::Lease& lease,SessionFence& fence,const HouseBinding::HouseTemplatePolicy& policy,const HouseBinding::InteriorRoutePolicy* route,const CrateBinding::ReloadPolicy* crates){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Capture>(*database_,[&]{
            Authority(*database_,lease,fence);return Capture{
                lease,Read(*database_,lease,policy,route),ReadCrates(*database_,lease,crates)
            };
        });
    }
    Result<Validated> Service::AfterLoad(const Capture& capture,const std::vector<Loaded>& loaded,SessionFence& fence,const HouseBinding::HouseTemplatePolicy& policy,const HouseBinding::InteriorRoutePolicy* route,const CrateBinding::ReloadPolicy* crates){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Validated>(*database_,[&]{
            Authority(*database_,capture.reservation,fence);auto current=Read(*database_,capture.reservation,policy,route);auto current_crates=ReadCrates(*database_,capture.reservation,crates);if(current.size()!=capture.entries.size()||current_crates.size()!=capture.crates.size()||loaded.size()!=current.size()+current_crates.size())Reject(Error::Conflict);Validated result;result.capture_=capture;result.loaded_=loaded;std::set<uint32_t> slots;std::set<int32_t> serials;
            for(size_t n=0;n<current.size();++n){
                const auto& before=capture.entries[n];const auto& now=current[n];if(before.binding.house_id!=now.binding.house_id||before.binding.revision!=now.binding.revision||before.binding.item_template_id!=now.binding.item_template_id||before.binding.location.kind!=HouseBinding::Location::Kind::Inventory||before.binding.location.character_id!=capture.reservation.actor.character_id||before.binding.location.custody_id||!before.binding.pending_key.empty()||before.source.values.slot_id!=before.binding.location.slot_id||before.item_definition_id!=now.item_definition_id||before.binding.location.slot_id!=now.binding.location.slot_id||!SameInterior(before.binding.interior,now.binding.interior))Reject(Error::Conflict);
                const Loaded* witness=nullptr;for(const auto& candidate:loaded)if(candidate.persisted.values.slot_id==now.source.values.slot_id){
                    if(witness)Reject(Error::Conflict);witness=&candidate;
                }if(!witness||witness->runtime_serial<=0||!slots.insert(witness->persisted.values.slot_id).second||!serials.insert(witness->runtime_serial).second||witness->persisted.values.guid!=uint64_t(witness->runtime_serial)||(witness->persisted.null_fields&LiveItems::NullGuid))Reject(Error::Conflict);
                auto expected=before.source;expected.values.guid=witness->runtime_serial;expected.null_fields&=~LiveItems::NullGuid;if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(now.source)||CustodyCodec::Encode(now.source)!=CustodyCodec::Encode(witness->persisted))Reject(Error::Conflict);if(!fence.LoadedMatches(witness->persisted,witness->runtime_serial))Reject(Error::UnheldReservation);
            }
            for(size_t n=0;n<current_crates.size();++n){
                const auto& before=capture.crates[n];const auto& now=current_crates[n];
                if(CrateBinding::EncodeAllocation(before.binding)!=CrateBinding::EncodeAllocation(now.binding)||before.placement_mask!=now.placement_mask||before.minimum_scale_bits!=now.minimum_scale_bits||before.maximum_scale_bits!=now.maximum_scale_bits)Reject(Error::Conflict);
                const Loaded* witness=nullptr;for(const auto& candidate:loaded)if(candidate.persisted.values.slot_id==now.source.values.slot_id){if(witness)Reject(Error::Conflict);witness=&candidate;}
                if(!witness||witness->runtime_serial<=0||!slots.insert(witness->persisted.values.slot_id).second||!serials.insert(witness->runtime_serial).second||witness->persisted.values.guid!=uint64_t(witness->runtime_serial)||(witness->persisted.null_fields&LiveItems::NullGuid))Reject(Error::Conflict);
                auto expected=before.source;expected.values.guid=witness->runtime_serial;expected.null_fields&=~LiveItems::NullGuid;
                if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(now.source)||CustodyCodec::Encode(now.source)!=CustodyCodec::Encode(witness->persisted))Reject(Error::Conflict);
                if(!fence.LoadedMatches(witness->persisted,witness->runtime_serial))Reject(Error::UnheldReservation);
            }
            return result;
        });
    }
    Result<Validated> Service::AfterInventoryLoad(const Capture& capture,const std::vector<Loaded>& all,SessionFence& fence,const HouseBinding::HouseTemplatePolicy& policy,const HouseBinding::InteriorRoutePolicy* route,const CrateBinding::ReloadPolicy* crates){
        if(!enabled_)return {{},Error::Disabled};
        const auto selected=SelectBoundLoadWitnesses(capture,all);
        if(!selected)return {{},selected.error};
        return AfterLoad(capture,selected.value,fence,policy,route,crates);
    }
    Result<bool> Service::Hydrate(const Validated& validated,EQ::InventoryProfile& inventory,SessionFence& fence){
        try{
            if(!fence.HeldFor(validated.capture_.reservation))Reject(Error::UnheldReservation);
            struct Assignment {
                EQ::ItemInstance* item;
                uint32_t estate;
                uint64_t house,crate;
            };
            std::vector<Assignment> assignments;
            std::set<EQ::ItemInstance*> pointers;
            for(const auto& entry:validated.capture_.entries){
                const Loaded* loaded=nullptr;
                for(const auto& row:validated.loaded_)if(row.persisted.values.slot_id==entry.source.values.slot_id)loaded=&row;
                if(!loaded||!fence.LoadedMatches(loaded->persisted,loaded->runtime_serial))Reject(Error::UnheldReservation);
                auto item=inventory.GetItem(int16_t(entry.source.values.slot_id));
                const auto estate=entry.binding.interior?entry.binding.interior->native_estate_id:UINT32_MAX;
                if(!item||item->GetHousingCrateID()||!pointers.insert(item).second||item->GetSerialNumber()!=loaded->runtime_serial||item->GetID()!=entry.source.values.item_id||!item->GetItem()||item->GetItem()->HousingDefinitionID!=entry.item_definition_id||(item->GetHousingEstateID()!=UINT32_MAX&&item->GetHousingEstateID()!=estate)||(item->GetHousingHouseID()!=0&&item->GetHousingHouseID()!=entry.binding.house_id))Reject(Error::Conflict);
                assignments.push_back({
                    item,estate,entry.binding.house_id,0
                });
            }
            for(const auto& entry:validated.capture_.crates){
                const Loaded* loaded=nullptr;for(const auto& row:validated.loaded_)if(row.persisted.values.slot_id==entry.source.values.slot_id)loaded=&row;
                if(!loaded||!fence.LoadedMatches(loaded->persisted,loaded->runtime_serial))Reject(Error::UnheldReservation);
                auto* item=inventory.GetItem(int16_t(entry.source.values.slot_id));const auto& b=entry.binding.binding;
                if(!item||!pointers.insert(item).second||item->GetSerialNumber()!=loaded->runtime_serial||item->GetID()!=entry.source.values.item_id||!CratePlacement::Model(*item)||item->GetUnscaledItem()->HousingDefinitionID!=b.definition_id||item->GetHousingHouseID()||(item->GetHousingCrateID()&&item->GetHousingCrateID()!=b.crate_id)||(item->GetHousingEstateID()!=UINT32_MAX&&item->GetHousingEstateID()!=b.native_estate_id))Reject(Error::Conflict);
                const auto* data=item->GetUnscaledItem();uint32_t minimum,maximum;std::memcpy(&minimum,&data->HousingMinimumScale,4);std::memcpy(&maximum,&data->HousingMaximumScale,4);
                if(data->Placeable!=entry.placement_mask||minimum!=entry.minimum_scale_bits||maximum!=entry.maximum_scale_bits)Reject(Error::Conflict);
                assignments.push_back({item,b.native_estate_id,0,b.crate_id});
            }
            for(const auto& [item,estate,house,crate]:assignments){
                if(crate){if(!item->SetHousingCrateID(crate)||!item->SetHousingEstateID(estate))Reject(Error::Conflict);}
                else if(!item->SetHousingEstateID(estate)||!item->SetHousingHouseID(house))Reject(Error::Conflict);
            }
            return {
                true,Error::None
            };
        }catch(const Failure& e){
            return {
                false,e.error
            };
        }catch(...){
            return {
                false,Error::DatabaseFailure
            };
        }
    }
}
