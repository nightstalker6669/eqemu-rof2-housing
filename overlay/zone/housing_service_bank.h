#ifndef EQEMU_ZONE_HOUSING_SERVICE_BANK_H
#define EQEMU_ZONE_HOUSING_SERVICE_BANK_H
#include <cstdint>
#include <string>
class Client;
namespace Housing::ServiceBank {
// All basic and housing guild-bank handlers share this per-guild DB lock.
// A stale client's request is refused after sending a fresh complete snapshot,
// rather than applying an old client slot index to a newly loaded inventory.
class Request {
 Client& client_;uint32_t guild_=0;uint64_t connection_=0;
 std::string name_;bool locked_=false,ready_=false,published_=false,transaction_=false,failed_=false;
public:
 explicit Request(Client&,bool publish_only=false) noexcept;
 ~Request();
 Request(const Request&)=delete;Request& operator=(const Request&)=delete;
 bool Ready()const noexcept{return ready_;}
 bool Published()const noexcept{return published_;}
 void Abort()noexcept{failed_=true;}
};
void AbortCurrent() noexcept;
bool Publish(Client&) noexcept;
}
#endif
