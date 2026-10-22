#ifndef EQEMU_HOUSING_CUSTODY_CODEC_H
#define EQEMU_HOUSING_CUSTODY_CODEC_H
#include "live_item_service.h"
#include "../emu_constants.h"
#include <cstring>
#include <stdexcept>
namespace Housing::CustodyCodec {
using LiveItems::InventoryRow;using namespace LiveItems;using Bytes=std::vector<uint8_t>;
inline constexpr size_t MaximumPayload=16*1024*1024-1,MaximumEvolvingPayload=4096;
inline constexpr size_t MaximumBagSlots=EQ::invbag::SLOT_COUNT;
static_assert(MaximumBagSlots<=UINT8_MAX,"Canonical bag child count must fit its native byte");
        inline void W(Bytes& b,uint64_t v,unsigned n){
            for(unsigned i=0;i<n;++i)b.push_back(uint8_t(v>>(8*i)));
        }
        inline void S(Bytes& b,const std::string& s){
            W(b,s.size(),4);
            b.insert(b.end(),s.begin(),s.end());
        }
        struct Reader{
            const Bytes& b;
            size_t at=0;
            uint64_t N(size_t n){
                if(n>b.size()-at)throw std::invalid_argument("Invalid canonical housing custody row");
                uint64_t v=0;
                for(size_t i=0;i<n;++i)v|=uint64_t(b[at++])<<(8*i);
                return v;
            }
            std::string Text(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw std::invalid_argument("Invalid canonical housing custody row");
                std::string s(b.begin()+at,b.begin()+at+n);
                at+=n;
                return s;
            }
            Bytes Blob(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw std::invalid_argument("Invalid canonical housing custody row");
                Bytes out(b.begin()+at,b.begin()+at+n);
                at+=n;
                return out;
            }
        };
        inline bool Valid(const InventoryRow& r){
            const auto& v=r.values;
            if((r.null_fields&~uint8_t(15))||v.custom_data.size()>65535||((r.null_fields&NullItemId)&&v.item_id)||((r.null_fields&NullCharges)&&v.charges)||((r.null_fields&NullCustomData)&&!v.custom_data.empty())||((r.null_fields&NullGuid)&&v.guid)||r.evolving_payload.size()>MaximumEvolvingPayload||r.children.size()>MaximumBagSlots)return false;
            int prior=-1;for(const auto& child:r.children){if(!child.children.empty()||child.values.character_id!=v.character_id||child.values.slot_id>=MaximumBagSlots||int(child.values.slot_id)<=prior||!Valid(child))return false;prior=int(child.values.slot_id);}return true;
        }
        inline Bytes Encode(const InventoryRow& r){
            Bytes b;
            if(!Valid(r))throw std::invalid_argument("Invalid canonical housing custody row");
            const bool extended=!r.children.empty()||!r.evolving_payload.empty();
            W(b,extended?2:1,4);
            W(b,r.null_fields,1);
            const auto& v=r.values;
            for(auto x:{
                v.character_id,v.slot_id,v.item_id
            })W(b,x,4);
            W(b,v.charges,2);
            for(auto x:{
                v.color,v.augment_one,v.augment_two,v.augment_three,v.augment_four,v.augment_five,v.augment_six
            })W(b,x,4);
            W(b,v.instnodrop,1);
            S(b,v.custom_data);
            W(b,v.ornament_icon,4);
            W(b,v.ornament_idfile,4);
            W(b,uint32_t(v.ornament_hero_model),4);
            W(b,v.guid,8);
            if(extended){W(b,r.evolving_payload.size(),4);b.insert(b.end(),r.evolving_payload.begin(),r.evolving_payload.end());W(b,r.children.size(),1);for(const auto& child:r.children){auto bytes=Encode(child);W(b,bytes.size(),4);b.insert(b.end(),bytes.begin(),bytes.end());}}
            if(b.size()>MaximumPayload)throw std::invalid_argument("Housing custody bundle exceeds native limit");
            return b;
        }
        inline InventoryRow DecodeBounded(const Bytes& b,unsigned depth){
            if(b.size()>MaximumPayload||depth>1)throw std::invalid_argument("Invalid custody bundle bounds");
            Reader r{
                b
            };
            const auto version=r.N(4);if(version!=1&&version!=2)throw std::invalid_argument("Invalid canonical housing custody row");
            InventoryRow out;
            out.null_fields=uint8_t(r.N(1));
            auto& v=out.values;
            v.character_id=uint32_t(r.N(4));
            v.slot_id=uint32_t(r.N(4));
            v.item_id=uint32_t(r.N(4));
            v.charges=uint16_t(r.N(2));
            v.color=uint32_t(r.N(4));
            v.augment_one=uint32_t(r.N(4));
            v.augment_two=uint32_t(r.N(4));
            v.augment_three=uint32_t(r.N(4));
            v.augment_four=uint32_t(r.N(4));
            v.augment_five=uint32_t(r.N(4));
            v.augment_six=uint32_t(r.N(4));
            v.instnodrop=uint8_t(r.N(1));
            v.custom_data=r.Text(65535);
            v.ornament_icon=uint32_t(r.N(4));
            v.ornament_idfile=uint32_t(r.N(4));
            auto hero=uint32_t(r.N(4));
            std::memcpy(&v.ornament_hero_model,&hero,4);
            v.guid=r.N(8);
            if(version==2){out.evolving_payload=r.Blob(MaximumEvolvingPayload);const auto count=r.N(1);if(count>MaximumBagSlots||(depth&&count))throw std::invalid_argument("Invalid native bag contents");for(size_t i=0;i<count;++i)out.children.push_back(DecodeBounded(r.Blob(MaximumPayload),depth+1));if(out.evolving_payload.empty()&&out.children.empty())throw std::invalid_argument("Noncanonical empty custody extension");}
            if(r.at!=b.size()||!Valid(out))throw std::invalid_argument("Invalid canonical housing custody row");
            return out;
        }
        inline InventoryRow Decode(const Bytes& b){return DecodeBounded(b,0);}
}
#endif
