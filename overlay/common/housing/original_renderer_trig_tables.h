#ifndef EQEMU_HOUSING_ORIGINAL_RENDERER_TRIG_TABLES_H
#define EQEMU_HOUSING_ORIGINAL_RENDERER_TRIG_TABLES_H
#include "original_actor_local_query.h"
#include <openssl/evp.h>
namespace Housing::OriginalActorLocalQuery {
// Original100b9680 construction: nearest FSTP32, x87 FCOS, exact original
// PI binary64 and1/256. Native captures at two poses give this same table SHA.
// PC53/PC64 both reproduced all1024 DWORDs; this bounded helper chooses PC53.
// CPU-dependent FCOS discrepancies REFUSE through the digest, never silently
// install different entries. No original asset/table blob is embedded here.
inline std::optional<TrigTables> OriginalRendererTables(){
 Detail::Environment environment;if(!environment.good||std::fesetround(FE_TONEAREST)!=0)return {};
 try{TrigTables out;constexpr double pi=std::bit_cast<double>(uint64_t(0x400921fb54411744));for(unsigned i=0;i<512;++i){const auto angle=Detail::product(Detail::product(i,pi),.00390625);volatile long double value=Detail::cosine(angle);volatile float stored=float(value);out[i]=stored;out[512+((i+128)&511)]=stored;}
 out[512]=0;out[640]=1;out[768]=0;out[896]=-1;out[256]=-1;out[0]=1;out[128]=0;out[384]=0;
 // Canonical little-endian DWORD digest, independent of host byte order.
 std::array<uint8_t,4096> bytes;for(unsigned i=0;i<1024;++i){const auto word=std::bit_cast<uint32_t>(out[i]);for(unsigned k=0;k<4;++k)bytes[i*4+k]=uint8_t(word>>(8*k));}
 std::array<unsigned char,32> digest{};unsigned size=0;if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)!=1||size!=32)return {};constexpr char alphabet[]="0123456789abcdef";std::string sha;for(auto b:digest){sha+=alphabet[b>>4];sha+=alphabet[b&15];}if(sha!="dff72501eac80cb924233dc31a7eaa74725872a283903b8ad8d601d2a0ecf779")return {};return out;
 }catch(...){return {};}
}
}
#endif
