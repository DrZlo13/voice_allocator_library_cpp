#include <iostream>
#include <vector>

struct Test {
    const char* name;
    bool (*test)();
};

#define TEST(name) #name, name

bool test_voice_allocator_mono_high();
bool test_voice_allocator_mono_low();
bool test_voice_allocator_mono_newest();
bool test_voice_allocator_mono_oldest();
bool test_voice_allocator_poly_4_least_recent_used();
bool test_voice_allocator_poly_4_most_recent_used();

int main() {
    std::vector<Test> tests = {
        {TEST(test_voice_allocator_mono_high)},
        {TEST(test_voice_allocator_mono_low)},
        {TEST(test_voice_allocator_mono_newest)},
        {TEST(test_voice_allocator_mono_oldest)},
        {TEST(test_voice_allocator_poly_4_least_recent_used)},
        {TEST(test_voice_allocator_poly_4_most_recent_used)},
    };

    bool success = true;

    // run tests
    for(size_t i = 0; i < tests.size(); i++) {
        bool result = tests[i].test();
        success = success && result;

        std::cout << "Test[" << i << "] " << tests[i].name << ": ";
        std::cout << (result ? " PASS " : " FAIL ") << std::endl;
    }

    return success ? 0 : 1;
}