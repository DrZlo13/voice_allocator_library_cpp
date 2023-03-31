#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <numeric>
#include <algorithm>
#include <cstring>

namespace voice_allocator {

typedef uint8_t VoiceNote;

namespace constants {
const size_t MaxNotes = 128;
const VoiceNote InvalidNote = UINT8_MAX;
const size_t InvalidVoice = SIZE_MAX;
}

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
        PolyLeastRecentlyUsed,
        PolyMostRecentlyUsed,
    };

    VoiceManager() {
        reset();
    }

    /** Reset the voice manager */
    void reset() {
        _note_stack.reset();
        _voice_stack.reset();
    }

    /** Set the strategy to use for voice allocation */
    void set_strategy(Strategy strategy) {
        _strategy = strategy;
    }

    /** Set the callbacks[VoiceCount] to use for output */
    void set_output_callbacks(
        VoiceOutputCallbacks callbacks[VoiceCount],
        void* context[VoiceCount]) {
        _voice_stack.set_output_callbacks(callbacks, context);
    }

    /** Note on */
    void note_on(VoiceNote note) {
        if(note >= constants::MaxNotes) {
            note = constants::MaxNotes - 1;
        }

        if(strategy_is_unison()) {
            _note_stack.push(note);
        }

        switch(_strategy) {
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
        case PolyLeastRecentlyUsed:
            poly_least_recently_used_note_on(note);
            break;
        case PolyMostRecentlyUsed:
            poly_most_recently_used_note_on(note);
            break;
        }
    }

    /** Note off */
    void note_off(VoiceNote note) {
        if(note >= constants::MaxNotes) {
            note = constants::MaxNotes - 1;
        }

        if(strategy_is_unison()) {
            _note_stack.pop(note);
        }

        switch(_strategy) {
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
        case PolyLeastRecentlyUsed:
        case PolyMostRecentlyUsed:
            poly_note_off(note);
            break;
        }
    }

private:
    class NoteStack;
    class VoiceStack;

    /** The note stack */
    NoteStack _note_stack;
    VoiceStack _voice_stack;

    /** The strategy to use for voice allocation */
    Strategy _strategy;

    bool strategy_is_unison() {
        return _strategy == UnisonHighestNote || _strategy == UnisonLowestNote ||
               _strategy == UnisonNewestNote || _strategy == UnisonOldestNote;
    }

    void unison_outputs_start(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            _voice_stack.voice_start(i, note, false);
        }
    }

    void unison_outputs_continue(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            _voice_stack.voice_continue(i, note, false);
        }
    }

    void unison_outputs_stop() {
        for(size_t i = 0; i < VoiceCount; i++) {
            _voice_stack.voice_stop(i, false);
        }
    }

    bool get_highest_note(VoiceNote& note) {
        if(!_note_stack.empty()) {
            note = _note_stack.get_highest_note();
            return true;
        }

        return false;
    }

    bool get_lowest_note(VoiceNote& note) {
        if(!_note_stack.empty()) {
            note = _note_stack.get_lowest_note();
            return true;
        }

        return false;
    }

    void unison_highest_note_on() {
        VoiceNote highest_note;
        get_highest_note(highest_note);
        unison_outputs_start(highest_note);
    }

    void unison_highest_note_off() {
        VoiceNote highest_note = 0;
        if(get_highest_note(highest_note)) {
            unison_outputs_continue(highest_note);
        } else {
            unison_outputs_stop();
        }
    }

    void unison_lowest_note_on() {
        VoiceNote lowest_note = 0;
        get_lowest_note(lowest_note);
        unison_outputs_start(lowest_note);
    }

    void unison_lowest_note_off() {
        VoiceNote lowest_note = 0;
        if(get_lowest_note(lowest_note)) {
            unison_outputs_continue(lowest_note);
        } else {
            unison_outputs_stop();
        }
    }

    void unison_newest_note_on() {
        unison_outputs_start(_note_stack.top());
    }

    void unison_newest_note_off() {
        if(!_note_stack.empty()) {
            unison_outputs_continue(_note_stack.top());
        } else {
            unison_outputs_stop();
        }
    }

    void unison_oldest_note_on() {
        unison_outputs_start(_note_stack.bottom());
    }

    void unison_oldest_note_off() {
        if(!_note_stack.empty()) {
            unison_outputs_continue(_note_stack.bottom());
        } else {
            unison_outputs_stop();
        }
    }

    void poly_least_recently_used_note_on(VoiceNote note) {
        size_t voice = _voice_stack.get_free();
        if(voice == constants::InvalidVoice) {
            voice = _voice_stack.get_least_recently_used();
        }
        _voice_stack.voice_start(voice, note);
    }

    void poly_most_recently_used_note_on(VoiceNote note) {
        size_t voice = _voice_stack.get_free();
        if(voice == constants::InvalidVoice) {
            voice = _voice_stack.get_most_recently_used();
        }
        _voice_stack.voice_start(voice, note);
    }

    void poly_note_off(VoiceNote note) {
        size_t voice = _voice_stack.get_by_note(note);
        if(voice != constants::InvalidVoice) {
            _voice_stack.voice_stop(voice);
        }
    }
};

