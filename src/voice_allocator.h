#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <bitset>

typedef uint8_t VoiceNote;

/** Start a new note (set note and open gate) */
typedef void (*VoiceOutputStartCallback)(void*, VoiceNote);

/** Stop a note (close gate) */
typedef void (*VoiceOutputStopCallback)(void*);

/** Callbacks for the voice manager to use to output notes */
struct VoiceOutputCallbacks {
    VoiceOutputStartCallback start;
    VoiceOutputStopCallback stop;
};

template <size_t VoiceCount> class VoiceManager {
public:
    enum Strategy {
        UnisonHighestNote,
        UnisonLowestNote,
        UnisonOldestNote,
        UnisonNewestNote,
    };

    VoiceManager() {
        for(size_t i = 0; i < VoiceCount; i++) {
            _callbacks[i].start = NULL;
            _callbacks[i].stop = NULL;
            _context[i] = NULL;
        }

        reset();
    }

    ~VoiceManager() {
    }

    /** Reset the voice manager */
    void reset() {
        _notes.reset();
    }

    /** Set the strategy to use for voice allocation */
    void set_strategy(Strategy strategy) {
        m_strategy = strategy;
        reset();
    }

    /** Set the callbacks[VoiceCount] to use for output */
    void set_output_callbacks(
        VoiceOutputCallbacks callbacks[VoiceCount],
        void* context[VoiceCount]) {
        for(size_t i = 0; i < VoiceCount; i++) {
            _callbacks[i] = callbacks[i];
            _context[i] = context[i];
        }
    }

    /** Note on */
    void note_on(VoiceNote note) {
        if(note >= MaxNotes) {
            note = MaxNotes - 1;
        }

        _notes.set(note);

        switch(m_strategy) {
        case UnisonHighestNote:
            unison_highest_note_on(note);
            break;
        case UnisonLowestNote:
            unison_lowest_note_on(note);
            break;
        default:
            std::abort();
        }
    }

    /** Note off */
    void note_off(VoiceNote note) {
        if(note >= MaxNotes) {
            note = MaxNotes - 1;
        }

        _notes.reset(note);

        switch(m_strategy) {
        case UnisonHighestNote:
            unison_highest_note_off(note);
            break;
        case UnisonLowestNote:
            unison_lowest_note_off(note);
            break;
        default:
            std::abort();
        }
    }

private:
    /** Max midi notes count */
    static constexpr size_t MaxNotes = 128;

    /** The notes that are currently playing */
    std::bitset<MaxNotes> _notes;

    /** The callbacks and context to use for output */
    VoiceOutputCallbacks _callbacks[VoiceCount];
    void* _context[VoiceCount];

    /** The strategy to use for voice allocation */
    Strategy m_strategy;

    void all_outputs_start(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            if(_callbacks[i].start) {
                _callbacks[i].start(_context[i], note);
            }
        }
    }

    void all_outputs_stop() {
        for(size_t i = 0; i < VoiceCount; i++) {
            if(_callbacks[i].stop) {
                _callbacks[i].stop(_context[i]);
            }
        }
    }

    void unison_highest_note_on(VoiceNote note) {
        // get the highest note
        VoiceNote highest_note = 0;

        for(size_t i = 0; i < MaxNotes; i++) {
            if(_notes.test(i)) {
                highest_note = i;
            }
        }

        all_outputs_start(highest_note);
    }

    void unison_highest_note_off(VoiceNote note) {
        // get the highest note
        VoiceNote highest_note = 0;
        bool found = false;

        for(size_t i = 0; i < MaxNotes; i++) {
            if(_notes.test(i)) {
                highest_note = i;
                found = true;
            }
        }

        if(found) {
            all_outputs_start(highest_note);
        } else {
            all_outputs_stop();
        }
    }

    void unison_lowest_note_on(VoiceNote note) {
        // get the lowest note
        VoiceNote lowest_note = MaxNotes - 1;

        for(size_t i = 0; i < MaxNotes; i++) {
            if(_notes.test(i)) {
                lowest_note = i;
                break;
            }
        }

        all_outputs_start(lowest_note);
    }

    void unison_lowest_note_off(VoiceNote note) {
        // get the lowest note
        VoiceNote lowest_note = MaxNotes - 1;
        bool found = false;

        for(size_t i = 0; i < MaxNotes; i++) {
            if(_notes.test(i)) {
                lowest_note = i;
                found = true;
                break;
            }
        }

        if(found) {
            all_outputs_start(lowest_note);
        } else {
            all_outputs_stop();
        }
    }
};