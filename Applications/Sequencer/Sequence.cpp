#include "Sequence.h"
#include "Scales.h"
#include <cmath>

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
    data.patternLength = 16;
    data.version = SEQUENCE_VERSION;
    data.tracks.clear();
    data.tracks.reserve(tracks);

    for (uint8_t i = 0; i < tracks; i++) {
        // Create data track
        data.tracks.emplace_back();
        data.tracks[i].channel = i;
        data.tracks[i].activeClip = i;

        // Create default clip 0 with one pattern
        data.tracks[i].clips[0] = SequenceClip();
        data.tracks[i].clips[0].enabled = true;
        data.tracks[i].clips[0].patterns.emplace_back();
        data.tracks[i].clips[0].patterns[0].steps = 16;
    }

    trackPlayback.clear();
    trackPlayback.resize(tracks);
    for (auto& tp : trackPlayback) {
        tp.position.pulse = UINT16_MAX;
        tp.position.step = 0;
        tp.position.pattern = 0;
        tp.position.clip = 0;
    }
    pulseSinceStart = 0;
    currentPulse = UINT16_MAX; // so first increment lands on pulse 0
    currentStep = 0;
    lastPulseTime = 0;

    data.solo = 0;
    data.mute = 0;
    data.record = 0xFFFFFFFF;

    dirty = true;

    UpdateTiming();
}

void Sequence::Tick()
{
    uint32_t now = MatrixOS::SYS::Micros();

    // Anchor clock on first tick
    if (lastClockTime == 0) {
        lastClockTime = now;
    }

    // Single loop: advance clock and pulses until nothing is due
    while (true) {
        now = MatrixOS::SYS::Micros();
        bool progressed = false;

        // Handle MIDI clock output and start countdown
        if ((now - lastClockTime) >= usPerClock) {
            if (clockOutput) {
                MatrixOS::MIDI::Send(MidiPacket::Clock(), MIDI_PORT_ALL);  // MIDI Clock message
            }

            lastClockTime += usPerClock; // advance by period to maintain phase

            // Update MIDI clock counter (24 PPQN)
            currentClock = (currentClock + 1) % 24;

            if (clocksTillStart > 0) {
                clocksTillStart--;
                if (clocksTillStart == 0) {
                    // Initialize timing at this clock edge
                    lastPulseTime = lastClockTime;
                    currentPulse = UINT16_MAX; // next advance produces pulse 0
                    currentStep = 0;
                    pulseSinceStart = 0;
                }
            }
            progressed = true;
        }

        if (!playing || clocksTillStart > 0) {
            break;
        }

        // Anchor pulse timer if uninitialized
        if (lastPulseTime == 0) {
            lastPulseTime = now;
            currentPulse = UINT16_MAX; // so first increment hits 0
        }

        // Handle pulse advance
        uint16_t period = usPerPulse[currentStep % 2];
        if ((now - lastPulseTime) >= period) {
            lastPulseTime += period; // advance by exact period

            if (currentPulse == UINT16_MAX) {
                currentPulse = 0; // first usable pulse
            } else {
                currentPulse++;
                pulseSinceStart++;
            }

            if (currentPulse >= pulsesPerStep) {
                currentPulse = 0;
                currentStep++;
            }

            if (currentStep >= barLength) {
                currentStep = 0;
            }

            for (uint8_t track = 0; track < trackPlayback.size(); track++) {
                ProcessTrack(track);
            }

            progressed = true;
        }

        if (!progressed) {
            break; // nothing due right now
        }
    }
}

