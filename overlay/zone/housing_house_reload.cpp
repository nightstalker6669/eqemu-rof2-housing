#include "housing_house_reload.h"
#include "housing_house_preload_fence.h"
#include "housing_inventory_bridge.h"
#include "housing_house_runtime.h"
#include "client.h"
#include "zonedb.h"
#include "../common/eqemu_config.h"
#include <mutex>
namespace Housing::HousePreload {
namespace {
std::mutex policy_mutex;
ZonePolicies policies;
bool configured=false,admission_started=false;
class UnreviewedTemplates final:public HouseBinding::HouseTemplatePolicy {
 bool VerifiedHouse(const LiveItems::InventoryRow&)const noexcept override{return false;}
};
const UnreviewedTemplates unreviewed;
std::unique_ptr<Database> Connect(){
 const auto* config=EQEmuConfig::get();if(!config)return {};
 auto db=std::make_unique<Database>();
 if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-house-profile-reload"))return {};
 return db;
}
}
bool ConfigureZonePoliciesAtSafeStartup(ZonePolicies selected){
 if(!InventoryBridge::IsRegisteredZoneThread()||!selected.templates||!selected.routes)return false;
 std::lock_guard<std::mutex> guard(policy_mutex);
 if(configured||admission_started)return false;
 policies=std::move(selected);configured=true;return true;
}
ZonePolicies CurrentZonePolicies(){std::lock_guard<std::mutex> guard(policy_mutex);return policies;}
bool LoadInventoryWithHouseBindings(Client& client,ZoneDatabase& loader,bool enabled){
 auto held=[&client]{return client.HasInventorySessionReservation();};
 if(!enabled||client.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return loader.GetInventory(&client,held);
 try{
  if(client.ClientDataLoaded())return false;
  auto fence=ZoneSessionFence::Create(client);if(!fence)return false;
  ZonePolicies selected;
  {std::lock_guard<std::mutex> guard(policy_mutex);admission_started=true;selected=policies;}
  auto db=Connect();if(!db)return false;
  // Preserve installations without the optional house extension. A partial
  // schema is an admission failure, not permission to expose unbound houses.
  auto schema=db->QueryDatabase(std::string("SELECT COUNT(*),COALESCE(SUM(engine='InnoDB'),0) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors','housing_house_receipt_effects')"),false);
  if(!schema.Success()||schema.RowCount()!=1)return false;
  auto row=schema.begin();if(!row[0]||!row[1])return false;
  const auto count=std::stoull(row[0]);const auto innodb=std::stoull(row[1]);
  if(!count)return loader.GetInventory(&client,held);
  if(count!=4||innodb!=4)return false;
  Service service(std::move(db),true);
  const auto& templates=selected.templates?*selected.templates:static_cast<const HouseBinding::HouseTemplatePolicy&>(unreviewed);
  auto before=service.BeforeLoad(fence->Reservation(),*fence,templates,selected.routes.get(),selected.crates.get());if(!before)return false;
  std::vector<Loaded> loaded;
  if(!loader.GetInventory(&client,held,&loaded))return false;
  auto checked=service.AfterInventoryLoad(before.value,loaded,*fence,templates,selected.routes.get(),selected.crates.get());if(!checked)return false;
  return bool(Service::Hydrate(checked.value,client.GetInv(),*fence));
 }catch(...){return false;}
}
}
