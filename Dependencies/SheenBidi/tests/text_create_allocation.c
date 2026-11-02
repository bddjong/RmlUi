/* Fault injection for immutable construction and mutable copies. */
#include <SheenBidi/SBText.h>
#include <SheenBidi/SBAllocator.h>
#include <SheenBidi/SBTextIterators.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct { size_t calls, fail, live; } State;
static void *alloc(SBUInteger n, void *v) { State *s=v; void *p; if (++s->calls==s->fail) return NULL; p=malloc(n); if (p) ++s->live; return p; }
static void *resize(void *p, SBUInteger n, void *v) { State *s=v; if (++s->calls==s->fail) return NULL; return realloc(p,n); }
static void release(void *p, void *v) { State *s=v; if (p) { --s->live; free(p); } }
static const SBUInt32 input[] = {'a',0x627,'\n','b',0x628,'\n','c'};
static void check(SBTextRef t) { SBUInt32 out[7]; SBScript scripts[7]; assert(t && SBTextGetLength(t)==7); SBTextGetCodeUnits(t,0,7,out); assert(!memcmp(out,input,sizeof input)); SBTextGetScripts(t,0,7,scripts); }
static void check_attributes(SBTextRef t, SBAttributeID id) {
 SBAttributeRunIteratorRef it=SBTextCreateAttributeRunIterator(t); const SBAttributeRun *run; assert(it); SBAttributeRunIteratorSetupAttributeID(it,id); SBAttributeRunIteratorReset(it,0,7); assert(SBAttributeRunIteratorMoveNext(it)); run=SBAttributeRunIteratorGetCurrent(it); assert(run->index==1 && run->length==3 && SBAttributeListGetCount(run->attributes)==1); assert(!SBAttributeRunIteratorMoveNext(it)); SBAttributeRunIteratorRelease(it);
}
int main(void) {
 State s={0}; SBAllocatorProtocol p={alloc,resize,release,NULL,NULL,NULL}; SBAllocatorRef a=SBAllocatorCreate(&p,&s); SBTextConfigRef c; SBAttributeRegistryRef r; SBTextRef source,t; SBMutableTextRef mutableSource; SBAttributeInfo info={"style",0,SBAttributeScopeCharacter}; int value=42; size_t base,n,i; int mode;
 assert(a); SBAllocatorSetDefault(a); c=SBTextConfigCreate(); r=SBAttributeRegistryCreate(&info,1,sizeof(int),NULL); assert(c&&r); SBTextConfigSetAttributeRegistry(c,r);
 source=SBTextCreate(input,7,SBStringEncodingUTF32,c); check(source); mutableSource=SBTextCreateMutableCopy(source); assert(mutableSource); SBTextSetAttribute(mutableSource,1,3,SBAttributeRegistryGetAttributeID(r,"style"),&value); SBTextRelease(source); source=mutableSource; base=s.live;
 for (mode=0;mode<2;mode++) {
  s.calls=0; t=mode ? SBTextCreateMutableCopy(source) : SBTextCreate(input,7,SBStringEncodingUTF32,c); check(t); n=s.calls; if (mode) check_attributes(t,SBAttributeRegistryGetAttributeID(r,"style")); SBTextRelease(t); assert(s.live==base);
  for(i=1;i<=n;i++) { s.calls=0; s.fail=i; t=mode ? SBTextCreateMutableCopy(source) : SBTextCreate(input,7,SBStringEncodingUTF32,c); assert(!t); assert(s.live==base); check(source); s.fail=0; t=mode ? SBTextCreateMutableCopy(source) : SBTextCreate(input,7,SBStringEncodingUTF32,c); check(t); if (mode) check_attributes(t,SBAttributeRegistryGetAttributeID(r,"style")); SBTextRelease(t); assert(s.live==base); }
 }
 SBTextRelease(source); SBTextConfigRelease(c); SBAttributeRegistryRelease(r); assert(!s.live); SBAllocatorSetDefault(NULL); SBAllocatorRelease(a); return 0;
}
