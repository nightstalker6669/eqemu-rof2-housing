#ifndef EQEMU_HOUSING_NATIVE_OWNERSHIP_BINDING_DB_H
#define EQEMU_HOUSING_NATIVE_OWNERSHIP_BINDING_DB_H
#include "estate_retirement_db.h"
#include "catalog_service.h"
namespace Housing::NativeOwnership {
// Server-only current-generation attachment, not a purchase/funding operation.
// Caller owns the property transaction and genuine held session fence. Never
// imports/revisions definitions, recreates native IDs or rewrites old tokens.
inline LiveItems::Result<bool> BindUnderTransaction(Database& db,bool enabled,
 const LiveItems::Lease& lease,EstateId estate,uint32_t native,
 LiveItems::InventorySessionFence& fence){
 using namespace EstateRetirement::Detail;using Error=LiveItems::Error;
 if(!enabled)return {{},Error::Disabled};
 if(!estate||!native||native==UINT32_MAX)return {{},Error::InvalidArgument};
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,lease,fence);if(!authenticated)return {{},authenticated.error};
  if(!authenticated.value.pending_key.empty()||CoinPending::Inspect(db,lease.actor.character_id,true)!=CoinPending::State::Clear)return {{},Error::PendingApplication};
  auto owner=Q(db,"SELECT plot_id,owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
  if(owner.RowCount()!=1)return {{},Error::NotFound};
  if(N(owner.begin()[1])!=1||N(owner.begin()[2])!=lease.actor.character_id)return {{},Error::Unauthorized};
  const auto plot=N(owner.begin()[0]);
  // Reuse requires a completed retirement producer, not an administrator just
  // flipping active while old occupants/items might still belong to the plot.
  Empty(db,"SELECT previous.id FROM housing_estates previous LEFT JOIN housing_estate_retirements retired ON retired.estate_id=previous.id WHERE previous.plot_id="+std::to_string(plot)+" AND previous.active=0 AND retired.estate_id IS NULL");
  auto node=Q(db,"SELECT n.plot_id,COALESCE(n.ownership_estate_id,0),n.definition_id,c.revision,HEX(c.definitions_blob),HEX(c.regions_blob) FROM housing_native_estates n JOIN housing_native_estates parent ON parent.native_id=n.parent_native_id AND parent.parent_native_id IS NULL AND parent.active=1 AND parent.catalog_key=n.catalog_key AND parent.neighborhood_id=n.neighborhood_id JOIN housing_catalogs c ON c.catalog_key=n.catalog_key WHERE n.native_id="+std::to_string(native)+" AND n.active=1 FOR UPDATE");
  if(node.RowCount()!=1||N(node.begin()[0])!=plot)return {{},Error::Conflict};auto row=node.begin();
  if(N(row[1])&&N(row[1])!=estate)return {{},Error::Conflict};
  auto decode=[](const char* p){if(!p)throw Failure{Error::DatabaseFailure};std::string s=p;if(s.size()%2||s.size()>RegionWire::MaximumPayload*2)throw Failure{Error::DatabaseFailure};std::vector<uint8_t> b;auto n=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t(n(s[i])*16+n(s[i+1])));return b;};
  if(!N(row[3]))return {{},Error::Conflict};auto bundle=Catalog::DecodePayloads(decode(row[4]),decode(row[5]));
  auto d=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& definition){return definition.id==N(row[2]);});if(d==bundle.definitions.end()||d->type!=4)return {{},Error::UnsupportedItem};
  if(N(row[1])==estate)return {false,Error::None};
  auto attached=Q(db,"UPDATE housing_native_estates SET ownership_estate_id="+std::to_string(estate)+" WHERE native_id="+std::to_string(native)+" AND plot_id="+std::to_string(plot)+" AND ownership_estate_id IS NULL AND active=1");if(attached.RowsAffected()!=1)return {{},Error::Conflict};
  return {true,Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
