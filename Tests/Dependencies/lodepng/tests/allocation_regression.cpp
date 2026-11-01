// Standalone: compile this file alone, preferably with address sanitizer.
#include <cassert>
#include <cstdlib>
#define LODEPNG_NO_COMPILE_ALLOCATORS
#include "../lodepng.cpp"

static int fail_after = -1;
static bool fail() { if(fail_after < 0) return false; if(fail_after-- == 0) return true; return false; }
void* lodepng_malloc(size_t n) { return fail() ? NULL : malloc(n); }
void* lodepng_realloc(void* p, size_t n) { return fail() ? NULL : realloc(p, n); }
void lodepng_free(void* p) { free(p); }

int main() {
  ucvector bytes = ucvector_init(NULL, 0);
  LodePNGBitWriter writer; LodePNGBitWriter_init(&writer, &bytes);
  fail_after = 0; writeBits(&writer, 1, 1); writeBits(&writer, 1, 1);
  assert(writer.error == 83 && bytes.size == 0); free(bytes.data);
  BPMNode nodes[2] = {}; fail_after = 0; assert(bpmnode_sort(nodes, 2) == 83);
  uivector values; uivector_init(&values); fail_after = 0;
  assert(!addLengthDistance(&values, 3, 1)); uivector_cleanup(&values);
  fail_after = -1;
  LodePNGInfo src; lodepng_info_init(&src);
  assert(!lodepng_add_text(&src, "key", "value"));
  assert(!lodepng_add_itext(&src, "key", "en", "translated", "value"));
  for(int i = 0; i < 20; ++i) {
    LodePNGInfo dst; lodepng_info_init(&dst); fail_after = i;
    lodepng_info_copy(&dst, &src); fail_after = -1;
    lodepng_info_cleanup(&dst);
  }
  lodepng_clear_text(&src); lodepng_clear_text(&src);
  lodepng_clear_itext(&src); lodepng_clear_itext(&src);
  assert(!lodepng_add_text(&src, "new", "value"));
  assert(!lodepng_add_itext(&src, "new", "en", "new", "value"));
  lodepng_info_cleanup(&src);
  unsigned pw[7], ph[7]; size_t f[8], p[8], a[8];
  Adam7_getpassvalues(pw, ph, f, p, a, 100000, 100000, 32);
  assert(a[7] == size_t(100000) * 100000 * 4);
}
