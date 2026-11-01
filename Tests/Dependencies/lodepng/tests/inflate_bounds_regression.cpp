// Standalone: compile this file alone, preferably with address sanitizer.
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#define LODEPNG_NO_COMPILE_ALLOCATORS
#include "../lodepng.cpp"

static bool fail_alloc = false;
void* lodepng_malloc(size_t n) { return fail_alloc ? NULL : malloc(n); }
void* lodepng_realloc(void* p, size_t n) { return fail_alloc ? NULL : realloc(p, n); }
void lodepng_free(void* p) { free(p); }

static unsigned oversized(unsigned char** out, size_t* size, const unsigned char*, size_t,
                          const LodePNGDecompressSettings*) {
  *out = (unsigned char*)malloc(9); *size = 9; return *out ? 0 : 83;
}
int main() {
  unsigned char input[1024]; memset(input, 'A', sizeof(input));
  for(unsigned type = 0; type != 3; ++type) {
    LodePNGCompressSettings cs; lodepng_compress_settings_init(&cs); cs.btype = type;
    unsigned char* compressed = NULL; size_t compressed_size = 0;
    assert(!lodepng_zlib_compress(&compressed, &compressed_size, input, sizeof(input), &cs));
    LodePNGDecompressSettings ds; lodepng_decompress_settings_init(&ds);
    unsigned char* result = NULL; size_t size = 0;
    assert(zlib_decompress(&result, &size, 8, compressed, compressed_size, &ds) == 91);
    assert(size <= 8); free(result);
    result = NULL; size = 0;
    assert(!zlib_decompress(&result, &size, sizeof(input), compressed, compressed_size, &ds));
    assert(size == sizeof(input) && !memcmp(result, input, size)); free(result);
    result = NULL; size = 0; fail_alloc = true;
    assert(zlib_decompress(&result, &size, 8, compressed, compressed_size, &ds) == 83);
    assert(!result && !size); fail_alloc = false; free(compressed);
  }
  LodePNGDecompressSettings ds; lodepng_decompress_settings_init(&ds);
  ds.custom_zlib = oversized;
  unsigned char* result = NULL; size_t size = 0;
  assert(zlib_decompress(&result, &size, 8, NULL, 0, &ds) == 91); free(result);
  LodePNGState state; lodepng_state_init(&state);
  result = NULL; size = 0;
  assert(lodepng_encode(&result, &size, NULL, UINT_MAX, UINT_MAX, &state) == 77);
  assert(!result && !size); lodepng_state_cleanup(&state);
  unsigned pw[7], ph[7]; size_t f[8], p[8], a[8];
  Adam7_getpassvalues(pw, ph, f, p, a, 17, 19, 32);
  assert(a[7] == size_t(17) * 19 * 4);
}
