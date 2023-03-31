#include <iostream>
#include <iomanip>
#include <vector>
#include <voice_allocator.h>

using namespace voice_allocator;

class TestVoice {
public:
    struct State {
        VoiceNote note;

        enum Gate {
            Open,
            Closed,
            ReTrigger,
        } gate;

        bool operator==(const State& other) const {
            if(gate != other.gate) {
                return false;
            }

            if(gate != Gate::Closed) {
                return note == other.note;
            } else {
                return true;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const State& dt) {
            const char* gate_str = "UNK";
            switch(dt.gate) {
            case Gate::Open:
                gate_str = "Opn";
                break;
            case Gate::ReTrigger:
                gate_str = "Ret";
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

    void cont(VoiceNote note) {
        State state = {.note = note, .gate = State::Gate::ReTrigger};
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

static void start(void* context, VoiceNote note) {
    TestVoice* test_data = (TestVoice*)context;
    test_data->start(note);
}

static void cont(void* context, VoiceNote note) {
    TestVoice* test_data = (TestVoice*)context;
    test_data->cont(note);
}

static void stop(void* context) {
    TestVoice* test_data = (TestVoice*)context;
    test_data->stop();
}

VoiceOutputCallbacks callbacks_mono[1] = {{.start = start, .cont = cont, .stop = stop}};

template <size_t N> class TestStepMaker {
public:
    TestStepMaker(VoiceManager<N>& voice_manager, std::vector<TestVoice::State>& expected_states)
        : _voice_manager(voice_manager)
        , _expected_states(expected_states) {
    }

    void on(VoiceNote note, TestVoice::State&& state) {
        _voice_manager.note_on(note);
        _expected_states.push_back(state);
    }

    void off(VoiceNote note, TestVoice::State&& state) {
        _voice_manager.note_off(note);
        _expected_states.push_back(state);
    }

    void on(VoiceNote note) {
        _voice_manager.note_on(note);
    }

    void off(VoiceNote note) {
        _voice_manager.note_off(note);
    }

    bool test(TestVoice& test_states) {
        bool success = test_states == _expected_states;

        if(!success) {
            test_states.dump_states_diff(_expected_states);
        }

        return success;
    }

private:
    VoiceManager<N>& _voice_manager;
    std::vector<TestVoice::State>& _expected_states;
};

bool test_voice_allocator_mono_high() {
    TestVoice test_states;
    std::vector<TestVoice::State> expected_states;
    VoiceManager<1> voice_manager;
    TestStepMaker<1> sm(voice_manager, expected_states);

    void* context[1] = {&test_states};

    voice_manager.set_output_callbacks(callbacks_mono, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonHighestNote);

    sm.on(0, {0, TestVoice::State::Gate::Open});
    sm.on(1, {1, TestVoice::State::Gate::Open});
    sm.on(2, {2, TestVoice::State::Gate::Open});

    sm.off(1);
    sm.off(2, {0, TestVoice::State::Gate::ReTrigger});
    sm.off(0, {0, TestVoice::State::Gate::Closed});

    sm.on(2, {2, TestVoice::State::Gate::Open});
    sm.on(1);
    sm.on(0);

    sm.off(2, {1, TestVoice::State::Gate::ReTrigger});
    sm.off(1, {0, TestVoice::State::Gate::ReTrigger});
    sm.off(0, {0, TestVoice::State::Gate::Closed});

    return sm.test(test_states);
}

bool test_voice_allocator_mono_low() {
    TestVoice test_states;
    std::vector<TestVoice::State> expected_states;
    VoiceManager<1> voice_manager;
    TestStepMaker<1> sm(voice_manager, expected_states);

    void* context[1] = {&test_states};

    voice_manager.set_output_callbacks(callbacks_mono, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonLowestNote);

    sm.on(0, {0, TestVoice::State::Gate::Open});
    sm.on(1);
    sm.on(2);

    sm.off(1);
    sm.off(2);
    sm.off(0, {0, TestVoice::State::Gate::Closed});

    sm.on(2, {2, TestVoice::State::Gate::Open});
    sm.on(1, {1, TestVoice::State::Gate::Open});
    sm.on(0, {0, TestVoice::State::Gate::Open});

    sm.off(0, {1, TestVoice::State::Gate::ReTrigger});
    sm.off(1, {2, TestVoice::State::Gate::ReTrigger});
    sm.off(2, {0, TestVoice::State::Gate::Closed});

    return sm.test(test_states);
}

bool test_voice_allocator_mono_newest() {
    TestVoice test_states;
    std::vector<TestVoice::State> expected_states;
    VoiceManager<1> voice_manager;
    TestStepMaker<1> sm(voice_manager, expected_states);

    void* context[1] = {&test_states};

    voice_manager.set_output_callbacks(callbacks_mono, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonNewestNote);

    sm.on(0, {0, TestVoice::State::Gate::Open});
    sm.on(1, {1, TestVoice::State::Gate::Open});
    sm.on(2, {2, TestVoice::State::Gate::Open});

    sm.off(2, {1, TestVoice::State::Gate::ReTrigger});
    sm.off(1, {0, TestVoice::State::Gate::ReTrigger});
    sm.off(0, {0, TestVoice::State::Gate::Closed});

    sm.on(2, {2, TestVoice::State::Gate::Open});
    sm.on(1, {1, TestVoice::State::Gate::Open});
    sm.on(0, {0, TestVoice::State::Gate::Open});

    sm.off(2);
    sm.off(1);
    sm.off(0, {0, TestVoice::State::Gate::Closed});

    return sm.test(test_states);
};

bool test_voice_allocator_mono_oldest() {
    TestVoice test_states;
    std::vector<TestVoice::State> expected_states;
    VoiceManager<1> voice_manager;
    TestStepMaker<1> sm(voice_manager, expected_states);

    void* context[1] = {&test_states};

    voice_manager.set_output_callbacks(callbacks_mono, context);
    voice_manager.set_strategy(VoiceManager<1>::Strategy::UnisonOldestNote);

    sm.on(0, {0, TestVoice::State::Gate::Open});
    sm.on(1);
    sm.on(2);

    sm.off(0, {1, TestVoice::State::Gate::ReTrigger});
    sm.off(1, {2, TestVoice::State::Gate::ReTrigger});
    sm.off(2, {0, TestVoice::State::Gate::Closed});

    sm.on(2, {2, TestVoice::State::Gate::Open});
    sm.on(1);
    sm.on(0);

    sm.off(0);
    sm.off(1);
    sm.off(2, {0, TestVoice::State::Gate::Closed});

    return sm.test(test_states);
}