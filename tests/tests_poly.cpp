#include <iostream>
#include <iomanip>
#include <vector>
#include <voice_allocator.h>

class PolyTestVoice {
public:
    struct State {
        VoiceNote note;

        enum Gate {
            Open,
            Closed,
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
        }

        size_t data_size = std::max(data.size(), other.size());

        std::cout << "      State    Expected" << std::endl;
        for(size_t i = 0; i < data_size; i++) {
            if(i < data.size() && i < other.size()) {
                if(data[i] == other[i]) {
                    std::cout << "  ";
                } else {
                    std::cout << "x ";
                }
                std::cout << std::setw(2) << i << ": " << data[i] << " != " << other[i]
                          << std::endl;
            } else if(i < data.size()) {
                std::cout << "x ";
                std::cout << std::setw(2) << i << ": " << data[i] << " != "
                          << "- ---" << std::endl;
            } else if(i < other.size()) {
                std::cout << "x ";
                std::cout << std::setw(2) << i << ": "
                          << "- ---"
                          << " != " << other[i] << std::endl;
            }
        }
    }
};

static void start(void* context, VoiceNote note) {
    PolyTestVoice* test_data = (PolyTestVoice*)context;
    test_data->start(note);
}

static void stop(void* context) {
    PolyTestVoice* test_data = (PolyTestVoice*)context;
    test_data->stop();
}

bool test_voice_allocator_poly_4_least_recent_used() {
    constexpr size_t num_voices = 4;
    PolyTestVoice test_states[num_voices];
    std::vector<PolyTestVoice::State> expected_states[num_voices];
    VoiceManager<num_voices> voice_manager;

    VoiceOutputCallbacks callbacks[num_voices] = {
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
    };

    void* context[num_voices] = {
        &test_states[0],
        &test_states[1],
        &test_states[2],
        &test_states[3],
    };

    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<num_voices>::Strategy::PolyLeastRecentlyUsed);

    voice_manager.note_on(0);
    expected_states[0].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states[1].push_back({.note = 1, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(2);
    expected_states[2].push_back({.note = 2, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(3);
    expected_states[3].push_back({.note = 3, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(4);
    expected_states[0].push_back({.note = 4, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(5);
    expected_states[1].push_back({.note = 5, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(6);
    expected_states[2].push_back({.note = 6, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(7);
    expected_states[3].push_back({.note = 7, .gate = PolyTestVoice::State::Gate::Open});

    voice_manager.note_off(7);
    expected_states[3].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(6);
    expected_states[2].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(5);
    expected_states[1].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(4);
    expected_states[0].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(3);
    voice_manager.note_off(2);
    voice_manager.note_off(1);
    voice_manager.note_off(0);

    bool success = true;
    for(size_t i = 0; i < num_voices; i++) {
        if(test_states[i].data != expected_states[i]) {
            test_states[i].dump_states_diff(expected_states[i]);
            success = false;
        }
    }

    return success;
}

bool test_voice_allocator_poly_4_most_recent_used() {
    constexpr size_t num_voices = 4;
    PolyTestVoice test_states[num_voices];
    std::vector<PolyTestVoice::State> expected_states[num_voices];
    VoiceManager<num_voices> voice_manager;

    VoiceOutputCallbacks callbacks[num_voices] = {
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
        {.start = start, .stop = stop},
    };

    void* context[num_voices] = {
        &test_states[0],
        &test_states[1],
        &test_states[2],
        &test_states[3],
    };

    voice_manager.set_output_callbacks(callbacks, context);
    voice_manager.set_strategy(VoiceManager<num_voices>::Strategy::PolyMostRecentlyUsed);

    voice_manager.note_on(0);
    expected_states[0].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(1);
    expected_states[1].push_back({.note = 1, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(2);
    expected_states[2].push_back({.note = 2, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(3);
    expected_states[3].push_back({.note = 3, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(4);
    expected_states[3].push_back({.note = 4, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(5);
    expected_states[3].push_back({.note = 5, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(6);
    expected_states[3].push_back({.note = 6, .gate = PolyTestVoice::State::Gate::Open});
    voice_manager.note_on(7);
    expected_states[3].push_back({.note = 7, .gate = PolyTestVoice::State::Gate::Open});

    voice_manager.note_off(7);
    expected_states[3].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(6);
    voice_manager.note_off(5);
    voice_manager.note_off(4);
    voice_manager.note_off(3);
    voice_manager.note_off(2);
    expected_states[2].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(1);
    expected_states[1].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});
    voice_manager.note_off(0);
    expected_states[0].push_back({.note = 0, .gate = PolyTestVoice::State::Gate::Closed});

    bool success = true;
    for(size_t i = 0; i < num_voices; i++) {
        if(test_states[i].data != expected_states[i]) {
            test_states[i].dump_states_diff(expected_states[i]);
            success = false;
        }
    }

    return success;
}