case 0x1003ad20u:{pushfp(fp((r[2]+4u)));pc=0x1003ad23u;break;} // d94104 fld dword ptr [ecx + 4]
case 0x1003ad23u:{pushfp(fp((r[2])));pc=0x1003ad25u;break;} // d901 fld dword ptr [ecx]
case 0x1003ad25u:{pushfp(fp((r[2]+8u)));pc=0x1003ad28u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x1003ad28u:{pushfp(st[1]);pc=0x1003ad2au;break;} // d9c1 fld st(1)
case 0x1003ad2au:{st[2]=product(st[2],st[0]);popfp();pc=0x1003ad2cu;break;} // deca fmulp st(2)
case 0x1003ad2cu:{pushfp(st[2]);pc=0x1003ad2eu;break;} // d9c2 fld st(2)
case 0x1003ad2eu:{st[3]=product(st[3],st[0]);popfp();pc=0x1003ad30u;break;} // decb fmulp st(3)
case 0x1003ad30u:{std::swap(st[0],st[1]);pc=0x1003ad32u;break;} // d9c9 fxch st(1)
case 0x1003ad32u:{st[2]=sum(st[2],st[0]);popfp();pc=0x1003ad34u;break;} // dec2 faddp st(2)
case 0x1003ad34u:{st[0]=product(st[0],st[0]);pc=0x1003ad36u;break;} // dcc8 fmul st(0), st(0)
case 0x1003ad36u:{st[1]=sum(st[1],st[0]);popfp();pc=0x1003ad38u;break;} // dec1 faddp st(1)
case 0x1003ad38u:{st[0]=root(st[0]);pc=0x1003ad3au;break;} // d9fa fsqrt
case 0x1003ad3au:{pushfp(fp((269687832u)));pc=0x1003ad40u;break;} // d905181c1310 fld dword ptr [0x10131c18]
case 0x1003ad40u:{compare(st[0],st[1]);popfp();pc=0x1003ad42u;break;} // d8d9 fcomp st(1)
case 0x1003ad42u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x1003ad44u;break;} // dfe0 fnstsw ax
case 0x1003ad44u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x1003ad47u;break;} // f6c405 test ah, 5
case 0x1003ad47u:{pc=(pf)?0x1003ad65u:0x1003ad49u;break;} // 7a1c jp 0x1003ad65
case 0x1003ad49u:{pushfp(fp((269687828u)));pc=0x1003ad4fu;break;} // d905141c1310 fld dword ptr [0x10131c14]
case 0x1003ad4fu:{compare(st[0],st[1]);popfp();pc=0x1003ad51u;break;} // d8d9 fcomp st(1)
case 0x1003ad51u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x1003ad53u;break;} // dfe0 fnstsw ax
case 0x1003ad53u:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x1003ad56u;break;} // f6c441 test ah, 0x41
case 0x1003ad56u:{pc=(!zf)?0x1003ad65u:0x1003ad58u;break;} // 750d jne 0x1003ad65
case 0x1003ad58u:{st[0]=st[0];popfp();pc=0x1003ad5au;break;} // ddd8 fstp st(0)
case 0x1003ad5au:{pushfp(0.);pc=0x1003ad5cu;break;} // d9ee fldz
case 0x1003ad5cu:{fpwrite((r[2]+8u),st[0]);pc=0x1003ad5fu;break;} // d95108 fst dword ptr [ecx + 8]
case 0x1003ad5fu:{fpwrite((r[2]+4u),st[0]);pc=0x1003ad62u;break;} // d95104 fst dword ptr [ecx + 4]
case 0x1003ad62u:{fpwrite((r[2]),st[0]);pc=0x1003ad64u;break;} // d911 fst dword ptr [ecx]
case 0x1003ad64u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x1003ad65u:{pushfp(1.);pc=0x1003ad67u;break;} // d9e8 fld1
case 0x1003ad67u:{st[0]=quotient(st[0],st[1]);pc=0x1003ad69u;break;} // d8f1 fdiv st(1)
case 0x1003ad69u:{pushfp(fp((r[2])));pc=0x1003ad6bu;break;} // d901 fld dword ptr [ecx]
case 0x1003ad6bu:{st[0]=product(st[0],st[1]);pc=0x1003ad6du;break;} // d8c9 fmul st(1)
case 0x1003ad6du:{fpwrite((r[2]),st[0]);popfp();pc=0x1003ad6fu;break;} // d919 fstp dword ptr [ecx]
case 0x1003ad6fu:{pushfp(st[0]);pc=0x1003ad71u;break;} // d9c0 fld st(0)
case 0x1003ad71u:{st[0]=product(st[0],fp((r[2]+4u)));pc=0x1003ad74u;break;} // d84904 fmul dword ptr [ecx + 4]
case 0x1003ad74u:{fpwrite((r[2]+4u),st[0]);popfp();pc=0x1003ad77u;break;} // d95904 fstp dword ptr [ecx + 4]
case 0x1003ad77u:{st[0]=product(st[0],fp((r[2]+8u)));pc=0x1003ad7au;break;} // d84908 fmul dword ptr [ecx + 8]
case 0x1003ad7au:{fpwrite((r[2]+8u),st[0]);popfp();pc=0x1003ad7du;break;} // d95908 fstp dword ptr [ecx + 8]
case 0x1003ad7du:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
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
case 0x100c565au:{pushfp(fp((r[4])));pc=0x100c565cu;break;} // d906 fld dword ptr [esi]
case 0x100c565cu:{st[0]=difference(st[0],fp((r[5])));pc=0x100c565eu;break;} // d827 fsub dword ptr [edi]
case 0x100c565eu:{pushfp(fp((r[4]+4u)));pc=0x100c5661u;break;} // d94604 fld dword ptr [esi + 4]
case 0x100c5661u:{st[0]=difference(st[0],fp((r[5]+4u)));pc=0x100c5664u;break;} // d86704 fsub dword ptr [edi + 4]
case 0x100c5664u:{pushfp(fp((r[4]+8u)));pc=0x100c5667u;break;} // d94608 fld dword ptr [esi + 8]
case 0x100c5667u:{st[0]=difference(st[0],fp((r[5]+8u)));pc=0x100c566au;break;} // d86708 fsub dword ptr [edi + 8]
case 0x100c566au:{pushfp(st[2]);pc=0x100c566cu;break;} // d9c2 fld st(2)
case 0x100c566cu:{st[3]=product(st[3],st[0]);popfp();pc=0x100c566eu;break;} // decb fmulp st(3)
case 0x100c566eu:{pushfp(st[1]);pc=0x100c5670u;break;} // d9c1 fld st(1)
case 0x100c5670u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c5672u;break;} // deca fmulp st(2)
case 0x100c5672u:{std::swap(st[0],st[2]);pc=0x100c5674u;break;} // d9ca fxch st(2)
case 0x100c5674u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c5676u;break;} // dec1 faddp st(1)
case 0x100c5676u:{pushfp(st[1]);pc=0x100c5678u;break;} // d9c1 fld st(1)
case 0x100c5678u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c567au;break;} // deca fmulp st(2)
case 0x100c567au:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c567cu;break;} // dec1 faddp st(1)
case 0x100c567cu:{pushfp(fp((r[6])));pc=0x100c567fu;break;} // d94500 fld dword ptr [ebp]
case 0x100c567fu:{st[0]=difference(st[0],fp((r[5])));pc=0x100c5681u;break;} // d827 fsub dword ptr [edi]
case 0x100c5681u:{pushfp(fp((r[6]+4u)));pc=0x100c5684u;break;} // d94504 fld dword ptr [ebp + 4]
case 0x100c5684u:{st[0]=difference(st[0],fp((r[5]+4u)));pc=0x100c5687u;break;} // d86704 fsub dword ptr [edi + 4]
case 0x100c5687u:{pushfp(fp((r[6]+8u)));pc=0x100c568au;break;} // d94508 fld dword ptr [ebp + 8]
case 0x100c568au:{st[0]=difference(st[0],fp((r[5]+8u)));pc=0x100c568du;break;} // d86708 fsub dword ptr [edi + 8]
case 0x100c568du:{pushfp(st[2]);pc=0x100c568fu;break;} // d9c2 fld st(2)
case 0x100c568fu:{st[3]=product(st[3],st[0]);popfp();pc=0x100c5691u;break;} // decb fmulp st(3)
case 0x100c5691u:{pushfp(st[1]);pc=0x100c5693u;break;} // d9c1 fld st(1)
case 0x100c5693u:{st[2]=product(st[2],st[0]);popfp();pc=0x100c5695u;break;} // deca fmulp st(2)
case 0x100c5695u:{std::swap(st[0],st[2]);pc=0x100c5697u;break;} // d9ca fxch st(2)
case 0x100c5697u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c5699u;break;} // dec1 faddp st(1)
case 0x100c5699u:{pushfp(st[1]);pc=0x100c569bu;break;} // d9c1 fld st(1)
case 0x100c569bu:{st[2]=product(st[2],st[0]);popfp();pc=0x100c569du;break;} // deca fmulp st(2)
case 0x100c569du:{st[1]=sum(st[1],st[0]);popfp();pc=0x100c569fu;break;} // dec1 faddp st(1)
case 0x100c569fu:{compare(st[0],st[1]);pc=0x100c56a1u;break;} // d8d1 fcom st(1)
case 0x100c56a1u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100c56a3u;break;} // dfe0 fnstsw ax
case 0x100c56a3u:{flags(((r[0]>>8)&255u)&(5u));cf=false;pc=0x100c56a6u;break;} // f6c405 test ah, 5
case 0x100c56a6u:{pc=(pf)?0x100c56acu:0x100c56a8u;break;} // 7a04 jp 0x100c56ac
case 0x100c56a8u:{st[0]=st[0];popfp();pc=0x100c56aau;break;} // ddd8 fstp st(0)
case 0x100c56aau:{pc=(true)?0x100c56aeu:0x100c56acu;break;} // eb02 jmp 0x100c56ae
case 0x100c56acu:{st[1]=st[0];popfp();pc=0x100c56aeu;break;} // ddd9 fstp st(1)
case 0x100c56aeu:{st[0]=root(st[0]);pc=0x100c56b0u;break;} // d9fa fsqrt
case 0x100c56b0u:{fpwrite((r[2]+28u),st[0]);popfp();pc=0x100c56b3u;break;} // d9591c fstp dword ptr [ecx + 0x1c]
case 0x100d0200u:{uint32_t v=(r[7])-(24u);r[7]=v;flags(v);pc=0x100d0203u;break;} // 83ec18 sub esp, 0x18
case 0x100d0203u:{r[0]=get((r[7]+28u),4);pc=0x100d0207u;break;} // 8b44241c mov eax, dword ptr [esp + 0x1c]
case 0x100d0207u:{pushfp(fp((r[0]+4u)));pc=0x100d020au;break;} // d94004 fld dword ptr [eax + 4]
case 0x100d020au:{r[2]=get((r[7]+32u),4);pc=0x100d020eu;break;} // 8b4c2420 mov ecx, dword ptr [esp + 0x20]
case 0x100d020eu:{fpwrite((r[7]),st[0]);popfp();pc=0x100d0211u;break;} // d91c24 fstp dword ptr [esp]
case 0x100d0211u:{auto v=0u;r[7]-=4;put(r[7],v,4);pc=0x100d0213u;break;} // 6a00 push 0
case 0x100d0213u:{pushfp(fp((r[0]+8u)));pc=0x100d0216u;break;} // d94008 fld dword ptr [eax + 8]
case 0x100d0216u:{fpwrite((r[7]+8u),st[0]);popfp();pc=0x100d021au;break;} // d95c2408 fstp dword ptr [esp + 8]
case 0x100d021au:{pushfp(fp((r[0]+12u)));pc=0x100d021du;break;} // d9400c fld dword ptr [eax + 0xc]
case 0x100d021du:{fpwrite((r[7]+12u),st[0]);popfp();pc=0x100d0221u;break;} // d95c240c fstp dword ptr [esp + 0xc]
case 0x100d0221u:{pushfp(fp((r[2])));pc=0x100d0223u;break;} // d901 fld dword ptr [ecx]
case 0x100d0223u:{fpwrite((r[7]+16u),st[0]);popfp();pc=0x100d0227u;break;} // d95c2410 fstp dword ptr [esp + 0x10]
case 0x100d0227u:{pushfp(fp((r[2]+4u)));pc=0x100d022au;break;} // d94104 fld dword ptr [ecx + 4]
case 0x100d022au:{fpwrite((r[7]+20u),st[0]);popfp();pc=0x100d022eu;break;} // d95c2414 fstp dword ptr [esp + 0x14]
case 0x100d022eu:{pushfp(fp((r[2]+8u)));pc=0x100d0231u;break;} // d94108 fld dword ptr [ecx + 8]
case 0x100d0231u:{r[2]=(r[7]+4u);pc=0x100d0235u;break;} // 8d4c2404 lea ecx, [esp + 4]
case 0x100d0235u:{fpwrite((r[7]+24u),st[0]);popfp();pc=0x100d0239u;break;} // d95c2418 fstp dword ptr [esp + 0x18]
case 0x100d0239u:{auto v=r[2];r[7]-=4;put(r[7],v,4);pc=0x100d023au;break;} // 51 push ecx
case 0x100d023au:{pushfp(fp((r[0])));pc=0x100d023cu;break;} // d900 fld dword ptr [eax]
case 0x100d023cu:{r[0]=get((r[7]+44u),4);pc=0x100d0240u;break;} // 8b44242c mov eax, dword ptr [esp + 0x2c]
case 0x100d0240u:{st[0]=sum(st[0],fp((r[0])));pc=0x100d0242u;break;} // d800 fadd dword ptr [eax]
case 0x100d0242u:{uint32_t v=(r[0])+(4u);r[0]=v;flags(v);pc=0x100d0245u;break;} // 83c004 add eax, 4
case 0x100d0245u:{auto v=r[0];r[7]-=4;put(r[7],v,4);pc=0x100d0246u;break;} // 50 push eax
case 0x100d0246u:{fpwrite((r[7]+40u),st[0]);popfp();pc=0x100d024au;break;} // d95c2428 fstp dword ptr [esp + 0x28]
case 0x100d024au:{r[7]-=4;put(r[7],0x100d024fu,4);pc=0x100d7c90u;break;} // e8417a0000 call 0x100d7c90
case 0x100d024fu:{pushfp(fp((r[7]+40u)));pc=0x100d0253u;break;} // d9442428 fld dword ptr [esp + 0x28]
case 0x100d0253u:{uint32_t v=(r[7])+(12u);r[7]=v;flags(v);pc=0x100d0256u;break;} // 83c40c add esp, 0xc
case 0x100d0256u:{st[0]=product(st[0],st[0]);pc=0x100d0258u;break;} // dcc8 fmul st(0), st(0)
case 0x100d0258u:{compare(st[0],st[1]);popfp();popfp();pc=0x100d025au;break;} // ded9 fcompp
case 0x100d025au:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d025cu;break;} // dfe0 fnstsw ax
case 0x100d025cu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100d025fu;break;} // f6c441 test ah, 0x41
case 0x100d025fu:{pc=(!zf)?0x100d026au:0x100d0261u;break;} // 7509 jne 0x100d026a
case 0x100d0261u:{r[0]=1u;pc=0x100d0266u;break;} // b801000000 mov eax, 1
case 0x100d0266u:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x100d0269u;break;} // 83c418 add esp, 0x18
case 0x100d0269u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x100d026au:{uint32_t v=(r[0])^(r[0]);r[0]=v;flags(v);pc=0x100d026cu;break;} // 33c0 xor eax, eax
case 0x100d026cu:{uint32_t v=(r[7])+(24u);r[7]=v;flags(v);pc=0x100d026fu;break;} // 83c418 add esp, 0x18
case 0x100d026fu:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x100d7c90u:{r[0]=get((r[7]+4u),4);pc=0x100d7c94u;break;} // 8b442404 mov eax, dword ptr [esp + 4]
case 0x100d7c94u:{r[2]=get((r[7]+8u),4);pc=0x100d7c98u;break;} // 8b4c2408 mov ecx, dword ptr [esp + 8]
case 0x100d7c98u:{pushfp(fp((r[0])));pc=0x100d7c9au;break;} // d900 fld dword ptr [eax]
case 0x100d7c9au:{st[0]=difference(st[0],fp((r[2])));pc=0x100d7c9cu;break;} // d821 fsub dword ptr [ecx]
case 0x100d7c9cu:{pushfp(fp((r[0]+4u)));pc=0x100d7c9fu;break;} // d94004 fld dword ptr [eax + 4]
case 0x100d7c9fu:{st[0]=difference(st[0],fp((r[2]+4u)));pc=0x100d7ca2u;break;} // d86104 fsub dword ptr [ecx + 4]
case 0x100d7ca2u:{pushfp(fp((r[0]+8u)));pc=0x100d7ca5u;break;} // d94008 fld dword ptr [eax + 8]
case 0x100d7ca5u:{st[0]=difference(st[0],fp((r[2]+8u)));pc=0x100d7ca8u;break;} // d86108 fsub dword ptr [ecx + 8]
case 0x100d7ca8u:{pushfp(fp((r[2]+12u)));pc=0x100d7cabu;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x100d7cabu:{st[0]=product(st[0],st[3]);pc=0x100d7cadu;break;} // d8cb fmul st(3)
case 0x100d7cadu:{pushfp(fp((r[2]+16u)));pc=0x100d7cb0u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x100d7cb0u:{st[0]=product(st[0],st[3]);pc=0x100d7cb2u;break;} // d8cb fmul st(3)
case 0x100d7cb2u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7cb4u;break;} // dec1 faddp st(1)
case 0x100d7cb4u:{pushfp(fp((r[2]+20u)));pc=0x100d7cb7u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x100d7cb7u:{st[0]=product(st[0],st[2]);pc=0x100d7cb9u;break;} // d8ca fmul st(2)
case 0x100d7cb9u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7cbbu;break;} // dec1 faddp st(1)
case 0x100d7cbbu:{pushfp(0.);pc=0x100d7cbdu;break;} // d9ee fldz
case 0x100d7cbdu:{compare(st[0],st[1]);pc=0x100d7cbfu;break;} // d8d1 fcom st(1)
case 0x100d7cbfu:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d7cc1u;break;} // dfe0 fnstsw ax
case 0x100d7cc1u:{flags(((r[0]>>8)&255u)&(1u));cf=false;pc=0x100d7cc4u;break;} // f6c401 test ah, 1
case 0x100d7cc4u:{pc=(!zf)?0x100d7ccau:0x100d7cc6u;break;} // 7504 jne 0x100d7cca
case 0x100d7cc6u:{st[1]=st[0];popfp();pc=0x100d7cc8u;break;} // ddd9 fstp st(1)
case 0x100d7cc8u:{pc=(true)?0x100d7d3cu:0x100d7ccau;break;} // eb72 jmp 0x100d7d3c
case 0x100d7ccau:{st[0]=st[0];popfp();pc=0x100d7cccu;break;} // ddd8 fstp st(0)
case 0x100d7cccu:{pushfp(fp((r[2]+16u)));pc=0x100d7ccfu;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x100d7ccfu:{pushfp(fp((r[2]+12u)));pc=0x100d7cd2u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x100d7cd2u:{pushfp(fp((r[2]+20u)));pc=0x100d7cd5u;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x100d7cd5u:{pushfp(st[2]);pc=0x100d7cd7u;break;} // d9c2 fld st(2)
case 0x100d7cd7u:{st[3]=product(st[3],st[0]);popfp();pc=0x100d7cd9u;break;} // decb fmulp st(3)
case 0x100d7cd9u:{pushfp(st[1]);pc=0x100d7cdbu;break;} // d9c1 fld st(1)
case 0x100d7cdbu:{st[2]=product(st[2],st[0]);popfp();pc=0x100d7cddu;break;} // deca fmulp st(2)
case 0x100d7cddu:{std::swap(st[0],st[2]);pc=0x100d7cdfu;break;} // d9ca fxch st(2)
case 0x100d7cdfu:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7ce1u;break;} // dec1 faddp st(1)
case 0x100d7ce1u:{pushfp(st[1]);pc=0x100d7ce3u;break;} // d9c1 fld st(1)
case 0x100d7ce3u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d7ce5u;break;} // deca fmulp st(2)
case 0x100d7ce5u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7ce7u;break;} // dec1 faddp st(1)
case 0x100d7ce7u:{compare(st[0],st[1]);pc=0x100d7ce9u;break;} // d8d1 fcom st(1)
case 0x100d7ce9u:{r[0]=(r[0]&4294901760u)|((sw<<0)&65535u);pc=0x100d7cebu;break;} // dfe0 fnstsw ax
case 0x100d7cebu:{flags(((r[0]>>8)&255u)&(65u));cf=false;pc=0x100d7ceeu;break;} // f6c441 test ah, 0x41
case 0x100d7ceeu:{pc=(pf)?0x100d7d11u:0x100d7cf0u;break;} // 7a21 jp 0x100d7d11
case 0x100d7cf0u:{st[1]=st[0];popfp();pc=0x100d7cf2u;break;} // ddd9 fstp st(1)
case 0x100d7cf2u:{st[0]=st[0];popfp();pc=0x100d7cf4u;break;} // ddd8 fstp st(0)
case 0x100d7cf4u:{pushfp(1.);pc=0x100d7cf6u;break;} // d9e8 fld1
case 0x100d7cf6u:{std::swap(st[0],st[3]);pc=0x100d7cf8u;break;} // d9cb fxch st(3)
case 0x100d7cf8u:{st[0]=difference(st[0],fp((r[2]+12u)));pc=0x100d7cfbu;break;} // d8610c fsub dword ptr [ecx + 0xc]
case 0x100d7cfbu:{std::swap(st[0],st[2]);pc=0x100d7cfdu;break;} // d9ca fxch st(2)
case 0x100d7cfdu:{st[0]=difference(st[0],fp((r[2]+16u)));pc=0x100d7d00u;break;} // d86110 fsub dword ptr [ecx + 0x10]
case 0x100d7d00u:{std::swap(st[0],st[1]);pc=0x100d7d02u;break;} // d9c9 fxch st(1)
case 0x100d7d02u:{st[0]=difference(st[0],fp((r[2]+20u)));pc=0x100d7d05u;break;} // d86114 fsub dword ptr [ecx + 0x14]
case 0x100d7d05u:{std::swap(st[0],st[2]);pc=0x100d7d07u;break;} // d9ca fxch st(2)
case 0x100d7d07u:{std::swap(st[0],st[3]);pc=0x100d7d09u;break;} // d9cb fxch st(3)
case 0x100d7d09u:{std::swap(st[0],st[1]);pc=0x100d7d0bu;break;} // d9c9 fxch st(1)
case 0x100d7d0bu:{std::swap(st[0],st[2]);pc=0x100d7d0du;break;} // d9ca fxch st(2)
case 0x100d7d0du:{std::swap(st[0],st[1]);pc=0x100d7d0fu;break;} // d9c9 fxch st(1)
case 0x100d7d0fu:{pc=(true)?0x100d7d3cu:0x100d7d11u;break;} // eb2b jmp 0x100d7d3c
case 0x100d7d11u:{st[1]=quotient(st[1],st[0]);popfp();pc=0x100d7d13u;break;} // def9 fdivp st(1)
case 0x100d7d13u:{pushfp(fp((r[2]+12u)));pc=0x100d7d16u;break;} // d9410c fld dword ptr [ecx + 0xc]
case 0x100d7d16u:{pushfp(fp((r[2]+16u)));pc=0x100d7d19u;break;} // d94110 fld dword ptr [ecx + 0x10]
case 0x100d7d19u:{pushfp(fp((r[2]+20u)));pc=0x100d7d1cu;break;} // d94114 fld dword ptr [ecx + 0x14]
case 0x100d7d1cu:{std::swap(st[0],st[2]);pc=0x100d7d1eu;break;} // d9ca fxch st(2)
case 0x100d7d1eu:{st[0]=product(st[0],st[3]);pc=0x100d7d20u;break;} // d8cb fmul st(3)
case 0x100d7d20u:{std::swap(st[0],st[1]);pc=0x100d7d22u;break;} // d9c9 fxch st(1)
case 0x100d7d22u:{st[0]=product(st[0],st[3]);pc=0x100d7d24u;break;} // d8cb fmul st(3)
case 0x100d7d24u:{std::swap(st[0],st[2]);pc=0x100d7d26u;break;} // d9ca fxch st(2)
case 0x100d7d26u:{st[0]=product(st[0],st[3]);pc=0x100d7d28u;break;} // d8cb fmul st(3)
case 0x100d7d28u:{std::swap(st[0],st[6]);pc=0x100d7d2au;break;} // d9ce fxch st(6)
case 0x100d7d2au:{st[1]=difference(st[0],st[1]);popfp();pc=0x100d7d2cu;break;} // dee1 fsubrp st(1)
case 0x100d7d2cu:{std::swap(st[0],st[4]);pc=0x100d7d2eu;break;} // d9cc fxch st(4)
case 0x100d7d2eu:{st[1]=difference(st[0],st[1]);popfp();pc=0x100d7d30u;break;} // dee1 fsubrp st(1)
case 0x100d7d30u:{std::swap(st[0],st[2]);pc=0x100d7d32u;break;} // d9ca fxch st(2)
case 0x100d7d32u:{st[4]=difference(st[0],st[4]);popfp();pc=0x100d7d34u;break;} // dee4 fsubrp st(4)
case 0x100d7d34u:{std::swap(st[0],st[2]);pc=0x100d7d36u;break;} // d9ca fxch st(2)
case 0x100d7d36u:{std::swap(st[0],st[3]);pc=0x100d7d38u;break;} // d9cb fxch st(3)
case 0x100d7d38u:{std::swap(st[0],st[1]);pc=0x100d7d3au;break;} // d9c9 fxch st(1)
case 0x100d7d3au:{std::swap(st[0],st[2]);pc=0x100d7d3cu;break;} // d9ca fxch st(2)
case 0x100d7d3cu:{r[0]=get((r[7]+12u),4);pc=0x100d7d40u;break;} // 8b44240c mov eax, dword ptr [esp + 0xc]
case 0x100d7d40u:{flags((r[0])&(r[0]));cf=false;pc=0x100d7d42u;break;} // 85c0 test eax, eax
case 0x100d7d42u:{pc=(zf)?0x100d7d57u:0x100d7d44u;break;} // 7413 je 0x100d7d57
case 0x100d7d44u:{fpwrite((r[0]),st[0]);popfp();pc=0x100d7d46u;break;} // d918 fstp dword ptr [eax]
case 0x100d7d46u:{pushfp(st[1]);pc=0x100d7d48u;break;} // d9c1 fld st(1)
case 0x100d7d48u:{st[2]=product(st[2],st[0]);popfp();pc=0x100d7d4au;break;} // deca fmulp st(2)
case 0x100d7d4au:{pushfp(st[2]);pc=0x100d7d4cu;break;} // d9c2 fld st(2)
case 0x100d7d4cu:{st[3]=product(st[3],st[0]);popfp();pc=0x100d7d4eu;break;} // decb fmulp st(3)
case 0x100d7d4eu:{std::swap(st[0],st[1]);pc=0x100d7d50u;break;} // d9c9 fxch st(1)
case 0x100d7d50u:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d7d52u;break;} // dec2 faddp st(2)
case 0x100d7d52u:{st[0]=product(st[0],st[0]);pc=0x100d7d54u;break;} // dcc8 fmul st(0), st(0)
case 0x100d7d54u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7d56u;break;} // dec1 faddp st(1)
case 0x100d7d56u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
case 0x100d7d57u:{st[0]=st[0];popfp();pc=0x100d7d59u;break;} // ddd8 fstp st(0)
case 0x100d7d59u:{pushfp(st[1]);pc=0x100d7d5bu;break;} // d9c1 fld st(1)
case 0x100d7d5bu:{st[2]=product(st[2],st[0]);popfp();pc=0x100d7d5du;break;} // deca fmulp st(2)
case 0x100d7d5du:{pushfp(st[2]);pc=0x100d7d5fu;break;} // d9c2 fld st(2)
case 0x100d7d5fu:{st[3]=product(st[3],st[0]);popfp();pc=0x100d7d61u;break;} // decb fmulp st(3)
case 0x100d7d61u:{std::swap(st[0],st[1]);pc=0x100d7d63u;break;} // d9c9 fxch st(1)
case 0x100d7d63u:{st[2]=sum(st[2],st[0]);popfp();pc=0x100d7d65u;break;} // dec2 faddp st(2)
case 0x100d7d65u:{st[0]=product(st[0],st[0]);pc=0x100d7d67u;break;} // dcc8 fmul st(0), st(0)
case 0x100d7d67u:{st[1]=sum(st[1],st[0]);popfp();pc=0x100d7d69u;break;} // dec1 faddp st(1)
case 0x100d7d69u:{pc=get(r[7],4);r[7]+=4u;if(pc==0xeeeeeeeeu)return;break;} // c3 ret
