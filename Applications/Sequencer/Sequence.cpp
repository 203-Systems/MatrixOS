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

    trackPlayback.clear();
    trackPlayback.resize(tracks);
    pulseCounter = 0;
    lastPulseTime = 0;

    data.solo = 0;
    data.mute = 0;
    data.record = 0xFFFFFFFF;

    UpdateTiming();
}

void Sequence::Tick()
{
    if (!playing) return;

    uint32_t currentTime = MatrixOS::SYS::Micros();
    uint32_t elapsed = currentTime - lastPulseTime;

    // Check if enough time for a tick (using swing timing)
    if (elapsed >= usPerPulse[currentPulse % 2]) {
        lastPulseTime = currentTime;

        // Send MIDI clock pulse if enabled (24 PPQN, unswung)
        if (clockOutput && (currentTime - lastMidiClockTime) >= usPerMidiClockPulse) {
            MatrixOS::MIDI::Send(MidiPacket(0xF8), MIDI_PORT_ALL);  // MIDI Clock message
            lastMidiClockTime = currentTime;
        }

        // Update quarter note timestamp for LED animation (at quarter note boundaries)
        if (pulseCounter % PPQN == 0) {
            lastQuarterNoteTime = currentTime;
        }

        // Process each track independently BEFORE incrementing tick counter
        // This ensures events at tick 0 (quarter note 0) fire immediately on first tick
        for (uint8_t track = 0; track < trackPlayback.size(); track++) {
            ProcessTrack(track);
        }

        currentPulse++;
        pulseCounter++;
    }
}

void Sequence::UpdateTiming()
{
    uint32_t pulseUs = 60000000UL / (data.bpm * PPQN);

    // Calculate unswung timing for LED animation
    usPerQuarterNoteUnswung = pulseUs * PPQN;

    // Calculate MIDI clock timing (24 PPQN standard)
    usPerMidiClockPulse = 60000000UL / (data.bpm * 24);

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (data.swing - 50) / 100.0f;

    // Swing modifies the timing between note triggers (not gate length)
    // On-beat gets longer by swing amount, off-beat gets shorter
    // This maintains total duration: ticksPerStep[0] + ticksPerStep[1] = 2 * baseTicksPerStep
    uint32_t swingUs = (uint32_t)(pulseUs * swingRatio);

    usPerPulse[0] = pulseUs + swingUs;  // On-beat (longer with positive swing)
    usPerPulse[1] = pulseUs - swingUs;  // Off-beat (shorter with positive swing)

    usPerQuarterNote[0] = usPerPulse[0] * PPQN;
    usPerQuarterNote[1] = usPerPulse[1] * PPQN;
}

// Playback control
void Sequence::Play()
{
    playing = true;
    currentPulse = 0;
    uint32_t currentTime = MatrixOS::SYS::Micros();
    lastPulseTime = currentTime;
    lastQuarterNoteTime = currentTime;
    pulseCounter = 0;

    for (uint8_t i = 0; i < trackPlayback.size(); i++) {
        // Play all tracks at their current clip position
        trackPlayback[i].position.pattern = 0;
        trackPlayback[i].position.quarterNote = 0;
        trackPlayback[i].position.pulse = 0;

        // Clear playback state and start all tracks
        trackPlayback[i].noteOffMap.clear();
        trackPlayback[i].noteOffQueue.clear();
        trackPlayback[i].playing = true;
    }
}

void Sequence::Play(uint8_t track)
{
    // Initialize timing if not already playing
    if (!playing) {
        playing = true;
        lastPulseTime = MatrixOS::SYS::Micros();
        pulseCounter = 0;
    }

    // Play the track at its current clip position
    trackPlayback[track].position.pattern = 0;
    trackPlayback[track].position.quarterNote = 0;
    trackPlayback[track].position.pulse = 0;

    // Clear playback state and start track
    trackPlayback[track].noteOffMap.clear();
    trackPlayback[track].noteOffQueue.clear();
    trackPlayback[track].playing = true;
}

