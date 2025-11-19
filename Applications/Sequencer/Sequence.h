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
    
    const static uint16_t ppqn = 96;

    uint16_t currentPulse;
    uint16_t quarterNoteSinceStart;

    uint32_t usPerTick[2];
    uint32_t usPerQuarterNote[2];

    vector<SequencePosition> position;
    vector<uint32_t> lastEvent;

    void UpdateTiming();
public:
    Sequence(uint8_t tracks = 8);

    void Tick();

    void Play();
    bool Playing();

    void Stop();

    void EnableRecord(bool val);
    bool RecordEnabled();

    SequencePattern& GetPattern(uint8_t track, uint8_t pattern);

    uint8_t GetTrackCount();

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
