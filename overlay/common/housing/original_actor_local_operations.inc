case 0x100076d0u:{pushfp(fp((r[2]+4u)));pc=0x100076d3u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x100076d3u:{pushfp(fp((r[2])));pc=0x100076d5u;break;} // d901 fld dword ptr [ecx]
case 0x100076d5u:{pushfp(fp((r[2]+8u)));pc=0x100076d8u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x100076d8u:{pushfp(st[1]);pc=0x100076dau;break;} // d9c1 fld st(1)
case 0x100076dau:{st[2]=product(st[2],st[0]);popfp();pc=0x100076dcu;break;} // deca fmulp st(2)
case 0x100076dcu:{pushfp(st[2]);pc=0x100076deu;break;} // d9c2 fld st(2)
case 0x100076deu:{st[3]=product(st[3],st[0]);popfp();pc=0x100076e0u;break;} // decb fmulp st(3)
case 0x100076e0u:{std::swap(st[0],st[1]);pc=0x100076e2u;break;} // d9c9 fxch st(1)
case 0x100076e2u:{st[2]=sum(st[2],st[0]);popfp();pc=0x100076e4u;break;} // dec2 faddp st(2)
case 0x100076e4u:{st[0]=product(st[0],st[0]);pc=0x100076e6u;break;} // dcc8 fmul st(0), st(0)
case 0x100076e6u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100076e8u;break;} // dec1 faddp st(1)
case 0x100076e8u:{st[0]=root(st[0]);pc=0x100076eau;break;} // d9fa fsqrt
case 0x100076eau:{pushfp(fp((269687832u)));pc=0x100076f0u;break;} // d905181c1310 fld dword ptr [0x10131c18]
case 0x100076f0u:{compare(st[0],st[1]);popfp();pc=0x100076f2u;break;} // d8d9 fcomp st(1)
case 0x100076f2u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100076f4u;break;} // dfe0 fnstsw ax
case 0x100076f4u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100076f7u;break;} // f6c405 test ah, 5
case 0x100076f7u:{pc=(pf)?0x10007715u:0x100076f9u;break;} // 7a1c jp 0x10007715
case 0x100076f9u:{pushfp(fp((269687828u)));pc=0x100076ffu;break;} // d905141c1310 fld dword ptr [0x10131c14]
case 0x100076ffu:{compare(st[0],st[1]);popfp();pc=0x10007701u;break;} // d8d9 fcomp st(1)
case 0x10007701u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x10007703u;break;} // dfe0 fnstsw ax
case 0x10007703u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x10007706u;break;} // f6c441 test ah, 0x41
case 0x10007706u:{pc=(!zf)?0x10007715u:0x10007708u;break;} // 750d jne 0x10007715
case 0x10007708u:{st[0]=st[0];popfp();pc=0x1000770au;break;} // ddd8 fstp st(0)
case 0x1000770au:{pushfp(0.);pc=0x1000770cu;break;} // d9ee fldz
case 0x1000770cu:{fpwrite((r[2]+8u),st[0]);pc=0x1000770fu;break;} // d95108 fst dword ptr [ecx + 8]
case 0x1000770fu:{fpwrite((r[2]+4u),st[0]);pc=0x10007712u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x10007712u:{fpwrite((r[2]),st[0]);popfp();pc=0x10007714u;break;} // d919 fstp dword ptr [ecx]
case 0x10007714u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x10007715u:{pushfp(1.);pc=0x10007717u;break;} // d9e8 fld1
case 0x10007717u:{st[1]=quotient(st[0],st[1]);popfp();pc=0x10007719u;break;} // def1 fdivrp st(1)
case 0x10007719u:{pushfp(fp((r[2])));pc=0x1000771bu;break;} // d901 fld dword ptr [ecx]
case 0x1000771bu:{st[0]=product(st[0],st[1]);pc=0x1000771du;break;} // d8c9 fmul st(1)
case 0x1000771du:{fpwrite((r[2]),st[0]);popfp();pc=0x1000771fu;break;} // d919 fstp dword ptr [ecx]
case 0x1000771fu:{pushfp(st[0]);pc=0x10007721u;break;} // d9c0 fld st(0)
case 0x10007721u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10007724u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10007724u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x10007727u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x10007727u:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x1000772au;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x1000772au:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x1000772du;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x1000772du:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x1000b210u:{r[3]=get((r[7]+8u),4);pc=0x1000b214u;break;} // 8b542408 mov edx, dword ptr [esp + 8]
case 0x1000b214u:{pushfp(fp((r[2]+16u)));pc=0x1000b217u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x1000b217u:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1000b21au;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1000b21au:{r[0]=get((r[7]+4u),4);pc=0x1000b21eu;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x1000b21eu:{pushfp(fp((r[3])));pc=0x1000b220u;break;} // d902 fld dword ptr [edx]
case 0x1000b220u:{st[0]=product(st[0],fp((r[2])));pc=0x1000b222u;break;} // d809 fmul dword ptr [ecx]
case 0x1000b222u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b224u;break;} // dec1 faddp st(1)
case 0x1000b224u:{pushfp(fp((r[2]+32u)));pc=0x1000b227u;break;} // d94120 fld dword ptr [ecx + 0x20]
case 0x1000b227u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1000b22au;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1000b22au:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b22cu;break;} // dec1 faddp st(1)
case 0x1000b22cu:{st[0]=sum(st[0],fp((r[2]+48u)));pc=0x1000b22fu;break;} // d84130 fadd dword ptr [ecx + 0x30]
case 0x1000b22fu:{fpwrite((r[0]),st[0]);popfp();pc=0x1000b231u;break;} // d918 fstp dword ptr [eax]
case 0x1000b231u:{pushfp(fp((r[2]+4u)));pc=0x1000b234u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x1000b234u:{st[0]=product(st[0],fp((r[3])));pc=0x1000b236u;break;} // d80a fmul dword ptr [edx]
case 0x1000b236u:{pushfp(fp((r[2]+20u)));pc=0x1000b239u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x1000b239u:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1000b23cu;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1000b23cu:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b23eu;break;} // dec1 faddp st(1)
case 0x1000b23eu:{pushfp(fp((r[2]+36u)));pc=0x1000b241u;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x1000b241u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1000b244u;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1000b244u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b246u;break;} // dec1 faddp st(1)
case 0x1000b246u:{st[0]=sum(st[0],fp((r[2]+52u)));pc=0x1000b249u;break;} // d84134 fadd dword ptr [ecx + 0x34]
case 0x1000b249u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x1000b24cu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x1000b24cu:{pushfp(fp((r[2]+8u)));pc=0x1000b24fu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x1000b24fu:{st[0]=product(st[0],fp((r[3])));pc=0x1000b251u;break;} // d80a fmul dword ptr [edx]
case 0x1000b251u:{pushfp(fp((r[2]+24u)));pc=0x1000b254u;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x1000b254u:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1000b257u;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1000b257u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b259u;break;} // dec1 faddp st(1)
case 0x1000b259u:{pushfp(fp((r[2]+40u)));pc=0x1000b25cu;break;} // d94128 fld dword ptr [ecx + 0x28]
case 0x1000b25cu:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1000b25fu;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1000b25fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x1000b261u;break;} // dec1 faddp st(1)
case 0x1000b261u:{st[0]=sum(st[0],fp((r[2]+56u)));pc=0x1000b264u;break;} // d84138 fadd dword ptr [ecx + 0x38]
case 0x1000b264u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x1000b267u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x1000b267u:{pc=get(r[7],4);r[7]+=12u;if(pc==0xeeeeeeeeu)return;break;} // c20800 ret 8
case 0x10021a60u:{uint32_t v=(r[7])-(32u);r[7]=v;flags(v);pc=0x10021a63u;break;} // 83ec20 sub esp, 0x20
case 0x10021a63u:{pushfp(fp((r[2])));pc=0x10021a65u;break;} // d901 fld dword ptr [ecx]
case 0x10021a65u:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021a68u;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021a68u:{pushfp(fp((r[2]+16u)));pc=0x10021a6bu;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x10021a6bu:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021a6eu;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021a6eu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021a70u;break;} // dee9 fsubp st(1)
case 0x10021a70u:{pushfp(fp((r[2]+24u)));pc=0x10021a73u;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021a73u:{st[0]=product(st[0],fp((r[2])));pc=0x10021a75u;break;} // d809 fmul dword ptr [ecx]
case 0x10021a75u:{pushfp(fp((r[2]+8u)));pc=0x10021a78u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021a78u:{st[0]=product(st[0],fp((r[2]+16u)));pc=0x10021a7bu;break;} // d84910 fmul dword ptr [ecx + 0x10]
case 0x10021a7bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021a7du;break;} // dee9 fsubp st(1)
case 0x10021a7du:{pushfp(fp((r[2]+28u)));pc=0x10021a80u;break;} // d9411c fld dword ptr [ecx + 0x1c]
case 0x10021a80u:{st[0]=product(st[0],fp((r[2])));pc=0x10021a82u;break;} // d809 fmul dword ptr [ecx]
case 0x10021a82u:{pushfp(fp((r[2]+12u)));pc=0x10021a85u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x10021a85u:{st[0]=product(st[0],fp((r[2]+16u)));pc=0x10021a88u;break;} // d84910 fmul dword ptr [ecx + 0x10]
case 0x10021a88u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021a8au;break;} // dee9 fsubp st(1)
case 0x10021a8au:{pushfp(fp((r[2]+24u)));pc=0x10021a8du;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021a8du:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021a90u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021a90u:{pushfp(fp((r[2]+8u)));pc=0x10021a93u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021a93u:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021a96u;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021a96u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021a98u;break;} // dee9 fsubp st(1)
case 0x10021a98u:{pushfp(fp((r[2]+28u)));pc=0x10021a9bu;break;} // d9411c fld dword ptr [ecx + 0x1c]
case 0x10021a9bu:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021a9eu;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021a9eu:{pushfp(fp((r[2]+12u)));pc=0x10021aa1u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x10021aa1u:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021aa4u;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021aa4u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021aa6u;break;} // dee9 fsubp st(1)
case 0x10021aa6u:{pushfp(fp((r[2]+8u)));pc=0x10021aa9u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021aa9u:{st[0]=product(st[0],fp((r[2]+28u)));pc=0x10021aacu;break;} // d8491c fmul dword ptr [ecx + 0x1c]
case 0x10021aacu:{pushfp(fp((r[2]+24u)));pc=0x10021aafu;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021aafu:{st[0]=product(st[0],fp((r[2]+12u)));pc=0x10021ab2u;break;} // d8490c fmul dword ptr [ecx + 0xc]
case 0x10021ab2u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021ab4u;break;} // dee9 fsubp st(1)
case 0x10021ab4u:{fpwrite((r[7]+28u),st[0]);pc=0x10021ab8u;break;} // d954241c fst dword ptr [esp + 0x1c]
case 0x10021ab8u:{pushfp(fp((r[2]+52u)));pc=0x10021abbu;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021abbu:{st[0]=product(st[0],fp((r[2]+32u)));pc=0x10021abeu;break;} // d84920 fmul dword ptr [ecx + 0x20]
case 0x10021abeu:{pushfp(fp((r[2]+48u)));pc=0x10021ac1u;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021ac1u:{st[0]=product(st[0],fp((r[2]+36u)));pc=0x10021ac4u;break;} // d84924 fmul dword ptr [ecx + 0x24]
case 0x10021ac4u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021ac6u;break;} // dee9 fsubp st(1)
case 0x10021ac6u:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x10021acau;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x10021acau:{pushfp(fp((r[2]+32u)));pc=0x10021acdu;break;} // d94120 fld dword ptr [ecx + 0x20]
case 0x10021acdu:{st[0]=product(st[0],fp((r[2]+56u)));pc=0x10021ad0u;break;} // d84938 fmul dword ptr [ecx + 0x38]
case 0x10021ad0u:{pushfp(fp((r[2]+48u)));pc=0x10021ad3u;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021ad3u:{st[0]=product(st[0],fp((r[2]+40u)));pc=0x10021ad6u;break;} // d84928 fmul dword ptr [ecx + 0x28]
case 0x10021ad6u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021ad8u;break;} // dee9 fsubp st(1)
case 0x10021ad8u:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x10021adcu;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x10021adcu:{pushfp(fp((r[2]+60u)));pc=0x10021adfu;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021adfu:{st[0]=product(st[0],fp((r[2]+32u)));pc=0x10021ae2u;break;} // d84920 fmul dword ptr [ecx + 0x20]
case 0x10021ae2u:{pushfp(fp((r[2]+48u)));pc=0x10021ae5u;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021ae5u:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021ae8u;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021ae8u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021aeau;break;} // dee9 fsubp st(1)
case 0x10021aeau:{fpwrite((r[7]),st[0]);popfp();pc=0x10021aedu;break;} // d91c24 fstp dword ptr [esp]
case 0x10021aedu:{pushfp(fp((r[2]+36u)));pc=0x10021af0u;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x10021af0u:{st[0]=product(st[0],fp((r[2]+56u)));pc=0x10021af3u;break;} // d84938 fmul dword ptr [ecx + 0x38]
case 0x10021af3u:{pushfp(fp((r[2]+52u)));pc=0x10021af6u;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021af6u:{st[0]=product(st[0],fp((r[2]+40u)));pc=0x10021af9u;break;} // d84928 fmul dword ptr [ecx + 0x28]
case 0x10021af9u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021afbu;break;} // dee9 fsubp st(1)
case 0x10021afbu:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x10021affu;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x10021affu:{pushfp(fp((r[2]+60u)));pc=0x10021b02u;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021b02u:{st[0]=product(st[0],fp((r[2]+36u)));pc=0x10021b05u;break;} // d84924 fmul dword ptr [ecx + 0x24]
case 0x10021b05u:{pushfp(fp((r[2]+52u)));pc=0x10021b08u;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021b08u:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021b0bu;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021b0bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b0du;break;} // dee9 fsubp st(1)
case 0x10021b0du:{fpwrite((r[7]+4u),st[0]);popfp();pc=0x10021b11u;break;} // d95c2404 fstp dword ptr [esp + 4]
case 0x10021b11u:{pushfp(fp((r[2]+60u)));pc=0x10021b14u;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021b14u:{st[0]=product(st[0],fp((r[2]+40u)));pc=0x10021b17u;break;} // d84928 fmul dword ptr [ecx + 0x28]
case 0x10021b17u:{pushfp(fp((r[2]+56u)));pc=0x10021b1au;break;} // d94138 fld dword ptr [ecx + 0x38]
case 0x10021b1au:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021b1du;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021b1du:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b1fu;break;} // dee9 fsubp st(1)
case 0x10021b1fu:{fpwrite((r[7]+16u),st[0]);pc=0x10021b23u;break;} // d9542410 fst dword ptr [esp + 0x10]
case 0x10021b23u:{st[0]=product(st[0],st[6]);pc=0x10021b25u;break;} // d8ce fmul st(6)
case 0x10021b25u:{pushfp(fp((r[7]+4u)));pc=0x10021b29u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x10021b29u:{st[0]=product(st[0],st[6]);pc=0x10021b2bu;break;} // d8ce fmul st(6)
case 0x10021b2bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b2du;break;} // dee9 fsubp st(1)
case 0x10021b2du:{pushfp(fp((r[7]+8u)));pc=0x10021b31u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x10021b31u:{st[0]=product(st[0],st[5]);pc=0x10021b33u;break;} // d8cd fmul st(5)
case 0x10021b33u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021b35u;break;} // dec1 faddp st(1)
case 0x10021b35u:{pushfp(fp((r[7])));pc=0x10021b38u;break;} // d90424 fld dword ptr [esp]
case 0x10021b38u:{st[0]=product(st[0],st[4]);pc=0x10021b3au;break;} // d8cc fmul st(4)
case 0x10021b3au:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021b3cu;break;} // dec1 faddp st(1)
case 0x10021b3cu:{pushfp(fp((r[7]+12u)));pc=0x10021b40u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x10021b40u:{st[0]=product(st[0],st[3]);pc=0x10021b42u;break;} // d8cb fmul st(3)
case 0x10021b42u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b44u;break;} // dee9 fsubp st(1)
case 0x10021b44u:{pushfp(fp((r[7]+20u)));pc=0x10021b48u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x10021b48u:{st[2]=product(st[2],st[0]);popfp();pc=0x10021b4au;break;} // deca fmulp st(2)
case 0x10021b4au:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021b4cu;break;} // dec1 faddp st(1)
case 0x10021b4cu:{fpwrite((r[7]+24u),st[0]);pc=0x10021b50u;break;} // d9542418 fst dword ptr [esp + 0x18]
case 0x10021b50u:{pushfp(0.);pc=0x10021b52u;break;} // d9ee fldz
case 0x10021b52u:{compare(st[0],st[1]);popfp();popfp();pc=0x10021b54u;break;} // dae9 fucompp
case 0x10021b54u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x10021b56u;break;} // dfe0 fnstsw ax
case 0x10021b56u:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x10021b59u;break;} // f6c444 test ah, 0x44
case 0x10021b59u:{pc=(pf)?0x10021b65u:0x10021b5bu;break;} // 7a0a jp 0x10021b65
case 0x10021b5bu:{pushfp(fp((269687828u)));pc=0x10021b61u;break;} // d905141c1310 fld dword ptr [0x10131c14]
case 0x10021b61u:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x10021b65u;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x10021b65u:{pushfp(fp((r[7]+16u)));pc=0x10021b69u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x10021b69u:{r[0]=get((r[7]+36u),4);pc=0x10021b6du;break;} // 8b442424 mov eax, dword ptr [esp + 0x24]
case 0x10021b6du:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021b70u;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021b70u:{pushfp(fp((r[2]+24u)));pc=0x10021b73u;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021b73u:{st[0]=product(st[0],fp((r[7]+4u)));pc=0x10021b77u;break;} // d84c2404 fmul dword ptr [esp + 4]
case 0x10021b77u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b79u;break;} // dee9 fsubp st(1)
case 0x10021b79u:{pushfp(fp((r[2]+28u)));pc=0x10021b7cu;break;} // d9411c fld dword ptr [ecx + 0x1c]
case 0x10021b7cu:{st[0]=product(st[0],fp((r[7]+8u)));pc=0x10021b80u;break;} // d84c2408 fmul dword ptr [esp + 8]
case 0x10021b80u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021b82u;break;} // dec1 faddp st(1)
case 0x10021b82u:{fpwrite((r[0]),st[0]);popfp();pc=0x10021b84u;break;} // d918 fstp dword ptr [eax]
case 0x10021b84u:{pushfp(fp((r[2]+24u)));pc=0x10021b87u;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021b87u:{st[0]=product(st[0],fp((r[7])));pc=0x10021b8au;break;} // d80c24 fmul dword ptr [esp]
case 0x10021b8au:{pushfp(fp((r[7]+16u)));pc=0x10021b8eu;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x10021b8eu:{st[0]=product(st[0],fp((r[2]+16u)));pc=0x10021b91u;break;} // d84910 fmul dword ptr [ecx + 0x10]
case 0x10021b91u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b93u;break;} // dee9 fsubp st(1)
case 0x10021b93u:{pushfp(fp((r[2]+28u)));pc=0x10021b96u;break;} // d9411c fld dword ptr [ecx + 0x1c]
case 0x10021b96u:{st[0]=product(st[0],fp((r[7]+12u)));pc=0x10021b9au;break;} // d84c240c fmul dword ptr [esp + 0xc]
case 0x10021b9au:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021b9cu;break;} // dee9 fsubp st(1)
case 0x10021b9cu:{fpwrite((r[0]+16u),st[0]);popfp();pc=0x10021b9fu;break;} // d95810 fstp dword ptr [eax + 0x10]
case 0x10021b9fu:{pushfp(fp((r[7]+4u)));pc=0x10021ba3u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x10021ba3u:{st[0]=product(st[0],fp((r[2]+16u)));pc=0x10021ba6u;break;} // d84910 fmul dword ptr [ecx + 0x10]
case 0x10021ba6u:{pushfp(fp((r[7])));pc=0x10021ba9u;break;} // d90424 fld dword ptr [esp]
case 0x10021ba9u:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021bacu;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021bacu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021baeu;break;} // dee9 fsubp st(1)
case 0x10021baeu:{pushfp(fp((r[2]+28u)));pc=0x10021bb1u;break;} // d9411c fld dword ptr [ecx + 0x1c]
case 0x10021bb1u:{pushfp(fp((r[7]+20u)));pc=0x10021bb5u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x10021bb5u:{st[1]=product(st[1],st[0]);pc=0x10021bb7u;break;} // dcc9 fmul st(1), st(0)
case 0x10021bb7u:{std::swap(st[0],st[2]);pc=0x10021bb9u;break;} // d9ca fxch st(2)
case 0x10021bb9u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021bbbu;break;} // dec1 faddp st(1)
case 0x10021bbbu:{fpwrite((r[0]+32u),st[0]);popfp();pc=0x10021bbeu;break;} // d95820 fstp dword ptr [eax + 0x20]
case 0x10021bbeu:{pushfp(fp((r[7]+12u)));pc=0x10021bc2u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x10021bc2u:{st[0]=product(st[0],fp((r[2]+20u)));pc=0x10021bc5u;break;} // d84914 fmul dword ptr [ecx + 0x14]
case 0x10021bc5u:{pushfp(fp((r[7]+8u)));pc=0x10021bc9u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x10021bc9u:{st[0]=product(st[0],fp((r[2]+16u)));pc=0x10021bccu;break;} // d84910 fmul dword ptr [ecx + 0x10]
case 0x10021bccu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021bceu;break;} // dee9 fsubp st(1)
case 0x10021bceu:{pushfp(fp((r[2]+24u)));pc=0x10021bd1u;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x10021bd1u:{st[2]=product(st[2],st[0]);popfp();pc=0x10021bd3u;break;} // deca fmulp st(2)
case 0x10021bd3u:{st[1]=difference(st[0],st[1]);popfp();pc=0x10021bd5u;break;} // dee1 fsubrp st(1)
case 0x10021bd5u:{fpwrite((r[0]+48u),st[0]);popfp();pc=0x10021bd8u;break;} // d95830 fstp dword ptr [eax + 0x30]
case 0x10021bd8u:{pushfp(fp((r[2]+8u)));pc=0x10021bdbu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021bdbu:{st[0]=product(st[0],fp((r[7]+4u)));pc=0x10021bdfu;break;} // d84c2404 fmul dword ptr [esp + 4]
case 0x10021bdfu:{pushfp(fp((r[7]+16u)));pc=0x10021be3u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x10021be3u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021be6u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021be6u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021be8u;break;} // dee9 fsubp st(1)
case 0x10021be8u:{pushfp(fp((r[2]+12u)));pc=0x10021bebu;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x10021bebu:{st[0]=product(st[0],fp((r[7]+8u)));pc=0x10021befu;break;} // d84c2408 fmul dword ptr [esp + 8]
case 0x10021befu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021bf1u;break;} // dee9 fsubp st(1)
case 0x10021bf1u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x10021bf4u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x10021bf4u:{pushfp(fp((r[2])));pc=0x10021bf6u;break;} // d901 fld dword ptr [ecx]
case 0x10021bf6u:{st[0]=product(st[0],fp((r[7]+16u)));pc=0x10021bfau;break;} // d84c2410 fmul dword ptr [esp + 0x10]
case 0x10021bfau:{pushfp(fp((r[2]+8u)));pc=0x10021bfdu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021bfdu:{st[0]=product(st[0],fp((r[7])));pc=0x10021c00u;break;} // d80c24 fmul dword ptr [esp]
case 0x10021c00u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c02u;break;} // dee9 fsubp st(1)
case 0x10021c02u:{pushfp(fp((r[2]+12u)));pc=0x10021c05u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x10021c05u:{st[0]=product(st[0],fp((r[7]+12u)));pc=0x10021c09u;break;} // d84c240c fmul dword ptr [esp + 0xc]
case 0x10021c09u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021c0bu;break;} // dec1 faddp st(1)
case 0x10021c0bu:{fpwrite((r[0]+20u),st[0]);popfp();pc=0x10021c0eu;break;} // d95814 fstp dword ptr [eax + 0x14]
case 0x10021c0eu:{pushfp(fp((r[7])));pc=0x10021c11u;break;} // d90424 fld dword ptr [esp]
case 0x10021c11u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021c14u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021c14u:{pushfp(fp((r[2])));pc=0x10021c16u;break;} // d901 fld dword ptr [ecx]
case 0x10021c16u:{st[0]=product(st[0],fp((r[7]+4u)));pc=0x10021c1au;break;} // d84c2404 fmul dword ptr [esp + 4]
case 0x10021c1au:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c1cu;break;} // dee9 fsubp st(1)
case 0x10021c1cu:{pushfp(fp((r[2]+12u)));pc=0x10021c1fu;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x10021c1fu:{pushfp(fp((r[7]+20u)));pc=0x10021c23u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x10021c23u:{st[1]=product(st[1],st[0]);pc=0x10021c25u;break;} // dcc9 fmul st(1), st(0)
case 0x10021c25u:{std::swap(st[0],st[2]);pc=0x10021c27u;break;} // d9ca fxch st(2)
case 0x10021c27u:{st[1]=difference(st[0],st[1]);popfp();pc=0x10021c29u;break;} // dee1 fsubrp st(1)
case 0x10021c29u:{fpwrite((r[0]+36u),st[0]);popfp();pc=0x10021c2cu;break;} // d95824 fstp dword ptr [eax + 0x24]
case 0x10021c2cu:{pushfp(fp((r[2])));pc=0x10021c2eu;break;} // d901 fld dword ptr [ecx]
case 0x10021c2eu:{st[0]=product(st[0],fp((r[7]+8u)));pc=0x10021c32u;break;} // d84c2408 fmul dword ptr [esp + 8]
case 0x10021c32u:{pushfp(fp((r[7]+12u)));pc=0x10021c36u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x10021c36u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x10021c39u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x10021c39u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c3bu;break;} // dee9 fsubp st(1)
case 0x10021c3bu:{pushfp(fp((r[2]+8u)));pc=0x10021c3eu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10021c3eu:{st[2]=product(st[2],st[0]);popfp();pc=0x10021c40u;break;} // deca fmulp st(2)
case 0x10021c40u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021c42u;break;} // dec1 faddp st(1)
case 0x10021c42u:{fpwrite((r[0]+52u),st[0]);popfp();pc=0x10021c45u;break;} // d95834 fstp dword ptr [eax + 0x34]
case 0x10021c45u:{pushfp(fp((r[2]+52u)));pc=0x10021c48u;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021c48u:{pushfp(fp((r[7]+28u)));pc=0x10021c4cu;break;} // d944241c fld dword ptr [esp + 0x1c]
case 0x10021c4cu:{st[1]=product(st[1],st[0]);pc=0x10021c4eu;break;} // dcc9 fmul st(1), st(0)
case 0x10021c4eu:{pushfp(st[2]);pc=0x10021c50u;break;} // d9c2 fld st(2)
case 0x10021c50u:{st[0]=product(st[0],fp((r[2]+56u)));pc=0x10021c53u;break;} // d84938 fmul dword ptr [ecx + 0x38]
case 0x10021c53u:{st[2]=difference(st[2],st[0]);popfp();pc=0x10021c55u;break;} // deea fsubp st(2)
case 0x10021c55u:{pushfp(fp((r[2]+60u)));pc=0x10021c58u;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021c58u:{st[0]=product(st[0],st[4]);pc=0x10021c5au;break;} // d8cc fmul st(4)
case 0x10021c5au:{st[2]=sum(st[2],st[0]);popfp();pc=0x10021c5cu;break;} // dec2 faddp st(2)
case 0x10021c5cu:{std::swap(st[0],st[1]);pc=0x10021c5eu;break;} // d9c9 fxch st(1)
case 0x10021c5eu:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x10021c61u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x10021c61u:{pushfp(st[3]);pc=0x10021c63u;break;} // d9c3 fld st(3)
case 0x10021c63u:{st[0]=product(st[0],fp((r[2]+56u)));pc=0x10021c66u;break;} // d84938 fmul dword ptr [ecx + 0x38]
case 0x10021c66u:{pushfp(fp((r[2]+48u)));pc=0x10021c69u;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021c69u:{st[0]=product(st[0],st[2]);pc=0x10021c6bu;break;} // d8ca fmul st(2)
case 0x10021c6bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c6du;break;} // dee9 fsubp st(1)
case 0x10021c6du:{pushfp(fp((r[2]+60u)));pc=0x10021c70u;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021c70u:{st[0]=product(st[0],st[6]);pc=0x10021c72u;break;} // d8ce fmul st(6)
case 0x10021c72u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c74u;break;} // dee9 fsubp st(1)
case 0x10021c74u:{fpwrite((r[0]+24u),st[0]);popfp();pc=0x10021c77u;break;} // d95818 fstp dword ptr [eax + 0x18]
case 0x10021c77u:{pushfp(fp((r[2]+48u)));pc=0x10021c7au;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021c7au:{st[0]=product(st[0],st[2]);pc=0x10021c7cu;break;} // d8ca fmul st(2)
case 0x10021c7cu:{pushfp(fp((r[2]+52u)));pc=0x10021c7fu;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021c7fu:{st[0]=product(st[0],st[5]);pc=0x10021c81u;break;} // d8cd fmul st(5)
case 0x10021c81u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c83u;break;} // dee9 fsubp st(1)
case 0x10021c83u:{pushfp(fp((r[2]+60u)));pc=0x10021c86u;break;} // d9413c fld dword ptr [ecx + 0x3c]
case 0x10021c86u:{st[0]=product(st[0],st[7]);pc=0x10021c88u;break;} // d8cf fmul st(7)
case 0x10021c88u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021c8au;break;} // dec1 faddp st(1)
case 0x10021c8au:{fpwrite((r[0]+40u),st[0]);popfp();pc=0x10021c8du;break;} // d95828 fstp dword ptr [eax + 0x28]
case 0x10021c8du:{pushfp(fp((r[2]+52u)));pc=0x10021c90u;break;} // d94134 fld dword ptr [ecx + 0x34]
case 0x10021c90u:{st[0]=product(st[0],st[5]);pc=0x10021c92u;break;} // d8cd fmul st(5)
case 0x10021c92u:{pushfp(fp((r[2]+48u)));pc=0x10021c95u;break;} // d94130 fld dword ptr [ecx + 0x30]
case 0x10021c95u:{st[0]=product(st[0],st[4]);pc=0x10021c97u;break;} // d8cc fmul st(4)
case 0x10021c97u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021c99u;break;} // dee9 fsubp st(1)
case 0x10021c99u:{pushfp(st[6]);pc=0x10021c9bu;break;} // d9c6 fld st(6)
case 0x10021c9bu:{st[0]=product(st[0],fp((r[2]+56u)));pc=0x10021c9eu;break;} // d84938 fmul dword ptr [ecx + 0x38]
case 0x10021c9eu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021ca0u;break;} // dee9 fsubp st(1)
case 0x10021ca0u:{fpwrite((r[0]+56u),st[0]);popfp();pc=0x10021ca3u;break;} // d95838 fstp dword ptr [eax + 0x38]
case 0x10021ca3u:{pushfp(fp((r[2]+40u)));pc=0x10021ca6u;break;} // d94128 fld dword ptr [ecx + 0x28]
case 0x10021ca6u:{st[0]=product(st[0],st[2]);pc=0x10021ca8u;break;} // d8ca fmul st(2)
case 0x10021ca8u:{pushfp(fp((r[2]+36u)));pc=0x10021cabu;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x10021cabu:{st[0]=product(st[0],st[2]);pc=0x10021cadu;break;} // d8ca fmul st(2)
case 0x10021cadu:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021cafu;break;} // dee9 fsubp st(1)
case 0x10021cafu:{pushfp(st[3]);pc=0x10021cb1u;break;} // d9c3 fld st(3)
case 0x10021cb1u:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021cb4u;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021cb4u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021cb6u;break;} // dee9 fsubp st(1)
case 0x10021cb6u:{fpwrite((r[0]+12u),st[0]);popfp();pc=0x10021cb9u;break;} // d9580c fstp dword ptr [eax + 0xc]
case 0x10021cb9u:{st[0]=product(st[0],fp((r[2]+32u)));pc=0x10021cbcu;break;} // d84920 fmul dword ptr [ecx + 0x20]
case 0x10021cbcu:{pushfp(fp((r[2]+40u)));pc=0x10021cbfu;break;} // d94128 fld dword ptr [ecx + 0x28]
case 0x10021cbfu:{st[0]=product(st[0],st[4]);pc=0x10021cc1u;break;} // d8cc fmul st(4)
case 0x10021cc1u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021cc3u;break;} // dee9 fsubp st(1)
case 0x10021cc3u:{pushfp(st[4]);pc=0x10021cc5u;break;} // d9c4 fld st(4)
case 0x10021cc5u:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021cc8u;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021cc8u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021ccau;break;} // dec1 faddp st(1)
case 0x10021ccau:{fpwrite((r[0]+28u),st[0]);popfp();pc=0x10021ccdu;break;} // d9581c fstp dword ptr [eax + 0x1c]
case 0x10021ccdu:{pushfp(fp((r[2]+36u)));pc=0x10021cd0u;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x10021cd0u:{st[3]=product(st[3],st[0]);popfp();pc=0x10021cd2u;break;} // decb fmulp st(3)
case 0x10021cd2u:{st[0]=product(st[0],fp((r[2]+32u)));pc=0x10021cd5u;break;} // d84920 fmul dword ptr [ecx + 0x20]
case 0x10021cd5u:{st[2]=difference(st[2],st[0]);popfp();pc=0x10021cd7u;break;} // deea fsubp st(2)
case 0x10021cd7u:{pushfp(st[3]);pc=0x10021cd9u;break;} // d9c3 fld st(3)
case 0x10021cd9u:{st[0]=product(st[0],fp((r[2]+44u)));pc=0x10021cdcu;break;} // d8492c fmul dword ptr [ecx + 0x2c]
case 0x10021cdcu:{st[2]=difference(st[2],st[0]);popfp();pc=0x10021cdeu;break;} // deea fsubp st(2)
case 0x10021cdeu:{std::swap(st[0],st[1]);pc=0x10021ce0u;break;} // d9c9 fxch st(1)
case 0x10021ce0u:{fpwrite((r[0]+44u),st[0]);popfp();pc=0x10021ce3u;break;} // d9582c fstp dword ptr [eax + 0x2c]
case 0x10021ce3u:{st[0]=product(st[0],fp((r[2]+32u)));pc=0x10021ce6u;break;} // d84920 fmul dword ptr [ecx + 0x20]
case 0x10021ce6u:{pushfp(fp((r[2]+36u)));pc=0x10021ce9u;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x10021ce9u:{st[2]=product(st[2],st[0]);popfp();pc=0x10021cebu;break;} // deca fmulp st(2)
case 0x10021cebu:{st[1]=difference(st[0],st[1]);popfp();pc=0x10021cedu;break;} // dee1 fsubrp st(1)
case 0x10021cedu:{pushfp(fp((r[2]+40u)));pc=0x10021cf0u;break;} // d94128 fld dword ptr [ecx + 0x28]
case 0x10021cf0u:{st[2]=product(st[2],st[0]);popfp();pc=0x10021cf2u;break;} // deca fmulp st(2)
case 0x10021cf2u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021cf4u;break;} // dec1 faddp st(1)
case 0x10021cf4u:{fpwrite((r[0]+60u),st[0]);popfp();pc=0x10021cf7u;break;} // d9583c fstp dword ptr [eax + 0x3c]
case 0x10021cf7u:{pushfp(1.);pc=0x10021cf9u;break;} // d9e8 fld1
case 0x10021cf9u:{st[0]=quotient(st[0],fp((r[7]+24u)));pc=0x10021cfdu;break;} // d8742418 fdiv dword ptr [esp + 0x18]
case 0x10021cfdu:{pushfp(fp((r[0])));pc=0x10021cffu;break;} // d900 fld dword ptr [eax]
case 0x10021cffu:{st[0]=product(st[0],st[1]);pc=0x10021d01u;break;} // d8c9 fmul st(1)
case 0x10021d01u:{fpwrite((r[0]),st[0]);popfp();pc=0x10021d03u;break;} // d918 fstp dword ptr [eax]
case 0x10021d03u:{pushfp(st[0]);pc=0x10021d05u;break;} // d9c0 fld st(0)
case 0x10021d05u:{st[0]=product(st[0],fp((r[0]+4u)));pc=0x10021d08u;break;} // d84804 fmul dword ptr [eax + 4]
case 0x10021d08u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x10021d0bu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x10021d0bu:{pushfp(st[0]);pc=0x10021d0du;break;} // d9c0 fld st(0)
case 0x10021d0du:{st[0]=product(st[0],fp((r[0]+8u)));pc=0x10021d10u;break;} // d84808 fmul dword ptr [eax + 8]
case 0x10021d10u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x10021d13u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x10021d13u:{pushfp(st[0]);pc=0x10021d15u;break;} // d9c0 fld st(0)
case 0x10021d15u:{st[0]=product(st[0],fp((r[0]+12u)));pc=0x10021d18u;break;} // d8480c fmul dword ptr [eax + 0xc]
case 0x10021d18u:{fpwrite((r[0]+12u),st[0]);popfp();pc=0x10021d1bu;break;} // d9580c fstp dword ptr [eax + 0xc]
case 0x10021d1bu:{pushfp(fp((r[0]+16u)));pc=0x10021d1eu;break;} // d94010 fld dword ptr [eax + 0x10]
case 0x10021d1eu:{st[0]=product(st[0],st[1]);pc=0x10021d20u;break;} // d8c9 fmul st(1)
case 0x10021d20u:{fpwrite((r[0]+16u),st[0]);popfp();pc=0x10021d23u;break;} // d95810 fstp dword ptr [eax + 0x10]
case 0x10021d23u:{pushfp(fp((r[0]+20u)));pc=0x10021d26u;break;} // d94014 fld dword ptr [eax + 0x14]
case 0x10021d26u:{st[0]=product(st[0],st[1]);pc=0x10021d28u;break;} // d8c9 fmul st(1)
case 0x10021d28u:{fpwrite((r[0]+20u),st[0]);popfp();pc=0x10021d2bu;break;} // d95814 fstp dword ptr [eax + 0x14]
case 0x10021d2bu:{pushfp(fp((r[0]+24u)));pc=0x10021d2eu;break;} // d94018 fld dword ptr [eax + 0x18]
case 0x10021d2eu:{st[0]=product(st[0],st[1]);pc=0x10021d30u;break;} // d8c9 fmul st(1)
case 0x10021d30u:{fpwrite((r[0]+24u),st[0]);popfp();pc=0x10021d33u;break;} // d95818 fstp dword ptr [eax + 0x18]
case 0x10021d33u:{pushfp(fp((r[0]+28u)));pc=0x10021d36u;break;} // d9401c fld dword ptr [eax + 0x1c]
case 0x10021d36u:{st[0]=product(st[0],st[1]);pc=0x10021d38u;break;} // d8c9 fmul st(1)
case 0x10021d38u:{fpwrite((r[0]+28u),st[0]);popfp();pc=0x10021d3bu;break;} // d9581c fstp dword ptr [eax + 0x1c]
case 0x10021d3bu:{pushfp(fp((r[0]+32u)));pc=0x10021d3eu;break;} // d94020 fld dword ptr [eax + 0x20]
case 0x10021d3eu:{st[0]=product(st[0],st[1]);pc=0x10021d40u;break;} // d8c9 fmul st(1)
case 0x10021d40u:{fpwrite((r[0]+32u),st[0]);popfp();pc=0x10021d43u;break;} // d95820 fstp dword ptr [eax + 0x20]
case 0x10021d43u:{pushfp(fp((r[0]+36u)));pc=0x10021d46u;break;} // d94024 fld dword ptr [eax + 0x24]
case 0x10021d46u:{st[0]=product(st[0],st[1]);pc=0x10021d48u;break;} // d8c9 fmul st(1)
case 0x10021d48u:{fpwrite((r[0]+36u),st[0]);popfp();pc=0x10021d4bu;break;} // d95824 fstp dword ptr [eax + 0x24]
case 0x10021d4bu:{pushfp(fp((r[0]+40u)));pc=0x10021d4eu;break;} // d94028 fld dword ptr [eax + 0x28]
case 0x10021d4eu:{st[0]=product(st[0],st[1]);pc=0x10021d50u;break;} // d8c9 fmul st(1)
case 0x10021d50u:{fpwrite((r[0]+40u),st[0]);popfp();pc=0x10021d53u;break;} // d95828 fstp dword ptr [eax + 0x28]
case 0x10021d53u:{pushfp(fp((r[0]+44u)));pc=0x10021d56u;break;} // d9402c fld dword ptr [eax + 0x2c]
case 0x10021d56u:{st[0]=product(st[0],st[1]);pc=0x10021d58u;break;} // d8c9 fmul st(1)
case 0x10021d58u:{fpwrite((r[0]+44u),st[0]);popfp();pc=0x10021d5bu;break;} // d9582c fstp dword ptr [eax + 0x2c]
case 0x10021d5bu:{pushfp(st[0]);pc=0x10021d5du;break;} // d9c0 fld st(0)
case 0x10021d5du:{st[0]=product(st[0],fp((r[0]+48u)));pc=0x10021d60u;break;} // d84830 fmul dword ptr [eax + 0x30]
case 0x10021d60u:{fpwrite((r[0]+48u),st[0]);popfp();pc=0x10021d63u;break;} // d95830 fstp dword ptr [eax + 0x30]
case 0x10021d63u:{pushfp(st[0]);pc=0x10021d65u;break;} // d9c0 fld st(0)
case 0x10021d65u:{st[0]=product(st[0],fp((r[0]+52u)));pc=0x10021d68u;break;} // d84834 fmul dword ptr [eax + 0x34]
case 0x10021d68u:{fpwrite((r[0]+52u),st[0]);popfp();pc=0x10021d6bu;break;} // d95834 fstp dword ptr [eax + 0x34]
case 0x10021d6bu:{pushfp(st[0]);pc=0x10021d6du;break;} // d9c0 fld st(0)
case 0x10021d6du:{st[0]=product(st[0],fp((r[0]+56u)));pc=0x10021d70u;break;} // d84838 fmul dword ptr [eax + 0x38]
case 0x10021d70u:{fpwrite((r[0]+56u),st[0]);popfp();pc=0x10021d73u;break;} // d95838 fstp dword ptr [eax + 0x38]
case 0x10021d73u:{st[0]=product(st[0],fp((r[0]+60u)));pc=0x10021d76u;break;} // d8483c fmul dword ptr [eax + 0x3c]
case 0x10021d76u:{fpwrite((r[0]+60u),st[0]);popfp();pc=0x10021d79u;break;} // d9583c fstp dword ptr [eax + 0x3c]
case 0x10021d79u:{uint32_t v=(r[7])+(32u);r[7]=v;flags(v);pc=0x10021d7cu;break;} // 83c420 add esp, 0x20
case 0x10021d7cu:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x1002fb20u:{r[3]=get((r[7]+8u),4);pc=0x1002fb24u;break;} // 8b542408 mov edx, dword ptr [esp + 8]
case 0x1002fb24u:{pushfp(fp((r[2]+16u)));pc=0x1002fb27u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x1002fb27u:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1002fb2au;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1002fb2au:{r[0]=get((r[7]+4u),4);pc=0x1002fb2eu;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x1002fb2eu:{pushfp(fp((r[3])));pc=0x1002fb30u;break;} // d902 fld dword ptr [edx]
case 0x1002fb30u:{st[0]=product(st[0],fp((r[2])));pc=0x1002fb32u;break;} // d809 fmul dword ptr [ecx]
case 0x1002fb32u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb34u;break;} // dec1 faddp st(1)
case 0x1002fb34u:{pushfp(fp((r[2]+32u)));pc=0x1002fb37u;break;} // d94120 fld dword ptr [ecx + 0x20]
case 0x1002fb37u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1002fb3au;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1002fb3au:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb3cu;break;} // dec1 faddp st(1)
case 0x1002fb3cu:{fpwrite((r[0]),st[0]);popfp();pc=0x1002fb3eu;break;} // d918 fstp dword ptr [eax]
case 0x1002fb3eu:{pushfp(fp((r[2]+4u)));pc=0x1002fb41u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x1002fb41u:{st[0]=product(st[0],fp((r[3])));pc=0x1002fb43u;break;} // d80a fmul dword ptr [edx]
case 0x1002fb43u:{pushfp(fp((r[2]+20u)));pc=0x1002fb46u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x1002fb46u:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1002fb49u;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1002fb49u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb4bu;break;} // dec1 faddp st(1)
case 0x1002fb4bu:{pushfp(fp((r[2]+36u)));pc=0x1002fb4eu;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x1002fb4eu:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1002fb51u;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1002fb51u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb53u;break;} // dec1 faddp st(1)
case 0x1002fb53u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x1002fb56u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x1002fb56u:{pushfp(fp((r[2]+8u)));pc=0x1002fb59u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x1002fb59u:{st[0]=product(st[0],fp((r[3])));pc=0x1002fb5bu;break;} // d80a fmul dword ptr [edx]
case 0x1002fb5bu:{pushfp(fp((r[2]+24u)));pc=0x1002fb5eu;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x1002fb5eu:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x1002fb61u;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x1002fb61u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb63u;break;} // dec1 faddp st(1)
case 0x1002fb63u:{pushfp(fp((r[2]+40u)));pc=0x1002fb66u;break;} // d94128 fld dword ptr [ecx + 0x28]
case 0x1002fb66u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x1002fb69u;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x1002fb69u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002fb6bu;break;} // dec1 faddp st(1)
case 0x1002fb6bu:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x1002fb6eu;break;} // d95808 fstp dword ptr [eax + 8]
case 0x1002fb6eu:{pc=get(r[7],4);r[7]+=12u;if(pc==0xeeeeeeeeu)return;break;} // c20800 ret 8
case 0x10045320u:{pushfp(fp((r[2])));pc=0x10045322u;break;} // d901 fld dword ptr [ecx]
case 0x10045322u:{pushfp(fp((r[7]+4u)));pc=0x10045326u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x10045326u:{st[1]=product(st[1],st[0]);pc=0x10045328u;break;} // dcc9 fmul st(1), st(0)
case 0x10045328u:{std::swap(st[0],st[1]);pc=0x1004532au;break;} // d9c9 fxch st(1)
case 0x1004532au:{fpwrite((r[2]),st[0]);popfp();pc=0x1004532cu;break;} // d919 fstp dword ptr [ecx]
case 0x1004532cu:{pushfp(fp((r[2]+4u)));pc=0x1004532fu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x1004532fu:{st[0]=product(st[0],st[1]);pc=0x10045331u;break;} // d8c9 fmul st(1)
case 0x10045331u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x10045334u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x10045334u:{pushfp(fp((r[2]+8u)));pc=0x10045337u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x10045337u:{st[0]=product(st[0],st[1]);pc=0x10045339u;break;} // d8c9 fmul st(1)
case 0x10045339u:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x1004533cu;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x1004533cu:{pushfp(fp((r[2]+16u)));pc=0x1004533fu;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x1004533fu:{st[0]=product(st[0],st[1]);pc=0x10045341u;break;} // d8c9 fmul st(1)
case 0x10045341u:{fpwrite((r[2]+16u),st[0]);popfp();pc=0x10045344u;break;} // d95910 fstp dword ptr [ecx + 0x10]
case 0x10045344u:{pushfp(fp((r[2]+20u)));pc=0x10045347u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x10045347u:{st[0]=product(st[0],st[1]);pc=0x10045349u;break;} // d8c9 fmul st(1)
case 0x10045349u:{fpwrite((r[2]+20u),st[0]);popfp();pc=0x1004534cu;break;} // d95914 fstp dword ptr [ecx + 0x14]
case 0x1004534cu:{pushfp(fp((r[2]+24u)));pc=0x1004534fu;break;} // d94118 fld dword ptr [ecx + 0x18]
case 0x1004534fu:{st[0]=product(st[0],st[1]);pc=0x10045351u;break;} // d8c9 fmul st(1)
case 0x10045351u:{fpwrite((r[2]+24u),st[0]);popfp();pc=0x10045354u;break;} // d95918 fstp dword ptr [ecx + 0x18]
case 0x10045354u:{pushfp(fp((r[2]+32u)));pc=0x10045357u;break;} // d94120 fld dword ptr [ecx + 0x20]
case 0x10045357u:{st[0]=product(st[0],st[1]);pc=0x10045359u;break;} // d8c9 fmul st(1)
case 0x10045359u:{fpwrite((r[2]+32u),st[0]);popfp();pc=0x1004535cu;break;} // d95920 fstp dword ptr [ecx + 0x20]
case 0x1004535cu:{pushfp(fp((r[2]+36u)));pc=0x1004535fu;break;} // d94124 fld dword ptr [ecx + 0x24]
case 0x1004535fu:{st[0]=product(st[0],st[1]);pc=0x10045361u;break;} // d8c9 fmul st(1)
case 0x10045361u:{fpwrite((r[2]+36u),st[0]);popfp();pc=0x10045364u;break;} // d95924 fstp dword ptr [ecx + 0x24]
case 0x10045364u:{st[0]=product(st[0],fp((r[2]+40u)));pc=0x10045367u;break;} // d84928 fmul dword ptr [ecx + 0x28]
case 0x10045367u:{fpwrite((r[2]+40u),st[0]);popfp();pc=0x1004536au;break;} // d95928 fstp dword ptr [ecx + 0x28]
case 0x1004536au:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x100c42b8u:{pushfp(fp((r[7]+24u)));pc=0x100c42bcu;break;} // d9442418 fld dword ptr [esp + 0x18]
case 0x100c42bcu:{pushfp(st[0]);pc=0x100c42beu;break;} // d9c0 fld st(0)
case 0x100c42beu:{pushfp(fp((r[7]+20u)));pc=0x100c42c2u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x100c42c2u:{st[1]=product(st[1],st[0]);pc=0x100c42c4u;break;} // dcc9 fmul st(1), st(0)
case 0x100c42c4u:{pushfp(st[3]);pc=0x100c42c6u;break;} // d9c3 fld st(3)
case 0x100c42c6u:{st[0]=product(st[0],st[1]);pc=0x100c42c8u;break;} // d8c9 fmul st(1)
case 0x100c42c8u:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x100c42ccu;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x100c42ccu:{pushfp(fp((r[7]+4u)));pc=0x100c42d0u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x100c42d0u:{pushfp(st[0]);pc=0x100c42d2u;break;} // d9c0 fld st(0)
case 0x100c42d2u:{pushfp(fp((r[7]+8u)));pc=0x100c42d6u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x100c42d6u:{st[1]=product(st[1],st[0]);pc=0x100c42d8u;break;} // dcc9 fmul st(1), st(0)
case 0x100c42d8u:{std::swap(st[0],st[1]);pc=0x100c42dau;break;} // d9c9 fxch st(1)
case 0x100c42dau:{fpwrite((r[4]+4u),st[0]);popfp();pc=0x100c42ddu;break;} // d95e04 fstp dword ptr [esi + 4]
case 0x100c42ddu:{pushfp(st[1]);pc=0x100c42dfu;break;} // d9c1 fld st(1)
case 0x100c42dfu:{pushfp(fp((r[7]+12u)));pc=0x100c42e3u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x100c42e3u:{st[1]=product(st[1],st[0]);pc=0x100c42e5u;break;} // dcc9 fmul st(1), st(0)
case 0x100c42e5u:{std::swap(st[0],st[1]);pc=0x100c42e7u;break;} // d9c9 fxch st(1)
case 0x100c42e7u:{fpwrite((r[4]+8u),st[0]);popfp();pc=0x100c42eau;break;} // d95e08 fstp dword ptr [esi + 8]
case 0x100c42eau:{std::swap(st[0],st[3]);pc=0x100c42ecu;break;} // d9cb fxch st(3)
case 0x100c42ecu:{st[0]=-st[0];pc=0x100c42eeu;break;} // d9e0 fchs
case 0x100c42eeu:{fpwrite((r[4]+12u),st[0]);popfp();pc=0x100c42f1u;break;} // d95e0c fstp dword ptr [esi + 0xc]
case 0x100c42f1u:{pushfp(st[3]);pc=0x100c42f3u;break;} // d9c3 fld st(3)
case 0x100c42f3u:{st[0]=product(st[0],st[1]);pc=0x100c42f5u;break;} // d8c9 fmul st(1)
case 0x100c42f5u:{pushfp(st[6]);pc=0x100c42f7u;break;} // d9c6 fld st(6)
case 0x100c42f7u:{st[0]=product(st[0],st[4]);pc=0x100c42f9u;break;} // d8cc fmul st(4)
case 0x100c42f9u:{st[1]=difference(st[1],st[0]);popfp();pc=0x100c42fbu;break;} // dee9 fsubp st(1)
case 0x100c42fbu:{fpwrite((r[4]+20u),st[0]);popfp();pc=0x100c42feu;break;} // d95e14 fstp dword ptr [esi + 0x14]
case 0x100c42feu:{pushfp(st[5]);pc=0x100c4300u;break;} // d9c5 fld st(5)
case 0x100c4300u:{st[0]=product(st[0],st[1]);pc=0x100c4302u;break;} // d8c9 fmul st(1)
case 0x100c4302u:{std::swap(st[0],st[4]);pc=0x100c4304u;break;} // d9cc fxch st(4)
case 0x100c4304u:{st[0]=product(st[0],st[3]);pc=0x100c4306u;break;} // d8cb fmul st(3)
case 0x100c4306u:{st[4]=sum(st[4],st[0]);popfp();pc=0x100c4308u;break;} // dec4 faddp st(4)
case 0x100c4308u:{std::swap(st[0],st[3]);pc=0x100c430au;break;} // d9cb fxch st(3)
case 0x100c430au:{fpwrite((r[4]+24u),st[0]);popfp();pc=0x100c430du;break;} // d95e18 fstp dword ptr [esi + 0x18]
case 0x100c430du:{pushfp(st[3]);pc=0x100c430fu;break;} // d9c3 fld st(3)
case 0x100c430fu:{st[0]=product(st[0],st[1]);pc=0x100c4311u;break;} // d8c9 fmul st(1)
case 0x100c4311u:{fpwrite((r[4]+28u),st[0]);popfp();pc=0x100c4314u;break;} // d95e1c fstp dword ptr [esi + 0x1c]
case 0x100c4314u:{pushfp(fp((r[7]+24u)));pc=0x100c4318u;break;} // d9442418 fld dword ptr [esp + 0x18]
case 0x100c4318u:{pushfp(st[0]);pc=0x100c431au;break;} // d9c0 fld st(0)
case 0x100c431au:{st[0]=product(st[0],st[4]);pc=0x100c431cu;break;} // d8cc fmul st(4)
case 0x100c431cu:{pushfp(st[5]);pc=0x100c431eu;break;} // d9c5 fld st(5)
case 0x100c431eu:{st[0]=product(st[0],st[4]);pc=0x100c4320u;break;} // d8cc fmul st(4)
case 0x100c4320u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c4322u;break;} // dec1 faddp st(1)
case 0x100c4322u:{fpwrite((r[4]+36u),st[0]);popfp();pc=0x100c4325u;break;} // d95e24 fstp dword ptr [esi + 0x24]
case 0x100c4325u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c4327u;break;} // deca fmulp st(2)
case 0x100c4327u:{std::swap(st[0],st[3]);pc=0x100c4329u;break;} // d9cb fxch st(3)
case 0x100c4329u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c432bu;break;} // deca fmulp st(2)
case 0x100c432bu:{st[1]=difference(st[0],st[1]);popfp();pc=0x100c432du;break;} // dee1 fsubrp st(1)
case 0x100c432du:{fpwrite((r[4]+40u),st[0]);popfp();pc=0x100c4330u;break;} // d95e28 fstp dword ptr [esi + 0x28]
case 0x100c4330u:{st[1]=product(st[1],st[0]);popfp();pc=0x100c4332u;break;} // dec9 fmulp st(1)
case 0x100c4332u:{fpwrite((r[4]+44u),st[0]);popfp();pc=0x100c4335u;break;} // d95e2c fstp dword ptr [esi + 0x2c]
case 0x100c559du:{pushfp(fp((r[4])));pc=0x100c559fu;break;} // d906 fld dword ptr [esi]
case 0x100c559fu:{st[0]=difference(st[0],fp((r[6])));pc=0x100c55a2u;break;} // d86500 fsub dword ptr [ebp]
case 0x100c55a2u:{pushfp(fp((r[4]+4u)));pc=0x100c55a5u;break;} // d94604 fld dword ptr [esi + 4]
case 0x100c55a5u:{st[0]=difference(st[0],fp((r[6]+4u)));pc=0x100c55a8u;break;} // d86504 fsub dword ptr [ebp + 4]
case 0x100c55a8u:{pushfp(fp((r[4]+8u)));pc=0x100c55abu;break;} // d94608 fld dword ptr [esi + 8]
case 0x100c55abu:{st[0]=difference(st[0],fp((r[6]+8u)));pc=0x100c55aeu;break;} // d86508 fsub dword ptr [ebp + 8]
case 0x100c55aeu:{pushfp(fp((r[5])));pc=0x100c55b0u;break;} // d907 fld dword ptr [edi]
case 0x100c55b0u:{st[0]=difference(st[0],fp((r[4])));pc=0x100c55b2u;break;} // d826 fsub dword ptr [esi]
case 0x100c55b2u:{pushfp(fp((r[5]+4u)));pc=0x100c55b5u;break;} // d94704 fld dword ptr [edi + 4]
case 0x100c55b5u:{st[0]=difference(st[0],fp((r[4]+4u)));pc=0x100c55b8u;break;} // d86604 fsub dword ptr [esi + 4]
case 0x100c55b8u:{pushfp(fp((r[5]+8u)));pc=0x100c55bbu;break;} // d94708 fld dword ptr [edi + 8]
case 0x100c55bbu:{st[0]=difference(st[0],fp((r[4]+8u)));pc=0x100c55beu;break;} // d86608 fsub dword ptr [esi + 8]
case 0x100c55beu:{pushfp(st[0]);pc=0x100c55c0u;break;} // d9c0 fld st(0)
case 0x100c55c0u:{st[0]=product(st[0],st[5]);pc=0x100c55c2u;break;} // d8cd fmul st(5)
case 0x100c55c2u:{pushfp(st[4]);pc=0x100c55c4u;break;} // d9c4 fld st(4)
case 0x100c55c4u:{st[0]=product(st[0],st[3]);pc=0x100c55c6u;break;} // d8cb fmul st(3)
case 0x100c55c6u:{st[1]=difference(st[1],st[0]);popfp();pc=0x100c55c8u;break;} // dee9 fsubp st(1)
case 0x100c55c8u:{std::swap(st[0],st[4]);pc=0x100c55cau;break;} // d9cc fxch st(4)
case 0x100c55cau:{st[0]=product(st[0],st[3]);pc=0x100c55ccu;break;} // d8cb fmul st(3)
case 0x100c55ccu:{std::swap(st[0],st[1]);pc=0x100c55ceu;break;} // d9c9 fxch st(1)
case 0x100c55ceu:{st[0]=product(st[0],st[6]);pc=0x100c55d0u;break;} // d8ce fmul st(6)
case 0x100c55d0u:{st[1]=difference(st[1],st[0]);popfp();pc=0x100c55d2u;break;} // dee9 fsubp st(1)
case 0x100c55d2u:{std::swap(st[0],st[1]);pc=0x100c55d4u;break;} // d9c9 fxch st(1)
case 0x100c55d4u:{st[5]=product(st[5],st[0]);popfp();pc=0x100c55d6u;break;} // decd fmulp st(5)
case 0x100c55d6u:{std::swap(st[0],st[3]);pc=0x100c55d8u;break;} // d9cb fxch st(3)
case 0x100c55d8u:{st[1]=product(st[1],st[0]);popfp();pc=0x100c55dau;break;} // dec9 fmulp st(1)
case 0x100c55dau:{st[3]=difference(st[3],st[0]);popfp();pc=0x100c55dcu;break;} // deeb fsubp st(3)
case 0x100c55dcu:{fpwrite((r[2]),st[0]);pc=0x100c55deu;break;} // d911 fst dword ptr [ecx]
case 0x100c55deu:{std::swap(st[0],st[1]);pc=0x100c55e0u;break;} // d9c9 fxch st(1)
case 0x100c55e0u:{fpwrite((r[2]+4u),st[0]);pc=0x100c55e3u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x100c55e3u:{std::swap(st[0],st[2]);pc=0x100c55e5u;break;} // d9ca fxch st(2)
case 0x100c55e5u:{fpwrite((r[2]+8u),st[0]);pc=0x100c55e8u;break;} // d95108 fst dword ptr [ecx + 8]
case 0x100c55e8u:{pushfp(st[1]);pc=0x100c55eau;break;} // d9c1 fld st(1)
case 0x100c55eau:{st[0]=product(st[0],st[2]);pc=0x100c55ecu;break;} // d8ca fmul st(2)
case 0x100c55ecu:{pushfp(st[3]);pc=0x100c55eeu;break;} // d9c3 fld st(3)
case 0x100c55eeu:{st[0]=product(st[0],st[4]);pc=0x100c55f0u;break;} // d8cc fmul st(4)
case 0x100c55f0u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c55f2u;break;} // dec1 faddp st(1)
case 0x100c55f2u:{pushfp(st[1]);pc=0x100c55f4u;break;} // d9c1 fld st(1)
case 0x100c55f4u:{st[0]=product(st[0],st[2]);pc=0x100c55f6u;break;} // d8ca fmul st(2)
case 0x100c55f6u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c55f8u;break;} // dec1 faddp st(1)
case 0x100c55f8u:{st[0]=root(st[0]);pc=0x100c55fau;break;} // d9fa fsqrt
case 0x100c55fau:{pushfp(fp((269687832u)));pc=0x100c5600u;break;} // d905181c1310 fld dword ptr [0x10131c18]
case 0x100c5600u:{compare(st[0],st[1]);popfp();pc=0x100c5602u;break;} // d8d9 fcomp st(1)
case 0x100c5602u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c5604u;break;} // dfe0 fnstsw ax
case 0x100c5604u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100c5607u;break;} // f6c405 test ah, 5
case 0x100c5607u:{pc=(pf)?0x100c562fu:0x100c5609u;break;} // 7a26 jp 0x100c562f
case 0x100c5609u:{pushfp(fp((269687828u)));pc=0x100c560fu;break;} // d905141c1310 fld dword ptr [0x10131c14]
case 0x100c560fu:{compare(st[0],st[1]);popfp();pc=0x100c5611u;break;} // d8d9 fcomp st(1)
case 0x100c5611u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c5613u;break;} // dfe0 fnstsw ax
case 0x100c5613u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100c5616u;break;} // f6c441 test ah, 0x41
case 0x100c5616u:{pc=(!zf)?0x100c562fu:0x100c5618u;break;} // 7517 jne 0x100c562f
case 0x100c5618u:{st[0]=st[0];popfp();pc=0x100c561au;break;} // ddd8 fstp st(0)
case 0x100c561au:{st[1]=st[0];popfp();pc=0x100c561cu;break;} // ddd9 fstp st(1)
case 0x100c561cu:{st[1]=st[0];popfp();pc=0x100c561eu;break;} // ddd9 fstp st(1)
case 0x100c561eu:{st[0]=st[0];popfp();pc=0x100c5620u;break;} // ddd8 fstp st(0)
case 0x100c5620u:{pushfp(0.);pc=0x100c5622u;break;} // d9ee fldz
case 0x100c5622u:{fpwrite((r[2]+12u),st[0]);pc=0x100c5625u;break;} // d9510c fst dword ptr [ecx + 0xc]
case 0x100c5625u:{fpwrite((r[2]+8u),st[0]);pc=0x100c5628u;break;} // d95108 fst dword ptr [ecx + 8]
case 0x100c5628u:{fpwrite((r[2]+4u),st[0]);pc=0x100c562bu;break;} // d95104 fst dword ptr [ecx + 4]
case 0x100c562bu:{fpwrite((r[2]),st[0]);pc=0x100c562du;break;} // d911 fst dword ptr [ecx]
case 0x100c562du:{pc=(true)?0x100c5643u:0x100c562fu;break;} // eb14 jmp 0x100c5643
case 0x100c562fu:{st[2]=quotient(st[2],st[0]);pc=0x100c5631u;break;} // dcfa fdiv st(2), st(0)
case 0x100c5631u:{std::swap(st[0],st[2]);pc=0x100c5633u;break;} // d9ca fxch st(2)
case 0x100c5633u:{fpwrite((r[2]),st[0]);popfp();pc=0x100c5635u;break;} // d919 fstp dword ptr [ecx]
case 0x100c5635u:{std::swap(st[0],st[2]);pc=0x100c5637u;break;} // d9ca fxch st(2)
case 0x100c5637u:{st[0]=quotient(st[0],st[1]);pc=0x100c5639u;break;} // d8f1 fdiv st(1)
case 0x100c5639u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x100c563cu;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x100c563cu:{st[1]=quotient(st[1],st[0]);popfp();pc=0x100c563eu;break;} // def9 fdivp st(1)
case 0x100c563eu:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x100c5641u;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x100c5641u:{pushfp(0.);pc=0x100c5643u;break;} // d9ee fldz
case 0x100c5643u:{pushfp(fp((r[2]+4u)));pc=0x100c5646u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x100c5646u:{st[0]=product(st[0],fp((r[5]+4u)));pc=0x100c5649u;break;} // d84f04 fmul dword ptr [edi + 4]
case 0x100c5649u:{pushfp(fp((r[5])));pc=0x100c564bu;break;} // d907 fld dword ptr [edi]
case 0x100c564bu:{st[0]=product(st[0],fp((r[2])));pc=0x100c564du;break;} // d809 fmul dword ptr [ecx]
case 0x100c564du:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c564fu;break;} // dec1 faddp st(1)
case 0x100c564fu:{pushfp(fp((r[2]+8u)));pc=0x100c5652u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x100c5652u:{st[0]=product(st[0],fp((r[5]+8u)));pc=0x100c5655u;break;} // d84f08 fmul dword ptr [edi + 8]
case 0x100c5655u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c5657u;break;} // dec1 faddp st(1)
case 0x100c5657u:{fpwrite((r[2]+12u),st[0]);popfp();pc=0x100c565au;break;} // d9590c fstp dword ptr [ecx + 0xc]
