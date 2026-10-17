#include "inventory_session_owner.h"
#include "housing_inventory_bridge.h"
#include "client.h"
#include "../common/eqemu_config.h"
#include <openssl/rand.h>
#include <chrono>
#include <thread>
#include <type_traits>
#include <cstdlib>
namespace Housing::InventorySessions {namespace {
std::string Nonce(){uint8_t raw[32];if(RAND_bytes(raw,32)!=1)throw std::runtime_error("Inventory session nonce failed");const char* h="0123456789abcdef";std::string s;for(auto c:raw){s+=h[c>>4];s+=h[c&15];}return s;}
bool Supervised(){const char* flag=std::getenv("EQEMU_INVENTORY_SUPERVISED");return flag&&std::string(flag)=="1";}
const std::string& ProcessKey(){static const std::string key=[] {
 if(!Supervised())return Nonce();
 const char* birth=std::getenv("EQEMU_INVENTORY_PROCESS_KEY");
 if(!birth||std::string(birth).size()!=64)throw std::runtime_error("Supervised inventory birth key missing");
 std::string value=birth;
 for(const char c:value)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))throw std::runtime_error("Supervised inventory birth key invalid");
 return value;
}();return key;}
void Wait(){std::this_thread::sleep_for(std::chrono::milliseconds(250));}
}
std::unique_ptr<Service> ZoneReservation::Fresh()const{const auto* cfg=EQEmuConfig::get();if(!cfg)return nullptr;auto db=std::make_unique<Database>();if(!db->Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"inventory-session-exclusive"))return nullptr;return std::make_unique<Service>(std::move(db),true,Supervised());}
std::shared_ptr<ZoneReservation> ZoneReservation::ClaimForAuthenticatedClient(Client& c){
 static_assert(std::is_nothrow_move_assignable_v<Lease>);
 if(!InventoryBridge::IsRegisteredZoneThread()||!c.CharacterID()||!c.AccountID()||!c.Connection()||!zone)return nullptr;
 auto ticket=std::shared_ptr<ZoneReservation>(new ZoneReservation);ticket->connection_=c.Connection();AuthenticatedPurchaser actor{c.CharacterID(),c.AccountID()};Owner owner;try{owner={ProcessKey(),Nonce(),zone->GetZoneID(),uint16_t(zone->GetInstanceID())};}catch(...){return nullptr;}bool uncertain=false;
 for(;;){try{auto service=ticket->Fresh();if(!service){if(!uncertain)return nullptr;}else{auto claimed=service->Claim(actor,owner);if(claimed){ticket->lease_=std::move(claimed.value);return ticket;}if(claimed.error==Error::CommitUncertain)uncertain=true;else if(!uncertain||claimed.error==Error::Busy||claimed.error==Error::AlreadyReleased||claimed.error==Error::Unauthorized)return nullptr;}}catch(...){if(!uncertain)return nullptr;}Wait();}}
bool ZoneReservation::HeldFor(const Client& c)const noexcept{return !released_&&lease_.active&&InventoryBridge::IsRegisteredZoneThread()&&c.CharacterID()==lease_.actor.character_id&&c.AccountID()==lease_.actor.account_id&&const_cast<Client&>(c).Connection()==connection_;}
std::optional<Lease> ZoneReservation::SnapshotFor(const Client& c)const {
 if(!HeldFor(c)||!zone||zone->GetZoneID()!=lease_.owner.zone_id||zone->GetInstanceID()!=lease_.owner.instance_id)return std::nullopt;
 return lease_;
}
void ZoneReservation::ReleaseAfterCompleteDelete(){
 if(released_)return; // Token/release history stays durable even if next epoch starts.
 if(!InventoryBridge::IsRegisteredZoneThread())for(;;)Wait();
 for(;;){try{auto service=Fresh();if(service){auto closed=service->Release(lease_);if(closed){released_=true;lease_.active=false;return;}if(closed.error==Error::CommitUncertain||closed.error==Error::DatabaseFailure){service.reset();auto fresh=Fresh();if(fresh){auto resolved=fresh->ResolveReleased(lease_);if(resolved&&resolved.value){released_=true;lease_.active=false;return;}}}}}catch(...){}Wait();}}
}
