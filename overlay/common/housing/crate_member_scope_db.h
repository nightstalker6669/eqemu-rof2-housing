#ifndef EQEMU_HOUSING_CRATE_MEMBER_SCOPE_DB_H
#define EQEMU_HOUSING_CRATE_MEMBER_SCOPE_DB_H
#include "house_item_scope_db.h"
namespace Housing::CrateMemberScope {
struct Membership {
 uint64_t crate_id=0,revision=0;
 uint32_t origin_native_estate_id=0;
 std::vector<uint8_t> origin_pose;
 bool origin_stowed=false;
 uint64_t origin_house_id=0;
};
inline bool Installed(Database& db){
 auto r=HouseItemScope::Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_members'");
 if(!r.RowCount())return false;
 if(r.RowCount()!=1||!r.begin()[0]||std::string(r.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid crate member schema");
 return true;
}
inline std::optional<Membership> Locked(Database& db,uint64_t custody,bool allow_room=false){
 if(!custody)throw std::invalid_argument("Invalid crate custody");if(!Installed(db))return {};
 auto r=HouseItemScope::Query(db,"SELECT crate_id,revision,origin_native_estate_id,HEX(origin_pose_blob),origin_stowed,origin_house_id FROM housing_crate_members WHERE custody_id="+std::to_string(custody)+" FOR UPDATE");
 if(!r.RowCount())return {};if(r.RowCount()!=1)throw std::invalid_argument("Ambiguous crate member");auto row=r.begin();
 for(size_t i=0;i<5;++i)if(!row[i])throw std::invalid_argument("Incomplete crate member");
 Membership m;m.crate_id=std::stoull(row[0]);m.revision=std::stoull(row[1]);m.origin_native_estate_id=std::stoul(row[2]);
 const std::string hex=row[3];if(hex.size()!=56||!m.crate_id||!m.revision||!m.origin_native_estate_id||m.origin_native_estate_id==UINT32_MAX||std::stoul(row[4])>1)throw std::invalid_argument("Invalid crate member identity");
 // Interior members require their separate house-scope restoration adapter.
 if(row[5]){if(!allow_room||!(m.origin_house_id=std::stoull(row[5])))throw std::invalid_argument("Unreviewed crate house scope");}
 auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Invalid crate pose encoding");};
 for(size_t i=0;i<hex.size();i+=2)m.origin_pose.push_back((nibble(hex[i])<<4)|nibble(hex[i+1]));
 m.origin_stowed=std::stoul(row[4]);return m;
}
// List-only members retain their original room scope; their budget belongs to
// the child estate, just like physical crate members. Alias is server-owned.
inline std::string VaultPredicate(Database& db,const std::string& alias="c"){
 auto rows=HouseItemScope::Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_vault_members'");
 if(!rows.RowCount())return {};
 if(rows.RowCount()!=1||!rows.begin()[0]||std::string(rows.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid crate vault member schema");
 return " AND NOT EXISTS (SELECT 1 FROM housing_crate_vault_members vault_scope WHERE vault_scope.custody_id="+alias+".custody_id)";
}
inline std::string ExteriorPredicate(Database& db){return Installed(db)?" AND NOT EXISTS (SELECT 1 FROM housing_crate_members crate_scope WHERE crate_scope.custody_id=c.custody_id)":"";}
}
#endif
