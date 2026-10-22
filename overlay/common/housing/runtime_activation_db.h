#ifndef EQEMU_HOUSING_RUNTIME_ACTIVATION_DB_H
#define EQEMU_HOUSING_RUNTIME_ACTIVATION_DB_H
#include "../database.h"
#include <string>
namespace Housing::RuntimeActivation {
// Reconstructed content is an explicit installation policy, never provenance.
// Historical fixture namespaces retain compatibility; fixture funding/actors
// keep their independent restrictions and must not use this authorization.
inline bool AllowsReconstructedContent(Database& db){
 auto name=db.QueryDatabase(std::string("SELECT DATABASE()"),false);
 if(!name.Success()||name.RowCount()!=1||!name.begin()[0])return false;
 const std::string current=name.begin()[0];
 if(current=="peq_housing_test"||current.rfind("peq_housing_test_",0)==0)return true;
 auto table=db.QueryDatabase(std::string("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_runtime_activation' AND engine='InnoDB'"),false);
 if(!table.Success()||table.RowCount()!=1||std::string(table.begin()[0])!="1")return false;
 auto row=db.QueryDatabase(std::string("SELECT database_name,client_build,policy_revision,allow_reconstructed_content FROM housing_runtime_activation WHERE id=1"),false);
 if(!row.Success()||row.RowCount()!=1)return false;auto r=row.begin();
 return r[0]&&current==r[0]&&r[1]&&std::string(r[1])=="20130510"&&r[2]&&std::string(r[2])=="1"&&r[3]&&std::string(r[3])=="1";
}
}
#endif