template <size_t VoiceCount> class VoiceManager<VoiceCount>::NoteStack {
public:
    NoteStack() {
        reset();
    }

    void reset() {
        _top = 0;
        std::fill_n(_notes, constants::MaxNotes, constants::InvalidNote);
    }

    void push(VoiceNote note) {
        _notes[_top] = note;
        _top++;
        if(_top >= constants::MaxNotes) {
            _top = constants::MaxNotes - 1;
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

    VoiceNote get_highest_note() {
        VoiceNote highest_note = 0;
        for(size_t i = 0; i < _top; i++) {
            if(_notes[i] > highest_note) {
                highest_note = _notes[i];
            }
        }
        return highest_note;
    }

    VoiceNote get_lowest_note() {
        VoiceNote lowest_note = constants::MaxNotes - 1;
        for(size_t i = 0; i < _top; i++) {
            if(_notes[i] < lowest_note) {
                lowest_note = _notes[i];
            }
        }
        return lowest_note;
    }

private:
    VoiceNote _notes[constants::MaxNotes];
    size_t _top;
};

template <size_t VoiceCount> class VoiceManager<VoiceCount>::VoiceStack {
public:
    VoiceStack() {
        std::fill_n(
            _callbacks, VoiceCount, (VoiceOutputCallbacks){.start = 0, .cont = 0, .stop = 0});
        std::fill_n(_context, VoiceCount, nullptr);
        reset();
    }

    void set_output_callbacks(
        VoiceOutputCallbacks callbacks[VoiceCount],
        void* context[VoiceCount]) {
        std::copy(callbacks, callbacks + VoiceCount, _callbacks);
        std::copy(context, context + VoiceCount, _context);
    }

    void reset() {
        _round_robin = 0;
        std::iota(_voice, _voice + VoiceCount, 0);
        std::fill_n(_notes, VoiceCount, constants::InvalidNote);
    }

    size_t get_by_note(VoiceNote note) {
        for(size_t i = 0; i < VoiceCount; i++) {
            if(_notes[i] == note) {
                return i;
            }
        }
        return constants::InvalidVoice;
    }

    size_t get_free() {
        for(size_t i = 0; i < VoiceCount; i++) {
            if(_notes[i] == constants::InvalidNote) {
                return i;
            }
        }
        return constants::InvalidVoice;
    }

    size_t get_least_recently_used() {
        return _voice[VoiceCount - 1];
    }

    size_t get_most_recently_used() {
        return _voice[0];
    }

    void voice_start(size_t voice, VoiceNote note, bool need_to_touch = true) {
        if(_notes[voice] != note) {
            _notes[voice] = note;
            if(_callbacks[voice].start) {
                _callbacks[voice].start(_context[voice], note);
            }
            if(need_to_touch) touch(voice, note);
        }
    }

    void voice_continue(size_t voice, VoiceNote note, bool need_to_touch = true) {
        if(_notes[voice] != note) {
            _notes[voice] = note;
            if(_callbacks[voice].cont) {
                _callbacks[voice].cont(_context[voice], note);
            }
            if(need_to_touch) touch(voice, note);
        }
    }

    void voice_stop(size_t voice, bool need_to_touch = true) {
        _notes[voice] = constants::InvalidNote;
        if(_callbacks[voice].stop) {
            _callbacks[voice].stop(_context[voice]);
        }
        if(need_to_touch) touch(voice, constants::InvalidNote);
    }

private:
    /** Array of voice indices */
    size_t _voice[VoiceCount];

    /** Round robin index */
    size_t _round_robin;

    /** Array of voice notes */
    VoiceNote _notes[VoiceCount];

    /** Callbacks to output events */
    VoiceOutputCallbacks _callbacks[VoiceCount];
    void* _context[VoiceCount];

    void touch(size_t voice_index, VoiceNote note) {
        // Move voice to the start of the stack
        int32_t source = VoiceCount - 1;
        int32_t destination = VoiceCount - 1;
        while(source >= 0) {
            if(_voice[source] != voice_index) {
                _voice[destination--] = _voice[source];
            }
            --source;
        }
        _voice[0] = voice_index;
        _notes[voice_index] = note;
    }
};
}