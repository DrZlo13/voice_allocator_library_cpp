#include <iostream>
#include <vector>
#include <voice_allocator.h>
#include <iostream>
#include <iomanip>

class TestVoice {
public:
    struct State {
        VoiceNote note;

        enum Gate {
            Open,
            Closed,
        } gate;

        bool operator==(const State& other) const {
            return note == other.note && gate == other.gate;
        }

        friend std::ostream& operator<<(std::ostream& os, const State& dt) {
            const char* gate_str = "UNK";
            switch(dt.gate) {
            case Gate::Open:
                gate_str = "Opn";
                break;
            case Gate::Closed:
                gate_str = "Cls";
                break;
            }

            os << (uint32_t)dt.note << " " << gate_str;

            return os;
        }
    };

    std::vector<State> data;

    void start(VoiceNote note) {
        State state = {.note = note, .gate = State::Gate::Open};
        data.push_back(state);
    }

    void stop() {
        State state = {.note = 0, .gate = State::Gate::Closed};
        data.push_back(state);
    }

    void reset() {
        data.clear();
    }

    bool operator==(const std::vector<State>& other) const {
        return data == other;
    }

    void dump_states_diff(const std::vector<State>& other) const {
        if(data.size() != other.size()) {
            std::cout << "Size mismatch: " << data.size() << " != " << other.size() << std::endl;
            return;
        }

        std::cout << "      State    Expected" << std::endl;
        for(size_t i = 0; i < data.size(); i++) {
            if(data[i] == other[i]) {
                std::cout << "  ";
            } else {
                std::cout << "x ";
            }
            std::cout << std::setw(2) << i << ": " << data[i] << " != " << other[i] << std::endl;
        }
    }
};

void start(void* context, VoiceNote note) {
    TestVoice* test_data = (TestVoice*)context;
    test_data->start(note);
}

void stop(void* context) {
    TestVoice* test_data = (TestVoice*)context;
    test_data->stop();
}

bool test_voice_allocator_mono_high() {
    bool success = true;

    TestVoice test_states;
    VoiceOutputCallbacks callbacks[1] = {{.start = start, .stop = stop}};
    void* context[1] = {&test_states};

    VoiceManager<1> voice_manager;
    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonHighestNote);

    voice_manager.note_on(0); // start note 0, note 0 should be playing
    voice_manager.note_on(1); // start note 1, note 1 should be playing
    voice_manager.note_on(2); // start note 2, note 2 should be playing

    voice_manager.note_off(1); // stop note 1, note 2 should still be playing
    voice_manager.note_off(2); // stop note 2, note 0 should still be playing
    voice_manager.note_off(0); // stop note 0, no notes should be playing

    voice_manager.note_on(2); // start note 2, note 2 should be playing
    voice_manager.note_on(1); // start note 1, note 2 should be playing
    voice_manager.note_on(0); // start note 0, note 2 should be playing

    voice_manager.note_off(2); // stop note 2, note 1 should still be playing
    voice_manager.note_off(1); // stop note 1, note 0 should still be playing
    voice_manager.note_off(0); // stop note 0, no notes should be playing

    std::vector<TestVoice::State> expected_states = {
        {0, TestVoice::State::Gate::Open},
        {1, TestVoice::State::Gate::Open},
        {2, TestVoice::State::Gate::Open},

        {2, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Closed},

        {2, TestVoice::State::Gate::Open},
        {2, TestVoice::State::Gate::Open},
        {2, TestVoice::State::Gate::Open},

        {1, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Closed},
    };

    success = success && test_states == expected_states;

    if(!success) {
        test_states.dump_states_diff(expected_states);
    }

    return success;
}

