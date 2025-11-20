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

        // Create default clip 0 with one pattern
        data.tracks[i].clips[0] = SequenceClip();
        data.tracks[i].clips[0].enabled = true;
        data.tracks[i].clips[0].patterns.emplace_back();
        data.tracks[i].clips[0].patterns[0].quarterNotes = 16;
    }

    position.clear();
    position.reserve(tracks);
    for (uint8_t i = 0; i < tracks; i++) {
        position.emplace_back();
        position[i].clip = 0;
        position[i].pattern = 0;
        position[i].quarterNote = 0;
    }

    data.solo = 0;
    data.mute = 0;
    data.record = 0xFFFFFFFF;

    lastEvent.resize(tracks, 0);
    activeNotes.resize(tracks);

    UpdateTiming();
}

void Sequence::Tick()
{
    // TODO: Implement tick logic for sequencer playback
}

void Sequence::UpdateTiming()
{
    uint32_t pulseUs = 60000000UL / (data.bpm * PPQN);

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (data.swing - 50) / 100.0f;

    // Swing modifies the timing between note triggers (not gate length)
    // On-beat gets longer by swing amount, off-beat gets shorter
    // This maintains total duration: ticksPerStep[0] + ticksPerStep[1] = 2 * baseTicksPerStep
    uint32_t swingUs = (uint32_t)(pulseUs * swingRatio);

    usPerTick[0] = pulseUs + swingUs;  // On-beat (longer with positive swing)
    usPerTick[1] = pulseUs - swingUs;  // Off-beat (shorter with positive swing)

    usPerQuarterNote[0] = usPerTick[0] * PPQN;
    usPerQuarterNote[1] = usPerTick[1] * PPQN;
}

