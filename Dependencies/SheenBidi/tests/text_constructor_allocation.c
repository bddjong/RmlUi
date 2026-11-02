/* Deterministic allocation failures during mutable text construction. */
#include <SheenBidi/SBText.h>
#include <SheenBidi/SBAllocator.h>
#include <assert.h>
#include <stdlib.h>

typedef struct { size_t calls, fail, live; } State;
static void *allocate(SBUInteger size, void *info) {
    State *s = info; void *p;
    if (++s->calls == s->fail) return NULL;
    p = malloc(size); if (p) ++s->live; return p;
}
static void *reallocate(void *ptr, SBUInteger size, void *info) {
    State *s = info;
    if (++s->calls == s->fail) return NULL;
    return realloc(ptr, size);
}
static void deallocate(void *ptr, void *info) {
    State *s = info; if (ptr) { --s->live; free(ptr); }
}
int main(void) {
    State state = {0};
    SBAllocatorProtocol protocol = {allocate, reallocate, deallocate, NULL, NULL, NULL};
    SBAllocatorRef allocator = SBAllocatorCreate(&protocol, &state);
    SBTextConfigRef config;
    SBAttributeRegistryRef registry;
    size_t baseline, allocations, i;
    SBMutableTextRef text;
    assert(allocator); SBAllocatorSetDefault(allocator);
    config = SBTextConfigCreate(); assert(config);
    registry = SBAttributeRegistryCreate(NULL, 0, sizeof(int), NULL); assert(registry);
    SBTextConfigSetAttributeRegistry(config, registry);
    baseline = state.live; state.calls = 0;
    text = SBTextCreateMutable(SBStringEncodingUTF8, config); assert(text);
    allocations = state.calls; SBTextRelease(text); assert(state.live == baseline);
    for (i = 1; i <= allocations; ++i) {
        state.calls = 0; state.fail = i;
        text = SBTextCreateMutable(SBStringEncodingUTF8, config);
        assert(!text); assert(state.live == baseline);
        state.fail = 0;
        text = SBTextCreateMutable(SBStringEncodingUTF8, config); assert(text);
        SBTextRelease(text); assert(state.live == baseline);
    }
    SBTextConfigRelease(config); SBAttributeRegistryRelease(registry);
    assert(state.live == 0); SBAllocatorSetDefault(NULL); SBAllocatorRelease(allocator);
    return 0;
}