void Sequence::UpdateTiming()
{
    // Update Step Division
    pulsesPerStep = (PPQN * 4) / stepDivision; // stepDivision: 4=quarter, 8=eighth, 16=sixteenth

    // Update barLength (in steps) from timeSignature
    barLength = data.beatsPerBar * (PPQN * 4 / data.beatUnit) / pulsesPerStep;
    
    // Base pulse timing stays tied to PPQN; pulsesPerStep only controls when we advance a step
    uint32_t pulseUs = 60000000UL / (data.bpm * PPQN);

    // Calculate clock timing (24 PPQN standard)
    usPerClock = 60000000UL / (data.bpm * 24);

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (data.swing - 50) / 100.0f;

    // Swing modifies the timing between note triggers (not gate length)
    // On-beat gets longer by swing amount, off-beat gets shorter
    // This maintains total duration: ticksPerStep[0] + ticksPerStep[1] = 2 * baseTicksPerStep
    uint32_t swingUs = (uint32_t)(pulseUs * swingRatio);

    usPerPulse[0] = pulseUs + swingUs;  // On-beat (longer with positive swing)
    usPerPulse[1] = pulseUs - swingUs;  // Off-beat (shorter with positive swing)

    MLOGD("Sequence",
          "Timing Updated - BPM:%u Swing:%u StepDiv:%u PatternLen:%u BarLen:%u Beats:%u/%u pulses/step:%u usPerPulse[%lu,%lu] usPerClock:%lu",
          data.bpm,
          data.swing,
          stepDivision,
          data.patternLength,
          barLength,
          data.beatsPerBar,
          data.beatUnit,
          pulsesPerStep,
          (unsigned long)usPerPulse[0],
          (unsigned long)usPerPulse[1],
          (unsigned long)usPerClock);
}

// Playback control
void Sequence::Play()
{
    playing = true;  // Set immediately for UI feedback

    // Schedule playback to start at next MIDI clock edge (24 PPQN)
    // Can be increased for count-in: 96 = 1 bar at 4/4
    clocksTillStart = record ? 24 * 4 + 1 : 1;
    currentPulse = UINT16_MAX;
    currentStep = 0;
    pulseSinceStart = 0;

    for (uint8_t i = 0; i < trackPlayback.size(); i++) {
        // Reset positions and prepare for playback
        trackPlayback[i].position.pattern = 0;
        trackPlayback[i].position.step = 0;
        trackPlayback[i].position.pulse = UINT16_MAX; // so first advance lands on 0

        // Clear playback state
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
        clocksTillStart = record ? 24 * 4 + 1 : 1;
        currentPulse = UINT16_MAX;
        currentStep = 0;
        pulseSinceStart = 0;
    }

    // Play the track at its current clip position
    trackPlayback[track].position.pattern = 0;
    trackPlayback[track].position.step = 0;
    trackPlayback[track].position.pulse = UINT16_MAX;

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
        clocksTillStart = record ? 24 * 4 + 1 : 1;
        currentPulse = UINT16_MAX;
        currentStep = 0;
        pulseSinceStart = 0;
    }

    // Terminate recorded notes on this track before switching
    TerminateRecordedNotes(track);

    // Queue this clip to play after current patterns finish
    trackPlayback[track].nextClip = clip;
}

void Sequence::PlayClipForAllTracks(uint8_t clip)
{
    // Initialize timing if not already playing
    if (!playing) {
        playing = true;
        clocksTillStart = record ? 24 * 4 + 1 : 1;
        currentPulse = UINT16_MAX;
        currentStep = 0;
        pulseSinceStart = 0;
    }

    // For each track, check if clip exists
    for (uint8_t track = 0; track < data.tracks.size(); track++)
    {
        TerminateRecordedNotes(track);
        if (ClipExists(track, clip))
        {
            // Track has this clip, queue it to play
            trackPlayback[track].nextClip = clip;
        }
        else if (trackPlayback[track].playing)
        {
            // Track doesn't have this clip but is playing, queue stop
            trackPlayback[track].nextClip = 254;
        }
        // If track is not playing and has no clip, do nothing (don't update nextClip)
    }
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
    record = false;

    // Send note-off for all queued notes before clearing
    for (uint8_t track = 0; track < trackPlayback.size(); track++) {
        uint8_t channel = GetChannel(track);
        TerminateRecordedNotes(track);
        for (const auto& [note, tick] : trackPlayback[track].noteOffMap) {
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
        }
        MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, 120, 0), MIDI_PORT_ALL); // All sound off per channel
    
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
    TerminateRecordedNotes(track);
    for (const auto& [note, tick] : trackPlayback[track].noteOffMap) {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
    }
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, 120, 0), MIDI_PORT_ALL); // All sound off

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

