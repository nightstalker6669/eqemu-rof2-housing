#ifndef EQEMU_HOUSING_PLOT_SEARCH_DISPATCH_H
#define EQEMU_HOUSING_PLOT_SEARCH_DISPATCH_H
#include "estate_wire.h"
namespace Housing::PlotSearchDispatch {
// Exact original REPSW button chain: MyPlots(+22c)->74b920(mode4,arg2=0),
// Search(+228)->74b920(mode4,arg2=1); arg2 is written to request BYTE220.
// MyItems(+230)->74be00(mode5). RENW uses distinct neighborhood modes0/1.
inline constexpr size_t SelfFilterOffset=220;
enum class Kind {Invalid,OwnedPlots,ExplicitOwnerSearch,OwnedItems};
inline Kind Classify(const EstateWire::PlotSearchRequest& request){
 if(request.mode==5)return Kind::OwnedItems;
 if(request.mode!=4)return Kind::Invalid;
 return request.source[SelfFilterOffset]==0?Kind::OwnedPlots:request.source[SelfFilterOffset]==1?Kind::ExplicitOwnerSearch:Kind::Invalid;
}
}
#endif
