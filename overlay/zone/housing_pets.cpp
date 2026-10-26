#include "housing_pets.h"
#include "client.h"
#include "entity.h"
#include "npc.h"
#include "zone.h"
#include "map.h"
#include "mob_movement_manager.h"
#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <cstring>
extern Zone* zone;
namespace Housing::Pets {
namespace {
using Key=std::pair<uint32_t,uint32_t>;
struct Entry {Binding binding;NPC* actor=nullptr;uint16_t spawn=0;PetCommands::Command mode=PetCommands::Command::Stay;uint32_t follow=0;float goal_x=0,goal_y=0;uint64_t sequence=0;std::chrono::steady_clock::time_point created=std::chrono::steady_clock::now();};
std::map<Key,Entry> pets;
using Clock=std::chrono::steady_clock;
auto next_tick=Clock::now(),next_validate=Clock::now();
uint32_t zone_id=0;uint16_t instance_id=0;
bool Route(){return zone&&zone_id==zone->GetZoneID()&&instance_id==zone->GetInstanceID()&&instance_id;}
bool Live(const Entry& e){return entity_list.GetMob(e.spawn)==e.actor&&e.actor&&!e.actor->GetDepop()&&e.actor->GetHousingEstateId()==e.binding.native_estate_id&&e.actor->GetHousingItemId()==e.binding.native_item_id;}
void Remove(Entry& e){if(Live(e))e.actor->Depop(false);}
bool Same(const Binding& a,const Binding& b){return a.catalog_revision==b.catalog_revision&&a.custody_id==b.custody_id&&a.pose_revision==b.pose_revision&&a.owner==b.owner&&a.estate_id==b.estate_id&&a.native_estate_id==b.native_estate_id&&a.native_item_id==b.native_item_id&&NpcRuntime::Same(a.profile,b.profile)&&a.pose.position==b.pose.position&&a.pose.heading==b.pose.heading&&a.pose.scale==b.pose.scale;}
bool Inside(const Binding& b,float x,float y,float z){
 for(const auto& r:b.regions){const auto&m=r.matrix;double a=m[0],c=m[2],d=m[4],f=m[6],g=m[8],i=m[10],v=m[1],e=m[5],h=m[9];
  std::array<double,9> adj{e*i-f*h,c*h-v*i,v*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,v*g-a*h,a*e-v*d};
  double det=a*adj[0]+v*adj[3]+c*adj[6];if(!std::isfinite(det)||!det)continue;std::array<double,3> p{y,x,z};bool in=true;
  for(unsigned k=0;k<3;++k){double local=0;for(unsigned j=0;j<3;++j)local+=(p[j]-m[12+j])*adj[3*j+k]/det;if(!std::isfinite(local)||std::abs(local)>=.98)in=false;}if(in)return true;
 }return false;
}
Client* Follower(Entry&e){for(const auto& row:entity_list.GetClientList())if(row.second&&row.second->CharacterID()==e.follow&&row.second->Connected()&&row.second->ClientDataLoaded())return row.second;return nullptr;}
bool Authorized(const Entry&e){for(const auto& row:entity_list.GetClientList())if(row.second&&row.second->Connected()&&row.second->ClientDataLoaded()&&row.second->ValidateHousingPet(e.binding,false))return true;return false;}
void Stop(Entry& e){e.actor->StopNavigation();e.actor->SetMoving(false);MobMovementManager::Get().SendCommandToClients(e.actor,0,0,0,0,0,ClientRangeAny);}
}
bool Supported(const NpcRuntime::Profile&p)noexcept{try{return NpcRuntime::Valid(p)&&!p.service&&p.respawn_interval&&p.original_model_default;}catch(...){return false;}}
void Clear()noexcept{try{for(auto& [key,e]:pets)Remove(e);}catch(...){}pets.clear();zone_id=0;instance_id=0;}
bool Reconcile(uint32_t estate,const std::vector<Binding>& wanted)noexcept{
 try{
  if(!zone||!zone->GetInstanceID()||!estate||wanted.size()>128)return false;if(!pets.empty()&&!Route())Clear();zone_id=zone->GetZoneID();instance_id=zone->GetInstanceID();
  std::set<Key> keys;for(const auto&b:wanted){if(b.native_estate_id!=estate||!b.native_item_id||!b.custody_id||!b.owner||!b.pose_revision||!Supported(b.profile)||b.regions.empty()||b.name.empty()||b.name.size()>63||!keys.insert({estate,b.native_item_id}).second)return false;for(const auto&r:b.regions)RegionWire::Detail::ValidateMatrix(r.matrix);}
  for(auto it=pets.begin();it!=pets.end();){if(it->first.first!=estate){++it;continue;}const Binding* match=nullptr;for(const auto&b:wanted)if(b.native_item_id==it->first.second)match=&b;if(!match||!Same(it->second.binding,*match)||!Live(it->second)){Remove(it->second);it=pets.erase(it);}else{it->second.binding=*match;++it;}}
  for(const auto&b:wanted){Key key{estate,b.native_item_id};if(pets.count(key))continue;auto type=std::make_unique<NPCType>();std::memset(type.get(),0,sizeof(NPCType));
   std::string name=b.name;for(char&c:name)if(c==' ')c='_';std::memcpy(type->name,name.data(),name.size());type->race=b.profile.race;type->gender=b.profile.gender;type->class_=1;type->bodytype=1;type->level=1;type->current_hp=type->max_hp=1;type->size=NpcNativeHeight::Float(b.frame?b.frame->height_bits:b.profile.allowed_heights.height_bits[0]);type->runspeed=.7f;type->npc_aggro=false;type->skip_global_loot=true;type->skip_auto_scale=true;type->trackable=false;type->show_name=true;
   const auto pose=NpcRuntime::ActorPose(b.profile,b.pose,b.frame);auto npc=std::make_unique<NPC>(type.get(),nullptr,glm::vec4(pose.position[1],pose.position[0],pose.position[2],pose.heading),GravityBehavior::Flying);npc->GiveNPCTypeData(type.release());
   char guid[17];std::snprintf(guid,sizeof(guid),"%016llx",static_cast<unsigned long long>(b.custody_id));if(!npc->SetHousingItemBinding(guid,estate,b.native_item_id))return false;npc->AI_Stop();npc->Mob::AI_Stop();npc->SetInvul(true);npc->SetEntityVariable("housing_pet_owned","1");npc->SaveGuardSpot();entity_list.AddNPC(npc.get(),true,true);if(!npc->GetID())return false;
   Entry e;e.binding=b;e.actor=npc.get();e.spawn=npc->GetID();e.goal_x=npc->GetX();e.goal_y=npc->GetY();npc.release();pets.emplace(key,std::move(e));
  }return true;
 }catch(...){Clear();return false;}
}
bool Publish(Client&c,uint32_t estate,const std::function<std::string(uint32_t)>& guid)noexcept{
 try{if(!Route())return pets.empty();for(const auto&[key,e]:pets)if(key.first==estate&&Live(e)){auto text=guid(key.second);if(text.size()!=16||text.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)return false;EQApplicationPacket p(OP_RealEstateBindSpawn,32);std::memset(p.pBuffer,0,32);auto put=[&](unsigned at,uint32_t v){for(unsigned j=0;j<4;++j)p.pBuffer[at+j]=uint8_t(v>>(j*8));};put(0,e.spawn);std::memcpy(p.pBuffer+4,text.data(),16);put(24,estate);put(28,key.second);c.QueuePacket(&p);}return true;}catch(...){return false;}
}
std::optional<float> Distance(Client&c,uint32_t estate,uint32_t item)noexcept{auto it=pets.find({estate,item});if(!Route()||it==pets.end()||!Live(it->second))return {};auto*n=it->second.actor;double dx=c.GetX()-n->GetX(),dy=c.GetY()-n->GetY(),dz=c.GetZ()-n->GetZ();return float(std::sqrt(dx*dx+dy*dy+dz*dz));}
bool Command(Client&c,const PetCommands::Request&r)noexcept{
 try{auto it=pets.find({r.estate_id,r.item_id});if(!Route()||it==pets.end()||!Live(it->second)||!c.ValidateHousingPet(it->second.binding,true))return false;auto distance=Distance(c,r.estate_id,r.item_id);if(!distance||*distance>20)return false;auto&e=it->second;Stop(e);e.mode=r.command;e.follow=r.command==PetCommands::Command::Follow?c.CharacterID():0;e.goal_x=e.actor->GetX();e.goal_y=e.actor->GetY();e.actor->SetAppearance(r.command==PetCommands::Command::PlayDead?eaDead:eaStanding);return true;}catch(...){return false;}
}
void Process()noexcept{
 try{if(pets.empty())return;if(!Route()){Clear();return;}auto now=Clock::now();if(now<next_tick)return;next_tick=now+std::chrono::milliseconds(250);bool validate=now>=next_validate;if(validate)next_validate=now+std::chrono::seconds(5);
  for(auto it=pets.begin();it!=pets.end();){auto&e=it->second;if(!Live(e)||(validate&&now-e.created>=std::chrono::seconds(5)&&!Authorized(e))){Remove(e);it=pets.erase(it);continue;}++it;
   if(e.mode==PetCommands::Command::Stay||e.mode==PetCommands::Command::PlayDead)continue;float x=e.actor->GetX(),y=e.actor->GetY(),z=e.actor->GetZ();
   if(e.mode==PetCommands::Command::Follow){auto*c=Follower(e);if(!c||(validate&&!c->ValidateHousingPet(e.binding,true))||!Inside(e.binding,c->GetX(),c->GetY(),c->GetZ())){e.mode=PetCommands::Command::Stay;e.follow=0;Stop(e);continue;}e.goal_x=c->GetX();e.goal_y=c->GetY();if(std::hypot(e.goal_x-x,e.goal_y-y)<6){Stop(e);continue;}}
   else if(std::hypot(e.goal_x-x,e.goal_y-y)<1){++e.sequence;double angle=e.sequence*2.399963229728653;float radius=4+float(e.sequence%5)*2;e.goal_x=e.binding.pose.position[1]+radius*std::cos(angle);e.goal_y=e.binding.pose.position[0]+radius*std::sin(angle);}
   float dx=e.goal_x-x,dy=e.goal_y-y,length=std::hypot(dx,dy);if(!std::isfinite(length)||length<.01f)continue;float step=std::min(length,e.mode==PetCommands::Command::Walk?.5f:1.5f);float nx=x+dx/length*step,ny=y+dy/length*step;
   if(!zone->zonemap){Stop(e);continue;}glm::vec3 point(nx,ny,z);float ground=zone->zonemap->FindBestZ(point,nullptr);float offset=NpcNativeHeight::Float(e.binding.frame?e.binding.frame->height_bits:e.binding.profile.allowed_heights.height_bits[0]);float nz=ground+offset;
   if(!std::isfinite(ground)||ground<-100000||std::abs(nz-z)>3||!Inside(e.binding,nx,ny,ground)||!zone->zonemap->CheckLoS({x,y,z},{nx,ny,nz})){e.goal_x=x;e.goal_y=y;Stop(e);continue;}
   e.actor->SetHeading(e.actor->CalculateHeadingToTarget(nx,ny));e.actor->SetPosition(nx,ny,nz);e.actor->SetMoving(true);MobMovementManager::Get().SendCommandToClients(e.actor,0,0,0,0,e.mode==PetCommands::Command::Walk?e.actor->GetWalkspeed():e.actor->GetRunspeed(),ClientRangeAny);
  }
 }catch(...){Clear();}
}
}
