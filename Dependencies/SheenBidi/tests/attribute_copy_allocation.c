/* Exercise every allocation in transactional attribute-manager copying. */
#include "../Source/SheenBidi.c"
#include <SheenBidi/SBAllocator.h>
#include <Text/AttributeManager.h>
#include <assert.h>
#include <stdlib.h>

typedef struct { size_t calls, fail, live; } State;
static int retained;
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
    State *s = info; if (ptr) { assert(s->live); --s->live; free(ptr); }
}
static const void *retain(const void *value) { ++retained; return value; }
static void release(const void *value) { (void)value; --retained; assert(retained >= 0); }
static int value_at(AttributeManager *manager, SBUInteger entry, SBAttributeID id) {
    AttributeEntry *e = ListGetRef(&manager->_entries, entry);
    const int *v = AttributeDictionaryFindValue(e->attributes, id);
    assert(v); return *v;
}
int main(void) {
    State state = {0};
    SBAllocatorProtocol protocol = {allocate, reallocate, deallocate, NULL, NULL, NULL};
    SBAttributeValueCallbacks callbacks = {NULL, retain, release};
    SBAttributeInfo info = {"test", SBAttributeGroupNone, SBAttributeScopeCharacter};
    SBAllocatorRef allocator = SBAllocatorCreate(&protocol, &state);
    SBAttributeRegistryRef registry;
    SBAttributeID id;
    AttributeManager source, destination;
    AttributeEntry entry;
    size_t baseline, allocations, i;
    int one = 1, two = 2, old = 9, refs;
    assert(allocator); SBAllocatorSetDefault(allocator);
    registry = SBAttributeRegistryCreate(&info, 1, sizeof(int), &callbacks); assert(registry);
    id = SBAttributeRegistryGetAttributeID(registry, "test");
    AttributeManagerInitialize(&source, NULL, registry);
    AttributeManagerInitialize(&destination, NULL, registry);
    assert(source._entries.count == 1 && destination._entries.count == 1);
    AttributeDictionaryPut(ListGetVal(&source._entries, 0).attributes, id, &one, registry, NULL);
    entry.index = 4; entry.attributes = AttributeDictionaryCreate(sizeof(int)); assert(entry.attributes);
    AttributeDictionaryPut(entry.attributes, id, &two, registry, NULL);
    assert(ListAdd(&source._entries, &entry)); source._stringLength = 8;
    AttributeDictionaryPut(ListGetVal(&destination._entries, 0).attributes, id, &old, registry, NULL);
    destination._stringLength = 3;
    baseline = state.live; refs = retained;
    state.calls = 0; assert(AttributeManagerCopyAttributes(&destination, &source));
    allocations = state.calls;
    assert(destination._entries.count == 2 && destination._stringLength == 8);
    assert(value_at(&destination, 0, id) == 1 && value_at(&destination, 1, id) == 2);
    AttributeManagerFinalize(&destination);
    AttributeManagerInitialize(&destination, NULL, registry);
    AttributeDictionaryPut(ListGetVal(&destination._entries, 0).attributes, id, &old, registry, NULL);
    destination._stringLength = 3;
    assert(state.live == baseline && retained == refs);
    for (i = 1; i <= allocations; ++i) {
        state.calls = 0; state.fail = i;
        assert(!AttributeManagerCopyAttributes(&destination, &source));
        assert(destination._entries.count == 1 && destination._stringLength == 3);
        assert(value_at(&destination, 0, id) == 9);
        assert(state.live == baseline && retained == refs);
        state.fail = 0;
        assert(AttributeManagerCopyAttributes(&destination, &source));
        assert(value_at(&destination, 0, id) == 1 && value_at(&destination, 1, id) == 2);
        AttributeManagerFinalize(&destination);
        AttributeManagerInitialize(&destination, NULL, registry);
        AttributeDictionaryPut(ListGetVal(&destination._entries, 0).attributes, id, &old, registry, NULL);
        destination._stringLength = 3;
        assert(state.live == baseline && retained == refs);
    }
    AttributeManagerFinalize(&destination); AttributeManagerFinalize(&source);
    assert(retained == 0); SBAttributeRegistryRelease(registry);
    assert(state.live == 0); SBAllocatorSetDefault(NULL); SBAllocatorRelease(allocator);
    return 0;
}
