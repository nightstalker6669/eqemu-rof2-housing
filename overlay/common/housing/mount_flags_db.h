#ifndef EQEMU_HOUSING_MOUNT_FLAGS_DB_H
#define EQEMU_HOUSING_MOUNT_FLAGS_DB_H
#include "../database.h"
#include <stdexcept>
namespace Housing::MountFlags {
struct Flags {bool known=false,disable_rotation=false,disable_free=false;};
inline Flags ReadUnderTransaction(Database& db,uint32_t item){
 auto q=[&](const std::string&s){auto r=db.QueryDatabase(s,false);if(!r.Success())throw std::invalid_argument("Mount flags query");return r;};
 auto n=q("SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='items' AND column_name IN ('housing_disable_rotation','housing_disable_free_placement')");if(n.RowCount()!=1||!n.begin()[0])throw std::invalid_argument("Mount flags schema");auto count=std::stoul(n.begin()[0]);if(!count)return {};if(count!=2)throw std::invalid_argument("Partial mount flags schema");
 auto rows=q("SELECT housing_disable_rotation,housing_disable_free_placement FROM items WHERE id="+std::to_string(item)+" FOR UPDATE");if(rows.RowCount()!=1)throw std::invalid_argument("Mount flags item");auto r=rows.begin();if(!r[0]||!r[1])return {};auto a=std::stol(r[0]),b=std::stol(r[1]);if(a<0||a>1||b<0||b>1)throw std::invalid_argument("Mount flags range");return {true,bool(a),bool(b)};
}
}
#endif
