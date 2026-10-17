#ifndef EQEMU_HOUSING_REGION_WIRE_H
#define EQEMU_HOUSING_REGION_WIRE_H
#include "estate_wire.h"
namespace Housing::RegionWire {
inline constexpr uint16_t BootstrapOpcode=0x49df;
inline constexpr size_t RecordSize=88;
// Server resource policy, not a recovered native transport maximum. The native
// reader consumes a DWORD count and replaces its whole collection per49df.
// Larger than64KiB application delivery must be verified with the exact client.
inline constexpr size_t MaximumPayload=262144;
struct Region {
 uint32_t id{};
 std::array<float,16> matrix{}; // Affine row matrix, native translation Y/X/Z at12/13/14.
 std::array<float,3> opaque_vector{};
 std::array<uint32_t,2> opaque_ids{0xffffffff,0xffffffff};
};
namespace Detail {
struct Writer {
 std::vector<uint8_t> bytes;
 void Word(uint32_t value){if(bytes.size()>MaximumPayload-4)throw std::invalid_argument("Region packet too large");NeighborhoodWire::Detail::Write32(bytes,value);}
 void Float(float value){if(!std::isfinite(value))throw std::invalid_argument("Nonfinite region value");uint32_t bits;std::memcpy(&bits,&value,4);Word(bits);}
};
inline float Product(float a,float b){float v=a*b;if(!std::isfinite(v))throw std::invalid_argument("Region cofactor overflow");return v;}
inline float Difference(float a,float b){float v=a-b;if(!std::isfinite(v))throw std::invalid_argument("Region inverse overflow");return v;}
inline float Sum(float a,float b){float v=a+b;if(!std::isfinite(v))throw std::invalid_argument("Region determinant overflow");return v;}
inline void ValidateMatrix(const std::array<float,16>& m){
 for(float v:m)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite region matrix");
 if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)throw std::invalid_argument("Region requires affine matrix");
 // Native inversion stores full-matrix minors/cofactors in float32 before division.
 for(size_t r0=0;r0<4;++r0)for(size_t r1=r0+1;r1<4;++r1)
  for(size_t c0=0;c0<4;++c0)for(size_t c1=c0+1;c1<4;++c1)
   Difference(Product(m[4*r0+c0],m[4*r1+c1]),Product(m[4*r0+c1],m[4*r1+c0]));
 for(size_t excluded_row=0;excluded_row<4;++excluded_row)
  for(size_t excluded_column=0;excluded_column<4;++excluded_column){
   std::array<float,9> sub{};size_t at=0;
   for(size_t row=0;row<4;++row)if(row!=excluded_row)
    for(size_t column=0;column<4;++column)if(column!=excluded_column)sub[at++]=m[4*row+column];
   const float first=Product(sub[0],Difference(Product(sub[4],sub[8]),Product(sub[5],sub[7])));
   const float second=Product(sub[1],Difference(Product(sub[3],sub[8]),Product(sub[5],sub[6])));
   const float third=Product(sub[2],Difference(Product(sub[3],sub[7]),Product(sub[4],sub[6])));
   Sum(Difference(first,second),third);
  }
 const float a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
 std::array<float,9> adj{
  Difference(Product(e,i),Product(f,h)),Difference(Product(c,h),Product(b,i)),Difference(Product(b,f),Product(c,e)),
  Difference(Product(f,g),Product(d,i)),Difference(Product(a,i),Product(c,g)),Difference(Product(c,d),Product(a,f)),
  Difference(Product(d,h),Product(e,g)),Difference(Product(b,g),Product(a,h)),Difference(Product(a,e),Product(b,d))};
 float determinant=Sum(Sum(Product(a,adj[0]),Product(b,adj[3])),Product(c,adj[6]));
 if(!determinant)throw std::invalid_argument("Singular or underflowing native region matrix");
 for(float& v:adj){v/=determinant;if(!std::isfinite(v))throw std::invalid_argument("Native region inverse overflow");}
 for(size_t j=0;j<3;++j)
  Sum(Sum(Product(m[12],adj[j]),Product(m[13],adj[j+3])),Product(m[14],adj[j+6]));
}
}
// Explicit test/import bounds only. This helper does not recover Live plot geometry.
inline Region AxisBoxServerXYZ(uint32_t id,std::array<float,3> center,std::array<float,3> half_extents){
 for(float v:half_extents)if(!(v>0)||!std::isfinite(v))throw std::invalid_argument("Invalid region extent");
 Region r{};r.id=id;r.matrix={half_extents[1],0,0,0,0,half_extents[0],0,0,0,0,half_extents[2],0,center[1],center[0],center[2],1};
 EstateWire::Detail::Identity(id);Detail::ValidateMatrix(r.matrix);return r;
}
inline std::vector<uint8_t> EncodeBootstrap(const std::vector<Region>& regions){
 if(regions.size()>(MaximumPayload-4)/RecordSize)throw std::invalid_argument("Region count exceeds payload");
 Detail::Writer w;w.Word(uint32_t(regions.size()));std::set<uint32_t> ids;
 for(const auto& r:regions){
  EstateWire::Detail::Identity(r.id);if(!ids.insert(r.id).second)throw std::invalid_argument("Duplicate region ID");
  Detail::ValidateMatrix(r.matrix);w.Word(r.id);for(float v:r.matrix)w.Float(v);
  for(float v:r.opaque_vector)w.Float(v);for(uint32_t v:r.opaque_ids)w.Word(v);
 }
 return std::move(w.bytes);
}
} // namespace Housing::RegionWire
#endif
