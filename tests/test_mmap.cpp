#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
#include <filesystem>

#include "internal_log.h"
#include "mmap/mmap_handle.h"

using namespace logger;

static void print_cwd() {
    std::cout << "cwd: " << std::filesystem::current_path() << std::endl;
}

// 检查文件是否存在且大小 >= 头部 + Size
static void assert_file_consistent(const std::filesystem::path& p, size_t payload_size_at_least) {
    assert(std::filesystem::exists(p));
    auto sz = std::filesystem::file_size(p);
    // 这里只能检查文件是否存在且不为0；更严格校验需要暴露头部结构
    assert(sz >= payload_size_at_least);
}

void test_constructor_and_empty() {
    print_cwd();
    std::filesystem::path test_file = "./test_mmap_handle.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    assert(mmap.Empty());
    assert(mmap.Size() == 0);
    assert(mmap.Data() != nullptr);
    assert(mmap.Capacity() >= mmap.Size());
    assert_file_consistent(test_file, 0);

    LOG_INFO("test_constructor_and_empty passed");
}

void test_push_and_size() {
    std::filesystem::path test_file = "./test_mmap_handle_push.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};

    size_t original_size = mmap.Size();
    bool ok = mmap.Push(data.data(), data.size());
    assert(ok);
    assert(mmap.Size() == original_size + data.size());

    uint8_t* ptr = mmap.Data();
    assert(ptr != nullptr);
    // 验证末尾追加的数据与写入一致
    assert(std::memcmp(ptr + original_size, data.data(), data.size()) == 0);

    assert_file_consistent(test_file, mmap.Size());
    LOG_INFO("test_push_and_size passed");
}

void test_push_and_read_string() {
    std::filesystem::path test_file = "./test_mmap_handle_string.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    std::string msg1 = "hello ";
    std::string msg2 = "mmap!";
    std::string combined = msg1 + msg2;

    // 原始内容
    size_t original_size = mmap.Size();
    std::string original(reinterpret_cast<char*>(mmap.Data()), mmap.Size());

    bool ok1 = mmap.Push(msg1.data(), msg1.size());
    bool ok2 = mmap.Push(msg2.data(), msg2.size());
    assert(ok1 && ok2);

    assert(mmap.Size() == original_size + combined.size());
    std::string read(reinterpret_cast<char*>(mmap.Data()), mmap.Size());
    assert(read == original + combined);

    assert_file_consistent(test_file, mmap.Size());
    LOG_INFO("test_push_and_read_string passed");
}

void test_push_empty_data() {
    std::filesystem::path test_file = "./test_mmap_handle_empty_push.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    size_t original_size = mmap.Size();
    std::vector<uint8_t> empty;
    bool ok = mmap.Push(empty.data(), empty.size());
    // 如果实现允许空数据，ok 可能为 true；关键是 Size 不变
    assert(ok);
    assert(mmap.Size() == original_size);
    LOG_INFO("test_push_empty_data passed");
}

void test_resize_expand_and_shrink() {
    std::filesystem::path test_file = "./test_mmap_handle_resize2.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {10, 20, 30};
    mmap.Push(data.data(), data.size());
    assert(mmap.Size() == data.size());

    // 扩大
    LOG_INFO("mmap.Size() {}", mmap.Size());
    LOG_INFO("mmap.Capacity() {}", mmap.Capacity());
    size_t resize = mmap.Capacity() + 10;
    bool ok_expand = mmap.Resize(resize);
    assert(ok_expand);
    assert(mmap.Size() == resize);
    assert(mmap.Capacity() >= resize);
    LOG_INFO("mmap.Size() {}", mmap.Size());
    LOG_INFO("mmap.Capacity() {}", mmap.Capacity());
    // 收缩
    bool ok_shrink = mmap.Resize(2);
    assert(ok_shrink);
    assert(mmap.Size() == 2);

    // 前两个字节应当仍是原来数据的前两个
    uint8_t* ptr = mmap.Data();
    assert(ptr[0] == data[0] && ptr[1] == data[1]);

    assert_file_consistent(test_file, mmap.Size());
    LOG_INFO("test_resize_expand_and_shrink passed");
}

void test_clear() {
    std::filesystem::path test_file = "./test_mmap_handle_clear.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    std::vector<uint8_t> data = {1, 2, 3};
    mmap.Push(data.data(), data.size());
    assert(mmap.Size() == 3);

    mmap.Clear();
    assert(mmap.Size() == 0);
    assert(mmap.Empty());

    // 映射仍应有效
    assert(mmap.Data() != nullptr);
    assert(mmap.Capacity() >= 0);
    assert_file_consistent(test_file, mmap.Size());
    LOG_INFO("test_clear passed");
}

void test_multiple_pushes() {
    std::filesystem::path test_file = "./test_mmap_handle_multi_push.dat";
    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);

    MMapHandle mmap(test_file);
    std::string s1 = "ABC";
    std::string s2 = "DEF";
    std::string s3 = "GHI";
    bool ok1 = mmap.Push(s1.data(), s1.size());
    bool ok2 = mmap.Push(s2.data(), s2.size());
    bool ok3 = mmap.Push(s3.data(), s3.size());
    assert(ok1 && ok2 && ok3);

    std::string read(reinterpret_cast<char*>(mmap.Data()), mmap.Size());
    assert(read == s1 + s2 + s3);
    LOG_INFO("test_multiple_pushes passed");
}

int main() {
    test_constructor_and_empty();
    test_push_and_size();
    test_push_and_read_string();
    test_push_empty_data();
    test_resize_expand_and_shrink();
    test_clear();
    test_multiple_pushes();
    LOG_INFO("All MMapHandle tests passed!");
    return 0;
}