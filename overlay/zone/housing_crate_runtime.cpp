#include "housing_crate_runtime.h"
#include "housing_inventory_bridge.h"
#include "housing_house_runtime.h"
#include "../common/housing/interior_placement_review.h"
#include "client.h"
#include "zone.h"
#include "../common/eqemu_config.h"
#include "../common/housing/crate_content_loader.h"
#include "../common/housing/crate_placement_model.h"
#include "../common/housing/reviewed_crate_runtime.h"
#include <fstream>
#include <filesystem>
extern Zone* zone;
namespace Housing::CrateRuntime {
namespace {
using Content=CrateTest::Content;
std::shared_ptr<const Content> selected;bool initialized=false;
std::string Lower(std::string s){for(auto& c:s)if(c>='A'&&c<='F')c=char(c-'A'+'a');return s;}
bool DatabaseReady(){const auto* config=EQEmuConfig::get();return config&&config->DatabaseDB=="peq_housing_test"&&InventoryBridge::IsRegisteredZoneThread()&&RuleB(Housing,EnablePlayerHousing);}
bool Metadata(Database& db,const Content& s){
 auto r=db.QueryDatabase(std::string("SELECT idfile,UNK224,UNK227,UNK228,UNK229,itemclass,bagslots,stackable,UNK226 FROM items WHERE id=62586 FOR UPDATE"),false);if(!r.Success()||r.RowCount()!=1)return false;auto row=r.begin();for(size_t i=0;i<9;++i)if(!row[i])return false;
 if(std::string(row[0])!="IT20028"||std::stoull(row[1])!=s.placement_mask||std::stoull(row[2])!=174||std::stoull(row[3])!=s.minimum_scale_bits||std::stoull(row[4])!=s.maximum_scale_bits)return false;for(size_t i=5;i<9;++i)if(std::stoull(row[i]))return false;
 auto catalog=db.QueryDatabase("SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key='"+db.Escape(s.catalog_key)+"' FOR UPDATE",false);
 if(!catalog.Success()||catalog.RowCount()!=1)return false;auto c=catalog.begin();for(size_t i=0;i<5;++i)if(!c[i])return false;
 return std::stoull(c[0])==s.catalog_revision&&std::stoull(c[1])==2&&std::stoull(c[2])==20130510&&Lower(c[3])==s.definitions_hex&&Lower(c[4])==s.regions_hex;
}
std::string Hex(const std::vector<uint8_t>& bytes){const char* h="0123456789abcdef";std::string out;for(auto b:bytes){out+=h[b>>4];out+=h[b&15];}return out;}
bool ClientReady(Client& c){return DatabaseReady()&&zone&&c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&c.Connected()&&c.ClientDataLoaded()&&c.HasInventorySessionReservation()&&c.CharacterID()&&c.AccountID();}
class Custody final:public CrateBinding::CustodyContext {
 Client& client_;LiveItems::InventorySessionFence& fence_;std::shared_ptr<const Content> content_;
 bool Held(const LiveItems::Lease& l,LiveItems::InventorySessionFence& f)const{return &f==&fence_&&ClientReady(client_)&&l.actor.character_id==client_.CharacterID()&&l.actor.account_id==client_.AccountID()&&f.HeldFor(l.actor,l.epoch,l.session_key);}
 bool SourceItem(const LiveItems::InventoryRow& source,int32_t serial,LiveItems::InventorySessionFence& f)const{
  if(source.values.item_id!=62586||source.values.character_id!=client_.CharacterID()||!f.SourceMatches(source,serial))return false;
  const auto* item=client_.GetInv().GetItem(int16_t(source.values.slot_id));if(!item||!CratePlacement::Metadata(*item,174,7)||item->GetUnscaledItem()->Placeable!=content_->placement_mask)return false;
  uint32_t lo,hi;std::memcpy(&lo,&item->GetUnscaledItem()->HousingMinimumScale,4);std::memcpy(&hi,&item->GetUnscaledItem()->HousingMaximumScale,4);return lo==content_->minimum_scale_bits&&hi==content_->maximum_scale_bits;
 }
public:
 Custody(Client& c,LiveItems::InventorySessionFence& f,std::shared_ptr<const Content> s):client_(c),fence_(f),content_(std::move(s)){}
 std::array<uint8_t,32> ManifestDigest()const noexcept override{return content_->digest;}
 bool ReviewInteriorPlacementUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Request& r,const InteriorContent::PlacementReview& reviewed,const CrateBinding::Allocation* carried)const override{
  if(!Held(r.lease,f)||!HouseRuntime::ReviewedRoomForCurrentClient(reviewed.profile.context,reviewed.profile.room,reviewed.profile.profile_revision)||!reviewed.profile.context.access.effective.Allows(AccessPolicy::Capability::AddCrate)||!SourceItem(r.expected,r.verified_runtime_serial,f)||!Metadata(db,*content_))return false;
  const auto* item=client_.GetInv().GetItem(int16_t(r.expected.values.slot_id));return carried?carried->created_by_character_id==r.lease.actor.character_id&&item->GetHousingCrateID()==carried->binding.crate_id&&item->GetHousingEstateID()==carried->binding.native_estate_id&&carried->content_manifest_sha256==content_->digest:!item->GetHousingCrateID()&&item->GetHousingEstateID()==UINT32_MAX;
 }
 bool ReviewInteriorSceneUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const Scene::InteriorSnapshot& scene,const LiveItems::Stored& stored,const CrateBinding::Allocation& a)const override{
  return Held(l,f)&&scene.room_profile&&HouseRuntime::ReviewedRoomForCurrentClient(scene,*scene.room_profile,scene.profile_revision)&&scene.access.effective.Allows(AccessPolicy::Capability::Enter)&&stored.placement&&stored.row.values.item_id==62586&&stored.row.values.character_id==a.created_by_character_id&&a.created_by_character_id&&a.binding.parent_native_estate_id==scene.room_profile->native_estate_id&&a.content_manifest_sha256==content_->digest&&Metadata(db,*content_);
 }
 std::optional<ContentBinding::CrateProfile> ReviewParentSceneUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const CrateBinding::ContentFacts& facts,const Scene::InteriorSnapshot& room)const override{
  if(!Held(facts.lease,f)||!ReviewedCrateRuntime::ParentReadFacts(facts,room,{client_.CharacterID(),client_.AccountID()},{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())})||!Metadata(db,*content_))return {};
  return ContentBinding::CrateProfile{174,Hex(std::vector<uint8_t>(content_->digest.begin(),content_->digest.end())),true};
 }
 std::optional<ContentBinding::CrateProfile> ReviewContentUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const CrateBinding::ContentFacts& facts)const override{
  if(!Held(facts.lease,f)||!ReviewedCrateRuntime::ExteriorFacts(facts,{client_.CharacterID(),client_.AccountID()},{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())})||!f.PlacementContextMatches(facts.reservation.estate_id,facts.reservation.native_estate_id,facts.neighborhood_instance)||!Metadata(db,*content_))return {};
  if((facts.stage==CrateBinding::Stage::Preview||facts.stage==CrateBinding::Stage::Store)&&!SourceItem(facts.source,facts.runtime_serial,f))return {};
  return ContentBinding::CrateProfile{174,Hex(std::vector<uint8_t>(content_->digest.begin(),content_->digest.end())),true};
 }
 bool VerifiedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const CrateBinding::PlacementFacts& p)const override{
  if(!p.parent.placement||!zone)return false;CrateBinding::ContentFacts facts;facts.lease=p.parent.lease;facts.reservation=p.parent.placement->reservation;facts.source=p.parent.expected;facts.runtime_serial=p.parent.verified_runtime_serial;facts.pose=p.parent.placement->transform;facts.stage=CrateBinding::Stage::Store;facts.catalog_revision=p.catalog_revision;facts.neighborhood_instance=zone->GetInstanceID();facts.access=p.access;facts.plot=p.plot;facts.crate=p.crate;facts.region=p.region;
  return bool(ReviewContentUnderTransaction(db,f,facts));
 }
 bool ReparentReviewedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const CrateBinding::PlacementFacts& facts)const override{return facts.carried&&facts.carried->created_by_character_id==facts.parent.lease.actor.character_id&&VerifiedUnderTransaction(db,f,facts);}
 bool CatalogRelocationReviewedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const CrateBinding::PlacementFacts& facts)const override{
  if(!facts.carried||facts.carried->created_by_character_id!=facts.parent.lease.actor.character_id||!VerifiedUnderTransaction(db,f,facts))return false;
  const auto* item=client_.GetInv().GetItem(int16_t(facts.parent.expected.values.slot_id));return item&&item->GetHousingCrateID()==facts.carried->binding.crate_id&&item->GetHousingEstateID()==facts.carried->binding.native_estate_id;
 }
 bool ReviewPickupUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const LiveItems::Stored& stored,const CrateBinding::Allocation& a)const override{
  // Operate/OperateInterior already proves current physical route, locked
  // estate ancestry, fresh RemoveCrate ACL, current room assets and creator.
  // Rechecking a fixture's original estate here breaks relocated houses.
  return Held(l,f)&&stored.placement&&stored.row.values.item_id==62586&&stored.row.values.character_id==l.actor.character_id&&a.created_by_character_id==l.actor.character_id&&a.binding.item_template_id==62586&&a.binding.custody_id==stored.custody_id&&a.binding.parent_native_estate_id==stored.placement->reservation.native_estate_id&&a.content_manifest_sha256==content_->digest&&Metadata(db,*content_);
 }
};
class Reload final:public CrateBinding::ReloadPolicy {
 std::shared_ptr<const Content> content_;
public:
 explicit Reload(std::shared_ptr<const Content> s):content_(std::move(s)){}
 std::array<uint8_t,32> ManifestDigest()const noexcept override{return content_->digest;}
 bool ReviewedUnderTransaction(Database& db,const InventorySessions::Lease& l,const LiveItems::InventoryRow& row,const CrateBinding::Allocation& a)const override{
  // Preload already holds the real session and validates permanent carried
  // identity and retained members. Source-property access is not a reload grant.
  if(!DatabaseReady()||!l.active||!l.actor.character_id||!l.actor.account_id||row.values.character_id!=l.actor.character_id||row.values.item_id!=62586||a.binding.item_template_id!=62586||a.created_by_character_id!=l.actor.character_id||a.inventory_character_id!=l.actor.character_id||a.inventory_slot_id!=row.values.slot_id||a.content_manifest_sha256!=content_->digest||!Metadata(db,*content_))return false;
  auto actor=db.QueryDatabase("SELECT c.id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(l.actor.character_id)+" AND c.account_id="+std::to_string(l.actor.account_id)+" AND c.deleted_at IS NULL FOR UPDATE",false);return actor.Success()&&actor.RowCount()==1;
 }
};
std::string FileDigest(const char* path){std::ifstream file(path,std::ios::binary);if(!file)throw std::invalid_argument("Missing crate resource");SHA256_CTX c;SHA256_Init(&c);char b[65536];while(file.read(b,sizeof b)||file.gcount())SHA256_Update(&c,b,size_t(file.gcount()));if(file.bad())throw std::invalid_argument("Unreadable crate resource");std::array<uint8_t,32> sum;SHA256_Final(sum.data(),&c);std::string out;const char* h="0123456789abcdef";for(auto v:sum){out+=h[v>>4];out+=h[v&15];}return out;}
}
bool LoadAtSafeStartup(){
 if(initialized||!InventoryBridge::IsRegisteredZoneThread())return false;initialized=true;std::ifstream input(".crate-test.json",std::ios::binary);
 if(!input){std::error_code error;const bool present=std::filesystem::exists(".crate-test.json",error);return !present&&!error;}
 try{if(!DatabaseReady())return false;std::string raw;char b[4096];while(input.read(b,sizeof b)||input.gcount()){raw.append(b,size_t(input.gcount()));if(raw.size()>262144)return false;}if(input.bad())return false;auto parsed=CrateTest::Parse(raw);if(!parsed||FileDigest("housing-test-assets/furniture06.eqg")!=CrateGeometry::Original.archive_sha256||FileDigest("maps/base/neighborhood.map")!=parsed->map_sha256)return false;selected=std::make_shared<const Content>(std::move(*parsed));return true;}catch(...){return false;}
}
std::shared_ptr<const CrateBinding::ReloadPolicy> ReloadPolicy(const std::vector<std::shared_ptr<const HouseRuntime::PersonalTestSelection>>&){return selected?std::make_shared<const Reload>(selected):nullptr;}
std::unique_ptr<CrateBinding::CustodyContext> MakeForClient(Client& c,LiveItems::InventorySessionFence& f,std::shared_ptr<const HouseRuntime::PersonalTestSelection>){return selected&&ClientReady(c)?std::make_unique<Custody>(c,f,selected):nullptr;}
}