bool test_voice_allocator_mono_low() {
    bool success = true;

    TestVoice test_states;
    VoiceOutputCallbacks callbacks[1] = {{.start = start, .stop = stop}};
    void* context[1] = {&test_states};

    VoiceManager<1> voice_manager;
    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonLowestNote);

    voice_manager.note_on(0); // start note 0, note 0 should be playing
    voice_manager.note_on(1); // start note 1, note 0 should be playing
    voice_manager.note_on(2); // start note 2, note 0 should be playing

    voice_manager.note_off(1); // stop note 1, note 0 should still be playing
    voice_manager.note_off(2); // stop note 2, note 0 should still be playing
    voice_manager.note_off(0); // stop note 0, no notes should be playing

    voice_manager.note_on(2); // start note 2, note 2 should be playing
    voice_manager.note_on(1); // start note 1, note 1 should be playing
    voice_manager.note_on(0); // start note 0, note 0 should be playing

    voice_manager.note_off(0); // stop note 0, note 1 should still be playing
    voice_manager.note_off(1); // stop note 1, note 2 should still be playing
    voice_manager.note_off(2); // stop note 2, no notes should be playing

    std::vector<TestVoice::State> expected_states = {
        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},

        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Closed},

        {2, TestVoice::State::Gate::Open},
        {1, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Open},

        {1, TestVoice::State::Gate::Open},
        {2, TestVoice::State::Gate::Open},
        {0, TestVoice::State::Gate::Closed},
    };

    success = success && test_states == expected_states;

    if(!success) {
        test_states.dump_states_diff(expected_states);
    }

    return success;
}

bool test_voice_allocator_mono_newest() {
    bool success = true;

    TestVoice test_states;
    VoiceOutputCallbacks callbacks[1] = {{.start = start, .stop = stop}};
    void* context[1] = {&test_states};

    VoiceManager<1> voice_manager;
    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonNewestNote);

    std::vector<TestVoice::State> expected_states;

    voice_manager.note_on(0);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states.push_back({1, TestVoice::State::Gate::Open});
    voice_manager.note_on(2);
    expected_states.push_back({2, TestVoice::State::Gate::Open});

    voice_manager.note_off(2);
    expected_states.push_back({1, TestVoice::State::Gate::Open});
    voice_manager.note_off(1);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_off(0);
    expected_states.push_back({0, TestVoice::State::Gate::Closed});

    voice_manager.note_on(2);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states.push_back({1, TestVoice::State::Gate::Open});
    voice_manager.note_on(0);
    expected_states.push_back({0, TestVoice::State::Gate::Open});

    voice_manager.note_off(2);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_off(1);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_off(0);
    expected_states.push_back({0, TestVoice::State::Gate::Closed});

    success = success && test_states == expected_states;

    if(!success) {
        test_states.dump_states_diff(expected_states);
    }

    return success;
};

bool test_voice_allocator_mono_oldest() {
    bool success = true;

    TestVoice test_states;
    VoiceOutputCallbacks callbacks[1] = {{.start = start, .stop = stop}};
    void* context[1] = {&test_states};

    VoiceManager<1> voice_manager;
    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonOldestNote);

    std::vector<TestVoice::State> expected_states;

    voice_manager.note_on(0);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states.push_back({0, TestVoice::State::Gate::Open});
    voice_manager.note_on(2);
    expected_states.push_back({0, TestVoice::State::Gate::Open});

    voice_manager.note_off(0);
    expected_states.push_back({1, TestVoice::State::Gate::Open});
    voice_manager.note_off(1);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_off(2);
    expected_states.push_back({0, TestVoice::State::Gate::Closed});

    voice_manager.note_on(2);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_on(0);
    expected_states.push_back({2, TestVoice::State::Gate::Open});

    voice_manager.note_off(0);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_off(1);
    expected_states.push_back({2, TestVoice::State::Gate::Open});
    voice_manager.note_off(2);
    expected_states.push_back({0, TestVoice::State::Gate::Closed});

    success = success && test_states == expected_states;

    if(!success) {
        test_states.dump_states_diff(expected_states);
    }

    return success;
}

struct Test {
    const char* name;
    bool (*test)();
};

#define TEST(name) #name, name

int main() {
    std::vector<Test> tests = {
        {TEST(test_voice_allocator_mono_high)},
        {TEST(test_voice_allocator_mono_low)},
        {TEST(test_voice_allocator_mono_newest)},
        {TEST(test_voice_allocator_mono_oldest)},
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