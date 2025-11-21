#pragma once

#include "SequenceData.h"
#include "SequenceMeta.h"

struct SequencePosition
{
    uint8_t clip = 0;
    uint8_t pattern = 0;
    uint8_t quarterNote = 0;
    uint8_t pulse = 0; // Pulse within the current quarter note (0-95 for PPQN=96)
};

class Sequence
{
private:
    SequenceData data;

    bool dirty = false;

    bool playing = false;
    int16_t clocksTillStart = 0;            // MIDI clocks until playback starts (24 PPQN, 0 = not scheduled, negative = count-in)
    bool record = false;

    bool clockOutput = false;

    // Internal sequencer timing (96 PPQN)
    uint32_t lastPulseTime = 0;             // Last time the sequencer tick was processed (microseconds)
    uint32_t pulseCounter = 0;              // Global tick counter for note-off scheduling (96 PPQN)
    uint8_t currentPulse = 0;               // Current pulse for swing timing (alternates 0/1 for on/off beat)
    uint32_t usPerPulse[2];                 // Microseconds per pulse with swing (on-beat/off-beat)

    // Unswung Clock timing (24 PPQN) - For MIDI Clock
    uint32_t lastClockTime = 0;             // Last time a MIDI clock pulse was sent (microseconds)
    uint8_t  clockCounter = 0;              // Counter for clocks (0-23, wraps at 24)
    uint32_t usPerClock;                    // Microseconds per clock pulse (24 PPQN)

    // Unswung LED animation timing (1 PPQN) - For LED animation
    uint32_t lastQuarterNoteTime = 0;       // Last time a quarter note boundary was crossed (microseconds)
    uint32_t usPerQuarterNote;              // Microseconds per quarterNote (usPerClock * 24) - Just a helper variable

    // Playback state per track
    struct TrackPlayback {
        bool playing = false;                     // Is this track playing
        SequencePosition position;                // Current playback position
        uint8_t nextClip = 255;                   // Next clip to play (255 = none)
        uint32_t lastEvent = 0;                   // Last event time (for animation)
        unordered_set<uint8_t> activeNotes;       // Currently active notes
        map<uint8_t, uint32_t> noteOffMap;        // note -> tick time (for overwrite lookup)
        multimap<uint32_t, uint8_t> noteOffQueue; // tick -> note (for efficient processing)
    };

    vector<TrackPlayback> trackPlayback;

    void UpdateTiming();
    void ProcessTrack(uint8_t track);
public:
    const static uint16_t PPQN = 96;

    Sequence(uint8_t tracks = 8);
    void New(uint8_t tracks = 8);

    void Tick();

    void Play();
    void Play(uint8_t track);
    void PlayClip(uint8_t track, uint8_t clip);
    bool Playing();
    bool Playing(uint8_t track);

    void Stop();
    void Stop(uint8_t track);

    void EnableRecord(bool val);
    bool RecordEnabled();

    void EnableClockOutput(bool val);
    bool ClockOutputEnabled();

    uint8_t GetTrackCount();

    // Clip management
    uint8_t GetClipCount(uint8_t track);
    bool ClipExists(uint8_t track, uint8_t clip);
    bool GetClipEnabled(uint8_t track, uint8_t clip);
    void SetClipEnabled(uint8_t track, uint8_t clip, bool enabled);
    bool NewClip(uint8_t track, uint8_t clipId);
    void DeleteClip(uint8_t track, uint8_t clip);
    void CopyClip(uint8_t sourceTrack, uint8_t sourceClip, uint8_t destTrack, uint8_t destClip);

    // Pattern management (now with clip parameter)
    uint8_t GetPatternCount(uint8_t track, uint8_t clip);
    SequencePattern& GetPattern(uint8_t track, uint8_t clip, uint8_t pattern);
    int8_t NewPattern(uint8_t track, uint8_t clip, uint8_t quarterNotes = 16);
    void ClearPattern(uint8_t track, uint8_t clip, uint8_t pattern);
    void DeletePattern(uint8_t track, uint8_t clip, uint8_t pattern);
    void CopyPattern(uint8_t sourceTrack, uint8_t sourceClip, uint8_t sourcePattern, uint8_t destTrack, uint8_t destClip, uint8_t destPattern = 255); // if destPattern is 255, then will create new pattern

    uint8_t GetChannel(uint8_t track);
    void SetChannel(uint8_t track, uint8_t channel);

    uint16_t GetBPM();
    void SetBPM(uint16_t bpm);

    uint8_t GetSwing();
    void SetSwing(uint8_t swing);

    bool GetDirty();
    void SetDirty(bool val = true);

    bool GetSolo(uint8_t track);
    void SetSolo(uint8_t track, bool val);

    bool GetMute(uint8_t track);
    void SetMute(uint8_t track, bool val);

    bool GetRecord(uint8_t track);
    void SetRecord(uint8_t track, bool val);

    bool GetEnabled(uint8_t track); // Disabled if mute or other track have solo

    SequencePosition& GetPosition(uint8_t track);
    void SetPosition(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t quarterNote = 0);
    void SetClip(uint8_t track, uint8_t clip);
    void SetPattern(uint8_t track, uint8_t pattern);

    uint8_t GetNextClip(uint8_t track);
    void SetNextClip(uint8_t track, uint8_t clip);

    Fract16 GetQuarterNoteProgress();
    uint8_t QuarterNoteProgressBreath(uint8_t lowBound = 0); // LED Helper 

    void RecordEvent(MidiPacket packet);
};
