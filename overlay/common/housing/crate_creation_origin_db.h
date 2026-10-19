#ifndef EQEMU_HOUSING_CRATE_CREATION_ORIGIN_DB_H
#define EQEMU_HOUSING_CRATE_CREATION_ORIGIN_DB_H
#include "../database.h"
#include <stdexcept>
namespace Housing::CrateCreationOrigin {
inline MySQLRequestResult Q(Database& db,const std::string& s){auto r=db.QueryDatabase(s,false);if(!r.Success())throw std::invalid_argument("Crate creation origin SQL failed");return r;}
inline bool Installed(Database& db){auto r=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_creation_origins'");if(!r.RowCount())return false;if(r.RowCount()!=1||!r.begin()[0]||std::string(r.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid crate creation origin schema");return true;}
inline std::string S(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
inline void Live(Database& db,const std::string& key){if(!Installed(db))return;auto r=Q(db,"INSERT INTO housing_crate_creation_origins(request_key,kind,live_request_key,offline_estate_id) VALUES("+S(db,key)+",1,"+S(db,key)+",NULL)");if(r.RowsAffected()!=1)throw std::invalid_argument("Crate live origin conflict");}
inline void Offline(Database& db,const std::string& key,uint32_t estate){if(!Installed(db))throw std::invalid_argument("Offline crate origin schema missing");if(key!="offline-inactivity:"+std::to_string(estate))throw std::invalid_argument("Offline crate origin key mismatch");auto r=Q(db,"INSERT INTO housing_crate_creation_origins(request_key,kind,live_request_key,offline_estate_id) VALUES("+S(db,key)+",2,NULL,"+std::to_string(estate)+")");if(r.RowsAffected()!=1)throw std::invalid_argument("Crate offline origin conflict");}
// Guild recipient identity is recorded immutably at creation; later leadership
// or membership changes never rewrite the owner of delivered property.
inline void OfflineGuild(Database& db,const std::string& key,uint32_t estate,uint32_t creator){
 if(!Installed(db)||!creator||key!="offline-guild-inactivity:"+std::to_string(estate))throw std::invalid_argument("Invalid guild crate creation origin");
 auto r=Q(db,"INSERT INTO housing_crate_creation_origins(request_key,kind,live_request_key,offline_estate_id,offline_character_id) SELECT "+S(db,key)+",3,NULL,id,"+std::to_string(creator)+" FROM housing_current_estates WHERE id="+std::to_string(estate)+" AND owner_kind=3");if(r.RowsAffected()!=1)throw std::invalid_argument("Guild crate origin conflict");
}
// Multiple bins retain their original recipient even after the source guild or
// ownership generation has ended. The caller owns the authorized batch TX.
inline void OfflineReturn(Database& db,const std::string& key,uint32_t estate,uint32_t recipient,uint32_t ordinal){
 if(!Installed(db)||!recipient||!ordinal||ordinal>32||key!="offline-return:"+std::to_string(estate)+":"+std::to_string(recipient)+":"+std::to_string(ordinal))throw std::invalid_argument("Invalid batch crate origin");
 auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw std::invalid_argument("Batch origin requires transaction");
 auto r=Q(db,"INSERT INTO housing_crate_creation_origins(request_key,kind,live_request_key,offline_estate_id,offline_character_id,offline_return_ordinal) SELECT "+S(db,key)+",4,NULL,id,"+std::to_string(recipient)+","+std::to_string(ordinal)+" FROM housing_current_estates WHERE id="+std::to_string(estate)+" AND owner_kind IN(1,3)");if(r.RowsAffected()!=1)throw std::invalid_argument("Batch crate origin conflict");
}
// Creator provenance is historical; current source-property access is not a
// reload/placement grant. Reparenting does not rewrite the creation generation.
inline void Validate(Database& db,const std::string& key,uint32_t creator){
 if(!Installed(db))return;auto r=Q(db,"SELECT o.kind,o.live_request_key,o.offline_estate_id,l.character_id,e.owner_kind,e.owner_id FROM housing_crate_creation_origins o LEFT JOIN housing_live_item_receipts l ON l.request_key=o.live_request_key LEFT JOIN housing_estates e ON e.id=o.offline_estate_id WHERE o.request_key="+S(db,key)+" FOR UPDATE");if(r.RowCount()!=1)throw std::invalid_argument("Missing crate creation origin");auto p=r.begin();if(!p[0])throw std::invalid_argument("Invalid crate origin kind");
 if(std::string(p[0])=="1"){if(!p[1]||std::string(p[1])!=key||p[2]||!p[3]||std::stoull(p[3])!=creator)throw std::invalid_argument("Invalid live crate origin");}
 else if(std::string(p[0])=="2"){if(p[1]||!p[2]||!p[4]||std::string(p[4])!="1"||!p[5]||std::stoull(p[5])!=creator||key!="offline-inactivity:"+std::string(p[2]))throw std::invalid_argument("Invalid offline crate origin");}
 else if(std::string(p[0])=="3"){
  if(p[1]||!p[2]||!p[4]||std::string(p[4])!="3"||key!="offline-guild-inactivity:"+std::string(p[2]))throw std::invalid_argument("Invalid guild crate origin");
  auto bound=Q(db,"SELECT offline_character_id FROM housing_crate_creation_origins WHERE request_key="+S(db,key)+" FOR UPDATE");if(bound.RowCount()!=1||!bound.begin()[0]||std::stoull(bound.begin()[0])!=creator)throw std::invalid_argument("Guild crate creator mismatch");
 }
 else if(std::string(p[0])=="4"){
  if(p[1]||!p[2]||!p[4]||(std::string(p[4])!="1"&&std::string(p[4])!="3"))throw std::invalid_argument("Invalid batch crate origin");
  auto bound=Q(db,"SELECT offline_character_id,offline_return_ordinal FROM housing_crate_creation_origins WHERE request_key="+S(db,key)+" FOR UPDATE");if(bound.RowCount()!=1||!bound.begin()[0]||!bound.begin()[1]||std::stoull(bound.begin()[0])!=creator||!std::stoull(bound.begin()[1])||std::stoull(bound.begin()[1])>32||key!="offline-return:"+std::string(p[2])+":"+std::string(bound.begin()[0])+":"+std::string(bound.begin()[1]))throw std::invalid_argument("Batch crate recipient mismatch");
 }
 else throw std::invalid_argument("Unsupported crate origin kind");
}
}
#endif
