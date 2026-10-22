#include "runtime_activation_db.h"
#include "npc_exit_collision.h"
#include "npc_native_frame_db.h"
#include "npc_item_metadata_db.h"
#include "npc_runtime_db.h"
#include "rent_state_db.h"
#include "crate_member_scope_db.h"
#include "inventory_row_db.h"
#include "item_payload_db.h"
#include <set>
#include "scene_service.h"
#include "vault_storage_db.h"
#include "crate_vault_members_db.h"
#include "crate_binding.h"
#include "interior_profile_db.h"
#include "interior_item_content_db.h"
#include "house_binding_service.h"
#include "custody_codec.h"
#include "house_item_scope_db.h"
#include "item_storage_db.h"
#include "catalog_content_db.h"
#include "catalog_service.h"
#include "../database.h"
#include <cmath>
#include <cstring>
namespace Housing::Scene {
using Error=LiveItems::Error;
namespace {
using namespace LiveItems;
using Bytes=std::vector<uint8_t>;
struct Failure{Error error;};
MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
        bool Key(const std::string& s){
            if(s.empty()||s.size()>96)return false;
            for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.'))return false;
            return true;
        }
        std::string Quote(Database& db,const std::string& s){
            return "'"+db.Escape(s)+"'";
        }
        Bytes Unhex(const char* p){
            if(!p)throw Failure{
                Error::DatabaseFailure
            };
            std::string s=p;
            if(s.size()%2||s.size()>std::max(size_t(2*CustodyCodec::MaximumPayload),2*RegionWire::MaximumPayload))throw Failure{
                Error::DatabaseFailure
            };
            Bytes b;
            auto n=[](char c)->uint8_t{
                if(c>='0'&&c<='9')return uint8_t(c-'0');
                if(c>='A'&&c<='F')return uint8_t(c-'A'+10);
                if(c>='a'&&c<='f')return uint8_t(c-'a'+10);
                throw Failure{
                    Error::DatabaseFailure
                };
            };
            for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t(n(s[i])*16+n(s[i+1])));
            return b;
        }
        struct Reader{
            const Bytes& b;
            size_t at=0;
            uint64_t N(size_t n){
                if(n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                uint64_t v=0;
                for(size_t i=0;i<n;++i)v|=uint64_t(b[at++])<<(8*i);
                return v;
            }
            std::string Text(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                std::string s(b.begin()+at,b.begin()+at+n);
                at+=n;
                return s;
            }
            Bytes Blob(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                Bytes out(b.begin()+at,b.begin()+at+n);
                at+=n;
                return out;
            }
        };
const EstateWire::Definition& Definition(const Housing::Catalog::Bundle& b,uint32_t id){for(const auto& d:b.definitions)if(d.id==id)return d;throw Failure{Error::UnsupportedItem};}
std::string Character(Database& db,uint32_t id){auto r=Q(db,"SELECT c.name FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(id)+" AND c.deleted_at IS NULL FOR UPDATE");if(r.RowCount()!=1||!r.begin()[0])throw Failure{Error::Unauthorized};return r.begin()[0];}
uint32_t Number(const char* value){if(!value)throw Failure{Error::DatabaseFailure};auto n=std::stoull(value);if(n>UINT32_MAX)throw Failure{Error::DatabaseFailure};return uint32_t(n);}
AccessPolicy::Acl Acl(Database& db,EstateId estate,uint64_t& revision){
 AccessPolicy::Acl acl;auto state=Q(db,"SELECT revision FROM housing_acl_state WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");
 if(!state.RowCount())return acl;if(state.RowCount()!=1||!(revision=std::stoull(state.begin()[0])))throw Failure{Error::DatabaseFailure};
 auto groups=Q(db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+std::to_string(estate)+" ORDER BY group_id FOR UPDATE");if(groups.RowCount()!=7)throw Failure{Error::DatabaseFailure};uint32_t i=0;for(auto r=groups.begin();r.operator!=(groups.end());++r){if(Number(r[0])!=i)throw Failure{Error::DatabaseFailure};acl.masks[i++]=Number(r[1]);}
 auto members=Q(db,"SELECT character_id,group_id FROM housing_acl_members WHERE estate_id="+std::to_string(estate)+" ORDER BY character_id FOR UPDATE");if(members.RowCount()>1000)throw Failure{Error::DatabaseFailure};for(auto r=members.begin();r.operator!=(members.end());++r){auto id=Number(r[0]),group=Number(r[1]);Character(db,id);if(group>5)throw Failure{Error::DatabaseFailure};acl.assignments.push_back({{AccessPolicy::SubjectKind::Character,id},uint8_t(group)});}
 auto special=Q(db,"SELECT subject_kind,subject_id,group_id FROM housing_acl_special_members WHERE estate_id="+std::to_string(estate)+" ORDER BY subject_kind,subject_id FOR UPDATE");if(special.RowCount()>9)throw Failure{Error::DatabaseFailure};for(auto r=special.begin();r.operator!=(special.end());++r){auto kind=Number(r[0]),group=Number(r[2]);if((kind!=1&&kind!=2)||group>5)throw Failure{Error::DatabaseFailure};acl.assignments.push_back({{kind==1?AccessPolicy::SubjectKind::GuildRank:AccessPolicy::SubjectKind::Fellowship,Number(r[1])},uint8_t(group)});}
 if(!AccessPolicy::ValidAcl(acl))throw Failure{Error::DatabaseFailure};return acl;
}
PlacementPolicy::Transform Pose(const Bytes& b){if(b.size()!=28)throw Failure{Error::DatabaseFailure};Reader r{b};std::array<float,7>a{};for(auto& f:a){uint32_t bits=r.N(4);std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw Failure{Error::DatabaseFailure};}return {a[0],a[1],a[2],a[3],{a[4],a[5],a[6]}};}
}
namespace {
ExteriorExitContext LockedExitGeometry(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,const HouseBinding::Binding& binding,const ExitContext& context,const Housing::Catalog::Bundle& bundle,const EstateWire::Definition& parent,const ContentBinding::HouseRoute& immutable){
 ExteriorExitContext out;out.parent=context;bool found=false;for(const auto&r:bundle.regions)if(r.id==parent.area_id){if(found)throw Failure{Error::Conflict};out.region=r;found=true;}if(!found)throw Failure{Error::UnsupportedItem};
 // A genuine NPC's physical sphere may cross its plot's native711a
 // containment envelope. Collect typed NPC actors across this same root,
 // without granting the viewer any access to another ownership generation.
 const auto catalog_key=Quote(db,binding.interior->catalog_key);
 auto roots=Q(db,"SELECT n.parent_native_id,n.neighborhood_id,r.definition_id,h.zone_id,h.zone_version,h.instance_id FROM housing_native_estates n JOIN housing_native_estates r ON r.native_id=n.parent_native_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id WHERE n.native_id="+std::to_string(context.native_plot_id)+" AND n.active=1 AND r.active=1 AND r.parent_native_id IS NULL AND n.catalog_key="+catalog_key+" AND r.catalog_key=n.catalog_key AND r.neighborhood_id=n.neighborhood_id FOR UPDATE");
 if(roots.RowCount()!=1)throw Failure{Error::Conflict};auto rootrow=roots.begin();const auto root_id=Number(rootrow[0]),neighborhood_id=Number(rootrow[1]);const auto& root_definition=Definition(bundle,Number(rootrow[2]));
 if(!root_id||!neighborhood_id||root_definition.type!=5||Number(rootrow[3])!=context.neighborhood.zone_id||Number(rootrow[4])!=context.neighborhood.zone_version||Number(rootrow[5])!=context.neighborhood.instance_id)throw Failure{Error::Conflict};
 const bool npc_columns=ContentBinding::ColumnsInstalled(db);
 if(npc_columns){
  // Include historical reservations: the final Return deletes live custody
  // before inventory ACK, while its old actor can still be published. Do not
  // borrow restart recovery or infer writer death for this read-only witness.
  auto writers=Q(db,"SELECT DISTINCT l.character_id FROM housing_live_item_leases l JOIN housing_live_placement_reservations r ON r.character_id=l.character_id JOIN housing_native_estates n ON n.native_id=r.native_estate_id WHERE l.pending_key IS NOT NULL AND r.content_binding_version IN (7,8) AND r.catalog_key="+catalog_key+" AND n.parent_native_id="+std::to_string(root_id)+" AND n.neighborhood_id="+std::to_string(neighborhood_id)+" ORDER BY l.character_id LIMIT 10001 FOR UPDATE");if(writers.RowCount()>10000)throw Failure{Error::UnsupportedItem};
  for(auto writer=writers.begin();writer.operator!=(writers.end());++writer){
   auto pending_npc=LiveItems::Service::PendingNpcPlacementUnderTransaction(db,true,lease,Number(writer[0]),fence);if(!pending_npc)throw Failure{pending_npc.error};if(!pending_npc.value)continue;const auto& reservation=*pending_npc.value;
   if(reservation.catalog_key!=binding.interior->catalog_key)continue;
   auto node=Q(db,"SELECT parent_native_id,neighborhood_id,definition_id FROM housing_native_estates WHERE native_id="+std::to_string(reservation.native_estate_id)+" AND catalog_key="+catalog_key+" FOR UPDATE");if(node.RowCount()!=1)throw Failure{Error::Conflict};auto nr=node.begin();const auto& definition=Definition(bundle,Number(nr[2]));
   if(Number(nr[1])==neighborhood_id&&nr[0]&&Number(nr[0])==root_id){if(definition.type!=4&&definition.type!=8)throw Failure{Error::Conflict};throw Failure{Error::PendingApplication};}
   if(definition.type!=2&&definition.type!=3&&definition.type!=4&&definition.type!=8)throw Failure{Error::Conflict};
  }
 }
 auto pending=Q(db,"SELECT l.character_id FROM housing_live_item_leases l JOIN housing_live_item_custody c ON c.owner_character_id=l.character_id WHERE c.estate_id="+std::to_string(context.estate_id)+" AND l.pending_key IS NOT NULL LIMIT 1 FOR UPDATE");if(pending.RowCount())throw Failure{Error::PendingApplication};
 // 10000 is an explicit fail-closed query-work budget, not an original
 // neighborhood or pet capacity. Fresh per-plot native caps remain unchanged.
 auto rows=Q(db,"SELECT c.custody_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.state,r.custody_id,r.character_id,"+ContentBinding::SelectColumns(db,"r.")+",HEX(r.source_row),HEX(c.catalog_blob),n.ownership_estate_id,n.definition_id,n.plot_id,n.active,n.neighborhood_id,n.parent_native_id,e.owner_kind,e.owner_id,e.plot_id,c.estate_id FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id LEFT JOIN housing_native_estates n ON n.native_id=r.native_estate_id AND n.catalog_key=r.catalog_key LEFT JOIN housing_current_estates e ON e.id=n.ownership_estate_id WHERE (c.estate_id="+std::to_string(context.estate_id)+(npc_columns?" OR (r.content_binding_version IN (7,8) AND r.catalog_key="+catalog_key+" AND n.parent_native_id="+std::to_string(root_id)+" AND n.neighborhood_id="+std::to_string(neighborhood_id)+")":"")+")"+HouseItemScope::ExteriorPredicate(db)+" ORDER BY c.custody_id LIMIT 10001 FOR UPDATE");if(rows.RowCount()>10000)throw Failure{Error::UnsupportedItem};
 for(auto row=rows.begin();row.operator!=(rows.end());++row){
  LiveItems::Stored stored;stored.custody_id=std::stoull(row[0]);stored.estate_id=Number(row[8]);const bool own=Number(row[7])==context.native_plot_id;

  if(!stored.custody_id||!row[2]||std::string(row[2])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::UnsupportedItem};stored.row=CustodyCodec::Decode(Unhex(row[3]));ItemPayload::ValidateHeld(db,stored.row,stored.custody_id);
  if(stored.row.values.character_id!=Number(row[1])||Number(row[13])!=stored.row.values.character_id||(own&&stored.estate_id!=context.estate_id)||!row[9]||std::string(row[9])!=binding.interior->catalog_key||(!std::stoull(row[10])||std::stoull(row[10])>context.catalog_revision)||Number(row[11])!=1||std::stoull(row[12])!=stored.custody_id)throw Failure{Error::Conflict};
  auto storage=ItemStorage::Locked(db,stored.custody_id);if(storage.stowed||CrateMemberScope::Locked(db,stored.custody_id))continue;
  const auto& plot_definition=own?parent:Definition(bundle,Number(row[19]));
  RegionWire::Region region=out.region;if(!own){bool matched=false;for(const auto& candidate:bundle.regions)if(candidate.id==plot_definition.area_id){if(matched)throw Failure{Error::Conflict};region=candidate;matched=true;}if(!matched)throw Failure{Error::UnsupportedItem};}
  if(Number(row[27])!=stored.estate_id||!row[18]||Number(row[18])!=stored.estate_id||Number(row[19])!=plot_definition.id||Number(row[21])!=1||Number(row[22])!=neighborhood_id||Number(row[23])!=root_id||!row[24]||!Number(row[25])||Number(row[26])!=Number(row[20])||!((Number(row[24])==1&&plot_definition.type==4)||(Number(row[24])==3&&plot_definition.type==8))||std::find(root_definition.children.begin(),root_definition.children.end(),plot_definition.id)==root_definition.children.end())throw Failure{Error::Conflict};
  LiveItems::Placed pose;pose.reservation={Number(row[4]),Number(row[7]),stored.estate_id,row[9],std::stoull(row[10])};pose.reservation.content_binding=ContentBinding::DecodeIdentity(row[14],row[15]);pose.pose_revision=std::stoull(row[5]);pose.transform=Pose(Unhex(row[6]));if(!pose.reservation.native_item_id||pose.reservation.native_item_id==UINT32_MAX||!pose.pose_revision)throw Failure{Error::Conflict};stored.placement=pose;stored.current_catalog_revision=context.catalog_revision;stored.storage_revision=storage.revision;
  auto meta=Q(db,"SELECT idfile,UNK227,UNK224,UNK228,UNK229,itemclass,bagslots,loregroup,nodrop FROM items WHERE id="+std::to_string(stored.row.values.item_id)+" FOR UPDATE");if(meta.RowCount()!=1||!meta.begin()[0])throw Failure{Error::UnsupportedItem};std::string model=meta.begin()[0];const auto* house=HouseGeometry::Lookup(stored.row.values.item_id,model);const auto* furniture=StaticFurniture::Lookup(stored.row.values.item_id,model);const auto* crate=CrateGeometry::Lookup(stored.row.values.item_id,Number(meta.begin()[1]),model);
  auto source=NpcItemMetadata::ReadUnderTransaction(db,stored.row.values.item_id);auto npc=source?NpcRuntime::Review(*source):std::nullopt;
  if(source&&!NpcItemMetadata::Ordinary(*source)&&!npc)throw Failure{Error::UnsupportedItem};
  const auto retained=pose.reservation.content_binding;
  if(npc){
   // Authenticate genuine source/custody/frame before consulting original
   // physical sphere authority. IgnoreCollisions flags alone are insufficient;
   // native711a's containment envelope is not physical collision geometry.
   if(house||furniture||crate||!retained||(retained->version!=ContentBinding::NpcClassVersion&&retained->version!=ContentBinding::GenericNpcClassVersion)||
      (own&&binding.location.kind==HouseBinding::Location::Kind::Custody&&stored.custody_id==binding.location.custody_id)||
      !fence.HeldFor(lease.actor,lease.epoch,lease.session_key)||!fence.NpcProfileMatchesUnderTransaction(db,lease,*npc))throw Failure{Error::Conflict};
   if(!row[16]||Unhex(row[16])!=CustodyCodec::Encode(stored.row)||!row[17])throw Failure{Error::Conflict};auto catalog=Unhex(row[17]);if(catalog.size()!=16)throw Failure{Error::Conflict};Reader catalog_reader{catalog};
   auto mr=meta.begin();for(size_t i=0;i<4;++i){if(!mr[5+i]||int32_t(catalog_reader.N(4))!=std::stoll(mr[5+i]))throw Failure{Error::Conflict};}if(std::stoll(mr[5])||std::stoll(mr[6]))throw Failure{Error::UnsupportedItem};
   const auto expected=ContentBinding::Current(db,binding.interior->catalog_key,bundle,pose.reservation.native_estate_id,context.neighborhood,stored.row.values.item_id);if(!ContentBinding::Same(*retained,expected))throw Failure{Error::Conflict};
   pose.npc_frame=NpcNativeFrame::ReadUnderTransaction(db,stored.custody_id,pose.reservation.native_item_id,pose.pose_revision,pose.transform);auto geometry=NpcRuntime::Geometry(*npc,pose.npc_frame,pose.transform);
   uint32_t minbits=Number(mr[3]),maxbits=Number(mr[4]);float minimum,maximum;std::memcpy(&minimum,&minbits,4);std::memcpy(&maximum,&maxbits,4);
   PlacementPolicy::ItemMetadata item{stored.row.values.item_id,npc->model_id,Number(mr[2]),minimum,maximum,false};PlacementPolicy::Context committed{pose.reservation.native_estate_id,plot_definition.type,region.id,{6,AccessPolicy::KnownCapabilities,true},true,false,true};
   if(!PlacementPolicy::Check(committed,item,geometry,region,pose.transform,PlacementPolicy::Operation::Add))throw Failure{Error::Conflict};
   if(!NpcExitCollision::SphereMode(*npc))throw Failure{Error::UnsupportedItem};
   out.blockers.push_back(NpcExitCollision::Bounds(*npc,pose.npc_frame,pose.transform));
   continue; // Authenticated original physical sphere, never unknown-class skip.
  }
  if(!own)throw Failure{Error::Conflict}; // Foreign rows must be genuine typed NPC actors.
  if(retained&&(retained->version==ContentBinding::NpcClassVersion||retained->version==ContentBinding::GenericNpcClassVersion))throw Failure{Error::Conflict};
  if(unsigned(bool(house))+unsigned(bool(furniture))+unsigned(bool(crate))!=1)throw Failure{Error::UnsupportedItem};
  out.blockers.push_back(ExteriorExit::Bounds(house?house->geometry:crate?crate->geometry:furniture->geometry,pose.transform));
  if(binding.location.kind==HouseBinding::Location::Kind::Custody&&stored.custody_id==binding.location.custody_id){
   if(!house||stored.row.values.item_id!=binding.item_template_id||stored.row.values.character_id!=context.creator_character_id||out.current_house)throw Failure{Error::Conflict};
   const auto expected=ContentBinding::Current(db,binding.interior->catalog_key,bundle,context.native_plot_id,context.neighborhood,binding.item_template_id,&immutable);if(!pose.reservation.content_binding||!ContentBinding::Same(*pose.reservation.content_binding,expected))throw Failure{Error::Conflict};out.current_house=std::move(stored);
  }
 }
 if(binding.location.kind==HouseBinding::Location::Kind::Custody&&!out.current_house){
  auto stored=Q(db,"SELECT owner_character_id,schema_descriptor,HEX(row_blob) FROM housing_live_item_custody WHERE custody_id="+std::to_string(binding.location.custody_id)+" FOR UPDATE");if(stored.RowCount()!=1||!stored.begin()[1]||std::string(stored.begin()[1])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::Conflict};auto raw=CustodyCodec::Decode(Unhex(stored.begin()[2]));if(raw.values.item_id!=binding.item_template_id||raw.values.character_id!=context.creator_character_id||Number(stored.begin()[0])!=context.creator_character_id||(!ItemStorage::Locked(db,binding.location.custody_id).stowed&&!CrateMemberScope::Locked(db,binding.location.custody_id)))throw Failure{Error::Conflict};
 }
 return out;
}
InteriorSnapshot LockedInterior(Database& db,const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,bool egress,bool include_contents,const Housing::InteriorContent::ProfilePolicy* room_policy=nullptr,const Snapshot* attached_parent=nullptr,const CrateBinding::CustodyContext* crates=nullptr,bool remote_anchor=false,uint64_t anchor_custody=0,bool management_view=false,bool strict_placed=false,AttachedRead read=AttachedRead::Storage,bool published_collision=false){
 if(read==AttachedRead::TransferConservation&&(!attached_parent||egress||remote_anchor||anchor_custody||management_view||strict_placed))throw Failure{Error::InvalidArgument};
 if(remote_anchor&&(!attached_parent||egress))throw Failure{Error::InvalidArgument};
 if(!source.zone_id||!source.instance_id||source.instance_id>32000||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))throw Failure{Error::InvalidArgument};
 if(!fence.HeldFor(l.actor,l.epoch,l.session_key))throw Failure{Error::UnheldFence};
 auto transaction=Q(db,"SELECT @@in_transaction");if(transaction.RowCount()!=1||Number(transaction.begin()[0])!=1)throw Failure{Error::InvalidArgument};
  auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_estates','instance_list','instance_list_player','housing_live_item_leases','character_data','account')");
  if(tables.RowCount()!=1||Number(tables.begin()[0])!=12)throw Failure{Error::DatabaseFailure};
  auto actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(l.actor.character_id)+" FOR UPDATE");
  if(actor.RowCount()!=1||Number(actor.begin()[0])!=l.actor.account_id||actor.begin()[1])throw Failure{Error::Unauthorized};Character(db,l.actor.character_id);
  auto lease=Q(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");
  if(lease.RowCount()!=1)throw Failure{Error::StaleLease};auto lr=lease.begin();
  if(Number(lr[0])!=l.actor.account_id||std::stoull(lr[1])!=l.epoch||!lr[2]||std::string(lr[2])!=l.session_key||Unhex(lr[3])!=Bytes(l.token.begin(),l.token.end())||Number(lr[4])!=1)throw Failure{Error::StaleLease};
  if(lr[5])throw Failure{Error::PendingApplication};
  auto houses=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id,i.native_estate_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision FROM housing_house_interiors i JOIN housing_house_instances h ON h.house_id=i.house_id JOIN housing_house_locations l ON l.house_id=h.house_id WHERE i.zone_id="+std::to_string(source.zone_id)+" AND i.instance_id="+std::to_string(source.instance_id)+" AND i.zone_version="+std::to_string(source.zone_version)+" FOR UPDATE");
  if(houses.RowCount()!=1)throw Failure{Error::NotFound};auto h=houses.begin();HouseBinding::Binding binding;
  binding.house_id=std::stoull(h[0]);binding.revision=std::stoull(h[1]);binding.item_template_id=Number(h[2]);
  ExitContext context;context.source=source;context.creator_character_id=Number(h[3]);
  if(!binding.house_id||!binding.revision||!binding.item_template_id||!context.creator_character_id||h[4])throw Failure{Error::Conflict};
  auto kind=Number(h[5]);
  if(kind==1){if(!h[6]||!h[7]||h[8]||!Number(h[6]))throw Failure{Error::Conflict};binding.location={HouseBinding::Location::Kind::Inventory,Number(h[6]),Number(h[7]),0};}
  else if(kind==2){if(h[6]||h[7]||!h[8]||!std::stoull(h[8]))throw Failure{Error::Conflict};binding.location={HouseBinding::Location::Kind::Custody,0,0,std::stoull(h[8])};}
  else throw Failure{Error::Conflict};
  // A contained house retains its interior for conservation and egress only.
  if(!egress&&binding.location.kind==HouseBinding::Location::Kind::Custody&&CrateMemberScope::Locked(db,binding.location.custody_id)&&read!=AttachedRead::TransferConservation)throw Failure{Error::Unauthorized};
  HouseBinding::Interior interior{Number(h[9]),Number(h[10]),Number(h[11]),uint16_t(Number(h[12])),uint8_t(Number(h[13])),h[14]?h[14]:"",std::stoull(h[15])};
  if(!interior.native_estate_id||interior.native_estate_id==UINT32_MAX||!Key(interior.catalog_key)||!interior.catalog_revision)throw Failure{Error::Conflict};binding.interior=interior;
  const auto key=Quote(db,interior.catalog_key);
  auto graph=Q(db,"SELECT n.definition_id,n.parent_native_id,n.neighborhood_id,p.definition_id,p.ownership_estate_id,p.plot_id,p.parent_native_id,root.definition_id FROM housing_native_estates n JOIN housing_native_estates p ON p.native_id=n.parent_native_id JOIN housing_native_estates root ON root.native_id=p.parent_native_id WHERE n.native_id="+std::to_string(interior.native_estate_id)+" AND n.active=1 AND p.active=1 AND root.active=1 AND root.parent_native_id IS NULL AND n.catalog_key="+key+" AND p.catalog_key=n.catalog_key AND root.catalog_key=n.catalog_key AND p.neighborhood_id=n.neighborhood_id AND root.neighborhood_id=n.neighborhood_id FOR UPDATE");
  if(graph.RowCount()!=1)throw Failure{Error::Conflict};auto g=graph.begin();
  if(Number(g[0])!=interior.definition_id)throw Failure{Error::Conflict};
  context.native_plot_id=Number(g[1]);const auto neighborhood=Number(g[2]),plot=Number(g[5]);context.estate_id=Number(g[4]);
  auto estate=Q(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(context.estate_id)+" FOR UPDATE");
  if(estate.RowCount()!=1||!Number(estate.begin()[1])||Number(estate.begin()[2])!=plot)throw Failure{Error::Conflict};auto owner_kind=Number(estate.begin()[0]);
  auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+key+" FOR UPDATE");
  if(catalog.RowCount()!=1||Number(catalog.begin()[2])!=20130510||std::stoull(catalog.begin()[0])!=interior.catalog_revision)throw Failure{Error::Conflict};auto cr=catalog.begin();context.catalog_revision=interior.catalog_revision;
  if(Number(cr[1])==2){auto name=Q(db,"SELECT DATABASE()");std::string database=name.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}else if(Number(cr[1])!=1)throw Failure{Error::UnsupportedItem};
  auto bundle=Housing::Catalog::DecodePayloads(Unhex(cr[3]),Unhex(cr[4]));const auto& child=Definition(bundle,interior.definition_id);const auto& parent=Definition(bundle,Number(g[3]));const auto& root=Definition(bundle,Number(g[7]));
  if(root.type!=5||child.zone_id!=source.zone_id||!((owner_kind==1&&parent.type==4&&child.type==3)||(owner_kind==3&&parent.type==8&&child.type==2))||std::find(parent.children.begin(),parent.children.end(),child.id)==parent.children.end()||std::find(root.children.begin(),root.children.end(),parent.id)==root.children.end())throw Failure{Error::UnsupportedItem};context.house_definition=child;
  auto route=Q(db,"SELECT n.zone_id,n.zone_version,n.instance_id,i.zone,i.version,i.never_expires,source.zone,source.version,source.never_expires FROM housing_neighborhoods n JOIN instance_list i ON i.id=n.instance_id JOIN instance_list source ON source.id="+std::to_string(source.instance_id)+" WHERE n.id="+std::to_string(neighborhood)+" FOR UPDATE");
  if(route.RowCount()!=1)throw Failure{Error::Conflict};auto rr=route.begin();auto instance=Number(rr[2]),version=Number(rr[1]);
  if(!instance||instance>32000||version>255||Number(rr[0])!=Number(rr[3])||version!=Number(rr[4])||Number(rr[5])!=1||Number(rr[6])!=source.zone_id||Number(rr[7])!=source.zone_version||Number(rr[8])!=1||root.zone_id!=Number(rr[0]))throw Failure{Error::Conflict};
  context.neighborhood={Number(rr[0]),uint8_t(version),uint16_t(instance)};
  std::optional<ContentBinding::HouseRoute> reviewed;
  if(attached_parent){
   if(egress||attached_parent->estate_id!=context.estate_id||attached_parent->native_plot_id!=context.native_plot_id||attached_parent->current_catalog_revision!=context.catalog_revision||attached_parent->route.zone_id!=context.neighborhood.zone_id||attached_parent->route.zone_version!=context.neighborhood.zone_version||attached_parent->route.instance_id!=context.neighborhood.instance_id)throw Failure{Error::Conflict};
   const Item* selected=nullptr;
   auto find=[&](const std::vector<Item>& items){for(const auto& item:items)if(item.house&&item.house->house_id==binding.house_id){if(selected)throw Failure{Error::Conflict};selected=&item;}};
   find(attached_parent->items);
   if(read==AttachedRead::TransferConservation)find(attached_parent->crate_contents);
   if(!selected||selected->house->revision!=binding.revision||selected->house->location.custody_id!=binding.location.custody_id)throw Failure{Error::Conflict};
   if(selected->contained_in_crate){
    // LockedExterior already authenticates the complete current crate graph.
    // Require this exact member and a placed, reviewed containing crate too.
    const auto& m=selected->crate_membership;
    if(read!=AttachedRead::TransferConservation||!selected->stored.stowed||!m||m->crate_id!=selected->contained_in_crate||m->origin_house_id||m->origin_native_estate_id!=context.native_plot_id)throw Failure{Error::Conflict};
    auto container=std::find_if(attached_parent->items.begin(),attached_parent->items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==m->crate_id;});
    if(container==attached_parent->items.end()||container->stored.stowed||container->contained_in_crate||container->crate->inventory_character_id||container->crate->binding.estate_id!=context.estate_id||container->crate->binding.parent_native_estate_id!=context.native_plot_id)throw Failure{Error::Conflict};
   }
   reviewed=remote_anchor?policy.ReviewedAnchorHouseUnderTransaction(db,fence,l,selected->stored,binding,child):policy.ReviewedAttachedInteriorUnderTransaction(db,fence,l,*attached_parent,*selected);
  }else{
   auto membership=Q(db,"SELECT charid FROM instance_list_player WHERE id="+std::to_string(source.instance_id)+" AND charid="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(membership.RowCount()!=1)throw Failure{Error::Unauthorized};
   reviewed=egress?policy.ReviewedExitUnderTransaction(db,fence,l,binding,context):policy.ReviewedInteriorUnderTransaction(db,fence,l,binding,context);
  }
  if(!reviewed||!reviewed->reviewed_server_route||reviewed->definition_id!=child.id||reviewed->zone_id!=source.zone_id||reviewed->zone_version!=source.zone_version)throw Failure{Error::Unauthorized};
  for(float value:reviewed->exit)if(!std::isfinite(value))throw Failure{Error::UnsupportedItem};
  InteriorSnapshot out;out.house=std::make_shared<const HouseBinding::Binding>(binding);out.context=context;if(attached_parent)out.exterior_source=attached_parent->route;
  if(!egress||strict_placed){
   if(!egress&&!RentState::MutableItem(db,context.estate_id,context.native_plot_id,binding.location.custody_id))throw Failure{Error::Unauthorized};
   auto read_tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations','items','housing_plot_definitions')");if(read_tables.RowCount()!=1||Number(read_tables.begin()[0])!=5)throw Failure{Error::DatabaseFailure};
   if(binding.location.kind!=HouseBinding::Location::Kind::Custody)throw Failure{Error::NotFound};
   if(strict_placed&&(ItemStorage::Locked(db,binding.location.custody_id).stowed||CrateMemberScope::Locked(db,binding.location.custody_id)))throw Failure{Error::Unauthorized};
   auto access=LoadCurrentEstateAccessUnderTransaction(db,context.estate_id,context.native_plot_id,l.actor);
   if(!access||!access.value.effective.Allows(AccessPolicy::Capability::Enter))throw Failure{Error::Unauthorized};out.access=access.value;
   auto placed=Q(db,"SELECT c.estate_id,c.owner_character_id,HEX(c.row_blob),r.native_estate_id,r.catalog_key,r.state,r.custody_id,r.content_binding_version,HEX(r.content_binding_digest),p.native_item_id FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE c.custody_id="+std::to_string(binding.location.custody_id)+" FOR UPDATE");
   if(placed.RowCount()!=1)throw Failure{Error::Conflict};auto row=placed.begin();
   if(Number(row[0])!=context.estate_id||Number(row[1])!=context.creator_character_id||Number(row[3])!=context.native_plot_id||!row[4]||std::string(row[4])!=interior.catalog_key||Number(row[5])!=1||std::stoull(row[6])!=binding.location.custody_id||Number(row[7])!=ContentBinding::HouseVersion||!Number(row[9])||Number(row[9])==UINT32_MAX)throw Failure{Error::Conflict};
   auto stored=CustodyCodec::Decode(Unhex(row[2]));if(stored.values.item_id!=binding.item_template_id||stored.values.character_id!=context.creator_character_id)throw Failure{Error::Conflict};
   const auto expected=ContentBinding::Current(db,interior.catalog_key,bundle,context.native_plot_id,context.neighborhood,binding.item_template_id,&*reviewed);
   const auto actual=ContentBinding::DecodeIdentity(row[7],row[8]);if(!actual||!ContentBinding::Same(*actual,expected))throw Failure{Error::Conflict};
   if(management_view){out.access.effective.capabilities&=RentState::ManagementCapabilities;return out;}
   if(published_collision){
    if(!include_contents||!room_policy||!HouseItemScope::Installed(db))throw Failure{Error::DatabaseFailure};
    auto publication=LiveItems::Service::NoPendingInteriorPublicationUnderTransaction(db,l,interior.native_estate_id,binding.house_id,fence);
    if(!publication)throw Failure{publication.error};if(!publication.value)throw Failure{Error::PendingApplication};
   }
   if(include_contents&&HouseItemScope::Installed(db)){
    if(room_policy){auto current=Housing::InteriorContent::ReadProfileUnderTransaction(db,l,source,fence,policy,*room_policy,attached_parent&&!remote_anchor?std::optional<Housing::InteriorContent::AttachedSelection>{{context.estate_id,binding.house_id,attached_parent->route,crates,read}}:std::nullopt,remote_anchor?std::optional<Housing::InteriorContent::AnchorSelection>{{context.estate_id,binding.house_id}}:std::nullopt);if(!current)throw Failure{current.error};if(!current.value.context.house||current.value.context.house->house_id!=binding.house_id||current.value.context.house->revision!=binding.revision)throw Failure{Error::Conflict};out.profile_revision=current.value.profile_revision;out.room_profile=std::make_shared<const Housing::InteriorContent::Profile>(std::move(current.value.room));}
    const uint64_t content_limit=out.room_profile?out.room_profile->definition.max_items:1000;

    const bool have_vault=VaultStorage::Installed(db);
    auto contents=Q(db,"SELECT scope.custody_id,scope.revision,c.estate_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),HEX(c.catalog_blob),p.custody_id,h.house_id FROM housing_house_item_scopes scope LEFT JOIN housing_live_item_custody c ON c.custody_id=scope.custody_id LEFT JOIN housing_live_placed_items p ON p.custody_id=c.custody_id LEFT JOIN housing_house_locations h ON h.custody_id=c.custody_id WHERE scope.house_id="+std::to_string(binding.house_id)+(anchor_custody?" AND scope.custody_id="+std::to_string(anchor_custody):std::string{})+(have_vault?" AND NOT EXISTS (SELECT 1 FROM housing_live_vault_items v WHERE v.custody_id=scope.custody_id)":"")+" ORDER BY scope.custody_id LIMIT "+std::to_string(content_limit+1001)+" FOR UPDATE");
    if(contents.RowCount()>content_limit+1000)throw Failure{Error::UnsupportedItem};
    for(auto row=contents.begin();row!=contents.end();++row){
     InteriorContent content;auto& stored=content.stored;
     if(!row[0]||!row[1]||!row[2]||!row[3]||!row[4]||!row[5]||!row[6])throw Failure{Error::DatabaseFailure};
     stored.custody_id=std::stoull(row[0]);content.scope_revision=std::stoull(row[1]);stored.estate_id=Number(row[2]);
     if(!stored.custody_id||!content.scope_revision||stored.custody_id==binding.location.custody_id||row[8])throw Failure{Error::Conflict};
     if(std::string(row[4])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::UnsupportedItem};
     stored.row=CustodyCodec::Decode(Unhex(row[5]));ItemPayload::ValidateHeld(db,stored.row,stored.custody_id);if(stored.row.values.character_id!=Number(row[3]))throw Failure{Error::Conflict};
     // Retain historical custody estate/creator, independently of CURRENT
     // inherited parent access already proven above. No ownership migration.
     auto bytes=Unhex(row[6]);if(bytes.size()!=16)throw Failure{Error::DatabaseFailure};Reader cat{bytes};
     stored.catalog={int32_t(cat.N(4)),int32_t(cat.N(4)),int32_t(cat.N(4)),int32_t(cat.N(4))};
     auto metadata=Q(db,"SELECT itemclass,bagslots,loregroup,nodrop FROM items WHERE id="+std::to_string(stored.row.values.item_id)+" FOR UPDATE");
     if(metadata.RowCount()!=1||stored.catalog.item_class!=0||stored.catalog.bag_slots!=0)throw Failure{Error::UnsupportedItem};
     const std::array<int32_t,4> expected_catalog{stored.catalog.item_class,stored.catalog.bag_slots,stored.catalog.lore_group,stored.catalog.no_drop};
     for(size_t i=0;i<expected_catalog.size();++i)if(!metadata.begin()[i]||std::stoll(metadata.begin()[i])!=expected_catalog[i])throw Failure{Error::Conflict};
     for(const auto& model:HouseGeometry::Records)if(model.item_id==stored.row.values.item_id)throw Failure{Error::UnsupportedItem};
     // A placed room actor needs its own reviewed room content/pose identity.
     // Do not interpret exterior v1/v2 as an interior placement witness.
     content.creator_name=Character(db,stored.row.values.character_id);
     Bytes retained_pose;
     if(row[7]){
      auto pending=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(stored.row.values.character_id)+" FOR UPDATE");if(pending.RowCount()>1)throw Failure{Error::DatabaseFailure};if(pending.RowCount()&&pending.begin()[0]){if(published_collision)throw Failure{Error::PendingApplication};continue;}
      if(!room_policy)throw Failure{Error::UnsupportedItem};
      if(!out.room_profile)throw Failure{Error::UnsupportedItem};
      auto placed=Q(db,"SELECT p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.state,r.custody_id,r.character_id,HEX(r.source_row),r.content_binding_version,r.content_binding_digest,t.house_id,t.source_zone,t.source_version,t.source_instance,t.content_version,HEX(t.content_digest),t.house_revision,t.profile_revision FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id JOIN housing_interior_placement_reservations t ON t.native_item_id=r.native_item_id WHERE p.custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
      if(placed.RowCount()!=1)throw Failure{Error::Conflict};auto pr=placed.begin();for(size_t i=0;i<21;++i)if(i!=11&&i!=12&&!pr[i])throw Failure{Error::DatabaseFailure};
      if(!Number(pr[0])||Number(pr[0])==UINT32_MAX||!std::stoull(pr[1])||Number(pr[3])!=interior.native_estate_id||Number(pr[4])!=stored.estate_id||!Key(pr[5])||!std::stoull(pr[6])||Number(pr[7])!=1||std::stoull(pr[8])!=stored.custody_id||Number(pr[9])!=stored.row.values.character_id||Unhex(pr[10])!=CustodyCodec::Encode(stored.row)||pr[11]||pr[12]||std::stoull(pr[13])!=binding.house_id||Number(pr[14])!=source.zone_id||Number(pr[15])!=source.zone_version||Number(pr[16])!=source.instance_id||Number(pr[17])!=Housing::InteriorContent::Version)throw Failure{Error::Conflict};
      auto tag_digest=Unhex(pr[18]);if(tag_digest.size()!=SHA256_DIGEST_LENGTH)throw Failure{Error::Conflict};std::array<uint8_t,SHA256_DIGEST_LENGTH> expected_digest{};std::copy(tag_digest.begin(),tag_digest.end(),expected_digest.begin());auto input=Housing::InteriorContent::ItemInputUnderTransaction(db,*out.room_profile,stored.row.values.item_id,expected_digest);if(!input)throw Failure{input.error};if(input.value.npc&&!fence.NpcProfileMatchesUnderTransaction(db,l,*input.value.npc))throw Failure{Error::Conflict};auto fingerprint=Housing::InteriorContent::Build(input.value);if(!fingerprint||Unhex(pr[18])!=Bytes(fingerprint.value.digest.begin(),fingerprint.value.digest.end()))throw Failure{Error::Conflict};
      retained_pose=Unhex(pr[2]);auto pose=Pose(retained_pose);auto npc_frame=NpcNativeFrame::ReadUnderTransaction(db,stored.custody_id,Number(pr[0]),std::stoull(pr[1]),pose);if(input.value.npc)input.value.geometry=NpcRuntime::Geometry(*input.value.npc,npc_frame,pose);else if(npc_frame)throw Failure{Error::Conflict};
      // Geometry validation only after actual Enter authorization. Synthetic
      // geometry bits confer no Add/Move rights to a viewer.
      PlacementPolicy::Context geometry{interior.native_estate_id,out.room_profile->definition.type,out.room_profile->region.id,{6,AccessPolicy::KnownCapabilities,true},true,false,true}; // Committed scene geometry, no admission.
      if(!ItemStorage::Locked(db,stored.custody_id).stowed&&!Housing::InteriorContent::CheckPlacement(input.value.room,geometry,input.value.item,input.value.geometry,pose,PlacementPolicy::Operation::Add))throw Failure{Error::Conflict};
      stored.placement=LiveItems::Placed{{Number(pr[0]),interior.native_estate_id,stored.estate_id,pr[5],std::stoull(pr[6])},pose,std::stoull(pr[1]),npc_frame};stored.current_catalog_revision=context.catalog_revision;
      if(!std::stoull(pr[19])||!std::stoull(pr[20]))throw Failure{Error::Conflict};LiveItems::InteriorReservation witness;witness.placement=stored.placement->reservation;witness.house_id=binding.house_id;witness.house_revision=std::stoull(pr[19]);witness.profile_revision=std::stoull(pr[20]);witness.source=source;witness.content_digest=fingerprint.value.digest;content.room=std::move(witness);
     }
     auto storage=ItemStorage::Locked(db,stored.custody_id);
     if(storage.revision&&(!stored.placement||storage.native_estate_id!=interior.native_estate_id))throw Failure{Error::Conflict};
     stored.storage_revision=storage.revision;stored.stowed=storage.stowed;
     if(CrateGeometry::TemplateDefinition(stored.row.values.item_id)){
      if(!crates||!stored.placement||!content.room)throw Failure{Error::UnsupportedItem};
      auto allocation=CrateBinding::ReadPlacedUnderTransaction(db,stored);if(!allocation)throw Failure{allocation.error};
      if(!crates->ReviewInteriorSceneUnderTransaction(db,fence,l,out,stored,allocation.value))throw Failure{Error::Unauthorized};
      content.crate=std::make_shared<const CrateBinding::Allocation>(std::move(allocation.value));
     }
     auto membership=CrateMemberScope::Locked(db,stored.custody_id,true);
     if(membership){if(!membership->origin_house_id||membership->origin_house_id!=binding.house_id||membership->origin_native_estate_id!=interior.native_estate_id||!stored.stowed||content.crate||retained_pose.empty()||membership->origin_pose!=retained_pose)throw Failure{Error::Conflict};content.contained_in_crate=membership->crate_id;content.crate_membership=std::make_shared<const CrateMemberScope::Membership>(*membership);out.crate_contents.push_back(std::move(content));}
     else out.contents.push_back(std::move(content));
    }
   }
   out.vault_items=VaultStorage::Items(db,context.estate_id,interior.native_estate_id,binding.house_id);
   for(const auto& c:out.contents)if(c.crate&&!c.stored.stowed){auto vaults=CrateVault::Read(db,*c.crate);out.crate_vault_items.insert(out.crate_vault_items.end(),vaults.begin(),vaults.end());}
   if(out.room_profile&&out.contents.size()>out.room_profile->definition.max_items)throw Failure{Error::Conflict};
   for(const auto& member:out.crate_contents)if(std::none_of(out.contents.begin(),out.contents.end(),[&](const auto& entry){return entry.crate&&entry.crate->binding.crate_id==member.contained_in_crate;}))throw Failure{Error::Conflict};
   return out;
  }
  if(management_view){
   auto access=LoadCurrentEstateAccessUnderTransaction(db,context.estate_id,context.native_plot_id,l.actor);
   if(!access||!access.value.effective.Allows(AccessPolicy::Capability::Enter))throw Failure{Error::Unauthorized};
   out.access=access.value;out.access.effective.capabilities&=RentState::ManagementCapabilities;return out; // No membership/exit effect and no room contents.
  }
  // Public neighborhood egress does not require retaining Enter rights to the
  // house. Neither the native body nor prior membership selects this route.
  auto exit_geometry=LockedExitGeometry(db,l,fence,binding,context,bundle,parent,*reviewed);
  auto source_bound_exit=policy.ReviewedExteriorPointUnderTransaction(db,fence,l,binding,exit_geometry,*reviewed);
  if(!fence.HeldFor(l.actor,l.epoch,l.session_key))throw Failure{Error::UnheldFence};
  if(!source_bound_exit)throw Failure{Error::UnsupportedItem};for(float value:*source_bound_exit)if(!std::isfinite(value)||std::abs(value)>65536)throw Failure{Error::UnsupportedItem};
  auto destination=Q(db,"SELECT charid FROM instance_list_player WHERE id="+std::to_string(instance)+" AND charid="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(destination.RowCount()>1)throw Failure{Error::DatabaseFailure};
  if(!destination.RowCount())Q(db,"INSERT INTO instance_list_player(id,charid) VALUES ("+std::to_string(instance)+","+std::to_string(l.actor.character_id)+")");
  out.exit=Admission{binding.house_id,interior.native_estate_id,context.neighborhood,*source_bound_exit,*source_bound_exit};return out;
}
}
Service::Service(std::unique_ptr<Database> db,bool enabled,const HouseScenePolicy* houses,const Housing::InteriorContent::ProfilePolicy* room_profile,const CrateBinding::CustodyContext* crates):database_(std::move(db)),enabled_(enabled),houses_(houses),room_profile_(room_profile),crates_(crates){}
Service::~Service()=default;
LiveItems::Result<Admission> Service::ExitHouse(const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence){
 auto result=ResolveInterior(l,source,fence,true);if(!result)return {{},result.error};if(!result.value.exit)return {{},Error::Conflict};return {*result.value.exit,Error::None};
}
LiveItems::Result<InteriorSnapshot> Service::ReadInteriorUnderTransaction(Database& db,const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy){
 if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 try{return {LockedInterior(db,l,source,fence,policy,false,false),Error::None};}
 catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadInteriorSceneUnderTransaction(Database& db,const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy& room,const CrateBinding::CustodyContext* crates){
 if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 try{return {LockedInterior(db,l,source,fence,policy,false,true,&room,nullptr,crates),Error::None};}
 catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadPublishedInteriorCollisionUnderTransaction(Database& db,const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy& room,const CrateBinding::CustodyContext* crates){
 if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 try{return {LockedInterior(db,l,source,fence,policy,false,true,&room,nullptr,crates,false,0,false,false,AttachedRead::Storage,true),Error::None};}
 catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadInterior(const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence){return ResolveInterior(l,source,fence,false);}
LiveItems::Result<InteriorSnapshot> Service::ReadInteriorManagementUnderTransaction(Database& db,const LiveItems::Lease& lease,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& houses){
 try{return {LockedInterior(db,lease,source,fence,houses,true,false,nullptr,nullptr,nullptr,false,0,true,true),Error::None};}
 catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadInteriorForManagement(const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence){return ResolveInterior(l,source,fence,true,true);}
LiveItems::Result<InteriorSnapshot> Service::ResolveInterior(const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,bool egress,bool management_view){
 std::lock_guard<std::mutex> guard(mutex_);
 if(!enabled_)return {{},Error::Disabled};
 if(!source.zone_id||!source.instance_id||source.instance_id>32000||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};
 if(!houses_)return {{},Error::Unauthorized};
 if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 auto& db=*database_;bool active=false;
 try{
  Q(db,"START TRANSACTION");active=true;
  auto out=LockedInterior(db,l,source,fence,*houses_,egress,true,room_profile_,nullptr,crates_,false,0,management_view);Q(db,"COMMIT");active=false;return {std::move(out),Error::None};
 }catch(const Failure& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Snapshot> Service::ReadScene(const LiveItems::Lease& l,EstateId estate,LiveItems::InventorySessionFence& fence){
 return ReadCurrent(l,estate,fence,nullptr);
}
LiveItems::Result<Snapshot> Service::AdmitHouse(const LiveItems::Lease& l,EstateId estate,const EntryRequest& request,LiveItems::InventorySessionFence& fence){
 if(!request.native_plot_id||request.native_plot_id==UINT32_MAX||!request.native_item_id||request.native_item_id==UINT32_MAX||!request.house_id||!request.house_revision||!request.custody_id)return {{},Error::InvalidArgument};
 return ReadCurrent(l,estate,fence,&request);
}
namespace {
Snapshot LockedExterior(Database& db,const LiveItems::Lease& l,EstateId estate,LiveItems::InventorySessionFence& fence,const EntryRequest* entry,const HouseScenePolicy* houses,const InteriorSnapshot* occupied=nullptr,const CrateBinding::CustodyContext* crates=nullptr,const LobbyRequest* lobby=nullptr,uint32_t lobby_guild=0,uint64_t anchor_custody=0,bool remote_house=false){
 if(remote_house&&!anchor_custody)throw Failure{Error::InvalidArgument};
 if(anchor_custody&&(entry||occupied||lobby))throw Failure{Error::InvalidArgument};
 if(!crates&&houses)crates=houses->Crates();
 if(!estate||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))throw Failure{Error::InvalidArgument};
 if(!fence.HeldFor(l.actor,l.epoch,l.session_key))throw Failure{Error::UnheldFence};
 auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1)throw Failure{Error::InvalidArgument};
 auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_acl_state','housing_acl_groups','housing_acl_members','housing_acl_special_members','housing_estates','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_plot_definitions','instance_list','character_data','account','guild_members','guilds','items','housing_live_item_leases','housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations')");if(tables.RowCount()!=1||Number(tables.begin()[0])!=19)throw Failure{Error::DatabaseFailure};
 auto actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(actor.RowCount()!=1||Number(actor.begin()[0])!=l.actor.account_id||actor.begin()[1])throw Failure{Error::Unauthorized};Character(db,l.actor.character_id);
 auto lease=Q(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(lease.RowCount()!=1)throw Failure{Error::StaleLease};auto lr=lease.begin();auto token=Unhex(lr[3]);if(Number(lr[0])!=l.actor.account_id||std::stoull(lr[1])!=l.epoch||std::string(lr[2])!=l.session_key||token!=Bytes(l.token.begin(),l.token.end())||Number(lr[4])!=1)throw Failure{Error::StaleLease};if(lr[5])throw Failure{Error::PendingApplication};
 auto domain=Q(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");if(domain.RowCount()!=1)throw Failure{Error::NotFound};auto dr=domain.begin();uint32_t owner_kind=Number(dr[0]),owner=Number(dr[1]),plot=Number(dr[2]);if(!owner||(owner_kind!=1&&owner_kind!=3))throw Failure{Error::Unauthorized};
 auto nodes=Q(db,"SELECT native_id,catalog_key,definition_id,parent_native_id,neighborhood_id FROM housing_native_estates WHERE active=1 AND ownership_estate_id="+std::to_string(estate)+" AND plot_id="+std::to_string(plot)+" FOR UPDATE");if(nodes.RowCount()!=1)throw Failure{Error::NotFound};auto nr=nodes.begin();Snapshot out;out.estate_id=estate;out.native_plot_id=Number(nr[0]);std::string key=nr[1];uint32_t definition_id=Number(nr[2]),parent_id=Number(nr[3]),neighborhood=Number(nr[4]);
 auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,key)+" FOR UPDATE");if(catalog.RowCount()!=1||Number(catalog.begin()[2])!=20130510)throw Failure{Error::UnsupportedItem};auto cr=catalog.begin();uint64_t catalog_revision=std::stoull(cr[0]);out.current_catalog_revision=catalog_revision;if(!catalog_revision)throw Failure{Error::DatabaseFailure};if(Number(cr[1])==2){auto n=Q(db,"SELECT DATABASE()");std::string name=n.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}else if(Number(cr[1])!=1)throw Failure{Error::UnsupportedItem};auto bundle=Housing::Catalog::DecodePayloads(Unhex(cr[3]),Unhex(cr[4]));const auto& definition=Definition(bundle,definition_id);if((owner_kind==1&&definition.type!=4)||(owner_kind==3&&definition.type!=8))throw Failure{Error::UnsupportedItem};
 auto root=Q(db,"SELECT definition_id FROM housing_native_estates WHERE native_id="+std::to_string(parent_id)+" AND active=1 AND parent_native_id IS NULL AND catalog_key="+Quote(db,key)+" AND neighborhood_id="+std::to_string(neighborhood)+" FOR UPDATE");if(root.RowCount()!=1)throw Failure{Error::InvalidArgument};const auto& root_definition=Definition(bundle,Number(root.begin()[0]));if(root_definition.type!=5||std::find(root_definition.children.begin(),root_definition.children.end(),definition.id)==root_definition.children.end())throw Failure{Error::UnsupportedItem};
 auto route=Q(db,"SELECT h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires,p.neighborhood_id FROM housing_neighborhoods h JOIN instance_list i ON i.id=h.instance_id JOIN housing_plot_definitions p ON p.id="+std::to_string(plot)+" WHERE h.id="+std::to_string(neighborhood)+" FOR UPDATE");if(route.RowCount()!=1)throw Failure{Error::UnheldFence};auto rr=route.begin();uint32_t instance=Number(rr[2]);if(!instance||instance>32000||Number(rr[0])!=Number(rr[3])||Number(rr[1])!=Number(rr[4])||Number(rr[5])!=1||Number(rr[6])!=neighborhood||root_definition.zone_id!=Number(rr[0]))throw Failure{Error::UnheldFence};out.route={Number(rr[0]),uint8_t(Number(rr[1])),uint16_t(instance)};if(lobby){
  if(!entry||occupied||owner_kind!=3||owner!=lobby_guild||!houses||!houses->LobbySourceUnderTransaction(db,fence,l,*lobby,lobby_guild))throw Failure{Error::Unauthorized};
 }else if(occupied){
  if(entry||!occupied->house||!occupied->house->interior||occupied->context.estate_id!=estate||occupied->context.native_plot_id!=out.native_plot_id||occupied->context.catalog_revision!=catalog_revision||occupied->context.neighborhood.zone_id!=out.route.zone_id||occupied->context.neighborhood.zone_version!=out.route.zone_version||occupied->context.neighborhood.instance_id!=out.route.instance_id)throw Failure{Error::Conflict};
 }else if(!anchor_custody&&!fence.PlacementContextMatches(estate,out.native_plot_id,out.route.instance_id))throw Failure{Error::UnheldFence};
 AccessPolicy::TrustedActor trusted;trusted.character_id=l.actor.character_id;trusted.authenticated=trusted.feature_allowed=true;auto membership=Q(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(membership.RowCount()>1)throw Failure{Error::DatabaseFailure};if(membership.RowCount()){trusted.guild_id=Number(membership.begin()[0]);auto rank=AccessPolicy::NativeGuildRankFromServer(Number(membership.begin()[1]));if(!rank)throw Failure{Error::Unauthorized};trusted.guild_rank=*rank;auto current_guild=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(trusted.guild_id)+" FOR UPDATE");if(current_guild.RowCount()!=1||(trusted.guild_rank==1&&Number(current_guild.begin()[0])!=l.actor.character_id))throw Failure{Error::Unauthorized};}
 AccessPolicy::ResolvedPlot resolved;resolved.estate_id=out.native_plot_id;resolved.type=owner_kind==1?AccessPolicy::PlotType::Personal:AccessPolicy::PlotType::Guild;std::string owner_name;if(owner_kind==1){resolved.owner_character_id=owner;owner_name=Character(db,owner);}else{resolved.guild_id=owner;auto guild=Q(db,"SELECT name FROM guilds WHERE id="+std::to_string(owner)+" FOR UPDATE");if(guild.RowCount()!=1||!guild.begin()[0])throw Failure{Error::Unauthorized};owner_name=guild.begin()[0];}
 auto acl=Acl(db,estate,out.acl_revision);auto access=AccessPolicy::Resolve(resolved,trusted,&acl);if(!access||!access->Allows(AccessPolicy::Capability::Enter))throw Failure{Error::Unauthorized};out.access=*access;RentState::Restrict(db,estate,out.native_plot_id,out.access);
 const RegionWire::Region* region=nullptr;for(const auto& r:bundle.regions)if(r.id==definition.area_id)region=&r;if(!region)throw Failure{Error::UnsupportedItem};out.definition=definition;out.region=*region;
 auto pending=Q(db,"SELECT l.character_id FROM housing_live_item_leases l JOIN housing_live_item_custody c ON c.owner_character_id=l.character_id WHERE c.estate_id="+std::to_string(estate)+" AND l.pending_key IS NOT NULL LIMIT 1 FOR UPDATE");if(pending.RowCount())throw Failure{Error::PendingApplication};
 auto rows=Q(db,"SELECT c.custody_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),HEX(c.catalog_blob),p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.state,r.custody_id,r.character_id,"+ContentBinding::SelectColumns(db,"r.")+" FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE c.estate_id="+std::to_string(estate)+HouseItemScope::ExteriorPredicate(db)+(lobby?" AND c.custody_id="+std::to_string(entry->custody_id):anchor_custody?" AND c.custody_id="+std::to_string(anchor_custody):std::string{})+" ORDER BY c.custody_id LIMIT 1001 FOR UPDATE");if(rows.RowCount()>1000)throw Failure{Error::UnsupportedItem};
 std::set<uint64_t> carried_room_crates;
 for(auto row=rows.begin();row.operator!=(rows.end());++row){
  // Picking up a loaded room crate detaches its members from house scope.
  // Their retained room reservations are not yard furniture. Authenticate the
  // complete carried graph before omitting it from this read-only yard view.
  const auto custody=std::stoull(row[0]);
  auto retained_room=CrateMemberScope::Locked(db,custody,true);
  if(retained_room&&retained_room->origin_house_id){
   if(!crates)throw Failure{Error::Conflict};
   if(!carried_room_crates.count(retained_room->crate_id)){
    auto location=Q(db,"SELECT character_id,slot_id FROM housing_crate_locations WHERE crate_id="+std::to_string(retained_room->crate_id)+" AND location_kind=1 AND custody_id IS NULL FOR UPDATE");
    if(location.RowCount()!=1||!location.begin()[0]||!location.begin()[1])throw Failure{Error::Conflict};
    auto inventory=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(Number(location.begin()[0]))+" AND slot_id="+std::to_string(Number(location.begin()[1]))+" FOR UPDATE");
    if(inventory.RowCount()!=1)throw Failure{Error::Conflict};
    auto carried=CrateBinding::ReadCarriedUnderTransaction(db,InventoryRowDB::Decode(inventory.begin()));if(!carried)throw Failure{carried.error};
    const auto& allocation=carried.value;const auto& binding=allocation.binding;
    if(binding.crate_id!=retained_room->crate_id||binding.estate_id!=estate||binding.parent_native_estate_id!=retained_room->origin_native_estate_id||binding.catalog_key!=key||binding.catalog_revision!=catalog_revision||allocation.packed||allocation.content_manifest_sha256!=crates->ManifestDigest())throw Failure{Error::Conflict};
    auto origin=Q(db,"SELECT native_estate_id FROM housing_house_interiors WHERE house_id="+std::to_string(retained_room->origin_house_id)+" FOR UPDATE");
    if(origin.RowCount()!=1||Number(origin.begin()[0])!=binding.parent_native_estate_id)throw Failure{Error::Conflict};
    carried_room_crates.insert(binding.crate_id);
   }
   continue;
  }
  Item item;auto& s=item.stored;s.custody_id=custody;s.estate_id=estate;if(std::string(row[2])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::UnsupportedItem};s.row=CustodyCodec::Decode(Unhex(row[3]));ItemPayload::ValidateHeld(db,s.row,s.custody_id);auto npc_source=NpcItemMetadata::ReadUnderTransaction(db,s.row.values.item_id);auto npc=npc_source?NpcRuntime::Review(*npc_source):std::nullopt;if(npc_source&&!NpcItemMetadata::Ordinary(*npc_source)&&!npc)throw Failure{Error::UnsupportedItem};if(npc&&!fence.NpcProfileMatchesUnderTransaction(db,l,*npc))throw Failure{Error::Conflict};uint32_t creator=Number(row[1]);if(s.row.values.character_id!=creator||Number(row[14])!=creator||Number(row[8])!=out.native_plot_id||Number(row[9])!=estate||std::string(row[10])!=key||Number(row[12])!=1||std::stoull(row[13])!=s.custody_id)throw Failure{Error::DatabaseFailure};auto cb=Unhex(row[4]);Reader catreader{cb};if(cb.size()!=16)throw Failure{Error::DatabaseFailure};s.catalog={int32_t(catreader.N(4)),int32_t(catreader.N(4)),int32_t(catreader.N(4)),int32_t(catreader.N(4))};auto meta=Q(db,"SELECT idfile,UNK224,UNK228,UNK229,itemclass,bagslots,loregroup,nodrop,UNK227 FROM items WHERE id="+std::to_string(s.row.values.item_id)+" FOR UPDATE");const auto* supported=!npc&&meta.RowCount()==1&&meta.begin()[0]?StaticFurniture::Lookup(s.row.values.item_id,std::string(meta.begin()[0])):nullptr;const auto* house=meta.RowCount()==1&&meta.begin()[0]?HouseGeometry::Lookup(s.row.values.item_id,std::string(meta.begin()[0])):nullptr;const auto* crate=CrateGeometry::TemplateDefinition(s.row.values.item_id)&&meta.RowCount()==1&&meta.begin()[0]?CrateGeometry::Lookup(s.row.values.item_id,Number(meta.begin()[8]),meta.begin()[0]):nullptr;if(!npc&&!supported&&(!house||!houses)&&(!crate||!crates))throw Failure{Error::UnsupportedItem};if((house&&crate)||(crate&&!crates)||(npc&&(house||crate)))throw Failure{Error::UnsupportedItem};auto mr=meta.begin();if(std::stoll(mr[4])!=s.catalog.item_class||std::stoll(mr[5])!=s.catalog.bag_slots||std::stoll(mr[6])!=s.catalog.lore_group||std::stoll(mr[7])!=s.catalog.no_drop)throw Failure{Error::UnsupportedItem};uint32_t minbits=Number(mr[2]),maxbits=Number(mr[3]);float minimum,maximum;std::memcpy(&minimum,&minbits,4);std::memcpy(&maximum,&maxbits,4);LiveItems::Placed placed;placed.reservation={Number(row[5]),out.native_plot_id,estate,key,std::stoull(row[11])};placed.reservation.content_binding=ContentBinding::DecodeIdentity(row[15],row[16]);if((npc&&!placed.reservation.content_binding)||(!placed.reservation.content_binding&&placed.reservation.catalog_revision!=catalog_revision))throw Failure{Error::Conflict};placed.pose_revision=std::stoull(row[6]);placed.transform=Pose(Unhex(row[7]));placed.npc_frame=NpcNativeFrame::ReadUnderTransaction(db,s.custody_id,placed.reservation.native_item_id,placed.pose_revision,placed.transform);if(!placed.pose_revision||!placed.reservation.native_item_id||placed.reservation.native_item_id==UINT32_MAX)throw Failure{Error::DatabaseFailure};PlacementPolicy::ItemMetadata metadata{s.row.values.item_id,npc?npc->model_id:house?house->model_id:crate?crate->model_id:supported->model_id,Number(mr[1]),minimum,maximum,bool(supported),house?(HouseTemplates::Type(s.row.values.item_id)):crate?7u:0u};if(!npc&&placed.npc_frame)throw Failure{Error::Conflict};auto geometry=npc?NpcRuntime::Geometry(*npc,placed.npc_frame,placed.transform):house?house->geometry:crate?crate->geometry:supported->geometry;
 std::optional<ContentBinding::HouseRoute> house_route;
 std::optional<ContentBinding::CrateProfile> crate_profile;
 if(house){
  if(!placed.reservation.content_binding||placed.reservation.content_binding->version!=ContentBinding::HouseVersion)throw Failure{Error::UnsupportedItem};
  auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=3)throw Failure{Error::DatabaseFailure};
  auto linked=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.location_kind,l.custody_id,i.native_estate_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision,n.parent_native_id,n.definition_id,n.active,n.catalog_key,instance.zone,instance.version,instance.never_expires FROM housing_house_locations l LEFT JOIN housing_house_instances h ON h.house_id=l.house_id LEFT JOIN housing_house_interiors i ON i.house_id=h.house_id LEFT JOIN housing_native_estates n ON n.native_id=i.native_estate_id LEFT JOIN instance_list instance ON instance.id=i.instance_id WHERE l.location_kind=2 AND l.custody_id="+std::to_string(s.custody_id)+" FOR UPDATE");
  if(linked.RowCount()!=1)throw Failure{Error::Conflict};auto h=linked.begin();
  if(!h[0]||!h[1]||!std::stoull(h[0])||!std::stoull(h[1])||Number(h[2])!=s.row.values.item_id||Number(h[3])!=creator||h[4]||Number(h[5])!=2||std::stoull(h[6])!=s.custody_id||!h[7]||!h[8]||!h[9]||!h[10]||!h[11]||!h[12]||!h[13]||!h[14]||!h[15]||!h[16]||!h[17]||!h[18]||!h[19]||!h[20])throw Failure{Error::Conflict};
  const auto native_house=Number(h[7]),house_definition=Number(h[8]),zone=Number(h[9]),instance=Number(h[10]),version=Number(h[11]);
  if(!native_house||native_house==UINT32_MAX||!instance||instance>32000||version>255||std::string(h[12])!=key||std::stoull(h[13])!=catalog_revision||Number(h[14])!=out.native_plot_id||Number(h[15])!=house_definition||Number(h[16])!=1||std::string(h[17])!=key||Number(h[18])!=zone||Number(h[19])!=version||Number(h[20])!=1)throw Failure{Error::Conflict};
  const auto& child=Definition(bundle,house_definition);
  if(child.type!=metadata.house_definition_type||child.zone_id!=zone||std::find(definition.children.begin(),definition.children.end(),child.id)==definition.children.end())throw Failure{Error::UnsupportedItem};
  auto binding=std::make_shared<HouseBinding::Binding>();binding->house_id=std::stoull(h[0]);binding->revision=std::stoull(h[1]);binding->item_template_id=s.row.values.item_id;binding->location={HouseBinding::Location::Kind::Custody,0,0,s.custody_id};binding->interior=HouseBinding::Interior{native_house,house_definition,zone,uint16_t(instance),uint8_t(version),key,catalog_revision};
  s.placement=placed;s.current_catalog_revision=catalog_revision;
  if(lobby){
   if(child.type!=2||definition.type!=8)throw Failure{Error::Unauthorized};
   house_route=houses->ReviewedLobbyRouteUnderTransaction(db,fence,l,s,*binding,child,*lobby,lobby_guild);
  }else if(occupied){
   if(binding->house_id==occupied->house->house_id){
    if(binding->revision!=occupied->house->revision||binding->location.custody_id!=occupied->house->location.custody_id)throw Failure{Error::Conflict};
    house_route=houses->ReviewedInteriorUnderTransaction(db,fence,l,*binding,occupied->context);
   }else{
    // The occupied host was independently authenticated above. Other house
    // tokens may be retained in this same yard's storage after a swap. Review
    // their immutable content against that genuine physical source; never
    // admit a second placed house or invent neighborhood position authority.
    if(!ItemStorage::Locked(db,s.custody_id).stowed)throw Failure{Error::Conflict};
    auto content=occupied->context;content.creator_character_id=creator;content.house_definition=child;
    house_route=houses->ReviewedInteriorUnderTransaction(db,fence,l,*binding,content);
   }
  }else house_route=remote_house?houses->ReviewedAnchorHouseUnderTransaction(db,fence,l,s,*binding,child):houses->ReviewedRouteUnderTransaction(db,fence,l,s,*binding,child);
  if(!house_route)throw Failure{Error::Unauthorized};item.house=std::move(binding);
 }
 if(crate){
  if(!crates||!placed.reservation.content_binding||placed.reservation.content_binding->version!=ContentBinding::CrateVersion)throw Failure{Error::UnsupportedItem};
  s.placement=placed;s.current_catalog_revision=catalog_revision;
  auto allocation=CrateBinding::ReadPlacedUnderTransaction(db,s);if(!allocation)throw Failure{allocation.error};
  const auto& child=Definition(bundle,allocation.value.binding.definition_id);
  if(child.type!=7||std::find(definition.children.begin(),definition.children.end(),child.id)==definition.children.end())throw Failure{Error::UnsupportedItem};
  CrateBinding::ContentFacts facts;facts.lease=l;facts.reservation=placed.reservation;facts.source=s.row;facts.pose=placed.transform;facts.stage=CrateBinding::Stage::Read;facts.catalog_revision=catalog_revision;facts.neighborhood_instance=out.route.instance_id;facts.access={OwnerKind(owner_kind),owner,plot,out.acl_revision,out.access};facts.plot=definition;facts.crate=child;facts.region=*region;
  crate_profile=occupied?crates->ReviewParentSceneUnderTransaction(db,fence,facts,*occupied):crates->ReviewContentUnderTransaction(db,fence,facts);
  if(!crate_profile||crate_profile->definition_id!=child.id||!crate_profile->reviewed_server_content||crates->ManifestDigest()!=allocation.value.content_manifest_sha256||Unhex(crate_profile->content_manifest_sha256.c_str())!=Bytes(allocation.value.content_manifest_sha256.begin(),allocation.value.content_manifest_sha256.end()))throw Failure{Error::Unauthorized};
  item.crate=std::make_shared<CrateBinding::Allocation>(std::move(allocation.value));
 }

if(placed.reservation.content_binding){auto identity=ContentBinding::Current(db,key,bundle,out.native_plot_id,out.route,s.row.values.item_id,house_route?&*house_route:nullptr,crate_profile?&*crate_profile:nullptr);if(!ContentBinding::Same(*placed.reservation.content_binding,identity))throw Failure{Error::Conflict};}PlacementPolicy::Context context{out.native_plot_id,definition.type,region->id,{6,0x1fff,true},true,false,true}; // Committed scene geometry only.
if(!ItemStorage::Locked(db,s.custody_id).stowed&&!PlacementPolicy::Check(context,metadata,geometry,*region,placed.transform,PlacementPolicy::Operation::Add))throw Failure{Error::UnsupportedItem};s.placement=placed;s.current_catalog_revision=catalog_revision;item.creator_name=Character(db,creator);item.estate_owner_name=owner_name;item.metadata=metadata;item.geometry=geometry;auto storage=ItemStorage::Locked(db,s.custody_id);if(storage.revision&&storage.native_estate_id!=out.native_plot_id)throw Failure{Error::Conflict};s.storage_revision=storage.revision;s.stowed=storage.stowed;
 auto member=CrateMemberScope::Locked(db,s.custody_id);
 if(member){if(item.crate||!storage.revision||!storage.stowed||member->origin_native_estate_id!=out.native_plot_id||member->origin_pose!=Unhex(row[7]))throw Failure{Error::Conflict};item.contained_in_crate=member->crate_id;item.crate_membership=std::make_shared<const CrateMemberScope::Membership>(*member);out.crate_contents.push_back(std::move(item));}
 else out.items.push_back(std::move(item));}
 out.vault_items=VaultStorage::Items(db,estate,out.native_plot_id);
 for(const auto& c:out.items)if(c.crate&&!c.stored.stowed){auto vaults=CrateVault::Read(db,*c.crate);out.crate_vault_items.insert(out.crate_vault_items.end(),vaults.begin(),vaults.end());}
 // Retained contents may belong to a placed crate or its exact carried
 // inventory identity. Carried contents disappear from this plot's visible
 // snapshot, while their immutable custody/pose and crate membership persist.
 std::set<uint64_t> carried_crates;
 for(const auto& member:out.crate_contents){
  auto container=std::find_if(out.items.begin(),out.items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==member.contained_in_crate;});
  if(container!=out.items.end()){if(container->stored.stowed)throw Failure{Error::Conflict};continue;}
  if(carried_crates.count(member.contained_in_crate))continue;
  if(!crates)throw Failure{Error::Conflict};
  auto location=Q(db,"SELECT character_id,slot_id FROM housing_crate_locations WHERE crate_id="+std::to_string(member.contained_in_crate)+" AND location_kind=1 AND custody_id IS NULL FOR UPDATE");
  if(location.RowCount()!=1||!location.begin()[0]||!location.begin()[1])throw Failure{Error::Conflict};
  auto inventory=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(Number(location.begin()[0]))+" AND slot_id="+std::to_string(Number(location.begin()[1]))+" FOR UPDATE");
  if(inventory.RowCount()!=1)throw Failure{Error::Conflict};
  auto carried=CrateBinding::ReadCarriedUnderTransaction(db,InventoryRowDB::Decode(inventory.begin()));if(!carried)throw Failure{carried.error};
  const auto& allocation=carried.value;const auto& binding=allocation.binding;
  if(binding.crate_id!=member.contained_in_crate||binding.estate_id!=estate||binding.parent_native_estate_id!=out.native_plot_id||binding.catalog_key!=key||binding.catalog_revision!=catalog_revision||allocation.packed||allocation.content_manifest_sha256!=crates->ManifestDigest())throw Failure{Error::Conflict};
  carried_crates.insert(binding.crate_id);
 }
 out.crate_contents.erase(std::remove_if(out.crate_contents.begin(),out.crate_contents.end(),[&](const auto& member){return carried_crates.count(member.contained_in_crate)!=0;}),out.crate_contents.end());
 if(occupied&&std::none_of(out.items.begin(),out.items.end(),[&](const auto& item){return item.house&&item.house->house_id==occupied->house->house_id;}))throw Failure{Error::Conflict};
 if(entry){
  if(!RentState::PlotActive(db,estate,out.native_plot_id))throw Failure{Error::Unauthorized};
  if(!houses||entry->native_plot_id!=out.native_plot_id)throw Failure{Error::Unauthorized};
  const Item* selected=nullptr;
  for(const auto& item:out.items)if(item.stored.placement&&item.stored.placement->reservation.native_item_id==entry->native_item_id){if(selected)throw Failure{Error::Conflict};selected=&item;}
  if(!selected||!selected->house||!selected->house->interior)throw Failure{Error::NotFound};
  if(selected->stored.stowed||selected->contained_in_crate)throw Failure{Error::Unauthorized};
  const auto& h=*selected->house;const auto& i=*h.interior;
  if(h.house_id!=entry->house_id||h.revision!=entry->house_revision||selected->stored.custody_id!=entry->custody_id)throw Failure{Error::Conflict};
  if(!RentState::HouseActive(db,selected->stored.custody_id))throw Failure{Error::Unauthorized};
  auto admitted=lobby?houses->ReviewedLobbyRouteUnderTransaction(db,fence,l,selected->stored,h,Definition(bundle,i.definition_id),*lobby,lobby_guild):houses->ReviewedEntryUnderTransaction(db,fence,l,out,*selected);
  if(!admitted||!admitted->reviewed_server_route||admitted->zone_id!=i.zone_id||admitted->zone_version!=i.zone_version||admitted->definition_id!=i.definition_id)throw Failure{Error::Unauthorized};
  // Require exactly the immutable route already used to validate the current
  // placed content identity, including spawn/exit and original asset digests.
  auto identity=ContentBinding::Current(db,key,bundle,out.native_plot_id,out.route,h.item_template_id,&*admitted);
  if(!selected->stored.placement->reservation.content_binding||!ContentBinding::Same(*selected->stored.placement->reservation.content_binding,identity))throw Failure{Error::Conflict};
  auto engine=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='instance_list_player'");
  if(engine.RowCount()!=1||!engine.begin()[0]||std::string(engine.begin()[0])!="InnoDB")throw Failure{Error::DatabaseFailure};
  auto membership=Q(db,"SELECT charid FROM instance_list_player WHERE id="+std::to_string(i.instance_id)+" AND charid="+std::to_string(l.actor.character_id)+" FOR UPDATE");
  if(membership.RowCount()>1)throw Failure{Error::DatabaseFailure};
  if(!membership.RowCount())Q(db,"INSERT INTO instance_list_player(id,charid) VALUES ("+std::to_string(i.instance_id)+","+std::to_string(l.actor.character_id)+")");
  out.admission=Admission{h.house_id,i.native_estate_id,{i.zone_id,i.zone_version,i.instance_id},admitted->spawn,admitted->exit};
 }
 return out;
}
}
LiveItems::Result<InteriorSnapshot> Service::ReadAnchorAttachedInteriorUnderTransaction(Database& db,const LiveItems::Lease& l,EstateId estate,uint64_t house_id,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy){
 if(!estate||!house_id)return {{},Error::InvalidArgument};
 try{
  auto house=Q(db,"SELECT custody_id FROM housing_house_locations WHERE house_id="+std::to_string(house_id)+" AND location_kind=2 FOR UPDATE");
  if(house.RowCount()!=1||!house.begin()[0])throw Failure{Error::NotFound};
  auto parent=LockedExterior(db,l,estate,fence,nullptr,&policy,nullptr,nullptr,nullptr,0,std::stoull(house.begin()[0]),true);
  if(parent.items.size()!=1||!parent.items[0].house||parent.items[0].house->house_id!=house_id||parent.items[0].stored.stowed||parent.items[0].contained_in_crate||!parent.items[0].house->interior)throw Failure{Error::Conflict};
  const auto& target=*parent.items[0].house->interior;
  return {LockedInterior(db,l,{target.zone_id,target.zone_version,target.instance_id},fence,policy,false,false,nullptr,&parent,nullptr,true),Error::None};
 }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<Snapshot> Service::ReadTrophyYardSourceUnderTransaction(Database& db,const LiveItems::Lease& l,EstateId estate,uint64_t custody,LiveItems::InventorySessionFence& fence,const HouseScenePolicy* houses,const CrateBinding::CustodyContext* crates){
 if(!estate||!custody)return {{},Error::InvalidArgument};
 try{if(!RentState::PlotActive(db,estate))return {{},Error::Unauthorized};return {LockedExterior(db,l,estate,fence,nullptr,houses,nullptr,crates,nullptr,0,custody),Error::None};}catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadTrophySourceUnderTransaction(Database& db,const LiveItems::Lease& l,EstateId estate,uint64_t house_id,uint64_t custody,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy& profile,const CrateBinding::CustodyContext* crates){
 if(!estate||!house_id||!custody)return {{},Error::InvalidArgument};
 try{
  auto house=Q(db,"SELECT custody_id FROM housing_house_locations WHERE house_id="+std::to_string(house_id)+" AND location_kind=2 FOR UPDATE");if(house.RowCount()!=1||!house.begin()[0])throw Failure{Error::NotFound};
  auto parent=LockedExterior(db,l,estate,fence,nullptr,&policy,nullptr,crates,nullptr,0,std::stoull(house.begin()[0]),true);
  if(parent.items.size()!=1||!parent.items[0].house||parent.items[0].house->house_id!=house_id||parent.items[0].stored.stowed||parent.items[0].contained_in_crate||!parent.items[0].house->interior)throw Failure{Error::Conflict};
  const auto& destination=*parent.items[0].house->interior;
  return {LockedInterior(db,l,{destination.zone_id,destination.zone_version,destination.instance_id},fence,policy,false,true,&profile,&parent,crates,true,custody),Error::None};
 }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<LobbyAdmission> Service::AdmitOwnGuildHallFromLobby(const LiveItems::Lease& l,const LobbyRequest& request,LiveItems::InventorySessionFence& fence){
 std::lock_guard<std::mutex> guard(mutex_);
 if(!enabled_)return {{},Error::Disabled};
 if(!houses_||request.source.zone_id!=344||!request.door_id||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};
 if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 auto& db=*database_;bool active=false;
 try{
  Q(db,"START TRANSACTION");active=true;
  auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('character_data','account','housing_live_item_leases','guild_members','guilds','housing_estates','housing_native_estates','housing_house_instances','housing_house_locations','housing_live_item_custody','housing_live_placed_items')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=11)throw Failure{Error::DatabaseFailure};
  auto actor=Q(db,"SELECT c.account_id,c.deleted_at,a.id FROM character_data c LEFT JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(actor.RowCount()!=1||Number(actor.begin()[0])!=l.actor.account_id||actor.begin()[1]||!actor.begin()[2])throw Failure{Error::Unauthorized};
  auto lease=Q(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(lease.RowCount()!=1)throw Failure{Error::StaleLease};auto lr=lease.begin();if(Number(lr[0])!=l.actor.account_id||std::stoull(lr[1])!=l.epoch||std::string(lr[2])!=l.session_key||Unhex(lr[3])!=Bytes(l.token.begin(),l.token.end())||Number(lr[4])!=1)throw Failure{Error::StaleLease};if(lr[5])throw Failure{Error::PendingApplication};
  LobbyAdmission result;
  auto member=Q(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");if(member.RowCount()>1)throw Failure{Error::DatabaseFailure};
  if(member.RowCount()){
   result.guild_id=Number(member.begin()[0]);auto rank=AccessPolicy::NativeGuildRankFromServer(Number(member.begin()[1]));if(!rank||!AccessPolicy::PositiveIdentity(result.guild_id))throw Failure{Error::Unauthorized};
   auto guild=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(result.guild_id)+" FOR UPDATE");if(guild.RowCount()!=1||(*rank==1&&Number(guild.begin()[0])!=l.actor.character_id))throw Failure{Error::Unauthorized};
  }
  if(!houses_->LobbySourceUnderTransaction(db,fence,l,request,result.guild_id))throw Failure{Error::Unauthorized};
  if(!result.guild_id){Q(db,"COMMIT");active=false;return {result,Error::None};}
  // Candidate IDs are not authority. LockedExterior re-locks/revalidates the
  // actual domain, canonical type8/type2 graph, placed custody, house revision,
  // retained content identity, Enter ACL and reviewed permanent destination.
  Q(db,"SELECT id FROM housing_current_estates WHERE owner_kind=3 AND owner_id="+std::to_string(result.guild_id)+" ORDER BY id FOR UPDATE");
  std::string visible=HouseItemScope::ExteriorPredicate(db);
  if(ItemStorage::Installed(db))visible+=" AND NOT EXISTS (SELECT 1 FROM housing_live_item_storage st WHERE st.custody_id=c.custody_id AND st.stowed=1)";
  if(CrateMemberScope::Installed(db))visible+=" AND NOT EXISTS (SELECT 1 FROM housing_crate_members cm WHERE cm.custody_id=c.custody_id)";
  auto candidate=Q(db,"SELECT e.id,n.native_id,p.native_item_id,h.house_id,h.revision,c.custody_id FROM housing_current_estates e JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_live_item_custody c ON c.estate_id=e.id JOIN housing_house_locations loc ON loc.custody_id=c.custody_id AND loc.location_kind=2 JOIN housing_house_instances h ON h.house_id=loc.house_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id WHERE e.owner_kind=3 AND e.owner_id="+std::to_string(result.guild_id)+visible+" ORDER BY e.id,h.house_id LIMIT 2 FOR UPDATE");
  if(candidate.RowCount()>1)throw Failure{Error::Conflict};
  if(!candidate.RowCount()){Q(db,"COMMIT");active=false;return {result,Error::None};}
  auto row=candidate.begin();EntryRequest entry{Number(row[1]),Number(row[2]),std::stoull(row[3]),std::stoull(row[4]),std::stoull(row[5])};
  auto scene=LockedExterior(db,l,Number(row[0]),fence,&entry,houses_,nullptr,crates_,&request,result.guild_id);
  if(!scene.admission)throw Failure{Error::Conflict};result.admission=scene.admission;
  // Policy checks the actual server source/door/range again before commit.
  if(!houses_->LobbySourceUnderTransaction(db,fence,l,request,result.guild_id))throw Failure{Error::Unauthorized};
  Q(db,"COMMIT");active=false;return {std::move(result),Error::None};
 }catch(const Failure& f){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadAttachedInteriorUnderTransaction(Database& db,const LiveItems::Lease& l,EstateId estate,uint64_t house_id,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy* room_policy,bool contents,const CrateBinding::CustodyContext* crates,AttachedRead read){
 if(!house_id||!estate||(contents&&!room_policy))return {{},Error::InvalidArgument};
 try{
  auto parent=LockedExterior(db,l,estate,fence,nullptr,&policy,nullptr,crates);
  const Item* selected=nullptr;
  auto find=[&](const std::vector<Item>& items){for(const auto& item:items)if(item.house&&item.house->house_id==house_id){if(selected)throw Failure{Error::Conflict};selected=&item;}};
  find(parent.items);if(read==AttachedRead::TransferConservation)find(parent.crate_contents);
  if(!selected||!selected->house->interior)return {{},Error::NotFound};const auto& i=*selected->house->interior;
  return {LockedInterior(db,l,{i.zone_id,i.zone_version,i.instance_id},fence,policy,false,contents,room_policy,&parent,crates,false,0,false,false,read),Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadCurrentAttachedInteriorUnderTransaction(Database& db,const LiveItems::Lease& l,uint64_t house_id,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy* room_policy,const CrateBinding::CustodyContext* crates){
 if(!house_id||!room_policy)return {{},Error::InvalidArgument};if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||Number(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  // Candidate only: the full attached read below authenticates the lease and
  // locks the current graph/house in its existing order, rejecting a moved host.
  auto host=Q(db,"SELECT c.estate_id FROM housing_house_locations l JOIN housing_live_item_custody c ON c.custody_id=l.custody_id WHERE l.house_id="+std::to_string(house_id)+" AND l.location_kind=2");if(host.RowCount()!=1||!host.begin()[0])return {{},Error::NotFound};
  return ReadAttachedInteriorUnderTransaction(db,l,Number(host.begin()[0]),house_id,fence,policy,room_policy,true,crates);
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadCurrentAttachedInterior(const LiveItems::Lease& l,uint64_t house_id,LiveItems::InventorySessionFence& fence){
 std::lock_guard<std::mutex> lock(mutex_);if(!enabled_||!database_)return {{},Error::Disabled};if(!houses_||!room_profile_)return {{},Error::Unauthorized};bool active=false;
 try{auto& db=*database_;Q(db,"START TRANSACTION");active=true;auto out=ReadCurrentAttachedInteriorUnderTransaction(db,l,house_id,fence,*houses_,room_profile_,crates_);if(!out){Q(db,"ROLLBACK");active=false;return out;}Q(db,"COMMIT");active=false;return out;}
 catch(...){if(active)database_->QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadAttachedInterior(const LiveItems::Lease& l,EstateId estate,uint64_t house_id,LiveItems::InventorySessionFence& fence){
 std::lock_guard<std::mutex> lock(mutex_);if(!enabled_||!database_)return {{},Error::Disabled};if(!houses_||!room_profile_)return {{},Error::Unauthorized};bool active=false;
 try{auto& db=*database_;Q(db,"START TRANSACTION");active=true;auto out=ReadAttachedInteriorUnderTransaction(db,l,estate,house_id,fence,*houses_,room_profile_,true,crates_);if(!out){Q(db,"ROLLBACK");return out;}Q(db,"COMMIT");active=false;return out;}
 catch(...){if(active)database_->QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Snapshot> Service::ReadParentSceneUnderTransaction(Database& db,const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const CrateBinding::CustodyContext* crates){
 try{auto occupied=LockedInterior(db,l,source,fence,policy,false,false);auto out=LockedExterior(db,l,occupied.context.estate_id,fence,nullptr,&policy,&occupied,crates);return {std::move(out),Error::None};}
 catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Snapshot> Service::ReadParentScene(const LiveItems::Lease& l,Route source,LiveItems::InventorySessionFence& fence){
 std::lock_guard<std::mutex> lock(mutex_);if(!enabled_||!database_)return {{},Error::Disabled};if(!houses_)return {{},Error::Unauthorized};bool active=false;
 try{auto& db=*database_;Q(db,"START TRANSACTION");active=true;auto out=ReadParentSceneUnderTransaction(db,l,source,fence,*houses_,crates_);if(!out){Q(db,"ROLLBACK");return out;}Q(db,"COMMIT");active=false;return out;}
 catch(...){if(active)database_->QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<AnchorDestination> Service::ResolveAnchor(const LiveItems::Lease& l,
 const AnchorRequest& request,LiveItems::InventorySessionFence& fence,bool admit,const AnchorDestination* expected){
 std::lock_guard<std::mutex> guard(mutex_);
 if(!enabled_)return {{},Error::Disabled};
 if((request.item_id!=50874&&request.item_id!=52584&&request.item_id!=52585)||
    !request.caster.character_id||!request.caster.account_id||!request.owner.character_id||!request.owner.account_id||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))return {{},Error::InvalidArgument};
 const bool owner_is_caster=request.owner.character_id==request.caster.character_id&&request.owner.account_id==request.caster.account_id;
 const bool owner_is_recipient=request.owner.character_id==l.actor.character_id&&request.owner.account_id==l.actor.account_id;
 if((!owner_is_caster&&!owner_is_recipient)||(request.item_id==50874&&!owner_is_caster))return {{},Error::Unauthorized};
 if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};
 if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 auto& db=*database_;bool active=false;
 try{
  Q(db,"START TRANSACTION");active=true;
  // Same recipient/session authority as LockedExterior, before candidate locks.
  auto actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(l.actor.character_id)+" FOR UPDATE");
  if(actor.RowCount()!=1||Number(actor.begin()[0])!=l.actor.account_id||actor.begin()[1])throw Failure{Error::Unauthorized};
  auto lease=Q(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(l.actor.character_id)+" FOR UPDATE");
  if(lease.RowCount()!=1)throw Failure{Error::StaleLease};auto lr=lease.begin();
  if(Number(lr[0])!=l.actor.account_id||std::stoull(lr[1])!=l.epoch||!lr[2]||std::string(lr[2])!=l.session_key||Unhex(lr[3])!=Bytes(l.token.begin(),l.token.end())||Number(lr[4])!=1)throw Failure{Error::StaleLease};
  if(lr[5])throw Failure{Error::PendingApplication};
  auto caster=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(request.caster.character_id)+" FOR UPDATE");
  if(caster.RowCount()!=1||Number(caster.begin()[0])!=request.caster.account_id||caster.begin()[1])throw Failure{Error::Unauthorized};
  auto owning_actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(request.owner.character_id)+" FOR UPDATE");
  if(owning_actor.RowCount()!=1||Number(owning_actor.begin()[0])!=request.owner.account_id||owning_actor.begin()[1])throw Failure{Error::Unauthorized};
  AnchorDestination result;result.caster=request.caster;result.owner=request.owner;result.item_id=request.item_id;result.caster_name=Character(db,request.caster.character_id);
  // Anchor admission independently verifies the recipient's current guild
  // subject before the common ACL loader can grant implicit rank1 access.
  // This also protects this new operation when merged ahead of shared loader
  // hardening, and never treats a deleted guild or stale leader row as owner.
  auto current_guild=[&](uint32_t character){
   auto member=Q(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+std::to_string(character)+" FOR UPDATE");
   if(member.RowCount()>1)throw Failure{Error::DatabaseFailure};if(!member.RowCount())return uint32_t(0);
   const auto id=Number(member.begin()[0]);auto rank=AccessPolicy::NativeGuildRankFromServer(Number(member.begin()[1]));
   if(!rank||!AccessPolicy::PositiveIdentity(id))throw Failure{Error::Unauthorized};
   auto guild=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(id)+" FOR UPDATE");
   if(guild.RowCount()!=1||(*rank==1&&Number(guild.begin()[0])!=character))throw Failure{Error::Unauthorized};return id;
  };
  auto recipient_guild=current_guild(l.actor.character_id);
  uint32_t owner=request.owner.character_id;
  if(request.item_id==50874){
   const auto guild=request.owner.character_id==l.actor.character_id?recipient_guild:current_guild(request.owner.character_id);
   if(!guild)throw Failure{Error::Unauthorized};result.guild_id=owner=guild;
  }
  // A personal anchor is the recipient/caster's actual custody item even in
  // another permitted property. Guild anchors belong to the actual owning
  // guild property. Item creator and current plot owner remain independent.
  const auto ownership=request.item_id==50874?"e.owner_kind=3 AND e.owner_id="+std::to_string(owner):"c.owner_character_id="+std::to_string(request.owner.character_id);
  struct Candidate {uint64_t custody=0,house=0;EstateId estate=0;};std::vector<Candidate> selected;
  auto inspect=[&](MySQLRequestResult& candidates,bool room){
   if(candidates.RowCount()>1000)throw Failure{Error::UnsupportedItem};
   for(auto row=candidates.begin();row!=candidates.end();++row){
    if(!row[2]||std::string(row[2])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::UnsupportedItem};
    auto stored=CustodyCodec::Decode(Unhex(row[3]));if(stored.values.item_id!=request.item_id)continue;
    if(request.item_id!=50874&&stored.values.character_id!=request.owner.character_id)throw Failure{Error::Conflict};
    auto id=std::stoull(row[0]);auto storage=ItemStorage::Locked(db,id);if(storage.stowed||CrateMemberScope::Locked(db,id))continue;
    selected.push_back({id,room?std::stoull(row[4]):0,Number(row[1])});
    if(selected.size()>1)throw Failure{Error::Conflict};
   }
  };
  auto yard_candidates=Q(db,"SELECT c.custody_id,e.id,c.schema_descriptor,HEX(c.row_blob) FROM housing_current_estates e JOIN housing_live_item_custody c ON c.estate_id=e.id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id WHERE "+ownership+HouseItemScope::ExteriorPredicate(db)+" ORDER BY c.custody_id LIMIT 1001 FOR UPDATE");inspect(yard_candidates,false);
  if(HouseItemScope::Installed(db)){
   auto room_candidates=Q(db,"SELECT c.custody_id,e.id,c.schema_descriptor,HEX(c.row_blob),scope.house_id FROM housing_house_item_scopes scope JOIN housing_live_item_custody c ON c.custody_id=scope.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_house_locations loc ON loc.house_id=scope.house_id AND loc.location_kind=2 JOIN housing_live_item_custody hc ON hc.custody_id=loc.custody_id JOIN housing_live_placed_items hp ON hp.custody_id=hc.custody_id JOIN housing_current_estates e ON e.id=hc.estate_id WHERE "+ownership+" ORDER BY c.custody_id LIMIT 1001 FOR UPDATE");inspect(room_candidates,true);
  }
  if(selected.size()!=1)throw Failure{Error::NotFound};const auto target=selected[0];
  result.estate_id=target.estate;result.custody_id=target.custody;
  if(!RentState::PlotActive(db,target.estate))throw Failure{Error::Unauthorized};
  if(!target.house){
   auto scene=LockedExterior(db,l,target.estate,fence,nullptr,houses_,nullptr,crates_,nullptr,0,target.custody);
   if(scene.items.size()!=1||scene.items[0].stored.stowed||scene.items[0].contained_in_crate)throw Failure{Error::Conflict};
   const auto& stored=scene.items[0].stored;
   if(stored.custody_id!=target.custody||stored.row.values.item_id!=request.item_id||!stored.placement||scene.route.zone_id!=712||scene.route.zone_version)throw Failure{Error::Conflict};
   result.native_estate_id=scene.native_plot_id;result.route=scene.route;result.pose_revision=stored.placement->pose_revision;result.pose=stored.placement->transform;result.destination_name=scene.definition.address;
   if(result.destination_name.empty())result.destination_name=scene.items[0].estate_owner_name;
  }else{
   if(!houses_||!room_profile_)throw Failure{Error::UnsupportedItem};
   auto house=Q(db,"SELECT custody_id FROM housing_house_locations WHERE house_id="+std::to_string(target.house)+" AND location_kind=2 FOR UPDATE");
   if(house.RowCount()!=1||!house.begin()[0])throw Failure{Error::NotFound};
   auto parent=LockedExterior(db,l,target.estate,fence,nullptr,houses_,nullptr,crates_,nullptr,0,std::stoull(house.begin()[0]),true);
   if(parent.items.size()!=1||!parent.items[0].house||parent.items[0].house->house_id!=target.house||parent.items[0].stored.stowed||parent.items[0].contained_in_crate||!parent.items[0].house->interior)throw Failure{Error::Conflict};
   const auto& destination=*parent.items[0].house->interior;
   auto room=LockedInterior(db,l,{destination.zone_id,destination.zone_version,destination.instance_id},fence,*houses_,false,true,room_profile_,&parent,crates_,true,target.custody);
   if(!room.house||!room.room_profile||room.contents.size()!=1||!room.crate_contents.empty())throw Failure{Error::Conflict};
   const auto& content=room.contents[0];const auto& stored=content.stored;
   if(stored.custody_id!=target.custody||stored.row.values.item_id!=request.item_id||stored.stowed||!stored.placement||!content.room||content.contained_in_crate)throw Failure{Error::Conflict};
   result.house_id=room.house->house_id;result.house_revision=room.house->revision;result.profile_revision=room.profile_revision;result.scope_revision=content.scope_revision;
   result.native_estate_id=destination.native_estate_id;result.route={destination.zone_id,destination.zone_version,destination.instance_id};
   result.pose_revision=stored.placement->pose_revision;result.pose=stored.placement->transform;result.destination_name=room.room_profile->definition.address;
   if(result.destination_name.empty())result.destination_name=parent.items[0].estate_owner_name;
  }
  if(expected&&!SameAnchor(*expected,result))throw Failure{Error::Conflict};
  if(admit){
   auto engine=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='instance_list_player'");
   if(engine.RowCount()!=1||!engine.begin()[0]||std::string(engine.begin()[0])!="InnoDB")throw Failure{Error::DatabaseFailure};
   auto membership=Q(db,"SELECT charid FROM instance_list_player WHERE id="+std::to_string(result.route.instance_id)+" AND charid="+std::to_string(l.actor.character_id)+" FOR UPDATE");
   if(membership.RowCount()>1)throw Failure{Error::DatabaseFailure};
   if(!membership.RowCount())Q(db,"INSERT INTO instance_list_player(id,charid) VALUES ("+std::to_string(result.route.instance_id)+","+std::to_string(l.actor.character_id)+")");
  }
  Q(db,"COMMIT");active=false;return {std::move(result),Error::None};
 }catch(const Failure& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}
 catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<InteriorSnapshot> Service::ReadPublishedAttachedInteriorCollisionUnderTransaction(Database& db,const LiveItems::Lease& l,uint64_t house,LiveItems::InventorySessionFence& fence,const HouseScenePolicy& policy,const Housing::InteriorContent::ProfilePolicy& room,const CrateBinding::CustodyContext* crates){
 if(!house||!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||Number(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  auto host=Q(db,"SELECT c.estate_id FROM housing_house_locations l JOIN housing_live_item_custody c ON c.custody_id=l.custody_id WHERE l.house_id="+std::to_string(house)+" AND l.location_kind=2");if(host.RowCount()!=1||!host.begin()[0])return {{},Error::NotFound};
  auto parent=LockedExterior(db,l,Number(host.begin()[0]),fence,nullptr,&policy,nullptr,crates);const Item* selected=nullptr;
  for(const auto& item:parent.items)if(item.house&&item.house->house_id==house){if(selected)throw Failure{Error::Conflict};selected=&item;}
  if(!selected||!selected->house->interior)throw Failure{Error::NotFound};const auto& i=*selected->house->interior;
  return {LockedInterior(db,l,{i.zone_id,i.zone_version,i.instance_id},fence,policy,false,true,&room,&parent,crates,false,0,false,false,AttachedRead::Storage,true),Error::None};
 }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Snapshot> Service::ReadSceneUnderTransaction(Database& db,const LiveItems::Lease& l,EstateId estate,LiveItems::InventorySessionFence& fence,const HouseScenePolicy* houses,const CrateBinding::CustodyContext* crates){
 try{return {LockedExterior(db,l,estate,fence,nullptr,houses,nullptr,crates),Error::None};}catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Snapshot> Service::ReadCurrent(const LiveItems::Lease& l,EstateId estate,LiveItems::InventorySessionFence& fence,const EntryRequest* entry){
 std::lock_guard<std::mutex> guard(mutex_);if(!enabled_)return {{},Error::Disabled};if(!estate||!l.actor.character_id||!l.actor.account_id||!l.epoch||!Key(l.session_key))return {{},Error::InvalidArgument};if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};auto& db=*database_;bool active=false;
 try{
 Q(db,"START TRANSACTION");active=true;
 auto out=LockedExterior(db,l,estate,fence,entry,houses_,nullptr,crates_);
 Q(db,"COMMIT");active=false;return {std::move(out),Error::None};
 }catch(const Failure& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
}
