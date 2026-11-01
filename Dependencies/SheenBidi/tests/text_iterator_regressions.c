// Standalone experimental text API regression. Run with leak sanitizer enabled.
#include <SheenBidi/SBText.h>
#include <assert.h>

int main(void)
{
    const SBUInt32 text_units[] = { 'a', 'b', 0x0627, 0x0628, 'c' };
    SBTextConfigRef config = SBTextConfigCreate();
    assert(config);
    SBTextRef text = SBTextCreate(text_units, 5, SBStringEncodingUTF32, config);
    assert(text);
    SBScript scripts[5];
    SBTextGetScripts(text, 0, 5, scripts);
    assert(scripts[0] != scripts[2]);
    SBScriptRunIteratorRef script = SBTextCreateScriptRunIterator(text);
    assert(script);
    SBScriptRunIteratorReset(script, 2, 2);
    assert(SBScriptRunIteratorMoveNext(script));
    const SBScriptRun *run = SBScriptRunIteratorGetCurrent(script);
    assert(run->index == 2 && run->length == 2 && run->script == scripts[2]);
    assert(!SBScriptRunIteratorMoveNext(script));
    SBScriptRunIteratorRelease(script);

    SBVisualRunIteratorRef visual = SBTextCreateVisualRunIterator(text, 0, 5);
    assert(visual);
    for (int i = 0; i < 100; ++i) {
        assert(SBVisualRunIteratorMoveNext(visual));
        SBVisualRunIteratorReset(visual, 0, 5);
    }
    SBVisualRunIteratorRelease(visual);
    SBTextRelease(text);
    SBTextConfigRelease(config);
    return 0;
}
