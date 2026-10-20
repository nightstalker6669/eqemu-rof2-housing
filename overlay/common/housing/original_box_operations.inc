case 0x7a2b90u:{r[0]=get((r[7]+4u),4);pc=0x7a2b94u;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x7a2b94u:{pushfp(fp((r[2])));pc=0x7a2b96u;break;} // d901 fld dword ptr [ecx]
case 0x7a2b96u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x7a2b99u;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x7a2b99u:{pushfp(fp((r[2]+4u)));pc=0x7a2b9cu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a2b9cu:{st[0]=difference(st[0],fp((r[2]+16u)));pc=0x7a2b9fu;break;} // d86110 fsub dword ptr [ecx + 0x10]
case 0x7a2b9fu:{pushfp(fp((r[2]+8u)));pc=0x7a2ba2u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a2ba2u:{st[0]=difference(st[0],fp((r[2]+20u)));pc=0x7a2ba5u;break;} // d86114 fsub dword ptr [ecx + 0x14]
case 0x7a2ba5u:{std::swap(st[0],st[2]);pc=0x7a2ba7u;break;} // d9ca fxch st(2)
case 0x7a2ba7u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a2ba9u;break;} // d918 fstp dword ptr [eax]
case 0x7a2ba9u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a2bacu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a2bacu:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a2bafu;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a2bafu:{pushfp(fp((r[2])));pc=0x7a2bb1u;break;} // d901 fld dword ptr [ecx]
case 0x7a2bb1u:{st[0]=sum(st[0],fp((r[2]+12u)));pc=0x7a2bb4u;break;} // d8410c fadd dword ptr [ecx + 0xc]
case 0x7a2bb4u:{pushfp(fp((r[2]+16u)));pc=0x7a2bb7u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x7a2bb7u:{st[0]=sum(st[0],fp((r[2]+4u)));pc=0x7a2bbau;break;} // d84104 fadd dword ptr [ecx + 4]
case 0x7a2bbau:{pushfp(fp((r[2]+20u)));pc=0x7a2bbdu;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x7a2bbdu:{st[0]=sum(st[0],fp((r[2]+8u)));pc=0x7a2bc0u;break;} // d84108 fadd dword ptr [ecx + 8]
case 0x7a2bc0u:{std::swap(st[0],st[2]);pc=0x7a2bc2u;break;} // d9ca fxch st(2)
case 0x7a2bc2u:{fpwrite((r[0]+12u),st[0]);popfp();pc=0x7a2bc5u;break;} // d9580c fstp dword ptr [eax + 0xc]
case 0x7a2bc5u:{fpwrite((r[0]+16u),st[0]);popfp();pc=0x7a2bc8u;break;} // d95810 fstp dword ptr [eax + 0x10]
case 0x7a2bc8u:{fpwrite((r[0]+20u),st[0]);popfp();pc=0x7a2bcbu;break;} // d95814 fstp dword ptr [eax + 0x14]
case 0x7a2bcbu:{pushfp(fp((r[0]+4u)));pc=0x7a2bceu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a2bceu:{pushfp(fp((r[0]+20u)));pc=0x7a2bd1u;break;} // d94014 fld dword ptr [eax + 0x14]
case 0x7a2bd1u:{pushfp(fp((r[0])));pc=0x7a2bd3u;break;} // d900 fld dword ptr [eax]
case 0x7a2bd3u:{fpwrite((r[0]+24u),st[0]);popfp();pc=0x7a2bd6u;break;} // d95818 fstp dword ptr [eax + 0x18]
case 0x7a2bd6u:{std::swap(st[0],st[1]);pc=0x7a2bd8u;break;} // d9c9 fxch st(1)
case 0x7a2bd8u:{fpwrite((r[0]+28u),st[0]);popfp();pc=0x7a2bdbu;break;} // d9581c fstp dword ptr [eax + 0x1c]
case 0x7a2bdbu:{fpwrite((r[0]+32u),st[0]);popfp();pc=0x7a2bdeu;break;} // d95820 fstp dword ptr [eax + 0x20]
case 0x7a2bdeu:{pushfp(fp((r[0]+16u)));pc=0x7a2be1u;break;} // d94010 fld dword ptr [eax + 0x10]
case 0x7a2be1u:{pushfp(fp((r[0]+8u)));pc=0x7a2be4u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a2be4u:{pushfp(fp((r[0])));pc=0x7a2be6u;break;} // d900 fld dword ptr [eax]
case 0x7a2be6u:{fpwrite((r[0]+36u),st[0]);popfp();pc=0x7a2be9u;break;} // d95824 fstp dword ptr [eax + 0x24]
case 0x7a2be9u:{std::swap(st[0],st[1]);pc=0x7a2bebu;break;} // d9c9 fxch st(1)
case 0x7a2bebu:{fpwrite((r[0]+40u),st[0]);popfp();pc=0x7a2beeu;break;} // d95828 fstp dword ptr [eax + 0x28]
case 0x7a2beeu:{fpwrite((r[0]+44u),st[0]);popfp();pc=0x7a2bf1u;break;} // d9582c fstp dword ptr [eax + 0x2c]
case 0x7a2bf1u:{pushfp(fp((r[0]+16u)));pc=0x7a2bf4u;break;} // d94010 fld dword ptr [eax + 0x10]
case 0x7a2bf4u:{pushfp(fp((r[0]+20u)));pc=0x7a2bf7u;break;} // d94014 fld dword ptr [eax + 0x14]
case 0x7a2bf7u:{pushfp(fp((r[0])));pc=0x7a2bf9u;break;} // d900 fld dword ptr [eax]
case 0x7a2bf9u:{fpwrite((r[0]+48u),st[0]);popfp();pc=0x7a2bfcu;break;} // d95830 fstp dword ptr [eax + 0x30]
case 0x7a2bfcu:{std::swap(st[0],st[1]);pc=0x7a2bfeu;break;} // d9c9 fxch st(1)
case 0x7a2bfeu:{fpwrite((r[0]+52u),st[0]);popfp();pc=0x7a2c01u;break;} // d95834 fstp dword ptr [eax + 0x34]
case 0x7a2c01u:{fpwrite((r[0]+56u),st[0]);popfp();pc=0x7a2c04u;break;} // d95838 fstp dword ptr [eax + 0x38]
case 0x7a2c04u:{pushfp(fp((r[0]+4u)));pc=0x7a2c07u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a2c07u:{pushfp(fp((r[0]+8u)));pc=0x7a2c0au;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a2c0au:{pushfp(fp((r[0]+12u)));pc=0x7a2c0du;break;} // d9400c fld dword ptr [eax + 0xc]
case 0x7a2c0du:{fpwrite((r[0]+60u),st[0]);popfp();pc=0x7a2c10u;break;} // d9583c fstp dword ptr [eax + 0x3c]
case 0x7a2c10u:{std::swap(st[0],st[1]);pc=0x7a2c12u;break;} // d9c9 fxch st(1)
case 0x7a2c12u:{fpwrite((r[0]+64u),st[0]);popfp();pc=0x7a2c15u;break;} // d95840 fstp dword ptr [eax + 0x40]
case 0x7a2c15u:{fpwrite((r[0]+68u),st[0]);popfp();pc=0x7a2c18u;break;} // d95844 fstp dword ptr [eax + 0x44]
case 0x7a2c18u:{pushfp(fp((r[0]+4u)));pc=0x7a2c1bu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a2c1bu:{pushfp(fp((r[0]+20u)));pc=0x7a2c1eu;break;} // d94014 fld dword ptr [eax + 0x14]
case 0x7a2c1eu:{pushfp(fp((r[0]+12u)));pc=0x7a2c21u;break;} // d9400c fld dword ptr [eax + 0xc]
case 0x7a2c21u:{fpwrite((r[0]+72u),st[0]);popfp();pc=0x7a2c24u;break;} // d95848 fstp dword ptr [eax + 0x48]
case 0x7a2c24u:{std::swap(st[0],st[1]);pc=0x7a2c26u;break;} // d9c9 fxch st(1)
case 0x7a2c26u:{fpwrite((r[0]+76u),st[0]);popfp();pc=0x7a2c29u;break;} // d9584c fstp dword ptr [eax + 0x4c]
case 0x7a2c29u:{fpwrite((r[0]+80u),st[0]);popfp();pc=0x7a2c2cu;break;} // d95850 fstp dword ptr [eax + 0x50]
case 0x7a2c2cu:{pushfp(fp((r[0]+16u)));pc=0x7a2c2fu;break;} // d94010 fld dword ptr [eax + 0x10]
case 0x7a2c2fu:{pushfp(fp((r[0]+8u)));pc=0x7a2c32u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a2c32u:{pushfp(fp((r[0]+12u)));pc=0x7a2c35u;break;} // d9400c fld dword ptr [eax + 0xc]
case 0x7a2c35u:{fpwrite((r[0]+84u),st[0]);popfp();pc=0x7a2c38u;break;} // d95854 fstp dword ptr [eax + 0x54]
case 0x7a2c38u:{std::swap(st[0],st[1]);pc=0x7a2c3au;break;} // d9c9 fxch st(1)
case 0x7a2c3au:{fpwrite((r[0]+88u),st[0]);popfp();pc=0x7a2c3du;break;} // d95858 fstp dword ptr [eax + 0x58]
case 0x7a2c3du:{fpwrite((r[0]+92u),st[0]);popfp();pc=0x7a2c40u;break;} // d9585c fstp dword ptr [eax + 0x5c]
case 0x7a2c40u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x7a30d0u:{uint32_t v=(r[7])-(108u);r[7]=v;flags(v);pc=0x7a30d3u;break;} // 83ec6c sub esp, 0x6c
case 0x7a30d3u:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x7a30d4u;break;} // 53 push ebx
case 0x7a30d4u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a30d5u;break;} // 55 push ebp
case 0x7a30d5u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a30d6u;break;} // 56 push esi
case 0x7a30d6u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a30d7u;break;} // 57 push edi
case 0x7a30d7u:{r[0]=(r[7]+28u);pc=0x7a30dbu;break;} // 8d44241c lea eax, [esp + 0x1c]
case 0x7a30dbu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a30dcu;break;} // 50 push eax
case 0x7a30dcu:{r[1]=r[2];pc=0x7a30deu;break;} // 8bd9 mov ebx, ecx
case 0x7a30deu:{r[7]-=4;put(r[7],0x7a30e3u,4);pc=0x7a2b90u;break;} // e8adfaffff call 0x7a2b90
case 0x7a30e3u:{r[4]=get((r[7]+128u),4);pc=0x7a30eau;break;} // 8bb42480000000 mov esi, dword ptr [esp + 0x80]
case 0x7a30eau:{pushfp(fp((r[4])));pc=0x7a30ecu;break;} // d906 fld dword ptr [esi]
case 0x7a30ecu:{r[5]=(r[7]+36u);pc=0x7a30f0u;break;} // 8d7c2424 lea edi, [esp + 0x24]
case 0x7a30f0u:{st[0]=product(st[0],fp((r[1])));pc=0x7a30f2u;break;} // d80b fmul dword ptr [ebx]
case 0x7a30f2u:{r[6]=8u;pc=0x7a30f7u;break;} // bd08000000 mov ebp, 8
case 0x7a30f7u:{pushfp(fp((r[4]+16u)));pc=0x7a30fau;break;} // d94610 fld dword ptr [esi + 0x10]
case 0x7a30fau:{st[0]=product(st[0],fp((r[1]+4u)));pc=0x7a30fdu;break;} // d84b04 fmul dword ptr [ebx + 4]
case 0x7a30fdu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a30ffu;break;} // dec1 faddp st(1)
case 0x7a30ffu:{pushfp(fp((r[4]+32u)));pc=0x7a3102u;break;} // d94620 fld dword ptr [esi + 0x20]
case 0x7a3102u:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a3105u;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a3105u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3107u;break;} // dec1 faddp st(1)
case 0x7a3107u:{pushfp(fp((r[1]+4u)));pc=0x7a310au;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a310au:{st[0]=product(st[0],fp((r[4]+20u)));pc=0x7a310du;break;} // d84e14 fmul dword ptr [esi + 0x14]
case 0x7a310du:{pushfp(fp((r[4]+4u)));pc=0x7a3110u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a3110u:{st[0]=product(st[0],fp((r[1])));pc=0x7a3112u;break;} // d80b fmul dword ptr [ebx]
case 0x7a3112u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3114u;break;} // dec1 faddp st(1)
case 0x7a3114u:{pushfp(fp((r[4]+36u)));pc=0x7a3117u;break;} // d94624 fld dword ptr [esi + 0x24]
case 0x7a3117u:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a311au;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a311au:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a311cu;break;} // dec1 faddp st(1)
case 0x7a311cu:{pushfp(fp((r[1]+4u)));pc=0x7a311fu;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a311fu:{st[0]=product(st[0],fp((r[4]+24u)));pc=0x7a3122u;break;} // d84e18 fmul dword ptr [esi + 0x18]
case 0x7a3122u:{pushfp(fp((r[4]+8u)));pc=0x7a3125u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a3125u:{st[0]=product(st[0],fp((r[1])));pc=0x7a3127u;break;} // d80b fmul dword ptr [ebx]
case 0x7a3127u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3129u;break;} // dec1 faddp st(1)
case 0x7a3129u:{pushfp(fp((r[4]+40u)));pc=0x7a312cu;break;} // d94628 fld dword ptr [esi + 0x28]
case 0x7a312cu:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a312fu;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a312fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3131u;break;} // dec1 faddp st(1)
case 0x7a3131u:{std::swap(st[0],st[2]);pc=0x7a3133u;break;} // d9ca fxch st(2)
case 0x7a3133u:{fpwrite((r[1]),st[0]);popfp();pc=0x7a3135u;break;} // d91b fstp dword ptr [ebx]
case 0x7a3135u:{fpwrite((r[1]+4u),st[0]);popfp();pc=0x7a3138u;break;} // d95b04 fstp dword ptr [ebx + 4]
case 0x7a3138u:{fpwrite((r[1]+8u),st[0]);popfp();pc=0x7a313bu;break;} // d95b08 fstp dword ptr [ebx + 8]
case 0x7a313bu:{pushfp(0.);pc=0x7a313du;break;} // d9ee fldz
case 0x7a313du:{fpwrite((r[1]+12u),st[0]);pc=0x7a3140u;break;} // d9530c fst dword ptr [ebx + 0xc]
case 0x7a3140u:{fpwrite((r[1]+16u),st[0]);pc=0x7a3143u;break;} // d95310 fst dword ptr [ebx + 0x10]
case 0x7a3143u:{fpwrite((r[1]+20u),st[0]);popfp();pc=0x7a3146u;break;} // d95b14 fstp dword ptr [ebx + 0x14]
case 0x7a3146u:{pushfp(fp((r[4]+16u)));pc=0x7a3149u;break;} // d94610 fld dword ptr [esi + 0x10]
case 0x7a3149u:{r[2]=(r[7]+16u);pc=0x7a314du;break;} // 8d4c2410 lea ecx, [esp + 0x10]
case 0x7a314du:{st[0]=product(st[0],fp((r[5]+4294967292u)));pc=0x7a3150u;break;} // d84ffc fmul dword ptr [edi - 4]
case 0x7a3150u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a3151u;break;} // 51 push ecx
case 0x7a3151u:{pushfp(fp((r[5]+4294967288u)));pc=0x7a3154u;break;} // d947f8 fld dword ptr [edi - 8]
case 0x7a3154u:{r[2]=r[1];pc=0x7a3156u;break;} // 8bcb mov ecx, ebx
case 0x7a3156u:{st[0]=product(st[0],fp((r[4])));pc=0x7a3158u;break;} // d80e fmul dword ptr [esi]
case 0x7a3158u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a315au;break;} // dec1 faddp st(1)
case 0x7a315au:{pushfp(fp((r[4]+32u)));pc=0x7a315du;break;} // d94620 fld dword ptr [esi + 0x20]
case 0x7a315du:{st[0]=product(st[0],fp((r[5])));pc=0x7a315fu;break;} // d80f fmul dword ptr [edi]
case 0x7a315fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3161u;break;} // dec1 faddp st(1)
case 0x7a3161u:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x7a3165u;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x7a3165u:{pushfp(fp((r[4]+4u)));pc=0x7a3168u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a3168u:{st[0]=product(st[0],fp((r[5]+4294967288u)));pc=0x7a316bu;break;} // d84ff8 fmul dword ptr [edi - 8]
case 0x7a316bu:{pushfp(fp((r[5]+4294967292u)));pc=0x7a316eu;break;} // d947fc fld dword ptr [edi - 4]
case 0x7a316eu:{st[0]=product(st[0],fp((r[4]+20u)));pc=0x7a3171u;break;} // d84e14 fmul dword ptr [esi + 0x14]
case 0x7a3171u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3173u;break;} // dec1 faddp st(1)
case 0x7a3173u:{pushfp(fp((r[4]+36u)));pc=0x7a3176u;break;} // d94624 fld dword ptr [esi + 0x24]
case 0x7a3176u:{st[0]=product(st[0],fp((r[5])));pc=0x7a3178u;break;} // d80f fmul dword ptr [edi]
case 0x7a3178u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a317au;break;} // dec1 faddp st(1)
case 0x7a317au:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a317eu;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a317eu:{pushfp(fp((r[4]+8u)));pc=0x7a3181u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a3181u:{st[0]=product(st[0],fp((r[5]+4294967288u)));pc=0x7a3184u;break;} // d84ff8 fmul dword ptr [edi - 8]
case 0x7a3184u:{pushfp(fp((r[5]+4294967292u)));pc=0x7a3187u;break;} // d947fc fld dword ptr [edi - 4]
case 0x7a3187u:{st[0]=product(st[0],fp((r[4]+24u)));pc=0x7a318au;break;} // d84e18 fmul dword ptr [esi + 0x18]
case 0x7a318au:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a318cu;break;} // dec1 faddp st(1)
case 0x7a318cu:{pushfp(fp((r[4]+40u)));pc=0x7a318fu;break;} // d94628 fld dword ptr [esi + 0x28]
case 0x7a318fu:{st[0]=product(st[0],fp((r[5])));pc=0x7a3191u;break;} // d80f fmul dword ptr [edi]
case 0x7a3191u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3193u;break;} // dec1 faddp st(1)
case 0x7a3193u:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a3197u;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a3197u:{r[7]-=4;put(r[7],0x7a319cu,4);pc=0x7a31b0u;break;} // e814000000 call 0x7a31b0
case 0x7a319cu:{uint32_t v=(r[5])+(12u);r[5]=v;flags(v);pc=0x7a319fu;break;} // 83c70c add edi, 0xc
case 0x7a319fu:{uint32_t v=(r[6])-1u;r[6]=v;flags(v);pc=0x7a31a0u;break;} // 4d dec ebp
case 0x7a31a0u:{pc=(!zf)?0x7a3146u:0x7a31a2u;break;} // 75a4 jne 0x7a3146
case 0x7a31a2u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a31a3u;break;} // 5f pop edi
case 0x7a31a3u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a31a4u;break;} // 5e pop esi
case 0x7a31a4u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a31a5u;break;} // 5d pop ebp
case 0x7a31a5u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a31a6u;break;} // 5b pop ebx
case 0x7a31a6u:{uint32_t v=(r[7])+(108u);r[7]=v;flags(v);pc=0x7a31a9u;break;} // 83c46c add esp, 0x6c
case 0x7a31a9u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x7a31b0u:{pushfp(fp((r[2])));pc=0x7a31b2u;break;} // d901 fld dword ptr [ecx]
case 0x7a31b2u:{r[3]=get((r[7]+4u),4);pc=0x7a31b6u;break;} // 8b542404 mov edx, dword ptr [esp + 4]
case 0x7a31b6u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x7a31b9u;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x7a31b9u:{uint32_t v=(r[7])-(24u);r[7]=v;flags(v);pc=0x7a31bcu;break;} // 83ec18 sub esp, 0x18
case 0x7a31bcu:{pushfp(fp((r[2]+4u)));pc=0x7a31bfu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a31bfu:{st[0]=difference(st[0],fp((r[2]+16u)));pc=0x7a31c2u;break;} // d86110 fsub dword ptr [ecx + 0x10]
case 0x7a31c2u:{pushfp(fp((r[2]+8u)));pc=0x7a31c5u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a31c5u:{st[0]=difference(st[0],fp((r[2]+20u)));pc=0x7a31c8u;break;} // d86114 fsub dword ptr [ecx + 0x14]
case 0x7a31c8u:{pushfp(fp((r[2])));pc=0x7a31cau;break;} // d901 fld dword ptr [ecx]
case 0x7a31cau:{st[0]=sum(st[0],fp((r[2]+12u)));pc=0x7a31cdu;break;} // d8410c fadd dword ptr [ecx + 0xc]
case 0x7a31cdu:{pushfp(fp((r[2]+4u)));pc=0x7a31d0u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a31d0u:{st[0]=sum(st[0],fp((r[2]+16u)));pc=0x7a31d3u;break;} // d84110 fadd dword ptr [ecx + 0x10]
case 0x7a31d3u:{pushfp(fp((r[2]+8u)));pc=0x7a31d6u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a31d6u:{st[0]=sum(st[0],fp((r[2]+20u)));pc=0x7a31d9u;break;} // d84114 fadd dword ptr [ecx + 0x14]
case 0x7a31d9u:{std::swap(st[0],st[5]);pc=0x7a31dbu;break;} // d9cd fxch st(5)
case 0x7a31dbu:{compare(st[0],fp((r[3])));pc=0x7a31ddu;break;} // d812 fcom dword ptr [edx]
case 0x7a31ddu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a31dfu;break;} // dfe0 fnstsw ax
case 0x7a31dfu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a31e2u;break;} // f6c441 test ah, 0x41
case 0x7a31e2u:{pc=(!zf)?0x7a31e8u:0x7a31e4u;break;} // 7504 jne 0x7a31e8
case 0x7a31e4u:{st[0]=st[0];popfp();pc=0x7a31e6u;break;} // ddd8 fstp st(0)
case 0x7a31e6u:{pushfp(fp((r[3])));pc=0x7a31e8u;break;} // d902 fld dword ptr [edx]
case 0x7a31e8u:{std::swap(st[0],st[4]);pc=0x7a31eau;break;} // d9cc fxch st(4)
case 0x7a31eau:{compare(st[0],fp((r[3]+4u)));pc=0x7a31edu;break;} // d85204 fcom dword ptr [edx + 4]
case 0x7a31edu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a31efu;break;} // dfe0 fnstsw ax
case 0x7a31efu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a31f2u;break;} // f6c441 test ah, 0x41
case 0x7a31f2u:{pc=(!zf)?0x7a31f9u:0x7a31f4u;break;} // 7505 jne 0x7a31f9
case 0x7a31f4u:{st[0]=st[0];popfp();pc=0x7a31f6u;break;} // ddd8 fstp st(0)
case 0x7a31f6u:{pushfp(fp((r[3]+4u)));pc=0x7a31f9u;break;} // d94204 fld dword ptr [edx + 4]
case 0x7a31f9u:{std::swap(st[0],st[3]);pc=0x7a31fbu;break;} // d9cb fxch st(3)
case 0x7a31fbu:{compare(st[0],fp((r[3]+8u)));pc=0x7a31feu;break;} // d85208 fcom dword ptr [edx + 8]
case 0x7a31feu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a3200u;break;} // dfe0 fnstsw ax
case 0x7a3200u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a3203u;break;} // f6c441 test ah, 0x41
case 0x7a3203u:{pc=(!zf)?0x7a320au:0x7a3205u;break;} // 7505 jne 0x7a320a
case 0x7a3205u:{st[0]=st[0];popfp();pc=0x7a3207u;break;} // ddd8 fstp st(0)
case 0x7a3207u:{pushfp(fp((r[3]+8u)));pc=0x7a320au;break;} // d94208 fld dword ptr [edx + 8]
case 0x7a320au:{std::swap(st[0],st[2]);pc=0x7a320cu;break;} // d9ca fxch st(2)
case 0x7a320cu:{compare(st[0],fp((r[3])));pc=0x7a320eu;break;} // d812 fcom dword ptr [edx]
case 0x7a320eu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a3210u;break;} // dfe0 fnstsw ax
case 0x7a3210u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a3213u;break;} // f6c405 test ah, 5
case 0x7a3213u:{pc=(pf)?0x7a3219u:0x7a3215u;break;} // 7a04 jp 0x7a3219
case 0x7a3215u:{st[0]=st[0];popfp();pc=0x7a3217u;break;} // ddd8 fstp st(0)
case 0x7a3217u:{pushfp(fp((r[3])));pc=0x7a3219u;break;} // d902 fld dword ptr [edx]
case 0x7a3219u:{std::swap(st[0],st[1]);pc=0x7a321bu;break;} // d9c9 fxch st(1)
case 0x7a321bu:{compare(st[0],fp((r[3]+4u)));pc=0x7a321eu;break;} // d85204 fcom dword ptr [edx + 4]
case 0x7a321eu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a3220u;break;} // dfe0 fnstsw ax
case 0x7a3220u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a3223u;break;} // f6c405 test ah, 5
case 0x7a3223u:{pc=(pf)?0x7a322au:0x7a3225u;break;} // 7a05 jp 0x7a322a
case 0x7a3225u:{st[0]=st[0];popfp();pc=0x7a3227u;break;} // ddd8 fstp st(0)
case 0x7a3227u:{pushfp(fp((r[3]+4u)));pc=0x7a322au;break;} // d94204 fld dword ptr [edx + 4]
case 0x7a322au:{std::swap(st[0],st[5]);pc=0x7a322cu;break;} // d9cd fxch st(5)
case 0x7a322cu:{compare(st[0],fp((r[3]+8u)));pc=0x7a322fu;break;} // d85208 fcom dword ptr [edx + 8]
case 0x7a322fu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a3231u;break;} // dfe0 fnstsw ax
case 0x7a3231u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a3234u;break;} // f6c405 test ah, 5
case 0x7a3234u:{pc=(pf)?0x7a323bu:0x7a3236u;break;} // 7a05 jp 0x7a323b
case 0x7a3236u:{st[0]=st[0];popfp();pc=0x7a3238u;break;} // ddd8 fstp st(0)
case 0x7a3238u:{pushfp(fp((r[3]+8u)));pc=0x7a323bu;break;} // d94208 fld dword ptr [edx + 8]
case 0x7a323bu:{pushfp(st[1]);pc=0x7a323du;break;} // d9c1 fld st(1)
case 0x7a323du:{st[0]=sum(st[0],st[5]);pc=0x7a323fu;break;} // d8c5 fadd st(5)
case 0x7a323fu:{pushfp(st[4]);pc=0x7a3241u;break;} // d9c4 fld st(4)
case 0x7a3241u:{st[0]=sum(st[0],st[7]);pc=0x7a3243u;break;} // d8c7 fadd st(7)
case 0x7a3243u:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x7a3247u;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x7a3247u:{pushfp(st[1]);pc=0x7a3249u;break;} // d9c1 fld st(1)
case 0x7a3249u:{st[0]=sum(st[0],st[4]);pc=0x7a324bu;break;} // d8c4 fadd st(4)
case 0x7a324bu:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x7a324fu;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x7a324fu:{pushfp(fp((10242456u)));pc=0x7a3255u;break;} // d90598499c00 fld dword ptr [0x9c4998]
case 0x7a3255u:{st[1]=product(st[1],st[0]);pc=0x7a3257u;break;} // dcc9 fmul st(1), st(0)
case 0x7a3257u:{std::swap(st[0],st[1]);pc=0x7a3259u;break;} // d9c9 fxch st(1)
case 0x7a3259u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a325cu;break;} // d91c24 fstp dword ptr [esp]
case 0x7a325cu:{pushfp(fp((r[7]+16u)));pc=0x7a3260u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a3260u:{st[0]=product(st[0],st[1]);pc=0x7a3262u;break;} // d8c9 fmul st(1)
case 0x7a3262u:{fpwrite((r[7]+4u),st[0]);popfp();pc=0x7a3266u;break;} // d95c2404 fstp dword ptr [esp + 4]
case 0x7a3266u:{pushfp(fp((r[7]+20u)));pc=0x7a326au;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x7a326au:{st[0]=product(st[0],st[1]);pc=0x7a326cu;break;} // d8c9 fmul st(1)
case 0x7a326cu:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a3270u;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a3270u:{pushfp(fp((r[7])));pc=0x7a3273u;break;} // d90424 fld dword ptr [esp]
case 0x7a3273u:{fpwrite((r[2]),st[0]);popfp();pc=0x7a3275u;break;} // d919 fstp dword ptr [ecx]
case 0x7a3275u:{pushfp(fp((r[7]+4u)));pc=0x7a3279u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a3279u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x7a327cu;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x7a327cu:{pushfp(fp((r[7]+8u)));pc=0x7a3280u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a3280u:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x7a3283u;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x7a3283u:{std::swap(st[0],st[2]);pc=0x7a3285u;break;} // d9ca fxch st(2)
case 0x7a3285u:{st[5]=difference(st[0],st[5]);popfp();pc=0x7a3287u;break;} // dee5 fsubrp st(5)
case 0x7a3287u:{std::swap(st[0],st[5]);pc=0x7a3289u;break;} // d9cd fxch st(5)
case 0x7a3289u:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a328bu;break;} // dee3 fsubrp st(3)
case 0x7a328bu:{std::swap(st[0],st[4]);pc=0x7a328du;break;} // d9cc fxch st(4)
case 0x7a328du:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a328fu;break;} // dee1 fsubrp st(1)
case 0x7a328fu:{std::swap(st[0],st[2]);pc=0x7a3291u;break;} // d9ca fxch st(2)
case 0x7a3291u:{st[0]=product(st[0],st[3]);pc=0x7a3293u;break;} // d8cb fmul st(3)
case 0x7a3293u:{std::swap(st[0],st[1]);pc=0x7a3295u;break;} // d9c9 fxch st(1)
case 0x7a3295u:{st[0]=product(st[0],st[3]);pc=0x7a3297u;break;} // d8cb fmul st(3)
case 0x7a3297u:{std::swap(st[0],st[2]);pc=0x7a3299u;break;} // d9ca fxch st(2)
case 0x7a3299u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a329bu;break;} // decb fmulp st(3)
case 0x7a329bu:{fpwrite((r[2]+12u),st[0]);popfp();pc=0x7a329eu;break;} // d9590c fstp dword ptr [ecx + 0xc]
case 0x7a329eu:{fpwrite((r[2]+16u),st[0]);popfp();pc=0x7a32a1u;break;} // d95910 fstp dword ptr [ecx + 0x10]
case 0x7a32a1u:{fpwrite((r[2]+20u),st[0]);popfp();pc=0x7a32a4u;break;} // d95914 fstp dword ptr [ecx + 0x14]
case 0x7a32a4u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a32a7u;break;} // 83c418 add esp, 0x18
case 0x7a32a7u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x7a32b0u:{r[0]=get((r[7]+4u),4);pc=0x7a32b4u;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x7a32b4u:{pushfp(fp((r[0])));pc=0x7a32b6u;break;} // d900 fld dword ptr [eax]
case 0x7a32b6u:{st[0]=sine(st[0]);pc=0x7a32b8u;break;} // d9fe fsin
case 0x7a32b8u:{pushfp(fp((r[0])));pc=0x7a32bau;break;} // d900 fld dword ptr [eax]
case 0x7a32bau:{st[0]=cosine(st[0]);pc=0x7a32bcu;break;} // d9ff fcos
case 0x7a32bcu:{pushfp(fp((r[0]+4u)));pc=0x7a32bfu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a32bfu:{st[0]=-st[0];pc=0x7a32c1u;break;} // d9e0 fchs
case 0x7a32c1u:{pushfp(st[0]);pc=0x7a32c3u;break;} // d9c0 fld st(0)
case 0x7a32c3u:{st[0]=sine(st[0]);pc=0x7a32c5u;break;} // d9fe fsin
case 0x7a32c5u:{std::swap(st[0],st[1]);pc=0x7a32c7u;break;} // d9c9 fxch st(1)
case 0x7a32c7u:{st[0]=cosine(st[0]);pc=0x7a32c9u;break;} // d9ff fcos
case 0x7a32c9u:{pushfp(fp((r[0]+8u)));pc=0x7a32ccu;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a32ccu:{st[0]=sine(st[0]);pc=0x7a32ceu;break;} // d9fe fsin
case 0x7a32ceu:{pushfp(fp((r[0]+8u)));pc=0x7a32d1u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a32d1u:{st[0]=cosine(st[0]);pc=0x7a32d3u;break;} // d9ff fcos
case 0x7a32d3u:{pushfp(st[1]);pc=0x7a32d5u;break;} // d9c1 fld st(1)
case 0x7a32d5u:{st[0]=product(st[0],st[4]);pc=0x7a32d7u;break;} // d8cc fmul st(4)
case 0x7a32d7u:{pushfp(st[1]);pc=0x7a32d9u;break;} // d9c1 fld st(1)
case 0x7a32d9u:{st[0]=product(st[0],st[5]);pc=0x7a32dbu;break;} // d8cd fmul st(5)
case 0x7a32dbu:{fpwrite((r[7]+4u),st[0]);popfp();pc=0x7a32dfu;break;} // d95c2404 fstp dword ptr [esp + 4]
case 0x7a32dfu:{pushfp(st[3]);pc=0x7a32e1u;break;} // d9c3 fld st(3)
case 0x7a32e1u:{st[0]=product(st[0],st[6]);pc=0x7a32e3u;break;} // d8ce fmul st(6)
case 0x7a32e3u:{fpwrite((r[2]),st[0]);popfp();pc=0x7a32e5u;break;} // d919 fstp dword ptr [ecx]
case 0x7a32e5u:{pushfp(st[3]);pc=0x7a32e7u;break;} // d9c3 fld st(3)
case 0x7a32e7u:{st[0]=product(st[0],st[7]);pc=0x7a32e9u;break;} // d8cf fmul st(7)
case 0x7a32e9u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x7a32ecu;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x7a32ecu:{std::swap(st[0],st[4]);pc=0x7a32eeu;break;} // d9cc fxch st(4)
case 0x7a32eeu:{st[0]=-st[0];pc=0x7a32f0u;break;} // d9e0 fchs
case 0x7a32f0u:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x7a32f3u;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x7a32f3u:{pushfp(st[3]);pc=0x7a32f5u;break;} // d9c3 fld st(3)
case 0x7a32f5u:{st[0]=product(st[0],st[5]);pc=0x7a32f7u;break;} // d8cd fmul st(5)
case 0x7a32f7u:{pushfp(st[1]);pc=0x7a32f9u;break;} // d9c1 fld st(1)
case 0x7a32f9u:{st[0]=product(st[0],st[7]);pc=0x7a32fbu;break;} // d8cf fmul st(7)
case 0x7a32fbu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a32fdu;break;} // dee9 fsubp st(1)
case 0x7a32fdu:{fpwrite((r[2]+16u),st[0]);popfp();pc=0x7a3300u;break;} // d95910 fstp dword ptr [ecx + 0x10]
case 0x7a3300u:{pushfp(st[0]);pc=0x7a3302u;break;} // d9c0 fld st(0)
case 0x7a3302u:{st[0]=product(st[0],st[5]);pc=0x7a3304u;break;} // d8cd fmul st(5)
case 0x7a3304u:{std::swap(st[0],st[4]);pc=0x7a3306u;break;} // d9cc fxch st(4)
case 0x7a3306u:{st[0]=product(st[0],st[6]);pc=0x7a3308u;break;} // d8ce fmul st(6)
case 0x7a3308u:{st[4]=sum(st[4],st[0]);popfp();pc=0x7a330au;break;} // dec4 faddp st(4)
case 0x7a330au:{std::swap(st[0],st[3]);pc=0x7a330cu;break;} // d9cb fxch st(3)
case 0x7a330cu:{fpwrite((r[2]+20u),st[0]);popfp();pc=0x7a330fu;break;} // d95914 fstp dword ptr [ecx + 0x14]
case 0x7a330fu:{pushfp(st[0]);pc=0x7a3311u;break;} // d9c0 fld st(0)
case 0x7a3311u:{st[0]=product(st[0],st[2]);pc=0x7a3313u;break;} // d8ca fmul st(2)
case 0x7a3313u:{fpwrite((r[2]+24u),st[0]);popfp();pc=0x7a3316u;break;} // d95918 fstp dword ptr [ecx + 0x18]
case 0x7a3316u:{pushfp(fp((r[7]+4u)));pc=0x7a331au;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a331au:{pushfp(st[0]);pc=0x7a331cu;break;} // d9c0 fld st(0)
case 0x7a331cu:{st[0]=product(st[0],st[5]);pc=0x7a331eu;break;} // d8cd fmul st(5)
case 0x7a331eu:{pushfp(st[2]);pc=0x7a3320u;break;} // d9c2 fld st(2)
case 0x7a3320u:{st[0]=product(st[0],st[7]);pc=0x7a3322u;break;} // d8cf fmul st(7)
case 0x7a3322u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3324u;break;} // dec1 faddp st(1)
case 0x7a3324u:{fpwrite((r[2]+32u),st[0]);popfp();pc=0x7a3327u;break;} // d95920 fstp dword ptr [ecx + 0x20]
case 0x7a3327u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a3329u;break;} // decd fmulp st(5)
case 0x7a3329u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a332bu;break;} // decb fmulp st(3)
case 0x7a332bu:{std::swap(st[0],st[3]);pc=0x7a332du;break;} // d9cb fxch st(3)
case 0x7a332du:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a332fu;break;} // dee2 fsubrp st(2)
case 0x7a332fu:{std::swap(st[0],st[1]);pc=0x7a3331u;break;} // d9c9 fxch st(1)
case 0x7a3331u:{fpwrite((r[2]+36u),st[0]);popfp();pc=0x7a3334u;break;} // d95924 fstp dword ptr [ecx + 0x24]
case 0x7a3334u:{st[1]=product(st[1],st[0]);popfp();pc=0x7a3336u;break;} // dec9 fmulp st(1)
case 0x7a3336u:{fpwrite((r[2]+40u),st[0]);popfp();pc=0x7a3339u;break;} // d95928 fstp dword ptr [ecx + 0x28]
case 0x7a3339u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
