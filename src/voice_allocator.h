#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <bitset>

typedef uint8_t VoiceNote;

/** Start a new note (set note and open gate) */
typedef void (*VoiceOutputStartCallback)(void*, VoiceNote);

/** Continue old note (set note and do retrigger) */
typedef void (*VoiceOutputСontinueCallback)(void*, VoiceNote);

/** Stop a note (close gate) */
typedef void (*VoiceOutputStopCallback)(void*);

/** Callbacks for the voice manager to use to output notes */
struct VoiceOutputCallbacks {
    VoiceOutputStartCallback start;
    VoiceOutputСontinueCallback cont;
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
        std::memset(&_callbacks, 0, sizeof(VoiceOutputCallbacks) * VoiceCount);
        std::memset(&_context, 0, sizeof(void*) * VoiceCount);
        reset();
    }

    ~VoiceManager() {
    }

    /** Reset the voice manager */
    void reset() {
        std::memset(_notes, false, sizeof(bool) * MaxNotes);
        std::memset(&_voice_notes, InvalidNote, sizeof(VoiceNote) * VoiceCount);
    }

    /** Set the strategy to use for voice allocation */
    void set_strategy(Strategy strategy) {
        m_strategy = strategy;
    }

    /** Set the callbacks[VoiceCount] to use for output */
    void set_output_callbacks(
        VoiceOutputCallbacks callbacks[VoiceCount],
        void* context[VoiceCount]) {
        std::memcpy(_callbacks, callbacks, sizeof(VoiceOutputCallbacks) * VoiceCount);
        std::memcpy(_context, context, sizeof(void*) * VoiceCount);
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
    static constexpr VoiceNote InvalidNote = 0xFF;

    /** The notes that are currently playing */
    bool _notes[MaxNotes];

    /** The note stack */
    NoteStack _note_stack;

    /** The callbacks and context to use for output */
    VoiceOutputCallbacks _callbacks[VoiceCount];
    void* _context[VoiceCount];

    /** The current notes for each voice */
    VoiceNote _voice_notes[VoiceCount];

    /** The strategy to use for voice allocation */
    Strategy m_strategy;

    void voice_start(size_t voice, VoiceNote note) {
        if(_voice_notes[voice] != note) {
            _voice_notes[voice] = note;
            if(_callbacks[voice].start) {
                _callbacks[voice].start(_context[voice], note);
            }
        }
    }

    void voice_continue(size_t voice, VoiceNote note) {
        if(_voice_notes[voice] != note) {
            _voice_notes[voice] = note;
            if(_callbacks[voice].cont) {
                _callbacks[voice].cont(_context[voice], note);
            }
        }
    }

    void voice_stop(size_t voice) {
        _voice_notes[voice] = InvalidNote;
        if(_callbacks[voice].stop) {
            _callbacks[voice].stop(_context[voice]);
        }
    }

    void all_outputs_start(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            voice_start(i, note);
        }
    }

    void all_outputs_continue(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            voice_continue(i, note);
        }
    }

    void all_outputs_stop() {
        for(size_t i = 0; i < VoiceCount; i++) {
            voice_stop(i);
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
            all_outputs_continue(highest_note);
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
            all_outputs_continue(lowest_note);
        } else {
            all_outputs_stop();
        }
    }

    void unison_newest_note_on() {
        all_outputs_start(_note_stack.top());
    }

    void unison_newest_note_off() {
        if(!_note_stack.empty()) {
            all_outputs_continue(_note_stack.top());
        } else {
            all_outputs_stop();
        }
    }

    void unison_oldest_note_on() {
        all_outputs_start(_note_stack.bottom());
    }

    void unison_oldest_note_off() {
        if(!_note_stack.empty()) {
            all_outputs_continue(_note_stack.bottom());
        } else {
            all_outputs_stop();
        }
    }
};