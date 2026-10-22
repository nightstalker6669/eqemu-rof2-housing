#include "runtime_activation_db.h"
// Optional clone-only helper. This program cannot mint or certify process death.
// The direct parent supervisor records exact owned-child wait evidence first.
#include "inventory_process_recovery.h"
#include "../eqemu_config.h"
#include "../platform.h"
#include <iostream>
#include <cctype>
const EQEmuConfig* Config=nullptr;
int main(int argc,char** argv)
{
 try {
  if(argc!=2) return 2;
  const std::string process=argv[1];
  if(process.size()!=64) return 2;
  for(char c:process) if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))) return 2;
  RegisterExecutablePlatform(ExePlatformClientImport);
  EQEmuLogSys::Instance()->LoadLogSettingsDefaults();
  // This standalone helper is launched with cwd=server; PathManager is not initialized.
  if(!EQEmuConfig::LoadConfig(".")) return 3;
  const auto* cfg=EQEmuConfig::get();
  if(!cfg) return 3;
  auto connect=[&] {
   auto db=std::make_unique<Database>();
   if(!db->Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"supervised-recovery")) throw std::runtime_error("connect");
   return db;
  };
  auto db=connect();
  if(!Housing::RuntimeActivation::AllowsReconstructedContent(*db))return 3;
  auto rows=db->QueryDatabase("SELECT character_id,account_id,epoch,HEX(token),client_key,zone_id,instance_id FROM housing_inventory_sessions WHERE process_key='"+process+"' AND active=1 ORDER BY character_id",false);
  if(!rows.Success())return 4;
  std::vector<Housing::InventorySessions::Lease> leases;
  auto nibble=[](char c)->uint8_t {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("token");};
  for(auto row=rows.begin();row.operator!=(rows.end());++row){
   Housing::InventorySessions::Lease lease;
   lease.actor={uint32_t(std::stoul(row[0])),uint32_t(std::stoul(row[1]))};
   lease.epoch=std::stoull(row[2]);
   if(!row[3]||std::string(row[3]).size()!=64)return 4;
   for(size_t i=0;i<32;++i)lease.token[i]=(nibble(row[3][i*2])<<4)|nibble(row[3][i*2+1]);
   lease.owner={process,row[4],uint32_t(std::stoul(row[5])),uint16_t(std::stoul(row[6]))};lease.active=true;
   leases.push_back(std::move(lease));
  }
  for(const auto& lease:leases){
   bool reconciled=false;
   // Fresh-connection idempotent retry only after a commit uncertainty. No retry
   // inside a transaction and no fallback forced release after a failure.
   for(int attempt=0;attempt<3;++attempt){
    Housing::InventoryRecovery::Service service(connect(),true);
    const auto result=service.Reconcile(lease);
    if(result){reconciled=true;break;}
    if(result.error!=Housing::InventoryRecovery::Error::CommitUncertain)return 5;
   }
   if(!reconciled)return 5;
  }
  return 0;
 }catch(...){return 6;}
}
