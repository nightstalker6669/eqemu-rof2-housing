#ifndef EQEMU_HOUSING_TROPHY_LOCATION_H
#define EQEMU_HOUSING_TROPHY_LOCATION_H
#include "catalog_service.h"
#include <cmath>
#include <algorithm>
#include <limits>
namespace Housing::TrophyBenefits {
struct SelectionLocation {Route route;std::array<float,3> position{};};
// Actual server character point in the locked catalog region. This is a narrow
// selection-location test, not placement collision or an invented click radius.
inline bool Within(const RegionWire::Region& region,const std::array<float,3>& point){
 try{RegionWire::Detail::ValidateMatrix(region.matrix);}catch(...){return false;}
 const auto& m=region.matrix;for(auto p:point)if(!std::isfinite(p)||std::abs(p)>65536)return false;
 const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
 std::array<double,9> inverse{e*i-f*h,c*h-b*i,b*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,b*g-a*h,a*e-b*d};
 const double determinant=a*inverse[0]+b*inverse[3]+c*inverse[6];if(!determinant||!std::isfinite(determinant))return false;
 double matrix_norm=0,inverse_norm=0;for(auto& v:inverse){v/=determinant;if(!std::isfinite(v))return false;}
 for(size_t row=0;row<3;++row){double x=0,y=0;for(size_t col=0;col<3;++col){x+=std::abs(m[4*row+col]);y+=std::abs(inverse[3*row+col]);}matrix_norm=std::max(matrix_norm,x);inverse_norm=std::max(inverse_norm,y);}
 const double condition=matrix_norm*inverse_norm;if(!std::isfinite(condition)||condition>128)return false;
 for(size_t axis=0;axis<3;++axis){double value=0,magnitude=0;for(size_t j=0;j<3;++j){const auto part=(double(point[j])-m[12+j])*inverse[j*3+axis];value+=part;magnitude+=std::abs(part);}const double guard=256*std::numeric_limits<float>::epsilon()*condition*(1+magnitude);if(!std::isfinite(value)||std::abs(value)+guard>=1)return false;}
 return true;
}
// Reviewed room profiles are unions of placement volumes. The first volume
// alone need not contain the character (for example, at an interior stair).
inline bool Within(const RegionWire::Region& primary,const std::vector<RegionWire::Region>& additional,const std::array<float,3>& point){
 if(Within(primary,point))return true;
 return std::any_of(additional.begin(),additional.end(),[&](const auto& region){return Within(region,point);});
}
}
#endif
