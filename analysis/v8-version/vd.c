#include <stdio.h>
#include <stdint.h>
static const uint64_t M=0xc6a4a7935bd1e995ULL;
static inline uint64_t wang_full(uint32_t v){
  uint32_t r10=(uint32_t)((v<<15)+(~v));
  uint32_t eax=(uint32_t)((r10>>12)^r10);
  eax=(uint32_t)(eax*5u);
  uint32_t ecx=(uint32_t)((eax>>4)^eax);
  eax=(uint32_t)(ecx*0x809u);
  ecx=eax;
  return (uint64_t)(((uint64_t)eax>>16)^ecx);
}
static uint32_t hv(uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3){
  uint64_t rax=wang_full(a3)*M;
  uint64_t rdx=((rax>>0x2f)^rax)*0x35a98f4d286a90b9ULL;
  uint64_t rax2=wang_full(a2)*M;
  uint64_t r8=((rax2>>0x2f)^rax2); r8*=M; r8^=rdx; r8*=M;
  uint64_t rax3=wang_full(a1)*M;
  uint64_t r9=((rax3>>0x2f)^rax3); r9*=M; r9^=r8; r9*=M;
  uint64_t rdx2=wang_full(a0)*M;
  uint64_t rr=((rdx2>>0x2f)^rdx2)*M; rr^=r9; rr*=M;
  return (uint32_t)rr;
}
int main(){
  uint32_t t=0x472058a6; int n=0;
  for(uint32_t a0=1;a0<40;a0++)for(uint32_t a1=0;a1<64;a1++)
   for(uint32_t a2=0;a2<2000;a2++)for(uint32_t a3=0;a3<2000;a3++)
     if(hv(a0,a1,a2,a3)==t){printf("MATCH %u.%u.%u.%u\n",a0,a1,a2,a3);if(++n>30)return 0;}
  printf("done, %d matches\n",n);
}
