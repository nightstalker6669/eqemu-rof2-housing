case 0x441bb0u:{pushfp(fp((r[2]+4u)));pc=0x441bb3u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x441bb3u:{pushfp(fp((r[2])));pc=0x441bb5u;break;} // d901 fld dword ptr [ecx]
case 0x441bb5u:{pushfp(fp((r[2]+8u)));pc=0x441bb8u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x441bb8u:{pushfp(st[1]);pc=0x441bbau;break;} // d9c1 fld st(1)
case 0x441bbau:{st[2]=product(st[2],st[0]);popfp();pc=0x441bbcu;break;} // deca fmulp st(2)
case 0x441bbcu:{pushfp(st[2]);pc=0x441bbeu;break;} // d9c2 fld st(2)
case 0x441bbeu:{st[3]=product(st[3],st[0]);popfp();pc=0x441bc0u;break;} // decb fmulp st(3)
case 0x441bc0u:{std::swap(st[0],st[1]);pc=0x441bc2u;break;} // d9c9 fxch st(1)
case 0x441bc2u:{st[2]=sum(st[2],st[0]);popfp();pc=0x441bc4u;break;} // dec2 faddp st(2)
case 0x441bc4u:{st[0]=product(st[0],st[0]);pc=0x441bc6u;break;} // dcc8 fmul st(0), st(0)
case 0x441bc6u:{st[1]=sum(st[1],st[0]);popfp();pc=0x441bc8u;break;} // dec1 faddp st(1)
case 0x441bc8u:{st[0]=root(st[0]);pc=0x441bcau;break;} // d9fa fsqrt
case 0x441bcau:{pushfp(fp((10236812u)));pc=0x441bd0u;break;} // d9058c339c00 fld dword ptr [0x9c338c]
case 0x441bd0u:{compare(st[0],st[1]);popfp();pc=0x441bd2u;break;} // d8d9 fcomp st(1)
case 0x441bd2u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x441bd4u;break;} // dfe0 fnstsw ax
case 0x441bd4u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x441bd7u;break;} // f6c405 test ah, 5
case 0x441bd7u:{pc=(pf)?0x441bf5u:0x441bd9u;break;} // 7a1c jp 0x441bf5
case 0x441bd9u:{pushfp(fp((10236808u)));pc=0x441bdfu;break;} // d90588339c00 fld dword ptr [0x9c3388]
case 0x441bdfu:{compare(st[0],st[1]);popfp();pc=0x441be1u;break;} // d8d9 fcomp st(1)
case 0x441be1u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x441be3u;break;} // dfe0 fnstsw ax
case 0x441be3u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x441be6u;break;} // f6c441 test ah, 0x41
case 0x441be6u:{pc=(!zf)?0x441bf5u:0x441be8u;break;} // 750d jne 0x441bf5
case 0x441be8u:{st[0]=st[0];popfp();pc=0x441beau;break;} // ddd8 fstp st(0)
case 0x441beau:{pushfp(0.);pc=0x441becu;break;} // d9ee fldz
case 0x441becu:{fpwrite((r[2]+8u),st[0]);pc=0x441befu;break;} // d95108 fst dword ptr [ecx + 8]
case 0x441befu:{fpwrite((r[2]+4u),st[0]);pc=0x441bf2u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x441bf2u:{fpwrite((r[2]),st[0]);pc=0x441bf4u;break;} // d911 fst dword ptr [ecx]
case 0x441bf4u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x441bf5u:{pushfp(1.);pc=0x441bf7u;break;} // d9e8 fld1
case 0x441bf7u:{st[0]=quotient(st[0],st[1]);pc=0x441bf9u;break;} // d8f1 fdiv st(1)
case 0x441bf9u:{pushfp(fp((r[2])));pc=0x441bfbu;break;} // d901 fld dword ptr [ecx]
case 0x441bfbu:{st[0]=product(st[0],st[1]);pc=0x441bfdu;break;} // d8c9 fmul st(1)
case 0x441bfdu:{fpwrite((r[2]),st[0]);popfp();pc=0x441bffu;break;} // d919 fstp dword ptr [ecx]
case 0x441bffu:{pushfp(st[0]);pc=0x441c01u;break;} // d9c0 fld st(0)
case 0x441c01u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x441c04u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x441c04u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x441c07u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x441c07u:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x441c0au;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x441c0au:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x441c0du;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x441c0du:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x509130u:{pushfp(fp((r[2]+4u)));pc=0x509133u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x509133u:{pushfp(fp((r[2])));pc=0x509135u;break;} // d901 fld dword ptr [ecx]
case 0x509135u:{pushfp(fp((r[2]+8u)));pc=0x509138u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x509138u:{pushfp(st[1]);pc=0x50913au;break;} // d9c1 fld st(1)
case 0x50913au:{st[2]=product(st[2],st[0]);popfp();pc=0x50913cu;break;} // deca fmulp st(2)
case 0x50913cu:{pushfp(st[2]);pc=0x50913eu;break;} // d9c2 fld st(2)
case 0x50913eu:{st[3]=product(st[3],st[0]);popfp();pc=0x509140u;break;} // decb fmulp st(3)
case 0x509140u:{std::swap(st[0],st[1]);pc=0x509142u;break;} // d9c9 fxch st(1)
case 0x509142u:{st[2]=sum(st[2],st[0]);popfp();pc=0x509144u;break;} // dec2 faddp st(2)
case 0x509144u:{st[0]=product(st[0],st[0]);pc=0x509146u;break;} // dcc8 fmul st(0), st(0)
case 0x509146u:{st[1]=sum(st[1],st[0]);popfp();pc=0x509148u;break;} // dec1 faddp st(1)
case 0x509148u:{st[0]=root(st[0]);pc=0x50914au;break;} // d9fa fsqrt
case 0x50914au:{pushfp(fp((10236812u)));pc=0x509150u;break;} // d9058c339c00 fld dword ptr [0x9c338c]
case 0x509150u:{compare(st[0],st[1]);popfp();pc=0x509152u;break;} // d8d9 fcomp st(1)
case 0x509152u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x509154u;break;} // dfe0 fnstsw ax
case 0x509154u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x509157u;break;} // f6c405 test ah, 5
case 0x509157u:{pc=(pf)?0x509175u:0x509159u;break;} // 7a1c jp 0x509175
case 0x509159u:{pushfp(fp((10236808u)));pc=0x50915fu;break;} // d90588339c00 fld dword ptr [0x9c3388]
case 0x50915fu:{compare(st[0],st[1]);popfp();pc=0x509161u;break;} // d8d9 fcomp st(1)
case 0x509161u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x509163u;break;} // dfe0 fnstsw ax
case 0x509163u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x509166u;break;} // f6c441 test ah, 0x41
case 0x509166u:{pc=(!zf)?0x509175u:0x509168u;break;} // 750d jne 0x509175
case 0x509168u:{st[0]=st[0];popfp();pc=0x50916au;break;} // ddd8 fstp st(0)
case 0x50916au:{pushfp(0.);pc=0x50916cu;break;} // d9ee fldz
case 0x50916cu:{fpwrite((r[2]+8u),st[0]);pc=0x50916fu;break;} // d95108 fst dword ptr [ecx + 8]
case 0x50916fu:{fpwrite((r[2]+4u),st[0]);pc=0x509172u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x509172u:{fpwrite((r[2]),st[0]);popfp();pc=0x509174u;break;} // d919 fstp dword ptr [ecx]
case 0x509174u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x509175u:{pushfp(1.);pc=0x509177u;break;} // d9e8 fld1
case 0x509177u:{st[1]=quotient(st[0],st[1]);popfp();pc=0x509179u;break;} // def1 fdivrp st(1)
case 0x509179u:{pushfp(fp((r[2])));pc=0x50917bu;break;} // d901 fld dword ptr [ecx]
case 0x50917bu:{st[0]=product(st[0],st[1]);pc=0x50917du;break;} // d8c9 fmul st(1)
case 0x50917du:{fpwrite((r[2]),st[0]);popfp();pc=0x50917fu;break;} // d919 fstp dword ptr [ecx]
case 0x50917fu:{pushfp(st[0]);pc=0x509181u;break;} // d9c0 fld st(0)
case 0x509181u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x509184u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x509184u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x509187u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x509187u:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x50918au;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x50918au:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x50918du;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x50918du:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x542830u:{r[2]=get((r[7]+12u),4);pc=0x542834u;break;} // 8b4c240c mov ecx, dword ptr [esp + 0xc]
case 0x542834u:{pushfp(fp((r[2])));pc=0x542836u;break;} // d901 fld dword ptr [ecx]
case 0x542836u:{r[0]=get((r[7]+4u),4);pc=0x54283au;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x54283au:{pushfp(fp((r[7]+8u)));pc=0x54283eu;break;} // d9442408 fld dword ptr [esp + 8]
case 0x54283eu:{st[1]=product(st[1],st[0]);pc=0x542840u;break;} // dcc9 fmul st(1), st(0)
case 0x542840u:{std::swap(st[0],st[1]);pc=0x542842u;break;} // d9c9 fxch st(1)
case 0x542842u:{fpwrite((r[0]),st[0]);popfp();pc=0x542844u;break;} // d918 fstp dword ptr [eax]
case 0x542844u:{pushfp(fp((r[2]+4u)));pc=0x542847u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x542847u:{st[0]=product(st[0],st[1]);pc=0x542849u;break;} // d8c9 fmul st(1)
case 0x542849u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x54284cu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x54284cu:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x54284fu;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x54284fu:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x542852u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x542852u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a2680u:{uint32_t v=(r[7])-(12u);r[7]=v;flags(v);pc=0x7a2683u;break;} // 83ec0c sub esp, 0xc
case 0x7a2683u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a2684u;break;} // 56 push esi
case 0x7a2684u:{r[4]=r[2];pc=0x7a2686u;break;} // 8bf1 mov esi, ecx
case 0x7a2686u:{pushfp(fp((r[4]+56u)));pc=0x7a2689u;break;} // d94638 fld dword ptr [esi + 0x38]
case 0x7a2689u:{r[2]=(r[7]+4u);pc=0x7a268du;break;} // 8d4c2404 lea ecx, [esp + 4]
case 0x7a268du:{pushfp(fp((r[4]+28u)));pc=0x7a2690u;break;} // d9461c fld dword ptr [esi + 0x1c]
case 0x7a2690u:{pushfp(fp((r[4]+32u)));pc=0x7a2693u;break;} // d94620 fld dword ptr [esi + 0x20]
case 0x7a2693u:{std::swap(st[0],st[1]);pc=0x7a2695u;break;} // d9c9 fxch st(1)
case 0x7a2695u:{st[0]=product(st[0],st[2]);pc=0x7a2697u;break;} // d8ca fmul st(2)
case 0x7a2697u:{fpwrite((r[7]+4u),st[0]);popfp();pc=0x7a269bu;break;} // d95c2404 fstp dword ptr [esp + 4]
case 0x7a269bu:{st[0]=product(st[0],st[1]);pc=0x7a269du;break;} // d8c9 fmul st(1)
case 0x7a269du:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a26a1u;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a26a1u:{st[0]=product(st[0],fp((r[4]+36u)));pc=0x7a26a4u;break;} // d84e24 fmul dword ptr [esi + 0x24]
case 0x7a26a4u:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a26a8u;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a26a8u:{r[7]-=4;put(r[7],0x7a26adu,4);pc=0x441bb0u;break;} // e803f5c9ff call 0x441bb0
case 0x7a26adu:{pushfp(fp((10243048u)));pc=0x7a26b3u;break;} // d905e84b9c00 fld dword ptr [0x9c4be8]
case 0x7a26b3u:{compare(st[0],st[1]);pc=0x7a26b5u;break;} // d8d1 fcom st(1)
case 0x7a26b5u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a26b7u;break;} // dfe0 fnstsw ax
case 0x7a26b7u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a26bau;break;} // f6c401 test ah, 1
case 0x7a26bau:{pc=(!zf)?0x7a26dcu:0x7a26bcu;break;} // 7520 jne 0x7a26dc
case 0x7a26bcu:{r[0]=get((r[7]+20u),4);pc=0x7a26c0u;break;} // 8b442414 mov eax, dword ptr [esp + 0x14]
case 0x7a26c0u:{st[1]=st[0];popfp();pc=0x7a26c2u;break;} // ddd9 fstp st(1)
case 0x7a26c2u:{st[0]=st[0];popfp();pc=0x7a26c4u;break;} // ddd8 fstp st(0)
case 0x7a26c4u:{pushfp(fp((r[4]+4u)));pc=0x7a26c7u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a26c7u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a26c9u;break;} // d918 fstp dword ptr [eax]
case 0x7a26c9u:{pushfp(fp((r[4]+8u)));pc=0x7a26ccu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a26ccu:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a26cfu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a26cfu:{pushfp(fp((r[4]+12u)));pc=0x7a26d2u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a26d2u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a26d3u;break;} // 5e pop esi
case 0x7a26d3u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a26d6u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a26d6u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x7a26d9u;break;} // 83c40c add esp, 0xc
case 0x7a26d9u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x7a26dcu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a26deu;break;} // dee9 fsubp st(1)
case 0x7a26deu:{pushfp(0.);pc=0x7a26e0u;break;} // d9ee fldz
case 0x7a26e0u:{compare(st[0],st[1]);pc=0x7a26e2u;break;} // d8d1 fcom st(1)
case 0x7a26e2u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a26e4u;break;} // dfe0 fnstsw ax
case 0x7a26e4u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a26e7u;break;} // f6c441 test ah, 0x41
case 0x7a26e7u:{pc=(!zf)?0x7a26edu:0x7a26e9u;break;} // 7504 jne 0x7a26ed
case 0x7a26e9u:{st[1]=st[0];popfp();pc=0x7a26ebu;break;} // ddd9 fstp st(1)
case 0x7a26ebu:{pc=(true)?0x7a26efu:0x7a26edu;break;} // eb02 jmp 0x7a26ef
case 0x7a26edu:{st[0]=st[0];popfp();pc=0x7a26efu;break;} // ddd8 fstp st(0)
case 0x7a26efu:{pushfp(fp((r[7]+4u)));pc=0x7a26f3u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a26f3u:{r[0]=get((r[7]+20u),4);pc=0x7a26f7u;break;} // 8b442414 mov eax, dword ptr [esp + 0x14]
case 0x7a26f7u:{st[0]=product(st[0],st[1]);pc=0x7a26f9u;break;} // d8c9 fmul st(1)
case 0x7a26f9u:{pushfp(fp((r[7]+8u)));pc=0x7a26fdu;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a26fdu:{st[0]=product(st[0],st[2]);pc=0x7a26ffu;break;} // d8ca fmul st(2)
case 0x7a26ffu:{pushfp(fp((r[7]+12u)));pc=0x7a2703u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x7a2703u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a2705u;break;} // decb fmulp st(3)
case 0x7a2705u:{std::swap(st[0],st[1]);pc=0x7a2707u;break;} // d9c9 fxch st(1)
case 0x7a2707u:{st[0]=sum(st[0],fp((r[4]+4u)));pc=0x7a270au;break;} // d84604 fadd dword ptr [esi + 4]
case 0x7a270au:{fpwrite((r[0]),st[0]);popfp();pc=0x7a270cu;break;} // d918 fstp dword ptr [eax]
case 0x7a270cu:{st[0]=sum(st[0],fp((r[4]+8u)));pc=0x7a270fu;break;} // d84608 fadd dword ptr [esi + 8]
case 0x7a270fu:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a2712u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a2712u:{st[0]=sum(st[0],fp((r[4]+12u)));pc=0x7a2715u;break;} // d8460c fadd dword ptr [esi + 0xc]
case 0x7a2715u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a2716u;break;} // 5e pop esi
case 0x7a2716u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a2719u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a2719u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x7a271cu;break;} // 83c40c add esp, 0xc
case 0x7a271cu:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x7a3550u:{r[0]=get((r[7]+4u),4);pc=0x7a3554u;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x7a3554u:{r[2]=get((r[7]+8u),4);pc=0x7a3558u;break;} // 8b4c2408 mov ecx, dword ptr [esp + 8]
case 0x7a3558u:{pushfp(fp((r[0])));pc=0x7a355au;break;} // d900 fld dword ptr [eax]
case 0x7a355au:{st[0]=difference(st[0],fp((r[2])));pc=0x7a355cu;break;} // d821 fsub dword ptr [ecx]
case 0x7a355cu:{pushfp(fp((r[0]+4u)));pc=0x7a355fu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a355fu:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x7a3562u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x7a3562u:{pushfp(fp((r[0]+8u)));pc=0x7a3565u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a3565u:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x7a3568u;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x7a3568u:{pushfp(fp((r[2]+12u)));pc=0x7a356bu;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x7a356bu:{st[0]=product(st[0],st[3]);pc=0x7a356du;break;} // d8cb fmul st(3)
case 0x7a356du:{pushfp(fp((r[2]+16u)));pc=0x7a3570u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x7a3570u:{st[0]=product(st[0],st[3]);pc=0x7a3572u;break;} // d8cb fmul st(3)
case 0x7a3572u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3574u;break;} // dec1 faddp st(1)
case 0x7a3574u:{pushfp(fp((r[2]+20u)));pc=0x7a3577u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x7a3577u:{st[0]=product(st[0],st[2]);pc=0x7a3579u;break;} // d8ca fmul st(2)
case 0x7a3579u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a357bu;break;} // dec1 faddp st(1)
case 0x7a357bu:{pushfp(0.);pc=0x7a357du;break;} // d9ee fldz
case 0x7a357du:{compare(st[0],st[1]);pc=0x7a357fu;break;} // d8d1 fcom st(1)
case 0x7a357fu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a3581u;break;} // dfe0 fnstsw ax
case 0x7a3581u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a3584u;break;} // f6c401 test ah, 1
case 0x7a3584u:{pc=(!zf)?0x7a358au:0x7a3586u;break;} // 7504 jne 0x7a358a
case 0x7a3586u:{st[1]=st[0];popfp();pc=0x7a3588u;break;} // ddd9 fstp st(1)
case 0x7a3588u:{pc=(true)?0x7a35fcu:0x7a358au;break;} // eb72 jmp 0x7a35fc
case 0x7a358au:{st[0]=st[0];popfp();pc=0x7a358cu;break;} // ddd8 fstp st(0)
case 0x7a358cu:{pushfp(fp((r[2]+16u)));pc=0x7a358fu;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x7a358fu:{pushfp(fp((r[2]+12u)));pc=0x7a3592u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x7a3592u:{pushfp(fp((r[2]+20u)));pc=0x7a3595u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x7a3595u:{pushfp(st[2]);pc=0x7a3597u;break;} // d9c2 fld st(2)
case 0x7a3597u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a3599u;break;} // decb fmulp st(3)
case 0x7a3599u:{pushfp(st[1]);pc=0x7a359bu;break;} // d9c1 fld st(1)
case 0x7a359bu:{st[2]=product(st[2],st[0]);popfp();pc=0x7a359du;break;} // deca fmulp st(2)
case 0x7a359du:{std::swap(st[0],st[2]);pc=0x7a359fu;break;} // d9ca fxch st(2)
case 0x7a359fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a35a1u;break;} // dec1 faddp st(1)
case 0x7a35a1u:{pushfp(st[1]);pc=0x7a35a3u;break;} // d9c1 fld st(1)
case 0x7a35a3u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a35a5u;break;} // deca fmulp st(2)
case 0x7a35a5u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a35a7u;break;} // dec1 faddp st(1)
case 0x7a35a7u:{compare(st[0],st[1]);pc=0x7a35a9u;break;} // d8d1 fcom st(1)
case 0x7a35a9u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a35abu;break;} // dfe0 fnstsw ax
case 0x7a35abu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a35aeu;break;} // f6c441 test ah, 0x41
case 0x7a35aeu:{pc=(pf)?0x7a35d1u:0x7a35b0u;break;} // 7a21 jp 0x7a35d1
case 0x7a35b0u:{st[1]=st[0];popfp();pc=0x7a35b2u;break;} // ddd9 fstp st(1)
case 0x7a35b2u:{st[0]=st[0];popfp();pc=0x7a35b4u;break;} // ddd8 fstp st(0)
case 0x7a35b4u:{pushfp(1.);pc=0x7a35b6u;break;} // d9e8 fld1
case 0x7a35b6u:{std::swap(st[0],st[3]);pc=0x7a35b8u;break;} // d9cb fxch st(3)
case 0x7a35b8u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x7a35bbu;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x7a35bbu:{std::swap(st[0],st[2]);pc=0x7a35bdu;break;} // d9ca fxch st(2)
case 0x7a35bdu:{st[0]=difference(st[0],fp((r[2]+16u)));pc=0x7a35c0u;break;} // d86110 fsub dword ptr [ecx + 0x10]
case 0x7a35c0u:{std::swap(st[0],st[1]);pc=0x7a35c2u;break;} // d9c9 fxch st(1)
case 0x7a35c2u:{st[0]=difference(st[0],fp((r[2]+20u)));pc=0x7a35c5u;break;} // d86114 fsub dword ptr [ecx + 0x14]
case 0x7a35c5u:{std::swap(st[0],st[2]);pc=0x7a35c7u;break;} // d9ca fxch st(2)
case 0x7a35c7u:{std::swap(st[0],st[3]);pc=0x7a35c9u;break;} // d9cb fxch st(3)
case 0x7a35c9u:{std::swap(st[0],st[1]);pc=0x7a35cbu;break;} // d9c9 fxch st(1)
case 0x7a35cbu:{std::swap(st[0],st[2]);pc=0x7a35cdu;break;} // d9ca fxch st(2)
case 0x7a35cdu:{std::swap(st[0],st[1]);pc=0x7a35cfu;break;} // d9c9 fxch st(1)
case 0x7a35cfu:{pc=(true)?0x7a35fcu:0x7a35d1u;break;} // eb2b jmp 0x7a35fc
case 0x7a35d1u:{st[1]=quotient(st[1],st[0]);popfp();pc=0x7a35d3u;break;} // def9 fdivp st(1)
case 0x7a35d3u:{pushfp(fp((r[2]+12u)));pc=0x7a35d6u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x7a35d6u:{pushfp(fp((r[2]+16u)));pc=0x7a35d9u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x7a35d9u:{pushfp(fp((r[2]+20u)));pc=0x7a35dcu;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x7a35dcu:{std::swap(st[0],st[2]);pc=0x7a35deu;break;} // d9ca fxch st(2)
case 0x7a35deu:{st[0]=product(st[0],st[3]);pc=0x7a35e0u;break;} // d8cb fmul st(3)
case 0x7a35e0u:{std::swap(st[0],st[1]);pc=0x7a35e2u;break;} // d9c9 fxch st(1)
case 0x7a35e2u:{st[0]=product(st[0],st[3]);pc=0x7a35e4u;break;} // d8cb fmul st(3)
case 0x7a35e4u:{std::swap(st[0],st[2]);pc=0x7a35e6u;break;} // d9ca fxch st(2)
case 0x7a35e6u:{st[0]=product(st[0],st[3]);pc=0x7a35e8u;break;} // d8cb fmul st(3)
case 0x7a35e8u:{std::swap(st[0],st[6]);pc=0x7a35eau;break;} // d9ce fxch st(6)
case 0x7a35eau:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a35ecu;break;} // dee1 fsubrp st(1)
case 0x7a35ecu:{std::swap(st[0],st[4]);pc=0x7a35eeu;break;} // d9cc fxch st(4)
case 0x7a35eeu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a35f0u;break;} // dee1 fsubrp st(1)
case 0x7a35f0u:{std::swap(st[0],st[2]);pc=0x7a35f2u;break;} // d9ca fxch st(2)
case 0x7a35f2u:{st[4]=difference(st[0],st[4]);popfp();pc=0x7a35f4u;break;} // dee4 fsubrp st(4)
case 0x7a35f4u:{std::swap(st[0],st[2]);pc=0x7a35f6u;break;} // d9ca fxch st(2)
case 0x7a35f6u:{std::swap(st[0],st[3]);pc=0x7a35f8u;break;} // d9cb fxch st(3)
case 0x7a35f8u:{std::swap(st[0],st[1]);pc=0x7a35fau;break;} // d9c9 fxch st(1)
case 0x7a35fau:{std::swap(st[0],st[2]);pc=0x7a35fcu;break;} // d9ca fxch st(2)
case 0x7a35fcu:{r[0]=get((r[7]+12u),4);pc=0x7a3600u;break;} // 8b44240c mov eax, dword ptr [esp + 0xc]
case 0x7a3600u:{flags((r[0])&(r[0]));cf=false;pc=0x7a3602u;break;} // 85c0 test eax, eax
case 0x7a3602u:{pc=(zf)?0x7a3617u:0x7a3604u;break;} // 7413 je 0x7a3617
case 0x7a3604u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a3606u;break;} // d918 fstp dword ptr [eax]
case 0x7a3606u:{pushfp(st[1]);pc=0x7a3608u;break;} // d9c1 fld st(1)
case 0x7a3608u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a360au;break;} // deca fmulp st(2)
case 0x7a360au:{pushfp(st[2]);pc=0x7a360cu;break;} // d9c2 fld st(2)
case 0x7a360cu:{st[3]=product(st[3],st[0]);popfp();pc=0x7a360eu;break;} // decb fmulp st(3)
case 0x7a360eu:{std::swap(st[0],st[1]);pc=0x7a3610u;break;} // d9c9 fxch st(1)
case 0x7a3610u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a3612u;break;} // dec2 faddp st(2)
case 0x7a3612u:{st[0]=product(st[0],st[0]);pc=0x7a3614u;break;} // dcc8 fmul st(0), st(0)
case 0x7a3614u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3616u;break;} // dec1 faddp st(1)
case 0x7a3616u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a3617u:{st[0]=st[0];popfp();pc=0x7a3619u;break;} // ddd8 fstp st(0)
case 0x7a3619u:{pushfp(st[1]);pc=0x7a361bu;break;} // d9c1 fld st(1)
case 0x7a361bu:{st[2]=product(st[2],st[0]);popfp();pc=0x7a361du;break;} // deca fmulp st(2)
case 0x7a361du:{pushfp(st[2]);pc=0x7a361fu;break;} // d9c2 fld st(2)
case 0x7a361fu:{st[3]=product(st[3],st[0]);popfp();pc=0x7a3621u;break;} // decb fmulp st(3)
case 0x7a3621u:{std::swap(st[0],st[1]);pc=0x7a3623u;break;} // d9c9 fxch st(1)
case 0x7a3623u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a3625u;break;} // dec2 faddp st(2)
case 0x7a3625u:{st[0]=product(st[0],st[0]);pc=0x7a3627u;break;} // dcc8 fmul st(0), st(0)
case 0x7a3627u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a3629u;break;} // dec1 faddp st(1)
case 0x7a3629u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a63c0u:{uint32_t v=(r[7])-(24u);r[7]=v;flags(v);pc=0x7a63c3u;break;} // 83ec18 sub esp, 0x18
case 0x7a63c3u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a63c4u;break;} // 55 push ebp
case 0x7a63c4u:{r[6]=get((r[7]+32u),4);pc=0x7a63c8u;break;} // 8b6c2420 mov ebp, dword ptr [esp + 0x20]
case 0x7a63c8u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a63c9u;break;} // 57 push edi
case 0x7a63c9u:{r[5]=r[0];pc=0x7a63cbu;break;} // 8bf8 mov edi, eax
case 0x7a63cbu:{r[0]=(r[5]+r[5]*2u);pc=0x7a63ceu;break;} // 8d047f lea eax, [edi + edi*2]
case 0x7a63ceu:{pushfp(fp((r[6]+r[0]*4u)));pc=0x7a63d2u;break;} // d9448500 fld dword ptr [ebp + eax*4]
case 0x7a63d2u:{r[2]=(r[6]+r[0]*4u);pc=0x7a63d6u;break;} // 8d4c8500 lea ecx, [ebp + eax*4]
case 0x7a63d6u:{st[0]=difference(st[0],fp((r[4]+4u)));pc=0x7a63d9u;break;} // d86604 fsub dword ptr [esi + 4]
case 0x7a63d9u:{pushfp(fp((r[2]+4u)));pc=0x7a63dcu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a63dcu:{st[0]=difference(st[0],fp((r[4]+8u)));pc=0x7a63dfu;break;} // d86608 fsub dword ptr [esi + 8]
case 0x7a63dfu:{pushfp(fp((r[2]+8u)));pc=0x7a63e2u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a63e2u:{st[0]=difference(st[0],fp((r[4]+12u)));pc=0x7a63e5u;break;} // d8660c fsub dword ptr [esi + 0xc]
case 0x7a63e5u:{pushfp(fp((r[4])));pc=0x7a63e7u;break;} // d906 fld dword ptr [esi]
case 0x7a63e7u:{pushfp(st[2]);pc=0x7a63e9u;break;} // d9c2 fld st(2)
case 0x7a63e9u:{st[0]=product(st[0],st[3]);pc=0x7a63ebu;break;} // d8cb fmul st(3)
case 0x7a63ebu:{pushfp(st[4]);pc=0x7a63edu;break;} // d9c4 fld st(4)
case 0x7a63edu:{st[0]=product(st[0],st[5]);pc=0x7a63efu;break;} // d8cd fmul st(5)
case 0x7a63efu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a63f1u;break;} // dec1 faddp st(1)
case 0x7a63f1u:{pushfp(st[2]);pc=0x7a63f3u;break;} // d9c2 fld st(2)
case 0x7a63f3u:{st[0]=product(st[0],st[3]);pc=0x7a63f5u;break;} // d8cb fmul st(3)
case 0x7a63f5u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a63f7u;break;} // dec1 faddp st(1)
case 0x7a63f7u:{pushfp(st[1]);pc=0x7a63f9u;break;} // d9c1 fld st(1)
case 0x7a63f9u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a63fbu;break;} // deca fmulp st(2)
case 0x7a63fbu:{compare(st[0],st[1]);popfp();popfp();pc=0x7a63fdu;break;} // ded9 fcompp
case 0x7a63fdu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a63ffu;break;} // dfe0 fnstsw ax
case 0x7a63ffu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a6402u;break;} // f6c405 test ah, 5
case 0x7a6402u:{pc=(pf)?0x7a641au:0x7a6404u;break;} // 7a16 jp 0x7a641a
case 0x7a6404u:{r[2]=get((r[7]+48u),4);pc=0x7a6408u;break;} // 8b4c2430 mov ecx, dword ptr [esp + 0x30]
case 0x7a6408u:{st[2]=st[0];popfp();pc=0x7a640au;break;} // ddda fstp st(2)
case 0x7a640au:{st[0]=st[0];popfp();pc=0x7a640cu;break;} // ddd8 fstp st(0)
case 0x7a640cu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a640du;break;} // 5f pop edi
case 0x7a640du:{st[0]=st[0];popfp();pc=0x7a640fu;break;} // ddd8 fstp st(0)
case 0x7a640fu:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6411u;break;} // 32c0 xor al, al
case 0x7a6411u:{pushfp(0.);pc=0x7a6413u;break;} // d9ee fldz
case 0x7a6413u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a6414u;break;} // 5d pop ebp
case 0x7a6414u:{fpwrite((r[2]),st[0]);popfp();pc=0x7a6416u;break;} // d919 fstp dword ptr [ecx]
case 0x7a6416u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a6419u;break;} // 83c418 add esp, 0x18
case 0x7a6419u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a641au:{r[0]=get((r[7]+44u),4);pc=0x7a641eu;break;} // 8b44242c mov eax, dword ptr [esp + 0x2c]
case 0x7a641eu:{pushfp(fp((r[0])));pc=0x7a6420u;break;} // d900 fld dword ptr [eax]
case 0x7a6420u:{r[0]=get((r[7]+40u),4);pc=0x7a6424u;break;} // 8b442428 mov eax, dword ptr [esp + 0x28]
case 0x7a6424u:{st[0]=difference(st[0],fp((r[0])));pc=0x7a6426u;break;} // d820 fsub dword ptr [eax]
case 0x7a6426u:{r[0]=get((r[7]+44u),4);pc=0x7a642au;break;} // 8b44242c mov eax, dword ptr [esp + 0x2c]
case 0x7a642au:{pushfp(fp((r[0]+4u)));pc=0x7a642du;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a642du:{r[0]=get((r[7]+40u),4);pc=0x7a6431u;break;} // 8b442428 mov eax, dword ptr [esp + 0x28]
case 0x7a6431u:{st[0]=difference(st[0],fp((r[0]+4u)));pc=0x7a6434u;break;} // d86004 fsub dword ptr [eax + 4]
case 0x7a6434u:{r[0]=get((r[7]+44u),4);pc=0x7a6438u;break;} // 8b44242c mov eax, dword ptr [esp + 0x2c]
case 0x7a6438u:{pushfp(fp((r[0]+8u)));pc=0x7a643bu;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a643bu:{r[0]=get((r[7]+40u),4);pc=0x7a643fu;break;} // 8b442428 mov eax, dword ptr [esp + 0x28]
case 0x7a643fu:{st[0]=difference(st[0],fp((r[0]+8u)));pc=0x7a6442u;break;} // d86008 fsub dword ptr [eax + 8]
case 0x7a6442u:{pushfp(st[1]);pc=0x7a6444u;break;} // d9c1 fld st(1)
case 0x7a6444u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a6446u;break;} // decd fmulp st(5)
case 0x7a6446u:{pushfp(st[2]);pc=0x7a6448u;break;} // d9c2 fld st(2)
case 0x7a6448u:{st[6]=product(st[6],st[0]);popfp();pc=0x7a644au;break;} // dece fmulp st(6)
case 0x7a644au:{std::swap(st[0],st[4]);pc=0x7a644cu;break;} // d9cc fxch st(4)
case 0x7a644cu:{st[5]=sum(st[5],st[0]);popfp();pc=0x7a644eu;break;} // dec5 faddp st(5)
case 0x7a644eu:{pushfp(st[3]);pc=0x7a6450u;break;} // d9c3 fld st(3)
case 0x7a6450u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6452u;break;} // decb fmulp st(3)
case 0x7a6452u:{std::swap(st[0],st[4]);pc=0x7a6454u;break;} // d9cc fxch st(4)
case 0x7a6454u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6456u;break;} // dec2 faddp st(2)
case 0x7a6456u:{pushfp(0.);pc=0x7a6458u;break;} // d9ee fldz
case 0x7a6458u:{compare(st[0],st[2]);pc=0x7a645au;break;} // d8d2 fcom st(2)
case 0x7a645au:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a645cu;break;} // dfe0 fnstsw ax
case 0x7a645cu:{st[2]=st[0];popfp();pc=0x7a645eu;break;} // ddda fstp st(2)
case 0x7a645eu:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a6461u;break;} // f6c401 test ah, 1
case 0x7a6461u:{pc=(zf)?0x7a6495u:0x7a6463u;break;} // 7432 je 0x7a6495
case 0x7a6463u:{pushfp(fp((r[3]+4u)));pc=0x7a6466u;break;} // d94204 fld dword ptr [edx + 4]
case 0x7a6466u:{st[0]=product(st[0],st[4]);pc=0x7a6468u;break;} // d8cc fmul st(4)
case 0x7a6468u:{pushfp(st[1]);pc=0x7a646au;break;} // d9c1 fld st(1)
case 0x7a646au:{st[0]=product(st[0],fp((r[3])));pc=0x7a646cu;break;} // d80a fmul dword ptr [edx]
case 0x7a646cu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a646eu;break;} // dec1 faddp st(1)
case 0x7a646eu:{pushfp(st[3]);pc=0x7a6470u;break;} // d9c3 fld st(3)
case 0x7a6470u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x7a6473u;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x7a6473u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6475u;break;} // dec1 faddp st(1)
case 0x7a6475u:{pushfp(fp((r[4]+4u)));pc=0x7a6478u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6478u:{pushfp(fp((r[4]+8u)));pc=0x7a647bu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a647bu:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a647fu;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a647fu:{pushfp(fp((r[4]+12u)));pc=0x7a6482u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6482u:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x7a6486u;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x7a6486u:{pushfp(st[1]);pc=0x7a6488u;break;} // d9c1 fld st(1)
case 0x7a6488u:{compare(st[0],st[4]);popfp();pc=0x7a648au;break;} // ddec fucomp st(4)
case 0x7a648au:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a648cu;break;} // dfe0 fnstsw ax
case 0x7a648cu:{st[3]=st[0];popfp();pc=0x7a648eu;break;} // dddb fstp st(3)
case 0x7a648eu:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x7a6491u;break;} // f6c444 test ah, 0x44
case 0x7a6491u:{pc=(pf)?0x7a64a5u:0x7a6493u;break;} // 7a12 jp 0x7a64a5
case 0x7a6493u:{st[0]=st[0];popfp();pc=0x7a6495u;break;} // ddd8 fstp st(0)
case 0x7a6495u:{st[0]=st[0];popfp();pc=0x7a6497u;break;} // ddd8 fstp st(0)
case 0x7a6497u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6498u;break;} // 5f pop edi
case 0x7a6498u:{st[2]=st[0];popfp();pc=0x7a649au;break;} // ddda fstp st(2)
case 0x7a649au:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a649cu;break;} // 32c0 xor al, al
case 0x7a649cu:{st[0]=st[0];popfp();pc=0x7a649eu;break;} // ddd8 fstp st(0)
case 0x7a649eu:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a649fu;break;} // 5d pop ebp
case 0x7a649fu:{st[0]=st[0];popfp();pc=0x7a64a1u;break;} // ddd8 fstp st(0)
case 0x7a64a1u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a64a4u;break;} // 83c418 add esp, 0x18
case 0x7a64a4u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a64a5u:{pushfp(fp((r[2])));pc=0x7a64a7u;break;} // d901 fld dword ptr [ecx]
case 0x7a64a7u:{st[0]=product(st[0],fp((r[3])));pc=0x7a64a9u;break;} // d80a fmul dword ptr [edx]
case 0x7a64a9u:{pushfp(fp((r[2]+4u)));pc=0x7a64acu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a64acu:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x7a64afu;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x7a64afu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64b1u;break;} // dec1 faddp st(1)
case 0x7a64b1u:{pushfp(fp((r[2]+8u)));pc=0x7a64b4u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a64b4u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x7a64b7u;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x7a64b7u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64b9u;break;} // dec1 faddp st(1)
case 0x7a64b9u:{pushfp(fp((r[4]+8u)));pc=0x7a64bcu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a64bcu:{st[0]=product(st[0],fp((r[3]+4u)));pc=0x7a64bfu;break;} // d84a04 fmul dword ptr [edx + 4]
case 0x7a64bfu:{pushfp(fp((r[3])));pc=0x7a64c1u;break;} // d902 fld dword ptr [edx]
case 0x7a64c1u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7a64c4u;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7a64c4u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64c6u;break;} // dec1 faddp st(1)
case 0x7a64c6u:{pushfp(fp((r[4]+12u)));pc=0x7a64c9u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a64c9u:{st[0]=product(st[0],fp((r[3]+8u)));pc=0x7a64ccu;break;} // d84a08 fmul dword ptr [edx + 8]
case 0x7a64ccu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64ceu;break;} // dec1 faddp st(1)
case 0x7a64ceu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a64d0u;break;} // dee9 fsubp st(1)
case 0x7a64d0u:{st[1]=quotient(st[0],st[1]);popfp();pc=0x7a64d2u;break;} // def1 fdivrp st(1)
case 0x7a64d2u:{st[1]=product(st[1],st[0]);pc=0x7a64d4u;break;} // dcc9 fmul st(1), st(0)
case 0x7a64d4u:{st[4]=product(st[4],st[0]);pc=0x7a64d6u;break;} // dccc fmul st(4), st(0)
case 0x7a64d6u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a64d8u;break;} // decb fmulp st(3)
case 0x7a64d8u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64dau;break;} // dec1 faddp st(1)
case 0x7a64dau:{std::swap(st[0],st[2]);pc=0x7a64dcu;break;} // d9ca fxch st(2)
case 0x7a64dcu:{st[0]=sum(st[0],fp((r[7]+12u)));pc=0x7a64e0u;break;} // d844240c fadd dword ptr [esp + 0xc]
case 0x7a64e0u:{std::swap(st[0],st[1]);pc=0x7a64e2u;break;} // d9c9 fxch st(1)
case 0x7a64e2u:{st[0]=sum(st[0],fp((r[7]+16u)));pc=0x7a64e6u;break;} // d8442410 fadd dword ptr [esp + 0x10]
case 0x7a64e6u:{pushfp(fp((r[1]+4u)));pc=0x7a64e9u;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a64e9u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a64ebu;break;} // deca fmulp st(2)
case 0x7a64ebu:{std::swap(st[0],st[2]);pc=0x7a64edu;break;} // d9ca fxch st(2)
case 0x7a64edu:{st[0]=product(st[0],fp((r[1])));pc=0x7a64efu;break;} // d80b fmul dword ptr [ebx]
case 0x7a64efu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64f1u;break;} // dec1 faddp st(1)
case 0x7a64f1u:{pushfp(fp((r[1]+8u)));pc=0x7a64f4u;break;} // d94308 fld dword ptr [ebx + 8]
case 0x7a64f4u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a64f6u;break;} // deca fmulp st(2)
case 0x7a64f6u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a64f8u;break;} // dec1 faddp st(1)
case 0x7a64f8u:{pushfp(fp((r[2])));pc=0x7a64fau;break;} // d901 fld dword ptr [ecx]
case 0x7a64fau:{st[0]=product(st[0],fp((r[1])));pc=0x7a64fcu;break;} // d80b fmul dword ptr [ebx]
case 0x7a64fcu:{pushfp(fp((r[2]+4u)));pc=0x7a64ffu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a64ffu:{st[0]=product(st[0],fp((r[1]+4u)));pc=0x7a6502u;break;} // d84b04 fmul dword ptr [ebx + 4]
case 0x7a6502u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6504u;break;} // dec1 faddp st(1)
case 0x7a6504u:{pushfp(fp((r[2]+8u)));pc=0x7a6507u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6507u:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a650au;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a650au:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a650cu;break;} // dec1 faddp st(1)
case 0x7a650cu:{compare(st[0],st[1]);popfp();popfp();pc=0x7a650eu;break;} // ded9 fcompp
case 0x7a650eu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6510u;break;} // dfe0 fnstsw ax
case 0x7a6510u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6513u;break;} // f6c441 test ah, 0x41
case 0x7a6513u:{pc=(pf)?0x7a654fu:0x7a6515u;break;} // 7a3a jp 0x7a654f
case 0x7a6515u:{r[0]=(r[5]+1u);pc=0x7a6518u;break;} // 8d4701 lea eax, [edi + 1]
case 0x7a6518u:{pushfp(fp((r[2])));pc=0x7a651au;break;} // d901 fld dword ptr [ecx]
case 0x7a651au:{r[3]=(r[0]&0x80000000u)?0xffffffffu:0u;pc=0x7a651bu;break;} // 99 cdq
case 0x7a651bu:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a651fu;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a651fu:{r[5]=3u;pc=0x7a6524u;break;} // bf03000000 mov edi, 3
case 0x7a6524u:{pushfp(fp((r[2]+4u)));pc=0x7a6527u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6527u:{int64_t numerator=int64_t(int32_t(r[3]))*4294967296LL+r[0];int64_t denominator=int32_t(r[5]);if(!denominator)throw Fault{};int64_t q=numerator/denominator;if(q<INT32_MIN||q>INT32_MAX)throw Fault{};r[0]=uint32_t(q);r[3]=uint32_t(numerator%denominator);pc=0x7a6529u;break;} // f7ff idiv edi
case 0x7a6529u:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a652du;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a652du:{pushfp(fp((r[2]+8u)));pc=0x7a6530u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6530u:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x7a6534u;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x7a6534u:{r[3]=(r[3]+r[3]*2u);pc=0x7a6537u;break;} // 8d1452 lea edx, [edx + edx*2]
case 0x7a6537u:{pushfp(fp((r[6]+r[3]*4u)));pc=0x7a653bu;break;} // d9449500 fld dword ptr [ebp + edx*4]
case 0x7a653bu:{r[0]=(r[6]+r[3]*4u);pc=0x7a653fu;break;} // 8d449500 lea eax, [ebp + edx*4]
case 0x7a653fu:{st[0]=difference(st[0],fp((r[2])));pc=0x7a6541u;break;} // d821 fsub dword ptr [ecx]
case 0x7a6541u:{pushfp(fp((r[0]+4u)));pc=0x7a6544u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a6544u:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x7a6547u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x7a6547u:{pushfp(fp((r[0]+8u)));pc=0x7a654au;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a654au:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x7a654du;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x7a654du:{pc=(true)?0x7a6587u:0x7a654fu;break;} // eb38 jmp 0x7a6587
case 0x7a654fu:{flags((r[5])&(r[5]));cf=false;pc=0x7a6551u;break;} // 85ff test edi, edi
case 0x7a6551u:{pc=(zf)?0x7a6566u:0x7a6553u;break;} // 7413 je 0x7a6566
case 0x7a6553u:{pushfp(fp((r[2]+4294967284u)));pc=0x7a6556u;break;} // d941f4 fld dword ptr [ecx - 0xc]
case 0x7a6556u:{fpwrite((r[7]+8u),st[0]);pc=0x7a655au;break;} // d9542408 fst dword ptr [esp + 8]
case 0x7a655au:{pushfp(fp((r[2]+4294967288u)));pc=0x7a655du;break;} // d941f8 fld dword ptr [ecx - 8]
case 0x7a655du:{fpwrite((r[7]+12u),st[0]);pc=0x7a6561u;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x7a6561u:{pushfp(fp((r[2]+4294967292u)));pc=0x7a6564u;break;} // d941fc fld dword ptr [ecx - 4]
case 0x7a6564u:{pc=(true)?0x7a6577u:0x7a6566u;break;} // eb11 jmp 0x7a6577
case 0x7a6566u:{pushfp(fp((r[6]+24u)));pc=0x7a6569u;break;} // d94518 fld dword ptr [ebp + 0x18]
case 0x7a6569u:{fpwrite((r[7]+8u),st[0]);pc=0x7a656du;break;} // d9542408 fst dword ptr [esp + 8]
case 0x7a656du:{pushfp(fp((r[6]+28u)));pc=0x7a6570u;break;} // d9451c fld dword ptr [ebp + 0x1c]
case 0x7a6570u:{fpwrite((r[7]+12u),st[0]);pc=0x7a6574u;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x7a6574u:{pushfp(fp((r[6]+32u)));pc=0x7a6577u;break;} // d94520 fld dword ptr [ebp + 0x20]
case 0x7a6577u:{fpwrite((r[7]+16u),st[0]);pc=0x7a657bu;break;} // d9542410 fst dword ptr [esp + 0x10]
case 0x7a657bu:{pushfp(fp((r[2])));pc=0x7a657du;break;} // d901 fld dword ptr [ecx]
case 0x7a657du:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a657fu;break;} // dee3 fsubrp st(3)
case 0x7a657fu:{pushfp(fp((r[2]+4u)));pc=0x7a6582u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6582u:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a6584u;break;} // dee2 fsubrp st(2)
case 0x7a6584u:{st[0]=difference(fp((r[2]+8u)),st[0]);pc=0x7a6587u;break;} // d86908 fsubr dword ptr [ecx + 8]
case 0x7a6587u:{r[0]=get((r[7]+60u),4);pc=0x7a658bu;break;} // 8b44243c mov eax, dword ptr [esp + 0x3c]
case 0x7a658bu:{std::swap(st[0],st[2]);pc=0x7a658du;break;} // d9ca fxch st(2)
case 0x7a658du:{r[2]=get((r[7]+56u),4);pc=0x7a6591u;break;} // 8b4c2438 mov ecx, dword ptr [esp + 0x38]
case 0x7a6591u:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x7a6595u;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x7a6595u:{r[3]=get((r[7]+48u),4);pc=0x7a6599u;break;} // 8b542430 mov edx, dword ptr [esp + 0x30]
case 0x7a6599u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a659au;break;} // 50 push eax
case 0x7a659au:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a659eu;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a659eu:{r[0]=get((r[7]+48u),4);pc=0x7a65a2u;break;} // 8b442430 mov eax, dword ptr [esp + 0x30]
case 0x7a65a2u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a65a3u;break;} // 51 push ecx
case 0x7a65a3u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a65a7u;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a65a7u:{pushfp(fp((r[7]+60u)));pc=0x7a65abu;break;} // d944243c fld dword ptr [esp + 0x3c]
case 0x7a65abu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a65acu;break;} // 51 push ecx
case 0x7a65acu:{r[2]=get((r[7]+52u),4);pc=0x7a65b0u;break;} // 8b4c2434 mov ecx, dword ptr [esp + 0x34]
case 0x7a65b0u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a65b3u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a65b3u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a65b4u;break;} // 52 push edx
case 0x7a65b4u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a65b5u;break;} // 50 push eax
case 0x7a65b5u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a65b6u;break;} // 56 push esi
case 0x7a65b6u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a65b7u;break;} // 51 push ecx
case 0x7a65b7u:{r[3]=(r[7]+36u);pc=0x7a65bbu;break;} // 8d542424 lea edx, [esp + 0x24]
case 0x7a65bbu:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a65bcu;break;} // 52 push edx
case 0x7a65bcu:{r[7]-=4;put(r[7],0x7a65c1u,4);pc=0x7a9220u;break;} // e85f2c0000 call 0x7a9220
case 0x7a65c1u:{uint32_t v=(r[7])+(32u);r[7]=v;flags(v);pc=0x7a65c4u;break;} // 83c420 add esp, 0x20
case 0x7a65c4u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a65c5u;break;} // 5f pop edi
case 0x7a65c5u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a65c6u;break;} // 5d pop ebp
case 0x7a65c6u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a65c9u;break;} // 83c418 add esp, 0x18
case 0x7a65c9u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a65d0u:{uint32_t v=(r[7])-(24u);r[7]=v;flags(v);pc=0x7a65d3u;break;} // 83ec18 sub esp, 0x18
case 0x7a65d3u:{pushfp(fp((r[5])));pc=0x7a65d5u;break;} // d907 fld dword ptr [edi]
case 0x7a65d5u:{r[0]=get((r[7]+32u),4);pc=0x7a65d9u;break;} // 8b442420 mov eax, dword ptr [esp + 0x20]
case 0x7a65d9u:{st[0]=difference(st[0],fp((r[3])));pc=0x7a65dbu;break;} // d822 fsub dword ptr [edx]
case 0x7a65dbu:{r[2]=get((r[7]+28u),4);pc=0x7a65dfu;break;} // 8b4c241c mov ecx, dword ptr [esp + 0x1c]
case 0x7a65dfu:{pushfp(fp((r[5]+4u)));pc=0x7a65e2u;break;} // d94704 fld dword ptr [edi + 4]
case 0x7a65e2u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a65e3u;break;} // 55 push ebp
case 0x7a65e3u:{st[0]=difference(st[0],fp((r[3]+4u)));pc=0x7a65e6u;break;} // d86204 fsub dword ptr [edx + 4]
case 0x7a65e6u:{r[6]=get((r[7]+44u),4);pc=0x7a65eau;break;} // 8b6c242c mov ebp, dword ptr [esp + 0x2c]
case 0x7a65eau:{pushfp(fp((r[5]+8u)));pc=0x7a65edu;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a65edu:{st[0]=difference(st[0],fp((r[3]+8u)));pc=0x7a65f0u;break;} // d86208 fsub dword ptr [edx + 8]
case 0x7a65f0u:{fpwrite((r[7]+24u),st[0]);pc=0x7a65f4u;break;} // d9542418 fst dword ptr [esp + 0x18]
case 0x7a65f4u:{pushfp(fp((r[0]+4u)));pc=0x7a65f7u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a65f7u:{st[0]=difference(st[0],fp((r[2])));pc=0x7a65f9u;break;} // d821 fsub dword ptr [ecx]
case 0x7a65f9u:{pushfp(fp((r[0]+8u)));pc=0x7a65fcu;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a65fcu:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x7a65ffu;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x7a65ffu:{pushfp(fp((r[0]+12u)));pc=0x7a6602u;break;} // d9400c fld dword ptr [eax + 0xc]
case 0x7a6602u:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x7a6605u;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x7a6605u:{pushfp(st[3]);pc=0x7a6607u;break;} // d9c3 fld st(3)
case 0x7a6607u:{st[0]=product(st[0],st[2]);pc=0x7a6609u;break;} // d8ca fmul st(2)
case 0x7a6609u:{pushfp(st[5]);pc=0x7a660bu;break;} // d9c5 fld st(5)
case 0x7a660bu:{st[0]=product(st[0],st[2]);pc=0x7a660du;break;} // d8ca fmul st(2)
case 0x7a660du:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a660fu;break;} // dee9 fsubp st(1)
case 0x7a660fu:{fpwrite((r[7]+4u),st[0]);popfp();pc=0x7a6613u;break;} // d95c2404 fstp dword ptr [esp + 4]
case 0x7a6613u:{pushfp(st[5]);pc=0x7a6615u;break;} // d9c5 fld st(5)
case 0x7a6615u:{st[0]=product(st[0],st[1]);pc=0x7a6617u;break;} // d8c9 fmul st(1)
case 0x7a6617u:{pushfp(st[4]);pc=0x7a6619u;break;} // d9c4 fld st(4)
case 0x7a6619u:{st[0]=product(st[0],st[4]);pc=0x7a661bu;break;} // d8cc fmul st(4)
case 0x7a661bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a661du;break;} // dee9 fsubp st(1)
case 0x7a661du:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a6621u;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a6621u:{pushfp(st[4]);pc=0x7a6623u;break;} // d9c4 fld st(4)
case 0x7a6623u:{st[0]=product(st[0],st[3]);pc=0x7a6625u;break;} // d8cb fmul st(3)
case 0x7a6625u:{pushfp(st[2]);pc=0x7a6627u;break;} // d9c2 fld st(2)
case 0x7a6627u:{st[0]=product(st[0],st[7]);pc=0x7a6629u;break;} // d8cf fmul st(7)
case 0x7a6629u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a662bu;break;} // dee9 fsubp st(1)
case 0x7a662bu:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a662fu;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a662fu:{pushfp(fp((r[0])));pc=0x7a6631u;break;} // d900 fld dword ptr [eax]
case 0x7a6631u:{st[0]=product(st[0],st[0]);pc=0x7a6633u;break;} // dcc8 fmul st(0), st(0)
case 0x7a6633u:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a6637u;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a6637u:{pushfp(st[4]);pc=0x7a6639u;break;} // d9c4 fld st(4)
case 0x7a6639u:{st[0]=product(st[0],st[5]);pc=0x7a663bu;break;} // d8cd fmul st(5)
case 0x7a663bu:{pushfp(st[6]);pc=0x7a663du;break;} // d9c6 fld st(6)
case 0x7a663du:{st[0]=product(st[0],st[7]);pc=0x7a663fu;break;} // d8cf fmul st(7)
case 0x7a663fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6641u;break;} // dec1 faddp st(1)
case 0x7a6641u:{pushfp(st[4]);pc=0x7a6643u;break;} // d9c4 fld st(4)
case 0x7a6643u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a6645u;break;} // decd fmulp st(5)
case 0x7a6645u:{st[4]=sum(st[4],st[0]);popfp();pc=0x7a6647u;break;} // dec4 faddp st(4)
case 0x7a6647u:{std::swap(st[0],st[3]);pc=0x7a6649u;break;} // d9cb fxch st(3)
case 0x7a6649u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a664du;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a664du:{pushfp(fp((r[7]+8u)));pc=0x7a6651u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a6651u:{st[0]=product(st[0],st[0]);pc=0x7a6653u;break;} // dcc8 fmul st(0), st(0)
case 0x7a6653u:{pushfp(fp((r[7]+4u)));pc=0x7a6657u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a6657u:{st[0]=product(st[0],st[0]);pc=0x7a6659u;break;} // dcc8 fmul st(0), st(0)
case 0x7a6659u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a665bu;break;} // dec1 faddp st(1)
case 0x7a665bu:{pushfp(fp((r[7]+12u)));pc=0x7a665fu;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x7a665fu:{st[0]=product(st[0],st[0]);pc=0x7a6661u;break;} // dcc8 fmul st(0), st(0)
case 0x7a6661u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6663u;break;} // dec1 faddp st(1)
case 0x7a6663u:{pushfp(fp((r[7]+36u)));pc=0x7a6667u;break;} // d9442424 fld dword ptr [esp + 0x24]
case 0x7a6667u:{pushfp(fp((r[7]+32u)));pc=0x7a666bu;break;} // d9442420 fld dword ptr [esp + 0x20]
case 0x7a666bu:{st[1]=product(st[1],st[0]);pc=0x7a666du;break;} // dcc9 fmul st(1), st(0)
case 0x7a666du:{std::swap(st[0],st[2]);pc=0x7a666fu;break;} // d9ca fxch st(2)
case 0x7a666fu:{compare(st[0],st[1]);popfp();popfp();pc=0x7a6671u;break;} // ded9 fcompp
case 0x7a6671u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6673u;break;} // dfe0 fnstsw ax
case 0x7a6673u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6676u;break;} // f6c441 test ah, 0x41
case 0x7a6676u:{pc=(!zf)?0x7a668bu:0x7a6678u;break;} // 7513 jne 0x7a668b
case 0x7a6678u:{st[2]=st[0];popfp();pc=0x7a667au;break;} // ddda fstp st(2)
case 0x7a667au:{st[0]=st[0];popfp();pc=0x7a667cu;break;} // ddd8 fstp st(0)
case 0x7a667cu:{st[1]=st[0];popfp();pc=0x7a667eu;break;} // ddd9 fstp st(1)
case 0x7a667eu:{st[2]=st[0];popfp();pc=0x7a6680u;break;} // ddda fstp st(2)
case 0x7a6680u:{st[0]=st[0];popfp();pc=0x7a6682u;break;} // ddd8 fstp st(0)
case 0x7a6682u:{st[0]=st[0];popfp();pc=0x7a6684u;break;} // ddd8 fstp st(0)
case 0x7a6684u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6686u;break;} // 32c0 xor al, al
case 0x7a6686u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a6687u;break;} // 5d pop ebp
case 0x7a6687u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a668au;break;} // 83c418 add esp, 0x18
case 0x7a668au:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a668bu:{std::swap(st[0],st[4]);pc=0x7a668du;break;} // d9cc fxch st(4)
case 0x7a668du:{st[0]=product(st[0],st[1]);pc=0x7a668fu;break;} // d8c9 fmul st(1)
case 0x7a668fu:{pushfp(st[2]);pc=0x7a6691u;break;} // d9c2 fld st(2)
case 0x7a6691u:{st[6]=product(st[6],st[0]);popfp();pc=0x7a6693u;break;} // dece fmulp st(6)
case 0x7a6693u:{st[5]=sum(st[5],st[0]);popfp();pc=0x7a6695u;break;} // dec5 faddp st(5)
case 0x7a6695u:{pushfp(fp((r[7]+24u)));pc=0x7a6699u;break;} // d9442418 fld dword ptr [esp + 0x18]
case 0x7a6699u:{st[0]=product(st[0],st[3]);pc=0x7a669bu;break;} // d8cb fmul st(3)
case 0x7a669bu:{st[5]=sum(st[5],st[0]);popfp();pc=0x7a669du;break;} // dec5 faddp st(5)
case 0x7a669du:{st[0]=product(st[0],st[0]);pc=0x7a669fu;break;} // dcc8 fmul st(0), st(0)
case 0x7a669fu:{pushfp(st[1]);pc=0x7a66a1u;break;} // d9c1 fld st(1)
case 0x7a66a1u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a66a3u;break;} // deca fmulp st(2)
case 0x7a66a3u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a66a5u;break;} // dec1 faddp st(1)
case 0x7a66a5u:{pushfp(st[1]);pc=0x7a66a7u;break;} // d9c1 fld st(1)
case 0x7a66a7u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a66a9u;break;} // deca fmulp st(2)
case 0x7a66a9u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a66abu;break;} // dec1 faddp st(1)
case 0x7a66abu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a66adu;break;} // dee1 fsubrp st(1)
case 0x7a66adu:{pushfp(st[1]);pc=0x7a66afu;break;} // d9c1 fld st(1)
case 0x7a66afu:{st[0]=product(st[0],st[2]);pc=0x7a66b1u;break;} // d8ca fmul st(2)
case 0x7a66b1u:{pushfp(st[1]);pc=0x7a66b3u;break;} // d9c1 fld st(1)
case 0x7a66b3u:{st[0]=product(st[0],fp((r[7]+36u)));pc=0x7a66b7u;break;} // d84c2424 fmul dword ptr [esp + 0x24]
case 0x7a66b7u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a66b9u;break;} // dee9 fsubp st(1)
case 0x7a66b9u:{st[0]=std::abs(st[0]);pc=0x7a66bbu;break;} // d9e1 fabs
case 0x7a66bbu:{pushfp(fp((10236808u)));pc=0x7a66c1u;break;} // d90588339c00 fld dword ptr [0x9c3388]
case 0x7a66c1u:{compare(st[0],st[1]);pc=0x7a66c3u;break;} // d8d1 fcom st(1)
case 0x7a66c3u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a66c5u;break;} // dfe0 fnstsw ax
case 0x7a66c5u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a66c8u;break;} // f6c441 test ah, 0x41
case 0x7a66c8u:{pc=(!zf)?0x7a66f4u:0x7a66cau;break;} // 752a jne 0x7a66f4
case 0x7a66cau:{st[1]=st[0];popfp();pc=0x7a66ccu;break;} // ddd9 fstp st(1)
case 0x7a66ccu:{put((r[6]),1u,4);pc=0x7a66d3u;break;} // c7450001000000 mov dword ptr [ebp], 1
case 0x7a66d3u:{st[1]=st[0];popfp();pc=0x7a66d5u;break;} // ddd9 fstp st(1)
case 0x7a66d5u:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a66d7u;break;} // b001 mov al, 1
case 0x7a66d7u:{st[0]=st[0];popfp();pc=0x7a66d9u;break;} // ddd8 fstp st(0)
case 0x7a66d9u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a66dau;break;} // 5d pop ebp
case 0x7a66dau:{st[0]=st[0];popfp();pc=0x7a66dcu;break;} // ddd8 fstp st(0)
case 0x7a66dcu:{pushfp(0.);pc=0x7a66deu;break;} // d9ee fldz
case 0x7a66deu:{fpwrite((r[1]),st[0]);popfp();pc=0x7a66e0u;break;} // d91b fstp dword ptr [ebx]
case 0x7a66e0u:{pushfp(fp((r[2])));pc=0x7a66e2u;break;} // d901 fld dword ptr [ecx]
case 0x7a66e2u:{fpwrite((r[4]),st[0]);popfp();pc=0x7a66e4u;break;} // d91e fstp dword ptr [esi]
case 0x7a66e4u:{pushfp(fp((r[2]+4u)));pc=0x7a66e7u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a66e7u:{fpwrite((r[4]+4u),st[0]);popfp();pc=0x7a66eau;break;} // d95e04 fstp dword ptr [esi + 4]
case 0x7a66eau:{pushfp(fp((r[2]+8u)));pc=0x7a66edu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a66edu:{fpwrite((r[4]+8u),st[0]);popfp();pc=0x7a66f0u;break;} // d95e08 fstp dword ptr [esi + 8]
case 0x7a66f0u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a66f3u;break;} // 83c418 add esp, 0x18
case 0x7a66f3u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a66f4u:{std::swap(st[0],st[1]);pc=0x7a66f6u;break;} // d9c9 fxch st(1)
case 0x7a66f6u:{st[0]=root(st[0]);pc=0x7a66f8u;break;} // d9fa fsqrt
case 0x7a66f8u:{pushfp(1.);pc=0x7a66fau;break;} // d9e8 fld1
case 0x7a66fau:{pushfp(st[0]);pc=0x7a66fcu;break;} // d9c0 fld st(0)
case 0x7a66fcu:{st[2]=quotient(st[0],st[2]);popfp();pc=0x7a66feu;break;} // def2 fdivrp st(2)
case 0x7a66feu:{pushfp(st[1]);pc=0x7a6700u;break;} // d9c1 fld st(1)
case 0x7a6700u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a6702u;break;} // decd fmulp st(5)
case 0x7a6702u:{st[4]=difference(st[0],st[4]);popfp();pc=0x7a6704u;break;} // dee4 fsubrp st(4)
case 0x7a6704u:{pushfp(st[3]);pc=0x7a6706u;break;} // d9c3 fld st(3)
case 0x7a6706u:{st[0]=std::abs(st[0]);pc=0x7a6708u;break;} // d9e1 fabs
case 0x7a6708u:{compare(st[0],st[2]);popfp();pc=0x7a670au;break;} // d8da fcomp st(2)
case 0x7a670au:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a670cu;break;} // dfe0 fnstsw ax
case 0x7a670cu:{st[1]=st[0];popfp();pc=0x7a670eu;break;} // ddd9 fstp st(1)
case 0x7a670eu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a6711u;break;} // f6c405 test ah, 5
case 0x7a6711u:{pc=(pf)?0x7a6720u:0x7a6713u;break;} // 7a0d jp 0x7a6720
case 0x7a6713u:{st[1]=st[0];popfp();pc=0x7a6715u;break;} // ddd9 fstp st(1)
case 0x7a6715u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6717u;break;} // 32c0 xor al, al
case 0x7a6717u:{st[1]=st[0];popfp();pc=0x7a6719u;break;} // ddd9 fstp st(1)
case 0x7a6719u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a671au;break;} // 5d pop ebp
case 0x7a671au:{st[0]=st[0];popfp();pc=0x7a671cu;break;} // ddd8 fstp st(0)
case 0x7a671cu:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a671fu;break;} // 83c418 add esp, 0x18
case 0x7a671fu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6720u:{st[1]=product(st[1],st[0]);popfp();pc=0x7a6722u;break;} // dec9 fmulp st(1)
case 0x7a6722u:{st[1]=quotient(st[0],st[1]);popfp();pc=0x7a6724u;break;} // def1 fdivrp st(1)
case 0x7a6724u:{fpwrite((r[1]),st[0]);pc=0x7a6726u;break;} // d913 fst dword ptr [ebx]
case 0x7a6726u:{compare(st[0],fp((r[7]+40u)));popfp();pc=0x7a672au;break;} // d85c2428 fcomp dword ptr [esp + 0x28]
case 0x7a672au:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a672cu;break;} // dfe0 fnstsw ax
case 0x7a672cu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a672fu;break;} // f6c441 test ah, 0x41
case 0x7a672fu:{pc=(zf)?0x7a6684u:0x7a6735u;break;} // 0f844fffffff je 0x7a6684
case 0x7a6735u:{put((r[6]),1u,4);pc=0x7a673cu;break;} // c7450001000000 mov dword ptr [ebp], 1
case 0x7a673cu:{pushfp(fp((r[1])));pc=0x7a673eu;break;} // d903 fld dword ptr [ebx]
case 0x7a673eu:{pushfp(fp((r[3])));pc=0x7a6740u;break;} // d902 fld dword ptr [edx]
case 0x7a6740u:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a6742u;break;} // b001 mov al, 1
case 0x7a6742u:{pushfp(fp((r[3]+4u)));pc=0x7a6745u;break;} // d94204 fld dword ptr [edx + 4]
case 0x7a6745u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a6746u;break;} // 5d pop ebp
case 0x7a6746u:{pushfp(fp((r[3]+8u)));pc=0x7a6749u;break;} // d94208 fld dword ptr [edx + 8]
case 0x7a6749u:{std::swap(st[0],st[2]);pc=0x7a674bu;break;} // d9ca fxch st(2)
case 0x7a674bu:{st[0]=product(st[0],st[3]);pc=0x7a674du;break;} // d8cb fmul st(3)
case 0x7a674du:{std::swap(st[0],st[1]);pc=0x7a674fu;break;} // d9c9 fxch st(1)
case 0x7a674fu:{st[0]=product(st[0],st[3]);pc=0x7a6751u;break;} // d8cb fmul st(3)
case 0x7a6751u:{std::swap(st[0],st[2]);pc=0x7a6753u;break;} // d9ca fxch st(2)
case 0x7a6753u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6755u;break;} // decb fmulp st(3)
case 0x7a6755u:{st[0]=sum(st[0],fp((r[2])));pc=0x7a6757u;break;} // d801 fadd dword ptr [ecx]
case 0x7a6757u:{std::swap(st[0],st[1]);pc=0x7a6759u;break;} // d9c9 fxch st(1)
case 0x7a6759u:{st[0]=sum(st[0],fp((r[2]+4u)));pc=0x7a675cu;break;} // d84104 fadd dword ptr [ecx + 4]
case 0x7a675cu:{pushfp(fp((r[2]+8u)));pc=0x7a675fu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a675fu:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a6761u;break;} // dec3 faddp st(3)
case 0x7a6761u:{std::swap(st[0],st[1]);pc=0x7a6763u;break;} // d9c9 fxch st(1)
case 0x7a6763u:{fpwrite((r[4]),st[0]);popfp();pc=0x7a6765u;break;} // d91e fstp dword ptr [esi]
case 0x7a6765u:{fpwrite((r[4]+4u),st[0]);popfp();pc=0x7a6768u;break;} // d95e04 fstp dword ptr [esi + 4]
case 0x7a6768u:{fpwrite((r[4]+8u),st[0]);popfp();pc=0x7a676bu;break;} // d95e08 fstp dword ptr [esi + 8]
case 0x7a676bu:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x7a676eu;break;} // 83c418 add esp, 0x18
case 0x7a676eu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6770u:{uint32_t v=(r[7])-(172u);r[7]=v;flags(v);pc=0x7a6776u;break;} // 81ecac000000 sub esp, 0xac
case 0x7a6776u:{r[2]=get((r[7]+176u),4);pc=0x7a677du;break;} // 8b8c24b0000000 mov ecx, dword ptr [esp + 0xb0]
case 0x7a677du:{r[0]=get((r[2]+16u),4);pc=0x7a6780u;break;} // 8b4110 mov eax, dword ptr [ecx + 0x10]
case 0x7a6780u:{pushfp(fp((r[0])));pc=0x7a6782u;break;} // d900 fld dword ptr [eax]
case 0x7a6782u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a6783u;break;} // 56 push esi
case 0x7a6783u:{fpwrite((r[7]+80u),st[0]);pc=0x7a6787u;break;} // d9542450 fst dword ptr [esp + 0x50]
case 0x7a6787u:{r[4]=get((r[7]+188u),4);pc=0x7a678eu;break;} // 8bb424bc000000 mov esi, dword ptr [esp + 0xbc]
case 0x7a678eu:{pushfp(fp((r[0]+4u)));pc=0x7a6791u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a6791u:{fpwrite((r[7]+84u),st[0]);pc=0x7a6795u;break;} // d9542454 fst dword ptr [esp + 0x54]
case 0x7a6795u:{pushfp(fp((r[0]+8u)));pc=0x7a6798u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a6798u:{r[0]=get((r[2]+20u),4);pc=0x7a679bu;break;} // 8b4114 mov eax, dword ptr [ecx + 0x14]
case 0x7a679bu:{fpwrite((r[7]+88u),st[0]);pc=0x7a679fu;break;} // d9542458 fst dword ptr [esp + 0x58]
case 0x7a679fu:{pushfp(fp((r[0])));pc=0x7a67a1u;break;} // d900 fld dword ptr [eax]
case 0x7a67a1u:{fpwrite((r[7]+92u),st[0]);pc=0x7a67a5u;break;} // d954245c fst dword ptr [esp + 0x5c]
case 0x7a67a5u:{pushfp(fp((r[0]+4u)));pc=0x7a67a8u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a67a8u:{fpwrite((r[7]+96u),st[0]);pc=0x7a67acu;break;} // d9542460 fst dword ptr [esp + 0x60]
case 0x7a67acu:{pushfp(fp((r[0]+8u)));pc=0x7a67afu;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a67afu:{r[0]=get((r[2]+24u),4);pc=0x7a67b2u;break;} // 8b4118 mov eax, dword ptr [ecx + 0x18]
case 0x7a67b2u:{fpwrite((r[7]+100u),st[0]);pc=0x7a67b6u;break;} // d9542464 fst dword ptr [esp + 0x64]
case 0x7a67b6u:{pushfp(fp((r[0])));pc=0x7a67b8u;break;} // d900 fld dword ptr [eax]
case 0x7a67b8u:{fpwrite((r[7]+104u),st[0]);pc=0x7a67bcu;break;} // d9542468 fst dword ptr [esp + 0x68]
case 0x7a67bcu:{pushfp(fp((r[0]+4u)));pc=0x7a67bfu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a67bfu:{fpwrite((r[7]+108u),st[0]);popfp();pc=0x7a67c3u;break;} // d95c246c fstp dword ptr [esp + 0x6c]
case 0x7a67c3u:{pushfp(fp((r[0]+8u)));pc=0x7a67c6u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a67c6u:{fpwrite((r[7]+112u),st[0]);popfp();pc=0x7a67cau;break;} // d95c2470 fstp dword ptr [esp + 0x70]
case 0x7a67cau:{pushfp(st[3]);pc=0x7a67ccu;break;} // d9c3 fld st(3)
case 0x7a67ccu:{st[0]=difference(st[0],st[7]);pc=0x7a67ceu;break;} // d8e7 fsub st(7)
case 0x7a67ceu:{fpwrite((r[7]+44u),st[0]);popfp();pc=0x7a67d2u;break;} // d95c242c fstp dword ptr [esp + 0x2c]
case 0x7a67d2u:{pushfp(st[2]);pc=0x7a67d4u;break;} // d9c2 fld st(2)
case 0x7a67d4u:{st[0]=difference(st[0],st[6]);pc=0x7a67d6u;break;} // d8e6 fsub st(6)
case 0x7a67d6u:{fpwrite((r[7]+48u),st[0]);popfp();pc=0x7a67dau;break;} // d95c2430 fstp dword ptr [esp + 0x30]
case 0x7a67dau:{pushfp(st[1]);pc=0x7a67dcu;break;} // d9c1 fld st(1)
case 0x7a67dcu:{st[0]=difference(st[0],st[5]);pc=0x7a67deu;break;} // d8e5 fsub st(5)
case 0x7a67deu:{fpwrite((r[7]+52u),st[0]);popfp();pc=0x7a67e2u;break;} // d95c2434 fstp dword ptr [esp + 0x34]
case 0x7a67e2u:{pushfp(st[0]);pc=0x7a67e4u;break;} // d9c0 fld st(0)
case 0x7a67e4u:{st[4]=difference(st[0],st[4]);popfp();pc=0x7a67e6u;break;} // dee4 fsubrp st(4)
case 0x7a67e6u:{std::swap(st[0],st[3]);pc=0x7a67e8u;break;} // d9cb fxch st(3)
case 0x7a67e8u:{fpwrite((r[7]+56u),st[0]);pc=0x7a67ecu;break;} // d9542438 fst dword ptr [esp + 0x38]
case 0x7a67ecu:{pushfp(fp((r[7]+108u)));pc=0x7a67f0u;break;} // d944246c fld dword ptr [esp + 0x6c]
case 0x7a67f0u:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a67f2u;break;} // dee3 fsubrp st(3)
case 0x7a67f2u:{std::swap(st[0],st[2]);pc=0x7a67f4u;break;} // d9ca fxch st(2)
case 0x7a67f4u:{fpwrite((r[7]+60u),st[0]);popfp();pc=0x7a67f8u;break;} // d95c243c fstp dword ptr [esp + 0x3c]
case 0x7a67f8u:{pushfp(fp((r[7]+112u)));pc=0x7a67fcu;break;} // d9442470 fld dword ptr [esp + 0x70]
case 0x7a67fcu:{pushfp(st[0]);pc=0x7a67feu;break;} // d9c0 fld st(0)
case 0x7a67feu:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a6800u;break;} // dee2 fsubrp st(2)
case 0x7a6800u:{std::swap(st[0],st[1]);pc=0x7a6802u;break;} // d9c9 fxch st(1)
case 0x7a6802u:{fpwrite((r[7]+64u),st[0]);pc=0x7a6806u;break;} // d9542440 fst dword ptr [esp + 0x40]
case 0x7a6806u:{pushfp(st[6]);pc=0x7a6808u;break;} // d9c6 fld st(6)
case 0x7a6808u:{st[4]=difference(st[0],st[4]);popfp();pc=0x7a680au;break;} // dee4 fsubrp st(4)
case 0x7a680au:{std::swap(st[0],st[3]);pc=0x7a680cu;break;} // d9cb fxch st(3)
case 0x7a680cu:{fpwrite((r[7]+68u),st[0]);popfp();pc=0x7a6810u;break;} // d95c2444 fstp dword ptr [esp + 0x44]
case 0x7a6810u:{pushfp(st[4]);pc=0x7a6812u;break;} // d9c4 fld st(4)
case 0x7a6812u:{st[0]=difference(st[0],fp((r[7]+108u)));pc=0x7a6816u;break;} // d864246c fsub dword ptr [esp + 0x6c]
case 0x7a6816u:{fpwrite((r[7]+72u),st[0]);popfp();pc=0x7a681au;break;} // d95c2448 fstp dword ptr [esp + 0x48]
case 0x7a681au:{st[0]=difference(st[3],st[0]);pc=0x7a681cu;break;} // d8eb fsubr st(3)
case 0x7a681cu:{fpwrite((r[7]+76u),st[0]);popfp();pc=0x7a6820u;break;} // d95c244c fstp dword ptr [esp + 0x4c]
case 0x7a6820u:{pushfp(fp((r[7]+60u)));pc=0x7a6824u;break;} // d944243c fld dword ptr [esp + 0x3c]
case 0x7a6824u:{st[0]=product(st[0],fp((r[7]+52u)));pc=0x7a6828u;break;} // d84c2434 fmul dword ptr [esp + 0x34]
case 0x7a6828u:{pushfp(st[2]);pc=0x7a682au;break;} // d9c2 fld st(2)
case 0x7a682au:{st[0]=product(st[0],fp((r[7]+48u)));pc=0x7a682eu;break;} // d84c2430 fmul dword ptr [esp + 0x30]
case 0x7a682eu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a6830u;break;} // dee9 fsubp st(1)
case 0x7a6830u:{pushfp(fp((r[7]+44u)));pc=0x7a6834u;break;} // d944242c fld dword ptr [esp + 0x2c]
case 0x7a6834u:{st[3]=product(st[3],st[0]);pc=0x7a6836u;break;} // dccb fmul st(3), st(0)
case 0x7a6836u:{pushfp(fp((r[7]+52u)));pc=0x7a683au;break;} // d9442434 fld dword ptr [esp + 0x34]
case 0x7a683au:{st[0]=product(st[0],st[3]);pc=0x7a683cu;break;} // d8cb fmul st(3)
case 0x7a683cu:{st[4]=difference(st[4],st[0]);popfp();pc=0x7a683eu;break;} // deec fsubp st(4)
case 0x7a683eu:{pushfp(fp((r[7]+48u)));pc=0x7a6842u;break;} // d9442430 fld dword ptr [esp + 0x30]
case 0x7a6842u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6844u;break;} // decb fmulp st(3)
case 0x7a6844u:{st[0]=product(st[0],fp((r[7]+60u)));pc=0x7a6848u;break;} // d84c243c fmul dword ptr [esp + 0x3c]
case 0x7a6848u:{st[2]=difference(st[2],st[0]);popfp();pc=0x7a684au;break;} // deea fsubp st(2)
case 0x7a684au:{pushfp(fp((r[4]+8u)));pc=0x7a684du;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a684du:{st[0]=product(st[0],st[3]);pc=0x7a684fu;break;} // d8cb fmul st(3)
case 0x7a684fu:{pushfp(fp((r[4]+4u)));pc=0x7a6852u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6852u:{st[0]=product(st[0],st[2]);pc=0x7a6854u;break;} // d8ca fmul st(2)
case 0x7a6854u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6856u;break;} // dec1 faddp st(1)
case 0x7a6856u:{pushfp(fp((r[4]+12u)));pc=0x7a6859u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6859u:{st[0]=product(st[0],st[3]);pc=0x7a685bu;break;} // d8cb fmul st(3)
case 0x7a685bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a685du;break;} // dec1 faddp st(1)
case 0x7a685du:{fpwrite((r[7]+116u),st[0]);popfp();pc=0x7a6861u;break;} // d95c2474 fstp dword ptr [esp + 0x74]
case 0x7a6861u:{pushfp(st[2]);pc=0x7a6863u;break;} // d9c2 fld st(2)
case 0x7a6863u:{st[0]=product(st[0],st[3]);pc=0x7a6865u;break;} // d8cb fmul st(3)
case 0x7a6865u:{pushfp(st[1]);pc=0x7a6867u;break;} // d9c1 fld st(1)
case 0x7a6867u:{st[0]=product(st[0],st[2]);pc=0x7a6869u;break;} // d8ca fmul st(2)
case 0x7a6869u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a686bu;break;} // dec1 faddp st(1)
case 0x7a686bu:{pushfp(st[2]);pc=0x7a686du;break;} // d9c2 fld st(2)
case 0x7a686du:{st[0]=product(st[0],st[3]);pc=0x7a686fu;break;} // d8cb fmul st(3)
case 0x7a686fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6871u;break;} // dec1 faddp st(1)
case 0x7a6871u:{st[0]=product(st[0],fp((r[4])));pc=0x7a6873u;break;} // d80e fmul dword ptr [esi]
case 0x7a6873u:{st[0]=product(st[0],fp((r[4])));pc=0x7a6875u;break;} // d80e fmul dword ptr [esi]
case 0x7a6875u:{fpwrite((r[7]+172u),st[0]);popfp();pc=0x7a687cu;break;} // d99c24ac000000 fstp dword ptr [esp + 0xac]
case 0x7a687cu:{pushfp(st[0]);pc=0x7a687eu;break;} // d9c0 fld st(0)
case 0x7a687eu:{st[0]=product(st[0],st[6]);pc=0x7a6880u;break;} // d8ce fmul st(6)
case 0x7a6880u:{pushfp(st[3]);pc=0x7a6882u;break;} // d9c3 fld st(3)
case 0x7a6882u:{st[0]=product(st[0],st[6]);pc=0x7a6884u;break;} // d8ce fmul st(6)
case 0x7a6884u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6886u;break;} // dec1 faddp st(1)
case 0x7a6886u:{pushfp(st[2]);pc=0x7a6888u;break;} // d9c2 fld st(2)
case 0x7a6888u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a688au;break;} // decd fmulp st(5)
case 0x7a688au:{st[4]=sum(st[4],st[0]);popfp();pc=0x7a688cu;break;} // dec4 faddp st(4)
case 0x7a688cu:{std::swap(st[0],st[3]);pc=0x7a688eu;break;} // d9cb fxch st(3)
case 0x7a688eu:{fpwrite((r[7]+40u),st[0]);pc=0x7a6892u;break;} // d9542428 fst dword ptr [esp + 0x28]
case 0x7a6892u:{st[0]=difference(fp((r[7]+116u)),st[0]);pc=0x7a6896u;break;} // d86c2474 fsubr dword ptr [esp + 0x74]
case 0x7a6896u:{fpwrite((r[7]+160u),st[0]);popfp();pc=0x7a689du;break;} // d99c24a0000000 fstp dword ptr [esp + 0xa0]
case 0x7a689du:{pushfp(fp((r[7]+48u)));pc=0x7a68a1u;break;} // d9442430 fld dword ptr [esp + 0x30]
case 0x7a68a1u:{st[0]=product(st[0],st[1]);pc=0x7a68a3u;break;} // d8c9 fmul st(1)
case 0x7a68a3u:{pushfp(st[2]);pc=0x7a68a5u;break;} // d9c2 fld st(2)
case 0x7a68a5u:{pushfp(fp((r[7]+52u)));pc=0x7a68a9u;break;} // d9442434 fld dword ptr [esp + 0x34]
case 0x7a68a9u:{st[1]=product(st[1],st[0]);pc=0x7a68abu;break;} // dcc9 fmul st(1), st(0)
case 0x7a68abu:{std::swap(st[0],st[2]);pc=0x7a68adu;break;} // d9ca fxch st(2)
case 0x7a68adu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a68afu;break;} // dee1 fsubrp st(1)
case 0x7a68afu:{fpwrite((r[7]+120u),st[0]);popfp();pc=0x7a68b3u;break;} // d95c2478 fstp dword ptr [esp + 0x78]
case 0x7a68b3u:{st[0]=product(st[0],st[3]);pc=0x7a68b5u;break;} // d8cb fmul st(3)
case 0x7a68b5u:{pushfp(st[1]);pc=0x7a68b7u;break;} // d9c1 fld st(1)
case 0x7a68b7u:{pushfp(fp((r[7]+44u)));pc=0x7a68bbu;break;} // d944242c fld dword ptr [esp + 0x2c]
case 0x7a68bbu:{st[1]=product(st[1],st[0]);pc=0x7a68bdu;break;} // dcc9 fmul st(1), st(0)
case 0x7a68bdu:{std::swap(st[0],st[2]);pc=0x7a68bfu;break;} // d9ca fxch st(2)
case 0x7a68bfu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a68c1u;break;} // dee1 fsubrp st(1)
case 0x7a68c1u:{fpwrite((r[7]+124u),st[0]);popfp();pc=0x7a68c5u;break;} // d95c247c fstp dword ptr [esp + 0x7c]
case 0x7a68c5u:{st[0]=product(st[0],st[2]);pc=0x7a68c7u;break;} // d8ca fmul st(2)
case 0x7a68c7u:{pushfp(fp((r[7]+48u)));pc=0x7a68cbu;break;} // d9442430 fld dword ptr [esp + 0x30]
case 0x7a68cbu:{st[0]=product(st[0],st[4]);pc=0x7a68cdu;break;} // d8cc fmul st(4)
case 0x7a68cdu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a68cfu;break;} // dee9 fsubp st(1)
case 0x7a68cfu:{fpwrite((r[7]+128u),st[0]);popfp();pc=0x7a68d6u;break;} // d99c2480000000 fstp dword ptr [esp + 0x80]
case 0x7a68d6u:{pushfp(fp((r[7]+60u)));pc=0x7a68dau;break;} // d944243c fld dword ptr [esp + 0x3c]
case 0x7a68dau:{st[0]=product(st[0],st[1]);pc=0x7a68dcu;break;} // d8c9 fmul st(1)
case 0x7a68dcu:{pushfp(st[2]);pc=0x7a68deu;break;} // d9c2 fld st(2)
case 0x7a68deu:{pushfp(fp((r[7]+64u)));pc=0x7a68e2u;break;} // d9442440 fld dword ptr [esp + 0x40]
case 0x7a68e2u:{st[1]=product(st[1],st[0]);pc=0x7a68e4u;break;} // dcc9 fmul st(1), st(0)
case 0x7a68e4u:{std::swap(st[0],st[2]);pc=0x7a68e6u;break;} // d9ca fxch st(2)
case 0x7a68e6u:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a68e8u;break;} // dee1 fsubrp st(1)
case 0x7a68e8u:{fpwrite((r[7]+132u),st[0]);popfp();pc=0x7a68efu;break;} // d99c2484000000 fstp dword ptr [esp + 0x84]
case 0x7a68efu:{st[0]=product(st[0],st[3]);pc=0x7a68f1u;break;} // d8cb fmul st(3)
case 0x7a68f1u:{pushfp(st[1]);pc=0x7a68f3u;break;} // d9c1 fld st(1)
case 0x7a68f3u:{pushfp(fp((r[7]+56u)));pc=0x7a68f7u;break;} // d9442438 fld dword ptr [esp + 0x38]
case 0x7a68f7u:{st[1]=product(st[1],st[0]);pc=0x7a68f9u;break;} // dcc9 fmul st(1), st(0)
case 0x7a68f9u:{std::swap(st[0],st[2]);pc=0x7a68fbu;break;} // d9ca fxch st(2)
case 0x7a68fbu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a68fdu;break;} // dee1 fsubrp st(1)
case 0x7a68fdu:{fpwrite((r[7]+136u),st[0]);popfp();pc=0x7a6904u;break;} // d99c2488000000 fstp dword ptr [esp + 0x88]
case 0x7a6904u:{st[0]=product(st[0],st[2]);pc=0x7a6906u;break;} // d8ca fmul st(2)
case 0x7a6906u:{pushfp(fp((r[7]+60u)));pc=0x7a690au;break;} // d944243c fld dword ptr [esp + 0x3c]
case 0x7a690au:{st[0]=product(st[0],st[4]);pc=0x7a690cu;break;} // d8cc fmul st(4)
case 0x7a690cu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a690eu;break;} // dee9 fsubp st(1)
case 0x7a690eu:{fpwrite((r[7]+140u),st[0]);popfp();pc=0x7a6915u;break;} // d99c248c000000 fstp dword ptr [esp + 0x8c]
case 0x7a6915u:{pushfp(fp((r[7]+72u)));pc=0x7a6919u;break;} // d9442448 fld dword ptr [esp + 0x48]
case 0x7a6919u:{st[0]=product(st[0],st[1]);pc=0x7a691bu;break;} // d8c9 fmul st(1)
case 0x7a691bu:{pushfp(st[2]);pc=0x7a691du;break;} // d9c2 fld st(2)
case 0x7a691du:{pushfp(fp((r[7]+76u)));pc=0x7a6921u;break;} // d944244c fld dword ptr [esp + 0x4c]
case 0x7a6921u:{st[1]=product(st[1],st[0]);pc=0x7a6923u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6923u:{std::swap(st[0],st[2]);pc=0x7a6925u;break;} // d9ca fxch st(2)
case 0x7a6925u:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a6927u;break;} // dee1 fsubrp st(1)
case 0x7a6927u:{fpwrite((r[7]+144u),st[0]);popfp();pc=0x7a692eu;break;} // d99c2490000000 fstp dword ptr [esp + 0x90]
case 0x7a692eu:{st[0]=product(st[0],st[3]);pc=0x7a6930u;break;} // d8cb fmul st(3)
case 0x7a6930u:{pushfp(fp((r[7]+68u)));pc=0x7a6934u;break;} // d9442444 fld dword ptr [esp + 0x44]
case 0x7a6934u:{st[2]=product(st[2],st[0]);pc=0x7a6936u;break;} // dcca fmul st(2), st(0)
case 0x7a6936u:{std::swap(st[0],st[1]);pc=0x7a6938u;break;} // d9c9 fxch st(1)
case 0x7a6938u:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a693au;break;} // dee2 fsubrp st(2)
case 0x7a693au:{std::swap(st[0],st[1]);pc=0x7a693cu;break;} // d9c9 fxch st(1)
case 0x7a693cu:{fpwrite((r[7]+148u),st[0]);popfp();pc=0x7a6943u;break;} // d99c2494000000 fstp dword ptr [esp + 0x94]
case 0x7a6943u:{st[1]=product(st[1],st[0]);pc=0x7a6945u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6945u:{pushfp(fp((r[7]+72u)));pc=0x7a6949u;break;} // d9442448 fld dword ptr [esp + 0x48]
case 0x7a6949u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a694bu;break;} // decb fmulp st(3)
case 0x7a694bu:{std::swap(st[0],st[1]);pc=0x7a694du;break;} // d9c9 fxch st(1)
case 0x7a694du:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a694fu;break;} // dee2 fsubrp st(2)
case 0x7a694fu:{std::swap(st[0],st[1]);pc=0x7a6951u;break;} // d9c9 fxch st(1)
case 0x7a6951u:{fpwrite((r[7]+152u),st[0]);popfp();pc=0x7a6958u;break;} // d99c2498000000 fstp dword ptr [esp + 0x98]
case 0x7a6958u:{pushfp(fp((r[2]+4u)));pc=0x7a695bu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a695bu:{pushfp(fp((r[2])));pc=0x7a695du;break;} // d901 fld dword ptr [ecx]
case 0x7a695du:{pushfp(fp((r[2]+8u)));pc=0x7a6960u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6960u:{pushfp(st[1]);pc=0x7a6962u;break;} // d9c1 fld st(1)
case 0x7a6962u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a6964u;break;} // deca fmulp st(2)
case 0x7a6964u:{pushfp(st[2]);pc=0x7a6966u;break;} // d9c2 fld st(2)
case 0x7a6966u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6968u;break;} // decb fmulp st(3)
case 0x7a6968u:{std::swap(st[0],st[1]);pc=0x7a696au;break;} // d9c9 fxch st(1)
case 0x7a696au:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a696cu;break;} // dec2 faddp st(2)
case 0x7a696cu:{st[0]=product(st[0],st[0]);pc=0x7a696eu;break;} // dcc8 fmul st(0), st(0)
case 0x7a696eu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6970u;break;} // dec1 faddp st(1)
case 0x7a6970u:{pushfp(0.);pc=0x7a6972u;break;} // d9ee fldz
case 0x7a6972u:{pushfp(st[0]);pc=0x7a6974u;break;} // d9c0 fld st(0)
case 0x7a6974u:{compare(st[0],st[2]);popfp();pc=0x7a6976u;break;} // ddea fucomp st(2)
case 0x7a6976u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6978u;break;} // dfe0 fnstsw ax
case 0x7a6978u:{st[1]=st[0];popfp();pc=0x7a697au;break;} // ddd9 fstp st(1)
case 0x7a697au:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x7a697du;break;} // f6c444 test ah, 0x44
case 0x7a697du:{pc=(pf)?0x7a6991u:0x7a697fu;break;} // 7a12 jp 0x7a6991
case 0x7a697fu:{st[0]=st[0];popfp();pc=0x7a6981u;break;} // ddd8 fstp st(0)
case 0x7a6981u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6983u;break;} // 32c0 xor al, al
case 0x7a6983u:{st[2]=st[0];popfp();pc=0x7a6985u;break;} // ddda fstp st(2)
case 0x7a6985u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6986u;break;} // 5e pop esi
case 0x7a6986u:{st[0]=st[0];popfp();pc=0x7a6988u;break;} // ddd8 fstp st(0)
case 0x7a6988u:{st[0]=st[0];popfp();pc=0x7a698au;break;} // ddd8 fstp st(0)
case 0x7a698au:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6990u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6990u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6991u:{r[3]=get((r[7]+184u),4);pc=0x7a6998u;break;} // 8b9424b8000000 mov edx, dword ptr [esp + 0xb8]
case 0x7a6998u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a6999u;break;} // 57 push edi
case 0x7a6999u:{r[5]=get((r[7]+196u),4);pc=0x7a69a0u;break;} // 8bbc24c4000000 mov edi, dword ptr [esp + 0xc4]
case 0x7a69a0u:{pushfp(fp((r[5])));pc=0x7a69a2u;break;} // d907 fld dword ptr [edi]
case 0x7a69a2u:{st[0]=difference(st[0],fp((r[3])));pc=0x7a69a4u;break;} // d822 fsub dword ptr [edx]
case 0x7a69a4u:{fpwrite((r[7]+172u),st[0]);pc=0x7a69abu;break;} // d99424ac000000 fst dword ptr [esp + 0xac]
case 0x7a69abu:{pushfp(fp((r[5]+4u)));pc=0x7a69aeu;break;} // d94704 fld dword ptr [edi + 4]
case 0x7a69aeu:{st[0]=difference(st[0],fp((r[3]+4u)));pc=0x7a69b1u;break;} // d86204 fsub dword ptr [edx + 4]
case 0x7a69b1u:{fpwrite((r[7]+160u),st[0]);pc=0x7a69b8u;break;} // d99424a0000000 fst dword ptr [esp + 0xa0]
case 0x7a69b8u:{fpwrite((r[7]+12u),st[0]);pc=0x7a69bcu;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x7a69bcu:{pushfp(fp((r[5]+8u)));pc=0x7a69bfu;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a69bfu:{st[0]=difference(st[0],fp((r[3]+8u)));pc=0x7a69c2u;break;} // d86208 fsub dword ptr [edx + 8]
case 0x7a69c2u:{fpwrite((r[7]+168u),st[0]);pc=0x7a69c9u;break;} // d99424a8000000 fst dword ptr [esp + 0xa8]
case 0x7a69c9u:{pushfp(st[1]);pc=0x7a69cbu;break;} // d9c1 fld st(1)
case 0x7a69cbu:{st[2]=product(st[2],st[0]);popfp();pc=0x7a69cdu;break;} // deca fmulp st(2)
case 0x7a69cdu:{pushfp(st[2]);pc=0x7a69cfu;break;} // d9c2 fld st(2)
case 0x7a69cfu:{st[0]=product(st[0],st[3]);pc=0x7a69d1u;break;} // d8cb fmul st(3)
case 0x7a69d1u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a69d3u;break;} // dec2 faddp st(2)
case 0x7a69d3u:{pushfp(st[0]);pc=0x7a69d5u;break;} // d9c0 fld st(0)
case 0x7a69d5u:{st[0]=product(st[0],st[1]);pc=0x7a69d7u;break;} // d8c9 fmul st(1)
case 0x7a69d7u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a69d9u;break;} // dec2 faddp st(2)
case 0x7a69d9u:{std::swap(st[0],st[1]);pc=0x7a69dbu;break;} // d9c9 fxch st(1)
case 0x7a69dbu:{compare(st[0],st[3]);popfp();pc=0x7a69ddu;break;} // d8db fcomp st(3)
case 0x7a69ddu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a69dfu;break;} // dfe0 fnstsw ax
case 0x7a69dfu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a69e2u;break;} // f6c441 test ah, 0x41
case 0x7a69e2u:{pc=(!zf)?0x7a6a84u:0x7a69e8u;break;} // 0f859c000000 jne 0x7a6a84
case 0x7a69e8u:{pushfp(fp((r[7]+44u)));pc=0x7a69ecu;break;} // d944242c fld dword ptr [esp + 0x2c]
case 0x7a69ecu:{compare(st[0],fp((r[7]+120u)));popfp();pc=0x7a69f0u;break;} // d85c2478 fcomp dword ptr [esp + 0x78]
case 0x7a69f0u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a69f2u;break;} // dfe0 fnstsw ax
case 0x7a69f2u:{pushfp(fp((r[7]+12u)));pc=0x7a69f6u;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x7a69f6u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x7a69f9u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x7a69f9u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a69fcu;break;} // f6c405 test ah, 5
case 0x7a69fcu:{std::swap(st[0],st[2]);pc=0x7a69feu;break;} // d9ca fxch st(2)
case 0x7a69feu:{st[0]=product(st[0],fp((r[2])));pc=0x7a6a00u;break;} // d809 fmul dword ptr [ecx]
case 0x7a6a00u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6a02u;break;} // dec2 faddp st(2)
case 0x7a6a02u:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x7a6a05u;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x7a6a05u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6a07u;break;} // dec1 faddp st(1)
case 0x7a6a07u:{compare(st[0],st[1]);popfp();pc=0x7a6a09u;break;} // d8d9 fcomp st(1)
case 0x7a6a09u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6a0bu;break;} // dfe0 fnstsw ax
case 0x7a6a0bu:{pc=(pf)?0x7a6a3fu:0x7a6a0du;break;} // 7a32 jp 0x7a6a3f
case 0x7a6a0du:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6a10u;break;} // f6c441 test ah, 0x41
case 0x7a6a10u:{pc=(zf)?0x7a6a44u:0x7a6a12u;break;} // 7432 je 0x7a6a44
case 0x7a6a12u:{pushfp(fp((r[4])));pc=0x7a6a14u;break;} // d906 fld dword ptr [esi]
case 0x7a6a14u:{pushfp(fp((r[2])));pc=0x7a6a16u;break;} // d901 fld dword ptr [ecx]
case 0x7a6a16u:{pushfp(fp((r[2]+4u)));pc=0x7a6a19u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6a19u:{pushfp(fp((r[2]+8u)));pc=0x7a6a1cu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6a1cu:{std::swap(st[0],st[2]);pc=0x7a6a1eu;break;} // d9ca fxch st(2)
case 0x7a6a1eu:{st[0]=product(st[0],st[3]);pc=0x7a6a20u;break;} // d8cb fmul st(3)
case 0x7a6a20u:{std::swap(st[0],st[1]);pc=0x7a6a22u;break;} // d9c9 fxch st(1)
case 0x7a6a22u:{st[0]=product(st[0],st[3]);pc=0x7a6a24u;break;} // d8cb fmul st(3)
case 0x7a6a24u:{std::swap(st[0],st[2]);pc=0x7a6a26u;break;} // d9ca fxch st(2)
case 0x7a6a26u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6a28u;break;} // decb fmulp st(3)
case 0x7a6a28u:{st[0]=difference(fp((r[4]+4u)),st[0]);pc=0x7a6a2bu;break;} // d86e04 fsubr dword ptr [esi + 4]
case 0x7a6a2bu:{pushfp(fp((r[4]+8u)));pc=0x7a6a2eu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6a2eu:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a6a30u;break;} // dee2 fsubrp st(2)
case 0x7a6a30u:{pushfp(fp((r[4]+12u)));pc=0x7a6a33u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6a33u:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a6a35u;break;} // dee3 fsubrp st(3)
case 0x7a6a35u:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a6a39u;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a6a39u:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a6a3du;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a6a3du:{pc=(true)?0x7a6a99u:0x7a6a3fu;break;} // eb5a jmp 0x7a6a99
case 0x7a6a3fu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a6a42u;break;} // f6c405 test ah, 5
case 0x7a6a42u:{pc=(pf)?0x7a6a57u:0x7a6a44u;break;} // 7a13 jp 0x7a6a57
case 0x7a6a44u:{st[0]=st[0];popfp();pc=0x7a6a46u;break;} // ddd8 fstp st(0)
case 0x7a6a46u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6a47u;break;} // 5f pop edi
case 0x7a6a47u:{st[2]=st[0];popfp();pc=0x7a6a49u;break;} // ddda fstp st(2)
case 0x7a6a49u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6a4bu;break;} // 32c0 xor al, al
case 0x7a6a4bu:{st[0]=st[0];popfp();pc=0x7a6a4du;break;} // ddd8 fstp st(0)
case 0x7a6a4du:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6a4eu;break;} // 5e pop esi
case 0x7a6a4eu:{st[0]=st[0];popfp();pc=0x7a6a50u;break;} // ddd8 fstp st(0)
case 0x7a6a50u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6a56u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6a56u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6a57u:{pushfp(fp((r[4])));pc=0x7a6a59u;break;} // d906 fld dword ptr [esi]
case 0x7a6a59u:{pushfp(fp((r[2])));pc=0x7a6a5bu;break;} // d901 fld dword ptr [ecx]
case 0x7a6a5bu:{pushfp(fp((r[2]+4u)));pc=0x7a6a5eu;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6a5eu:{pushfp(fp((r[2]+8u)));pc=0x7a6a61u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6a61u:{std::swap(st[0],st[2]);pc=0x7a6a63u;break;} // d9ca fxch st(2)
case 0x7a6a63u:{st[0]=product(st[0],st[3]);pc=0x7a6a65u;break;} // d8cb fmul st(3)
case 0x7a6a65u:{std::swap(st[0],st[1]);pc=0x7a6a67u;break;} // d9c9 fxch st(1)
case 0x7a6a67u:{st[0]=product(st[0],st[3]);pc=0x7a6a69u;break;} // d8cb fmul st(3)
case 0x7a6a69u:{std::swap(st[0],st[2]);pc=0x7a6a6bu;break;} // d9ca fxch st(2)
case 0x7a6a6bu:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6a6du;break;} // decb fmulp st(3)
case 0x7a6a6du:{st[0]=sum(st[0],fp((r[4]+4u)));pc=0x7a6a70u;break;} // d84604 fadd dword ptr [esi + 4]
case 0x7a6a70u:{pushfp(fp((r[4]+8u)));pc=0x7a6a73u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6a73u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6a75u;break;} // dec2 faddp st(2)
case 0x7a6a75u:{pushfp(fp((r[4]+12u)));pc=0x7a6a78u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6a78u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a6a7au;break;} // dec3 faddp st(3)
case 0x7a6a7au:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a6a7eu;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a6a7eu:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a6a82u;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a6a82u:{pc=(true)?0x7a6a99u:0x7a6a84u;break;} // eb15 jmp 0x7a6a99
case 0x7a6a84u:{st[1]=st[0];popfp();pc=0x7a6a86u;break;} // ddd9 fstp st(1)
case 0x7a6a86u:{st[0]=st[0];popfp();pc=0x7a6a88u;break;} // ddd8 fstp st(0)
case 0x7a6a88u:{pushfp(fp((r[4]+4u)));pc=0x7a6a8bu;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6a8bu:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a6a8fu;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a6a8fu:{pushfp(fp((r[4]+8u)));pc=0x7a6a92u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6a92u:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x7a6a96u;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x7a6a96u:{pushfp(fp((r[4]+12u)));pc=0x7a6a99u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6a99u:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x7a6a9du;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x7a6a9du:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x7a6a9eu;break;} // 53 push ebx
case 0x7a6a9eu:{pushfp(fp((r[7]+168u)));pc=0x7a6aa5u;break;} // d98424a8000000 fld dword ptr [esp + 0xa8]
case 0x7a6aa5u:{st[0]=product(st[0],st[0]);pc=0x7a6aa7u;break;} // dcc8 fmul st(0), st(0)
case 0x7a6aa7u:{compare(st[0],fp((r[7]+180u)));popfp();pc=0x7a6aaeu;break;} // d89c24b4000000 fcomp dword ptr [esp + 0xb4]
case 0x7a6aaeu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6ab0u;break;} // dfe0 fnstsw ax
case 0x7a6ab0u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6ab3u;break;} // f6c441 test ah, 0x41
case 0x7a6ab3u:{pc=(pf)?0x7a6da0u:0x7a6ab9u;break;} // 0f8ae7020000 jp 0x7a6da0
case 0x7a6ab9u:{pushfp(fp((r[4]+8u)));pc=0x7a6abcu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6abcu:{st[0]=product(st[0],fp((r[7]+132u)));pc=0x7a6ac3u;break;} // d88c2484000000 fmul dword ptr [esp + 0x84]
case 0x7a6ac3u:{pushfp(fp((r[4]+4u)));pc=0x7a6ac6u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6ac6u:{pushfp(fp((r[7]+128u)));pc=0x7a6acdu;break;} // d9842480000000 fld dword ptr [esp + 0x80]
case 0x7a6acdu:{st[1]=product(st[1],st[0]);pc=0x7a6acfu;break;} // dcc9 fmul st(1), st(0)
case 0x7a6acfu:{std::swap(st[0],st[2]);pc=0x7a6ad1u;break;} // d9ca fxch st(2)
case 0x7a6ad1u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6ad3u;break;} // dec1 faddp st(1)
case 0x7a6ad3u:{pushfp(fp((r[4]+12u)));pc=0x7a6ad6u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6ad6u:{pushfp(fp((r[7]+136u)));pc=0x7a6addu;break;} // d9842488000000 fld dword ptr [esp + 0x88]
case 0x7a6addu:{st[1]=product(st[1],st[0]);pc=0x7a6adfu;break;} // dcc9 fmul st(1), st(0)
case 0x7a6adfu:{std::swap(st[0],st[2]);pc=0x7a6ae1u;break;} // d9ca fxch st(2)
case 0x7a6ae1u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6ae3u;break;} // dec1 faddp st(1)
case 0x7a6ae3u:{std::swap(st[0],st[2]);pc=0x7a6ae5u;break;} // d9ca fxch st(2)
case 0x7a6ae5u:{st[0]=product(st[0],st[6]);pc=0x7a6ae7u;break;} // d8ce fmul st(6)
case 0x7a6ae7u:{pushfp(fp((r[7]+132u)));pc=0x7a6aeeu;break;} // d9842484000000 fld dword ptr [esp + 0x84]
case 0x7a6aeeu:{st[0]=product(st[0],st[6]);pc=0x7a6af0u;break;} // d8ce fmul st(6)
case 0x7a6af0u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6af2u;break;} // dec1 faddp st(1)
case 0x7a6af2u:{std::swap(st[0],st[1]);pc=0x7a6af4u;break;} // d9c9 fxch st(1)
case 0x7a6af4u:{st[0]=product(st[0],fp((r[7]+96u)));pc=0x7a6af8u;break;} // d84c2460 fmul dword ptr [esp + 0x60]
case 0x7a6af8u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6afau;break;} // dec1 faddp st(1)
case 0x7a6afau:{compare(st[0],st[1]);popfp();popfp();pc=0x7a6afcu;break;} // ded9 fcompp
case 0x7a6afcu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6afeu;break;} // dfe0 fnstsw ax
case 0x7a6afeu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6b01u;break;} // f6c441 test ah, 0x41
case 0x7a6b01u:{pc=(pf)?0x7a6b0au:0x7a6b03u;break;} // 7a07 jp 0x7a6b0a
case 0x7a6b03u:{r[0]=1u;pc=0x7a6b08u;break;} // b801000000 mov eax, 1
case 0x7a6b08u:{pc=(true)?0x7a6b0cu:0x7a6b0au;break;} // eb02 jmp 0x7a6b0c
case 0x7a6b0au:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6b0cu;break;} // 33c0 xor eax, eax
case 0x7a6b0cu:{pushfp(fp((r[4]+8u)));pc=0x7a6b0fu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6b0fu:{r[1]=(r[1]&4294967040u)|(((r[0])&255u<<0)&255u);pc=0x7a6b11u;break;} // 8ad8 mov bl, al
case 0x7a6b11u:{pushfp(fp((r[7]+144u)));pc=0x7a6b18u;break;} // d9842490000000 fld dword ptr [esp + 0x90]
case 0x7a6b18u:{st[1]=product(st[1],st[0]);pc=0x7a6b1au;break;} // dcc9 fmul st(1), st(0)
case 0x7a6b1au:{pushfp(fp((r[4]+4u)));pc=0x7a6b1du;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6b1du:{pushfp(fp((r[7]+140u)));pc=0x7a6b24u;break;} // d984248c000000 fld dword ptr [esp + 0x8c]
case 0x7a6b24u:{st[1]=product(st[1],st[0]);pc=0x7a6b26u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6b26u:{std::swap(st[0],st[3]);pc=0x7a6b28u;break;} // d9cb fxch st(3)
case 0x7a6b28u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b2au;break;} // dec1 faddp st(1)
case 0x7a6b2au:{pushfp(fp((r[4]+12u)));pc=0x7a6b2du;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6b2du:{st[0]=product(st[0],fp((r[7]+148u)));pc=0x7a6b34u;break;} // d88c2494000000 fmul dword ptr [esp + 0x94]
case 0x7a6b34u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b36u;break;} // dec1 faddp st(1)
case 0x7a6b36u:{std::swap(st[0],st[2]);pc=0x7a6b38u;break;} // d9ca fxch st(2)
case 0x7a6b38u:{st[0]=product(st[0],fp((r[7]+100u)));pc=0x7a6b3cu;break;} // d84c2464 fmul dword ptr [esp + 0x64]
case 0x7a6b3cu:{std::swap(st[0],st[1]);pc=0x7a6b3eu;break;} // d9c9 fxch st(1)
case 0x7a6b3eu:{st[0]=product(st[0],fp((r[7]+104u)));pc=0x7a6b42u;break;} // d84c2468 fmul dword ptr [esp + 0x68]
case 0x7a6b42u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b44u;break;} // dec1 faddp st(1)
case 0x7a6b44u:{pushfp(fp((r[7]+148u)));pc=0x7a6b4bu;break;} // d9842494000000 fld dword ptr [esp + 0x94]
case 0x7a6b4bu:{st[0]=product(st[0],fp((r[7]+108u)));pc=0x7a6b4fu;break;} // d84c246c fmul dword ptr [esp + 0x6c]
case 0x7a6b4fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b51u;break;} // dec1 faddp st(1)
case 0x7a6b51u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a6b53u;break;} // ded9 fcompp
case 0x7a6b53u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6b55u;break;} // dfe0 fnstsw ax
case 0x7a6b55u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6b58u;break;} // f6c441 test ah, 0x41
case 0x7a6b58u:{pc=(pf)?0x7a6b61u:0x7a6b5au;break;} // 7a07 jp 0x7a6b61
case 0x7a6b5au:{r[0]=1u;pc=0x7a6b5fu;break;} // b801000000 mov eax, 1
case 0x7a6b5fu:{pc=(true)?0x7a6b63u:0x7a6b61u;break;} // eb02 jmp 0x7a6b63
case 0x7a6b61u:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6b63u;break;} // 33c0 xor eax, eax
case 0x7a6b63u:{pushfp(fp((r[4]+8u)));pc=0x7a6b66u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6b66u:{r[2]=(r[2]&4294967040u)|(((r[0])&255u<<0)&255u);pc=0x7a6b68u;break;} // 8ac8 mov cl, al
case 0x7a6b68u:{pushfp(fp((r[7]+156u)));pc=0x7a6b6fu;break;} // d984249c000000 fld dword ptr [esp + 0x9c]
case 0x7a6b6fu:{st[1]=product(st[1],st[0]);pc=0x7a6b71u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6b71u:{pushfp(fp((r[4]+4u)));pc=0x7a6b74u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a6b74u:{pushfp(fp((r[7]+152u)));pc=0x7a6b7bu;break;} // d9842498000000 fld dword ptr [esp + 0x98]
case 0x7a6b7bu:{st[1]=product(st[1],st[0]);pc=0x7a6b7du;break;} // dcc9 fmul st(1), st(0)
case 0x7a6b7du:{std::swap(st[0],st[3]);pc=0x7a6b7fu;break;} // d9cb fxch st(3)
case 0x7a6b7fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b81u;break;} // dec1 faddp st(1)
case 0x7a6b81u:{pushfp(fp((r[4]+12u)));pc=0x7a6b84u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6b84u:{st[0]=product(st[0],fp((r[7]+160u)));pc=0x7a6b8bu;break;} // d88c24a0000000 fmul dword ptr [esp + 0xa0]
case 0x7a6b8bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b8du;break;} // dec1 faddp st(1)
case 0x7a6b8du:{std::swap(st[0],st[2]);pc=0x7a6b8fu;break;} // d9ca fxch st(2)
case 0x7a6b8fu:{st[0]=product(st[0],fp((r[7]+112u)));pc=0x7a6b93u;break;} // d84c2470 fmul dword ptr [esp + 0x70]
case 0x7a6b93u:{std::swap(st[0],st[1]);pc=0x7a6b95u;break;} // d9c9 fxch st(1)
case 0x7a6b95u:{st[0]=product(st[0],fp((r[7]+116u)));pc=0x7a6b99u;break;} // d84c2474 fmul dword ptr [esp + 0x74]
case 0x7a6b99u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6b9bu;break;} // dec1 faddp st(1)
case 0x7a6b9bu:{pushfp(fp((r[7]+160u)));pc=0x7a6ba2u;break;} // d98424a0000000 fld dword ptr [esp + 0xa0]
case 0x7a6ba2u:{pushfp(fp((r[7]+120u)));pc=0x7a6ba6u;break;} // d9442478 fld dword ptr [esp + 0x78]
case 0x7a6ba6u:{st[1]=product(st[1],st[0]);pc=0x7a6ba8u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6ba8u:{std::swap(st[0],st[2]);pc=0x7a6baau;break;} // d9ca fxch st(2)
case 0x7a6baau:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6bacu;break;} // dec1 faddp st(1)
case 0x7a6bacu:{compare(st[0],st[2]);popfp();pc=0x7a6baeu;break;} // d8da fcomp st(2)
case 0x7a6baeu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6bb0u;break;} // dfe0 fnstsw ax
case 0x7a6bb0u:{st[1]=st[0];popfp();pc=0x7a6bb2u;break;} // ddd9 fstp st(1)
case 0x7a6bb2u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6bb5u;break;} // f6c441 test ah, 0x41
case 0x7a6bb5u:{pc=(pf)?0x7a6bbeu:0x7a6bb7u;break;} // 7a07 jp 0x7a6bbe
case 0x7a6bb7u:{r[0]=1u;pc=0x7a6bbcu;break;} // b801000000 mov eax, 1
case 0x7a6bbcu:{pc=(true)?0x7a6bc0u:0x7a6bbeu;break;} // eb02 jmp 0x7a6bc0
case 0x7a6bbeu:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6bc0u;break;} // 33c0 xor eax, eax
case 0x7a6bc0u:{flags(((r[1])&255u)&((r[1])&255u));cf=false;pc=0x7a6bc2u;break;} // 84db test bl, bl
case 0x7a6bc2u:{pc=(zf)?0x7a6cc2u:0x7a6bc8u;break;} // 0f84fa000000 je 0x7a6cc2
case 0x7a6bc8u:{st[4]=st[0];popfp();pc=0x7a6bcau;break;} // dddc fstp st(4)
case 0x7a6bcau:{flags(((r[2])&255u)&((r[2])&255u));cf=false;pc=0x7a6bccu;break;} // 84c9 test cl, cl
case 0x7a6bccu:{r[2]=get((r[7]+212u),4);pc=0x7a6bd3u;break;} // 8b8c24d4000000 mov ecx, dword ptr [esp + 0xd4]
case 0x7a6bd3u:{st[2]=st[0];popfp();pc=0x7a6bd5u;break;} // ddda fstp st(2)
case 0x7a6bd5u:{pc=(zf)?0x7a6c42u:0x7a6bd7u;break;} // 746b je 0x7a6c42
case 0x7a6bd7u:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a6bd9u;break;} // 84c0 test al, al
case 0x7a6bd9u:{pc=(zf)?0x7a6c15u:0x7a6bdbu;break;} // 743a je 0x7a6c15
case 0x7a6bdbu:{r[0]=get((r[7]+204u),4);pc=0x7a6be2u;break;} // 8b8424cc000000 mov eax, dword ptr [esp + 0xcc]
case 0x7a6be2u:{st[2]=st[0];popfp();pc=0x7a6be4u;break;} // ddda fstp st(2)
case 0x7a6be4u:{st[1]=st[0];popfp();pc=0x7a6be6u;break;} // ddd9 fstp st(1)
case 0x7a6be6u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a6be7u;break;} // 5b pop ebx
case 0x7a6be7u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a6be9u;break;} // d918 fstp dword ptr [eax]
case 0x7a6be9u:{r[0]=get((r[7]+212u),4);pc=0x7a6bf0u;break;} // 8b8424d4000000 mov eax, dword ptr [esp + 0xd4]
case 0x7a6bf0u:{pushfp(fp((r[7]+8u)));pc=0x7a6bf4u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a6bf4u:{put((r[2]),1u,4);pc=0x7a6bfau;break;} // c70101000000 mov dword ptr [ecx], 1
case 0x7a6bfau:{fpwrite((r[0]),st[0]);popfp();pc=0x7a6bfcu;break;} // d918 fstp dword ptr [eax]
case 0x7a6bfcu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6bfdu;break;} // 5f pop edi
case 0x7a6bfdu:{pushfp(fp((r[7]+8u)));pc=0x7a6c01u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a6c01u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6c02u;break;} // 5e pop esi
case 0x7a6c02u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a6c05u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a6c05u:{pushfp(fp((r[7]+8u)));pc=0x7a6c09u;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a6c09u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a6c0cu;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a6c0cu:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a6c0eu;break;} // b001 mov al, 1
case 0x7a6c0eu:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6c14u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6c14u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6c15u:{st[1]=st[0];popfp();pc=0x7a6c17u;break;} // ddd9 fstp st(1)
case 0x7a6c17u:{pushfp(fp((r[7]+112u)));pc=0x7a6c1bu;break;} // d9442470 fld dword ptr [esp + 0x70]
case 0x7a6c1bu:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a6c1fu;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a6c1fu:{pushfp(fp((r[7]+116u)));pc=0x7a6c23u;break;} // d9442474 fld dword ptr [esp + 0x74]
case 0x7a6c23u:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a6c27u;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a6c27u:{std::swap(st[0],st[1]);pc=0x7a6c29u;break;} // d9c9 fxch st(1)
case 0x7a6c29u:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a6c2du;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a6c2du:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a6c31u;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a6c31u:{pushfp(fp((r[7]+80u)));pc=0x7a6c35u;break;} // d9442450 fld dword ptr [esp + 0x50]
case 0x7a6c35u:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a6c39u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a6c39u:{pushfp(fp((r[7]+84u)));pc=0x7a6c3du;break;} // d9442454 fld dword ptr [esp + 0x54]
case 0x7a6c3du:{pc=(true)?0x7a712fu:0x7a6c42u;break;} // e9ed040000 jmp 0x7a712f
case 0x7a6c42u:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a6c44u;break;} // 84c0 test al, al
case 0x7a6c44u:{st[2]=st[0];popfp();pc=0x7a6c46u;break;} // ddda fstp st(2)
case 0x7a6c46u:{r[0]=get((r[7]+216u),4);pc=0x7a6c4du;break;} // 8b8424d8000000 mov eax, dword ptr [esp + 0xd8]
case 0x7a6c4du:{st[0]=st[0];popfp();pc=0x7a6c4fu;break;} // ddd8 fstp st(0)
case 0x7a6c4fu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6c50u;break;} // 50 push eax
case 0x7a6c50u:{st[0]=st[0];popfp();pc=0x7a6c52u;break;} // ddd8 fstp st(0)
case 0x7a6c52u:{r[0]=get((r[7]+208u),4);pc=0x7a6c59u;break;} // 8b8424d0000000 mov eax, dword ptr [esp + 0xd0]
case 0x7a6c59u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6c5au;break;} // 51 push ecx
case 0x7a6c5au:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6c5bu;break;} // 51 push ecx
case 0x7a6c5bu:{pc=(zf)?0x7a6c8eu:0x7a6c5du;break;} // 7431 je 0x7a6c8e
case 0x7a6c5du:{pushfp(fp((r[7]+112u)));pc=0x7a6c61u;break;} // d9442470 fld dword ptr [esp + 0x70]
case 0x7a6c61u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a6c65u;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a6c65u:{pushfp(fp((r[7]+116u)));pc=0x7a6c69u;break;} // d9442474 fld dword ptr [esp + 0x74]
case 0x7a6c69u:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a6c6du;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a6c6du:{pushfp(fp((r[7]+120u)));pc=0x7a6c71u;break;} // d9442478 fld dword ptr [esp + 0x78]
case 0x7a6c71u:{fpwrite((r[7]+44u),st[0]);popfp();pc=0x7a6c75u;break;} // d95c242c fstp dword ptr [esp + 0x2c]
case 0x7a6c75u:{pushfp(fp((r[7]+76u)));pc=0x7a6c79u;break;} // d944244c fld dword ptr [esp + 0x4c]
case 0x7a6c79u:{fpwrite((r[7]+48u),st[0]);popfp();pc=0x7a6c7du;break;} // d95c2430 fstp dword ptr [esp + 0x30]
case 0x7a6c7du:{pushfp(fp((r[7]+80u)));pc=0x7a6c81u;break;} // d9442450 fld dword ptr [esp + 0x50]
case 0x7a6c81u:{fpwrite((r[7]+52u),st[0]);popfp();pc=0x7a6c85u;break;} // d95c2434 fstp dword ptr [esp + 0x34]
case 0x7a6c85u:{pushfp(fp((r[7]+84u)));pc=0x7a6c89u;break;} // d9442454 fld dword ptr [esp + 0x54]
case 0x7a6c89u:{pc=(true)?0x7a7140u:0x7a6c8eu;break;} // e9b2040000 jmp 0x7a7140
case 0x7a6c8eu:{pushfp(fp((r[7]+220u)));pc=0x7a6c95u;break;} // d98424dc000000 fld dword ptr [esp + 0xdc]
case 0x7a6c95u:{r[2]=(r[7]+100u);pc=0x7a6c99u;break;} // 8d4c2464 lea ecx, [esp + 0x64]
case 0x7a6c99u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a6c9cu;break;} // d91c24 fstp dword ptr [esp]
case 0x7a6c9cu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6c9du;break;} // 50 push eax
case 0x7a6c9du:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a6c9eu;break;} // 57 push edi
case 0x7a6c9eu:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a6c9fu;break;} // 52 push edx
case 0x7a6c9fu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6ca0u;break;} // 51 push ecx
case 0x7a6ca0u:{r[1]=(r[7]+104u);pc=0x7a6ca4u;break;} // 8d5c2468 lea ebx, [esp + 0x68]
case 0x7a6ca4u:{r[3]=(r[7]+180u);pc=0x7a6cabu;break;} // 8d9424b4000000 lea edx, [esp + 0xb4]
case 0x7a6cabu:{r[0]=2u;pc=0x7a6cb0u;break;} // b802000000 mov eax, 2
case 0x7a6cb0u:{r[7]-=4;put(r[7],0x7a6cb5u,4);pc=0x7a63c0u;break;} // e80bf7ffff call 0x7a63c0
case 0x7a6cb5u:{uint32_t v=(r[7])+(28u);r[7]=v;flags(v);pc=0x7a6cb8u;break;} // 83c41c add esp, 0x1c
case 0x7a6cb8u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a6cb9u;break;} // 5b pop ebx
case 0x7a6cb9u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6cbau;break;} // 5f pop edi
case 0x7a6cbau:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6cbbu;break;} // 5e pop esi
case 0x7a6cbbu:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6cc1u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6cc1u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6cc2u:{st[0]=st[0];popfp();pc=0x7a6cc4u;break;} // ddd8 fstp st(0)
case 0x7a6cc4u:{st[0]=st[0];popfp();pc=0x7a6cc6u;break;} // ddd8 fstp st(0)
case 0x7a6cc6u:{st[0]=st[0];popfp();pc=0x7a6cc8u;break;} // ddd8 fstp st(0)
case 0x7a6cc8u:{flags(((r[2])&255u)&((r[2])&255u));cf=false;pc=0x7a6ccau;break;} // 84c9 test cl, cl
case 0x7a6ccau:{pc=(zf)?0x7a6d48u:0x7a6cccu;break;} // 747c je 0x7a6d48
case 0x7a6cccu:{r[2]=get((r[7]+212u),4);pc=0x7a6cd3u;break;} // 8b8c24d4000000 mov ecx, dword ptr [esp + 0xd4]
case 0x7a6cd3u:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a6cd5u;break;} // 84c0 test al, al
case 0x7a6cd5u:{r[0]=get((r[7]+216u),4);pc=0x7a6cdcu;break;} // 8b8424d8000000 mov eax, dword ptr [esp + 0xd8]
case 0x7a6cdcu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6cddu;break;} // 50 push eax
case 0x7a6cddu:{r[0]=get((r[7]+208u),4);pc=0x7a6ce4u;break;} // 8b8424d0000000 mov eax, dword ptr [esp + 0xd0]
case 0x7a6ce4u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6ce5u;break;} // 51 push ecx
case 0x7a6ce5u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6ce6u;break;} // 51 push ecx
case 0x7a6ce6u:{pc=(zf)?0x7a6d13u:0x7a6ce8u;break;} // 742b je 0x7a6d13
case 0x7a6ce8u:{std::swap(st[0],st[1]);pc=0x7a6ceau;break;} // d9c9 fxch st(1)
case 0x7a6ceau:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a6ceeu;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a6ceeu:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a6cf2u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a6cf2u:{pushfp(fp((r[7]+108u)));pc=0x7a6cf6u;break;} // d944246c fld dword ptr [esp + 0x6c]
case 0x7a6cf6u:{fpwrite((r[7]+44u),st[0]);popfp();pc=0x7a6cfau;break;} // d95c242c fstp dword ptr [esp + 0x2c]
case 0x7a6cfau:{pushfp(fp((r[7]+64u)));pc=0x7a6cfeu;break;} // d9442440 fld dword ptr [esp + 0x40]
case 0x7a6cfeu:{fpwrite((r[7]+48u),st[0]);popfp();pc=0x7a6d02u;break;} // d95c2430 fstp dword ptr [esp + 0x30]
case 0x7a6d02u:{pushfp(fp((r[7]+68u)));pc=0x7a6d06u;break;} // d9442444 fld dword ptr [esp + 0x44]
case 0x7a6d06u:{fpwrite((r[7]+52u),st[0]);popfp();pc=0x7a6d0au;break;} // d95c2434 fstp dword ptr [esp + 0x34]
case 0x7a6d0au:{pushfp(fp((r[7]+72u)));pc=0x7a6d0eu;break;} // d9442448 fld dword ptr [esp + 0x48]
case 0x7a6d0eu:{pc=(true)?0x7a7140u:0x7a6d13u;break;} // e92d040000 jmp 0x7a7140
case 0x7a6d13u:{st[1]=st[0];popfp();pc=0x7a6d15u;break;} // ddd9 fstp st(1)
case 0x7a6d15u:{r[2]=(r[7]+100u);pc=0x7a6d19u;break;} // 8d4c2464 lea ecx, [esp + 0x64]
case 0x7a6d19u:{st[0]=st[0];popfp();pc=0x7a6d1bu;break;} // ddd8 fstp st(0)
case 0x7a6d1bu:{r[1]=(r[7]+64u);pc=0x7a6d1fu;break;} // 8d5c2440 lea ebx, [esp + 0x40]
case 0x7a6d1fu:{pushfp(fp((r[7]+220u)));pc=0x7a6d26u;break;} // d98424dc000000 fld dword ptr [esp + 0xdc]
case 0x7a6d26u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a6d29u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a6d29u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6d2au;break;} // 50 push eax
case 0x7a6d2au:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a6d2bu;break;} // 57 push edi
case 0x7a6d2bu:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a6d2cu;break;} // 52 push edx
case 0x7a6d2cu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6d2du;break;} // 51 push ecx
case 0x7a6d2du:{r[3]=(r[7]+156u);pc=0x7a6d34u;break;} // 8d94249c000000 lea edx, [esp + 0x9c]
case 0x7a6d34u:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6d36u;break;} // 33c0 xor eax, eax
case 0x7a6d36u:{r[7]-=4;put(r[7],0x7a6d3bu,4);pc=0x7a63c0u;break;} // e885f6ffff call 0x7a63c0
case 0x7a6d3bu:{uint32_t v=(r[7])+(28u);r[7]=v;flags(v);pc=0x7a6d3eu;break;} // 83c41c add esp, 0x1c
case 0x7a6d3eu:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a6d3fu;break;} // 5b pop ebx
case 0x7a6d3fu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6d40u;break;} // 5f pop edi
case 0x7a6d40u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6d41u;break;} // 5e pop esi
case 0x7a6d41u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6d47u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6d47u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6d48u:{st[1]=st[0];popfp();pc=0x7a6d4au;break;} // ddd9 fstp st(1)
case 0x7a6d4au:{st[0]=st[0];popfp();pc=0x7a6d4cu;break;} // ddd8 fstp st(0)
case 0x7a6d4cu:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a6d4eu;break;} // 84c0 test al, al
case 0x7a6d4eu:{pc=(zf)?0x7a6df5u:0x7a6d54u;break;} // 0f84a1000000 je 0x7a6df5
case 0x7a6d54u:{r[0]=get((r[7]+216u),4);pc=0x7a6d5bu;break;} // 8b8424d8000000 mov eax, dword ptr [esp + 0xd8]
case 0x7a6d5bu:{pushfp(fp((r[7]+208u)));pc=0x7a6d62u;break;} // d98424d0000000 fld dword ptr [esp + 0xd0]
case 0x7a6d62u:{r[2]=get((r[7]+212u),4);pc=0x7a6d69u;break;} // 8b8c24d4000000 mov ecx, dword ptr [esp + 0xd4]
case 0x7a6d69u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6d6au;break;} // 50 push eax
case 0x7a6d6au:{r[0]=get((r[7]+208u),4);pc=0x7a6d71u;break;} // 8b8424d0000000 mov eax, dword ptr [esp + 0xd0]
case 0x7a6d71u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6d72u;break;} // 51 push ecx
case 0x7a6d72u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6d73u;break;} // 51 push ecx
case 0x7a6d73u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a6d76u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a6d76u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a6d77u;break;} // 50 push eax
case 0x7a6d77u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a6d78u;break;} // 57 push edi
case 0x7a6d78u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a6d79u;break;} // 52 push edx
case 0x7a6d79u:{r[2]=(r[7]+112u);pc=0x7a6d7du;break;} // 8d4c2470 lea ecx, [esp + 0x70]
case 0x7a6d7du:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a6d7eu;break;} // 51 push ecx
case 0x7a6d7eu:{r[1]=(r[7]+92u);pc=0x7a6d82u;break;} // 8d5c245c lea ebx, [esp + 0x5c]
case 0x7a6d82u:{r[3]=(r[7]+168u);pc=0x7a6d89u;break;} // 8d9424a8000000 lea edx, [esp + 0xa8]
case 0x7a6d89u:{r[0]=1u;pc=0x7a6d8eu;break;} // b801000000 mov eax, 1
case 0x7a6d8eu:{r[7]-=4;put(r[7],0x7a6d93u,4);pc=0x7a63c0u;break;} // e82df6ffff call 0x7a63c0
case 0x7a6d93u:{uint32_t v=(r[7])+(28u);r[7]=v;flags(v);pc=0x7a6d96u;break;} // 83c41c add esp, 0x1c
case 0x7a6d96u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a6d97u;break;} // 5b pop ebx
case 0x7a6d97u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6d98u;break;} // 5f pop edi
case 0x7a6d98u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6d99u;break;} // 5e pop esi
case 0x7a6d99u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6d9fu;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6d9fu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6da0u:{st[1]=st[0];popfp();pc=0x7a6da2u;break;} // ddd9 fstp st(1)
case 0x7a6da2u:{pushfp(fp((r[7]+176u)));pc=0x7a6da9u;break;} // d98424b0000000 fld dword ptr [esp + 0xb0]
case 0x7a6da9u:{fpwrite((r[7]+12u),st[0]);pc=0x7a6dadu;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x7a6dadu:{pushfp(fp((r[7]+164u)));pc=0x7a6db4u;break;} // d98424a4000000 fld dword ptr [esp + 0xa4]
case 0x7a6db4u:{fpwrite((r[7]+16u),st[0]);pc=0x7a6db8u;break;} // d9542410 fst dword ptr [esp + 0x10]
case 0x7a6db8u:{pushfp(fp((r[7]+172u)));pc=0x7a6dbfu;break;} // d98424ac000000 fld dword ptr [esp + 0xac]
case 0x7a6dbfu:{fpwrite((r[7]+20u),st[0]);pc=0x7a6dc3u;break;} // d9542414 fst dword ptr [esp + 0x14]
case 0x7a6dc3u:{pushfp(fp((r[7]+48u)));pc=0x7a6dc7u;break;} // d9442430 fld dword ptr [esp + 0x30]
case 0x7a6dc7u:{compare(st[0],fp((r[7]+124u)));popfp();pc=0x7a6dcbu;break;} // d85c247c fcomp dword ptr [esp + 0x7c]
case 0x7a6dcbu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6dcdu;break;} // dfe0 fnstsw ax
case 0x7a6dcdu:{std::swap(st[0],st[2]);pc=0x7a6dcfu;break;} // d9ca fxch st(2)
case 0x7a6dcfu:{st[0]=product(st[0],fp((r[2])));pc=0x7a6dd1u;break;} // d809 fmul dword ptr [ecx]
case 0x7a6dd1u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a6dd4u;break;} // f6c405 test ah, 5
case 0x7a6dd4u:{fpwrite((r[7]+48u),st[0]);pc=0x7a6dd8u;break;} // d9542430 fst dword ptr [esp + 0x30]
case 0x7a6dd8u:{std::swap(st[0],st[1]);pc=0x7a6ddau;break;} // d9c9 fxch st(1)
case 0x7a6ddau:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x7a6dddu;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x7a6dddu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6ddfu;break;} // dec1 faddp st(1)
case 0x7a6ddfu:{pushfp(fp((r[2]+8u)));pc=0x7a6de2u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6de2u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a6de4u;break;} // deca fmulp st(2)
case 0x7a6de4u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6de6u;break;} // dec1 faddp st(1)
case 0x7a6de6u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a6de8u;break;} // ded9 fcompp
case 0x7a6de8u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6deau;break;} // dfe0 fnstsw ax
case 0x7a6deau:{pc=(pf)?0x7a6e26u:0x7a6decu;break;} // 7a3a jp 0x7a6e26
case 0x7a6decu:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a6defu;break;} // f6c401 test ah, 1
case 0x7a6defu:{pc=(!zf)?0x7a6e01u:0x7a6df1u;break;} // 7510 jne 0x7a6e01
case 0x7a6df1u:{st[1]=st[0];popfp();pc=0x7a6df3u;break;} // ddd9 fstp st(1)
case 0x7a6df3u:{st[0]=st[0];popfp();pc=0x7a6df5u;break;} // ddd8 fstp st(0)
case 0x7a6df5u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a6df6u;break;} // 5b pop ebx
case 0x7a6df6u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a6df7u;break;} // 5f pop edi
case 0x7a6df7u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a6df9u;break;} // 32c0 xor al, al
case 0x7a6df9u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a6dfau;break;} // 5e pop esi
case 0x7a6dfau:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a6e00u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a6e00u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a6e01u:{pushfp(fp((r[4])));pc=0x7a6e03u;break;} // d906 fld dword ptr [esi]
case 0x7a6e03u:{pushfp(fp((r[2])));pc=0x7a6e05u;break;} // d901 fld dword ptr [ecx]
case 0x7a6e05u:{pushfp(fp((r[2]+4u)));pc=0x7a6e08u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6e08u:{pushfp(fp((r[2]+8u)));pc=0x7a6e0bu;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6e0bu:{std::swap(st[0],st[2]);pc=0x7a6e0du;break;} // d9ca fxch st(2)
case 0x7a6e0du:{st[0]=product(st[0],st[3]);pc=0x7a6e0fu;break;} // d8cb fmul st(3)
case 0x7a6e0fu:{std::swap(st[0],st[1]);pc=0x7a6e11u;break;} // d9c9 fxch st(1)
case 0x7a6e11u:{st[0]=product(st[0],st[3]);pc=0x7a6e13u;break;} // d8cb fmul st(3)
case 0x7a6e13u:{std::swap(st[0],st[2]);pc=0x7a6e15u;break;} // d9ca fxch st(2)
case 0x7a6e15u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6e17u;break;} // decb fmulp st(3)
case 0x7a6e17u:{st[0]=difference(fp((r[4]+4u)),st[0]);pc=0x7a6e1au;break;} // d86e04 fsubr dword ptr [esi + 4]
case 0x7a6e1au:{pushfp(fp((r[4]+8u)));pc=0x7a6e1du;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6e1du:{st[2]=difference(st[0],st[2]);popfp();pc=0x7a6e1fu;break;} // dee2 fsubrp st(2)
case 0x7a6e1fu:{pushfp(fp((r[4]+12u)));pc=0x7a6e22u;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6e22u:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a6e24u;break;} // dee3 fsubrp st(3)
case 0x7a6e24u:{pc=(true)?0x7a6e4eu:0x7a6e26u;break;} // eb28 jmp 0x7a6e4e
case 0x7a6e26u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6e29u;break;} // f6c441 test ah, 0x41
case 0x7a6e29u:{pc=(!pf)?0x7a6df1u:0x7a6e2bu;break;} // 7bc6 jnp 0x7a6df1
case 0x7a6e2bu:{pushfp(fp((r[4])));pc=0x7a6e2du;break;} // d906 fld dword ptr [esi]
case 0x7a6e2du:{pushfp(fp((r[2])));pc=0x7a6e2fu;break;} // d901 fld dword ptr [ecx]
case 0x7a6e2fu:{pushfp(fp((r[2]+4u)));pc=0x7a6e32u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x7a6e32u:{pushfp(fp((r[2]+8u)));pc=0x7a6e35u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6e35u:{std::swap(st[0],st[2]);pc=0x7a6e37u;break;} // d9ca fxch st(2)
case 0x7a6e37u:{st[0]=product(st[0],st[3]);pc=0x7a6e39u;break;} // d8cb fmul st(3)
case 0x7a6e39u:{std::swap(st[0],st[1]);pc=0x7a6e3bu;break;} // d9c9 fxch st(1)
case 0x7a6e3bu:{st[0]=product(st[0],st[3]);pc=0x7a6e3du;break;} // d8cb fmul st(3)
case 0x7a6e3du:{std::swap(st[0],st[2]);pc=0x7a6e3fu;break;} // d9ca fxch st(2)
case 0x7a6e3fu:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6e41u;break;} // decb fmulp st(3)
case 0x7a6e41u:{st[0]=sum(st[0],fp((r[4]+4u)));pc=0x7a6e44u;break;} // d84604 fadd dword ptr [esi + 4]
case 0x7a6e44u:{pushfp(fp((r[4]+8u)));pc=0x7a6e47u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a6e47u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6e49u;break;} // dec2 faddp st(2)
case 0x7a6e49u:{pushfp(fp((r[4]+12u)));pc=0x7a6e4cu;break;} // d9460c fld dword ptr [esi + 0xc]
case 0x7a6e4cu:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a6e4eu;break;} // dec3 faddp st(3)
case 0x7a6e4eu:{std::swap(st[0],st[2]);pc=0x7a6e50u;break;} // d9ca fxch st(2)
case 0x7a6e50u:{fpwrite((r[7]+32u),st[0]);pc=0x7a6e54u;break;} // d9542420 fst dword ptr [esp + 0x20]
case 0x7a6e54u:{pushfp(st[3]);pc=0x7a6e56u;break;} // d9c3 fld st(3)
case 0x7a6e56u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x7a6e59u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x7a6e59u:{pushfp(st[5]);pc=0x7a6e5bu;break;} // d9c5 fld st(5)
case 0x7a6e5bu:{st[0]=product(st[0],fp((r[2])));pc=0x7a6e5du;break;} // d809 fmul dword ptr [ecx]
case 0x7a6e5du:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6e5fu;break;} // dec1 faddp st(1)
case 0x7a6e5fu:{pushfp(fp((r[2]+8u)));pc=0x7a6e62u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6e62u:{st[0]=product(st[0],fp((r[7]+96u)));pc=0x7a6e66u;break;} // d84c2460 fmul dword ptr [esp + 0x60]
case 0x7a6e66u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6e68u;break;} // dec1 faddp st(1)
case 0x7a6e68u:{pushfp(st[2]);pc=0x7a6e6au;break;} // d9c2 fld st(2)
case 0x7a6e6au:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x7a6e6du;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x7a6e6du:{pushfp(st[4]);pc=0x7a6e6fu;break;} // d9c4 fld st(4)
case 0x7a6e6fu:{st[0]=product(st[0],fp((r[2])));pc=0x7a6e71u;break;} // d809 fmul dword ptr [ecx]
case 0x7a6e71u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6e73u;break;} // dec1 faddp st(1)
case 0x7a6e73u:{pushfp(fp((r[2]+8u)));pc=0x7a6e76u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6e76u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a6e78u;break;} // decb fmulp st(3)
case 0x7a6e78u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6e7au;break;} // dec2 faddp st(2)
case 0x7a6e7au:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a6e7cu;break;} // dee1 fsubrp st(1)
case 0x7a6e7cu:{pushfp(fp((r[7]+16u)));pc=0x7a6e80u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a6e80u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x7a6e83u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x7a6e83u:{st[0]=sum(st[0],fp((r[7]+48u)));pc=0x7a6e87u;break;} // d8442430 fadd dword ptr [esp + 0x30]
case 0x7a6e87u:{pushfp(fp((r[2]+8u)));pc=0x7a6e8au;break;} // d94108 fld dword ptr [ecx + 8]
case 0x7a6e8au:{pushfp(fp((r[7]+20u)));pc=0x7a6e8eu;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x7a6e8eu:{st[1]=product(st[1],st[0]);pc=0x7a6e90u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6e90u:{std::swap(st[0],st[2]);pc=0x7a6e92u;break;} // d9ca fxch st(2)
case 0x7a6e92u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6e94u;break;} // dec1 faddp st(1)
case 0x7a6e94u:{st[2]=quotient(st[2],st[0]);popfp();pc=0x7a6e96u;break;} // defa fdivp st(2)
case 0x7a6e96u:{pushfp(fp((r[7]+12u)));pc=0x7a6e9au;break;} // d944240c fld dword ptr [esp + 0xc]
case 0x7a6e9au:{st[0]=product(st[0],st[2]);pc=0x7a6e9cu;break;} // d8ca fmul st(2)
case 0x7a6e9cu:{pushfp(fp((r[7]+16u)));pc=0x7a6ea0u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a6ea0u:{st[0]=product(st[0],st[3]);pc=0x7a6ea2u;break;} // d8cb fmul st(3)
case 0x7a6ea2u:{std::swap(st[0],st[2]);pc=0x7a6ea4u;break;} // d9ca fxch st(2)
case 0x7a6ea4u:{st[0]=product(st[0],st[3]);pc=0x7a6ea6u;break;} // d8cb fmul st(3)
case 0x7a6ea6u:{std::swap(st[0],st[1]);pc=0x7a6ea8u;break;} // d9c9 fxch st(1)
case 0x7a6ea8u:{st[5]=sum(st[5],st[0]);popfp();pc=0x7a6eaau;break;} // dec5 faddp st(5)
case 0x7a6eaau:{std::swap(st[0],st[4]);pc=0x7a6eacu;break;} // d9cc fxch st(4)
case 0x7a6eacu:{fpwrite((r[7]+12u),st[0]);pc=0x7a6eb0u;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x7a6eb0u:{std::swap(st[0],st[1]);pc=0x7a6eb2u;break;} // d9c9 fxch st(1)
case 0x7a6eb2u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a6eb4u;break;} // dec3 faddp st(3)
case 0x7a6eb4u:{std::swap(st[0],st[2]);pc=0x7a6eb6u;break;} // d9ca fxch st(2)
case 0x7a6eb6u:{fpwrite((r[7]+16u),st[0]);pc=0x7a6ebau;break;} // d9542410 fst dword ptr [esp + 0x10]
case 0x7a6ebau:{std::swap(st[0],st[3]);pc=0x7a6ebcu;break;} // d9cb fxch st(3)
case 0x7a6ebcu:{st[0]=sum(st[0],fp((r[7]+32u)));pc=0x7a6ec0u;break;} // d8442420 fadd dword ptr [esp + 0x20]
case 0x7a6ec0u:{fpwrite((r[7]+20u),st[0]);pc=0x7a6ec4u;break;} // d9542414 fst dword ptr [esp + 0x14]
case 0x7a6ec4u:{std::swap(st[0],st[3]);pc=0x7a6ec6u;break;} // d9cb fxch st(3)
case 0x7a6ec6u:{st[0]=product(st[0],fp((r[7]+132u)));pc=0x7a6ecdu;break;} // d88c2484000000 fmul dword ptr [esp + 0x84]
case 0x7a6ecdu:{pushfp(st[2]);pc=0x7a6ecfu;break;} // d9c2 fld st(2)
case 0x7a6ecfu:{pushfp(fp((r[7]+128u)));pc=0x7a6ed6u;break;} // d9842480000000 fld dword ptr [esp + 0x80]
case 0x7a6ed6u:{st[1]=product(st[1],st[0]);pc=0x7a6ed8u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6ed8u:{std::swap(st[0],st[2]);pc=0x7a6edau;break;} // d9ca fxch st(2)
case 0x7a6edau:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6edcu;break;} // dec1 faddp st(1)
case 0x7a6edcu:{pushfp(fp((r[7]+136u)));pc=0x7a6ee3u;break;} // d9842488000000 fld dword ptr [esp + 0x88]
case 0x7a6ee3u:{st[5]=product(st[5],st[0]);pc=0x7a6ee5u;break;} // dccd fmul st(5), st(0)
case 0x7a6ee5u:{std::swap(st[0],st[1]);pc=0x7a6ee7u;break;} // d9c9 fxch st(1)
case 0x7a6ee7u:{st[5]=sum(st[5],st[0]);popfp();pc=0x7a6ee9u;break;} // dec5 faddp st(5)
case 0x7a6ee9u:{std::swap(st[0],st[1]);pc=0x7a6eebu;break;} // d9c9 fxch st(1)
case 0x7a6eebu:{st[0]=product(st[0],st[6]);pc=0x7a6eedu;break;} // d8ce fmul st(6)
case 0x7a6eedu:{pushfp(fp((r[7]+132u)));pc=0x7a6ef4u;break;} // d9842484000000 fld dword ptr [esp + 0x84]
case 0x7a6ef4u:{st[0]=product(st[0],st[6]);pc=0x7a6ef6u;break;} // d8ce fmul st(6)
case 0x7a6ef6u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6ef8u;break;} // dec1 faddp st(1)
case 0x7a6ef8u:{std::swap(st[0],st[1]);pc=0x7a6efau;break;} // d9c9 fxch st(1)
case 0x7a6efau:{st[0]=product(st[0],fp((r[7]+96u)));pc=0x7a6efeu;break;} // d84c2460 fmul dword ptr [esp + 0x60]
case 0x7a6efeu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f00u;break;} // dec1 faddp st(1)
case 0x7a6f00u:{compare(st[0],st[3]);popfp();pc=0x7a6f02u;break;} // d8db fcomp st(3)
case 0x7a6f02u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6f04u;break;} // dfe0 fnstsw ax
case 0x7a6f04u:{st[2]=st[0];popfp();pc=0x7a6f06u;break;} // ddda fstp st(2)
case 0x7a6f06u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6f09u;break;} // f6c441 test ah, 0x41
case 0x7a6f09u:{pc=(pf)?0x7a6f12u:0x7a6f0bu;break;} // 7a07 jp 0x7a6f12
case 0x7a6f0bu:{r[0]=1u;pc=0x7a6f10u;break;} // b801000000 mov eax, 1
case 0x7a6f10u:{pc=(true)?0x7a6f14u:0x7a6f12u;break;} // eb02 jmp 0x7a6f14
case 0x7a6f12u:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6f14u;break;} // 33c0 xor eax, eax
case 0x7a6f14u:{pushfp(fp((r[7]+16u)));pc=0x7a6f18u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a6f18u:{r[1]=(r[1]&4294967040u)|(((r[0])&255u<<0)&255u);pc=0x7a6f1au;break;} // 8ad8 mov bl, al
case 0x7a6f1au:{pushfp(fp((r[7]+144u)));pc=0x7a6f21u;break;} // d9842490000000 fld dword ptr [esp + 0x90]
case 0x7a6f21u:{st[1]=product(st[1],st[0]);pc=0x7a6f23u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6f23u:{pushfp(st[2]);pc=0x7a6f25u;break;} // d9c2 fld st(2)
case 0x7a6f25u:{pushfp(fp((r[7]+140u)));pc=0x7a6f2cu;break;} // d984248c000000 fld dword ptr [esp + 0x8c]
case 0x7a6f2cu:{st[1]=product(st[1],st[0]);pc=0x7a6f2eu;break;} // dcc9 fmul st(1), st(0)
case 0x7a6f2eu:{std::swap(st[0],st[3]);pc=0x7a6f30u;break;} // d9cb fxch st(3)
case 0x7a6f30u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f32u;break;} // dec1 faddp st(1)
case 0x7a6f32u:{pushfp(fp((r[7]+20u)));pc=0x7a6f36u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x7a6f36u:{st[0]=product(st[0],fp((r[7]+148u)));pc=0x7a6f3du;break;} // d88c2494000000 fmul dword ptr [esp + 0x94]
case 0x7a6f3du:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f3fu;break;} // dec1 faddp st(1)
case 0x7a6f3fu:{std::swap(st[0],st[2]);pc=0x7a6f41u;break;} // d9ca fxch st(2)
case 0x7a6f41u:{st[0]=product(st[0],fp((r[7]+100u)));pc=0x7a6f45u;break;} // d84c2464 fmul dword ptr [esp + 0x64]
case 0x7a6f45u:{std::swap(st[0],st[1]);pc=0x7a6f47u;break;} // d9c9 fxch st(1)
case 0x7a6f47u:{st[0]=product(st[0],fp((r[7]+104u)));pc=0x7a6f4bu;break;} // d84c2468 fmul dword ptr [esp + 0x68]
case 0x7a6f4bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f4du;break;} // dec1 faddp st(1)
case 0x7a6f4du:{pushfp(fp((r[7]+148u)));pc=0x7a6f54u;break;} // d9842494000000 fld dword ptr [esp + 0x94]
case 0x7a6f54u:{st[0]=product(st[0],fp((r[7]+108u)));pc=0x7a6f58u;break;} // d84c246c fmul dword ptr [esp + 0x6c]
case 0x7a6f58u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f5au;break;} // dec1 faddp st(1)
case 0x7a6f5au:{compare(st[0],st[1]);popfp();popfp();pc=0x7a6f5cu;break;} // ded9 fcompp
case 0x7a6f5cu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6f5eu;break;} // dfe0 fnstsw ax
case 0x7a6f5eu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a6f61u;break;} // f6c441 test ah, 0x41
case 0x7a6f61u:{pc=(pf)?0x7a6f6au:0x7a6f63u;break;} // 7a07 jp 0x7a6f6a
case 0x7a6f63u:{r[0]=1u;pc=0x7a6f68u;break;} // b801000000 mov eax, 1
case 0x7a6f68u:{pc=(true)?0x7a6f6cu:0x7a6f6au;break;} // eb02 jmp 0x7a6f6c
case 0x7a6f6au:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6f6cu;break;} // 33c0 xor eax, eax
case 0x7a6f6cu:{pushfp(fp((r[7]+16u)));pc=0x7a6f70u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a6f70u:{r[2]=(r[2]&4294967040u)|(((r[0])&255u<<0)&255u);pc=0x7a6f72u;break;} // 8ac8 mov cl, al
case 0x7a6f72u:{pushfp(fp((r[7]+156u)));pc=0x7a6f79u;break;} // d984249c000000 fld dword ptr [esp + 0x9c]
case 0x7a6f79u:{st[1]=product(st[1],st[0]);pc=0x7a6f7bu;break;} // dcc9 fmul st(1), st(0)
case 0x7a6f7bu:{pushfp(fp((r[7]+152u)));pc=0x7a6f82u;break;} // d9842498000000 fld dword ptr [esp + 0x98]
case 0x7a6f82u:{st[3]=product(st[3],st[0]);pc=0x7a6f84u;break;} // dccb fmul st(3), st(0)
case 0x7a6f84u:{std::swap(st[0],st[2]);pc=0x7a6f86u;break;} // d9ca fxch st(2)
case 0x7a6f86u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a6f88u;break;} // dec3 faddp st(3)
case 0x7a6f88u:{pushfp(fp((r[7]+20u)));pc=0x7a6f8cu;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x7a6f8cu:{pushfp(fp((r[7]+160u)));pc=0x7a6f93u;break;} // d98424a0000000 fld dword ptr [esp + 0xa0]
case 0x7a6f93u:{st[1]=product(st[1],st[0]);pc=0x7a6f95u;break;} // dcc9 fmul st(1), st(0)
case 0x7a6f95u:{std::swap(st[0],st[4]);pc=0x7a6f97u;break;} // d9cc fxch st(4)
case 0x7a6f97u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a6f99u;break;} // dec1 faddp st(1)
case 0x7a6f99u:{std::swap(st[0],st[2]);pc=0x7a6f9bu;break;} // d9ca fxch st(2)
case 0x7a6f9bu:{st[0]=product(st[0],fp((r[7]+112u)));pc=0x7a6f9fu;break;} // d84c2470 fmul dword ptr [esp + 0x70]
case 0x7a6f9fu:{pushfp(fp((r[7]+116u)));pc=0x7a6fa3u;break;} // d9442474 fld dword ptr [esp + 0x74]
case 0x7a6fa3u:{st[2]=product(st[2],st[0]);pc=0x7a6fa5u;break;} // dcca fmul st(2), st(0)
case 0x7a6fa5u:{std::swap(st[0],st[1]);pc=0x7a6fa7u;break;} // d9c9 fxch st(1)
case 0x7a6fa7u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a6fa9u;break;} // dec2 faddp st(2)
case 0x7a6fa9u:{pushfp(fp((r[7]+120u)));pc=0x7a6fadu;break;} // d9442478 fld dword ptr [esp + 0x78]
case 0x7a6fadu:{st[4]=product(st[4],st[0]);pc=0x7a6fafu;break;} // dccc fmul st(4), st(0)
case 0x7a6fafu:{std::swap(st[0],st[2]);pc=0x7a6fb1u;break;} // d9ca fxch st(2)
case 0x7a6fb1u:{st[4]=sum(st[4],st[0]);popfp();pc=0x7a6fb3u;break;} // dec4 faddp st(4)
case 0x7a6fb3u:{std::swap(st[0],st[2]);pc=0x7a6fb5u;break;} // d9ca fxch st(2)
case 0x7a6fb5u:{compare(st[0],st[3]);popfp();pc=0x7a6fb7u;break;} // d8db fcomp st(3)
case 0x7a6fb7u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6fb9u;break;} // dfe0 fnstsw ax
case 0x7a6fb9u:{st[2]=st[0];popfp();pc=0x7a6fbbu;break;} // ddda fstp st(2)
case 0x7a6fbbu:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a6fbeu;break;} // f6c401 test ah, 1
case 0x7a6fbeu:{pc=(!zf)?0x7a6fc7u:0x7a6fc0u;break;} // 7507 jne 0x7a6fc7
case 0x7a6fc0u:{r[0]=1u;pc=0x7a6fc5u;break;} // b801000000 mov eax, 1
case 0x7a6fc5u:{pc=(true)?0x7a6fc9u:0x7a6fc7u;break;} // eb02 jmp 0x7a6fc9
case 0x7a6fc7u:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a6fc9u;break;} // 33c0 xor eax, eax
case 0x7a6fc9u:{flags(((r[1])&255u)&((r[1])&255u));cf=false;pc=0x7a6fcbu;break;} // 84db test bl, bl
case 0x7a6fcbu:{pc=(zf)?0x7a70f0u:0x7a6fd1u;break;} // 0f841f010000 je 0x7a70f0
case 0x7a6fd1u:{st[4]=st[0];popfp();pc=0x7a6fd3u;break;} // dddc fstp st(4)
case 0x7a6fd3u:{st[2]=st[0];popfp();pc=0x7a6fd5u;break;} // ddda fstp st(2)
case 0x7a6fd5u:{flags(((r[2])&255u)&((r[2])&255u));cf=false;pc=0x7a6fd7u;break;} // 84c9 test cl, cl
case 0x7a6fd7u:{pc=(zf)?0x7a707fu:0x7a6fddu;break;} // 0f84a2000000 je 0x7a707f
case 0x7a6fddu:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a6fdfu;break;} // 84c0 test al, al
case 0x7a6fdfu:{pc=(zf)?0x7a7052u:0x7a6fe1u;break;} // 7471 je 0x7a7052
case 0x7a6fe1u:{st[2]=st[0];popfp();pc=0x7a6fe3u;break;} // ddda fstp st(2)
case 0x7a6fe3u:{st[0]=st[0];popfp();pc=0x7a6fe5u;break;} // ddd8 fstp st(0)
case 0x7a6fe5u:{pushfp(fp((r[7]+208u)));pc=0x7a6fecu;break;} // d98424d0000000 fld dword ptr [esp + 0xd0]
case 0x7a6fecu:{compare(st[0],st[1]);popfp();pc=0x7a6feeu;break;} // d8d9 fcomp st(1)
case 0x7a6feeu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a6ff0u;break;} // dfe0 fnstsw ax
case 0x7a6ff0u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a6ff3u;break;} // f6c405 test ah, 5
case 0x7a6ff3u:{pc=(!pf)?0x7a6df3u:0x7a6ff9u;break;} // 0f8bfafdffff jnp 0x7a6df3
case 0x7a6ff9u:{r[2]=get((r[7]+212u),4);pc=0x7a7000u;break;} // 8b8c24d4000000 mov ecx, dword ptr [esp + 0xd4]
case 0x7a7000u:{r[0]=get((r[7]+204u),4);pc=0x7a7007u;break;} // 8b8424cc000000 mov eax, dword ptr [esp + 0xcc]
case 0x7a7007u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a7008u;break;} // 52 push edx
case 0x7a7008u:{fpwrite((r[0]),st[0]);pc=0x7a700au;break;} // d910 fst dword ptr [eax]
case 0x7a700au:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a700bu;break;} // 51 push ecx
case 0x7a700bu:{r[3]=(r[7]+32u);pc=0x7a700fu;break;} // 8d542420 lea edx, [esp + 0x20]
case 0x7a700fu:{fpwrite((r[7]),st[0]);popfp();pc=0x7a7012u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a7012u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a7013u;break;} // 52 push edx
case 0x7a7013u:{put((r[2]),1u,4);pc=0x7a7019u;break;} // c70101000000 mov dword ptr [ecx], 1
case 0x7a7019u:{r[7]-=4;put(r[7],0x7a701eu,4);pc=0x542830u;break;} // e812b8d9ff call 0x542830
case 0x7a701eu:{pushfp(fp((r[7]+24u)));pc=0x7a7022u;break;} // d9442418 fld dword ptr [esp + 0x18]
case 0x7a7022u:{st[0]=sum(st[0],fp((r[0])));pc=0x7a7024u;break;} // d800 fadd dword ptr [eax]
case 0x7a7024u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x7a7027u;break;} // 83c40c add esp, 0xc
case 0x7a7027u:{pushfp(fp((r[0]+4u)));pc=0x7a702au;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a702au:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a702bu;break;} // 5b pop ebx
case 0x7a702bu:{st[0]=sum(st[0],fp((r[7]+12u)));pc=0x7a702fu;break;} // d844240c fadd dword ptr [esp + 0xc]
case 0x7a702fu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a7030u;break;} // 5f pop edi
case 0x7a7030u:{pushfp(fp((r[0]+8u)));pc=0x7a7033u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a7033u:{r[0]=get((r[7]+208u),4);pc=0x7a703au;break;} // 8b8424d0000000 mov eax, dword ptr [esp + 0xd0]
case 0x7a703au:{st[0]=sum(st[0],fp((r[7]+12u)));pc=0x7a703eu;break;} // d844240c fadd dword ptr [esp + 0xc]
case 0x7a703eu:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a703fu;break;} // 5e pop esi
case 0x7a703fu:{std::swap(st[0],st[2]);pc=0x7a7041u;break;} // d9ca fxch st(2)
case 0x7a7041u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a7043u;break;} // d918 fstp dword ptr [eax]
case 0x7a7043u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a7046u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a7046u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a7049u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a7049u:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a704bu;break;} // b001 mov al, 1
case 0x7a704bu:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a7051u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a7051u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a7052u:{st[0]=st[0];popfp();pc=0x7a7054u;break;} // ddd8 fstp st(0)
case 0x7a7054u:{pushfp(fp((r[7]+112u)));pc=0x7a7058u;break;} // d9442470 fld dword ptr [esp + 0x70]
case 0x7a7058u:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a705cu;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a705cu:{std::swap(st[0],st[1]);pc=0x7a705eu;break;} // d9c9 fxch st(1)
case 0x7a705eu:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a7062u;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a7062u:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a7066u;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a7066u:{pushfp(fp((r[7]+76u)));pc=0x7a706au;break;} // d944244c fld dword ptr [esp + 0x4c]
case 0x7a706au:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a706eu;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a706eu:{pushfp(fp((r[7]+80u)));pc=0x7a7072u;break;} // d9442450 fld dword ptr [esp + 0x50]
case 0x7a7072u:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a7076u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a7076u:{pushfp(fp((r[7]+84u)));pc=0x7a707au;break;} // d9442454 fld dword ptr [esp + 0x54]
case 0x7a707au:{pc=(true)?0x7a7128u:0x7a707fu;break;} // e9a9000000 jmp 0x7a7128
case 0x7a707fu:{st[0]=st[0];popfp();pc=0x7a7081u;break;} // ddd8 fstp st(0)
case 0x7a7081u:{st[1]=st[0];popfp();pc=0x7a7083u;break;} // ddd9 fstp st(1)
case 0x7a7083u:{st[0]=st[0];popfp();pc=0x7a7085u;break;} // ddd8 fstp st(0)
case 0x7a7085u:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a7087u;break;} // 84c0 test al, al
case 0x7a7087u:{pc=(zf)?0x7a70b7u:0x7a7089u;break;} // 742e je 0x7a70b7
case 0x7a7089u:{pushfp(fp((r[7]+100u)));pc=0x7a708du;break;} // d9442464 fld dword ptr [esp + 0x64]
case 0x7a708du:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a7091u;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a7091u:{pushfp(fp((r[7]+104u)));pc=0x7a7095u;break;} // d9442468 fld dword ptr [esp + 0x68]
case 0x7a7095u:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a7099u;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a7099u:{pushfp(fp((r[7]+108u)));pc=0x7a709du;break;} // d944246c fld dword ptr [esp + 0x6c]
case 0x7a709du:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a70a1u;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a70a1u:{pushfp(fp((r[7]+64u)));pc=0x7a70a5u;break;} // d9442440 fld dword ptr [esp + 0x40]
case 0x7a70a5u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a70a9u;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a70a9u:{pushfp(fp((r[7]+68u)));pc=0x7a70adu;break;} // d9442444 fld dword ptr [esp + 0x44]
case 0x7a70adu:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a70b1u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a70b1u:{pushfp(fp((r[7]+72u)));pc=0x7a70b5u;break;} // d9442448 fld dword ptr [esp + 0x48]
case 0x7a70b5u:{pc=(true)?0x7a7128u:0x7a70b7u;break;} // eb71 jmp 0x7a7128
case 0x7a70b7u:{r[0]=get((r[7]+212u),4);pc=0x7a70beu;break;} // 8b8424d4000000 mov eax, dword ptr [esp + 0xd4]
case 0x7a70beu:{pushfp(fp((r[7]+208u)));pc=0x7a70c5u;break;} // d98424d0000000 fld dword ptr [esp + 0xd0]
case 0x7a70c5u:{r[1]=get((r[7]+204u),4);pc=0x7a70ccu;break;} // 8b9c24cc000000 mov ebx, dword ptr [esp + 0xcc]
case 0x7a70ccu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a70cdu;break;} // 50 push eax
case 0x7a70cdu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a70ceu;break;} // 51 push ecx
case 0x7a70ceu:{fpwrite((r[7]),st[0]);popfp();pc=0x7a70d1u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a70d1u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a70d2u;break;} // 56 push esi
case 0x7a70d2u:{r[4]=get((r[7]+228u),4);pc=0x7a70d9u;break;} // 8bb424e4000000 mov esi, dword ptr [esp + 0xe4]
case 0x7a70d9u:{r[2]=(r[7]+124u);pc=0x7a70ddu;break;} // 8d4c247c lea ecx, [esp + 0x7c]
case 0x7a70ddu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a70deu;break;} // 51 push ecx
case 0x7a70deu:{r[7]-=4;put(r[7],0x7a70e3u,4);pc=0x7a65d0u;break;} // e8edf4ffff call 0x7a65d0
case 0x7a70e3u:{uint32_t v=(r[7])+(16u);r[7]=v;flags(v);pc=0x7a70e6u;break;} // 83c410 add esp, 0x10
case 0x7a70e6u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a70e7u;break;} // 5b pop ebx
case 0x7a70e7u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a70e8u;break;} // 5f pop edi
case 0x7a70e8u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a70e9u;break;} // 5e pop esi
case 0x7a70e9u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a70efu;break;} // 81c4ac000000 add esp, 0xac
case 0x7a70efu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a70f0u:{st[2]=st[0];popfp();pc=0x7a70f2u;break;} // ddda fstp st(2)
case 0x7a70f2u:{st[1]=st[0];popfp();pc=0x7a70f4u;break;} // ddd9 fstp st(1)
case 0x7a70f4u:{st[0]=st[0];popfp();pc=0x7a70f6u;break;} // ddd8 fstp st(0)
case 0x7a70f6u:{flags(((r[2])&255u)&((r[2])&255u));cf=false;pc=0x7a70f8u;break;} // 84c9 test cl, cl
case 0x7a70f8u:{pc=(zf)?0x7a71a6u:0x7a70feu;break;} // 0f84a8000000 je 0x7a71a6
case 0x7a70feu:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a7100u;break;} // 84c0 test al, al
case 0x7a7100u:{pc=(zf)?0x7a7169u:0x7a7102u;break;} // 7467 je 0x7a7169
case 0x7a7102u:{std::swap(st[0],st[1]);pc=0x7a7104u;break;} // d9c9 fxch st(1)
case 0x7a7104u:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a7108u;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a7108u:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a710cu;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a710cu:{pushfp(fp((r[7]+96u)));pc=0x7a7110u;break;} // d9442460 fld dword ptr [esp + 0x60]
case 0x7a7110u:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a7114u;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a7114u:{pushfp(fp((r[7]+52u)));pc=0x7a7118u;break;} // d9442434 fld dword ptr [esp + 0x34]
case 0x7a7118u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a711cu;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a711cu:{pushfp(fp((r[7]+56u)));pc=0x7a7120u;break;} // d9442438 fld dword ptr [esp + 0x38]
case 0x7a7120u:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a7124u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a7124u:{pushfp(fp((r[7]+60u)));pc=0x7a7128u;break;} // d944243c fld dword ptr [esp + 0x3c]
case 0x7a7128u:{r[2]=get((r[7]+212u),4);pc=0x7a712fu;break;} // 8b8c24d4000000 mov ecx, dword ptr [esp + 0xd4]
case 0x7a712fu:{r[0]=get((r[7]+216u),4);pc=0x7a7136u;break;} // 8b8424d8000000 mov eax, dword ptr [esp + 0xd8]
case 0x7a7136u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a7137u;break;} // 50 push eax
case 0x7a7137u:{r[0]=get((r[7]+208u),4);pc=0x7a713eu;break;} // 8b8424d0000000 mov eax, dword ptr [esp + 0xd0]
case 0x7a713eu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a713fu;break;} // 51 push ecx
case 0x7a713fu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a7140u;break;} // 51 push ecx
case 0x7a7140u:{fpwrite((r[7]+56u),st[0]);popfp();pc=0x7a7144u;break;} // d95c2438 fstp dword ptr [esp + 0x38]
case 0x7a7144u:{r[2]=(r[7]+36u);pc=0x7a7148u;break;} // 8d4c2424 lea ecx, [esp + 0x24]
case 0x7a7148u:{pushfp(fp((r[7]+220u)));pc=0x7a714fu;break;} // d98424dc000000 fld dword ptr [esp + 0xdc]
case 0x7a714fu:{fpwrite((r[7]),st[0]);popfp();pc=0x7a7152u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a7152u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a7153u;break;} // 50 push eax
case 0x7a7153u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a7154u;break;} // 57 push edi
case 0x7a7154u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a7155u;break;} // 56 push esi
case 0x7a7155u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a7156u;break;} // 52 push edx
case 0x7a7156u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a7157u;break;} // 51 push ecx
case 0x7a7157u:{r[7]-=4;put(r[7],0x7a715cu,4);pc=0x7a9220u;break;} // e8c4200000 call 0x7a9220
case 0x7a715cu:{uint32_t v=(r[7])+(32u);r[7]=v;flags(v);pc=0x7a715fu;break;} // 83c420 add esp, 0x20
case 0x7a715fu:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a7160u;break;} // 5b pop ebx
case 0x7a7160u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a7161u;break;} // 5f pop edi
case 0x7a7161u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a7162u;break;} // 5e pop esi
case 0x7a7162u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a7168u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a7168u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a7169u:{r[0]=get((r[7]+212u),4);pc=0x7a7170u;break;} // 8b8424d4000000 mov eax, dword ptr [esp + 0xd4]
case 0x7a7170u:{st[1]=st[0];popfp();pc=0x7a7172u;break;} // ddd9 fstp st(1)
case 0x7a7172u:{r[1]=get((r[7]+204u),4);pc=0x7a7179u;break;} // 8b9c24cc000000 mov ebx, dword ptr [esp + 0xcc]
case 0x7a7179u:{st[0]=st[0];popfp();pc=0x7a717bu;break;} // ddd8 fstp st(0)
case 0x7a717bu:{pushfp(fp((r[7]+208u)));pc=0x7a7182u;break;} // d98424d0000000 fld dword ptr [esp + 0xd0]
case 0x7a7182u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a7183u;break;} // 50 push eax
case 0x7a7183u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a7184u;break;} // 51 push ecx
case 0x7a7184u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a7187u;break;} // d91c24 fstp dword ptr [esp]
case 0x7a7187u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a7188u;break;} // 56 push esi
case 0x7a7188u:{r[4]=get((r[7]+228u),4);pc=0x7a718fu;break;} // 8bb424e4000000 mov esi, dword ptr [esp + 0xe4]
case 0x7a718fu:{r[2]=(r[7]+100u);pc=0x7a7193u;break;} // 8d4c2464 lea ecx, [esp + 0x64]
case 0x7a7193u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a7194u;break;} // 51 push ecx
case 0x7a7194u:{r[7]-=4;put(r[7],0x7a7199u,4);pc=0x7a65d0u;break;} // e837f4ffff call 0x7a65d0
case 0x7a7199u:{uint32_t v=(r[7])+(16u);r[7]=v;flags(v);pc=0x7a719cu;break;} // 83c410 add esp, 0x10
case 0x7a719cu:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a719du;break;} // 5b pop ebx
case 0x7a719du:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a719eu;break;} // 5f pop edi
case 0x7a719eu:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a719fu;break;} // 5e pop esi
case 0x7a719fu:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a71a5u;break;} // 81c4ac000000 add esp, 0xac
case 0x7a71a5u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a71a6u:{st[1]=st[0];popfp();pc=0x7a71a8u;break;} // ddd9 fstp st(1)
case 0x7a71a8u:{st[0]=st[0];popfp();pc=0x7a71aau;break;} // ddd8 fstp st(0)
case 0x7a71aau:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a71acu;break;} // 84c0 test al, al
case 0x7a71acu:{pc=(zf)?0x7a6df5u:0x7a71b2u;break;} // 0f8443fcffff je 0x7a6df5
case 0x7a71b2u:{r[0]=get((r[7]+212u),4);pc=0x7a71b9u;break;} // 8b8424d4000000 mov eax, dword ptr [esp + 0xd4]
case 0x7a71b9u:{pushfp(fp((r[7]+208u)));pc=0x7a71c0u;break;} // d98424d0000000 fld dword ptr [esp + 0xd0]
case 0x7a71c0u:{r[1]=get((r[7]+204u),4);pc=0x7a71c7u;break;} // 8b9c24cc000000 mov ebx, dword ptr [esp + 0xcc]
case 0x7a71c7u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a71c8u;break;} // 50 push eax
case 0x7a71c8u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a71c9u;break;} // 51 push ecx
case 0x7a71c9u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a71ccu;break;} // d91c24 fstp dword ptr [esp]
case 0x7a71ccu:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a71cdu;break;} // 56 push esi
case 0x7a71cdu:{r[4]=get((r[7]+228u),4);pc=0x7a71d4u;break;} // 8bb424e4000000 mov esi, dword ptr [esp + 0xe4]
case 0x7a71d4u:{r[2]=(r[7]+112u);pc=0x7a71d8u;break;} // 8d4c2470 lea ecx, [esp + 0x70]
case 0x7a71d8u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a71d9u;break;} // 51 push ecx
case 0x7a71d9u:{r[7]-=4;put(r[7],0x7a71deu,4);pc=0x7a65d0u;break;} // e8f2f3ffff call 0x7a65d0
case 0x7a71deu:{uint32_t v=(r[7])+(16u);r[7]=v;flags(v);pc=0x7a71e1u;break;} // 83c410 add esp, 0x10
case 0x7a71e1u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a71e2u;break;} // 5b pop ebx
case 0x7a71e2u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a71e3u;break;} // 5f pop edi
case 0x7a71e3u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a71e4u;break;} // 5e pop esi
case 0x7a71e4u:{uint32_t v=(r[7])+(172u);r[7]=v;flags(v);pc=0x7a71eau;break;} // 81c4ac000000 add esp, 0xac
case 0x7a71eau:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9220u:{r[0]=get((r[7]+8u),4);pc=0x7a9224u;break;} // 8b442408 mov eax, dword ptr [esp + 8]
case 0x7a9224u:{uint32_t v=(r[7])-(84u);r[7]=v;flags(v);pc=0x7a9227u;break;} // 83ec54 sub esp, 0x54
case 0x7a9227u:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x7a9228u;break;} // 53 push ebx
case 0x7a9228u:{r[1]=get((r[7]+104u),4);pc=0x7a922cu;break;} // 8b5c2468 mov ebx, dword ptr [esp + 0x68]
case 0x7a922cu:{pushfp(fp((r[1])));pc=0x7a922eu;break;} // d903 fld dword ptr [ebx]
case 0x7a922eu:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a922fu;break;} // 56 push esi
case 0x7a922fu:{st[0]=difference(st[0],fp((r[0])));pc=0x7a9231u;break;} // d820 fsub dword ptr [eax]
case 0x7a9231u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a9232u;break;} // 57 push edi
case 0x7a9232u:{pushfp(fp((r[1]+4u)));pc=0x7a9235u;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a9235u:{r[5]=get((r[7]+108u),4);pc=0x7a9239u;break;} // 8b7c246c mov edi, dword ptr [esp + 0x6c]
case 0x7a9239u:{st[0]=difference(st[0],fp((r[0]+4u)));pc=0x7a923cu;break;} // d86004 fsub dword ptr [eax + 4]
case 0x7a923cu:{pushfp(fp((r[1]+8u)));pc=0x7a923fu;break;} // d94308 fld dword ptr [ebx + 8]
case 0x7a923fu:{st[0]=difference(st[0],fp((r[0]+8u)));pc=0x7a9242u;break;} // d86008 fsub dword ptr [eax + 8]
case 0x7a9242u:{pushfp(st[1]);pc=0x7a9244u;break;} // d9c1 fld st(1)
case 0x7a9244u:{st[0]=product(st[0],st[2]);pc=0x7a9246u;break;} // d8ca fmul st(2)
case 0x7a9246u:{pushfp(st[3]);pc=0x7a9248u;break;} // d9c3 fld st(3)
case 0x7a9248u:{st[0]=product(st[0],st[4]);pc=0x7a924au;break;} // d8cc fmul st(4)
case 0x7a924au:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a924cu;break;} // dec1 faddp st(1)
case 0x7a924cu:{pushfp(st[1]);pc=0x7a924eu;break;} // d9c1 fld st(1)
case 0x7a924eu:{st[0]=product(st[0],st[2]);pc=0x7a9250u;break;} // d8ca fmul st(2)
case 0x7a9250u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9252u;break;} // dec1 faddp st(1)
case 0x7a9252u:{pushfp(0.);pc=0x7a9254u;break;} // d9ee fldz
case 0x7a9254u:{pushfp(st[0]);pc=0x7a9256u;break;} // d9c0 fld st(0)
case 0x7a9256u:{compare(st[0],st[2]);popfp();pc=0x7a9258u;break;} // ddea fucomp st(2)
case 0x7a9258u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a925au;break;} // dfe0 fnstsw ax
case 0x7a925au:{st[1]=st[0];popfp();pc=0x7a925cu;break;} // ddd9 fstp st(1)
case 0x7a925cu:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x7a925fu;break;} // f6c444 test ah, 0x44
case 0x7a925fu:{pc=(pf)?0x7a92cau:0x7a9261u;break;} // 7a69 jp 0x7a92ca
case 0x7a9261u:{r[0]=get((r[7]+100u),4);pc=0x7a9265u;break;} // 8b442464 mov eax, dword ptr [esp + 0x64]
case 0x7a9265u:{st[3]=st[0];popfp();pc=0x7a9267u;break;} // dddb fstp st(3)
case 0x7a9267u:{st[1]=st[0];popfp();pc=0x7a9269u;break;} // ddd9 fstp st(1)
case 0x7a9269u:{auto v=0u;r[7]-=4;put(r[7],v,4);pc=0x7a926bu;break;} // 6a00 push 0
case 0x7a926bu:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a926cu;break;} // 50 push eax
case 0x7a926cu:{st[0]=st[0];popfp();pc=0x7a926eu;break;} // ddd8 fstp st(0)
case 0x7a926eu:{r[4]=(r[5]+4u);pc=0x7a9271u;break;} // 8d7704 lea esi, [edi + 4]
case 0x7a9271u:{st[0]=st[0];popfp();pc=0x7a9273u;break;} // ddd8 fstp st(0)
case 0x7a9273u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a9274u;break;} // 56 push esi
case 0x7a9274u:{r[7]-=4;put(r[7],0x7a9279u,4);pc=0x7a3550u;break;} // e8d7a2ffff call 0x7a3550
case 0x7a9279u:{st[0]=difference(st[0],fp((10243044u)));pc=0x7a927fu;break;} // d825e44b9c00 fsub dword ptr [0x9c4be4]
case 0x7a927fu:{pushfp(fp((r[5])));pc=0x7a9281u;break;} // d907 fld dword ptr [edi]
case 0x7a9281u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x7a9284u;break;} // 83c40c add esp, 0xc
case 0x7a9284u:{st[0]=product(st[0],st[0]);pc=0x7a9286u;break;} // dcc8 fmul st(0), st(0)
case 0x7a9286u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a9288u;break;} // ded9 fcompp
case 0x7a9288u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a928au;break;} // dfe0 fnstsw ax
case 0x7a928au:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a928du;break;} // f6c401 test ah, 1
case 0x7a928du:{pc=(!zf)?0x7a92c1u:0x7a928fu;break;} // 7532 jne 0x7a92c1
case 0x7a928fu:{pushfp(0.);pc=0x7a9291u;break;} // d9ee fldz
case 0x7a9291u:{r[2]=get((r[7]+116u),4);pc=0x7a9295u;break;} // 8b4c2474 mov ecx, dword ptr [esp + 0x74]
case 0x7a9295u:{r[3]=get((r[7]+124u),4);pc=0x7a9299u;break;} // 8b54247c mov edx, dword ptr [esp + 0x7c]
case 0x7a9299u:{fpwrite((r[2]),st[0]);popfp();pc=0x7a929bu;break;} // d919 fstp dword ptr [ecx]
case 0x7a929bu:{r[0]=get((r[7]+128u),4);pc=0x7a92a2u;break;} // 8b842480000000 mov eax, dword ptr [esp + 0x80]
case 0x7a92a2u:{put((r[3]),1u,4);pc=0x7a92a8u;break;} // c70201000000 mov dword ptr [edx], 1
case 0x7a92a8u:{pushfp(fp((r[4])));pc=0x7a92aau;break;} // d906 fld dword ptr [esi]
case 0x7a92aau:{fpwrite((r[0]),st[0]);popfp();pc=0x7a92acu;break;} // d918 fstp dword ptr [eax]
case 0x7a92acu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a92adu;break;} // 5f pop edi
case 0x7a92adu:{pushfp(fp((r[4]+4u)));pc=0x7a92b0u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a92b0u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a92b3u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a92b3u:{pushfp(fp((r[4]+8u)));pc=0x7a92b6u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a92b6u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a92b7u;break;} // 5e pop esi
case 0x7a92b7u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a92bau;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a92bau:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a92bcu;break;} // b001 mov al, 1
case 0x7a92bcu:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a92bdu;break;} // 5b pop ebx
case 0x7a92bdu:{uint32_t v=(r[7])+(84u);r[7]=v;flags(v);pc=0x7a92c0u;break;} // 83c454 add esp, 0x54
case 0x7a92c0u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a92c1u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a92c2u;break;} // 5f pop edi
case 0x7a92c2u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a92c3u;break;} // 5e pop esi
case 0x7a92c3u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a92c5u;break;} // 32c0 xor al, al
case 0x7a92c5u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a92c6u;break;} // 5b pop ebx
case 0x7a92c6u:{uint32_t v=(r[7])+(84u);r[7]=v;flags(v);pc=0x7a92c9u;break;} // 83c454 add esp, 0x54
case 0x7a92c9u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a92cau:{pushfp(fp((r[5]+4u)));pc=0x7a92cdu;break;} // d94704 fld dword ptr [edi + 4]
case 0x7a92cdu:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a92ceu;break;} // 55 push ebp
case 0x7a92ceu:{r[6]=get((r[7]+104u),4);pc=0x7a92d2u;break;} // 8b6c2468 mov ebp, dword ptr [esp + 0x68]
case 0x7a92d2u:{st[0]=difference(st[0],fp((r[6])));pc=0x7a92d5u;break;} // d86500 fsub dword ptr [ebp]
case 0x7a92d5u:{pushfp(fp((r[5]+8u)));pc=0x7a92d8u;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a92d8u:{st[0]=difference(st[0],fp((r[6]+4u)));pc=0x7a92dbu;break;} // d86504 fsub dword ptr [ebp + 4]
case 0x7a92dbu:{pushfp(fp((r[5]+12u)));pc=0x7a92deu;break;} // d9470c fld dword ptr [edi + 0xc]
case 0x7a92deu:{st[0]=difference(st[0],fp((r[6]+8u)));pc=0x7a92e1u;break;} // d86508 fsub dword ptr [ebp + 8]
case 0x7a92e1u:{pushfp(st[1]);pc=0x7a92e3u;break;} // d9c1 fld st(1)
case 0x7a92e3u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a92e5u;break;} // deca fmulp st(2)
case 0x7a92e5u:{pushfp(st[2]);pc=0x7a92e7u;break;} // d9c2 fld st(2)
case 0x7a92e7u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a92e9u;break;} // decb fmulp st(3)
case 0x7a92e9u:{std::swap(st[0],st[1]);pc=0x7a92ebu;break;} // d9c9 fxch st(1)
case 0x7a92ebu:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a92edu;break;} // dec2 faddp st(2)
case 0x7a92edu:{st[0]=product(st[0],st[0]);pc=0x7a92efu;break;} // dcc8 fmul st(0), st(0)
case 0x7a92efu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a92f1u;break;} // dec1 faddp st(1)
case 0x7a92f1u:{st[0]=root(st[0]);pc=0x7a92f3u;break;} // d9fa fsqrt
case 0x7a92f3u:{compare(st[0],fp((r[5])));popfp();pc=0x7a92f5u;break;} // d81f fcomp dword ptr [edi]
case 0x7a92f5u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a92f7u;break;} // dfe0 fnstsw ax
case 0x7a92f7u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a92fau;break;} // f6c405 test ah, 5
case 0x7a92fau:{pc=(pf)?0x7a934bu:0x7a92fcu;break;} // 7a4f jp 0x7a934b
case 0x7a92fcu:{st[3]=st[0];popfp();pc=0x7a92feu;break;} // dddb fstp st(3)
case 0x7a92feu:{st[1]=st[0];popfp();pc=0x7a9300u;break;} // ddd9 fstp st(1)
case 0x7a9300u:{st[0]=st[0];popfp();pc=0x7a9302u;break;} // ddd8 fstp st(0)
case 0x7a9302u:{pushfp(fp((r[1])));pc=0x7a9304u;break;} // d903 fld dword ptr [ebx]
case 0x7a9304u:{pushfp(fp((r[1]+4u)));pc=0x7a9307u;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a9307u:{pushfp(fp((r[1]+8u)));pc=0x7a930au;break;} // d94308 fld dword ptr [ebx + 8]
case 0x7a930au:{pushfp(fp((r[5]+8u)));pc=0x7a930du;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a930du:{st[0]=product(st[0],fp((r[1]+4u)));pc=0x7a9310u;break;} // d84b04 fmul dword ptr [ebx + 4]
case 0x7a9310u:{pushfp(fp((r[1])));pc=0x7a9312u;break;} // d903 fld dword ptr [ebx]
case 0x7a9312u:{st[0]=product(st[0],fp((r[5]+4u)));pc=0x7a9315u;break;} // d84f04 fmul dword ptr [edi + 4]
case 0x7a9315u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9317u;break;} // dec1 faddp st(1)
case 0x7a9317u:{pushfp(fp((r[5]+12u)));pc=0x7a931au;break;} // d9470c fld dword ptr [edi + 0xc]
case 0x7a931au:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a931du;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a931du:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a931fu;break;} // dec1 faddp st(1)
case 0x7a931fu:{std::swap(st[0],st[2]);pc=0x7a9321u;break;} // d9ca fxch st(2)
case 0x7a9321u:{st[0]=product(st[0],fp((r[6]+4u)));pc=0x7a9324u;break;} // d84d04 fmul dword ptr [ebp + 4]
case 0x7a9324u:{std::swap(st[0],st[3]);pc=0x7a9326u;break;} // d9cb fxch st(3)
case 0x7a9326u:{st[0]=product(st[0],fp((r[6])));pc=0x7a9329u;break;} // d84d00 fmul dword ptr [ebp]
case 0x7a9329u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a932bu;break;} // dec3 faddp st(3)
case 0x7a932bu:{st[0]=product(st[0],fp((r[6]+8u)));pc=0x7a932eu;break;} // d84d08 fmul dword ptr [ebp + 8]
case 0x7a932eu:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9330u;break;} // dec2 faddp st(2)
case 0x7a9330u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a9332u;break;} // dee9 fsubp st(1)
case 0x7a9332u:{compare(st[0],st[1]);popfp();pc=0x7a9334u;break;} // d8d9 fcomp st(1)
case 0x7a9334u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9336u;break;} // dfe0 fnstsw ax
case 0x7a9336u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9339u;break;} // f6c441 test ah, 0x41
case 0x7a9339u:{pc=(zf)?0x7a95adu:0x7a933fu;break;} // 0f846e020000 je 0x7a95ad
case 0x7a933fu:{st[0]=st[0];popfp();pc=0x7a9341u;break;} // ddd8 fstp st(0)
case 0x7a9341u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a9342u;break;} // 5d pop ebp
case 0x7a9342u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a9343u;break;} // 5f pop edi
case 0x7a9343u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a9344u;break;} // 5e pop esi
case 0x7a9344u:{uint32_t v=((r[0])&255u)^((r[0])&255u);r[0]=(r[0]&4294967040u)|((v<<0)&255u);flags(v);pc=0x7a9346u;break;} // 32c0 xor al, al
case 0x7a9346u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a9347u;break;} // 5b pop ebx
case 0x7a9347u:{uint32_t v=(r[7])+(84u);r[7]=v;flags(v);pc=0x7a934au;break;} // 83c454 add esp, 0x54
case 0x7a934au:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a934bu:{st[0]=st[0];popfp();pc=0x7a934du;break;} // ddd8 fstp st(0)
case 0x7a934du:{r[0]=get((r[7]+132u),4);pc=0x7a9354u;break;} // 8b842484000000 mov eax, dword ptr [esp + 0x84]
case 0x7a9354u:{pushfp(fp((r[6])));pc=0x7a9357u;break;} // d94500 fld dword ptr [ebp]
case 0x7a9357u:{r[2]=get((r[7]+128u),4);pc=0x7a935eu;break;} // 8b8c2480000000 mov ecx, dword ptr [esp + 0x80]
case 0x7a935eu:{fpwrite((r[7]+72u),st[0]);popfp();pc=0x7a9362u;break;} // d95c2448 fstp dword ptr [esp + 0x48]
case 0x7a9362u:{r[4]=(r[6]+12u);pc=0x7a9365u;break;} // 8d750c lea esi, [ebp + 0xc]
case 0x7a9365u:{pushfp(fp((r[6]+4u)));pc=0x7a9368u;break;} // d94504 fld dword ptr [ebp + 4]
case 0x7a9368u:{r[3]=(r[7]+16u);pc=0x7a936cu;break;} // 8d542410 lea edx, [esp + 0x10]
case 0x7a936cu:{fpwrite((r[7]+76u),st[0]);popfp();pc=0x7a9370u;break;} // d95c244c fstp dword ptr [esp + 0x4c]
case 0x7a9370u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a9371u;break;} // 52 push edx
case 0x7a9371u:{pushfp(fp((r[6]+8u)));pc=0x7a9374u;break;} // d94508 fld dword ptr [ebp + 8]
case 0x7a9374u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a9375u;break;} // 50 push eax
case 0x7a9375u:{fpwrite((r[7]+88u),st[0]);popfp();pc=0x7a9379u;break;} // d95c2458 fstp dword ptr [esp + 0x58]
case 0x7a9379u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a937au;break;} // 51 push ecx
case 0x7a937au:{pushfp(fp((r[4])));pc=0x7a937cu;break;} // d906 fld dword ptr [esi]
case 0x7a937cu:{r[3]=(r[7]+84u);pc=0x7a9380u;break;} // 8d542454 lea edx, [esp + 0x54]
case 0x7a9380u:{fpwrite((r[7]+96u),st[0]);popfp();pc=0x7a9384u;break;} // d95c2460 fstp dword ptr [esp + 0x60]
case 0x7a9384u:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a9385u;break;} // 52 push edx
case 0x7a9385u:{pushfp(fp((r[4]+4u)));pc=0x7a9388u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a9388u:{r[0]=(r[7]+64u);pc=0x7a938cu;break;} // 8d442440 lea eax, [esp + 0x40]
case 0x7a938cu:{fpwrite((r[7]+104u),st[0]);popfp();pc=0x7a9390u;break;} // d95c2468 fstp dword ptr [esp + 0x68]
case 0x7a9390u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a9391u;break;} // 50 push eax
case 0x7a9391u:{pushfp(fp((r[4]+8u)));pc=0x7a9394u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a9394u:{fpwrite((r[7]+112u),st[0]);popfp();pc=0x7a9398u;break;} // d95c2470 fstp dword ptr [esp + 0x70]
case 0x7a9398u:{pushfp(fp((r[5])));pc=0x7a939au;break;} // d907 fld dword ptr [edi]
case 0x7a939au:{fpwrite((r[7]+116u),st[0]);popfp();pc=0x7a939eu;break;} // d95c2474 fstp dword ptr [esp + 0x74]
case 0x7a939eu:{pushfp(fp((r[5]+4u)));pc=0x7a93a1u;break;} // d94704 fld dword ptr [edi + 4]
case 0x7a93a1u:{fpwrite((r[7]+68u),st[0]);popfp();pc=0x7a93a5u;break;} // d95c2444 fstp dword ptr [esp + 0x44]
case 0x7a93a5u:{pushfp(fp((r[5]+8u)));pc=0x7a93a8u;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a93a8u:{fpwrite((r[7]+72u),st[0]);popfp();pc=0x7a93acu;break;} // d95c2448 fstp dword ptr [esp + 0x48]
case 0x7a93acu:{pushfp(fp((r[5]+12u)));pc=0x7a93afu;break;} // d9470c fld dword ptr [edi + 0xc]
case 0x7a93afu:{fpwrite((r[7]+76u),st[0]);popfp();pc=0x7a93b3u;break;} // d95c244c fstp dword ptr [esp + 0x4c]
case 0x7a93b3u:{std::swap(st[0],st[2]);pc=0x7a93b5u;break;} // d9ca fxch st(2)
case 0x7a93b5u:{fpwrite((r[7]+80u),st[0]);popfp();pc=0x7a93b9u;break;} // d95c2450 fstp dword ptr [esp + 0x50]
case 0x7a93b9u:{fpwrite((r[7]+84u),st[0]);popfp();pc=0x7a93bdu;break;} // d95c2454 fstp dword ptr [esp + 0x54]
case 0x7a93bdu:{fpwrite((r[7]+88u),st[0]);popfp();pc=0x7a93c1u;break;} // d95c2458 fstp dword ptr [esp + 0x58]
case 0x7a93c1u:{r[7]-=4;put(r[7],0x7a93c6u,4);pc=0x7a9d30u;break;} // e86a090000 call 0x7a9d30
case 0x7a93c6u:{uint32_t v=(r[7])+(20u);r[7]=v;flags(v);pc=0x7a93c9u;break;} // 83c414 add esp, 0x14
case 0x7a93c9u:{flags(((r[0])&255u)&((r[0])&255u));cf=false;pc=0x7a93cbu;break;} // 84c0 test al, al
case 0x7a93cbu:{pc=(zf)?0x7a9341u:0x7a93d1u;break;} // 0f8470ffffff je 0x7a9341
case 0x7a93d1u:{r[2]=get((r[7]+128u),4);pc=0x7a93d8u;break;} // 8b8c2480000000 mov ecx, dword ptr [esp + 0x80]
case 0x7a93d8u:{uint32_t x=get((r[2]),4),y=1u;flags(x-y);cf=x<y;pc=0x7a93dbu;break;} // 833901 cmp dword ptr [ecx], 1
case 0x7a93dbu:{pc=(!zf)?0x7a93e3u:0x7a93ddu;break;} // 7506 jne 0x7a93e3
case 0x7a93ddu:{pushfp(fp((r[7]+16u)));pc=0x7a93e1u;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a93e1u:{pc=(true)?0x7a93fau:0x7a93e3u;break;} // eb17 jmp 0x7a93fa
case 0x7a93e3u:{pushfp(fp((r[7]+20u)));pc=0x7a93e7u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x7a93e7u:{pushfp(fp((r[7]+16u)));pc=0x7a93ebu;break;} // d9442410 fld dword ptr [esp + 0x10]
case 0x7a93ebu:{compare(st[0],st[1]);pc=0x7a93edu;break;} // d8d1 fcom st(1)
case 0x7a93edu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a93efu;break;} // dfe0 fnstsw ax
case 0x7a93efu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a93f2u;break;} // f6c405 test ah, 5
case 0x7a93f2u:{pc=(pf)?0x7a93f8u:0x7a93f4u;break;} // 7a04 jp 0x7a93f8
case 0x7a93f4u:{st[1]=st[0];popfp();pc=0x7a93f6u;break;} // ddd9 fstp st(1)
case 0x7a93f6u:{pc=(true)?0x7a93fau:0x7a93f8u;break;} // eb02 jmp 0x7a93fa
case 0x7a93f8u:{st[0]=st[0];popfp();pc=0x7a93fau;break;} // ddd8 fstp st(0)
case 0x7a93fau:{fpwrite((r[7]+116u),st[0]);pc=0x7a93feu;break;} // d9542474 fst dword ptr [esp + 0x74]
case 0x7a93feu:{pushfp(0.);pc=0x7a9400u;break;} // d9ee fldz
case 0x7a9400u:{compare(st[0],st[1]);pc=0x7a9402u;break;} // d8d1 fcom st(1)
case 0x7a9402u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9404u;break;} // dfe0 fnstsw ax
case 0x7a9404u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a9407u;break;} // f6c405 test ah, 5
case 0x7a9407u:{pc=(!pf)?0x7a9411u:0x7a9409u;break;} // 7b08 jnp 0x7a9411
case 0x7a9409u:{st[1]=st[0];popfp();pc=0x7a940bu;break;} // ddd9 fstp st(1)
case 0x7a940bu:{fpwrite((r[7]+116u),st[0]);pc=0x7a940fu;break;} // d9542474 fst dword ptr [esp + 0x74]
case 0x7a940fu:{pc=(true)?0x7a9413u:0x7a9411u;break;} // eb02 jmp 0x7a9413
case 0x7a9411u:{st[0]=st[0];popfp();pc=0x7a9413u;break;} // ddd8 fstp st(0)
case 0x7a9413u:{pushfp(fp((r[7]+124u)));pc=0x7a9417u;break;} // d944247c fld dword ptr [esp + 0x7c]
case 0x7a9417u:{compare(st[0],st[1]);popfp();pc=0x7a9419u;break;} // d8d9 fcomp st(1)
case 0x7a9419u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a941bu;break;} // dfe0 fnstsw ax
case 0x7a941bu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a941eu;break;} // f6c441 test ah, 0x41
case 0x7a941eu:{pc=(!pf)?0x7a933fu:0x7a9424u;break;} // 0f8b1bffffff jnp 0x7a933f
case 0x7a9424u:{pushfp(fp((r[1])));pc=0x7a9426u;break;} // d903 fld dword ptr [ebx]
case 0x7a9426u:{r[0]=get((r[7]+108u),4);pc=0x7a942au;break;} // 8b44246c mov eax, dword ptr [esp + 0x6c]
case 0x7a942au:{pushfp(fp((r[1]+4u)));pc=0x7a942du;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a942du:{pushfp(fp((r[1]+8u)));pc=0x7a9430u;break;} // d94308 fld dword ptr [ebx + 8]
case 0x7a9430u:{std::swap(st[0],st[2]);pc=0x7a9432u;break;} // d9ca fxch st(2)
case 0x7a9432u:{st[0]=product(st[0],st[3]);pc=0x7a9434u;break;} // d8cb fmul st(3)
case 0x7a9434u:{std::swap(st[0],st[1]);pc=0x7a9436u;break;} // d9c9 fxch st(1)
case 0x7a9436u:{st[0]=product(st[0],st[3]);pc=0x7a9438u;break;} // d8cb fmul st(3)
case 0x7a9438u:{std::swap(st[0],st[2]);pc=0x7a943au;break;} // d9ca fxch st(2)
case 0x7a943au:{st[0]=product(st[0],st[3]);pc=0x7a943cu;break;} // d8cb fmul st(3)
case 0x7a943cu:{pushfp(fp((r[5]+4u)));pc=0x7a943fu;break;} // d94704 fld dword ptr [edi + 4]
case 0x7a943fu:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9441u;break;} // dec2 faddp st(2)
case 0x7a9441u:{std::swap(st[0],st[1]);pc=0x7a9443u;break;} // d9c9 fxch st(1)
case 0x7a9443u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a9447u;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a9447u:{std::swap(st[0],st[1]);pc=0x7a9449u;break;} // d9c9 fxch st(1)
case 0x7a9449u:{st[0]=sum(st[0],fp((r[5]+8u)));pc=0x7a944cu;break;} // d84708 fadd dword ptr [edi + 8]
case 0x7a944cu:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x7a9450u;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x7a9450u:{st[0]=sum(st[0],fp((r[5]+12u)));pc=0x7a9453u;break;} // d8470c fadd dword ptr [edi + 0xc]
case 0x7a9453u:{fpwrite((r[7]+44u),st[0]);popfp();pc=0x7a9457u;break;} // d95c242c fstp dword ptr [esp + 0x2c]
case 0x7a9457u:{pushfp(fp((r[0])));pc=0x7a9459u;break;} // d900 fld dword ptr [eax]
case 0x7a9459u:{pushfp(fp((r[0]+4u)));pc=0x7a945cu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a945cu:{pushfp(fp((r[0]+8u)));pc=0x7a945fu;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a945fu:{std::swap(st[0],st[2]);pc=0x7a9461u;break;} // d9ca fxch st(2)
case 0x7a9461u:{st[0]=product(st[0],st[3]);pc=0x7a9463u;break;} // d8cb fmul st(3)
case 0x7a9463u:{std::swap(st[0],st[1]);pc=0x7a9465u;break;} // d9c9 fxch st(1)
case 0x7a9465u:{st[0]=product(st[0],st[3]);pc=0x7a9467u;break;} // d8cb fmul st(3)
case 0x7a9467u:{std::swap(st[0],st[2]);pc=0x7a9469u;break;} // d9ca fxch st(2)
case 0x7a9469u:{st[0]=product(st[0],st[3]);pc=0x7a946bu;break;} // d8cb fmul st(3)
case 0x7a946bu:{std::swap(st[0],st[1]);pc=0x7a946du;break;} // d9c9 fxch st(1)
case 0x7a946du:{st[0]=sum(st[0],fp((r[6])));pc=0x7a9470u;break;} // d84500 fadd dword ptr [ebp]
case 0x7a9470u:{fpwrite((r[7]+24u),st[0]);pc=0x7a9474u;break;} // d9542418 fst dword ptr [esp + 0x18]
case 0x7a9474u:{std::swap(st[0],st[2]);pc=0x7a9476u;break;} // d9ca fxch st(2)
case 0x7a9476u:{st[0]=sum(st[0],fp((r[6]+4u)));pc=0x7a9479u;break;} // d84504 fadd dword ptr [ebp + 4]
case 0x7a9479u:{fpwrite((r[7]+28u),st[0]);pc=0x7a947du;break;} // d954241c fst dword ptr [esp + 0x1c]
case 0x7a947du:{pushfp(fp((r[6]+8u)));pc=0x7a9480u;break;} // d94508 fld dword ptr [ebp + 8]
case 0x7a9480u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9482u;break;} // dec2 faddp st(2)
case 0x7a9482u:{std::swap(st[0],st[1]);pc=0x7a9484u;break;} // d9c9 fxch st(1)
case 0x7a9484u:{fpwrite((r[7]+32u),st[0]);pc=0x7a9488u;break;} // d9542420 fst dword ptr [esp + 0x20]
case 0x7a9488u:{pushfp(st[1]);pc=0x7a948au;break;} // d9c1 fld st(1)
case 0x7a948au:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7a948du;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7a948du:{pushfp(st[3]);pc=0x7a948fu;break;} // d9c3 fld st(3)
case 0x7a948fu:{st[0]=product(st[0],fp((r[4])));pc=0x7a9491u;break;} // d80e fmul dword ptr [esi]
case 0x7a9491u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9493u;break;} // dec1 faddp st(1)
case 0x7a9493u:{std::swap(st[0],st[1]);pc=0x7a9495u;break;} // d9c9 fxch st(1)
case 0x7a9495u:{st[0]=product(st[0],fp((r[4]+8u)));pc=0x7a9498u;break;} // d84e08 fmul dword ptr [esi + 8]
case 0x7a9498u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a949au;break;} // dec1 faddp st(1)
case 0x7a949au:{pushfp(fp((r[4]+4u)));pc=0x7a949du;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a949du:{pushfp(fp((r[4])));pc=0x7a949fu;break;} // d906 fld dword ptr [esi]
case 0x7a949fu:{pushfp(fp((r[4]+8u)));pc=0x7a94a2u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a94a2u:{pushfp(st[2]);pc=0x7a94a4u;break;} // d9c2 fld st(2)
case 0x7a94a4u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a94a6u;break;} // decb fmulp st(3)
case 0x7a94a6u:{pushfp(st[1]);pc=0x7a94a8u;break;} // d9c1 fld st(1)
case 0x7a94a8u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a94aau;break;} // deca fmulp st(2)
case 0x7a94aau:{std::swap(st[0],st[2]);pc=0x7a94acu;break;} // d9ca fxch st(2)
case 0x7a94acu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a94aeu;break;} // dec1 faddp st(1)
case 0x7a94aeu:{pushfp(st[1]);pc=0x7a94b0u;break;} // d9c1 fld st(1)
case 0x7a94b0u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a94b2u;break;} // deca fmulp st(2)
case 0x7a94b2u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a94b4u;break;} // dec1 faddp st(1)
case 0x7a94b4u:{st[0]=sum(st[0],st[1]);pc=0x7a94b6u;break;} // d8c1 fadd st(1)
case 0x7a94b6u:{pushfp(fp((r[7]+40u)));pc=0x7a94bau;break;} // d9442428 fld dword ptr [esp + 0x28]
case 0x7a94bau:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7a94bdu;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7a94bdu:{pushfp(fp((r[7]+36u)));pc=0x7a94c1u;break;} // d9442424 fld dword ptr [esp + 0x24]
case 0x7a94c1u:{st[0]=product(st[0],fp((r[4])));pc=0x7a94c3u;break;} // d80e fmul dword ptr [esi]
case 0x7a94c3u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a94c5u;break;} // dec1 faddp st(1)
case 0x7a94c5u:{pushfp(fp((r[7]+44u)));pc=0x7a94c9u;break;} // d944242c fld dword ptr [esp + 0x2c]
case 0x7a94c9u:{st[0]=product(st[0],fp((r[4]+8u)));pc=0x7a94ccu;break;} // d84e08 fmul dword ptr [esi + 8]
case 0x7a94ccu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a94ceu;break;} // dec1 faddp st(1)
case 0x7a94ceu:{compare(st[0],st[2]);pc=0x7a94d0u;break;} // d8d2 fcom st(2)
case 0x7a94d0u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a94d2u;break;} // dfe0 fnstsw ax
case 0x7a94d2u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a94d5u;break;} // f6c405 test ah, 5
case 0x7a94d5u:{pc=(pf)?0x7a94f4u:0x7a94d7u;break;} // 7a1d jp 0x7a94f4
case 0x7a94d7u:{r[0]=get((r[7]+132u),4);pc=0x7a94deu;break;} // 8b842484000000 mov eax, dword ptr [esp + 0x84]
case 0x7a94deu:{st[2]=st[0];popfp();pc=0x7a94e0u;break;} // ddda fstp st(2)
case 0x7a94e0u:{st[1]=st[0];popfp();pc=0x7a94e2u;break;} // ddd9 fstp st(1)
case 0x7a94e2u:{st[0]=st[0];popfp();pc=0x7a94e4u;break;} // ddd8 fstp st(0)
case 0x7a94e4u:{std::swap(st[0],st[1]);pc=0x7a94e6u;break;} // d9c9 fxch st(1)
case 0x7a94e6u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a94e8u;break;} // d918 fstp dword ptr [eax]
case 0x7a94e8u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a94ebu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a94ebu:{pushfp(fp((r[7]+32u)));pc=0x7a94efu;break;} // d9442420 fld dword ptr [esp + 0x20]
case 0x7a94efu:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a94f2u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a94f2u:{pc=(true)?0x7a956du:0x7a94f4u;break;} // eb79 jmp 0x7a956d
case 0x7a94f4u:{compare(st[0],st[1]);pc=0x7a94f6u;break;} // d8d1 fcom st(1)
case 0x7a94f6u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a94f8u;break;} // dfe0 fnstsw ax
case 0x7a94f8u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a94fbu;break;} // f6c441 test ah, 0x41
case 0x7a94fbu:{pc=(!zf)?0x7a9526u:0x7a94fdu;break;} // 7529 jne 0x7a9526
case 0x7a94fdu:{r[0]=get((r[7]+132u),4);pc=0x7a9504u;break;} // 8b842484000000 mov eax, dword ptr [esp + 0x84]
case 0x7a9504u:{st[2]=st[0];popfp();pc=0x7a9506u;break;} // ddda fstp st(2)
case 0x7a9506u:{st[1]=st[0];popfp();pc=0x7a9508u;break;} // ddd9 fstp st(1)
case 0x7a9508u:{st[0]=st[0];popfp();pc=0x7a950au;break;} // ddd8 fstp st(0)
case 0x7a950au:{std::swap(st[0],st[1]);pc=0x7a950cu;break;} // d9c9 fxch st(1)
case 0x7a950cu:{st[0]=sum(st[0],fp((r[4])));pc=0x7a950eu;break;} // d806 fadd dword ptr [esi]
case 0x7a950eu:{std::swap(st[0],st[1]);pc=0x7a9510u;break;} // d9c9 fxch st(1)
case 0x7a9510u:{st[0]=sum(st[0],fp((r[4]+4u)));pc=0x7a9513u;break;} // d84604 fadd dword ptr [esi + 4]
case 0x7a9513u:{pushfp(fp((r[7]+32u)));pc=0x7a9517u;break;} // d9442420 fld dword ptr [esp + 0x20]
case 0x7a9517u:{st[0]=sum(st[0],fp((r[4]+8u)));pc=0x7a951au;break;} // d84608 fadd dword ptr [esi + 8]
case 0x7a951au:{std::swap(st[0],st[2]);pc=0x7a951cu;break;} // d9ca fxch st(2)
case 0x7a951cu:{fpwrite((r[0]),st[0]);popfp();pc=0x7a951eu;break;} // d918 fstp dword ptr [eax]
case 0x7a951eu:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a9521u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a9521u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a9524u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a9524u:{pc=(true)?0x7a956du:0x7a9526u;break;} // eb47 jmp 0x7a956d
case 0x7a9526u:{st[5]=st[0];popfp();pc=0x7a9528u;break;} // dddd fstp st(5)
case 0x7a9528u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a9529u;break;} // 56 push esi
case 0x7a9529u:{st[3]=st[0];popfp();pc=0x7a952bu;break;} // dddb fstp st(3)
case 0x7a952bu:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a952cu;break;} // 51 push ecx
case 0x7a952cu:{st[1]=st[0];popfp();pc=0x7a952eu;break;} // ddd9 fstp st(1)
case 0x7a952eu:{r[3]=(r[7]+44u);pc=0x7a9532u;break;} // 8d54242c lea edx, [esp + 0x2c]
case 0x7a9532u:{st[2]=difference(st[2],st[0]);pc=0x7a9534u;break;} // dcea fsub st(2), st(0)
case 0x7a9534u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a9536u;break;} // dee9 fsubp st(1)
case 0x7a9536u:{st[1]=quotient(st[1],st[0]);popfp();pc=0x7a9538u;break;} // def9 fdivp st(1)
case 0x7a9538u:{fpwrite((r[7]),st[0]);popfp();pc=0x7a953bu;break;} // d91c24 fstp dword ptr [esp]
case 0x7a953bu:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a953cu;break;} // 52 push edx
case 0x7a953cu:{r[7]-=4;put(r[7],0x7a9541u,4);pc=0x542830u;break;} // e8ef92d9ff call 0x542830
case 0x7a9541u:{pushfp(fp((r[0])));pc=0x7a9543u;break;} // d900 fld dword ptr [eax]
case 0x7a9543u:{st[0]=sum(st[0],fp((r[7]+36u)));pc=0x7a9547u;break;} // d8442424 fadd dword ptr [esp + 0x24]
case 0x7a9547u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x7a954au;break;} // 83c40c add esp, 0xc
case 0x7a954au:{pushfp(fp((r[0]+4u)));pc=0x7a954du;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a954du:{st[0]=sum(st[0],fp((r[7]+28u)));pc=0x7a9551u;break;} // d844241c fadd dword ptr [esp + 0x1c]
case 0x7a9551u:{pushfp(fp((r[0]+8u)));pc=0x7a9554u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7a9554u:{r[0]=get((r[7]+132u),4);pc=0x7a955bu;break;} // 8b842484000000 mov eax, dword ptr [esp + 0x84]
case 0x7a955bu:{st[0]=sum(st[0],fp((r[7]+32u)));pc=0x7a955fu;break;} // d8442420 fadd dword ptr [esp + 0x20]
case 0x7a955fu:{std::swap(st[0],st[2]);pc=0x7a9561u;break;} // d9ca fxch st(2)
case 0x7a9561u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a9563u;break;} // d918 fstp dword ptr [eax]
case 0x7a9563u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a9566u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a9566u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a9569u;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a9569u:{pushfp(fp((r[7]+116u)));pc=0x7a956du;break;} // d9442474 fld dword ptr [esp + 0x74]
case 0x7a956du:{pushfp(fp((r[1])));pc=0x7a956fu;break;} // d903 fld dword ptr [ebx]
case 0x7a956fu:{pushfp(fp((r[1]+4u)));pc=0x7a9572u;break;} // d94304 fld dword ptr [ebx + 4]
case 0x7a9572u:{pushfp(fp((r[1]+8u)));pc=0x7a9575u;break;} // d94308 fld dword ptr [ebx + 8]
case 0x7a9575u:{pushfp(fp((r[5]+8u)));pc=0x7a9578u;break;} // d94708 fld dword ptr [edi + 8]
case 0x7a9578u:{st[0]=product(st[0],fp((r[1]+4u)));pc=0x7a957bu;break;} // d84b04 fmul dword ptr [ebx + 4]
case 0x7a957bu:{pushfp(fp((r[1])));pc=0x7a957du;break;} // d903 fld dword ptr [ebx]
case 0x7a957du:{st[0]=product(st[0],fp((r[5]+4u)));pc=0x7a9580u;break;} // d84f04 fmul dword ptr [edi + 4]
case 0x7a9580u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9582u;break;} // dec1 faddp st(1)
case 0x7a9582u:{pushfp(fp((r[5]+12u)));pc=0x7a9585u;break;} // d9470c fld dword ptr [edi + 0xc]
case 0x7a9585u:{st[0]=product(st[0],fp((r[1]+8u)));pc=0x7a9588u;break;} // d84b08 fmul dword ptr [ebx + 8]
case 0x7a9588u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a958au;break;} // dec1 faddp st(1)
case 0x7a958au:{std::swap(st[0],st[3]);pc=0x7a958cu;break;} // d9cb fxch st(3)
case 0x7a958cu:{st[0]=product(st[0],fp((r[0])));pc=0x7a958eu;break;} // d808 fmul dword ptr [eax]
case 0x7a958eu:{pushfp(fp((r[0]+4u)));pc=0x7a9591u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7a9591u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a9593u;break;} // decb fmulp st(3)
case 0x7a9593u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9595u;break;} // dec2 faddp st(2)
case 0x7a9595u:{st[0]=product(st[0],fp((r[0]+8u)));pc=0x7a9598u;break;} // d84808 fmul dword ptr [eax + 8]
case 0x7a9598u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a959au;break;} // dec1 faddp st(1)
case 0x7a959au:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a959cu;break;} // dee1 fsubrp st(1)
case 0x7a959cu:{compare(st[0],fp((10236820u)));popfp();pc=0x7a95a2u;break;} // d81d94339c00 fcomp dword ptr [0x9c3394]
case 0x7a95a2u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a95a4u;break;} // dfe0 fnstsw ax
case 0x7a95a4u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a95a7u;break;} // f6c405 test ah, 5
case 0x7a95a7u:{pc=(!pf)?0x7a933fu:0x7a95adu;break;} // 0f8b92fdffff jnp 0x7a933f
case 0x7a95adu:{r[0]=get((r[7]+120u),4);pc=0x7a95b1u;break;} // 8b442478 mov eax, dword ptr [esp + 0x78]
case 0x7a95b1u:{r[2]=get((r[7]+128u),4);pc=0x7a95b8u;break;} // 8b8c2480000000 mov ecx, dword ptr [esp + 0x80]
case 0x7a95b8u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a95bau;break;} // d918 fstp dword ptr [eax]
case 0x7a95bau:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a95bbu;break;} // 5d pop ebp
case 0x7a95bbu:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a95bcu;break;} // 5f pop edi
case 0x7a95bcu:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a95bdu;break;} // 5e pop esi
case 0x7a95bdu:{r[0]=(r[0]&4294967040u)|((1u<<0)&255u);pc=0x7a95bfu;break;} // b001 mov al, 1
case 0x7a95bfu:{put((r[2]),1u,4);pc=0x7a95c5u;break;} // c70101000000 mov dword ptr [ecx], 1
case 0x7a95c5u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a95c6u;break;} // 5b pop ebx
case 0x7a95c6u:{uint32_t v=(r[7])+(84u);r[7]=v;flags(v);pc=0x7a95c9u;break;} // 83c454 add esp, 0x54
case 0x7a95c9u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a98c0u:{uint32_t v=(r[7])-(60u);r[7]=v;flags(v);pc=0x7a98c3u;break;} // 83ec3c sub esp, 0x3c
case 0x7a98c3u:{pushfp(fp((r[5]+12u)));pc=0x7a98c6u;break;} // d9470c fld dword ptr [edi + 0xc]
case 0x7a98c6u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a98c7u;break;} // 55 push ebp
case 0x7a98c7u:{r[6]=get((r[7]+68u),4);pc=0x7a98cbu;break;} // 8b6c2444 mov ebp, dword ptr [esp + 0x44]
case 0x7a98cbu:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x7a98cfu;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x7a98cfu:{pushfp(fp((r[5]+16u)));pc=0x7a98d2u;break;} // d94710 fld dword ptr [edi + 0x10]
case 0x7a98d2u:{r[2]=(r[7]+16u);pc=0x7a98d6u;break;} // 8d4c2410 lea ecx, [esp + 0x10]
case 0x7a98d6u:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x7a98dau;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x7a98dau:{pushfp(fp((r[5]+20u)));pc=0x7a98ddu;break;} // d94714 fld dword ptr [edi + 0x14]
case 0x7a98ddu:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x7a98e1u;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x7a98e1u:{r[7]-=4;put(r[7],0x7a98e6u,4);pc=0x441bb0u;break;} // e8ca82c9ff call 0x441bb0
case 0x7a98e6u:{auto v=1u;r[7]-=4;put(r[7],v,4);pc=0x7a98e8u;break;} // 6a01 push 1
case 0x7a98e8u:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x7a98ecu;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x7a98ecu:{r[0]=(r[7]+20u);pc=0x7a98f0u;break;} // 8d442414 lea eax, [esp + 0x14]
case 0x7a98f0u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x7a98f1u;break;} // 50 push eax
case 0x7a98f1u:{r[2]=(r[7]+60u);pc=0x7a98f5u;break;} // 8d4c243c lea ecx, [esp + 0x3c]
case 0x7a98f5u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x7a98f6u;break;} // 51 push ecx
case 0x7a98f6u:{r[3]=(r[7]+52u);pc=0x7a98fau;break;} // 8d542434 lea edx, [esp + 0x34]
case 0x7a98fau:{auto v=r[3];r[7]-=4;put(r[7],v,4);pc=0x7a98fbu;break;} // 52 push edx
case 0x7a98fbu:{r[7]-=4;put(r[7],0x7a9900u,4);pc=0x7aa200u;break;} // e800090000 call 0x7aa200
case 0x7a9900u:{pushfp(fp((r[4])));pc=0x7a9902u;break;} // d906 fld dword ptr [esi]
case 0x7a9902u:{st[0]=product(st[0],fp((r[7]+56u)));pc=0x7a9906u;break;} // d84c2438 fmul dword ptr [esp + 0x38]
case 0x7a9906u:{uint32_t v=(r[7])+(16u);r[7]=v;flags(v);pc=0x7a9909u;break;} // 83c410 add esp, 0x10
case 0x7a9909u:{pushfp(fp((r[4]+4u)));pc=0x7a990cu;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a990cu:{r[2]=(r[7]+28u);pc=0x7a9910u;break;} // 8d4c241c lea ecx, [esp + 0x1c]
case 0x7a9910u:{st[0]=product(st[0],fp((r[7]+44u)));pc=0x7a9914u;break;} // d84c242c fmul dword ptr [esp + 0x2c]
case 0x7a9914u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9916u;break;} // dec1 faddp st(1)
case 0x7a9916u:{pushfp(fp((r[4]+8u)));pc=0x7a9919u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a9919u:{st[0]=product(st[0],fp((r[7]+48u)));pc=0x7a991du;break;} // d84c2430 fmul dword ptr [esp + 0x30]
case 0x7a991du:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a991fu;break;} // dec1 faddp st(1)
case 0x7a991fu:{fpwrite((r[7]+28u),st[0]);popfp();pc=0x7a9923u;break;} // d95c241c fstp dword ptr [esp + 0x1c]
case 0x7a9923u:{pushfp(fp((r[4])));pc=0x7a9925u;break;} // d906 fld dword ptr [esi]
case 0x7a9925u:{st[0]=product(st[0],fp((r[7]+52u)));pc=0x7a9929u;break;} // d84c2434 fmul dword ptr [esp + 0x34]
case 0x7a9929u:{pushfp(fp((r[4]+4u)));pc=0x7a992cu;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a992cu:{st[0]=product(st[0],fp((r[7]+56u)));pc=0x7a9930u;break;} // d84c2438 fmul dword ptr [esp + 0x38]
case 0x7a9930u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9932u;break;} // dec1 faddp st(1)
case 0x7a9932u:{pushfp(fp((r[4]+8u)));pc=0x7a9935u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a9935u:{st[0]=product(st[0],fp((r[7]+60u)));pc=0x7a9939u;break;} // d84c243c fmul dword ptr [esp + 0x3c]
case 0x7a9939u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a993bu;break;} // dec1 faddp st(1)
case 0x7a993bu:{fpwrite((r[7]+32u),st[0]);popfp();pc=0x7a993fu;break;} // d95c2420 fstp dword ptr [esp + 0x20]
case 0x7a993fu:{pushfp(fp((r[4])));pc=0x7a9941u;break;} // d906 fld dword ptr [esi]
case 0x7a9941u:{st[0]=product(st[0],fp((r[7]+16u)));pc=0x7a9945u;break;} // d84c2410 fmul dword ptr [esp + 0x10]
case 0x7a9945u:{pushfp(fp((r[4]+4u)));pc=0x7a9948u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a9948u:{st[0]=product(st[0],fp((r[7]+20u)));pc=0x7a994cu;break;} // d84c2414 fmul dword ptr [esp + 0x14]
case 0x7a994cu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a994eu;break;} // dec1 faddp st(1)
case 0x7a994eu:{pushfp(fp((r[4]+8u)));pc=0x7a9951u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a9951u:{st[0]=product(st[0],fp((r[7]+24u)));pc=0x7a9955u;break;} // d84c2418 fmul dword ptr [esp + 0x18]
case 0x7a9955u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9957u;break;} // dec1 faddp st(1)
case 0x7a9957u:{fpwrite((r[7]+36u),st[0]);popfp();pc=0x7a995bu;break;} // d95c2424 fstp dword ptr [esp + 0x24]
case 0x7a995bu:{r[7]-=4;put(r[7],0x7a9960u,4);pc=0x441bb0u;break;} // e85082c9ff call 0x441bb0
case 0x7a9960u:{pushfp(1.);pc=0x7a9962u;break;} // d9e8 fld1
case 0x7a9962u:{st[0]=quotient(st[0],st[1]);pc=0x7a9964u;break;} // d8f1 fdiv st(1)
case 0x7a9964u:{fpwrite((r[7]+68u),st[0]);pc=0x7a9968u;break;} // d9542444 fst dword ptr [esp + 0x44]
case 0x7a9968u:{pushfp(fp((r[6])));pc=0x7a996bu;break;} // d94500 fld dword ptr [ebp]
case 0x7a996bu:{st[0]=difference(st[0],fp((r[5])));pc=0x7a996du;break;} // d827 fsub dword ptr [edi]
case 0x7a996du:{pushfp(fp((r[6]+4u)));pc=0x7a9970u;break;} // d94504 fld dword ptr [ebp + 4]
case 0x7a9970u:{st[0]=difference(st[0],fp((r[5]+4u)));pc=0x7a9973u;break;} // d86704 fsub dword ptr [edi + 4]
case 0x7a9973u:{pushfp(fp((r[6]+8u)));pc=0x7a9976u;break;} // d94508 fld dword ptr [ebp + 8]
case 0x7a9976u:{st[0]=difference(st[0],fp((r[5]+8u)));pc=0x7a9979u;break;} // d86708 fsub dword ptr [edi + 8]
case 0x7a9979u:{pushfp(st[1]);pc=0x7a997bu;break;} // d9c1 fld st(1)
case 0x7a997bu:{st[0]=product(st[0],fp((r[7]+44u)));pc=0x7a997fu;break;} // d84c242c fmul dword ptr [esp + 0x2c]
case 0x7a997fu:{pushfp(st[3]);pc=0x7a9981u;break;} // d9c3 fld st(3)
case 0x7a9981u:{st[0]=product(st[0],fp((r[7]+40u)));pc=0x7a9985u;break;} // d84c2428 fmul dword ptr [esp + 0x28]
case 0x7a9985u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9987u;break;} // dec1 faddp st(1)
case 0x7a9987u:{pushfp(st[1]);pc=0x7a9989u;break;} // d9c1 fld st(1)
case 0x7a9989u:{st[0]=product(st[0],fp((r[7]+48u)));pc=0x7a998du;break;} // d84c2430 fmul dword ptr [esp + 0x30]
case 0x7a998du:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a998fu;break;} // dec1 faddp st(1)
case 0x7a998fu:{pushfp(st[2]);pc=0x7a9991u;break;} // d9c2 fld st(2)
case 0x7a9991u:{st[0]=product(st[0],fp((r[7]+56u)));pc=0x7a9995u;break;} // d84c2438 fmul dword ptr [esp + 0x38]
case 0x7a9995u:{pushfp(st[4]);pc=0x7a9997u;break;} // d9c4 fld st(4)
case 0x7a9997u:{st[0]=product(st[0],fp((r[7]+52u)));pc=0x7a999bu;break;} // d84c2434 fmul dword ptr [esp + 0x34]
case 0x7a999bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a999du;break;} // dec1 faddp st(1)
case 0x7a999du:{pushfp(st[2]);pc=0x7a999fu;break;} // d9c2 fld st(2)
case 0x7a999fu:{st[0]=product(st[0],fp((r[7]+60u)));pc=0x7a99a3u;break;} // d84c243c fmul dword ptr [esp + 0x3c]
case 0x7a99a3u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a99a5u;break;} // dec1 faddp st(1)
case 0x7a99a5u:{std::swap(st[0],st[3]);pc=0x7a99a7u;break;} // d9cb fxch st(3)
case 0x7a99a7u:{st[0]=product(st[0],fp((r[7]+20u)));pc=0x7a99abu;break;} // d84c2414 fmul dword ptr [esp + 0x14]
case 0x7a99abu:{std::swap(st[0],st[4]);pc=0x7a99adu;break;} // d9cc fxch st(4)
case 0x7a99adu:{st[0]=product(st[0],fp((r[7]+16u)));pc=0x7a99b1u;break;} // d84c2410 fmul dword ptr [esp + 0x10]
case 0x7a99b1u:{st[4]=sum(st[4],st[0]);popfp();pc=0x7a99b3u;break;} // dec4 faddp st(4)
case 0x7a99b3u:{std::swap(st[0],st[1]);pc=0x7a99b5u;break;} // d9c9 fxch st(1)
case 0x7a99b5u:{st[0]=product(st[0],fp((r[7]+24u)));pc=0x7a99b9u;break;} // d84c2418 fmul dword ptr [esp + 0x18]
case 0x7a99b9u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a99bbu;break;} // dec3 faddp st(3)
case 0x7a99bbu:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a99bcu;break;} // 5d pop ebp
case 0x7a99bcu:{pushfp(fp((r[5]+24u)));pc=0x7a99bfu;break;} // d94718 fld dword ptr [edi + 0x18]
case 0x7a99bfu:{st[0]=product(st[0],st[0]);pc=0x7a99c1u;break;} // dcc8 fmul st(0), st(0)
case 0x7a99c1u:{pushfp(fp((r[7]+32u)));pc=0x7a99c5u;break;} // d9442420 fld dword ptr [esp + 0x20]
case 0x7a99c5u:{pushfp(st[0]);pc=0x7a99c7u;break;} // d9c0 fld st(0)
case 0x7a99c7u:{st[0]=std::abs(st[0]);pc=0x7a99c9u;break;} // d9e1 fabs
case 0x7a99c9u:{compare(st[0],fp((10493880u)));popfp();pc=0x7a99cfu;break;} // d81db81fa000 fcomp dword ptr [0xa01fb8]
case 0x7a99cfu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a99d1u;break;} // dfe0 fnstsw ax
case 0x7a99d1u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a99d4u;break;} // f6c401 test ah, 1
case 0x7a99d4u:{pc=(zf)?0x7a9c71u:0x7a99dau;break;} // 0f8497020000 je 0x7a9c71
case 0x7a99dau:{pushfp(fp((10236808u)));pc=0x7a99e0u;break;} // d90588339c00 fld dword ptr [0x9c3388]
case 0x7a99e0u:{compare(st[0],st[7]);popfp();pc=0x7a99e2u;break;} // d8df fcomp st(7)
case 0x7a99e2u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a99e4u;break;} // dfe0 fnstsw ax
case 0x7a99e4u:{st[6]=st[0];popfp();pc=0x7a99e6u;break;} // ddde fstp st(6)
case 0x7a99e6u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a99e9u;break;} // f6c441 test ah, 0x41
case 0x7a99e9u:{pc=(zf)?0x7a9c73u:0x7a99efu;break;} // 0f8484020000 je 0x7a9c73
case 0x7a99efu:{st[4]=st[0];popfp();pc=0x7a99f1u;break;} // dddc fstp st(4)
case 0x7a99f1u:{pushfp(fp((r[7]+28u)));pc=0x7a99f5u;break;} // d944241c fld dword ptr [esp + 0x1c]
case 0x7a99f5u:{st[0]=product(st[0],st[0]);pc=0x7a99f7u;break;} // dcc8 fmul st(0), st(0)
case 0x7a99f7u:{pushfp(fp((r[7]+24u)));pc=0x7a99fbu;break;} // d9442418 fld dword ptr [esp + 0x18]
case 0x7a99fbu:{pushfp(st[0]);pc=0x7a99fdu;break;} // d9c0 fld st(0)
case 0x7a99fdu:{st[0]=product(st[0],st[1]);pc=0x7a99ffu;break;} // d8c9 fmul st(1)
case 0x7a99ffu:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9a01u;break;} // dec2 faddp st(2)
case 0x7a9a01u:{pushfp(fp((r[7]+28u)));pc=0x7a9a05u;break;} // d944241c fld dword ptr [esp + 0x1c]
case 0x7a9a05u:{st[0]=product(st[0],st[4]);pc=0x7a9a07u;break;} // d8cc fmul st(4)
case 0x7a9a07u:{std::swap(st[0],st[1]);pc=0x7a9a09u;break;} // d9c9 fxch st(1)
case 0x7a9a09u:{st[0]=product(st[0],st[3]);pc=0x7a9a0bu;break;} // d8cb fmul st(3)
case 0x7a9a0bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9a0du;break;} // dec1 faddp st(1)
case 0x7a9a0du:{pushfp(st[3]);pc=0x7a9a0fu;break;} // d9c3 fld st(3)
case 0x7a9a0fu:{st[4]=product(st[4],st[0]);popfp();pc=0x7a9a11u;break;} // decc fmulp st(4)
case 0x7a9a11u:{pushfp(st[2]);pc=0x7a9a13u;break;} // d9c2 fld st(2)
case 0x7a9a13u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a9a15u;break;} // decb fmulp st(3)
case 0x7a9a15u:{std::swap(st[0],st[3]);pc=0x7a9a17u;break;} // d9cb fxch st(3)
case 0x7a9a17u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9a19u;break;} // dec2 faddp st(2)
case 0x7a9a19u:{std::swap(st[0],st[1]);pc=0x7a9a1bu;break;} // d9c9 fxch st(1)
case 0x7a9a1bu:{st[4]=difference(st[0],st[4]);popfp();pc=0x7a9a1du;break;} // dee4 fsubrp st(4)
case 0x7a9a1du:{std::swap(st[0],st[3]);pc=0x7a9a1fu;break;} // d9cb fxch st(3)
case 0x7a9a1fu:{fpwrite((r[7]+8u),st[0]);pc=0x7a9a23u;break;} // d9542408 fst dword ptr [esp + 8]
case 0x7a9a23u:{pushfp(st[1]);pc=0x7a9a25u;break;} // d9c1 fld st(1)
case 0x7a9a25u:{st[0]=product(st[0],st[2]);pc=0x7a9a27u;break;} // d8ca fmul st(2)
case 0x7a9a27u:{pushfp(st[1]);pc=0x7a9a29u;break;} // d9c1 fld st(1)
case 0x7a9a29u:{st[0]=product(st[0],st[5]);pc=0x7a9a2bu;break;} // d8cd fmul st(5)
case 0x7a9a2bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a9a2du;break;} // dee9 fsubp st(1)
case 0x7a9a2du:{pushfp(0.);pc=0x7a9a2fu;break;} // d9ee fldz
case 0x7a9a2fu:{compare(st[0],st[1]);pc=0x7a9a31u;break;} // d8d1 fcom st(1)
case 0x7a9a31u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9a33u;break;} // dfe0 fnstsw ax
case 0x7a9a33u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9a36u;break;} // f6c441 test ah, 0x41
case 0x7a9a36u:{pc=(!zf)?0x7a9a4cu:0x7a9a38u;break;} // 7514 jne 0x7a9a4c
case 0x7a9a38u:{st[1]=st[0];popfp();pc=0x7a9a3au;break;} // ddd9 fstp st(1)
case 0x7a9a3au:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a9a3cu;break;} // 33c0 xor eax, eax
case 0x7a9a3cu:{st[3]=st[0];popfp();pc=0x7a9a3eu;break;} // dddb fstp st(3)
case 0x7a9a3eu:{st[4]=st[0];popfp();pc=0x7a9a40u;break;} // dddc fstp st(4)
case 0x7a9a40u:{st[1]=st[0];popfp();pc=0x7a9a42u;break;} // ddd9 fstp st(1)
case 0x7a9a42u:{st[0]=st[0];popfp();pc=0x7a9a44u;break;} // ddd8 fstp st(0)
case 0x7a9a44u:{st[1]=st[0];popfp();pc=0x7a9a46u;break;} // ddd9 fstp st(1)
case 0x7a9a46u:{st[0]=st[0];popfp();pc=0x7a9a48u;break;} // ddd8 fstp st(0)
case 0x7a9a48u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9a4bu;break;} // 83c43c add esp, 0x3c
case 0x7a9a4bu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9a4cu:{compare(st[0],st[1]);pc=0x7a9a4eu;break;} // d8d1 fcom st(1)
case 0x7a9a4eu:{uint32_t v=(r[2])^(r[2]);r[2]=v;flags(v);pc=0x7a9a50u;break;} // 33c9 xor ecx, ecx
case 0x7a9a50u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9a52u;break;} // dfe0 fnstsw ax
case 0x7a9a52u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a9a55u;break;} // f6c405 test ah, 5
case 0x7a9a55u:{pc=(pf)?0x7a9ae4u:0x7a9a5bu;break;} // 0f8a89000000 jp 0x7a9ae4
case 0x7a9a5bu:{st[2]=st[0];popfp();pc=0x7a9a5du;break;} // ddda fstp st(2)
case 0x7a9a5du:{st[0]=root(st[0]);pc=0x7a9a5fu;break;} // d9fa fsqrt
case 0x7a9a5fu:{pushfp(1.);pc=0x7a9a61u;break;} // d9e8 fld1
case 0x7a9a61u:{st[5]=quotient(st[0],st[5]);popfp();pc=0x7a9a63u;break;} // def5 fdivrp st(5)
case 0x7a9a63u:{pushfp(st[2]);pc=0x7a9a65u;break;} // d9c2 fld st(2)
case 0x7a9a65u:{st[0]=-st[0];pc=0x7a9a67u;break;} // d9e0 fchs
case 0x7a9a67u:{st[0]=difference(st[0],st[1]);pc=0x7a9a69u;break;} // d8e1 fsub st(1)
case 0x7a9a69u:{st[0]=product(st[0],st[5]);pc=0x7a9a6bu;break;} // d8cd fmul st(5)
case 0x7a9a6bu:{fpwrite((r[7]+4u),st[0]);pc=0x7a9a6fu;break;} // d9542404 fst dword ptr [esp + 4]
case 0x7a9a6fu:{st[0]=product(st[0],st[6]);pc=0x7a9a71u;break;} // d8ce fmul st(6)
case 0x7a9a71u:{st[0]=sum(st[0],st[4]);pc=0x7a9a73u;break;} // d8c4 fadd st(4)
case 0x7a9a73u:{compare(st[0],st[2]);pc=0x7a9a75u;break;} // d8d2 fcom st(2)
case 0x7a9a75u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9a77u;break;} // dfe0 fnstsw ax
case 0x7a9a77u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9a7au;break;} // f6c401 test ah, 1
case 0x7a9a7au:{pc=(!zf)?0x7a9a97u:0x7a9a7cu;break;} // 751b jne 0x7a9a97
case 0x7a9a7cu:{compare(st[0],fp((r[7])));popfp();pc=0x7a9a7fu;break;} // d81c24 fcomp dword ptr [esp]
case 0x7a9a7fu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9a81u;break;} // dfe0 fnstsw ax
case 0x7a9a81u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9a84u;break;} // f6c441 test ah, 0x41
case 0x7a9a84u:{pc=(pf)?0x7a9a99u:0x7a9a86u;break;} // 7a13 jp 0x7a9a99
case 0x7a9a86u:{pushfp(fp((r[7]+4u)));pc=0x7a9a8au;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a9a8au:{r[2]=1u;pc=0x7a9a8fu;break;} // b901000000 mov ecx, 1
case 0x7a9a8fu:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9a93u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9a93u:{fpwrite((r[1]),st[0]);popfp();pc=0x7a9a95u;break;} // d91b fstp dword ptr [ebx]
case 0x7a9a95u:{pc=(true)?0x7a9a99u:0x7a9a97u;break;} // eb02 jmp 0x7a9a99
case 0x7a9a97u:{st[0]=st[0];popfp();pc=0x7a9a99u;break;} // ddd8 fstp st(0)
case 0x7a9a99u:{pushfp(fp((r[7])));pc=0x7a9a9cu;break;} // d90424 fld dword ptr [esp]
case 0x7a9a9cu:{std::swap(st[0],st[1]);pc=0x7a9a9eu;break;} // d9c9 fxch st(1)
case 0x7a9a9eu:{st[0]=difference(st[0],st[3]);pc=0x7a9aa0u;break;} // d8e3 fsub st(3)
case 0x7a9aa0u:{st[5]=product(st[5],st[0]);popfp();pc=0x7a9aa2u;break;} // decd fmulp st(5)
case 0x7a9aa2u:{pushfp(st[4]);pc=0x7a9aa4u;break;} // d9c4 fld st(4)
case 0x7a9aa4u:{st[0]=product(st[0],st[6]);pc=0x7a9aa6u;break;} // d8ce fmul st(6)
case 0x7a9aa6u:{st[0]=sum(st[0],st[4]);pc=0x7a9aa8u;break;} // d8c4 fadd st(4)
case 0x7a9aa8u:{compare(st[0],st[2]);pc=0x7a9aaau;break;} // d8d2 fcom st(2)
case 0x7a9aaau:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9aacu;break;} // dfe0 fnstsw ax
case 0x7a9aacu:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9aafu;break;} // f6c401 test ah, 1
case 0x7a9aafu:{pc=(!zf)?0x7a9ac7u:0x7a9ab1u;break;} // 7516 jne 0x7a9ac7
case 0x7a9ab1u:{compare(st[0],st[1]);popfp();pc=0x7a9ab3u;break;} // d8d9 fcomp st(1)
case 0x7a9ab3u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9ab5u;break;} // dfe0 fnstsw ax
case 0x7a9ab5u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9ab8u;break;} // f6c441 test ah, 0x41
case 0x7a9ab8u:{pc=(pf)?0x7a9ac9u:0x7a9abau;break;} // 7a0f jp 0x7a9ac9
case 0x7a9abau:{std::swap(st[0],st[4]);pc=0x7a9abcu;break;} // d9cc fxch st(4)
case 0x7a9abcu:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9abdu;break;} // 41 inc ecx
case 0x7a9abdu:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9ac1u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9ac1u:{fpwrite((r[1]+r[2]*4u+4294967292u),st[0]);popfp();pc=0x7a9ac5u;break;} // d95c8bfc fstp dword ptr [ebx + ecx*4 - 4]
case 0x7a9ac5u:{pc=(true)?0x7a9acbu:0x7a9ac7u;break;} // eb04 jmp 0x7a9acb
case 0x7a9ac7u:{st[0]=st[0];popfp();pc=0x7a9ac9u;break;} // ddd8 fstp st(0)
case 0x7a9ac9u:{st[4]=st[0];popfp();pc=0x7a9acbu;break;} // dddc fstp st(4)
case 0x7a9acbu:{uint32_t x=r[2],y=2u;flags(x-y);cf=x<y;pc=0x7a9aceu;break;} // 83f902 cmp ecx, 2
case 0x7a9aceu:{pc=(!zf)?0x7a9b8bu:0x7a9ad4u;break;} // 0f85b7000000 jne 0x7a9b8b
case 0x7a9ad4u:{st[2]=st[0];popfp();pc=0x7a9ad6u;break;} // ddda fstp st(2)
case 0x7a9ad6u:{r[0]=r[2];pc=0x7a9ad8u;break;} // 8bc1 mov eax, ecx
case 0x7a9ad8u:{st[3]=st[0];popfp();pc=0x7a9adau;break;} // dddb fstp st(3)
case 0x7a9adau:{st[0]=st[0];popfp();pc=0x7a9adcu;break;} // ddd8 fstp st(0)
case 0x7a9adcu:{st[1]=st[0];popfp();pc=0x7a9adeu;break;} // ddd9 fstp st(1)
case 0x7a9adeu:{st[0]=st[0];popfp();pc=0x7a9ae0u;break;} // ddd8 fstp st(0)
case 0x7a9ae0u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9ae3u;break;} // 83c43c add esp, 0x3c
case 0x7a9ae3u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9ae4u:{st[1]=st[0];popfp();pc=0x7a9ae6u;break;} // ddd9 fstp st(1)
case 0x7a9ae6u:{pushfp(st[2]);pc=0x7a9ae8u;break;} // d9c2 fld st(2)
case 0x7a9ae8u:{st[5]=quotient(st[0],st[5]);popfp();pc=0x7a9aeau;break;} // def5 fdivrp st(5)
case 0x7a9aeau:{std::swap(st[0],st[4]);pc=0x7a9aecu;break;} // d9cc fxch st(4)
case 0x7a9aecu:{st[0]=-st[0];pc=0x7a9aeeu;break;} // d9e0 fchs
case 0x7a9aeeu:{fpwrite((r[7]+4u),st[0]);pc=0x7a9af2u;break;} // d9542404 fst dword ptr [esp + 4]
case 0x7a9af2u:{st[0]=product(st[0],st[5]);pc=0x7a9af4u;break;} // d8cd fmul st(5)
case 0x7a9af4u:{st[0]=sum(st[0],st[3]);pc=0x7a9af6u;break;} // d8c3 fadd st(3)
case 0x7a9af6u:{compare(st[0],st[4]);pc=0x7a9af8u;break;} // d8d4 fcom st(4)
case 0x7a9af8u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9afau;break;} // dfe0 fnstsw ax
case 0x7a9afau:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9afdu;break;} // f6c401 test ah, 1
case 0x7a9afdu:{pc=(!zf)?0x7a9b2cu:0x7a9affu;break;} // 752d jne 0x7a9b2c
case 0x7a9affu:{pushfp(fp((r[7])));pc=0x7a9b02u;break;} // d90424 fld dword ptr [esp]
case 0x7a9b02u:{compare(st[0],st[1]);pc=0x7a9b04u;break;} // d8d1 fcom st(1)
case 0x7a9b04u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9b06u;break;} // dfe0 fnstsw ax
case 0x7a9b06u:{st[1]=st[0];popfp();pc=0x7a9b08u;break;} // ddd9 fstp st(1)
case 0x7a9b08u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9b0bu;break;} // f6c401 test ah, 1
case 0x7a9b0bu:{pc=(!zf)?0x7a9b31u:0x7a9b0du;break;} // 7524 jne 0x7a9b31
case 0x7a9b0du:{st[3]=st[0];popfp();pc=0x7a9b0fu;break;} // dddb fstp st(3)
case 0x7a9b0fu:{r[0]=1u;pc=0x7a9b14u;break;} // b801000000 mov eax, 1
case 0x7a9b14u:{st[4]=st[0];popfp();pc=0x7a9b16u;break;} // dddc fstp st(4)
case 0x7a9b16u:{st[2]=st[0];popfp();pc=0x7a9b18u;break;} // ddda fstp st(2)
case 0x7a9b18u:{st[1]=st[0];popfp();pc=0x7a9b1au;break;} // ddd9 fstp st(1)
case 0x7a9b1au:{st[1]=st[0];popfp();pc=0x7a9b1cu;break;} // ddd9 fstp st(1)
case 0x7a9b1cu:{st[0]=st[0];popfp();pc=0x7a9b1eu;break;} // ddd8 fstp st(0)
case 0x7a9b1eu:{pushfp(fp((r[7]+4u)));pc=0x7a9b22u;break;} // d9442404 fld dword ptr [esp + 4]
case 0x7a9b22u:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9b26u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9b26u:{fpwrite((r[1]),st[0]);popfp();pc=0x7a9b28u;break;} // d91b fstp dword ptr [ebx]
case 0x7a9b28u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9b2bu;break;} // 83c43c add esp, 0x3c
case 0x7a9b2bu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9b2cu:{st[0]=st[0];popfp();pc=0x7a9b2eu;break;} // ddd8 fstp st(0)
case 0x7a9b2eu:{pushfp(fp((r[7])));pc=0x7a9b31u;break;} // d90424 fld dword ptr [esp]
case 0x7a9b31u:{std::swap(st[0],st[4]);pc=0x7a9b33u;break;} // d9cc fxch st(4)
case 0x7a9b33u:{std::swap(st[0],st[1]);pc=0x7a9b35u;break;} // d9c9 fxch st(1)
case 0x7a9b35u:{pushfp(st[3]);pc=0x7a9b37u;break;} // d9c3 fld st(3)
case 0x7a9b37u:{st[0]=product(st[0],st[6]);pc=0x7a9b39u;break;} // d8ce fmul st(6)
case 0x7a9b39u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a9b3bu;break;} // dec3 faddp st(3)
case 0x7a9b3bu:{pushfp(st[3]);pc=0x7a9b3du;break;} // d9c3 fld st(3)
case 0x7a9b3du:{st[0]=product(st[0],st[4]);pc=0x7a9b3fu;break;} // d8cc fmul st(4)
case 0x7a9b3fu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9b41u;break;} // dec1 faddp st(1)
case 0x7a9b41u:{fpwrite((r[7]+8u),st[0]);pc=0x7a9b45u;break;} // d9542408 fst dword ptr [esp + 8]
case 0x7a9b45u:{pushfp(st[2]);pc=0x7a9b47u;break;} // d9c2 fld st(2)
case 0x7a9b47u:{st[0]=product(st[0],st[3]);pc=0x7a9b49u;break;} // d8cb fmul st(3)
case 0x7a9b49u:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a9b4bu;break;} // dee1 fsubrp st(1)
case 0x7a9b4bu:{compare(st[0],st[1]);pc=0x7a9b4du;break;} // d8d1 fcom st(1)
case 0x7a9b4du:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9b4fu;break;} // dfe0 fnstsw ax
case 0x7a9b4fu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9b52u;break;} // f6c441 test ah, 0x41
case 0x7a9b52u:{pc=(!zf)?0x7a9bc2u:0x7a9b54u;break;} // 756e jne 0x7a9bc2
case 0x7a9b54u:{st[0]=root(st[0]);pc=0x7a9b56u;break;} // d9fa fsqrt
case 0x7a9b56u:{pushfp(st[2]);pc=0x7a9b58u;break;} // d9c2 fld st(2)
case 0x7a9b58u:{st[0]=-st[0];pc=0x7a9b5au;break;} // d9e0 fchs
case 0x7a9b5au:{st[0]=difference(st[0],st[1]);pc=0x7a9b5cu;break;} // d8e1 fsub st(1)
case 0x7a9b5cu:{pushfp(st[0]);pc=0x7a9b5eu;break;} // d9c0 fld st(0)
case 0x7a9b5eu:{st[0]=product(st[0],st[7]);pc=0x7a9b60u;break;} // d8cf fmul st(7)
case 0x7a9b60u:{st[0]=sum(st[0],st[5]);pc=0x7a9b62u;break;} // d8c5 fadd st(5)
case 0x7a9b62u:{compare(st[0],st[3]);popfp();pc=0x7a9b64u;break;} // d8db fcomp st(3)
case 0x7a9b64u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9b66u;break;} // dfe0 fnstsw ax
case 0x7a9b66u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9b69u;break;} // f6c441 test ah, 0x41
case 0x7a9b69u:{pc=(pf)?0x7a9b91u:0x7a9b6bu;break;} // 7a26 jp 0x7a9b91
case 0x7a9b6bu:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9b6fu;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9b6fu:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9b70u;break;} // 41 inc ecx
case 0x7a9b70u:{fpwrite((r[1]+r[2]*4u+4294967292u),st[0]);popfp();pc=0x7a9b74u;break;} // d95c8bfc fstp dword ptr [ebx + ecx*4 - 4]
case 0x7a9b74u:{uint32_t x=r[2],y=2u;flags(x-y);cf=x<y;pc=0x7a9b77u;break;} // 83f902 cmp ecx, 2
case 0x7a9b77u:{pc=(!zf)?0x7a9b93u:0x7a9b79u;break;} // 751a jne 0x7a9b93
case 0x7a9b79u:{st[0]=st[0];popfp();pc=0x7a9b7bu;break;} // ddd8 fstp st(0)
case 0x7a9b7bu:{r[0]=r[2];pc=0x7a9b7du;break;} // 8bc1 mov eax, ecx
case 0x7a9b7du:{st[2]=st[0];popfp();pc=0x7a9b7fu;break;} // ddda fstp st(2)
case 0x7a9b7fu:{st[3]=st[0];popfp();pc=0x7a9b81u;break;} // dddb fstp st(3)
case 0x7a9b81u:{st[0]=st[0];popfp();pc=0x7a9b83u;break;} // ddd8 fstp st(0)
case 0x7a9b83u:{st[1]=st[0];popfp();pc=0x7a9b85u;break;} // ddd9 fstp st(1)
case 0x7a9b85u:{st[0]=st[0];popfp();pc=0x7a9b87u;break;} // ddd8 fstp st(0)
case 0x7a9b87u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9b8au;break;} // 83c43c add esp, 0x3c
case 0x7a9b8au:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9b8bu:{pushfp(fp((r[7]+8u)));pc=0x7a9b8fu;break;} // d9442408 fld dword ptr [esp + 8]
case 0x7a9b8fu:{pc=(true)?0x7a9b35u:0x7a9b91u;break;} // eba4 jmp 0x7a9b35
case 0x7a9b91u:{st[0]=st[0];popfp();pc=0x7a9b93u;break;} // ddd8 fstp st(0)
case 0x7a9b93u:{st[0]=difference(st[0],st[2]);pc=0x7a9b95u;break;} // d8e2 fsub st(2)
case 0x7a9b95u:{pushfp(st[0]);pc=0x7a9b97u;break;} // d9c0 fld st(0)
case 0x7a9b97u:{st[0]=product(st[0],st[6]);pc=0x7a9b99u;break;} // d8ce fmul st(6)
case 0x7a9b99u:{st[0]=sum(st[0],st[4]);pc=0x7a9b9bu;break;} // d8c4 fadd st(4)
case 0x7a9b9bu:{compare(st[0],st[2]);popfp();pc=0x7a9b9du;break;} // d8da fcomp st(2)
case 0x7a9b9du:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9b9fu;break;} // dfe0 fnstsw ax
case 0x7a9b9fu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9ba2u;break;} // f6c441 test ah, 0x41
case 0x7a9ba2u:{pc=(pf)?0x7a9bd3u:0x7a9ba4u;break;} // 7a2f jp 0x7a9bd3
case 0x7a9ba4u:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9ba8u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9ba8u:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9ba9u;break;} // 41 inc ecx
case 0x7a9ba9u:{fpwrite((r[1]+r[2]*4u+4294967292u),st[0]);popfp();pc=0x7a9badu;break;} // d95c8bfc fstp dword ptr [ebx + ecx*4 - 4]
case 0x7a9badu:{uint32_t x=r[2],y=2u;flags(x-y);cf=x<y;pc=0x7a9bb0u;break;} // 83f902 cmp ecx, 2
case 0x7a9bb0u:{pc=(!zf)?0x7a9bd5u:0x7a9bb2u;break;} // 7523 jne 0x7a9bd5
case 0x7a9bb2u:{st[2]=st[0];popfp();pc=0x7a9bb4u;break;} // ddda fstp st(2)
case 0x7a9bb4u:{r[0]=r[2];pc=0x7a9bb6u;break;} // 8bc1 mov eax, ecx
case 0x7a9bb6u:{st[3]=st[0];popfp();pc=0x7a9bb8u;break;} // dddb fstp st(3)
case 0x7a9bb8u:{st[0]=st[0];popfp();pc=0x7a9bbau;break;} // ddd8 fstp st(0)
case 0x7a9bbau:{st[1]=st[0];popfp();pc=0x7a9bbcu;break;} // ddd9 fstp st(1)
case 0x7a9bbcu:{st[0]=st[0];popfp();pc=0x7a9bbeu;break;} // ddd8 fstp st(0)
case 0x7a9bbeu:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9bc1u;break;} // 83c43c add esp, 0x3c
case 0x7a9bc1u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9bc2u:{pushfp(st[1]);pc=0x7a9bc4u;break;} // d9c1 fld st(1)
case 0x7a9bc4u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a9bc6u;break;} // dae9 fucompp
case 0x7a9bc6u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9bc8u;break;} // dfe0 fnstsw ax
case 0x7a9bc8u:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x7a9bcbu;break;} // f6c444 test ah, 0x44
case 0x7a9bcbu:{pc=(pf)?0x7a9bd5u:0x7a9bcdu;break;} // 7a08 jp 0x7a9bd5
case 0x7a9bcdu:{pushfp(st[1]);pc=0x7a9bcfu;break;} // d9c1 fld st(1)
case 0x7a9bcfu:{st[0]=-st[0];pc=0x7a9bd1u;break;} // d9e0 fchs
case 0x7a9bd1u:{pc=(true)?0x7a9b95u:0x7a9bd3u;break;} // ebc2 jmp 0x7a9b95
case 0x7a9bd3u:{st[0]=st[0];popfp();pc=0x7a9bd5u;break;} // ddd8 fstp st(0)
case 0x7a9bd5u:{pushfp(st[4]);pc=0x7a9bd7u;break;} // d9c4 fld st(4)
case 0x7a9bd7u:{st[0]=product(st[0],st[4]);pc=0x7a9bd9u;break;} // d8cc fmul st(4)
case 0x7a9bd9u:{st[2]=difference(st[2],st[0]);popfp();pc=0x7a9bdbu;break;} // deea fsubp st(2)
case 0x7a9bdbu:{pushfp(st[1]);pc=0x7a9bddu;break;} // d9c1 fld st(1)
case 0x7a9bddu:{st[0]=product(st[0],st[2]);pc=0x7a9bdfu;break;} // d8ca fmul st(2)
case 0x7a9bdfu:{pushfp(st[3]);pc=0x7a9be1u;break;} // d9c3 fld st(3)
case 0x7a9be1u:{st[0]=sum(st[0],st[0]);pc=0x7a9be3u;break;} // dcc0 fadd st(0), st(0)
case 0x7a9be3u:{st[0]=difference(st[5],st[0]);pc=0x7a9be5u;break;} // d8ed fsubr st(5)
case 0x7a9be5u:{st[0]=product(st[0],st[5]);pc=0x7a9be7u;break;} // d8cd fmul st(5)
case 0x7a9be7u:{st[0]=sum(st[0],fp((r[7]+8u)));pc=0x7a9bebu;break;} // d8442408 fadd dword ptr [esp + 8]
case 0x7a9bebu:{st[1]=difference(st[1],st[0]);popfp();pc=0x7a9bedu;break;} // dee9 fsubp st(1)
case 0x7a9bedu:{compare(st[0],st[1]);pc=0x7a9befu;break;} // d8d1 fcom st(1)
case 0x7a9befu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9bf1u;break;} // dfe0 fnstsw ax
case 0x7a9bf1u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9bf4u;break;} // f6c441 test ah, 0x41
case 0x7a9bf4u:{pc=(!zf)?0x7a9c56u:0x7a9bf6u;break;} // 7560 jne 0x7a9c56
case 0x7a9bf6u:{st[1]=st[0];popfp();pc=0x7a9bf8u;break;} // ddd9 fstp st(1)
case 0x7a9bf8u:{st[0]=root(st[0]);pc=0x7a9bfau;break;} // d9fa fsqrt
case 0x7a9bfau:{pushfp(st[1]);pc=0x7a9bfcu;break;} // d9c1 fld st(1)
case 0x7a9bfcu:{st[0]=-st[0];pc=0x7a9bfeu;break;} // d9e0 fchs
case 0x7a9bfeu:{st[0]=difference(st[0],st[1]);pc=0x7a9c00u;break;} // d8e1 fsub st(1)
case 0x7a9c00u:{pushfp(st[0]);pc=0x7a9c02u;break;} // d9c0 fld st(0)
case 0x7a9c02u:{st[0]=product(st[0],st[6]);pc=0x7a9c04u;break;} // d8ce fmul st(6)
case 0x7a9c04u:{st[0]=sum(st[0],st[4]);pc=0x7a9c06u;break;} // d8c4 fadd st(4)
case 0x7a9c06u:{compare(st[0],st[5]);popfp();pc=0x7a9c08u;break;} // d8dd fcomp st(5)
case 0x7a9c08u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9c0au;break;} // dfe0 fnstsw ax
case 0x7a9c0au:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9c0du;break;} // f6c401 test ah, 1
case 0x7a9c0du:{pc=(!zf)?0x7a9c2du:0x7a9c0fu;break;} // 751e jne 0x7a9c2d
case 0x7a9c0fu:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9c13u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9c13u:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9c14u;break;} // 41 inc ecx
case 0x7a9c14u:{fpwrite((r[1]+r[2]*4u+4294967292u),st[0]);popfp();pc=0x7a9c18u;break;} // d95c8bfc fstp dword ptr [ebx + ecx*4 - 4]
case 0x7a9c18u:{uint32_t x=r[2],y=2u;flags(x-y);cf=x<y;pc=0x7a9c1bu;break;} // 83f902 cmp ecx, 2
case 0x7a9c1bu:{pc=(!zf)?0x7a9c2fu:0x7a9c1du;break;} // 7512 jne 0x7a9c2f
case 0x7a9c1du:{st[0]=st[0];popfp();pc=0x7a9c1fu;break;} // ddd8 fstp st(0)
case 0x7a9c1fu:{r[0]=r[2];pc=0x7a9c21u;break;} // 8bc1 mov eax, ecx
case 0x7a9c21u:{st[1]=st[0];popfp();pc=0x7a9c23u;break;} // ddd9 fstp st(1)
case 0x7a9c23u:{st[2]=st[0];popfp();pc=0x7a9c25u;break;} // ddda fstp st(2)
case 0x7a9c25u:{st[1]=st[0];popfp();pc=0x7a9c27u;break;} // ddd9 fstp st(1)
case 0x7a9c27u:{st[0]=st[0];popfp();pc=0x7a9c29u;break;} // ddd8 fstp st(0)
case 0x7a9c29u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9c2cu;break;} // 83c43c add esp, 0x3c
case 0x7a9c2cu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9c2du:{st[0]=st[0];popfp();pc=0x7a9c2fu;break;} // ddd8 fstp st(0)
case 0x7a9c2fu:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a9c31u;break;} // dee1 fsubrp st(1)
case 0x7a9c31u:{pushfp(st[0]);pc=0x7a9c33u;break;} // d9c0 fld st(0)
case 0x7a9c33u:{st[4]=product(st[4],st[0]);popfp();pc=0x7a9c35u;break;} // decc fmulp st(4)
case 0x7a9c35u:{std::swap(st[0],st[3]);pc=0x7a9c37u;break;} // d9cb fxch st(3)
case 0x7a9c37u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9c39u;break;} // dec1 faddp st(1)
case 0x7a9c39u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a9c3bu;break;} // ded9 fcompp
case 0x7a9c3bu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9c3du;break;} // dfe0 fnstsw ax
case 0x7a9c3du:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9c40u;break;} // f6c401 test ah, 1
case 0x7a9c40u:{pc=(!zf)?0x7a9c69u:0x7a9c42u;break;} // 7527 jne 0x7a9c69
case 0x7a9c42u:{st[0]=product(st[0],fp((r[7]+64u)));pc=0x7a9c46u;break;} // d84c2440 fmul dword ptr [esp + 0x40]
case 0x7a9c46u:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9c47u;break;} // 41 inc ecx
case 0x7a9c47u:{fpwrite((r[1]+r[2]*4u+4294967292u),st[0]);popfp();pc=0x7a9c4bu;break;} // d95c8bfc fstp dword ptr [ebx + ecx*4 - 4]
case 0x7a9c4bu:{uint32_t x=r[2],y=2u;flags(x-y);cf=x<y;pc=0x7a9c4eu;break;} // 83f902 cmp ecx, 2
case 0x7a9c4eu:{pc=(!zf)?0x7a9c6bu:0x7a9c50u;break;} // 751b jne 0x7a9c6b
case 0x7a9c50u:{r[0]=r[2];pc=0x7a9c52u;break;} // 8bc1 mov eax, ecx
case 0x7a9c52u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9c55u;break;} // 83c43c add esp, 0x3c
case 0x7a9c55u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9c56u:{compare(st[0],st[1]);popfp();popfp();pc=0x7a9c58u;break;} // dae9 fucompp
case 0x7a9c58u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9c5au;break;} // dfe0 fnstsw ax
case 0x7a9c5au:{flags(((r[0]>>8)&255u)&(68u));cf=false;pc=0x7a9c5du;break;} // f6c444 test ah, 0x44
case 0x7a9c5du:{pc=(pf)?0x7a9c63u:0x7a9c5fu;break;} // 7a04 jp 0x7a9c63
case 0x7a9c5fu:{st[0]=-st[0];pc=0x7a9c61u;break;} // d9e0 fchs
case 0x7a9c61u:{pc=(true)?0x7a9c31u:0x7a9c63u;break;} // ebce jmp 0x7a9c31
case 0x7a9c63u:{st[1]=st[0];popfp();pc=0x7a9c65u;break;} // ddd9 fstp st(1)
case 0x7a9c65u:{st[2]=st[0];popfp();pc=0x7a9c67u;break;} // ddda fstp st(2)
case 0x7a9c67u:{st[1]=st[0];popfp();pc=0x7a9c69u;break;} // ddd9 fstp st(1)
case 0x7a9c69u:{st[0]=st[0];popfp();pc=0x7a9c6bu;break;} // ddd8 fstp st(0)
case 0x7a9c6bu:{r[0]=r[2];pc=0x7a9c6du;break;} // 8bc1 mov eax, ecx
case 0x7a9c6du:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9c70u;break;} // 83c43c add esp, 0x3c
case 0x7a9c70u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9c71u:{st[0]=st[0];popfp();pc=0x7a9c73u;break;} // ddd8 fstp st(0)
case 0x7a9c73u:{st[5]=st[0];popfp();pc=0x7a9c75u;break;} // dddd fstp st(5)
case 0x7a9c75u:{pushfp(fp((r[5]+16u)));pc=0x7a9c78u;break;} // d94710 fld dword ptr [edi + 0x10]
case 0x7a9c78u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7a9c7bu;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7a9c7bu:{pushfp(fp((r[4])));pc=0x7a9c7du;break;} // d906 fld dword ptr [esi]
case 0x7a9c7du:{st[0]=product(st[0],fp((r[5]+12u)));pc=0x7a9c80u;break;} // d84f0c fmul dword ptr [edi + 0xc]
case 0x7a9c80u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9c82u;break;} // dec1 faddp st(1)
case 0x7a9c82u:{pushfp(fp((r[5]+20u)));pc=0x7a9c85u;break;} // d94714 fld dword ptr [edi + 0x14]
case 0x7a9c85u:{st[0]=product(st[0],fp((r[4]+8u)));pc=0x7a9c88u;break;} // d84e08 fmul dword ptr [esi + 8]
case 0x7a9c88u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7a9c8au;break;} // dec1 faddp st(1)
case 0x7a9c8au:{pushfp(st[1]);pc=0x7a9c8cu;break;} // d9c1 fld st(1)
case 0x7a9c8cu:{st[2]=product(st[2],st[0]);popfp();pc=0x7a9c8eu;break;} // deca fmulp st(2)
case 0x7a9c8eu:{std::swap(st[0],st[5]);pc=0x7a9c90u;break;} // d9cd fxch st(5)
case 0x7a9c90u:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a9c92u;break;} // dee1 fsubrp st(1)
case 0x7a9c92u:{pushfp(st[1]);pc=0x7a9c94u;break;} // d9c1 fld st(1)
case 0x7a9c94u:{st[2]=product(st[2],st[0]);popfp();pc=0x7a9c96u;break;} // deca fmulp st(2)
case 0x7a9c96u:{st[1]=difference(st[0],st[1]);popfp();pc=0x7a9c98u;break;} // dee1 fsubrp st(1)
case 0x7a9c98u:{pushfp(0.);pc=0x7a9c9au;break;} // d9ee fldz
case 0x7a9c9au:{compare(st[0],st[4]);pc=0x7a9c9cu;break;} // d8d4 fcom st(4)
case 0x7a9c9cu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9c9eu;break;} // dfe0 fnstsw ax
case 0x7a9c9eu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9ca1u;break;} // f6c441 test ah, 0x41
case 0x7a9ca1u:{pc=(!zf)?0x7a9cd3u:0x7a9ca3u;break;} // 7530 jne 0x7a9cd3
case 0x7a9ca3u:{compare(st[0],st[1]);pc=0x7a9ca5u;break;} // d8d1 fcom st(1)
case 0x7a9ca5u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9ca7u;break;} // dfe0 fnstsw ax
case 0x7a9ca7u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7a9caau;break;} // f6c441 test ah, 0x41
case 0x7a9caau:{pc=(pf)?0x7a9cd3u:0x7a9cacu;break;} // 7a27 jp 0x7a9cd3
case 0x7a9cacu:{st[4]=st[0];popfp();pc=0x7a9caeu;break;} // dddc fstp st(4)
case 0x7a9caeu:{r[0]=2u;pc=0x7a9cb3u;break;} // b802000000 mov eax, 2
case 0x7a9cb3u:{st[3]=st[0];popfp();pc=0x7a9cb5u;break;} // dddb fstp st(3)
case 0x7a9cb5u:{std::swap(st[0],st[2]);pc=0x7a9cb7u;break;} // d9ca fxch st(2)
case 0x7a9cb7u:{st[0]=root(st[0]);pc=0x7a9cb9u;break;} // d9fa fsqrt
case 0x7a9cb9u:{pushfp(st[2]);pc=0x7a9cbbu;break;} // d9c2 fld st(2)
case 0x7a9cbbu:{st[0]=sum(st[0],st[1]);pc=0x7a9cbdu;break;} // d8c1 fadd st(1)
case 0x7a9cbdu:{st[0]=product(st[0],st[2]);pc=0x7a9cbfu;break;} // d8ca fmul st(2)
case 0x7a9cbfu:{fpwrite((r[1]),st[0]);popfp();pc=0x7a9cc1u;break;} // d91b fstp dword ptr [ebx]
case 0x7a9cc1u:{pushfp(fp((r[7])));pc=0x7a9cc4u;break;} // d90424 fld dword ptr [esp]
case 0x7a9cc4u:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a9cc6u;break;} // dee3 fsubrp st(3)
case 0x7a9cc6u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9cc8u;break;} // dec2 faddp st(2)
case 0x7a9cc8u:{st[1]=product(st[1],st[0]);popfp();pc=0x7a9ccau;break;} // dec9 fmulp st(1)
case 0x7a9ccau:{st[0]=-st[0];pc=0x7a9cccu;break;} // d9e0 fchs
case 0x7a9cccu:{fpwrite((r[1]+4u),st[0]);popfp();pc=0x7a9ccfu;break;} // d95b04 fstp dword ptr [ebx + 4]
case 0x7a9ccfu:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9cd2u;break;} // 83c43c add esp, 0x3c
case 0x7a9cd2u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9cd3u:{compare(st[0],st[4]);pc=0x7a9cd5u;break;} // d8d4 fcom st(4)
case 0x7a9cd5u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9cd7u;break;} // dfe0 fnstsw ax
case 0x7a9cd7u:{st[4]=st[0];popfp();pc=0x7a9cd9u;break;} // dddc fstp st(4)
case 0x7a9cd9u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x7a9cdcu;break;} // f6c405 test ah, 5
case 0x7a9cdcu:{pc=(pf)?0x7a9d0cu:0x7a9cdeu;break;} // 7a2e jp 0x7a9d0c
case 0x7a9cdeu:{compare(st[0],st[3]);pc=0x7a9ce0u;break;} // d8d3 fcom st(3)
case 0x7a9ce0u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9ce2u;break;} // dfe0 fnstsw ax
case 0x7a9ce2u:{st[3]=st[0];popfp();pc=0x7a9ce4u;break;} // dddb fstp st(3)
case 0x7a9ce4u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9ce7u;break;} // f6c401 test ah, 1
case 0x7a9ce7u:{pc=(!zf)?0x7a9d1au:0x7a9ce9u;break;} // 7531 jne 0x7a9d1a
case 0x7a9ce9u:{std::swap(st[0],st[2]);pc=0x7a9cebu;break;} // d9ca fxch st(2)
case 0x7a9cebu:{r[0]=2u;pc=0x7a9cf0u;break;} // b802000000 mov eax, 2
case 0x7a9cf0u:{st[0]=root(st[0]);pc=0x7a9cf2u;break;} // d9fa fsqrt
case 0x7a9cf2u:{pushfp(st[2]);pc=0x7a9cf4u;break;} // d9c2 fld st(2)
case 0x7a9cf4u:{st[0]=sum(st[0],st[1]);pc=0x7a9cf6u;break;} // d8c1 fadd st(1)
case 0x7a9cf6u:{st[0]=product(st[0],st[2]);pc=0x7a9cf8u;break;} // d8ca fmul st(2)
case 0x7a9cf8u:{st[0]=-st[0];pc=0x7a9cfau;break;} // d9e0 fchs
case 0x7a9cfau:{fpwrite((r[1]),st[0]);popfp();pc=0x7a9cfcu;break;} // d91b fstp dword ptr [ebx]
case 0x7a9cfcu:{pushfp(fp((r[7])));pc=0x7a9cffu;break;} // d90424 fld dword ptr [esp]
case 0x7a9cffu:{st[3]=difference(st[0],st[3]);popfp();pc=0x7a9d01u;break;} // dee3 fsubrp st(3)
case 0x7a9d01u:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9d03u;break;} // dec2 faddp st(2)
case 0x7a9d03u:{st[1]=product(st[1],st[0]);popfp();pc=0x7a9d05u;break;} // dec9 fmulp st(1)
case 0x7a9d05u:{fpwrite((r[1]+4u),st[0]);popfp();pc=0x7a9d08u;break;} // d95b04 fstp dword ptr [ebx + 4]
case 0x7a9d08u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9d0bu;break;} // 83c43c add esp, 0x3c
case 0x7a9d0bu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9d0cu:{st[0]=st[0];popfp();pc=0x7a9d0eu;break;} // ddd8 fstp st(0)
case 0x7a9d0eu:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a9d10u;break;} // 33c0 xor eax, eax
case 0x7a9d10u:{st[0]=st[0];popfp();pc=0x7a9d12u;break;} // ddd8 fstp st(0)
case 0x7a9d12u:{st[1]=st[0];popfp();pc=0x7a9d14u;break;} // ddd9 fstp st(1)
case 0x7a9d14u:{st[0]=st[0];popfp();pc=0x7a9d16u;break;} // ddd8 fstp st(0)
case 0x7a9d16u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9d19u;break;} // 83c43c add esp, 0x3c
case 0x7a9d19u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9d1au:{st[2]=st[0];popfp();pc=0x7a9d1cu;break;} // ddda fstp st(2)
case 0x7a9d1cu:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x7a9d1eu;break;} // 33c0 xor eax, eax
case 0x7a9d1eu:{st[1]=st[0];popfp();pc=0x7a9d20u;break;} // ddd9 fstp st(1)
case 0x7a9d20u:{st[0]=st[0];popfp();pc=0x7a9d22u;break;} // ddd8 fstp st(0)
case 0x7a9d22u:{uint32_t v=(r[7])+(60u);r[7]=v;flags(v);pc=0x7a9d25u;break;} // 83c43c add esp, 0x3c
case 0x7a9d25u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7a9d30u:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x7a9d31u;break;} // 53 push ebx
case 0x7a9d31u:{r[1]=get((r[7]+24u),4);pc=0x7a9d35u;break;} // 8b5c2418 mov ebx, dword ptr [esp + 0x18]
case 0x7a9d35u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a9d36u;break;} // 55 push ebp
case 0x7a9d36u:{r[6]=get((r[7]+12u),4);pc=0x7a9d3au;break;} // 8b6c240c mov ebp, dword ptr [esp + 0xc]
case 0x7a9d3au:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7a9d3bu;break;} // 56 push esi
case 0x7a9d3bu:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x7a9d3cu;break;} // 57 push edi
case 0x7a9d3cu:{r[5]=get((r[7]+24u),4);pc=0x7a9d40u;break;} // 8b7c2418 mov edi, dword ptr [esp + 0x18]
case 0x7a9d40u:{r[4]=(r[6]+12u);pc=0x7a9d43u;break;} // 8d750c lea esi, [ebp + 0xc]
case 0x7a9d43u:{auto v=r[6];r[7]-=4;put(r[7],v,4);pc=0x7a9d44u;break;} // 55 push ebp
case 0x7a9d44u:{r[7]-=4;put(r[7],0x7a9d49u,4);pc=0x7a98c0u;break;} // e877fbffff call 0x7a98c0
case 0x7a9d49u:{r[2]=get((r[7]+32u),4);pc=0x7a9d4du;break;} // 8b4c2420 mov ecx, dword ptr [esp + 0x20]
case 0x7a9d4du:{put((r[2]),r[0],4);pc=0x7a9d4fu;break;} // 8901 mov dword ptr [ecx], eax
case 0x7a9d4fu:{uint32_t v=(r[7])+(4u);r[7]=v;flags(v);pc=0x7a9d52u;break;} // 83c404 add esp, 4
case 0x7a9d52u:{uint32_t v=(r[5])^(r[5]);r[5]=v;flags(v);pc=0x7a9d54u;break;} // 33ff xor edi, edi
case 0x7a9d54u:{uint32_t v=(r[2])^(r[2]);r[2]=v;flags(v);pc=0x7a9d56u;break;} // 33c9 xor ecx, ecx
case 0x7a9d56u:{flags((r[0])&(r[0]));cf=false;pc=0x7a9d58u;break;} // 85c0 test eax, eax
case 0x7a9d58u:{pc=(zf)?0x7a9db7u:0x7a9d5au;break;} // 745d je 0x7a9db7
case 0x7a9d5au:{pushfp(0.);pc=0x7a9d5cu;break;} // d9ee fldz
case 0x7a9d5cu:{r[3]=get((r[7]+32u),4);pc=0x7a9d60u;break;} // 8b542420 mov edx, dword ptr [esp + 0x20]
case 0x7a9d60u:{pushfp(1.);pc=0x7a9d62u;break;} // d9e8 fld1
case 0x7a9d62u:{pushfp(fp((r[1]+r[2]*4u)));pc=0x7a9d65u;break;} // d9048b fld dword ptr [ebx + ecx*4]
case 0x7a9d65u:{compare(st[0],st[2]);popfp();pc=0x7a9d67u;break;} // d8da fcomp st(2)
case 0x7a9d67u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9d69u;break;} // dfe0 fnstsw ax
case 0x7a9d69u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9d6cu;break;} // f6c401 test ah, 1
case 0x7a9d6cu:{pc=(!zf)?0x7a9daau:0x7a9d6eu;break;} // 753c jne 0x7a9daa
case 0x7a9d6eu:{compare(st[0],fp((r[1]+r[2]*4u)));pc=0x7a9d71u;break;} // d8148b fcom dword ptr [ebx + ecx*4]
case 0x7a9d71u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7a9d73u;break;} // dfe0 fnstsw ax
case 0x7a9d73u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x7a9d76u;break;} // f6c401 test ah, 1
case 0x7a9d76u:{pc=(!zf)?0x7a9daau:0x7a9d78u;break;} // 7532 jne 0x7a9daa
case 0x7a9d78u:{pushfp(fp((r[1]+r[2]*4u)));pc=0x7a9d7bu;break;} // d9048b fld dword ptr [ebx + ecx*4]
case 0x7a9d7bu:{r[0]=r[3];pc=0x7a9d7du;break;} // 8bc2 mov eax, edx
case 0x7a9d7du:{pushfp(fp((r[4])));pc=0x7a9d7fu;break;} // d906 fld dword ptr [esi]
case 0x7a9d7fu:{uint32_t v=(r[5])+1u;r[5]=v;flags(v);pc=0x7a9d80u;break;} // 47 inc edi
case 0x7a9d80u:{pushfp(fp((r[4]+4u)));pc=0x7a9d83u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7a9d83u:{uint32_t v=(r[3])+(12u);r[3]=v;flags(v);pc=0x7a9d86u;break;} // 83c20c add edx, 0xc
case 0x7a9d86u:{pushfp(fp((r[4]+8u)));pc=0x7a9d89u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7a9d89u:{std::swap(st[0],st[2]);pc=0x7a9d8bu;break;} // d9ca fxch st(2)
case 0x7a9d8bu:{st[0]=product(st[0],st[3]);pc=0x7a9d8du;break;} // d8cb fmul st(3)
case 0x7a9d8du:{std::swap(st[0],st[1]);pc=0x7a9d8fu;break;} // d9c9 fxch st(1)
case 0x7a9d8fu:{st[0]=product(st[0],st[3]);pc=0x7a9d91u;break;} // d8cb fmul st(3)
case 0x7a9d91u:{std::swap(st[0],st[2]);pc=0x7a9d93u;break;} // d9ca fxch st(2)
case 0x7a9d93u:{st[3]=product(st[3],st[0]);popfp();pc=0x7a9d95u;break;} // decb fmulp st(3)
case 0x7a9d95u:{st[0]=sum(st[0],fp((r[6])));pc=0x7a9d98u;break;} // d84500 fadd dword ptr [ebp]
case 0x7a9d98u:{pushfp(fp((r[6]+4u)));pc=0x7a9d9bu;break;} // d94504 fld dword ptr [ebp + 4]
case 0x7a9d9bu:{st[2]=sum(st[2],st[0]);popfp();pc=0x7a9d9du;break;} // dec2 faddp st(2)
case 0x7a9d9du:{pushfp(fp((r[6]+8u)));pc=0x7a9da0u;break;} // d94508 fld dword ptr [ebp + 8]
case 0x7a9da0u:{st[3]=sum(st[3],st[0]);popfp();pc=0x7a9da2u;break;} // dec3 faddp st(3)
case 0x7a9da2u:{fpwrite((r[0]),st[0]);popfp();pc=0x7a9da4u;break;} // d918 fstp dword ptr [eax]
case 0x7a9da4u:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7a9da7u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7a9da7u:{fpwrite((r[0]+8u),st[0]);popfp();pc=0x7a9daau;break;} // d95808 fstp dword ptr [eax + 8]
case 0x7a9daau:{r[0]=get((r[7]+28u),4);pc=0x7a9daeu;break;} // 8b44241c mov eax, dword ptr [esp + 0x1c]
case 0x7a9daeu:{uint32_t v=(r[2])+1u;r[2]=v;flags(v);pc=0x7a9dafu;break;} // 41 inc ecx
case 0x7a9dafu:{uint32_t x=r[2],y=get((r[0]),4);flags(x-y);cf=x<y;pc=0x7a9db1u;break;} // 3b08 cmp ecx, dword ptr [eax]
case 0x7a9db1u:{pc=(cf)?0x7a9d62u:0x7a9db3u;break;} // 72af jb 0x7a9d62
case 0x7a9db3u:{st[1]=st[0];popfp();pc=0x7a9db5u;break;} // ddd9 fstp st(1)
case 0x7a9db5u:{st[0]=st[0];popfp();pc=0x7a9db7u;break;} // ddd8 fstp st(0)
case 0x7a9db7u:{r[2]=get((r[7]+28u),4);pc=0x7a9dbbu;break;} // 8b4c241c mov ecx, dword ptr [esp + 0x1c]
case 0x7a9dbbu:{uint32_t v=(r[3])^(r[3]);r[3]=v;flags(v);pc=0x7a9dbdu;break;} // 33d2 xor edx, edx
case 0x7a9dbdu:{uint32_t x=r[3],y=r[5];flags(x-y);cf=x<y;pc=0x7a9dbfu;break;} // 3bd7 cmp edx, edi
case 0x7a9dbfu:{put((r[2]),r[5],4);pc=0x7a9dc1u;break;} // 8939 mov dword ptr [ecx], edi
case 0x7a9dc1u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x7a9dc2u;break;} // 5f pop edi
case 0x7a9dc2u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7a9dc3u;break;} // 5e pop esi
case 0x7a9dc3u:{uint32_t v=(r[0])-(r[0])-uint32_t(cf);r[0]=v;flags(v);pc=0x7a9dc5u;break;} // 1bc0 sbb eax, eax
case 0x7a9dc5u:{auto v=get(r[7],4);r[6]=v;r[7]+=4;pc=0x7a9dc6u;break;} // 5d pop ebp
case 0x7a9dc6u:{uint32_t v=0u-(r[0]);cf=v!=0;r[0]=v;flags(v);pc=0x7a9dc8u;break;} // f7d8 neg eax
case 0x7a9dc8u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x7a9dc9u;break;} // 5b pop ebx
case 0x7a9dc9u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7aa200u:{uint32_t x=get((r[7]+16u),1),y=0u;flags(x-y);cf=x<y;pc=0x7aa205u;break;} // 807c241000 cmp byte ptr [esp + 0x10], 0
case 0x7aa205u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x7aa206u;break;} // 56 push esi
case 0x7aa206u:{r[4]=get((r[7]+16u),4);pc=0x7aa20au;break;} // 8b742410 mov esi, dword ptr [esp + 0x10]
case 0x7aa20au:{pc=(!zf)?0x7aa213u:0x7aa20cu;break;} // 7507 jne 0x7aa213
case 0x7aa20cu:{r[2]=r[4];pc=0x7aa20eu;break;} // 8bce mov ecx, esi
case 0x7aa20eu:{r[7]-=4;put(r[7],0x7aa213u,4);pc=0x509130u;break;} // e81defd5ff call 0x509130
case 0x7aa213u:{pushfp(fp((r[4])));pc=0x7aa215u;break;} // d906 fld dword ptr [esi]
case 0x7aa215u:{r[2]=get((r[7]+12u),4);pc=0x7aa219u;break;} // 8b4c240c mov ecx, dword ptr [esp + 0xc]
case 0x7aa219u:{st[0]=std::abs(st[0]);pc=0x7aa21bu;break;} // d9e1 fabs
case 0x7aa21bu:{pushfp(fp((r[4]+4u)));pc=0x7aa21eu;break;} // d94604 fld dword ptr [esi + 4]
case 0x7aa21eu:{st[0]=std::abs(st[0]);pc=0x7aa220u;break;} // d9e1 fabs
case 0x7aa220u:{compare(st[0],st[1]);popfp();popfp();pc=0x7aa222u;break;} // ded9 fcompp
case 0x7aa222u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x7aa224u;break;} // dfe0 fnstsw ax
case 0x7aa224u:{pushfp(fp((r[4]+8u)));pc=0x7aa227u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7aa227u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x7aa22au;break;} // f6c441 test ah, 0x41
case 0x7aa22au:{r[0]=get((r[7]+8u),4);pc=0x7aa22eu;break;} // 8b442408 mov eax, dword ptr [esp + 8]
case 0x7aa22eu:{pc=(pf)?0x7aa273u:0x7aa230u;break;} // 7a43 jp 0x7aa273
case 0x7aa230u:{pushfp(fp((r[4])));pc=0x7aa232u;break;} // d906 fld dword ptr [esi]
case 0x7aa232u:{pushfp(st[1]);pc=0x7aa234u;break;} // d9c1 fld st(1)
case 0x7aa234u:{st[2]=product(st[2],st[0]);popfp();pc=0x7aa236u;break;} // deca fmulp st(2)
case 0x7aa236u:{st[0]=product(st[0],st[0]);pc=0x7aa238u;break;} // dcc8 fmul st(0), st(0)
case 0x7aa238u:{st[1]=sum(st[1],st[0]);popfp();pc=0x7aa23au;break;} // dec1 faddp st(1)
case 0x7aa23au:{st[0]=root(st[0]);pc=0x7aa23cu;break;} // d9fa fsqrt
case 0x7aa23cu:{pushfp(1.);pc=0x7aa23eu;break;} // d9e8 fld1
case 0x7aa23eu:{st[1]=quotient(st[0],st[1]);popfp();pc=0x7aa240u;break;} // def1 fdivrp st(1)
case 0x7aa240u:{pushfp(fp((r[4]+8u)));pc=0x7aa243u;break;} // d94608 fld dword ptr [esi + 8]
case 0x7aa243u:{st[0]=product(st[0],st[1]);pc=0x7aa245u;break;} // d8c9 fmul st(1)
case 0x7aa245u:{st[0]=-st[0];pc=0x7aa247u;break;} // d9e0 fchs
case 0x7aa247u:{fpwrite((r[0]),st[0]);popfp();pc=0x7aa249u;break;} // d918 fstp dword ptr [eax]
case 0x7aa249u:{pushfp(0.);pc=0x7aa24bu;break;} // d9ee fldz
case 0x7aa24bu:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7aa24eu;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7aa24eu:{st[0]=product(st[0],fp((r[4])));pc=0x7aa250u;break;} // d80e fmul dword ptr [esi]
case 0x7aa250u:{fpwrite((r[0]+8u),st[0]);pc=0x7aa253u;break;} // d95008 fst dword ptr [eax + 8]
case 0x7aa253u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7aa256u;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7aa256u:{fpwrite((r[2]),st[0]);popfp();pc=0x7aa258u;break;} // d919 fstp dword ptr [ecx]
case 0x7aa258u:{pushfp(fp((r[4]+8u)));pc=0x7aa25bu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7aa25bu:{st[0]=product(st[0],fp((r[0])));pc=0x7aa25du;break;} // d808 fmul dword ptr [eax]
case 0x7aa25du:{pushfp(fp((r[0]+8u)));pc=0x7aa260u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7aa260u:{st[0]=product(st[0],fp((r[4])));pc=0x7aa262u;break;} // d80e fmul dword ptr [esi]
case 0x7aa262u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7aa264u;break;} // dee9 fsubp st(1)
case 0x7aa264u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x7aa267u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x7aa267u:{pushfp(fp((r[0])));pc=0x7aa269u;break;} // d900 fld dword ptr [eax]
case 0x7aa269u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7aa26cu;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7aa26cu:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7aa26du;break;} // 5e pop esi
case 0x7aa26du:{st[0]=-st[0];pc=0x7aa26fu;break;} // d9e0 fchs
case 0x7aa26fu:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x7aa272u;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x7aa272u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x7aa273u:{pushfp(fp((r[4]+4u)));pc=0x7aa276u;break;} // d94604 fld dword ptr [esi + 4]
case 0x7aa276u:{st[0]=product(st[0],st[0]);pc=0x7aa278u;break;} // dcc8 fmul st(0), st(0)
case 0x7aa278u:{pushfp(st[1]);pc=0x7aa27au;break;} // d9c1 fld st(1)
case 0x7aa27au:{st[2]=product(st[2],st[0]);popfp();pc=0x7aa27cu;break;} // deca fmulp st(2)
case 0x7aa27cu:{st[1]=sum(st[1],st[0]);popfp();pc=0x7aa27eu;break;} // dec1 faddp st(1)
case 0x7aa27eu:{st[0]=root(st[0]);pc=0x7aa280u;break;} // d9fa fsqrt
case 0x7aa280u:{pushfp(1.);pc=0x7aa282u;break;} // d9e8 fld1
case 0x7aa282u:{st[1]=quotient(st[0],st[1]);popfp();pc=0x7aa284u;break;} // def1 fdivrp st(1)
case 0x7aa284u:{pushfp(0.);pc=0x7aa286u;break;} // d9ee fldz
case 0x7aa286u:{fpwrite((r[0]),st[0]);popfp();pc=0x7aa288u;break;} // d918 fstp dword ptr [eax]
case 0x7aa288u:{pushfp(fp((r[4]+8u)));pc=0x7aa28bu;break;} // d94608 fld dword ptr [esi + 8]
case 0x7aa28bu:{st[0]=product(st[0],st[1]);pc=0x7aa28du;break;} // d8c9 fmul st(1)
case 0x7aa28du:{fpwrite((r[0]+4u),st[0]);popfp();pc=0x7aa290u;break;} // d95804 fstp dword ptr [eax + 4]
case 0x7aa290u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7aa293u;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7aa293u:{st[0]=-st[0];pc=0x7aa295u;break;} // d9e0 fchs
case 0x7aa295u:{fpwrite((r[0]+8u),st[0]);pc=0x7aa298u;break;} // d95008 fst dword ptr [eax + 8]
case 0x7aa298u:{st[0]=product(st[0],fp((r[4]+4u)));pc=0x7aa29bu;break;} // d84e04 fmul dword ptr [esi + 4]
case 0x7aa29bu:{pushfp(fp((r[0]+4u)));pc=0x7aa29eu;break;} // d94004 fld dword ptr [eax + 4]
case 0x7aa29eu:{st[0]=product(st[0],fp((r[4]+8u)));pc=0x7aa2a1u;break;} // d84e08 fmul dword ptr [esi + 8]
case 0x7aa2a1u:{st[1]=difference(st[1],st[0]);popfp();pc=0x7aa2a3u;break;} // dee9 fsubp st(1)
case 0x7aa2a3u:{fpwrite((r[2]),st[0]);popfp();pc=0x7aa2a5u;break;} // d919 fstp dword ptr [ecx]
case 0x7aa2a5u:{pushfp(fp((r[0]+8u)));pc=0x7aa2a8u;break;} // d94008 fld dword ptr [eax + 8]
case 0x7aa2a8u:{st[0]=product(st[0],fp((r[4])));pc=0x7aa2aau;break;} // d80e fmul dword ptr [esi]
case 0x7aa2aau:{st[0]=-st[0];pc=0x7aa2acu;break;} // d9e0 fchs
case 0x7aa2acu:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x7aa2afu;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x7aa2afu:{pushfp(fp((r[0]+4u)));pc=0x7aa2b2u;break;} // d94004 fld dword ptr [eax + 4]
case 0x7aa2b2u:{st[0]=product(st[0],fp((r[4])));pc=0x7aa2b4u;break;} // d80e fmul dword ptr [esi]
case 0x7aa2b4u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x7aa2b5u;break;} // 5e pop esi
case 0x7aa2b5u:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x7aa2b8u;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x7aa2b8u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x10021339u:{pushfp(fp((r[3])));pc=0x1002133bu;break;} // d902 fld dword ptr [edx]
case 0x1002133bu:{st[0]=difference(st[0],fp((r[1])));pc=0x1002133du;break;} // d823 fsub dword ptr [ebx]
case 0x1002133du:{pushfp(fp((r[3]+4u)));pc=0x10021340u;break;} // d94204 fld dword ptr [edx + 4]
case 0x10021340u:{st[0]=difference(st[0],fp((r[1]+4u)));pc=0x10021343u;break;} // d86304 fsub dword ptr [ebx + 4]
case 0x10021343u:{pushfp(fp((r[3]+8u)));pc=0x10021346u;break;} // d94208 fld dword ptr [edx + 8]
case 0x10021346u:{st[0]=difference(st[0],fp((r[1]+8u)));pc=0x10021349u;break;} // d86308 fsub dword ptr [ebx + 8]
case 0x10021349u:{pushfp(fp((r[5])));pc=0x1002134bu;break;} // d907 fld dword ptr [edi]
case 0x1002134bu:{st[0]=difference(st[0],fp((r[3])));pc=0x1002134du;break;} // d822 fsub dword ptr [edx]
case 0x1002134du:{pushfp(fp((r[5]+4u)));pc=0x10021350u;break;} // d94704 fld dword ptr [edi + 4]
case 0x10021350u:{st[0]=difference(st[0],fp((r[3]+4u)));pc=0x10021353u;break;} // d86204 fsub dword ptr [edx + 4]
case 0x10021353u:{pushfp(fp((r[5]+8u)));pc=0x10021356u;break;} // d94708 fld dword ptr [edi + 8]
case 0x10021356u:{st[0]=difference(st[0],fp((r[3]+8u)));pc=0x10021359u;break;} // d86208 fsub dword ptr [edx + 8]
case 0x10021359u:{pushfp(st[0]);pc=0x1002135bu;break;} // d9c0 fld st(0)
case 0x1002135bu:{st[0]=product(st[0],st[5]);pc=0x1002135du;break;} // d8cd fmul st(5)
case 0x1002135du:{pushfp(st[2]);pc=0x1002135fu;break;} // d9c2 fld st(2)
case 0x1002135fu:{st[0]=product(st[0],st[5]);pc=0x10021361u;break;} // d8cd fmul st(5)
case 0x10021361u:{st[1]=difference(st[1],st[0]);popfp();pc=0x10021363u;break;} // dee9 fsubp st(1)
case 0x10021363u:{std::swap(st[0],st[4]);pc=0x10021365u;break;} // d9cc fxch st(4)
case 0x10021365u:{st[0]=product(st[0],st[3]);pc=0x10021367u;break;} // d8cb fmul st(3)
case 0x10021367u:{std::swap(st[0],st[1]);pc=0x10021369u;break;} // d9c9 fxch st(1)
case 0x10021369u:{st[0]=product(st[0],st[6]);pc=0x1002136bu;break;} // d8ce fmul st(6)
case 0x1002136bu:{st[1]=difference(st[1],st[0]);popfp();pc=0x1002136du;break;} // dee9 fsubp st(1)
case 0x1002136du:{std::swap(st[0],st[1]);pc=0x1002136fu;break;} // d9c9 fxch st(1)
case 0x1002136fu:{st[5]=product(st[5],st[0]);popfp();pc=0x10021371u;break;} // decd fmulp st(5)
case 0x10021371u:{std::swap(st[0],st[3]);pc=0x10021373u;break;} // d9cb fxch st(3)
case 0x10021373u:{st[1]=product(st[1],st[0]);popfp();pc=0x10021375u;break;} // dec9 fmulp st(1)
case 0x10021375u:{st[3]=difference(st[3],st[0]);popfp();pc=0x10021377u;break;} // deeb fsubp st(3)
case 0x10021377u:{fpwrite((r[2]),st[0]);pc=0x10021379u;break;} // d911 fst dword ptr [ecx]
case 0x10021379u:{std::swap(st[0],st[1]);pc=0x1002137bu;break;} // d9c9 fxch st(1)
case 0x1002137bu:{fpwrite((r[2]+4u),st[0]);pc=0x1002137eu;break;} // d95104 fst dword ptr [ecx + 4]
case 0x1002137eu:{std::swap(st[0],st[2]);pc=0x10021380u;break;} // d9ca fxch st(2)
case 0x10021380u:{fpwrite((r[2]+8u),st[0]);pc=0x10021383u;break;} // d95108 fst dword ptr [ecx + 8]
case 0x10021383u:{pushfp(st[2]);pc=0x10021385u;break;} // d9c2 fld st(2)
case 0x10021385u:{st[0]=product(st[0],st[3]);pc=0x10021387u;break;} // d8cb fmul st(3)
case 0x10021387u:{pushfp(st[2]);pc=0x10021389u;break;} // d9c2 fld st(2)
case 0x10021389u:{st[0]=product(st[0],st[3]);pc=0x1002138bu;break;} // d8cb fmul st(3)
case 0x1002138bu:{st[1]=sum(st[1],st[0]);popfp();pc=0x1002138du;break;} // dec1 faddp st(1)
case 0x1002138du:{pushfp(st[1]);pc=0x1002138fu;break;} // d9c1 fld st(1)
case 0x1002138fu:{st[0]=product(st[0],st[2]);pc=0x10021391u;break;} // d8ca fmul st(2)
case 0x10021391u:{st[1]=sum(st[1],st[0]);popfp();pc=0x10021393u;break;} // dec1 faddp st(1)
case 0x10021393u:{st[0]=root(st[0]);pc=0x10021395u;break;} // d9fa fsqrt
case 0x10021395u:{pushfp(fp((269687832u)));pc=0x1002139bu;break;} // d905181c1310 fld dword ptr [0x10131c18]
case 0x1002139bu:{compare(st[0],st[1]);popfp();pc=0x1002139du;break;} // d8d9 fcomp st(1)
case 0x1002139du:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x1002139fu;break;} // dfe0 fnstsw ax
case 0x1002139fu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100213a2u;break;} // f6c405 test ah, 5
case 0x100213a2u:{pc=(pf)?0x100213cau:0x100213a4u;break;} // 7a26 jp 0x100213ca
case 0x100213a4u:{pushfp(fp((269687828u)));pc=0x100213aau;break;} // d905141c1310 fld dword ptr [0x10131c14]
case 0x100213aau:{compare(st[0],st[1]);popfp();pc=0x100213acu;break;} // d8d9 fcomp st(1)
case 0x100213acu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100213aeu;break;} // dfe0 fnstsw ax
case 0x100213aeu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100213b1u;break;} // f6c441 test ah, 0x41
case 0x100213b1u:{pc=(!zf)?0x100213cau:0x100213b3u;break;} // 7517 jne 0x100213ca
case 0x100213b3u:{st[0]=st[0];popfp();pc=0x100213b5u;break;} // ddd8 fstp st(0)
case 0x100213b5u:{st[1]=st[0];popfp();pc=0x100213b7u;break;} // ddd9 fstp st(1)
case 0x100213b7u:{st[1]=st[0];popfp();pc=0x100213b9u;break;} // ddd9 fstp st(1)
case 0x100213b9u:{st[0]=st[0];popfp();pc=0x100213bbu;break;} // ddd8 fstp st(0)
case 0x100213bbu:{pushfp(0.);pc=0x100213bdu;break;} // d9ee fldz
case 0x100213bdu:{fpwrite((r[2]+12u),st[0]);pc=0x100213c0u;break;} // d9510c fst dword ptr [ecx + 0xc]
case 0x100213c0u:{fpwrite((r[2]+8u),st[0]);pc=0x100213c3u;break;} // d95108 fst dword ptr [ecx + 8]
case 0x100213c3u:{fpwrite((r[2]+4u),st[0]);pc=0x100213c6u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x100213c6u:{fpwrite((r[2]),st[0]);pc=0x100213c8u;break;} // d911 fst dword ptr [ecx]
case 0x100213c8u:{pc=(true)?0x100213deu:0x100213cau;break;} // eb14 jmp 0x100213de
case 0x100213cau:{st[2]=quotient(st[2],st[0]);pc=0x100213ccu;break;} // dcfa fdiv st(2), st(0)
case 0x100213ccu:{std::swap(st[0],st[2]);pc=0x100213ceu;break;} // d9ca fxch st(2)
case 0x100213ceu:{fpwrite((r[2]),st[0]);popfp();pc=0x100213d0u;break;} // d919 fstp dword ptr [ecx]
case 0x100213d0u:{std::swap(st[0],st[2]);pc=0x100213d2u;break;} // d9ca fxch st(2)
case 0x100213d2u:{st[0]=quotient(st[0],st[1]);pc=0x100213d4u;break;} // d8f1 fdiv st(1)
case 0x100213d4u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x100213d7u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x100213d7u:{st[1]=quotient(st[1],st[0]);popfp();pc=0x100213d9u;break;} // def9 fdivp st(1)
case 0x100213d9u:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x100213dcu;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x100213dcu:{pushfp(0.);pc=0x100213deu;break;} // d9ee fldz
case 0x100213deu:{pushfp(fp((r[2]+4u)));pc=0x100213e1u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x100213e1u:{st[0]=product(st[0],fp((r[5]+4u)));pc=0x100213e4u;break;} // d84f04 fmul dword ptr [edi + 4]
case 0x100213e4u:{pushfp(fp((r[2])));pc=0x100213e6u;break;} // d901 fld dword ptr [ecx]
case 0x100213e6u:{st[0]=product(st[0],fp((r[5])));pc=0x100213e8u;break;} // d80f fmul dword ptr [edi]
case 0x100213e8u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100213eau;break;} // dec1 faddp st(1)
case 0x100213eau:{pushfp(fp((r[5]+8u)));pc=0x100213edu;break;} // d94708 fld dword ptr [edi + 8]
case 0x100213edu:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x100213f0u;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x100213f0u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100213f2u;break;} // dec1 faddp st(1)
case 0x100213f2u:{fpwrite((r[2]+12u),st[0]);popfp();pc=0x100213f5u;break;} // d9590c fstp dword ptr [ecx + 0xc]
