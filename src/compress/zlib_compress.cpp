#include "zlib_compress.h"

namespace logger {
namespace compress {

size_t ZlibCompress::Compress(const void* input_data, size_t input_size, void* output_data, size_t output_size) {}

size_t ZlibCompress::CompressBound(size_t input_size) {}

std::string ZlibCompress::DeCompress() {}

void ZlibCompress::ResetStream() {}
}  // namespace compress
}  // namespace logger