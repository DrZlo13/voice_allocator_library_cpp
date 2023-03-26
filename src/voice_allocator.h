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
        UnisonNewestNote,
        UnisonOldestNote,
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
        for(size_t i = 0; i < MaxNotes; i++) {
            _notes[i] = false;
        }
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

        _notes[note] = true;
        _note_stack.push(note);

        switch(m_strategy) {
        case UnisonHighestNote:
            unison_highest_note_on();
            break;
        case UnisonLowestNote:
            unison_lowest_note_on();
            break;
        case UnisonNewestNote:
            unison_newest_note_on();
            break;
        case UnisonOldestNote:
            unison_oldest_note_on();
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

        _notes[note] = false;
        _note_stack.pop(note);

        switch(m_strategy) {
        case UnisonHighestNote:
            unison_highest_note_off();
            break;
        case UnisonLowestNote:
            unison_lowest_note_off();
            break;
        case UnisonNewestNote:
            unison_newest_note_off();
            break;
        case UnisonOldestNote:
            unison_oldest_note_off();
            break;
        default:
            std::abort();
        }
    }

private:
    class NoteStack {
    public:
        NoteStack() {
            _top = 0;
            for(size_t i = 0; i < VoiceManager::MaxNotes; i++) {
                _notes[i] = 0;
            }
        }

        void push(VoiceNote note) {
            _notes[_top] = note;
            _top++;
            if(_top >= VoiceManager::MaxNotes) {
                _top = VoiceManager::MaxNotes - 1;
            }
        }

        void pop(VoiceNote note) {
            for(size_t i = 0; i < _top; i++) {
                if(_notes[i] == note) {
                    for(size_t j = i; j < _top - 1; j++) {
                        _notes[j] = _notes[j + 1];
                    }
                    _top--;
                    return;
                }
            }
        }

        VoiceNote top() {
            return _notes[_top - 1];
        }

        VoiceNote bottom() {
            return _notes[0];
        }

        bool empty() {
            return _top == 0;
        }

    private:
        /** Max midi notes count */
        static constexpr size_t MaxNotes = 128;
        VoiceNote _notes[MaxNotes];
        size_t _top;
    };

    /** Max midi notes count */
    static constexpr size_t MaxNotes = 128;

    /** The notes that are currently playing */
    bool _notes[MaxNotes];

    /** The note stack */
    NoteStack _note_stack;

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

    bool get_highest_note(VoiceNote& note) {
        for(size_t i = MaxNotes; i > 0; i--) {
            size_t index = i - 1;
            if(_notes[index]) {
                note = index;
                return true;
            }
        }

        return false;
    }

    void unison_highest_note_on() {
        VoiceNote highest_note;
        get_highest_note(highest_note);
        all_outputs_start(highest_note);
    }

    void unison_highest_note_off() {
        VoiceNote highest_note = 0;
        if(get_highest_note(highest_note)) {
            all_outputs_start(highest_note);
        } else {
            all_outputs_stop();
        }
    }

    bool get_lowest_note(VoiceNote& note) {
        for(size_t i = 0; i < MaxNotes; i++) {
            if(_notes[i]) {
                note = i;
                return true;
            }
        }

        return false;
    }

    void unison_lowest_note_on() {
        VoiceNote lowest_note = 0;
        get_lowest_note(lowest_note);
        all_outputs_start(lowest_note);
    }

    void unison_lowest_note_off() {
        VoiceNote lowest_note = 0;
        if(get_lowest_note(lowest_note)) {
            all_outputs_start(lowest_note);
        } else {
            all_outputs_stop();
        }
    }

    void unison_newest_note_on() {
        all_outputs_start(_note_stack.top());
    }

    void unison_newest_note_off() {
        if(!_note_stack.empty()) {
            all_outputs_start(_note_stack.top());
        } else {
            all_outputs_stop();
        }
    }

    void unison_oldest_note_on() {
        all_outputs_start(_note_stack.bottom());
    }

    void unison_oldest_note_off() {
        if(!_note_stack.empty()) {
            all_outputs_start(_note_stack.bottom());
        } else {
            all_outputs_stop();
        }
    }
};