// Playback control
void Sequence::Play()
{
    playing = true;
    currentPulse = 0;
    quarterNoteSinceStart = 0;
    for (uint8_t i = 0; i < position.size(); i++) {
        position[i].clip = 0;
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

// Clip management
uint8_t Sequence::GetClipCount(uint8_t track)
{
    return data.tracks[track].clips.size();
}

bool Sequence::ClipExists(uint8_t track, uint8_t clip)
{
    return data.tracks[track].clips.find(clip) != data.tracks[track].clips.end();
}

bool Sequence::GetClipEnabled(uint8_t track, uint8_t clip)
{
    if (!ClipExists(track, clip)) return false;
    return data.tracks[track].clips[clip].enabled;
}

void Sequence::SetClipEnabled(uint8_t track, uint8_t clip, bool enabled)
{
    if (!ClipExists(track, clip)) return;
    if(data.tracks[track].clips[clip].enabled != enabled)
    {
        data.tracks[track].clips[clip].enabled = enabled;
        dirty = true;
    }
}

bool Sequence::NewClip(uint8_t track, uint8_t clipId)
{
    if (ClipExists(track, clipId)) return false;

    data.tracks[track].clips[clipId] = SequenceClip();
    data.tracks[track].clips[clipId].enabled = true;
    data.tracks[track].clips[clipId].patterns.emplace_back();
    data.tracks[track].clips[clipId].patterns[0].quarterNotes = 16;
    dirty = true;
    return true;
}

void Sequence::DeleteClip(uint8_t track, uint8_t clip)
{
    if (!ClipExists(track, clip)) return;
    data.tracks[track].clips.erase(clip);

    // Update position if pointing to deleted clip
    if(position[track].clip == clip)
    {
        // Find first available clip or create clip 0
        if(data.tracks[track].clips.empty())
        {
            NewClip(track, 0);
        }
        position[track].clip = data.tracks[track].clips.begin()->first;
        position[track].pattern = 0;
        position[track].quarterNote = 0;
    }
    dirty = true;
}

void Sequence::CopyClip(uint8_t sourceTrack, uint8_t sourceClip, uint8_t destTrack, uint8_t destClip)
{
    // Check if source clip exists
    if (!ClipExists(sourceTrack, sourceClip)) return;

    // If destination clip exists, delete it first
    if (ClipExists(destTrack, destClip))
    {
        data.tracks[destTrack].clips.erase(destClip);
    }

    // Create new clip at destination
    data.tracks[destTrack].clips[destClip] = SequenceClip();

    // Copy enabled state
    data.tracks[destTrack].clips[destClip].enabled = data.tracks[sourceTrack].clips[sourceClip].enabled;

    // Copy all patterns
    auto& sourcePatterns = data.tracks[sourceTrack].clips[sourceClip].patterns;
    auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;

    destPatterns.clear();
    for (const auto& pattern : sourcePatterns)
    {
        destPatterns.push_back(pattern);
    }

    dirty = true;
}

// Pattern management (now with clip parameter)
uint8_t Sequence::GetPatternCount(uint8_t track, uint8_t clip)
{
    if (!ClipExists(track, clip)) return 0;
    return data.tracks[track].clips[clip].patterns.size();
}

SequencePattern& Sequence::GetPattern(uint8_t track, uint8_t clip, uint8_t pattern)
{
    return data.tracks[track].clips[clip].patterns[pattern];
}

int8_t Sequence::NewPattern(uint8_t track, uint8_t clip, uint8_t quarterNotes)
{
    if (!ClipExists(track, clip)) return -1;
    if(data.tracks[track].clips[clip].patterns.size() >= SEQUENCE_MAX_PATTERN_COUNT) {return -1;}

    data.tracks[track].clips[clip].patterns.emplace_back();
    data.tracks[track].clips[clip].patterns.back().quarterNotes = quarterNotes;
    dirty = true;
    return data.tracks[track].clips[clip].patterns.size() - 1;
}

void Sequence::ClearPattern(uint8_t track, uint8_t clip, uint8_t pattern)
{
    if (!ClipExists(track, clip)) return;
    if(pattern >= data.tracks[track].clips[clip].patterns.size()) return;
    data.tracks[track].clips[clip].patterns[pattern].events.clear();
    dirty = true;
}

void Sequence::DeletePattern(uint8_t track, uint8_t clip, uint8_t pattern)
{
    if (!ClipExists(track, clip)) return;

    auto& patterns = data.tracks[track].clips[clip].patterns;
    if(pattern >= patterns.size()) return;

    if(patterns.size() == 1)
    {
        // Clear instead of delete (keep at least 1 pattern)
        patterns[0].Clear();
    }
    else
    {
        patterns.erase(patterns.begin() + pattern);

        // Update position if pointing to this clip and pattern
        if(position[track].clip == clip && position[track].pattern >= pattern)
        {
            if(position[track].pattern > 0)
            {
                position[track].pattern--;
            }
        }
    }
    dirty = true;
}

void Sequence::CopyPattern(uint8_t sourceTrack, uint8_t sourceClip, uint8_t sourcePattern, uint8_t destTrack, uint8_t destClip, uint8_t destPattern)
{
    if (!ClipExists(sourceTrack, sourceClip) || !ClipExists(destTrack, destClip)) return;

    auto& sourcePatterns = data.tracks[sourceTrack].clips[sourceClip].patterns;
    if(sourcePattern >= sourcePatterns.size()) return;

    SequencePattern& source = sourcePatterns[sourcePattern];

    if(destPattern == 255)
    {
        // Create new pattern
        auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;
        if(destPatterns.size() >= SEQUENCE_MAX_PATTERN_COUNT) {return;}
        destPatterns.emplace_back();
        SequencePattern& dest = destPatterns.back();
        dest.quarterNotes = source.quarterNotes;
        dest.events = source.events;
    }
    else
    {
        // Overwrite existing pattern
        auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;
        if(destPattern >= destPatterns.size()) return;
        SequencePattern& dest = destPatterns[destPattern];
        dest.quarterNotes = source.quarterNotes;
        dest.events = source.events;
    }
    dirty = true;
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