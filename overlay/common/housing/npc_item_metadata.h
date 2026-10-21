#ifndef EQEMU_HOUSING_NPC_ITEM_METADATA_H
#define EQEMU_HOUSING_NPC_ITEM_METADATA_H
#include "item_wire.h"
#include "npc_reviewed_item_bindings.h"
#include "service_item_bindings.h"
#include "../item_data.h"
#include <algorithm>
#include <map>
namespace Housing::NpcItemMetadata {
struct Source {uint32_t item_id=0;std::string filename;bool interactive=false;uint32_t respawn_interval=0;std::string source_sha256;float actor_height=0;};
inline bool Digest(const std::string& value){return value.size()==64&&value.find_first_not_of("0123456789abcdef")==std::string::npos&&value.find_first_not_of('0')!=std::string::npos;}
inline bool Filename(const std::string& value){return value.size()<=ItemWire::MaximumFilename&&std::all_of(value.begin(),value.end(),[](unsigned char c){return c>=32&&c<=126;});}
inline bool Valid(const Source& source){return source.item_id&&source.item_id<=INT32_MAX&&Filename(source.filename)&&Digest(source.source_sha256)&&std::isfinite(source.actor_height)&&source.actor_height>=0;}
inline void Clear(EQ::ItemData& item){std::fill(std::begin(item.HousingHousepetFilename),std::end(item.HousingHousepetFilename),0);item.HousingInteractiveObject=0;item.HousingNpcRespawnInterval=0;item.HousingNpcMetadataKnown=false;item.HousingNpcActorHeight=0;std::fill(std::begin(item.HousingNpcSourceSha256),std::end(item.HousingNpcSourceSha256),0);}
inline void Apply(EQ::ItemData& item,const Source& source){
 if(!Valid(source)||item.ID!=source.item_id)throw std::invalid_argument("Invalid reviewed NPC item metadata source");
 Clear(item);std::copy(source.filename.begin(),source.filename.end(),item.HousingHousepetFilename);item.HousingInteractiveObject=source.interactive;item.HousingNpcRespawnInterval=source.respawn_interval;item.HousingNpcMetadataKnown=true;item.HousingNpcActorHeight=source.actor_height;std::copy(source.source_sha256.begin(),source.source_sha256.end(),item.HousingNpcSourceSha256);
}
inline bool Ordinary(const Source& source){return source.filename.empty()&&!source.interactive&&!source.respawn_interval;}
inline bool Ordinary(const EQ::ItemData& item){return !item.HousingHousepetFilename[0]&&!item.HousingInteractiveObject&&!item.HousingNpcRespawnInterval;}
inline void ToWire(ItemWire::Metadata& metadata,const Source& source){if(!Valid(source))throw std::invalid_argument("Invalid reviewed NPC item metadata source");metadata.housepet_filename=source.filename;metadata.interactive=source.interactive;metadata.respawn_interval=source.respawn_interval;if(const auto* binding=NpcReviewedBindings::Lookup(source.item_id);binding&&NpcReviewedBindings::Matches(*binding,source.filename,source.interactive,source.respawn_interval,source.source_sha256,source.actor_height))metadata.default_scale=1.0f;if(const auto* service=ServiceItemBindings::Lookup(source.item_id);service&&ServiceItemBindings::Matches(*service,source.filename,source.interactive,source.respawn_interval,source.source_sha256,source.actor_height))metadata.default_scale=1.0f;}
inline void ToWire(ItemWire::Metadata& metadata,const EQ::ItemData& item){
 metadata.housepet_filename.clear();metadata.interactive=false;metadata.respawn_interval=0;
 if(!item.HousingNpcMetadataKnown){if(!Ordinary(item))throw std::invalid_argument("Unreviewed NPC item metadata");return;}
 const auto end=std::find(std::begin(item.HousingHousepetFilename),std::end(item.HousingHousepetFilename),char(0));
 if(end==std::end(item.HousingHousepetFilename)||item.HousingInteractiveObject>1)throw std::invalid_argument("Invalid cached NPC item metadata");
 std::string filename(item.HousingHousepetFilename,end);if(!Filename(filename))throw std::invalid_argument("Invalid cached NPC item filename");metadata.housepet_filename=std::move(filename);metadata.interactive=item.HousingInteractiveObject!=0;metadata.respawn_interval=item.HousingNpcRespawnInterval;if(const auto* binding=NpcReviewedBindings::Lookup(item.ID);binding&&NpcReviewedBindings::Matches(*binding,metadata.housepet_filename,metadata.interactive,metadata.respawn_interval,std::string_view(item.HousingNpcSourceSha256,64),item.HousingNpcActorHeight))metadata.default_scale=1.0f;if(const auto* service=ServiceItemBindings::Lookup(item.ID);service&&ServiceItemBindings::Matches(*service,metadata.housepet_filename,metadata.interactive,metadata.respawn_interval,std::string_view(item.HousingNpcSourceSha256,64),item.HousingNpcActorHeight))metadata.default_scale=1.0f;
}
}
#endif