void Sequence::PlayClip(uint8_t track, uint8_t clip)
{
    // Initialize timing if not already playing
    if (!playing) {
        playing = true;
        lastPulseTime = MatrixOS::SYS::Micros();
        pulseCounter = 0;
    }

    // Queue this clip to play after current patterns finish
    trackPlayback[track].nextClip = clip;
}

bool Sequence::Playing()
{
    return playing;
}

bool Sequence::Playing(uint8_t track)
{
    return trackPlayback[track].playing;
}

void Sequence::Stop()
{
    playing = false;

    // Send note-off for all queued notes before clearing
    for (uint8_t track = 0; track < trackPlayback.size(); track++) {
        uint8_t channel = GetChannel(track);
        for (const auto& [note, tick] : trackPlayback[track].noteOffMap) {
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
        }
        trackPlayback[track].noteOffMap.clear();
        trackPlayback[track].noteOffQueue.clear();
        trackPlayback[track].nextClip = 255;
        trackPlayback[track].playing = false;
    }
}

void Sequence::Stop(uint8_t track)
{
    // Send note-off for all queued notes on this track before clearing
    uint8_t channel = GetChannel(track);
    for (const auto& [note, tick] : trackPlayback[track].noteOffMap) {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
    }
    trackPlayback[track].noteOffMap.clear();
    trackPlayback[track].noteOffQueue.clear();
    trackPlayback[track].nextClip = 255;
    trackPlayback[track].playing = false;

    // If no tracks are playing, stop global playback
    bool anyPlaying = false;
    for (uint8_t i = 0; i < trackPlayback.size(); i++) {
        if (trackPlayback[i].playing) {
            anyPlaying = true;
            break;
        }
    }
    if (!anyPlaying) {
        playing = false;
    }
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

// Clock Output
void Sequence::EnableClockOutput(bool val)
{
    clockOutput = val;
}

bool Sequence::ClockOutputEnabled()
{
    return clockOutput;
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
    if(trackPlayback[track].position.clip == clip)
    {
        // Find lowest available clip or create clip 0
        if(data.tracks[track].clips.empty())
        {
            NewClip(track, 0);
            trackPlayback[track].position.clip = 0;
        }
        else
        {
            // Find the lowest clip index
            uint8_t lowestClip = 255;
            for(const auto& [clipIdx, clipData] : data.tracks[track].clips)
            {
                if(clipIdx < lowestClip)
                {
                    lowestClip = clipIdx;
                }
            }
            trackPlayback[track].position.clip = lowestClip;
        }
        trackPlayback[track].position.pattern = 0;
        trackPlayback[track].position.quarterNote = 0;
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
        if(trackPlayback[track].position.clip == clip && trackPlayback[track].position.pattern >= pattern)
        {
            if(trackPlayback[track].position.pattern > 0)
            {
                trackPlayback[track].position.pattern--;
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
    return trackPlayback[track].position;
}

void Sequence::SetPosition(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t quarterNote)
{
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.quarterNote = quarterNote;
}

void Sequence::SetClip(uint8_t track, uint8_t clip)
{
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = 0;
    trackPlayback[track].position.quarterNote = 0;
}

void Sequence::SetPattern(uint8_t track, uint8_t pattern)
{
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.quarterNote = 0;
}

uint8_t Sequence::GetNextClip(uint8_t track)
{
    return trackPlayback[track].nextClip;
}

void Sequence::SetNextClip(uint8_t track, uint8_t clip)
{
    trackPlayback[track].nextClip = clip;
}

// Current pulse
uint16_t Sequence::getCurrentPulse()
{
    return currentPulse;
}

void Sequence::RecordEvent(MidiPacket packet)
{

}

void Sequence::ProcessTrack(uint8_t track)
{
    // Only process tracks that are currently playing
    if (!trackPlayback[track].playing) return;

    bool trackEnabled = GetEnabled(track);

    // 1. Process note-offs that have reached their time
    //    Only check the earliest entries (efficient with multimap)
    while (!trackPlayback[track].noteOffQueue.empty() &&
           trackPlayback[track].noteOffQueue.begin()->first <= pulseCounter) {

        uint8_t note = trackPlayback[track].noteOffQueue.begin()->second;

        if (trackEnabled) {
            uint8_t channel = GetChannel(track);
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
        }

        // Remove from both structures
        trackPlayback[track].noteOffMap.erase(note);
        trackPlayback[track].noteOffQueue.erase(trackPlayback[track].noteOffQueue.begin());
    }

    // 2. Fire events at current tick position
    uint8_t clip = trackPlayback[track].position.clip;
    uint8_t pattern = trackPlayback[track].position.pattern;

    if (trackEnabled && ClipExists(track, clip)) {
        SequencePattern& currentPattern = GetPattern(track, clip, pattern);

        // Calculate the absolute tick position within the pattern
        uint16_t currentTick = trackPlayback[track].position.quarterNote * PPQN + trackPlayback[track].position.pulse;

        // Fire all events at exactly this tick
        auto eventIt = currentPattern.events.find(currentTick);
        if (eventIt != currentPattern.events.end()) {
            // Execute event (sends MIDI)
            eventIt->second.ExecuteEvent(data, track);

            // If it's a note event, schedule note-off with overwrite and update lastEvent
            if (eventIt->second.eventType == SequenceEventType::NoteEvent) {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(eventIt->second.data);
                uint8_t note = noteData.note;
                uint32_t noteOffTick = pulseCounter + noteData.length;

                // Update lastEvent timestamp for recording purposes
                trackPlayback[track].lastEvent = MatrixOS::SYS::Millis();

                // Check if this note already has a pending note-off (overwrite case)
                if (trackPlayback[track].noteOffMap.count(note)) {
                    // Remove old note-off from queue
                    uint32_t oldTick = trackPlayback[track].noteOffMap[note];

                    // Find and remove from multimap
                    auto range = trackPlayback[track].noteOffQueue.equal_range(oldTick);
                    for (auto it = range.first; it != range.second; ++it) {
                        if (it->second == note) {
                            trackPlayback[track].noteOffQueue.erase(it);
                            break;
                        }
                    }
                }

                // Add new note-off
                trackPlayback[track].noteOffMap[note] = noteOffTick;
                trackPlayback[track].noteOffQueue.insert({noteOffTick, note});
            }
        }
    }

    // 3. Advance pulse position
    trackPlayback[track].position.pulse++;

    // 4. Check if we've completed a quarter note
    if (trackPlayback[track].position.pulse >= PPQN) {
        trackPlayback[track].position.pulse = 0;
        trackPlayback[track].position.quarterNote++;

        // Check if pattern ended
        if (ClipExists(track, clip)) {
            SequencePattern& currentPattern = GetPattern(track, clip, pattern);

            if (trackPlayback[track].position.quarterNote >= currentPattern.quarterNotes) {
                trackPlayback[track].position.quarterNote = 0;
                trackPlayback[track].position.pulse = 0;
                trackPlayback[track].position.pattern++;

                // Check if all patterns in clip ended
                if (trackPlayback[track].position.pattern >= GetPatternCount(track, clip)) {
                    // Check if there's a nextClip queued
                    if (trackPlayback[track].nextClip != 255) {
                        // Transition to the queued clip
                        trackPlayback[track].position.clip = trackPlayback[track].nextClip;
                        trackPlayback[track].position.pattern = 0;
                        trackPlayback[track].position.quarterNote = 0;
                        trackPlayback[track].position.pulse = 0;
                        // Clear nextClip after transitioning
                        trackPlayback[track].nextClip = 255;
                    } else {
                        // No nextClip, just loop the patterns
                        trackPlayback[track].position.pattern = 0;
                        trackPlayback[track].position.quarterNote = 0;
                        trackPlayback[track].position.pulse = 0;
                    }
                }
            }
        }
    }
}