void Sequence::StopAfter(uint8_t track)
{
    // Only queue stop if track is currently playing
    if (trackPlayback[track].playing)
    {
        // Set nextClip to 254 to signal stop at next bar boundary
        trackPlayback[track].nextClip = 254;
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

int16_t Sequence::GetClocksTillStart()
{
    return clocksTillStart;
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
    data.tracks[track].clips[clipId].patterns[0].steps = 16;
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
        trackPlayback[track].position.step = 0;
    }
    dirty = true;
}

void Sequence::CopyClip(uint8_t sourceTrack, uint8_t sourceClip, uint8_t destTrack, uint8_t destClip)
{
    if (sourceTrack == destTrack && sourceClip == destClip)
    {
        return;
    }

    // Check if source clip exists
    if (!ClipExists(sourceTrack, sourceClip)) return;

    // If destination clip exists, delete it first
    if (ClipExists(destTrack, destClip))
    {
        data.tracks[destTrack].clips.erase(destClip);
    }

    // Copy entire clip
    data.tracks[destTrack].clips[destClip] = data.tracks[sourceTrack].clips[sourceClip];

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

int8_t Sequence::NewPattern(uint8_t track, uint8_t clip, uint8_t steps)
{
    if (!ClipExists(track, clip)) return -1;
    if(data.tracks[track].clips[clip].patterns.size() >= SEQUENCE_MAX_PATTERN_COUNT) {return -1;}

    // If length is 0, use patternLength as default
    uint8_t actualLength = (steps == 0) ? data.patternLength : steps;

    data.tracks[track].clips[clip].patterns.emplace_back();
    data.tracks[track].clips[clip].patterns.back().steps = actualLength;
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
    if (sourceTrack == destTrack && sourceClip == destClip && sourcePattern == destPattern)
    {
        return;
    }

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
        dest.steps = source.steps;
        dest.events = source.events;
    }
    else
    {
        // Overwrite existing pattern
        auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;
        if(destPattern >= destPatterns.size()) return;
        SequencePattern& dest = destPatterns[destPattern];
        dest.steps = source.steps;
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

// Step length (division)
uint8_t Sequence::GetStepDivision()
{
    return stepDivision;
}

void Sequence::SetStepDivision(uint8_t stepLen)
{
    if(stepLen == 0) { return; }
    if(stepLen != stepDivision)
    {
        stepDivision = stepLen;
        UpdateTiming();
        dirty = true;
    }
}

// Pattern Length
uint8_t Sequence::GetPatternLength()
{
    return data.patternLength;
}

void Sequence::SetPatternLength(uint8_t patternLength)
{
    if(patternLength != data.patternLength)
    {
        data.patternLength = patternLength;
        dirty = true;
    }
}

void Sequence::UpdateEmptyPatternsWithPatternLength()
{
    // Iterate through all tracks
    for (uint8_t track = 0; track < data.tracks.size(); track++)
    {
        // Iterate through all clips
        for (auto& [clipId, clip] : data.tracks[track].clips)
        {
            // Iterate through all patterns in this clip
            for (auto& pattern : clip.patterns)
            {
                // Only update patterns that are empty (no events)
                if (pattern.events.empty())
                {
                    pattern.steps = data.patternLength;
                }
            }
        }
    }
    dirty = true;
}

void Sequence::SetTimeSignature(uint8_t beatsPerBar, uint8_t beatUnit)
{
    if (beatsPerBar == 0 || beatUnit == 0)
    {
        return;
    }

    if(beatsPerBar > 16)
    {
        return;
    }

    if (!(beatUnit == 1 || beatUnit == 2 || beatUnit == 4 || beatUnit == 8 || beatUnit == 16))
    {
        return;
    }

    bool changed = false;
    if (data.beatsPerBar != beatsPerBar)
    {
        data.beatsPerBar = beatsPerBar;
        changed = true;
    }
    if (data.beatUnit != beatUnit)
    {
         data.beatUnit = beatUnit;
         changed = true;
    }

    if (changed)
    {
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

        // Send all-notes-off on this track's channel when solo state changes
        uint8_t channel = GetChannel(track);
        MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, 123, 0), MIDI_PORT_ALL);
        // Also send all-notes-off to other channels when soloing to avoid stuck notes
        if (val) {
            for (uint8_t i = 0; i < trackPlayback.size(); i++) {
                uint8_t ch = GetChannel(i);
                if (ch == channel) continue;
                MatrixOS::MIDI::Send(MidiPacket::ControlChange(ch, 123, 0), MIDI_PORT_ALL);
            }
        }
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

        // Send all-notes-off on this track's channel when mute state changes
        uint8_t channel = GetChannel(track);
        MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, 123, 0), MIDI_PORT_ALL);
        dirty = true;
    }
}

