#pragma once

#include "SequenceData.h"
#include "SequenceMeta.h"
#include <unordered_map>

struct SequencePosition
{
    uint8_t clip = 0;
    uint8_t pattern = 0;
    uint8_t step = 0;
    uint16_t pulse = 0; // Pulse within the current step
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

    // Clip switching timing
    uint16_t barLength = 16; // Length of each bay in steps 

    // Internal sequencer timing (96 PPQN)
    uint32_t lastPulseTime = 0;             // Last time the sequencer tick was processed (microseconds)
    uint32_t pulseSinceStart = 0;           // Global tick counter for note-off scheduling (96 PPQN)
    uint16_t  currentStep = 0;
    uint16_t  currentPulse = 0;              // Current pulse for swing timing (alternates 0/1 for on/off beat)
    uint32_t usPerPulse[2];                 // Microseconds per pulse with swing (on-beat/off-beat)
    uint8_t stepDivision = 16;                // division: 4=quarter,8=8th,16=16th per step
    uint16_t pulsesPerStep = (PPQN * 4) / 16;      // will be updated with stepDivision
    uint8_t lastRecordLayer = 0;
    uint8_t currentRecordLayer = 0;

    // Unswung Clock timing (24 PPQN) - For MIDI Clock & LED Animation
    uint32_t lastClockTime = 0;             // Last time a MIDI clock pulse was sent (microseconds)
    uint8_t  currentClock = 0;              // Counter for clocks (0-23, wraps at 24)
    uint32_t usPerClock;                    // Microseconds per clock pulse (24 PPQN)

    // Playback state per track
    struct TrackPlayback {
        bool playing = false;                     // Is this track playing
        SequencePosition position;                // Current playback position
        uint8_t nextClip = 255;                   // Next clip to play (255 = none)
        uint32_t lastEventTime = 0;               // Last event time (for animation)
        map<uint8_t, uint32_t> noteOffMap;        // note -> tick time (for overwrite lookup)
        multimap<uint32_t, uint8_t> noteOffQueue; // tick -> note (for efficient processing)
        struct RecordedNote
        {
            uint32_t startPulse = 0;
            SequenceEvent* eventPtr = nullptr;
        };
        std::unordered_map<uint8_t, RecordedNote> recordedNotes; // note -> pending event info
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
    void PlayClipForAllTracks(uint8_t clip);
    bool Playing();
    bool Playing(uint8_t track);

    void Stop();
    void Stop(uint8_t track);
    void StopAfter(uint8_t track);

    void EnableRecord(bool val);
    bool RecordEnabled();

    void EnableClockOutput(bool val);
    bool ClockOutputEnabled();
    int16_t GetClocksTillStart();
    uint8_t GetBeatsPerBar() const { return data.beatsPerBar; }
    uint8_t GetBeatUnit() const { return data.beatUnit; }

    uint8_t GetTrackCount();

    // Clip management
    uint8_t GetClipCount(uint8_t track);
    bool ClipExists(uint8_t track, uint8_t clip);
    bool NewClip(uint8_t track, uint8_t clipId);
    void DeleteClip(uint8_t track, uint8_t clip);
    void CopyClip(uint8_t sourceTrack, uint8_t sourceClip, uint8_t destTrack, uint8_t destClip);

    // Pattern management (now with clip parameter)
    uint8_t GetPatternCount(uint8_t track, uint8_t clip);
    SequencePattern* GetPattern(uint8_t track, uint8_t clip, uint8_t pattern);
    int8_t NewPattern(uint8_t track, uint8_t clip, uint8_t length = 0); // if length is 0. Then use data.patternLength
    void ClearAllStepsInClip(uint8_t track, uint8_t clip);
    void CopyPattern(uint8_t sourceTrack, uint8_t sourceClip, uint8_t sourcePattern, uint8_t destTrack, uint8_t destClip, uint8_t destPattern = 255); // if destPattern is 255, then will create new pattern
    void DeletePattern(uint8_t track, uint8_t clip, uint8_t pattern);
    
    // Pattern helpers routed through Sequence
    void PatternClearAll(SequencePattern* pattern);
    void PatternAddEvent(SequencePattern* pattern, uint16_t timestamp, const SequenceEvent& event);
    bool PatternHasEventInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime, SequenceEventType type = SequenceEventType::Invalid);
    bool PatternClearNotesInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime, uint8_t note);
    void PatternClearEventsInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime);
    void PatternSetLength(SequencePattern* pattern, uint8_t steps);
    void PatternClearStepEvents(SequencePattern* pattern, uint8_t step, uint16_t pulsesPerStep);
    void PatternCopyStepEvents(SequencePattern* pattern, uint8_t src, uint8_t dest, uint16_t pulsesPerStep);
    void PatternCopyEventsInRange(SequencePattern* pattern, uint16_t sourceStart, uint16_t destStart, uint16_t length);
    void PatternNudge(SequencePattern* pattern, int16_t offsetPulse);

    uint8_t GetChannel(uint8_t track);
    void SetChannel(uint8_t track, uint8_t channel);

    uint16_t GetBPM();
    void SetBPM(uint16_t bpm);

    uint8_t GetSwing();
    void SetSwing(uint8_t swing);

    uint8_t GetStepDivision();
    void SetStepDivision(uint8_t stepLen);
    uint16_t GetPulsesPerStep() const { return pulsesPerStep; }

    uint8_t GetPatternLength();
    void SetPatternLength(uint8_t patternLength);
    void UpdateEmptyPatternsWithPatternLength();
    void SetTimeSignature(uint8_t beatsPerBar, uint8_t beatUnit);

    bool GetDirty();
    void SetDirty(bool val = true);

    bool GetSolo(uint8_t track);
    void SetSolo(uint8_t track, bool val);

    bool GetMute(uint8_t track);
    void SetMute(uint8_t track, bool val);

    bool ShouldRecord(uint8_t track); // Factors in solo & mute
    bool GetRecord(uint8_t track);
    void SetRecord(uint8_t track, bool val);

    bool GetEnabled(uint8_t track); // Disabled if mute or other track have solo

    void UndoLastRecorded();

    bool IsNoteActive(uint8_t track, uint8_t note) const;

    SequencePosition& GetPosition(uint8_t track);
    void SetPosition(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t step = 0);
    void SetClip(uint8_t track, uint8_t clip);
    void SetPattern(uint8_t track, uint8_t pattern);

    uint8_t GetNextClip(uint8_t track);
    void SetNextClip(uint8_t track, uint8_t clip);

    uint32_t GetLastEventTime(uint8_t track);

    Fract16 GetStepProgress(); // Swing Applied
    uint8_t StepProgressBreath(uint8_t lowBound = 0);  // LED Helper

    Fract16 GetQuarterNoteProgress();
    uint8_t QuarterNoteProgressBreath(uint8_t lowBound = 0); // LED Helper

    void RecordEvent(MidiPacket packet, uint8_t track = 0xFF); // if track is 0xff, will determain based on the packet channel. 

    // Data accessors (for serialization)
    const SequenceData& GetData() const { return data; }
    void SetData(const SequenceData& newData) { data = newData; UpdateEmptyPatternsWithPatternLength(); UpdateTiming(); lastRecordLayer = 0; currentRecordLayer = 0; dirty = true; }

private:
    void TerminateRecordedNotes(uint8_t track);
};
