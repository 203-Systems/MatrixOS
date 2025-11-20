#include "Sequence.h"
#include "Scales.h"

Sequence::Sequence(uint8_t tracks)
{
    New(tracks);
}

void Sequence::New(uint8_t tracks)
{
    if(tracks > 32)
    {
        // Don't support 32+ tracks
        MatrixOS::SYS::ErrorHandler("Too Many Tracks");
    }

    // Initialize sequence data
    data.bpm = 120;
    data.swing = 50;
    data.version = SEQUENCE_VERSION;
    data.tracks.clear();
    data.tracks.reserve(tracks);

    for (uint8_t i = 0; i < tracks; i++) {
        // Create data track
        data.tracks.emplace_back();
        data.tracks[i].channel = i;

        SequencePattern pattern;
        pattern.quarterNotes = 16;
        data.tracks[i].patterns.push_back(pattern);
    }

    position.clear();
    position.reserve(tracks);
    for (uint8_t i = 0; i < tracks; i++) {
        position.emplace_back();
        position[i].pattern = 0;
        position[i].quarterNote = 0;
    }

    data.solo = 0;
    data.mute = 0;
    data.record = 0xFFFFFFFF;

    UpdateTiming();
}

void Sequence::Tick()
{
    // TODO: Implement tick logic for sequencer playback
}

void Sequence::UpdateTiming()
{
    uint32_t pulseUs = 60000000UL / (data.bpm * ppqn);

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (data.swing - 50) / 100.0f;

    // Swing modifies the timing between note triggers (not gate length)
    // On-beat gets longer by swing amount, off-beat gets shorter
    // This maintains total duration: ticksPerStep[0] + ticksPerStep[1] = 2 * baseTicksPerStep
    uint32_t swingUs = (uint32_t)(pulseUs * swingRatio);

    usPerTick[0] = pulseUs + swingUs;  // On-beat (longer with positive swing)
    usPerTick[1] = pulseUs - swingUs;  // Off-beat (shorter with positive swing)

    usPerQuarterNote[0] = usPerTick[0] * ppqn;
    usPerQuarterNote[1] = usPerTick[1] * ppqn;
}

// Playback control
void Sequence::Play()
{
    playing = true;
    currentPulse = 0;
    quarterNoteSinceStart = 0;
    for (uint8_t i = 0; i < position.size(); i++) {
        position[i].pattern = 0;
        position[i].quarterNote = 0;
    }
}

bool Sequence::Playing()
{
    return playing;
}

void Sequence::Stop()
{
    playing = false;
}

// Recording
void Sequence::EnableRecord(bool val)
{
    record = val;
}

bool Sequence::RecordEnabled()
{
    return record;
}

// Track count
uint8_t Sequence::GetTrackCount()
{
    return data.tracks.size();
}

// Pattern access
SequencePattern& Sequence::GetPattern(uint8_t track, uint8_t pattern)
{
    return data.tracks[track].patterns[pattern];
}

// Track channel
uint8_t Sequence::GetChannel(uint8_t track)
{
    return data.tracks[track].channel;
}

void Sequence::SetChannel(uint8_t track, uint8_t channel)
{
    if(data.tracks[track].channel != channel)
    {
        data.tracks[track].channel = channel;
        dirty = true;
    }
}

// BPM
uint16_t Sequence::GetBPM()
{
    return data.bpm;
}

void Sequence::SetBPM(uint16_t bpm)
{
    if(bpm != data.bpm)
    {
        data.bpm = bpm;
        UpdateTiming();
        dirty = true;
    }
}

// Swing
uint8_t Sequence::GetSwing()
{
    return data.swing;
}

void Sequence::SetSwing(uint8_t swing)
{
    if(swing != data.swing)
    {
        data.swing = swing;
        UpdateTiming();
        dirty = true;
    }
}

// Dirty flag
bool Sequence::GetDirty()
{
    return dirty;
}

void Sequence::SetDirty(bool val)
{
    dirty = val;
}

// Solo/Mute/Record bitmap operations
bool Sequence::GetSolo(uint8_t track)
{
    return (data.solo >> track) & 1;
}

void Sequence::SetSolo(uint8_t track, bool val)
{
    uint32_t mask = 1 << track;
    bool currentVal = (data.solo >> track) & 1;

    if(currentVal != val)
    {
        if(val)
            data.solo |= mask;
        else
            data.solo &= ~mask;
        dirty = true;
    }
}

bool Sequence::GetMute(uint8_t track)
{
    return (data.mute >> track) & 1;
}

void Sequence::SetMute(uint8_t track, bool val)
{
    uint32_t mask = 1 << track;
    bool currentVal = (data.mute >> track) & 1;

    if(currentVal != val)
    {
        if(val)
            data.mute |= mask;
        else
            data.mute &= ~mask;
        dirty = true;
    }
}

bool Sequence::GetRecord(uint8_t track)
{
    return (data.record >> track) & 1;
}

void Sequence::SetRecord(uint8_t track, bool val)
{
    uint32_t mask = 1 << track;
    bool currentVal = (data.record >> track) & 1;

    if(currentVal != val)
    {
        if(val)
            data.record |= mask;
        else
            data.record &= ~mask;
        dirty = true;
    }
}

bool Sequence::GetEnabled(uint8_t track)
{
    // Track is disabled if:
    // 1. Track is muted
    // 2. Other tracks have solo (and this track doesn't have solo)
    if(GetMute(track))
        return false;

    if(data.solo != 0 && !GetSolo(track))
        return false;

    return true;
}

SequencePosition& Sequence::GetPosition(uint8_t track)
{
    return position[track];
}

// Current pulse
uint16_t Sequence::getCurrentPulse()
{
    return currentPulse;
}

void Sequence::RecordEvent(MidiPacket packet)
{

}