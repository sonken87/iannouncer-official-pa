#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
typedef unsigned __int128 u128;
static const uint64_t S1=0x2d358dccaa6c78a5ull,S2=0x8bb84b93962eacc9ull;
uint64_t rmix(uint64_t a,uint64_t b){u128 c=(u128)a*b;return (uint64_t)c^(uint64_t)(c>>64);}
uint64_t hrapid(uint32_t k){uint64_t x=k;return (uint32_t)rmix(x^S1,x^S2);}
uint64_t hwang(uint32_t v){v=~v+(v<<15);v^=v>>12;v+=v<<2;v^=v>>4;v*=2057;v^=v>>16;return v;}
uint64_t comb(uint64_t seed,uint64_t h){const uint64_t m=0xC6A4A7935BD1E995ull;h*=m;h^=h>>47;h*=m;seed^=h;seed*=m;return seed;}
int main(){
  const uint32_t target=0x472058a6;
  std::vector<std::string> emb={"","-electron.0"};
  for(int i=0;i<40;i++) emb.push_back("-node."+std::to_string(i));
  for(int alg=0;alg<2;alg++){
   auto H=[&](uint32_t x){return alg?hwang(x):hrapid(x);};
   for(int ma=10;ma<=16;ma++)for(int mi=0;mi<=9;mi++)for(int b=0;b<=500;b++)for(int p=0;p<=300;p++){
     // old: reverse-order variadic, no embedder
     uint64_t o=comb(comb(comb(comb(0,H(p)),H(b)),H(mi)),H(ma));
     if((uint32_t)o==target) printf("MATCH old alg=%d %d.%d.%d.%d\n",alg,ma,mi,b,p);
     uint64_t f=comb(comb(comb(comb(0,H(ma)),H(mi)),H(b)),H(p));
     if((uint32_t)f==target) printf("MATCH fwd-noemb alg=%d %d.%d.%d.%d\n",alg,ma,mi,b,p);
     for(auto&e:emb){uint64_t g=f;for(unsigned char c:e)g=comb(g,c);
       if((uint32_t)g==target) printf("MATCH fwd alg=%d %d.%d.%d.%d%s\n",alg,ma,mi,b,p,e.c_str());}
   }}
  puts("done");
}
