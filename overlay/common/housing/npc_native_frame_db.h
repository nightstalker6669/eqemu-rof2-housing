#ifndef EQEMU_HOUSING_NPC_NATIVE_FRAME_DB_H
#define EQEMU_HOUSING_NPC_NATIVE_FRAME_DB_H
#include "npc_native_frame.h"
#include "placement_policy.h"
#include "../database.h"
namespace Housing::NpcNativeFrame {
inline auto Query(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("NPC frame SQL");return r;}
inline bool Installed(Database& db){auto r=Query(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_npc_frames'");auto row=r.begin();if(!std::stoul(row[0]))return false;if(!row[1]||std::stoul(row[0])!=1||std::stoul(row[1])!=1)throw std::invalid_argument("NPC frame schema");return true;}
inline void Transaction(Database& db){auto r=Query(db,"SELECT @@in_transaction");if(r.RowCount()!=1||std::stoul(r.begin()[0])!=1)throw std::invalid_argument("NPC frame needs caller transaction");}
inline void Shape(const Frame& f,const PlacementPolicy::Transform& pose){NpcNativeHeight::Variants self{{f.height_bits,f.height_bits}};if(!Valid(self,pose.position[2],f))throw std::invalid_argument("NPC frame pose mismatch");}
inline std::optional<Frame> ReadUnderTransaction(Database& db,uint64_t custody,uint32_t native,uint64_t revision,const PlacementPolicy::Transform& pose){
 Transaction(db);if(!Installed(db))return {};auto r=Query(db,"SELECT native_item_id,pose_revision,height_bits,origin_z_bits,active FROM housing_live_npc_frames WHERE custody_id="+std::to_string(custody)+" FOR UPDATE");if(!r.RowCount())return {};if(r.RowCount()!=1)throw std::invalid_argument("NPC frame multiplicity");auto row=r.begin();for(size_t i=0;i<5;++i)if(!row[i])throw std::invalid_argument("Incomplete NPC frame");if(std::stoul(row[0])!=native||std::stoull(row[1])!=revision||std::stoul(row[4])!=1)throw std::invalid_argument("NPC frame identity");Frame f{uint32_t(std::stoul(row[2])),uint32_t(std::stoul(row[3]))};Shape(f,pose);return f;
}
inline void InsertUnderTransaction(Database& db,uint64_t custody,uint32_t native,uint64_t revision,const PlacementPolicy::Transform& pose,const std::optional<Frame>& frame){
 Transaction(db);if(!frame)return;Shape(*frame,pose);auto r=Query(db,"INSERT INTO housing_live_npc_frames(custody_id,native_item_id,pose_revision,height_bits,origin_z_bits,active) VALUES("+std::to_string(custody)+","+std::to_string(native)+","+std::to_string(revision)+","+std::to_string(frame->height_bits)+","+std::to_string(frame->origin_z_bits)+",1)");if(r.RowsAffected()!=1)throw std::invalid_argument("NPC frame insert");
}
inline void ReplaceUnderTransaction(Database& db,uint64_t custody,uint32_t old_native,uint64_t old_revision,uint32_t native,uint64_t revision,const PlacementPolicy::Transform& pose,const std::optional<Frame>& old_frame,const std::optional<Frame>& frame){
 Transaction(db);if(!old_frame){InsertUnderTransaction(db,custody,native,revision,pose,frame);return;}if(!frame)throw std::invalid_argument("NPC frame cannot disappear");Shape(*frame,pose);auto r=Query(db,"UPDATE housing_live_npc_frames SET native_item_id="+std::to_string(native)+",pose_revision="+std::to_string(revision)+",height_bits="+std::to_string(frame->height_bits)+",origin_z_bits="+std::to_string(frame->origin_z_bits)+" WHERE custody_id="+std::to_string(custody)+" AND native_item_id="+std::to_string(old_native)+" AND pose_revision="+std::to_string(old_revision)+" AND active=1 AND height_bits="+std::to_string(old_frame->height_bits)+" AND origin_z_bits="+std::to_string(old_frame->origin_z_bits));if(r.RowsAffected()!=1)throw std::invalid_argument("NPC frame CAS");
}
inline void RetireUnderTransaction(Database& db,uint64_t custody,uint32_t native,uint64_t revision,const std::optional<Frame>& frame){Transaction(db);if(!frame)return;auto r=Query(db,"UPDATE housing_live_npc_frames SET active=0 WHERE custody_id="+std::to_string(custody)+" AND native_item_id="+std::to_string(native)+" AND pose_revision="+std::to_string(revision)+" AND active=1 AND height_bits="+std::to_string(frame->height_bits)+" AND origin_z_bits="+std::to_string(frame->origin_z_bits));if(r.RowsAffected()!=1)throw std::invalid_argument("NPC frame retire");}
inline std::optional<Frame> Reframe(const Frame& old,const PlacementPolicy::Transform& pose){return FromSubmitted({{old.height_bits,old.height_bits}},pose.position[2],NpcNativeHeight::Float(old.height_bits));}
}
#endif
