// Standalone regression; custom allocator avoids large allocations.
#include <cassert>
#include <climits>
#include <cstdlib>
#define LODEPNG_NO_COMPILE_ALLOCATORS
#include "../lodepng.cpp"

static size_t requested;
static bool reject_large;
static bool reject_zero;
void* lodepng_malloc(size_t n) { requested = n; return (reject_zero && !n) || (reject_large && n > 1024) ? NULL : malloc(n); }
void* lodepng_realloc(void* p, size_t n) { return reject_large && n > 1024 ? NULL : realloc(p, n); }
void lodepng_free(void* p) { free(p); }

static unsigned oversized(unsigned char** out, size_t* size, const unsigned char*, size_t,
                          const LodePNGCompressSettings*) {
  *out = (unsigned char*)malloc(1); *size = (size_t)-1; return *out ? 0 : 83;
}

int main() {
  if(sizeof(size_t) > sizeof(unsigned)) {
    size_t large_count = (size_t)UINT_MAX;
    ++large_count;
    assert(ilog2i(large_count) == (double)large_count * 32.0);
  }
  assert(deflateRemaining((size_t)-1, (size_t)-3, 258) == 2);
  assert(deflateRemaining((size_t)-1, 0, 258) == 258);
  assert(deflateRemaining((size_t)-1, (size_t)-1, 258) == 0);
  const unsigned char byte = 7;
  assert(getHash(&byte, (size_t)-1, (size_t)-1) == 0);
  assert(getHash(&byte, 1, 0) == 7);
  LodePNGCompressSettings cs; lodepng_compress_settings_init(&cs);
  for(unsigned type = 0; type != 3; ++type) {
    cs.btype = type; unsigned char* compressed = NULL; size_t n = 0;
    assert(!lodepng_zlib_compress(&compressed, &n, NULL, 0, &cs));
    unsigned char* decoded = NULL; size_t decoded_size = 0;
    LodePNGDecompressSettings ds; lodepng_decompress_settings_init(&ds);
    assert(!lodepng_zlib_decompress(&decoded, &decoded_size, compressed, n, &ds));
    assert(decoded_size == 0); free(compressed); free(decoded);
  }
  cs.custom_deflate = oversized;
  unsigned char* overflow_out = NULL; size_t overflow_size = 0;
  assert(lodepng_zlib_compress(&overflow_out, &overflow_size, NULL, 0, &cs) == 77);
  assert(!overflow_out && !overflow_size);
  cs.custom_deflate = NULL;
  uivector values; uivector_init(&values);
  assert(!uivector_resize(&values, (size_t)-1));
  assert(!values.data && !values.size);
  cs.custom_zlib = oversized;
  ucvector chunks = ucvector_init(NULL, 0);
  assert(addChunk_IDAT(&chunks, NULL, 0, &cs) == 77);
  assert(addChunk_zTXt(&chunks, "key", "text", &cs) == 77);
  assert(addChunk_iTXt(&chunks, 1, "key", "", "", "text", &cs) == 77);
  assert(!chunks.data && !chunks.size);
  LodePNGColorMode grey = lodepng_color_mode_make(LCT_GREY, 1);
  assert(lodepng_get_raw_size(65536, 65536, &grey) == size_t(536870912));
  assert(lodepng_get_raw_size(9, 9, &grey) == 11);
  LodePNGColorMode rgba16 = lodepng_color_mode_make(LCT_RGBA, 16);
  assert(lodepng_convert(NULL, NULL, &rgba16, &rgba16, UINT_MAX, UINT_MAX) == 77);
  assert(!lodepng_pixel_overflow(8192, 8192, &grey, &grey));
  assert(lodepng_pixel_overflow(UINT_MAX, UINT_MAX, &rgba16, &rgba16));
  const size_t large_size = lodepng_get_raw_size(8192, 8192, &grey);
  unsigned char* large = (unsigned char*)calloc(large_size, 1);
  unsigned char* copy = (unsigned char*)malloc(large_size);
  assert(large && copy && large_size == 8u * 1024u * 1024u);
  assert(!lodepng_convert(copy, large, &grey, &grey, 8192, 8192));
  LodePNGColorStats large_stats; lodepng_color_stats_init(&large_stats);
  assert(!lodepng_compute_color_stats(&large_stats, large, 8192, 8192, &grey));
  assert(large_stats.numpixels == size_t(8192) * 8192);
  free(copy); free(large);
  LodePNGColorStats stats; lodepng_color_stats_init(&stats);
  stats.numpixels = (size_t)-1;
  assert(lodepng_compute_color_stats(&stats, NULL, 1, 1, &grey) == 77);
  LodePNGState state; lodepng_state_init(&state);
  unsigned char* out = NULL; size_t size = 0;
  assert(lodepng_encode(&out, &size, NULL, UINT_MAX, UINT_MAX, &state) == 77);
  assert(!out && !size);
  if(sizeof(size_t) > 4) {
    state.encoder.auto_convert = 0;
    reject_large = true; requested = 0;
    assert(lodepng_encode(&out, &size, NULL, 0x20000000u, 1, &state) == 83);
    assert(requested == size_t(0x80000000u) + 1 && !out && !size);
    reject_large = false;
  }
  state.encoder.auto_convert = 0;
  state.info_raw = grey; state.info_png.color = grey;
  state.info_png.interlace_method = 1;
  reject_large = true; requested = 0;
  assert(lodepng_encode(&out, &size, NULL, 8192, 8192, &state) == 83);
  assert(!out && !size && requested > 1024);
  reject_large = false;
  const unsigned char packed[11] = {0x81, 0x42, 0x24, 0x18, 0xff, 0, 1, 2, 3, 4, 0x80};
  assert(!lodepng_encode(&out, &size, packed, 9, 9, &state));
  unsigned char* packed_decoded = NULL; unsigned pw, ph;
  state.decoder.color_convert = 0;
  assert(!lodepng_decode(&packed_decoded, &pw, &ph, &state, out, size));
  assert(pw == 9 && ph == 9 && !memcmp(packed, packed_decoded, sizeof(packed)));
  free(out); free(packed_decoded); out = NULL; size = 0;
  lodepng_state_cleanup(&state); lodepng_state_init(&state);
  const unsigned char pixels[16] = {0};
  for(unsigned interlace = 0; interlace != 2; ++interlace) {
    state.info_png.interlace_method = interlace;
    assert(!lodepng_encode(&out, &size, pixels, 2, 2, &state));
    unsigned char* decoded = NULL; unsigned w, h;
    assert(!lodepng_decode32(&decoded, &w, &h, out, size));
    assert(w == 2 && h == 2 && !memcmp(decoded, pixels, sizeof(pixels)));
    free(out); free(decoded); out = NULL; size = 0;
  }
  lodepng_state_cleanup(&state);

  // Empty Adam7 passes must not depend on malloc(0) returning non-null.
  reject_zero = true;
  for(unsigned depth = 1; depth <= 4; depth *= 2) {
    for(unsigned width = 1; width <= 3; ++width) {
      lodepng::State cpp;
      cpp.encoder.auto_convert = 0;
      cpp.info_raw.colortype = cpp.info_png.color.colortype = LCT_GREY;
      cpp.info_raw.bitdepth = cpp.info_png.color.bitdepth = depth;
      cpp.info_png.interlace_method = 1;
      std::vector<unsigned char> raw((width * depth + 7) / 8, 0);
      std::vector<unsigned char> encoded, decoded;
      assert(!lodepng::encode(encoded, raw, width, 1, cpp));
      unsigned dw, dh;
      cpp.decoder.color_convert = 0;
      assert(!lodepng::decode(decoded, dw, dh, cpp, encoded));
      assert(dw == width && dh == 1 && decoded == raw);
      unsigned char* c_out = NULL; size_t c_size = 0;
      assert(!lodepng_encode(&c_out, &c_size, raw.data(), width, 1, &cpp));
      assert(c_size == encoded.size() && !memcmp(c_out, encoded.data(), c_size));
      free(c_out);
    }
  }
  reject_zero = false;
  lodepng::State cpp;
  cpp.info_raw.colortype = LCT_GREY;
  cpp.info_raw.bitdepth = 8;
  std::vector<unsigned char> empty, encoded;
  // On 32-bit, the default RGBA output preflight overflows for these dimensions,
  // while the actual auto-convert GREY8 input is representable.
  assert(!lodepng_pixel_overflow(65536, 32768, &cpp.info_raw, &cpp.info_raw));
  assert(lodepng::encode(encoded, empty, 65536, 32768, cpp) == 84);
  cpp.encoder.auto_convert = 0;
  if(sizeof(size_t) == 4) assert(lodepng::encode(encoded, empty, 65536, 32768, cpp) == 77);
}
