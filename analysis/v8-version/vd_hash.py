M=0xc6a4a7935bd1e995
MASK=(1<<64)-1
def u64(x): return x & MASK
def u32(x): return x & 0xffffffff

def wang32(v):
    # h = (v<<15) + (~v)  [32-bit]
    h = u32((u32(v<<15)) + u32(~v & 0xffffffff))
    a = u32(h ^ (h>>12))          # eax = h>>12 ^ h
    a = u32(a*5)                  # lea eax,[rax+rax*4]
    c = u32(a ^ (a>>4))           # ecx = a>>4 ^ a
    a = u32(c*0x809)              # imul eax,ecx,2057
    # rax >>=16 ; rax ^= rcx(=a low32)
    r = u64((a>>16) ^ a)         # note c had been overwritten: ecx=eax before shift
    return r

def h_version(a0,a1,a2,a3):
    # emulate the exact instruction flow
    # field patch (a3) -> rax; *M ; then rdx = mixfin(rax)*0x35a98f4d286a90b9
    def wang_full(v):
        r10 = u32((u32(v)<<15 & 0xffffffff) + u32((~v)&0xffffffff))
        eax = u32((r10>>12) ^ r10)
        eax = u32(eax*5)
        ecx = u32((eax>>4) ^ eax)
        eax = u32(ecx*0x809)
        ecx = eax
        rax = u64((eax>>16) ^ ecx)  # shr rax,16 (eax in rax) xor rcx(=ecx)
        return rax
    # patch
    rax = u64(wang_full(a3) * M)
    rdx = u64(((rax>>0x2f) ^ rax) * 0x35a98f4d286a90b9)
    # build
    rax2 = u64(wang_full(a2) * M)
    r8 = u64((rax2>>0x2f) ^ rax2)
    r8 = u64(r8 * M)
    r8 = u64(r8 ^ rdx)
    r8 = u64(r8 * M)
    # minor
    rax3 = u64(wang_full(a1) * M)
    r9 = u64((rax3>>0x2f) ^ rax3)
    r9 = u64(r9 * M)
    r9 = u64(r9 ^ r8)
    r9 = u64(r9 * M)
    # major
    eax = wang_full(a0)   # returns already the pre-M value? For major path code:
    # major path: after wang, edx=eax; shr rdx,16; xor rdx,rcx; imul rdx,M; rax=rdx; mixfin; *M; xor r9; *M
    # wang_full already applied the >>16 xor. But major branch reuses eax then does its own >>16.
    # Recompute major per its explicit sequence:
    v=a0
    r10 = u32((u32(v)<<15 & 0xffffffff) + u32((~v)&0xffffffff))
    eax = u32((r10>>12) ^ r10)
    eax = u32(eax*5)
    ecx = u32((eax>>4) ^ eax)
    eax = u32(ecx*0x809)
    ecx = eax
    edx = eax
    rdx = u64((edx>>16) ^ ecx)
    rdx = u64(rdx * M)
    rax = u64(((rdx>>0x2f) ^ rdx) * M)
    rax = u64(rax ^ r9)
    rax = u64(rax * M)
    return rax

target=0x472058a6
found=[]
for a0 in range(1,25):
  for a1 in range(0,40):
    for a2 in range(0,700):
      for a3 in range(0,500):
        if u32(h_version(a0,a1,a2,a3))==target:
          found.append((a0,a1,a2,a3))
print("matches:",found[:20], "count",len(found))
