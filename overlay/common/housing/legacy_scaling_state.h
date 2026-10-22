#ifndef EQEMU_HOUSING_LEGACY_SCALING_STATE_H
#define EQEMU_HOUSING_LEGACY_SCALING_STATE_H
#include "../item_instance.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace Housing::LegacyScaling {
// Server persistence extension, not an original client field or CEI progress.
// Kept in existing per-instance custom_data so normal saves, custody capsules
// and parcel/offline returns carry one identical, transactional representation.
inline constexpr const char* Key="eqemu.housing.legacy_scaling.v1";
struct State {uint32_t component=0,item=0,experience=0;bool operator==(const State& o)const{return component==o.component&&item==o.item&&experience==o.experience;}};
inline EQ::ItemInstance* Component(EQ::ItemInstance& item,unsigned n){return n==0?&item:(item.GetUnscaledItem()&&item.GetUnscaledItem()->IsClassCommon()?item.GetAugment(uint8_t(n-1)):nullptr);}
inline std::vector<State> Current(const EQ::ItemInstance& value){
 auto& item=const_cast<EQ::ItemInstance&>(value);std::vector<State> out;
 for(unsigned n=0;n<7;++n)if(auto* p=Component(item,n);p&&p->IsScaling())out.push_back({n,p->GetID(),p->GetExp()});return out;
}
inline std::string Encode(const std::vector<State>& states){std::string out="1";for(const auto& s:states)out+=";"+std::to_string(s.component)+":"+std::to_string(s.item)+":"+std::to_string(s.experience);return out;}
inline std::optional<std::vector<State>> Decode(const std::string& raw){
 if(raw.size()>256||raw.empty()||raw[0]!='1')return {};std::vector<State> out;size_t at=1;
 auto number=[&](char end,uint32_t& v){const auto first=at;while(at<raw.size()&&raw[at]>='0'&&raw[at]<='9')++at;if(first==at||(at-first>1&&raw[first]=='0'))return false;auto r=std::from_chars(raw.data()+first,raw.data()+at,v);if(r.ec!=std::errc{}||r.ptr!=raw.data()+at)return false;if(end){if(at==raw.size()||raw[at++]!=end)return false;}return true;};
 while(at<raw.size()){if(raw[at++]!=';'||out.size()==7)return {};State state;if(!number(':',state.component)||!number(':',state.item)||!number(0,state.experience)||state.component>6||!state.item||(!out.empty()&&state.component<=out.back().component))return {};out.push_back(state);}
 if(out.empty())return {};return out;
}
// Accept the historical writer's double separator between pairs and the
// single-separator form, without dropping empty values or duplicate fields.
inline std::optional<std::map<std::string,std::string>> Fields(const std::string& custom){
 std::map<std::string,std::string> fields;size_t at=0;
 while(at<custom.size()){auto end=custom.find('^',at);if(end==std::string::npos||end==at)return {};const auto key=custom.substr(at,end-at);at=end+1;end=custom.find('^',at);if(end==std::string::npos)end=custom.size();const auto value=custom.substr(at,end-at);at=end<custom.size()?end+1:end;
  if(!fields.emplace(key,value).second)return {};if(at<custom.size()&&custom[at]=='^'){if(++at==custom.size())return {};}}
 return fields;
}
inline bool Saved(const std::string& custom,std::optional<std::vector<State>>& states){
 states.reset();if(custom.find(Key)==std::string::npos)return true;auto fields=Fields(custom);if(!fields)return false;auto found=fields->find(Key);if(found==fields->end())return true;states=Decode(found->second);return bool(states);
}

inline std::string Write(const EQ::ItemInstance& item,std::map<std::string,std::string> fields){
 const auto states=Current(item);if(states.empty())fields.erase(Key);else fields[Key]=Encode(states);
 std::string out;for(const auto& [key,value]:fields){if(!out.empty())out+='^';out+=key;out+='^';out+=value;out+='^';}return out;
}
inline bool Persistable(const EQ::ItemInstance& item){
 if(item.GetCustomDataString().size()>65535)return false;
 for(const auto& [slot,child]:*const_cast<EQ::ItemInstance&>(item).GetContents())if(!child||!Persistable(*child))return false;return true;
}
inline bool Restore(EQ::ItemInstance& item,const std::string& custom){
 std::optional<std::vector<State>> states;if(!Saved(custom,states))return false;if(!states)return true;
 const auto current=Current(item);if(current.size()!=states->size())return false;
 for(size_t i=0;i<current.size();++i)if(current[i].component!=(*states)[i].component||current[i].item!=(*states)[i].item)return false;
 for(const auto& state:*states){auto* p=Component(item,state.component);p->SetExp(state.experience);p->ScaleItem();}return true;
}

inline bool Matches(const std::string& custom,const EQ::ItemInstance& item){std::optional<std::vector<State>> saved;if(!Saved(custom,saved))return false;const auto current=Current(item);return current.empty()?!saved:(saved&&*saved==current);}
inline bool Metadata(const std::string& custom,const std::array<uint32_t,7>& required){std::optional<std::vector<State>> saved;if(!Saved(custom,saved))return false;size_t count=0;for(unsigned i=0;i<required.size();++i)if(required[i]){++count;if(!saved)return false;auto found=std::find_if(saved->begin(),saved->end(),[&](const State& st){return st.component==i&&st.item==required[i];});if(found==saved->end())return false;}return count==(saved?saved->size():0);}
}
#endif
