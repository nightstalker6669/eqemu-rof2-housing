case 0x100c4d70u:{pushfp(fp((269694652u)));pc=0x100c4d76u;break;} // d905bc361310 fld dword ptr [0x101336bc]
case 0x100c4d76u:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x100c4d77u;break;} // 53 push ebx
case 0x100c4d77u:{pushfp(st[0]);pc=0x100c4d79u;break;} // d9c0 fld st(0)
case 0x100c4d79u:{uint32_t v=(r[1])^(r[1]);r[1]=v;flags(v);pc=0x100c4d7bu;break;} // 33db xor ebx, ebx
case 0x100c4d7bu:{pushfp(st[1]);pc=0x100c4d7du;break;} // d9c1 fld st(1)
case 0x100c4d7du:{pushfp(fp((269694648u)));pc=0x100c4d83u;break;} // d905b8361310 fld dword ptr [0x101336b8]
case 0x100c4d83u:{pushfp(st[0]);pc=0x100c4d85u;break;} // d9c0 fld st(0)
case 0x100c4d85u:{pushfp(st[1]);pc=0x100c4d87u;break;} // d9c1 fld st(1)
case 0x100c4d87u:{uint32_t x=get((r[2]+108u),4),y=r[1];flags(x-y);cf=x<y;pc=0x100c4d8au;break;} // 39596c cmp dword ptr [ecx + 0x6c], ebx
case 0x100c4d8au:{pc=(cf||zf)?0x100c4e46u:0x100c4d90u;break;} // 0f86b6000000 jbe 0x100c4e46
case 0x100c4d90u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x100c4d91u;break;} // 56 push esi
case 0x100c4d91u:{r[4]=get((r[2]+116u),4);pc=0x100c4d94u;break;} // 8b7174 mov esi, dword ptr [ecx + 0x74]
case 0x100c4d94u:{auto v=r[5];r[7]-=4;put(r[7],v,4);pc=0x100c4d95u;break;} // 57 push edi
case 0x100c4d95u:{r[5]=get((r[7]+16u),4);pc=0x100c4d99u;break;} // 8b7c2410 mov edi, dword ptr [esp + 0x10]
case 0x100c4d99u:{uint32_t v=(r[3])^(r[3]);r[3]=v;flags(v);pc=0x100c4d9bu;break;} // 33d2 xor edx, edx
case 0x100c4d9bu:{uint32_t v=(r[5])+(8u);r[5]=v;flags(v);pc=0x100c4d9eu;break;} // 83c708 add edi, 8
case 0x100c4d9eu:{pushfp(fp((r[5]+4294967288u)));pc=0x100c4da1u;break;} // d947f8 fld dword ptr [edi - 8]
case 0x100c4da1u:{fpwrite((r[3]+r[4]),st[0]);popfp();pc=0x100c4da4u;break;} // d91c32 fstp dword ptr [edx + esi]
case 0x100c4da4u:{r[0]=get((r[2]+116u),4);pc=0x100c4da7u;break;} // 8b4174 mov eax, dword ptr [ecx + 0x74]
case 0x100c4da7u:{pushfp(fp((r[5]+4294967292u)));pc=0x100c4daau;break;} // d947fc fld dword ptr [edi - 4]
case 0x100c4daau:{fpwrite((r[0]+r[3]+4u),st[0]);popfp();pc=0x100c4daeu;break;} // d95c1004 fstp dword ptr [eax + edx + 4]
case 0x100c4daeu:{r[0]=get((r[2]+116u),4);pc=0x100c4db1u;break;} // 8b4174 mov eax, dword ptr [ecx + 0x74]
case 0x100c4db1u:{pushfp(fp((r[5])));pc=0x100c4db3u;break;} // d907 fld dword ptr [edi]
case 0x100c4db3u:{fpwrite((r[0]+r[3]+8u),st[0]);popfp();pc=0x100c4db7u;break;} // d95c1008 fstp dword ptr [eax + edx + 8]
case 0x100c4db7u:{r[4]=get((r[2]+116u),4);pc=0x100c4dbau;break;} // 8b7174 mov esi, dword ptr [ecx + 0x74]
case 0x100c4dbau:{std::swap(st[0],st[5]);pc=0x100c4dbcu;break;} // d9cd fxch st(5)
case 0x100c4dbcu:{compare(st[0],fp((r[3]+r[4])));pc=0x100c4dbfu;break;} // d81432 fcom dword ptr [edx + esi]
case 0x100c4dbfu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4dc1u;break;} // dfe0 fnstsw ax
case 0x100c4dc1u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100c4dc4u;break;} // f6c441 test ah, 0x41
case 0x100c4dc4u:{pc=(!zf)?0x100c4dcbu:0x100c4dc6u;break;} // 7505 jne 0x100c4dcb
case 0x100c4dc6u:{st[0]=st[0];popfp();pc=0x100c4dc8u;break;} // ddd8 fstp st(0)
case 0x100c4dc8u:{pushfp(fp((r[3]+r[4])));pc=0x100c4dcbu;break;} // d90432 fld dword ptr [edx + esi]
case 0x100c4dcbu:{std::swap(st[0],st[3]);pc=0x100c4dcdu;break;} // d9cb fxch st(3)
case 0x100c4dcdu:{compare(st[0],fp((r[3]+r[4]+4u)));pc=0x100c4dd1u;break;} // d8543204 fcom dword ptr [edx + esi + 4]
case 0x100c4dd1u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4dd3u;break;} // dfe0 fnstsw ax
case 0x100c4dd3u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100c4dd6u;break;} // f6c441 test ah, 0x41
case 0x100c4dd6u:{pc=(!zf)?0x100c4ddeu:0x100c4dd8u;break;} // 7506 jne 0x100c4dde
case 0x100c4dd8u:{st[0]=st[0];popfp();pc=0x100c4ddau;break;} // ddd8 fstp st(0)
case 0x100c4ddau:{pushfp(fp((r[3]+r[4]+4u)));pc=0x100c4ddeu;break;} // d9443204 fld dword ptr [edx + esi + 4]
case 0x100c4ddeu:{std::swap(st[0],st[4]);pc=0x100c4de0u;break;} // d9cc fxch st(4)
case 0x100c4de0u:{compare(st[0],fp((r[3]+r[4]+8u)));pc=0x100c4de4u;break;} // d8543208 fcom dword ptr [edx + esi + 8]
case 0x100c4de4u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4de6u;break;} // dfe0 fnstsw ax
case 0x100c4de6u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100c4de9u;break;} // f6c441 test ah, 0x41
case 0x100c4de9u:{pc=(!zf)?0x100c4df1u:0x100c4debu;break;} // 7506 jne 0x100c4df1
case 0x100c4debu:{st[0]=st[0];popfp();pc=0x100c4dedu;break;} // ddd8 fstp st(0)
case 0x100c4dedu:{pushfp(fp((r[3]+r[4]+8u)));pc=0x100c4df1u;break;} // d9443208 fld dword ptr [edx + esi + 8]
case 0x100c4df1u:{std::swap(st[0],st[2]);pc=0x100c4df3u;break;} // d9ca fxch st(2)
case 0x100c4df3u:{compare(st[0],fp((r[3]+r[4])));pc=0x100c4df6u;break;} // d81432 fcom dword ptr [edx + esi]
case 0x100c4df6u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4df8u;break;} // dfe0 fnstsw ax
case 0x100c4df8u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100c4dfbu;break;} // f6c405 test ah, 5
case 0x100c4dfbu:{pc=(pf)?0x100c4e02u:0x100c4dfdu;break;} // 7a05 jp 0x100c4e02
case 0x100c4dfdu:{st[0]=st[0];popfp();pc=0x100c4dffu;break;} // ddd8 fstp st(0)
case 0x100c4dffu:{pushfp(fp((r[3]+r[4])));pc=0x100c4e02u;break;} // d90432 fld dword ptr [edx + esi]
case 0x100c4e02u:{std::swap(st[0],st[5]);pc=0x100c4e04u;break;} // d9cd fxch st(5)
case 0x100c4e04u:{compare(st[0],fp((r[3]+r[4]+4u)));pc=0x100c4e08u;break;} // d8543204 fcom dword ptr [edx + esi + 4]
case 0x100c4e08u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4e0au;break;} // dfe0 fnstsw ax
case 0x100c4e0au:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100c4e0du;break;} // f6c405 test ah, 5
case 0x100c4e0du:{pc=(pf)?0x100c4e15u:0x100c4e0fu;break;} // 7a06 jp 0x100c4e15
case 0x100c4e0fu:{st[0]=st[0];popfp();pc=0x100c4e11u;break;} // ddd8 fstp st(0)
case 0x100c4e11u:{pushfp(fp((r[3]+r[4]+4u)));pc=0x100c4e15u;break;} // d9443204 fld dword ptr [edx + esi + 4]
case 0x100c4e15u:{std::swap(st[0],st[1]);pc=0x100c4e17u;break;} // d9c9 fxch st(1)
case 0x100c4e17u:{compare(st[0],fp((r[3]+r[4]+8u)));pc=0x100c4e1bu;break;} // d8543208 fcom dword ptr [edx + esi + 8]
case 0x100c4e1bu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c4e1du;break;} // dfe0 fnstsw ax
case 0x100c4e1du:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100c4e20u;break;} // f6c405 test ah, 5
case 0x100c4e20u:{pc=(pf)?0x100c4e28u:0x100c4e22u;break;} // 7a06 jp 0x100c4e28
case 0x100c4e22u:{st[0]=st[0];popfp();pc=0x100c4e24u;break;} // ddd8 fstp st(0)
case 0x100c4e24u:{pushfp(fp((r[3]+r[4]+8u)));pc=0x100c4e28u;break;} // d9443208 fld dword ptr [edx + esi + 8]
case 0x100c4e28u:{std::swap(st[0],st[3]);pc=0x100c4e2au;break;} // d9cb fxch st(3)
case 0x100c4e2au:{uint32_t v=(r[1])+1u;r[1]=v;flags(v);pc=0x100c4e2bu;break;} // 43 inc ebx
case 0x100c4e2bu:{std::swap(st[0],st[5]);pc=0x100c4e2du;break;} // d9cd fxch st(5)
case 0x100c4e2du:{uint32_t v=(r[5])+(44u);r[5]=v;flags(v);pc=0x100c4e30u;break;} // 83c72c add edi, 0x2c
case 0x100c4e30u:{std::swap(st[0],st[2]);pc=0x100c4e32u;break;} // d9ca fxch st(2)
case 0x100c4e32u:{uint32_t v=(r[3])+(12u);r[3]=v;flags(v);pc=0x100c4e35u;break;} // 83c20c add edx, 0xc
case 0x100c4e35u:{std::swap(st[0],st[4]);pc=0x100c4e37u;break;} // d9cc fxch st(4)
case 0x100c4e37u:{std::swap(st[0],st[3]);pc=0x100c4e39u;break;} // d9cb fxch st(3)
case 0x100c4e39u:{std::swap(st[0],st[1]);pc=0x100c4e3bu;break;} // d9c9 fxch st(1)
case 0x100c4e3bu:{uint32_t x=r[1],y=get((r[2]+108u),4);flags(x-y);cf=x<y;pc=0x100c4e3eu;break;} // 3b596c cmp ebx, dword ptr [ecx + 0x6c]
case 0x100c4e3eu:{pc=(cf)?0x100c4d9eu:0x100c4e44u;break;} // 0f825affffff jb 0x100c4d9e
case 0x100c4e44u:{auto v=get(r[7],4);r[5]=v;r[7]+=4;pc=0x100c4e45u;break;} // 5f pop edi
case 0x100c4e45u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x100c4e46u;break;} // 5e pop esi
case 0x100c4e46u:{std::swap(st[0],st[2]);pc=0x100c4e48u;break;} // d9ca fxch st(2)
case 0x100c4e48u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x100c4e49u;break;} // 5b pop ebx
case 0x100c4e49u:{st[0]=difference(st[0],st[5]);pc=0x100c4e4bu;break;} // d8e5 fsub st(5)
case 0x100c4e4bu:{std::swap(st[0],st[2]);pc=0x100c4e4du;break;} // d9ca fxch st(2)
case 0x100c4e4du:{st[0]=difference(st[0],st[3]);pc=0x100c4e4fu;break;} // d8e3 fsub st(3)
case 0x100c4e4fu:{std::swap(st[0],st[1]);pc=0x100c4e51u;break;} // d9c9 fxch st(1)
case 0x100c4e51u:{st[0]=difference(st[0],st[4]);pc=0x100c4e53u;break;} // d8e4 fsub st(4)
case 0x100c4e53u:{pushfp(fp((269687836u)));pc=0x100c4e59u;break;} // d9051c1c1310 fld dword ptr [0x10131c1c]
case 0x100c4e59u:{st[3]=product(st[3],st[0]);pc=0x100c4e5bu;break;} // dccb fmul st(3), st(0)
case 0x100c4e5bu:{st[2]=product(st[2],st[0]);pc=0x100c4e5du;break;} // dcca fmul st(2), st(0)
case 0x100c4e5du:{st[1]=product(st[1],st[0]);popfp();pc=0x100c4e5fu;break;} // dec9 fmulp st(1)
case 0x100c4e5fu:{pushfp(st[2]);pc=0x100c4e61u;break;} // d9c2 fld st(2)
case 0x100c4e61u:{st[6]=sum(st[6],st[0]);popfp();pc=0x100c4e63u;break;} // dec6 faddp st(6)
case 0x100c4e63u:{pushfp(st[1]);pc=0x100c4e65u;break;} // d9c1 fld st(1)
case 0x100c4e65u:{st[4]=sum(st[4],st[0]);popfp();pc=0x100c4e67u;break;} // dec4 faddp st(4)
case 0x100c4e67u:{pushfp(st[0]);pc=0x100c4e69u;break;} // d9c0 fld st(0)
case 0x100c4e69u:{st[5]=sum(st[5],st[0]);popfp();pc=0x100c4e6bu;break;} // dec5 faddp st(5)
case 0x100c4e6bu:{std::swap(st[0],st[5]);pc=0x100c4e6du;break;} // d9cd fxch st(5)
case 0x100c4e6du:{fpwrite((r[2]+12u),st[0]);popfp();pc=0x100c4e70u;break;} // d9590c fstp dword ptr [ecx + 0xc]
case 0x100c4e70u:{std::swap(st[0],st[2]);pc=0x100c4e72u;break;} // d9ca fxch st(2)
case 0x100c4e72u:{fpwrite((r[2]+16u),st[0]);popfp();pc=0x100c4e75u;break;} // d95910 fstp dword ptr [ecx + 0x10]
case 0x100c4e75u:{std::swap(st[0],st[2]);pc=0x100c4e77u;break;} // d9ca fxch st(2)
case 0x100c4e77u:{fpwrite((r[2]+20u),st[0]);popfp();pc=0x100c4e7au;break;} // d95914 fstp dword ptr [ecx + 0x14]
case 0x100c4e7au:{st[0]=product(st[0],st[0]);pc=0x100c4e7cu;break;} // dcc8 fmul st(0), st(0)
case 0x100c4e7cu:{pushfp(st[1]);pc=0x100c4e7eu;break;} // d9c1 fld st(1)
case 0x100c4e7eu:{st[2]=product(st[2],st[0]);popfp();pc=0x100c4e80u;break;} // deca fmulp st(2)
case 0x100c4e80u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c4e82u;break;} // dec1 faddp st(1)
case 0x100c4e82u:{pushfp(st[1]);pc=0x100c4e84u;break;} // d9c1 fld st(1)
case 0x100c4e84u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c4e86u;break;} // deca fmulp st(2)
case 0x100c4e86u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c4e88u;break;} // dec1 faddp st(1)
case 0x100c4e88u:{st[0]=root(st[0]);pc=0x100c4e8au;break;} // d9fa fsqrt
case 0x100c4e8au:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x100c4e8du;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x100c4e8du:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x100c5469u:{pushfp(fp((r[1]+16u)));pc=0x100c546cu;break;} // d94310 fld dword ptr [ebx + 0x10]
case 0x100c546cu:{r[2]=get((r[1]+96u),4);pc=0x100c546fu;break;} // 8b4b60 mov ecx, dword ptr [ebx + 0x60]
case 0x100c546fu:{pushfp(fp((r[1]+12u)));pc=0x100c5472u;break;} // d9430c fld dword ptr [ebx + 0xc]
case 0x100c5472u:{r[0]=(r[7]+48u);pc=0x100c5476u;break;} // 8d442430 lea eax, [esp + 0x30]
case 0x100c5476u:{pushfp(fp((r[1]+20u)));pc=0x100c5479u;break;} // d94314 fld dword ptr [ebx + 0x14]
case 0x100c5479u:{pushfp(st[1]);pc=0x100c547bu;break;} // d9c1 fld st(1)
case 0x100c547bu:{st[2]=product(st[2],st[0]);popfp();pc=0x100c547du;break;} // deca fmulp st(2)
case 0x100c547du:{pushfp(st[2]);pc=0x100c547fu;break;} // d9c2 fld st(2)
case 0x100c547fu:{st[3]=product(st[3],st[0]);popfp();pc=0x100c5481u;break;} // decb fmulp st(3)
case 0x100c5481u:{std::swap(st[0],st[1]);pc=0x100c5483u;break;} // d9c9 fxch st(1)
case 0x100c5483u:{st[2]=sum(st[2],st[0]);popfp();pc=0x100c5485u;break;} // dec2 faddp st(2)
case 0x100c5485u:{st[0]=product(st[0],st[0]);pc=0x100c5487u;break;} // dcc8 fmul st(0), st(0)
case 0x100c5487u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c5489u;break;} // dec1 faddp st(1)
case 0x100c5489u:{st[0]=root(st[0]);pc=0x100c548bu;break;} // d9fa fsqrt
case 0x100c548bu:{pushfp(fp((r[1]+8u)));pc=0x100c548eu;break;} // d94308 fld dword ptr [ebx + 8]
case 0x100c548eu:{st[0]=sum(st[0],st[0]);pc=0x100c5490u;break;} // dcc0 fadd st(0), st(0)
case 0x100c5490u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c5492u;break;} // dec1 faddp st(1)
case 0x100c5492u:{fpwrite((r[7]+32u),st[0]);pc=0x100c5496u;break;} // d9542420 fst dword ptr [esp + 0x20]
case 0x100c5496u:{pushfp(0.);pc=0x100c5498u;break;} // d9ee fldz
case 0x100c5498u:{fpwrite((r[7]+48u),st[0]);pc=0x100c549cu;break;} // d9542430 fst dword ptr [esp + 0x30]
case 0x100c549cu:{fpwrite((r[7]+52u),st[0]);pc=0x100c54a0u;break;} // d9542434 fst dword ptr [esp + 0x34]
case 0x100c54a0u:{fpwrite((r[7]+56u),st[0]);popfp();pc=0x100c54a4u;break;} // d95c2438 fstp dword ptr [esp + 0x38]
case 0x100c54a4u:{fpwrite((r[7]),st[0]);popfp();pc=0x100c54a7u;break;} // d91c24 fstp dword ptr [esp]
case 0x100c54a7u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x100c54a8u;break;} // 50 push eax
case 0x100d14a0u:{uint32_t v=(r[7])-(12u);r[7]=v;flags(v);pc=0x100d14a3u;break;} // 83ec0c sub esp, 0xc
case 0x100d14a3u:{pushfp(fp((r[7]+20u)));pc=0x100d14a7u;break;} // d9442414 fld dword ptr [esp + 0x14]
case 0x100d14a7u:{auto v=r[1];r[7]-=4;put(r[7],v,4);pc=0x100d14a8u;break;} // 53 push ebx
case 0x100d14a8u:{r[1]=get((r[7]+40u),4);pc=0x100d14acu;break;} // 8b5c2428 mov ebx, dword ptr [esp + 0x28]
case 0x100d14acu:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x100d14adu;break;} // 56 push esi
case 0x100d14adu:{r[4]=r[2];pc=0x100d14afu;break;} // 8bf1 mov esi, ecx
case 0x100d14afu:{st[0]=product(st[0],fp((r[4]+28u)));pc=0x100d14b2u;break;} // d84e1c fmul dword ptr [esi + 0x1c]
case 0x100d14b2u:{st[0]=sum(st[0],fp((r[4]+32u)));pc=0x100d14b5u;break;} // d84620 fadd dword ptr [esi + 0x20]
case 0x100d14b5u:{fpwrite((r[7]+8u),st[0]);pc=0x100d14b9u;break;} // d9542408 fst dword ptr [esp + 8]
case 0x100d14b9u:{pushfp(fp((r[7]+32u)));pc=0x100d14bdu;break;} // d9442420 fld dword ptr [esp + 0x20]
case 0x100d14bdu:{st[0]=product(st[0],fp((r[4]+28u)));pc=0x100d14c0u;break;} // d84e1c fmul dword ptr [esi + 0x1c]
case 0x100d14c0u:{st[0]=sum(st[0],fp((r[4]+36u)));pc=0x100d14c3u;break;} // d84624 fadd dword ptr [esi + 0x24]
case 0x100d14c3u:{fpwrite((r[7]+12u),st[0]);pc=0x100d14c7u;break;} // d954240c fst dword ptr [esp + 0xc]
case 0x100d14c7u:{pushfp(fp((r[7]+36u)));pc=0x100d14cbu;break;} // d9442424 fld dword ptr [esp + 0x24]
case 0x100d14cbu:{st[0]=product(st[0],fp((r[4]+28u)));pc=0x100d14ceu;break;} // d84e1c fmul dword ptr [esi + 0x1c]
case 0x100d14ceu:{st[0]=sum(st[0],fp((r[4]+40u)));pc=0x100d14d1u;break;} // d84628 fadd dword ptr [esi + 0x28]
case 0x100d14d1u:{fpwrite((r[7]+16u),st[0]);pc=0x100d14d5u;break;} // d9542410 fst dword ptr [esp + 0x10]
case 0x100d14d5u:{std::swap(st[0],st[2]);pc=0x100d14d7u;break;} // d9ca fxch st(2)
case 0x100d14d7u:{st[0]=difference(st[0],fp((r[1]+4u)));pc=0x100d14dau;break;} // d86304 fsub dword ptr [ebx + 4]
case 0x100d14dau:{std::swap(st[0],st[1]);pc=0x100d14dcu;break;} // d9c9 fxch st(1)
case 0x100d14dcu:{st[0]=difference(st[0],fp((r[1]+8u)));pc=0x100d14dfu;break;} // d86308 fsub dword ptr [ebx + 8]
case 0x100d14dfu:{std::swap(st[0],st[2]);pc=0x100d14e1u;break;} // d9ca fxch st(2)
case 0x100d14e1u:{st[0]=difference(st[0],fp((r[1]+12u)));pc=0x100d14e4u;break;} // d8630c fsub dword ptr [ebx + 0xc]
case 0x100d14e4u:{pushfp(st[1]);pc=0x100d14e6u;break;} // d9c1 fld st(1)
case 0x100d14e6u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d14e8u;break;} // deca fmulp st(2)
case 0x100d14e8u:{pushfp(st[2]);pc=0x100d14eau;break;} // d9c2 fld st(2)
case 0x100d14eau:{st[3]=product(st[3],st[0]);popfp();pc=0x100d14ecu;break;} // decb fmulp st(3)
case 0x100d14ecu:{std::swap(st[0],st[1]);pc=0x100d14eeu;break;} // d9c9 fxch st(1)
case 0x100d14eeu:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d14f0u;break;} // dec2 faddp st(2)
case 0x100d14f0u:{st[0]=product(st[0],st[0]);pc=0x100d14f2u;break;} // dcc8 fmul st(0), st(0)
case 0x100d14f2u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d14f4u;break;} // dec1 faddp st(1)
case 0x100d14f4u:{st[0]=root(st[0]);pc=0x100d14f6u;break;} // d9fa fsqrt
case 0x100d14f6u:{st[0]=sum(st[0],fp((r[1])));pc=0x100d14f8u;break;} // d803 fadd dword ptr [ebx]
case 0x100d14f8u:{compare(st[0],fp((r[7]+40u)));popfp();pc=0x100d14fcu;break;} // d85c2428 fcomp dword ptr [esp + 0x28]
case 0x100d14fcu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d14feu;break;} // dfe0 fnstsw ax
case 0x100d14feu:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100d1501u;break;} // f6c405 test ah, 5
case 0x100d1501u:{pc=(!pf)?0x100d150du:0x100d1503u;break;} // 7b0a jnp 0x100d150d
case 0x100d1503u:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x100d1504u;break;} // 5e pop esi
case 0x100d1504u:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x100d1506u;break;} // 33c0 xor eax, eax
case 0x100d1506u:{auto v=get(r[7],4);r[1]=v;r[7]+=4;pc=0x100d1507u;break;} // 5b pop ebx
case 0x100d1507u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x100d150au;break;} // 83c40c add esp, 0xc
case 0x100d150au:{pc=get(r[7],4);r[7]+=28u;if(pc==0xeeeeeeeeu)return;break;} // c21800 ret 0x18
case 0x100d93e0u:{r[3]=get((r[2]+20u),4);pc=0x100d93e3u;break;} // 8b5114 mov edx, dword ptr [ecx + 0x14]
case 0x100d93e3u:{r[0]=get((r[2]+24u),4);pc=0x100d93e6u;break;} // 8b4118 mov eax, dword ptr [ecx + 0x18]
case 0x100d93e6u:{auto v=r[4];r[7]-=4;put(r[7],v,4);pc=0x100d93e7u;break;} // 56 push esi
case 0x100d93e7u:{r[4]=get((r[2]+16u),4);pc=0x100d93eau;break;} // 8b7110 mov esi, dword ptr [ecx + 0x10]
case 0x100d93eau:{pushfp(fp((r[4])));pc=0x100d93ecu;break;} // d906 fld dword ptr [esi]
case 0x100d93ecu:{r[2]=get((r[7]+8u),4);pc=0x100d93f0u;break;} // 8b4c2408 mov ecx, dword ptr [esp + 8]
case 0x100d93f0u:{st[0]=sum(st[0],fp((r[3])));pc=0x100d93f2u;break;} // d802 fadd dword ptr [edx]
case 0x100d93f2u:{pushfp(fp((r[4]+4u)));pc=0x100d93f5u;break;} // d94604 fld dword ptr [esi + 4]
case 0x100d93f5u:{st[0]=sum(st[0],fp((r[3]+4u)));pc=0x100d93f8u;break;} // d84204 fadd dword ptr [edx + 4]
case 0x100d93f8u:{pushfp(fp((r[4]+8u)));pc=0x100d93fbu;break;} // d94608 fld dword ptr [esi + 8]
case 0x100d93fbu:{st[0]=sum(st[0],fp((r[3]+8u)));pc=0x100d93feu;break;} // d84208 fadd dword ptr [edx + 8]
case 0x100d93feu:{pushfp(fp((r[0])));pc=0x100d9400u;break;} // d900 fld dword ptr [eax]
case 0x100d9400u:{st[3]=sum(st[3],st[0]);popfp();pc=0x100d9402u;break;} // dec3 faddp st(3)
case 0x100d9402u:{pushfp(fp((r[0]+4u)));pc=0x100d9405u;break;} // d94004 fld dword ptr [eax + 4]
case 0x100d9405u:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d9407u;break;} // dec2 faddp st(2)
case 0x100d9407u:{st[0]=sum(st[0],fp((r[0]+8u)));pc=0x100d940au;break;} // d84008 fadd dword ptr [eax + 8]
case 0x100d940au:{pushfp(fp((269744216u)));pc=0x100d9410u;break;} // d90558f81310 fld dword ptr [0x1013f858]
case 0x100d9410u:{st[3]=product(st[3],st[0]);pc=0x100d9412u;break;} // dccb fmul st(3), st(0)
case 0x100d9412u:{st[2]=product(st[2],st[0]);pc=0x100d9414u;break;} // dcca fmul st(2), st(0)
case 0x100d9414u:{st[1]=product(st[1],st[0]);popfp();pc=0x100d9416u;break;} // dec9 fmulp st(1)
case 0x100d9416u:{std::swap(st[0],st[2]);pc=0x100d9418u;break;} // d9ca fxch st(2)
case 0x100d9418u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x100d941bu;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x100d941bu:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x100d941eu;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x100d941eu:{fpwrite((r[2]+12u),st[0]);popfp();pc=0x100d9421u;break;} // d9590c fstp dword ptr [ecx + 0xc]
case 0x100d9421u:{pushfp(fp((r[4])));pc=0x100d9423u;break;} // d906 fld dword ptr [esi]
case 0x100d9423u:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x100d9426u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x100d9426u:{pushfp(fp((r[4]+4u)));pc=0x100d9429u;break;} // d94604 fld dword ptr [esi + 4]
case 0x100d9429u:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x100d942cu;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x100d942cu:{pushfp(fp((r[4]+8u)));pc=0x100d942fu;break;} // d94608 fld dword ptr [esi + 8]
case 0x100d942fu:{auto v=get(r[7],4);r[4]=v;r[7]+=4;pc=0x100d9430u;break;} // 5e pop esi
case 0x100d9430u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x100d9433u;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x100d9433u:{pushfp(st[1]);pc=0x100d9435u;break;} // d9c1 fld st(1)
case 0x100d9435u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d9437u;break;} // deca fmulp st(2)
case 0x100d9437u:{pushfp(st[2]);pc=0x100d9439u;break;} // d9c2 fld st(2)
case 0x100d9439u:{st[3]=product(st[3],st[0]);popfp();pc=0x100d943bu;break;} // decb fmulp st(3)
case 0x100d943bu:{std::swap(st[0],st[1]);pc=0x100d943du;break;} // d9c9 fxch st(1)
case 0x100d943du:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d943fu;break;} // dec2 faddp st(2)
case 0x100d943fu:{st[0]=product(st[0],st[0]);pc=0x100d9441u;break;} // dcc8 fmul st(0), st(0)
case 0x100d9441u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d9443u;break;} // dec1 faddp st(1)
case 0x100d9443u:{pushfp(fp((r[3])));pc=0x100d9445u;break;} // d902 fld dword ptr [edx]
case 0x100d9445u:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x100d9448u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x100d9448u:{pushfp(fp((r[3]+4u)));pc=0x100d944bu;break;} // d94204 fld dword ptr [edx + 4]
case 0x100d944bu:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x100d944eu;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x100d944eu:{pushfp(fp((r[3]+8u)));pc=0x100d9451u;break;} // d94208 fld dword ptr [edx + 8]
case 0x100d9451u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x100d9454u;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x100d9454u:{pushfp(st[1]);pc=0x100d9456u;break;} // d9c1 fld st(1)
case 0x100d9456u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d9458u;break;} // deca fmulp st(2)
case 0x100d9458u:{pushfp(st[2]);pc=0x100d945au;break;} // d9c2 fld st(2)
case 0x100d945au:{st[3]=product(st[3],st[0]);popfp();pc=0x100d945cu;break;} // decb fmulp st(3)
case 0x100d945cu:{std::swap(st[0],st[1]);pc=0x100d945eu;break;} // d9c9 fxch st(1)
case 0x100d945eu:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d9460u;break;} // dec2 faddp st(2)
case 0x100d9460u:{st[0]=product(st[0],st[0]);pc=0x100d9462u;break;} // dcc8 fmul st(0), st(0)
case 0x100d9462u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d9464u;break;} // dec1 faddp st(1)
case 0x100d9464u:{pushfp(fp((r[0])));pc=0x100d9466u;break;} // d900 fld dword ptr [eax]
case 0x100d9466u:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x100d9469u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x100d9469u:{pushfp(fp((r[0]+4u)));pc=0x100d946cu;break;} // d94004 fld dword ptr [eax + 4]
case 0x100d946cu:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x100d946fu;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x100d946fu:{pushfp(fp((r[0]+8u)));pc=0x100d9472u;break;} // d94008 fld dword ptr [eax + 8]
case 0x100d9472u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x100d9475u;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x100d9475u:{pushfp(st[1]);pc=0x100d9477u;break;} // d9c1 fld st(1)
case 0x100d9477u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d9479u;break;} // deca fmulp st(2)
case 0x100d9479u:{pushfp(st[2]);pc=0x100d947bu;break;} // d9c2 fld st(2)
case 0x100d947bu:{st[3]=product(st[3],st[0]);popfp();pc=0x100d947du;break;} // decb fmulp st(3)
case 0x100d947du:{std::swap(st[0],st[1]);pc=0x100d947fu;break;} // d9c9 fxch st(1)
case 0x100d947fu:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d9481u;break;} // dec2 faddp st(2)
case 0x100d9481u:{st[0]=product(st[0],st[0]);pc=0x100d9483u;break;} // dcc8 fmul st(0), st(0)
case 0x100d9483u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d9485u;break;} // dec1 faddp st(1)
case 0x100d9485u:{std::swap(st[0],st[2]);pc=0x100d9487u;break;} // d9ca fxch st(2)
case 0x100d9487u:{compare(st[0],st[1]);pc=0x100d9489u;break;} // d8d1 fcom st(1)
case 0x100d9489u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d948bu;break;} // dfe0 fnstsw ax
case 0x100d948bu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100d948eu;break;} // f6c441 test ah, 0x41
case 0x100d948eu:{pc=(!zf)?0x100d9494u:0x100d9490u;break;} // 7504 jne 0x100d9494
case 0x100d9490u:{st[1]=st[0];popfp();pc=0x100d9492u;break;} // ddd9 fstp st(1)
case 0x100d9492u:{pc=(true)?0x100d9496u:0x100d9494u;break;} // eb02 jmp 0x100d9496
case 0x100d9494u:{st[0]=st[0];popfp();pc=0x100d9496u;break;} // ddd8 fstp st(0)
case 0x100d9496u:{compare(st[0],st[1]);pc=0x100d9498u;break;} // d8d1 fcom st(1)
case 0x100d9498u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d949au;break;} // dfe0 fnstsw ax
case 0x100d949au:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100d949du;break;} // f6c441 test ah, 0x41
case 0x100d949du:{pc=(zf)?0x100d94aau:0x100d949fu;break;} // 740b je 0x100d94aa
case 0x100d949fu:{st[0]=st[0];popfp();pc=0x100d94a1u;break;} // ddd8 fstp st(0)
case 0x100d94a1u:{r[0]=r[2];pc=0x100d94a3u;break;} // 8bc1 mov eax, ecx
case 0x100d94a3u:{st[0]=root(st[0]);pc=0x100d94a5u;break;} // d9fa fsqrt
case 0x100d94a5u:{fpwrite((r[2]),st[0]);popfp();pc=0x100d94a7u;break;} // d919 fstp dword ptr [ecx]
case 0x100d94a7u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
case 0x100d94aau:{st[1]=st[0];popfp();pc=0x100d94acu;break;} // ddd9 fstp st(1)
case 0x100d94acu:{r[0]=r[2];pc=0x100d94aeu;break;} // 8bc1 mov eax, ecx
case 0x100d94aeu:{st[0]=root(st[0]);pc=0x100d94b0u;break;} // d9fa fsqrt
case 0x100d94b0u:{fpwrite((r[2]),st[0]);popfp();pc=0x100d94b2u;break;} // d919 fstp dword ptr [ecx]
case 0x100d94b2u:{pc=get(r[7],4);r[7]+=8u;if(pc==0xeeeeeeeeu)return;break;} // c20400 ret 4
