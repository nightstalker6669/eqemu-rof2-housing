#ifndef EQEMU_HOUSING_RETURN_PARCEL_DB_H
#define EQEMU_HOUSING_RETURN_PARCEL_DB_H
#include "inventory_row_db.h"
#include "item_payload_db.h"
#include "crate_binding.h"
#include "crate_creation_origin_db.h"
#include "vault_storage_db.h"
#include "../rulesys.h"
#include "../inventory_profile.h"
#include <openssl/sha.h>
#include <set>
namespace Housing::ReturnParcels {
// No authority or transaction lifecycle is acquired by these primitives.
// Queue is for the authenticated OFFLINE retirement transaction only. Claim
// additionally requires the live bridge's uninterrupted simulation barrier.
struct Capacity:std::runtime_error {Capacity():std::runtime_error("Housing return parcel capacity exhausted") {}};
struct Record {
 uint32_t parcel_id=0,estate_id=0,owner=0,account=0,owner_kind=0,estate_owner_id=0,native_slot=0;
 uint64_t source_custody_id=0,crate_id=0;bool generated=false,claimed=false;
 LiveItems::InventoryRow row,original_row;
 std::optional<CrateBinding::Allocation> allocation;
 std::vector<uint8_t> source_state;
};
namespace Detail {
using Bytes=std::vector<uint8_t>;
inline void Require(bool b){if(!b)throw std::invalid_argument("Housing parcel identity or delivery conflict");}
inline auto Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Housing parcel SQL failure");return r;}
inline uint64_t N(const char* p){Require(p&&*p);std::string s=p;Require(s.find_first_not_of("0123456789")==std::string::npos);return std::stoull(s);}
inline std::string I(uint64_t n){return std::to_string(n);}
inline std::string S(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
inline std::string H(const Bytes& b){return VaultStorage::Hex(b);}
inline Bytes U(const char* p){return VaultStorage::Unhex(p);}
inline Bytes Hash(const Bytes& b){Bytes h(32);Require(bool(SHA256(b.data(),b.size(),h.data())));return h;}
inline void One(Database& db,const std::string& sql){Require(Q(db,sql).RowsAffected()==1);}
inline void Tx(Database& db){auto r=Q(db,"SELECT @@in_transaction");Require(r.RowCount()==1&&N(r.begin()[0])==1);}
inline void Recipient(Database& db,uint32_t owner,uint32_t account){auto r=Q(db,"SELECT c.account_id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+I(owner)+" AND c.deleted_at IS NULL FOR UPDATE");Require(owner&&account&&r.RowCount()==1&&N(r.begin()[0])==account);}
inline bool Installed(Database& db){auto r=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_return_parcels'");if(!r.RowCount())return false;Require(r.RowCount()==1&&r.begin()[0]&&std::string(r.begin()[0])=="InnoDB");auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN('character_parcels','character_parcels_containers','inventory','housing_crate_locations')");Require(engines.RowCount()==1&&N(engines.begin()[0])==4);return true;}
// Native parcel columns are a presentation mirror. The typed sidecar is the
// complete item authority, including signed64 evolution amounts, augments and
// exact child rows. Native unsigned32 amounts saturate only this mirror.
inline uint32_t NativeAmount(const LiveItems::InventoryRow& row){for(const auto& state:EvolvingItemCustody::Decode(row.evolving_payload))if(!state.component)return state.amount<=0?0:uint32_t(std::min<uint64_t>(uint64_t(state.amount),UINT32_MAX));return 0;}
inline std::array<uint32_t,6> Augments(const LiveItems::InventoryRow& row){const auto& v=row.values;return {v.augment_one,v.augment_two,v.augment_three,v.augment_four,v.augment_five,v.augment_six};}
inline uint32_t Quantity(const LiveItems::InventoryRow& row){return std::max(1u,uint32_t(row.values.charges));}
inline std::string NativeValues(const LiveItems::InventoryRow& row){std::string text=I(row.values.item_id);for(auto id:Augments(row))text+=","+I(id);return text;}
inline void Children(Database& db,uint32_t parcel,const LiveItems::InventoryRow& row,bool insert=false){
 if(insert)for(const auto& child:row.children)One(db,"INSERT INTO character_parcels_containers(parcels_id,slot_id,item_id,aug_slot_1,aug_slot_2,aug_slot_3,aug_slot_4,aug_slot_5,aug_slot_6,quantity,evolve_amount) VALUES("+I(parcel)+","+I(child.values.slot_id)+","+NativeValues(child)+","+I(Quantity(child))+","+I(NativeAmount(child))+")");
 auto rows=Q(db,"SELECT slot_id,item_id,aug_slot_1,aug_slot_2,aug_slot_3,aug_slot_4,aug_slot_5,aug_slot_6,quantity,evolve_amount FROM character_parcels_containers WHERE parcels_id="+I(parcel)+" ORDER BY slot_id FOR UPDATE");Require(rows.RowCount()==row.children.size());size_t index=0;for(auto p=rows.begin();p!=rows.end();++p){const auto& child=row.children.at(index++);Require(N(p[0])==child.values.slot_id&&N(p[1])==child.values.item_id);auto aug=Augments(child);for(size_t k=0;k<6;++k)Require(N(p[k+2])==aug[k]);Require(N(p[8])==Quantity(child)&&N(p[9])==NativeAmount(child));}
}
inline Record Read(Database& db,uint32_t id,uint32_t owner,uint32_t account){
 auto rows=Q(db,"SELECT estate_id,owner_kind,estate_owner_id,owner_character_id,account_id,source_custody_id,crate_id,generated,native_slot,native_quantity,sent_epoch,schema_descriptor,HEX(row_blob),HEX(row_sha256),HEX(source_state),HEX(state_sha256),claim_key,claim_slot,HEX(claim_row),HEX(claim_sha256),lease_epoch,session_key,HEX(lease_token),claimed_epoch FROM housing_return_parcels WHERE parcel_id="+I(id)+" FOR UPDATE");Require(rows.RowCount()==1);auto p=rows.begin();
 Record out;out.parcel_id=id;out.estate_id=N(p[0]);out.owner_kind=N(p[1]);out.estate_owner_id=N(p[2]);out.owner=N(p[3]);out.account=N(p[4]);out.source_custody_id=p[5]?N(p[5]):0;out.crate_id=p[6]?N(p[6]):0;Require(N(p[7])<=1);out.generated=N(p[7]);out.native_slot=N(p[8]);auto quantity=N(p[9]),sent=N(p[10]);Require(id&&out.estate_id&&(out.owner_kind==1||out.owner_kind==3)&&out.estate_owner_id&&out.owner==owner&&out.account==account&&out.native_slot&&quantity&&sent&&p[11]&&std::string(p[11])==LiveItems::Service::SchemaDescriptor());
 Recipient(db,owner,account);auto estate=Q(db,"SELECT owner_kind,owner_id FROM housing_estates WHERE id="+I(out.estate_id)+" FOR UPDATE");Require(estate.RowCount()==1&&N(estate.begin()[0])==out.owner_kind&&N(estate.begin()[1])==out.estate_owner_id);
 const auto raw=U(p[12]);Require(Hash(raw)==U(p[13]));out.row=CustodyCodec::Decode(raw);Require(CustodyCodec::Valid(out.row)&&CustodyCodec::Encode(out.row)==raw&&out.row.values.character_id==owner&&out.row.values.item_id&&!(out.row.null_fields&LiveItems::NullItemId));out.original_row=out.row;out.source_state=U(p[14]);Require(!out.source_state.empty()&&out.source_state.size()<=64*1024*1024&&Hash(out.source_state)==U(p[15]));Require(out.generated?(!out.source_custody_id&&out.crate_id):bool(out.source_custody_id));
 out.claimed=bool(p[16]);auto native=Q(db,"SELECT char_id,item_id,aug_slot_1,aug_slot_2,aug_slot_3,aug_slot_4,aug_slot_5,aug_slot_6,slot_id,quantity,evolve_amount,from_name,note,UNIX_TIMESTAMP(sent_date) FROM character_parcels WHERE id="+I(id)+" FOR UPDATE");
 if(out.claimed){Require(!native.RowCount()&&p[16][0]&&p[17]&&p[18]&&p[19]&&p[20]&&N(p[20])&&p[21]&&p[21][0]&&U(p[22]).size()==32&&p[23]&&N(p[23]));auto after=U(p[18]);Require(Hash(after)==U(p[19]));auto expected=out.row;ItemPayload::Owner(expected,owner);expected.values.slot_id=N(p[17]);Require(expected.values.slot_id>=23&&expected.values.slot_id<=30&&CustodyCodec::Encode(expected)==after);out.row=expected;
 }else{
  for(unsigned i=17;i<24;++i)Require(!p[i]);Require(native.RowCount()==1);auto n=native.begin();Require(N(n[0])==owner&&N(n[1])==out.row.values.item_id);auto aug=Augments(out.row);for(unsigned i=2;i<8;++i)Require(N(n[i])==aug[i-2]);Require(N(n[8])==out.native_slot&&N(n[9])==quantity&&n[10]&&N(n[10])==NativeAmount(out.row)&&n[11]&&std::string(n[11])=="Real Estate"&&n[12]&&std::string(n[12])=="Property retirement return"&&N(n[13])==sent);
  Children(db,id,out.row);ItemPayload::Metadata(db,out.row);if(out.source_custody_id)ItemPayload::ValidateHeld(db,out.row,out.source_custody_id);
 }
 return out;
}
}
inline bool Installed(Database& db){return Detail::Installed(db);}
// Reads only the queued location, never fabricates a carried inventory row.
inline CrateBinding::Allocation ReadCrateUnderTransaction(Database& db,uint32_t parcel){
 using namespace Detail;Tx(db);auto identity=Q(db,"SELECT owner_character_id,account_id FROM housing_return_parcels WHERE parcel_id="+I(parcel)+" FOR UPDATE");Require(identity.RowCount()==1);auto record=Read(db,parcel,N(identity.begin()[0]),N(identity.begin()[1]));Require(!record.claimed&&record.crate_id);
 auto rows=Q(db,"SELECT c.crate_id,c.revision,c.item_template_id,c.created_by_character_id,c.packed,HEX(c.content_manifest_sha256),c.creation_request_key,e.native_estate_id,e.parent_native_estate_id,e.ownership_estate_id,e.definition_id,e.catalog_key,e.catalog_revision,n.node_key,n.active,n.parent_native_id,n.definition_id,n.catalog_key,n.neighborhood_id,p.neighborhood_id,l.character_id,l.slot_id,l.custody_id FROM housing_crate_instances c JOIN housing_crate_locations l USING(crate_id) JOIN housing_crate_estates e USING(crate_id) JOIN housing_native_estates n ON n.native_id=e.native_estate_id JOIN housing_native_estates p ON p.native_id=e.parent_native_estate_id WHERE c.crate_id="+I(record.crate_id)+" AND l.location_kind=3 AND l.parcel_id="+I(parcel)+" FOR UPDATE");Require(rows.RowCount()==1);auto p=rows.begin();CrateBinding::Allocation a;auto& b=a.binding;b={N(p[0]),N(p[1]),0,N(p[12]),uint32_t(N(p[2])),uint32_t(N(p[7])),uint32_t(N(p[8])),uint32_t(N(p[10])),uint32_t(N(p[9])),p[11]?p[11]:""};a.created_by_character_id=N(p[3]);Require(N(p[4])<=1);a.packed=N(p[4]);auto digest=U(p[5]);Require(digest.size()==32);std::copy(digest.begin(),digest.end(),a.content_manifest_sha256.begin());Require(p[6]);a.creation_request_key=p[6];a.parcel_id=parcel;
 Require(b.crate_id==record.crate_id&&b.item_template_id==record.row.values.item_id&&a.created_by_character_id==record.owner&&p[13]&&std::string(p[13])=="crate:"+I(b.crate_id)&&!N(p[14])&&N(p[15])==b.parent_native_estate_id&&N(p[16])==b.definition_id&&p[17]&&std::string(p[17])==b.catalog_key&&!p[20]&&!p[21]&&!p[22]);if(N(p[18])!=N(p[19]))Require(Q(db,"SELECT house_id FROM housing_house_interiors WHERE native_estate_id="+I(b.parent_native_estate_id)+" FOR UPDATE").RowCount()==1);
 CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);CrateBinding::EncodeAllocation(a);auto members=CrateBinding::ValidateMembersUnderTransaction(db,a);Require(bool(members));return a;
}
// Stable delivery witness: mutable native deletion/claim/slot is represented by
// its immutable claim record, never by changing the sealed retirement bytes.
inline std::vector<uint8_t> DeliveryWitness(const Record& r){using namespace Detail;Bytes b;CustodyCodec::W(b,1,4);for(auto n:{uint64_t(r.parcel_id),uint64_t(r.estate_id),uint64_t(r.owner_kind),uint64_t(r.estate_owner_id),uint64_t(r.owner),uint64_t(r.account),r.source_custody_id,r.crate_id,uint64_t(r.generated),uint64_t(r.native_slot)})CustodyCodec::W(b,n,8);auto raw=CustodyCodec::Encode(r.original_row);CustodyCodec::W(b,raw.size(),4);b.insert(b.end(),raw.begin(),raw.end());auto h=Hash(r.source_state);b.insert(b.end(),h.begin(),h.end());return b;}
inline std::optional<Record> Lookup(Database& db,uint32_t parcel,uint32_t owner,uint32_t account){using namespace Detail;Tx(db);if(!Detail::Installed(db))return {};auto exists=Q(db,"SELECT parcel_id FROM housing_return_parcels WHERE parcel_id="+I(parcel)+" FOR UPDATE");if(!exists.RowCount())return {};auto out=Read(db,parcel,owner,account);if(out.crate_id&&!out.claimed)out.allocation=ReadCrateUnderTransaction(db,parcel);return out;}
// Source is an exact canonical row captured before custody removal. No source
// metadata is regenerated from the item template; no inventory slot is staged.
inline Record QueueUnderTransaction(Database& db,uint32_t estate,uint32_t owner,uint32_t account,const LiveItems::InventoryRow& row,uint64_t custody,uint64_t crate,const std::vector<uint8_t>& state,bool generated=false){
 using namespace Detail;Tx(db);Require(Detail::Installed(db));Recipient(db,owner,account);Require(CustodyCodec::Valid(row)&&row.values.character_id==owner&&row.values.item_id&&!(row.null_fields&LiveItems::NullItemId)&&!state.empty()&&state.size()<=64*1024*1024&&(generated?(!custody&&crate):bool(custody)));Require(!Q(db,"SELECT character_id FROM housing_inventory_sessions WHERE character_id="+I(owner)+" AND active=1 LIMIT 1 FOR UPDATE").RowCount());Require(!Q(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+I(owner)+" AND (active=1 OR pending_key IS NOT NULL) LIMIT 1 FOR UPDATE").RowCount());
 auto e=Q(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+I(estate)+" FOR UPDATE");Require(e.RowCount()==1);auto kind=N(e.begin()[0]),estateowner=N(e.begin()[1]);Require(kind==1||kind==3);
 const auto limit=RuleI(Parcel,ParcelMaxItems);Require(limit>0&&limit<=INT32_MAX);std::set<uint32_t> occupied;auto slots=Q(db,"SELECT slot_id FROM character_parcels WHERE char_id="+I(owner)+" ORDER BY slot_id FOR UPDATE");for(auto p=slots.begin();p!=slots.end();++p)occupied.insert(N(p[0]));uint32_t slot=1;while(slot<=uint32_t(limit)&&occupied.count(slot))++slot;if(slot>uint32_t(limit))throw Capacity();const auto quantity=Quantity(row);ItemPayload::Metadata(db,row);if(custody)ItemPayload::ValidateHeld(db,row,custody);auto raw=CustodyCodec::Encode(row);
 auto made=Q(db,"INSERT INTO character_parcels(char_id,item_id,aug_slot_1,aug_slot_2,aug_slot_3,aug_slot_4,aug_slot_5,aug_slot_6,slot_id,quantity,evolve_amount,from_name,note,sent_date) VALUES("+I(owner)+","+NativeValues(row)+","+I(slot)+","+I(quantity)+","+I(NativeAmount(row))+",'Real Estate','Property retirement return',NOW())");Require(made.RowsAffected()==1&&made.LastInsertedID()&&made.LastInsertedID()<=UINT32_MAX);auto parcel=made.LastInsertedID();Children(db,uint32_t(parcel),row,true);
 One(db,"INSERT INTO housing_return_parcels(parcel_id,estate_id,owner_kind,estate_owner_id,owner_character_id,account_id,source_custody_id,crate_id,generated,native_slot,native_quantity,sent_epoch,schema_descriptor,row_blob,row_sha256,source_state,state_sha256) VALUES("+I(parcel)+","+I(estate)+","+I(kind)+","+I(estateowner)+","+I(owner)+","+I(account)+","+(custody?I(custody):"NULL")+","+(crate?I(crate):"NULL")+","+I(generated)+","+I(slot)+","+I(quantity)+",(SELECT UNIX_TIMESTAMP(sent_date) FROM character_parcels WHERE id="+I(parcel)+"),"+S(db,LiveItems::Service::SchemaDescriptor())+",x'"+H(raw)+"',x'"+H(Hash(raw))+"',x'"+H(state)+"',x'"+H(Hash(state))+"')");return Read(db,parcel,owner,account);
}
// Caller authenticated the exact lease and ReturnAllowed/DestinationEmpty while
// holding Bridge's main-thread barrier. SQL repeats every durable identity check.
inline Record ClaimUnderTransaction(Database& db,const Record& expected,uint32_t destination,const LiveItems::Lease& lease,const std::string& key){
 using namespace Detail;Tx(db);Require(destination>=23&&destination<=30&&!key.empty()&&key.size()<=96&&lease.actor.character_id==expected.owner&&lease.actor.account_id==expected.account&&lease.epoch&&!lease.session_key.empty()&&lease.pending_key.empty());auto l=Q(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+I(expected.owner)+" FOR UPDATE");Require(l.RowCount()==1);auto p=l.begin();Require(N(p[0])==expected.account&&N(p[1])==lease.epoch&&p[2]&&std::string(p[2])==lease.session_key&&U(p[3])==Bytes(lease.token.begin(),lease.token.end())&&N(p[4])==1&&!p[5]);
 auto found=Lookup(db,expected.parcel_id,expected.owner,expected.account);Require(found&&!found->claimed&&found->estate_id==expected.estate_id&&found->native_slot==expected.native_slot&&found->crate_id==expected.crate_id&&CustodyCodec::Encode(found->row)==CustodyCodec::Encode(expected.row));auto out=*found;
 std::set<uint32_t> slots{destination};for(unsigned i=0;i<EQ::invbag::SLOT_COUNT;++i)slots.insert(EQ::InventoryProfile::CalcSlotId(int16_t(destination),uint8_t(i)));std::string list;for(auto slot:slots){if(!list.empty())list+=",";list+=I(slot);}Require(!Q(db,"SELECT slot_id FROM inventory WHERE character_id="+I(out.owner)+" AND slot_id IN("+list+") FOR UPDATE").RowCount());Require(!Q(db,"SELECT house_id FROM housing_house_locations WHERE location_kind=1 AND character_id="+I(out.owner)+" AND slot_id="+I(destination)+" FOR UPDATE").RowCount());Require(!Q(db,"SELECT crate_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+I(out.owner)+" AND slot_id="+I(destination)+" FOR UPDATE").RowCount());
 ItemPayload::Collisions(db,out.row,out.owner,out.account,out.source_custody_id);out.row=ItemPayload::Release(db,out.original_row,out.source_custody_id,out.owner,destination);const auto raw=CustodyCodec::Encode(out.row);ItemPayload::Insert(db,out.row);auto saved=ItemPayload::Read(db,out.owner,destination,&out.row);ItemPayload::ValidateInventory(db,saved);Require(CustodyCodec::Encode(saved)==raw);
 if(out.crate_id){Require(out.allocation&&out.allocation->binding.revision<UINT64_MAX);auto revision=out.allocation->binding.revision;One(db,"UPDATE housing_crate_locations SET location_kind=1,character_id="+I(out.owner)+",slot_id="+I(destination)+",parcel_id=NULL WHERE crate_id="+I(out.crate_id)+" AND location_kind=3 AND parcel_id="+I(out.parcel_id));One(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+I(out.crate_id)+" AND revision="+I(revision));auto carried=CrateBinding::ReadCarriedUnderTransaction(db,out.row);Require(bool(carried)&&carried.value.binding.crate_id==out.crate_id);out.allocation=carried.value;}
 Require(Q(db,"DELETE FROM character_parcels_containers WHERE parcels_id="+I(out.parcel_id)).RowsAffected()==out.row.children.size());One(db,"DELETE FROM character_parcels WHERE id="+I(out.parcel_id)+" AND char_id="+I(out.owner)+" AND slot_id="+I(out.native_slot));One(db,"UPDATE housing_return_parcels SET claim_key="+S(db,key)+",claim_slot="+I(destination)+",claim_row=x'"+H(raw)+"',claim_sha256=x'"+H(Hash(raw))+"',lease_epoch="+I(lease.epoch)+",session_key="+S(db,lease.session_key)+",lease_token=x'"+H(Bytes(lease.token.begin(),lease.token.end()))+"',claimed_epoch=UNIX_TIMESTAMP() WHERE parcel_id="+I(out.parcel_id)+" AND claim_key IS NULL");out.claimed=true;return out;
}
// Fresh connection only, caller TX. The stable locking read waits out the old
// COMMIT before an absent claim key can establish rollback. This is a DB outcome
// record, not an assertion that live memory was applied.
inline std::optional<Record> ResolveClaim(Database& db,uint32_t id,uint32_t owner,uint32_t account,const std::string& key,uint32_t destination){using namespace Detail;Tx(db);auto r=Q(db,"SELECT claim_key,claim_slot FROM housing_return_parcels WHERE parcel_id="+I(id)+" FOR UPDATE");Require(r.RowCount()==1);auto p=r.begin();if(!p[0]){auto out=Lookup(db,id,owner,account);Require(out&&!out->claimed);return {};}Require(std::string(p[0])==key&&p[1]&&N(p[1])==destination);auto out=Lookup(db,id,owner,account);Require(out&&out->claimed);if(out->crate_id){auto carried=CrateBinding::ReadCarriedUnderTransaction(db,out->row);Require(bool(carried)&&carried.value.binding.crate_id==out->crate_id);out->allocation=carried.value;}return out;}
}
#endif
