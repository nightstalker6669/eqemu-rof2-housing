#ifndef EQEMU_HOUSING_EXTERIOR_EXIT_GEOMETRY_H
#define EQEMU_HOUSING_EXTERIOR_EXIT_GEOMETRY_H
#include "placement_policy.h"
#include <functional>
namespace Housing::ExteriorExit {
using Point=std::array<float,3>;using Destination=std::array<float,4>;
struct Box{Point minimum{},maximum{};};
inline bool Finite(Point p){for(float x:p)if(!std::isfinite(x)||std::abs(x)>65536)return false;return true;}
inline bool Overlap(const Box&a,const Box&b){for(unsigned i=0;i<3;++i)if(a.maximum[i]<b.minimum[i]||a.minimum[i]>b.maximum[i])return false;return true;}
inline Box Bounds(const PlacementPolicy::Geometry& geometry,const PlacementPolicy::Transform& pose){
 if(!std::isfinite(geometry.measured_local_error)||geometry.measured_local_error<0||!geometry.native_collision_verified||!std::isfinite(pose.scale)||pose.scale<=0||pose.scale>16)throw std::invalid_argument("Invalid exit obstacle");
 for(float v:{pose.heading,pose.pitch,pose.roll})if(!std::isfinite(v)||v<0||v>512)throw std::invalid_argument("Invalid exit obstacle pose");
 Box out;out.minimum.fill(INFINITY);out.maximum.fill(-INFINITY);auto rotation=PlacementPolicy::Rotation(pose.heading,pose.pitch,pose.roll);
 for(unsigned corner=0;corner<8;++corner){PlacementPolicy::Point p;for(unsigned i=0;i<3;++i){if(!std::isfinite(geometry.center[i])||!std::isfinite(geometry.extents[i])||geometry.extents[i]<=0)throw std::invalid_argument("Invalid exit obstacle model");p[i]=(geometry.center[i]+((corner>>i)&1?geometry.extents[i]:-geometry.extents[i]))*pose.scale;}p=PlacementPolicy::Rotate(p,rotation);Point world{float(p[1]+pose.position[1]),float(p[0]+pose.position[0]),float(p[2]+pose.position[2])};if(!Finite(world))throw std::invalid_argument("Invalid exit obstacle position");for(unsigned i=0;i<3;++i){out.minimum[i]=std::min(out.minimum[i],world[i]);out.maximum[i]=std::max(out.maximum[i],world[i]);}}
 // Expand rather than spend the reviewed float-cache uncertainty as clearance.
 float magnitude=1;for(unsigned i=0;i<3;++i)magnitude=std::max({magnitude,std::abs(out.minimum[i]),std::abs(out.maximum[i])});
 const float margin=float(3*geometry.measured_local_error*pose.scale)+16*std::numeric_limits<float>::epsilon()*magnitude;
 for(unsigned i=0;i<3;++i){out.minimum[i]=std::nextafter(out.minimum[i]-margin,-INFINITY);out.maximum[i]=std::nextafter(out.maximum[i]+margin,INFINITY);}
 return out;
}
inline bool TriangleOverlap(const Box&box,const std::array<Point,3>& triangle){
 if(!Finite(box.minimum)||!Finite(box.maximum))return true;
 double center[3],extent[3],v[3][3];for(unsigned i=0;i<3;++i){if(box.minimum[i]>box.maximum[i])return true;center[i]=(double(box.minimum[i])+box.maximum[i])/2;extent[i]=(double(box.maximum[i])-box.minimum[i])/2;}
 for(unsigned k=0;k<3;++k){if(!Finite(triangle[k]))return true;for(unsigned i=0;i<3;++i)v[k][i]=triangle[k][i]-center[i];}
 const auto separates=[&](std::array<double,3>axis){double low=INFINITY,high=-INFINITY,radius=0;for(unsigned k=0;k<3;++k){double d=0;for(unsigned i=0;i<3;++i)d+=v[k][i]*axis[i];low=std::min(low,d);high=std::max(high,d);}for(unsigned i=0;i<3;++i)radius+=extent[i]*std::abs(axis[i]);const double guard=1e-6*(1+radius);return low>radius+guard||high<-radius-guard;};
 for(unsigned i=0;i<3;++i){std::array<double,3>axis{};axis[i]=1;if(separates(axis))return false;}
 double edges[3][3];for(unsigned k=0;k<3;++k)for(unsigned i=0;i<3;++i)edges[k][i]=v[(k+1)%3][i]-v[k][i];
 std::array<double,3>normal{edges[0][1]*edges[1][2]-edges[0][2]*edges[1][1],edges[0][2]*edges[1][0]-edges[0][0]*edges[1][2],edges[0][0]*edges[1][1]-edges[0][1]*edges[1][0]};if(separates(normal))return false;
 for(const auto&edge:edges){if(separates({0,edge[2],-edge[1]})||separates({-edge[2],0,edge[0]})||separates({edge[1],-edge[0],0}))return false;}
 return true;
}
inline bool Inside(const RegionWire::Region&r,const Box& server){
 try{RegionWire::Detail::ValidateMatrix(r.matrix);}catch(...){return false;}
 const auto&m=r.matrix;double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];const std::array<double,9> adj{e*i-f*h,c*h-b*i,b*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,b*g-a*h,a*e-b*d};double det=a*adj[0]+b*adj[3]+c*adj[6];if(!std::isfinite(det)||det==0)return false;
 for(unsigned corner=0;corner<8;++corner){Point s;for(unsigned k=0;k<3;++k)s[k]=(corner>>k)&1?server.maximum[k]:server.minimum[k];if(!Finite(s))return false;double p[3]={s[1],s[0],s[2]};for(unsigned axis=0;axis<3;++axis){double value=0;for(unsigned k=0;k<3;++k)value+=(p[k]-m[12+k])*adj[3*k+axis]/det;if(!std::isfinite(value)||std::abs(value)>=0.999)return false;}}
 return true;
}
// Ground returns actual server Map hit+normal, not a template point. Clearance
// tests the complete conservative body box against actual map triangles.
struct Ground{Point point{},normal{};};
using Trace=std::function<std::optional<Ground>(Point,Point)>;
using Clear=std::function<bool(const Box&)>;
inline std::optional<Destination> Resolve(const RegionWire::Region& region,Point reference,float size,float zoffset,const std::vector<Box>& blockers,const Trace& trace,const Clear& clear){
 if(!Finite(reference)||!std::isfinite(size)||!std::isfinite(zoffset)||size<=0||size>128||zoffset<=0||zoffset>128||!trace||!clear)return {};
 try{RegionWire::Detail::ValidateMatrix(region.matrix);}catch(...){return {};}
 // Server Mob size is deliberately enlarged to a conservative square body,
 // not claimed to reproduce native race capsule dimensions.
 const float radius=std::max(size,zoffset),height=2*radius;const auto&m=region.matrix;
 std::vector<Point> candidates;const unsigned divisions=24;
 for(unsigned u=1;u<divisions;++u)for(unsigned v=1;v<divisions;++v){double x=-1+2*double(u)/divisions,y=-1+2*double(v)/divisions;Point p{float(x*m[1]+y*m[5]+m[13]),float(x*m[0]+y*m[4]+m[12]),m[14]};if(Finite(p))candidates.push_back(p);}
 std::stable_sort(candidates.begin(),candidates.end(),[&](Point a,Point b){auto distance=[&](Point p){return std::hypot(double(p[0])-reference[0],double(p[1])-reference[1]);};return distance(a)<distance(b);});
 const float vertical=std::abs(m[2])+std::abs(m[6])+std::abs(m[10]);
 for(auto p:candidates){Point from{p[0],p[1],m[14]+vertical},to{p[0],p[1],m[14]-vertical};auto hit=trace(from,to);if(!hit||!Finite(hit->point)||!Finite(hit->normal)||hit->normal[2]<0.95f)continue;const auto&g=hit->point;Box body{{g[0]-radius,g[1]-radius,g[2]+0.05f},{g[0]+radius,g[1]+radius,g[2]+height}};if(!Inside(region,body))continue;bool blocked=false;for(const auto&box:blockers)if(Overlap(body,box)){blocked=true;break;}if(blocked||!clear(body))continue;
  return Destination{g[0],g[1],g[2]+zoffset,0};
 }
 return {};
}
}
#endif
