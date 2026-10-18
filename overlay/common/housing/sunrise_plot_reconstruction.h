#ifndef EQEMU_HOUSING_SUNRISE_PLOT_RECONSTRUCTION_H
#define EQEMU_HOUSING_SUNRISE_PLOT_RECONSTRUCTION_H
#include "region_wire.h"
#include <string_view>
#include <optional>
namespace Housing::SunrisePlotReconstruction {
// Reconstructed local content, not recovered original Live server records.
// XY derives from installed neighborhood.txt four-edge footprints, with a
// two-unit physical-edge inset. Z policy must be supplied explicitly by caller.
// Source/map pins and reconstruction rules are documented alongside this file.
struct Plot {
 uint32_t marker_id;
 std::string_view address;
 bool address_label_verified;
 float x,y,reference_floor_z,half_xy,heading_radians,ground_min_z,ground_max_z;
 uint32_t first_map_line;
};
inline constexpr std::array<Plot,71> PersonalPlots{{
 {1,"108 Brimming Way",true,332.58750000f,-218.46250000f,-0.70000000f,81.54291275f,3.13609249f,0.00000000f,0.00000000f,15136},
 {2,"106 Brimming Way",true,552.78750000f,-346.32500000f,-0.70000000f,81.70330461f,-2.88571122f,0.00000000f,0.00000000f,15140},
 {3,"105 Brimming Way",true,612.13750000f,-547.15000000f,-0.70000000f,82.10434919f,-2.79096238f,-3.50763000f,0.00000000f,15152},
 {4,"104 Brimming Way",true,695.51250000f,-739.78750000f,-0.70000000f,81.44488687f,-2.47891159f,0.00000000f,0.00000000f,15156},
 {5,"103 Brimming Way",true,727.96250000f,-992.13747500f,-0.70000000f,81.85764688f,3.13226956f,0.00000000f,0.00000000f,15164},
 {6,"112 Brimming Way",true,686.27500000f,-1386.61247500f,-0.70000000f,81.47943953f,3.09697707f,0.00000000f,0.00012207f,15168},
 {7,"111 Brimming Way",true,496.15000000f,-867.52500000f,-0.70000000f,80.74066783f,-2.51162933f,0.00000000f,0.00042725f,15160},
 {8,"110 Brimming Way",true,413.83750000f,-627.22500000f,-0.70000000f,81.33610892f,-2.97104430f,0.00000000f,0.00000000f,15148},
 {9,"109 Brimming Way",true,380.53750000f,-426.73750000f,-0.70000000f,81.67983020f,-2.91589949f,0.00000000f,0.00000000f,15144},
 {10,"104 Brimming Heights",true,51.42500000f,-603.67500000f,59.20000000f,81.37202657f,3.11574403f,59.76520000f,60.00000000f,15128},
 {11,"103 Brimming Heights",true,66.30000000f,-861.61250000f,59.37500000f,82.18553970f,-2.83941264f,60.00000000f,60.00000000f,15124},
 {12,"102 Brimming Heights",true,201.81250000f,-1084.75000000f,59.30000000f,81.31730278f,-2.45465748f,60.00000000f,60.00000000f,15120},
 {13,"105 Brimming Heights",true,-103.43750000f,-289.96250000f,109.30000000f,81.68109054f,3.13788402f,110.00000000f,110.00000000f,15100},
 {14,"108 Brimming Heights",true,-250.33750000f,-896.96250000f,109.30000000f,81.92196846f,-2.88716438f,110.00000000f,110.00000000f,15112},
 {15,"107 Brimming Heights",true,-293.62500000f,-641.71250000f,109.30000000f,81.15829278f,-3.00369363f,110.00000000f,110.00000000f,15108},
 {16,"106 Brimming Heights",true,-315.25000000f,-372.12500000f,109.30000000f,82.29117868f,3.14070874f,110.00000000f,110.00000000f,15104},
 {17,"101 Brimming Heights",true,230.16250000f,-1428.63747500f,109.30000000f,81.41227872f,2.50223150f,110.00000000f,110.00000000f,15116},
 {18,"113 Brimming Way",true,668.71250000f,-1609.51242500f,-0.57500000f,80.47128606f,3.13905239f,0.00000000f,0.12115500f,15172},
 {19,"101 Brimming Way",true,1249.21240000f,-2017.56246250f,-30.65000000f,80.78724646f,3.13517343f,-30.00000000f,-30.00000000f,15180},
 {20,"102 Brimming Way",true,1051.96242500f,-1057.47500000f,37.30000000f,81.80347616f,-3.13892243f,38.00000000f,38.00000000f,15176},
 {21,"102 Market Heights",true,2328.11235000f,-1954.76235000f,89.77500000f,81.75093106f,-2.61442805f,90.00000000f,92.31290000f,14928},
 {22,"107 Brimming Way",true,458.13750000f,105.71250000f,149.10000000f,82.05522856f,-3.14144426f,148.83200000f,150.00000000f,15132},
 {23,"101 Betwixt Avenue",true,159.66250000f,750.92500000f,44.30000000f,82.41998680f,2.73047604f,45.00000000f,45.00000000f,14936},
 {24,"105 Betwixt Avenue",true,-571.47500000f,1376.13745000f,91.30000000f,82.17913863f,3.14026620f,92.00000000f,92.00000000f,14948},
 {25,"106 Betwixt Avenue",true,-574.95000000f,1116.26250000f,91.30000000f,82.30460922f,3.14115018f,92.00000000f,92.00000000f,14944},
 {26,"107 Betwixt Avenue",true,-569.86250000f,857.77500000f,91.30000000f,82.30128812f,-3.14129762f,92.00000000f,92.00000000f,14940},
 {27,"104 Betwixt Avenue",true,-90.53750000f,1237.17497500f,64.22500000f,81.67193234f,-3.14069916f,64.93100000f,65.00000000f,14960},
 {28,"102 Betwixt Avenue",true,-224.63750000f,1036.15000000f,64.30000000f,81.39230517f,3.14040729f,65.00000000f,65.00000000f,14952},
 {29,"103 Betwixt Avenue",false,-203.98750000f,1419.55000000f,64.30000000f,81.17904047f,-3.13802814f,65.00000000f,65.00000000f,14956},
 {31,"106 Terminus Heights",true,-681.26250000f,3172.04996250f,-1.70000000f,81.55357397f,-3.13206249f,-1.00000000f,-1.00000000f,14972},
 {32,"105 Terminus Street",true,-489.65000000f,2761.58731250f,-100.35000000f,81.89178590f,2.69878767f,-100.00000000f,-99.46700000f,14968},
 {33,"104 Terminus Street",true,-318.95000000f,2970.41240000f,-100.57500000f,80.65435862f,3.13875455f,-100.00000000f,-99.10350000f,14976},
 {34,"103 Terminus Street",true,-31.18750000f,2962.02487500f,-70.72500000f,81.39561466f,3.13714319f,-70.00000000f,-70.00000000f,14980},
 {35,"102 Terminus Street",true,201.06250000f,2962.34988750f,-70.67500000f,81.99975026f,-3.13092437f,-70.00000000f,-69.99980000f,14984},
 {36,"101 Terminus Street",true,422.87500000f,2961.37485000f,-70.57500000f,81.87175344f,3.13893013f,-70.00000000f,-69.98660000f,14988},
 {37,"107 Terminus Street",true,-273.87500000f,2555.33737500f,-101.12500000f,81.20130681f,-3.13788368f,-100.56400000f,-99.99930000f,14964},
 {38,"103 Guild Way",true,1652.99987500f,2952.82480000f,31.30000000f,81.43295155f,-2.76147322f,32.00000000f,32.00000000f,14992},
 {39,"104 Guild Way",true,1929.02492500f,2690.04981250f,31.30000000f,82.39289258f,-2.53662345f,32.00000000f,32.15260000f,15004},
 {40,"102 Guild Way",true,1919.81247500f,2974.78735000f,31.30000000f,82.00687334f,3.07796321f,32.00000000f,32.00000000f,14996},
 {41,"101 Guild Way",true,2180.66247500f,2989.48743750f,31.45000000f,81.81128062f,3.13762165f,32.00000000f,32.00000000f,15000},
 {42,"108 Park Circle",true,1551.21237500f,1617.28742500f,59.30000000f,82.33460111f,-3.13791261f,60.00000000f,60.00000000f,15012},
 {43,"106 Park Circle",true,1959.87487500f,1607.71242500f,59.30000000f,81.28112356f,2.90695827f,60.00000000f,60.00000000f,15020},
 {44,"105 Park Circle",true,2157.92487500f,1497.48745000f,59.30000000f,81.63892393f,-2.48389659f,60.00000000f,60.00000000f,15024},
 {45,"103 Park Circle",true,2150.88735000f,1063.48747500f,59.30000000f,81.27398543f,-2.60939257f,60.00000000f,60.00000000f,15032},
 {46,"102 Park Circle",true,1912.74995000f,1121.97500000f,59.30000000f,81.72219607f,2.88039943f,59.99990000f,60.00000000f,15036},
 {47,"101 Park Circle",true,1729.56250000f,1190.91245000f,59.32500000f,81.46560101f,2.78923327f,59.99990000f,60.00870000f,15040},
 {48,"105 Guild Way",true,2044.86242500f,2446.26231250f,31.30000000f,81.92833704f,3.13846705f,32.00000000f,32.00000000f,15008},
 {49,"104 Park Circle",true,2221.43748750f,1269.72492500f,59.30000000f,81.38248201f,2.92343587f,60.00000000f,60.00000000f,15028},
 {50,"107 Park Circle",true,1744.58742500f,1619.74987500f,59.30000000f,82.00833732f,-3.14100044f,60.00000000f,60.00000000f,15016},
 {51,"101 Vanward Heights",true,1292.69995000f,-125.88750000f,59.30000000f,81.81005116f,3.07976255f,60.00000000f,60.00000000f,15048},
 {52,"116 Vanward Street",true,1563.78732500f,-111.00000000f,-40.55000000f,81.73124575f,3.13684843f,-40.00000000f,-39.58670000f,15044},
 {53,"114 Vanward Street",true,1779.48742500f,-126.60000000f,-40.65000000f,82.00428573f,2.79337848f,-40.00000000f,-40.00000000f,15056},
 {54,"113 Vanward Street",true,2173.17475000f,84.80000000f,-40.70000000f,82.30493271f,2.61396410f,-40.00000000f,-40.00000000f,15064},
 {55,"112 Vanward Street",true,2028.86237500f,-168.01250000f,-40.70000000f,81.45249561f,3.13101339f,-40.00000000f,-40.00000000f,15060},
 {56,"110 Vanward Street",true,2021.63745000f,-376.86250000f,-40.62500000f,82.38607283f,3.12787467f,-40.00000000f,-39.03420000f,15052},
 {57,"108 Vanward Street",true,1867.97490000f,-544.17500000f,-40.52500000f,82.45713610f,2.88161876f,-40.00000000f,-39.96370000f,15196},
 {58,"106 Vanward Street",true,2024.47485000f,-708.68750000f,-40.70000000f,81.51397102f,3.09924510f,-40.00000000f,-40.00000000f,15080},
 {59,"102 Vanward Street",true,1804.94985000f,-796.85000000f,-40.70000000f,82.33909263f,2.56272636f,-40.00000000f,-40.00000000f,15200},
 {60,"101 Vanward Street",true,1599.76245000f,-1016.75000000f,-40.70000000f,81.46504860f,2.73742590f,-40.00000000f,-40.00000000f,15096},
 {61,"103 Vanward Street",true,1825.32490000f,-1074.33750000f,-40.70000000f,81.16115100f,3.05649135f,-40.00000000f,-40.00000000f,15092},
 {62,"104 Vanward Street",true,2053.01245000f,-1054.85000000f,-40.70000000f,81.67988730f,-2.83519229f,-40.00000000f,-40.00000000f,15088},
 {63,"105 Vanward Street",true,2311.17485000f,-1176.34992500f,-40.70000000f,81.30338342f,2.44248631f,-40.00000000f,-40.00000000f,15084},
 {64,"107 Vanward Street",true,2250.42475000f,-632.93750000f,-40.70000000f,81.67495353f,3.13967355f,-40.00000000f,-40.00000000f,15076},
 {65,"109 Vanward Street",true,2253.83743750f,-422.82500000f,-40.70000000f,81.49515172f,-3.13714758f,-40.00000000f,-40.00000000f,15072},
 {66,"111 Vanward Street",true,2254.03747500f,-204.33750000f,-40.70000000f,81.76333267f,-3.13341926f,-40.00000000f,-40.00000000f,15068},
 {67,"103 Market Street",true,2273.57490000f,-2403.82481250f,-0.70000000f,81.60197201f,-2.37008196f,0.00000000f,0.00000000f,15188},
 {68,"103 Market Heights",true,1858.62492500f,-1690.18740000f,59.30000000f,81.38445701f,3.04458749f,60.00000000f,60.00000000f,14932},
 {69,"101 Market Heights",true,2510.46226250f,-2597.98746250f,79.30000000f,80.90362003f,-3.13695172f,80.00000000f,80.00000000f,15184},
 {71,"101 Market Street",true,1936.19977500f,-2582.31228750f,-0.70000000f,82.42615889f,2.39186676f,0.00000000f,0.00000000f,14920},
 {72,"102 Market Street",true,2068.42493750f,-2449.49991250f,-0.70000000f,82.35556730f,-2.39100711f,0.00000000f,0.00000000f,14924},
 {73,"104 Market Street",true,2222.34982500f,-2647.87490000f,-0.70000000f,80.96123252f,-3.05772009f,0.00000000f,0.00000000f,15192},
}};
inline const Plot* FindPersonal(uint32_t marker_id,bool allow_inferred_address=false){
 for(const auto& p:PersonalPlots)if(p.marker_id==marker_id){if(!p.address_label_verified&&!allow_inferred_address)return nullptr;return &p;}
 return nullptr;
}
// bottom_z/top_z are reviewed server-space absolute limits. Neither the map's
// reference elevation nor the rendered pedestal determines a legal Z volume.
inline RegionWire::Region Region(const Plot& p,uint32_t region_id,float bottom_z,float top_z){
 if(!std::isfinite(bottom_z)||!std::isfinite(top_z)||!(top_z>bottom_z))throw std::invalid_argument("Explicit reconstructed plot Z bounds required");
 RegionWire::Region r{};r.id=region_id;EstateWire::Detail::Identity(region_id);
 const float c=std::cos(p.heading_radians),s=std::sin(p.heading_radians),h=p.half_xy;
 r.matrix={h*c,-h*s,0,0,h*s,h*c,0,0,0,0,(top_z-bottom_z)/2,0,p.y,p.x,(top_z+bottom_z)/2,1};
 RegionWire::Detail::ValidateMatrix(r.matrix);return r;
}
// Four physically supported reconstructed guild sites. No marker IDs are
// invented here: a content importer must allocate reviewed GUILDSQUARE doors.
// Brimming uses map edges; the other three use collision-plateau interiors.
struct GuildPlot {
 std::string_view address;
 float x,y,reference_floor_z,half_x,half_y,heading_radians,ground_min_z,ground_max_z;
};
inline constexpr std::array<GuildPlot,4> GuildPlots{{
 {"200 Guild Way",2743.54200000f,2612.97050000f,110.00000000f,318.00000000f,173.00000000f,0.00000000f,109.84900000f,110.10500000f},
 {"200 Terminus Heights",-1104.27260000f,2418.26200000f,110.00000000f,348.00000000f,158.00000000f,0.00000000f,109.64800000f,110.32900000f},
 {"200 Vanward Heights",2993.65800000f,-765.99660000f,0.00000000f,318.00000000f,173.00000000f,0.00000000f,-0.00000048f,0.01470950f},
 {"200 Brimming Way",571.65000000f,-2227.77482500f,0.00000000f,343.42335821f,171.71167910f,1.60833601f,-0.00000024f,0.19152800f},
}};
inline RegionWire::Region Region(const GuildPlot& p,uint32_t region_id,float bottom_z,float top_z){
 if(!std::isfinite(bottom_z)||!std::isfinite(top_z)||!(top_z>bottom_z))throw std::invalid_argument("Explicit reconstructed guild plot Z bounds required");
 RegionWire::Region r{};r.id=region_id;EstateWire::Detail::Identity(region_id);
 const float c=std::cos(p.heading_radians),s=std::sin(p.heading_radians);
 r.matrix={p.half_y*c,-p.half_y*s,0,0,p.half_x*s,p.half_x*c,0,0,0,0,(top_z-bottom_z)/2,0,p.y,p.x,(top_z+bottom_z)/2,1};
 RegionWire::Detail::ValidateMatrix(r.matrix);return r;
}
// Reconstructed later-Live defaults from wasami007's housing table,
// edited2022-05-12, reply2023-02-21; same source as default tariffs:
// https://forum.gamer.com.tw/C.php?bsn=5179&snA=2008
// These capacities are exterior plot inventories, not house/hall interior limits.
// Park Circle's105 items/6 pets is preserved exactly rather than normalized.
struct Capacity {uint32_t max_items,max_npc_items,max_non_estate_items=0,max_children=1;};
inline std::optional<Capacity> PersonalCapacity(std::string_view address){
 bool known=false;for(const auto& p:PersonalPlots)if(p.address==address){known=true;break;}
 if(!known)return {};const auto street=address.substr(4);
 if(street=="Market Street")return Capacity{90,4};
 if(street=="Betwixt Avenue"||street=="Guild Way"||street=="Vanward Street")return Capacity{105,5};
 if(street=="Brimming Way")return address.substr(0,3)=="101"||address.substr(0,3)=="102"?Capacity{120,6}:Capacity{105,5};
 if(street=="Park Circle")return Capacity{105,6};
 if(street=="Brimming Heights"||street=="Market Heights"||street=="Vanward Heights"||street=="Terminus Street"||street=="Terminus Heights")return Capacity{120,6};
 return {};
}
}
#endif
