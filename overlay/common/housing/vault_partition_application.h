#ifndef EQEMU_HOUSING_VAULT_PARTITION_APPLICATION_H
#define EQEMU_HOUSING_VAULT_PARTITION_APPLICATION_H
#include "stack_partition_application.h"
namespace Housing::VaultPartitionApplication {
using StackPartitionApplication::Owners;using StackPartitionApplication::Pieces;
inline LiveItems::InventoryRow Piece(const LiveItems::StackPartition& p){auto row=p.before;row.values.charges=p.quantity;row.values.guid=p.serial;row.null_fields&=~uint8_t(LiveItems::NullCharges|LiveItems::NullGuid);return row;}
inline bool Coherent(const LiveItems::VaultTransferRequest& request,const LiveItems::VaultTransferReceipt& result){
 if(!result.partition||!request.partition_quantity||!request.partition_serial||!request.custody_id||!result.custody_id||request.custody_id==result.custody_id||result.native_item_id==request.from.expected_native_item_id||result.native_estate_id!=request.to.native_estate_id||result.revision!=1||!result.custody_estate_id||result.custody_estate_id==UINT32_MAX)return false;const auto& p=*result.partition;auto after=p.before;const auto n=after.values.charges;if(!n||n>=32767||p.quantity!=request.partition_quantity||p.quantity>=n||p.serial!=request.partition_serial||p.serial>INT32_MAX||p.serial==p.before.values.guid||p.maximum<n||p.maximum>=32767||p.room_source||!p.vault_source||p.stowed||p.storage_before!=request.from.expected_revision||p.storage_before==UINT64_MAX||p.storage_after!=p.storage_before+1||CustodyCodec::Encode(request.expected)!=CustodyCodec::Encode(p.before))return false;after.values.charges=n-p.quantity;after.null_fields&=~LiveItems::NullCharges;return CustodyCodec::Encode(after)==CustodyCodec::Encode(p.after);
}
inline bool Applied(const LiveItems::VaultTransferRequest& request,const LiveItems::VaultTransferReceipt& result,const Owners& owners){
 if(!Coherent(request,result))return false;auto source=owners.find(request.custody_id),child=owners.find(result.custody_id);return source!=owners.end()&&child!=owners.end()&&source->second&&child->second&&source->second.get()!=child->second.get()&&InventoryProjection::Matches(result.partition->after,*source->second)&&InventoryProjection::Matches(Piece(*result.partition),*child->second)&&uint32_t(child->second->GetSerialNumber())==result.partition->serial;
}
// Custody owners only. This never inserts/debits inventory or a cursor. Commit
// uncertainty retains both server-held owners until exact journal resolution.
inline bool Apply(const LiveItems::VaultTransferRequest& request,const LiveItems::VaultTransferReceipt& result,Owners& owners,Pieces& pieces){
 if(Applied(request,result,owners))return true;if(!Coherent(request,result))return false;auto source=owners.find(request.custody_id),child=owners.find(result.custody_id);auto piece=pieces.find(request.request_key);if(source==owners.end()||!source->second||(child!=owners.end()&&child->second)||piece==pieces.end()||!piece->second||uint32_t(piece->second->GetSerialNumber())!=result.partition->serial||!InventoryProjection::Matches(Piece(*result.partition),*piece->second)||(!InventoryProjection::Matches(result.partition->before,*source->second)&&!InventoryProjection::Matches(result.partition->after,*source->second)))return false;if(child==owners.end())child=owners.emplace(result.custody_id,nullptr).first;source->second->SetCharges(result.partition->after.values.charges);child->second=std::move(piece->second);pieces.erase(piece);return Applied(request,result,owners);
}
}
#endif
