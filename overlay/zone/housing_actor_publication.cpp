#include "housing_actor_publication.h"
#include "client.h"
#include "entity.h"
#include "housing_service_npcs.h"
#include "../common/housing/npc_exit_collision.h"
#include "../common/classes.h"
#include "../common/eq_packet.h"
#include "../common/housing/ordinary_character_profiles.h"
#include <cmath>
#include <bit>
#include <cstring>
#include <limits>
namespace Housing::ActorPublication {
namespace {std::map<const Client*,Ledger> publications;const Ledger absent(false);}
void Ledger::Advance()noexcept{if(revision_==UINT64_MAX)valid_=false;else ++revision_;}
void Ledger::Dirty(uint32_t id)noexcept{Advance();auto it=records_.find(id);if(it!=records_.end())it->second.ordinary=false;}
void Ledger::Observe(const EQApplicationPacket* p)noexcept{
 if(!p||!valid_)return;
 try{switch(p->GetOpcode()){
 case OP_SpawnDoor:{
  if(!p->size||p->size%sizeof(Door_Struct)||p->size/sizeof(Door_Struct)>500){valid_=false;return;}
  std::map<uint32_t,DoorRecord> incoming;
  for(size_t offset=0;offset<p->size;offset+=sizeof(Door_Struct)){
   Door_Struct door{};std::memcpy(&door,p->pBuffer+offset,sizeof(door));Advance();
   if(!valid_||incoming.count(door.doorId)){valid_=false;return;}
   const bool closed=door.state_at_spawn==0&&door.invert_state==0;
   incoming.emplace(door.doorId,DoorRecord{door,revision_,closed});
  }
  if(doors_reset_)doors_=std::move(incoming);
  else{
   // Normal producer sends a full set, but GM object conversion can emit one
   // additive door with the same opcode. Only RemoveAllDoors proves deletion.
   // Retain omitted published IDs as stale Unknown rather than invent a clear.
   for(auto&[id,old]:doors_)if(!incoming.count(id)){old.closed_resource=false;old.generation=revision_;}
   for(auto&[id,door]:incoming)doors_[id]=std::move(door);
  }
  doors_reset_=false;break;
 }
 case OP_MoveDoor:{
  if(p->size!=sizeof(MoveDoor_Struct)){valid_=false;return;}
  MoveDoor_Struct move{};std::memcpy(&move,p->pBuffer,sizeof(move));Advance();
  auto old=doors_.find(move.doorid);if(old!=doors_.end()){old->second.closed_resource=false;old->second.generation=revision_;}
  // A move of an absent ID cannot create a native graphics actor.
  break;
 }
 case OP_RemoveAllDoors:{
  if(p->size){valid_=false;return;}Advance();doors_.clear();doors_reset_=true;break;
 }
 case OP_ZoneEntry:case OP_NewSpawn:case OP_ZoneSpawns:{
  if(!p->size||p->size%sizeof(Spawn_Struct)){valid_=false;return;}
  for(size_t offset=0;offset<p->size;offset+=sizeof(Spawn_Struct)){
   Spawn_Struct s{};std::memcpy(&s,p->pBuffer+offset,sizeof(s));Advance();
   if(!s.spawnId||s.spawnId>65535||records_.size()>=16384){valid_=false;return;}
   // Mirrors the ordinary RoF2 encoder's body property and size rewrite. The
   // special/destructible decoder families are deliberately not projected.
   bool ordinary=s.NPC<=1&&!s.DestructibleObject&&s.class_!=Class::LDoNTreasure&&s.bodytype<66&&!s.petOwnerId&&!s.is_pet;
   if(s.NPC&&s.race>12&&s.race!=128&&s.race!=130&&s.race!=330&&s.race!=522&&s.size==0)s.size=6;
   ordinary=ordinary&&std::isfinite(s.size)&&s.size>=0&&s.size<=256;
   records_[s.spawnId]={s,revision_,ordinary};
  }break;
 }
 case OP_RealEstateBindSpawn:{
  // Original5b26fa binds only an already resolved PlayerClient. The canonical
  // consumed profile has GUID16+NUL at4..20 and IDs at24/28;21..23 unobserved.
  if(p->size!=32){valid_=false;return;}
  const auto word=[&](size_t at){uint32_t value=0;for(size_t n=0;n<4;++n)value|=uint32_t(p->pBuffer[at+n])<<(8*n);return value;};
  const auto id=word(0);auto found=records_.find(id);if(found==records_.end())break;
  const std::string guid(reinterpret_cast<const char*>(p->pBuffer+4),16);const auto estate=word(24),item=word(28);
  if(p->pBuffer[20]||guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos||!estate||estate==UINT32_MAX||!item||item==UINT32_MAX){valid_=false;return;}
  Advance();std::memcpy(found->second.spawn.housing_item_guid,guid.data(),16);found->second.spawn.housing_item_guid[16]=0;found->second.spawn.housing_estate_id=estate;found->second.spawn.housing_item_id=item;found->second.generation=revision_;break;
 }
 case OP_DeleteSpawn:{if(p->size<4){valid_=false;return;}uint32_t id;std::memcpy(&id,p->pBuffer,4);Advance();records_.erase(id);break;}
 case OP_Illusion:case OP_ChangeSize:{if(p->size<4){valid_=false;return;}uint32_t id;std::memcpy(&id,p->pBuffer,4);LogInfo("Housing actor publication dirty spawn [{}] opcode [{}]",id,unsigned(p->GetOpcode()));Dirty(id);break;}
 case OP_SpawnAppearance:{
  if(p->size!=sizeof(SpawnAppearance_Struct)){valid_=false;return;}SpawnAppearance_Struct s;std::memcpy(&s,p->pBuffer,sizeof(s));
  // These setters can change the factory family/height or add a rider/pet.
  // Cosmetic/state-only appearance messages do not alter the volume2 proof.
  switch(s.type){case AppearanceType::Size:case AppearanceType::SetType:case AppearanceType::Pet:case AppearanceType::DamageState:case AppearanceType::TextureType:LogInfo("Housing actor publication dirty spawn [{}] appearance [{}] value [{}]",s.spawn_id,s.type,s.parameter);Dirty(s.spawn_id);break;default:break;}break;
 }
 default:break;
 }}catch(...){valid_=false;}
}
const Ledger& Publication(const Client& client)noexcept{auto i=publications.find(&client);return i==publications.end()?absent:i->second;}
void Observe(const Client& client,const EQApplicationPacket* packet)noexcept{try{publications.try_emplace(&client).first->second.Observe(packet);}catch(...){publications.erase(&client);}}
void Forget(const Client& client)noexcept{publications.erase(&client);}
uint64_t Revision(const Client& client)noexcept{return Publication(client).Revision();}
std::optional<PlacedHeightFilters::Actor> ProjectHeightActor(Client&client,uint16_t id)noexcept{
 try{const auto*record=Publication(client).Find(id);Mob*mob=entity_list.GetMob(id);
  // Client::FillSpawnStruct2639 intentionally sends size0, regardless of the
  // player's current Mob size. Other Mob spawns send their actual size1346.
  // Apply the exact RoF2 NPC-zero rewrite4717..4728 to that outgoing input,
  // rather than comparing an encoded default to an unencoded Mob field.
  float expected_size=mob?(mob->IsClient()?0.f:mob->GetSize()):0.f;
  if(record&&record->spawn.NPC&&record->spawn.race>12&&record->spawn.race!=128&&record->spawn.race!=130&&record->spawn.race!=330&&record->spawn.race!=522&&expected_size==0)expected_size=6;
  const auto refuse=[&](const char*reason)->std::optional<PlacedHeightFilters::Actor>{
   const auto*profile=record?OrdinaryCharacterProfiles::Find(record->spawn.race,record->spawn.gender):nullptr;
   LogInfo("Housing actor projection refused spawn [{}] reason [{}] revision [{}] record [{}] ordinary [{}] profile [{}] classifier [{}] wire npc/race/gender/body/size [{}/{}/{}/{}/{}] wire class/pet/destructible [{}/{}/{}] live [{}] race/base/gender/base/body/size/owner [{}/{}/{}/{}/{}/{}/{}] expected-outgoing-size [{}]",
    id,reason,Publication(client).Revision(),bool(record),record?record->ordinary:false,bool(profile),profile?profile->flags:0,
    record?unsigned(record->spawn.NPC):0,record?record->spawn.race:0,record?unsigned(record->spawn.gender):0,record?record->spawn.bodytype:0,record?record->spawn.size:0.f,
    record?unsigned(record->spawn.class_):0,record?record->spawn.petOwnerId:0,record?bool(record->spawn.DestructibleObject):false,
    bool(mob),mob?mob->GetRace():0,mob?mob->GetBaseRace():0,mob?unsigned(mob->GetGender()):0,mob?unsigned(mob->GetBaseGender()):0,mob?unsigned(mob->GetBodyType()):0,mob?mob->GetSize():0.f,mob?mob->GetOwnerID():0,expected_size);
   return {};
  };
  if(!record)return refuse("missing-publication");if(!record->ordinary)return refuse("nonordinary-or-invalidated");if(!mob)return refuse("missing-live-entity");const auto&s=record->spawn;
  if(s.race!=mob->GetRace()||s.gender!=mob->GetGender()||s.bodytype!=uint32_t(mob->GetBodyType())||s.size!=expected_size||mob->GetRace()!=mob->GetBaseRace()||mob->GetGender()!=mob->GetBaseGender()||mob->GetOwnerID())return refuse("identity-base-size-mismatch");
  // Original classifier rows BAM2/0=32 and EYE108/2=0 have bit0 clear.
  // Both use default volume2 after the ordinary decoder's singleton property
  // (not100), zero special-spawnID/SplineID and no rider/alternate creation.
  const auto*profile=OrdinaryCharacterProfiles::Find(s.race,s.gender);
  if(!profile||(profile->flags&1)||s.bodytype==100)return refuse("unreviewed-physical-profile");
  // Ordinary decoder59141f..43a reads transmitted size into rawHeight+13c.
  // Producer5a3f40 selects positive rawHeight, or positive default6 for zero;
  // its four model-option multipliers and BAM ROffset3.125 are all positive.
  // Correction59efd0 leaves BAM unchanged; EYE108 dispatches59f038 and writes
  // final AvatarHeight3. Source proof is positivity, NOT an exact radius.
  // Bound sizes away from underflow; zero follows original default-height path.
  if(s.size!=0&&(s.size<0x1p-20f||s.size>256))return refuse("height-input-range");
  PlacedHeightFilters::Actor actor;actor.application_identity=id;actor.type=1;actor.restriction_mask=s.NPC?6:2;actor.volume=2;
  // FloorPrimary's only radius predicate is ==0 (Ceiling ignores radius).
  // 1 is a normalized nonzero equivalence-class witness, never body geometry
  // or a claimed measured native radius. No consumer may use it as a bound.
  actor.radius=1.f;return actor;
 }catch(...){LogInfo("Housing actor projection refused spawn [{}] reason [exception]",id);return {};}
}
std::optional<PlacedHeightFilters::Actor> ProjectServiceHeightActor(Client&client,uint16_t id,
 const NpcRuntime::Profile&profile,uint64_t custody,uint32_t estate,uint32_t item)noexcept{
 try{
  const auto*record=Publication(client).Find(id);auto*mob=entity_list.GetMob(id);
  if(!record||!record->ordinary||!mob||!mob->IsNPC()||!profile.service||!NpcExitCollision::SphereMode(profile))return {};
  auto binding=ServiceNpcs::LiveBinding(mob);const auto*policy=ServiceItemBindings::Lookup(profile.item_id);
  if(!binding||!policy||binding->custody_id!=custody||binding->native_estate_id!=estate||binding->native_item_id!=item||binding->item_id!=profile.item_id||binding->npc_type_id!=policy->npc_type_id)return {};
  const auto&s=record->spawn;const std::string guid(s.housing_item_guid,16);
  if(s.housing_item_guid[16]||guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos||s.housing_estate_id!=estate||s.housing_item_id!=item||s.NPC!=1||s.race!=policy->race||s.gender!=policy->gender||s.race!=mob->GetRace()||s.gender!=mob->GetGender()||s.bodytype!=uint32_t(mob->GetBodyType())||s.class_!=mob->GetClass()||s.size!=mob->GetSize()||std::bit_cast<uint32_t>(s.size)!=policy->default_size_bits||mob->GetRace()!=mob->GetBaseRace()||mob->GetGender()!=mob->GetBaseGender()||mob->GetOwnerID())return {};
  // Same ordinary network decoder/factory boundary as ProjectHeightActor:
  // singleton ordinary body property, no special/rider/pet construction.
  // Reviewed719-prefix hierarchy+physicalbit0 authority includes I28. Its
  // positive raw size/ROffset/control multipliers prove nonzero native radius;
  // this is no exact physical radius or blanket movement nonblocking claim.
  PlacedHeightFilters::Actor actor;actor.application_identity=item;actor.type=1;actor.restriction_mask=6;actor.volume=2;actor.radius=1.f;return actor;
 }catch(...){return {};}
}
}
