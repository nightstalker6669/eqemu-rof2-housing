#ifndef EQEMU_HOUSING_ORIGINAL_WALL_FRAME_H
#define EQEMU_HOUSING_ORIGINAL_WALL_FRAME_H
#include "original_height_preimage.h"
namespace Housing::OriginalWallFrame {
namespace F=OriginalActorQueryFrame;
using Point=OriginalSphereQuery::Point;
// Native Wall mode1,557937..94f ->557952..56 skips Floor/Ceiling correction;
//5587c6..7ee subtracts the SAME rotated/scaled C, storing each coordinate.
// An actual stopped query centre is required: QueryCenter(actor) is NOT an
// inverse of this float-store chain and is not the original camera query.
inline std::optional<Point> ActorFromStopped(const Point& stopped,const F::Frame& frame){
 if(!F::Valid(frame)||!OriginalSphereSource::Finite(stopped))return {};
 OriginalSphereQuery::Detail::Rounding rounding;if(!rounding.good)return {};
 try {Point actor;for(unsigned k=0;k<3;++k)actor[k]=F::Detail::Store(F::Detail::Sub(stopped[k],frame.center[k]));return actor;}catch(...){return {};}
}
inline bool Matches(const Point& actor,const F::Frame& frame,const Point& actual_stopped){
 auto generated=ActorFromStopped(actual_stopped,frame);return generated&&OriginalSphereSource::Bits(*generated,actor);
}
// Complete three-axis binary32 preimage intervals within an independently
// reviewed centre envelope. These do NOT select a source/contact or establish
// a clear inbound corridor. No sample is promoted to collision authority.
using Interval=OriginalHeightPreimage::Interval;
struct Preimage {std::array<Interval,3> coordinates{};};
inline std::optional<Preimage> Preimages(const Point& actor,const F::Frame& frame,const Point& minimum,const Point& maximum){
 if(!F::Valid(frame)||!OriginalSphereSource::Finite(actor))return {};
 Preimage out;for(unsigned k=0;k<3;++k){auto i=OriginalHeightPreimage::Preimages(actor[k],frame.center[k],minimum[k],maximum[k]);if(!i)return {};out.coordinates[k]=*i;}return out;
}
inline bool Contains(const Preimage& p,const Point& stopped){
 if(!OriginalSphereSource::Finite(stopped))return false;
 for(unsigned k=0;k<3;++k)if(!p.coordinates[k].Contains(stopped[k]))return false;return true;
}
}
#endif
