#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef unsigned __int128 u128;
static const uint64_t S1=0x2d358dccaa6c78a5ULL,S2=0x8bb84b93962eacc9ULL,M=0xC6A4A7935BD1E995ULL;
static inline uint32_t h32(uint32_t k){u128 c=(u128)(k^S1)*(uint64_t)(k^S2);return (uint32_t)((uint64_t)c^(uint64_t)(c>>64));}
static inline uint64_t comb(uint64_t seed,uint64_t h){h*=M;h^=h>>47;h*=M;seed^=h;seed*=M;return seed;}
static uint32_t hv(int ma,int mi,int b,int p,const char*emb){
  uint64_t s=0;
  s=comb(s,h32((uint32_t)ma));s=comb(s,h32((uint32_t)mi));
  s=comb(s,h32((uint32_t)b));s=comb(s,h32((uint32_t)p));
  for(const char*e=emb;*e;e++) s=comb(s,(uint64_t)(unsigned char)*e); // AddRange bytes, trivial hash
  return (uint32_t)s;
}
int main(){
  uint32_t t=0x472058a6;int n=0;
  const char* embs[]={"", "-electron.0","-node.24","-electron","-node.0"};
  for(int ei=0;ei<5;ei++)
   for(int ma=10;ma<=16;ma++)for(int mi=0;mi<40;mi++)
    for(int b=0;b<3000;b++)for(int p=0;p<3000;p++)
      if(hv(ma,mi,b,p,embs[ei])==t){printf("MATCH %d.%d.%d.%d emb='%s'\n",ma,mi,b,p,embs[ei]);if(++n>20)return 0;}
  printf("done %d\n",n);
}