bool Sequence::ShouldRecord(uint8_t track)
{
    return GetEnabled(track) && GetRecord(track);
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

bool Sequence::IsNoteActive(uint8_t track, uint8_t note) const
{
    if (track >= trackPlayback.size()) return false;
    auto it = trackPlayback[track].noteOffMap.find(note);
    return it != trackPlayback[track].noteOffMap.end();
}

SequencePosition& Sequence::GetPosition(uint8_t track)
{
    return trackPlayback[track].position;
}

void Sequence::SetPosition(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t step)
{
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.step = step;
    // data.tracks[track].activeClip = clip;
    // dirty = true;
}

void Sequence::SetClip(uint8_t track, uint8_t clip)
{
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = 0;
    trackPlayback[track].position.step = 0;
    data.tracks[track].activeClip = clip;
    dirty = true;
}

void Sequence::SetPattern(uint8_t track, uint8_t pattern)
{
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.step = 0;
}

uint8_t Sequence::GetNextClip(uint8_t track)
{
    return trackPlayback[track].nextClip;
}

void Sequence::SetNextClip(uint8_t track, uint8_t clip)
{
    trackPlayback[track].nextClip = clip;
}

uint32_t Sequence::GetLastEventTime(uint8_t track)
{
    return trackPlayback[track].lastEventTime;
}

Fract16 Sequence::GetStepProgress()
{
    if (!playing) {
        return 0;
    }

    uint32_t currentTime = MatrixOS::SYS::Micros();
    uint32_t usPerCurrentPulse = usPerPulse[currentStep % 2];
    uint32_t timeElapsedSinceStep = (currentTime - lastPulseTime) + (currentPulse * usPerCurrentPulse);
    uint32_t usPerCurrentStep = usPerCurrentPulse * pulsesPerStep;

    uint64_t progress = ((uint64_t)timeElapsedSinceStep * UINT16_MAX) / usPerCurrentStep;
    // Clamp to UINT16_MAX in case we're past the step boundary
    if (progress > UINT16_MAX) {
        progress = UINT16_MAX;
    }
    return (Fract16)progress;
}

uint8_t Sequence::StepProgressBreath(uint8_t lowBound)
{
    // Get progress through step (0-65535) with swing
    Fract16 progress = GetStepProgress();

    // Convert to breathing effect using cosine wave
    // Map 0-65535 to 0-2π for full cosine cycle
    float phase = (float)progress * 2.0f * M_PI;
    float brightness = ((std::cos(phase - M_PI) + 1.0f) / 2.0f) * (255.0f - lowBound) + lowBound;

    return (uint8_t)brightness;
}

Fract16 Sequence::GetQuarterNoteProgress()
{

    uint32_t usPerQuarterNote = usPerClock * 24; // Clock is 24PPQN
    uint32_t timeElapsedSinceQuarterNote = (MatrixOS::SYS::Micros() - lastClockTime) + (usPerClock * currentClock);
    // Use 64-bit arithmetic to avoid overflow when multiplying
    uint64_t progress = ((uint64_t)timeElapsedSinceQuarterNote * UINT16_MAX) / usPerQuarterNote;
    // Clamp to UINT16_MAX in case we're past the quarter note boundary
    if (progress > UINT16_MAX) {
        progress = UINT16_MAX;
    }
    return (Fract16)progress;
}

uint8_t Sequence::QuarterNoteProgressBreath(uint8_t lowBound)
{
    // Get progress through quarter note (0-65535)
    Fract16 progress = GetQuarterNoteProgress();

    // Convert to breathing effect using cosine wave
    // Map 0-65535 to 0-2π for full cosine cycle
    float phase = (float)progress * 2.0f * M_PI;
    float brightness = ((std::cos(phase - M_PI) + 1.0f) / 2.0f) * (255.0f - lowBound) + lowBound;

    return (uint8_t)brightness;
}

void Sequence::RecordEvent(MidiPacket packet, uint8_t track)
{
    // if track is 0xff, determine based on the packet channel.
    if (!record) return;

    EMidiStatus status = packet.Status();
    if (status != EMidiStatus::NoteOn && status != EMidiStatus::NoteOff)
    {
        return;
    }

    std::vector<uint8_t> targets;
    if (track == 0xFF)
    {
        for (uint8_t i = 0; i < data.tracks.size(); ++i)
        {
            if (data.tracks[i].channel != packet.Channel()) continue;
            if (!GetEnabled(i)) continue;
            targets.push_back(i);
        }
        if (targets.empty()) return;
    }
    else
    {
        if (track >= data.tracks.size()) return;
        targets.push_back(track);
    }

    uint8_t note = packet.Note();
    uint8_t velocity = packet.Velocity();
    
    for (uint8_t t : targets)
    {
        if (!ClipExists(t, trackPlayback[t].position.clip)) continue;

        uint8_t clipIdx = trackPlayback[t].position.clip;
        uint8_t patIdx = trackPlayback[t].position.pattern;
        if (patIdx >= GetPatternCount(t, clipIdx)) continue;

        SequencePattern& pattern = GetPattern(t, clipIdx, patIdx);
        uint32_t currentTick = trackPlayback[t].position.step * pulsesPerStep + trackPlayback[t].position.pulse;
        auto& pending = trackPlayback[t].recordedNotes;

        if (status == EMidiStatus::NoteOn && velocity > 0)
        {
            // If a note is already pending, finalize its length to this tick
            auto prevIt = pending.find(note);
            if (prevIt != pending.end())
            {
                Sequence::TrackPlayback::RecordedNote prev = prevIt->second;
                pending.erase(prevIt);
                if (prev.eventPtr != nullptr)
                {
                    uint32_t prevLen = (pulseSinceStart > prev.startPulse) ? (pulseSinceStart - prev.startPulse) : 1;
                    if (prevLen == 0) prevLen = 1;
                    SequenceEventNote& prevNoteData = std::get<SequenceEventNote>(prev.eventPtr->data);
                    if (prevLen > UINT16_MAX) prevLen = UINT16_MAX;
                    prevNoteData.length = (uint16_t)prevLen;
                    dirty = true;
                }
            }

            auto evIt = pattern.events.insert({(uint16_t)currentTick, SequenceEvent::Note(note, velocity, false, 1)});
            SequenceEvent& evRef = evIt->second;
            Sequence::TrackPlayback::RecordedNote info;
            info.startPulse = pulseSinceStart;
            info.eventPtr = &evRef;
            pending[note] = info;
            dirty = true;
        }
        else if (status == EMidiStatus::NoteOff || (status == EMidiStatus::NoteOn && velocity == 0))
        {
            auto itPending = pending.find(note);
            if (itPending == pending.end()) continue;

            Sequence::TrackPlayback::RecordedNote info = itPending->second;
            pending.erase(itPending);

            if (info.eventPtr == nullptr) continue;

            uint32_t length = (pulseSinceStart > info.startPulse) ? (pulseSinceStart - info.startPulse) : 1;
            if (length > UINT16_MAX) length = UINT16_MAX;
            if (length == 0) length = 1;

            SequenceEventNote& noteData = std::get<SequenceEventNote>(info.eventPtr->data);
            noteData.length = (uint16_t)length;
            dirty = true;
        }
        else
        {
            // Ignore other statuses for now
        }
    }
}

void Sequence::TerminateRecordedNotes(uint8_t track)
{
    if (track >= trackPlayback.size()) return;
    auto& recordedNotes = trackPlayback[track].recordedNotes;
    if (recordedNotes.empty()) return;

    bool updated = false;
    // Use global pulse counter for a stop-time reference
    uint32_t currentPulseGlobal = pulseSinceStart;

    for (auto& entry : recordedNotes)
    {
        const auto& info = entry.second;
        if (info.eventPtr == nullptr) continue;

        uint32_t length = (currentPulseGlobal > info.startPulse) ? (currentPulseGlobal - info.startPulse) : 1;
        if (length == 0) length = 1;
        if (length > UINT16_MAX) length = UINT16_MAX;

        SequenceEventNote& noteData = std::get<SequenceEventNote>(info.eventPtr->data);
        noteData.length = (uint16_t)length;
        updated = true;
    }

    recordedNotes.clear();
    if (updated) dirty = true;
}

void Sequence::ProcessTrack(uint8_t track)
{
    bool trackEnabled = GetEnabled(track);

    SequencePosition& pos = trackPlayback[track].position;

    // Advance to the pulse being processed
    if (pos.pulse == UINT16_MAX) {
        pos.pulse = 0;
    }
    else {
        pos.pulse++;
        if (pos.pulse >= pulsesPerStep) {
            pos.pulse = 0;
            pos.step++;
        }
    }

    // Wrap step/pattern based on active pattern length
    {
        uint8_t clip = pos.clip;
        uint8_t pattern = pos.pattern;
        uint8_t patternSteps = data.patternLength;
        if (ClipExists(track, clip) && pattern < GetPatternCount(track, clip)) {
            patternSteps = GetPattern(track, clip, pattern).steps;
        }
        if (pos.step >= patternSteps) {
            pos.step = 0;
            pos.pattern++;
            if (ClipExists(track, clip) && pos.pattern >= GetPatternCount(track, clip)) {
                pos.pattern = 0;
            }
        }
    }

    // Check for queued clip change at bar boundary (step 0, pulse 0)
    if (trackPlayback[track].nextClip != 255 && pos.step == 0 && pos.pulse == 0) {
        if (trackPlayback[track].nextClip == 254) {
            Stop(track);
            return;
        }

        pos.clip = trackPlayback[track].nextClip;
        pos.pattern = 0;
        pos.step = 0;
        pos.pulse = 0;
        trackPlayback[track].nextClip = 255;
        trackPlayback[track].playing = true;
    }

    // Only process tracks that are currently playing
    if (!trackPlayback[track].playing) return;

    // 2. Fire events at current tick position
    uint8_t clip = pos.clip;
    uint8_t pattern = pos.pattern;
    if (trackEnabled && ClipExists(track, clip)) {
        SequencePattern& currentPattern = GetPattern(track, clip, pattern);

        // Calculate the absolute tick position within the pattern
        uint16_t currentTick = pos.step * pulsesPerStep + pos.pulse;

        // Fire all events at exactly this tick (there can be multiple at the same timestamp)
        auto range = currentPattern.events.equal_range(currentTick);
        for (auto eventIt = range.first; eventIt != range.second; ++eventIt) {
            const auto& ev = eventIt->second;
            switch (ev.eventType)
            {
                case SequenceEventType::NoteEvent:
                {
                    const SequenceEventNote& noteData = std::get<SequenceEventNote>(ev.data);
                    uint8_t channel = data.tracks[track].channel;
                    if (noteData.aftertouch)
                    {
                        MatrixOS::MIDI::Send(MidiPacket::AfterTouch(channel, noteData.note, noteData.velocity), MIDI_PORT_ALL);
                    }
                    else
                    {
                        MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity), MIDI_PORT_ALL);
                    }

                    uint8_t note = noteData.note;
                    uint32_t noteOffTick = pulseSinceStart + noteData.length;

                    // Update lastEventTime timestamp for led purposes
                    trackPlayback[track].lastEventTime = MatrixOS::SYS::Millis();

                    // Remove any pending note-off for this note (overwrite case)
                    auto mapIt = trackPlayback[track].noteOffMap.find(note);
                    if (mapIt != trackPlayback[track].noteOffMap.end()) {
                        uint32_t oldTick = mapIt->second;
                        for (auto qIt = trackPlayback[track].noteOffQueue.begin();
                             qIt != trackPlayback[track].noteOffQueue.end(); ) {
                            if (qIt->second == note) {
                                qIt = trackPlayback[track].noteOffQueue.erase(qIt);
                            } else {
                                ++qIt;
                            }
                        }
                        trackPlayback[track].noteOffMap.erase(mapIt);
                    }

                    // Add new note-off
                    trackPlayback[track].noteOffMap[note] = noteOffTick;
                    trackPlayback[track].noteOffQueue.insert({noteOffTick, note});
                    break;
                }
                case SequenceEventType::ControlChangeEvent:
                {
                    const SequenceEventCC& ccEvent = std::get<SequenceEventCC>(ev.data);
                    uint8_t channel = data.tracks[track].channel;
                    MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, ccEvent.param, ccEvent.value), MIDI_PORT_ALL);
                    break;
                }
                default:
                    break;
            }
        }
    }

    // 3. Process note-offs that have reached their time
    //    Only check the earliest entries (efficient with multimap)
    while (!trackPlayback[track].noteOffQueue.empty() &&
           trackPlayback[track].noteOffQueue.begin()->first <= pulseSinceStart) {

        uint8_t note = trackPlayback[track].noteOffQueue.begin()->second;

        if (trackEnabled) {
            uint8_t channel = GetChannel(track);
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
        }

        trackPlayback[track].noteOffMap.erase(note);
        trackPlayback[track].noteOffQueue.erase(trackPlayback[track].noteOffQueue.begin());
    }
}
