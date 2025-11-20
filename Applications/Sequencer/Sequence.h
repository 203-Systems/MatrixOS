#pragma once

#include "SequenceData.h"
#include "SequenceMeta.h"

struct SequencePosition
{
    uint8_t pattern;
    uint8_t quarterNote;
};

class Sequence
{
private:
    SequenceData data;

    bool dirty = false;

    bool playing = false;
    bool record = false;


    uint16_t currentPulse;
    uint16_t quarterNoteSinceStart;

    uint32_t usPerTick[2];
    uint32_t usPerQuarterNote[2];

    vector<SequencePosition> position;

    vector<uint32_t> lastEvent;
    vector<unordered_set<uint8_t>> activeNotes;

    void UpdateTiming();
public:
    const static uint16_t PPQN = 96;

    Sequence(uint8_t tracks = 8);
    void New(uint8_t tracks = 8);

    void Tick();

    void Play();
    bool Playing();

    void Stop();

    void EnableRecord(bool val);
    bool RecordEnabled();

    uint8_t GetTrackCount();

    uint8_t GetPatternCount(uint8_t track);

    SequencePattern& GetPattern(uint8_t track, uint8_t pattern);
    int8_t NewPattern(uint8_t track, uint8_t quarterNotes = 16);
    void ClearPattern(uint8_t track, uint8_t pattern);
    void DeletePattern(uint8_t track, uint8_t pattern);
    void CopyPattern(uint8_t sourceTrack, uint8_t sourcePattern, uint8_t destTrack, uint8_t destPattern = 255); // if destPattern is 255, then will create new pattern

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

    uint16_t getCurrentPulse();

    void RecordEvent(MidiPacket packet);
};
