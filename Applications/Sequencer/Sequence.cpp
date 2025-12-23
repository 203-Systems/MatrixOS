#include "Sequence.h"
#include "Scales.h"
#include <cmath>

using std::vector;

Sequence::Sequence(uint8_t tracks)
{
    New(tracks);
}

void Sequence::New(uint8_t tracks)
{
    if (tracks > 32)
    {
        // Don't support 32+ tracks
        MatrixOS::SYS::ErrorHandler("Too Many Tracks");
    }

    // Initialize sequence data
    data.bpm = 120;
    data.swing = 50;
    data.beatsPerBar = 4;
    data.beatUnit = 4;
    data.stepDivision = 4;
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
    lastRecordLayer = 0;
    currentRecordLayer = 0;

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
    for(uint8_t track = 0; track < trackPlayback.size(); track++)
    {
        uint8_t currentClip = trackPlayback[track].position.clip;
        PlayClip(track, currentClip);
    }
}

void Sequence::Play(uint8_t track)
{
    uint8_t currentClip = trackPlayback[track].position.clip;
    PlayClip(track, currentClip);
}

void Sequence::PlayClip(uint8_t track, uint8_t clip)
{
    if (!ClipExists(track, clip))
    {
        clip = 254;
    }

    // Initialize timing if not already playing
    if (!playing) {
        playing = true;
        clocksTillStart = record ? 24 * 4 + 1 : 1;
        currentPulse = UINT16_MAX;
        currentStep = 0;
        pulseSinceStart = 0;
        currentRecordLayer = 0;

        for (uint8_t i = 0; i < trackPlayback.size(); i++) {
            // Reset positions and prepare for playback
            trackPlayback[i].position.pattern = 0;
            trackPlayback[i].position.step = 0;
            trackPlayback[i].position.pulse = UINT16_MAX; // so first advance lands on 0

            // Clear playback state
            trackPlayback[i].nextClip = 255;
            trackPlayback[i].playing = false;
            trackPlayback[i].noteOffMap.clear();
            trackPlayback[i].noteOffQueue.clear();
        }

        MatrixOS::MIDI::Send(MidiPacket::Start(), MIDI_PORT_ALL);
    }

    // Terminate recorded notes on this track before switching
    TerminateRecordedNotes(track);

    // Queue this clip to play after current patterns finish
    trackPlayback[track].nextClip = clip;
}

void Sequence::PlayClipForAllTracks(uint8_t clip)
{
    for (uint8_t track = 0; track < data.tracks.size(); track++)
    {
       PlayClip(track, clip);
    }
}

void Sequence::PlayFrom(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t step)
{
    // Validate clip exists
    if (!ClipExists(track, clip)) return;

    // Validate pattern exists
    if (pattern >= GetPatternCount(track, clip)) return;

    // Get the pattern to validate step
    SequencePattern* targetPattern = GetPattern(track, clip, pattern);
    if (!targetPattern) return;

    // Validate step is within pattern bounds
    if (step >= targetPattern->steps) return;

    // Initialize timing if not already playing
    if (!playing) {
        playing = true;
        clocksTillStart = record ? 24 * 4 + 1 : 1;
        currentPulse = UINT16_MAX;
        pulseSinceStart = 0;
        currentRecordLayer = 0;

        for (uint8_t i = 0; i < trackPlayback.size(); i++) {
            // Reset positions and prepare for playback
            trackPlayback[i].position.pattern = 0;
            trackPlayback[i].position.step = 0;
            trackPlayback[i].position.pulse = UINT16_MAX; // so first advance lands on 0

            // Clear playback state
            trackPlayback[i].nextClip = 255;
            trackPlayback[i].playing = false;
            trackPlayback[i].canResume = false;  // Clear resume state when starting fresh playback
            trackPlayback[i].noteOffMap.clear();
            trackPlayback[i].noteOffQueue.clear();
        }
    }

    // Set the track position directly to our target location
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.step = step;
    trackPlayback[track].position.pulse = UINT16_MAX;  // Will advance to 0 on first tick

    // Start playback for this track
    trackPlayback[track].playing = true;
    trackPlayback[track].nextClip = 255;  // No queued clip change

    // Calculate steps elapsed from clip start to our starting position
    uint16_t stepSinceStart = 0;

    // Sum up all steps in patterns before our target pattern
    for (uint8_t p = 0; p < pattern; p++) {
        SequencePattern* pat = GetPattern(track, clip, p);
        if (pat) {
            stepSinceStart += pat->steps;
        }
    }

    // Add steps within the target pattern up to our starting step
    stepSinceStart += step;

    // Set currentStep to sync with bar boundaries
    currentStep = stepSinceStart % barLength;
}

void Sequence::Resume()
{
    if(CanResume() == false)
    {
        return;
    }

    // Initialize timing if not already playing
    playing = true;
    currentPulse = UINT16_MAX;
    clocksTillStart = record ? 24 * 4 + 1 : 1;  // Count-in for recording, immediate otherwise

    // Resume only tracks that were playing before stop
    for (uint8_t track = 0; track < trackPlayback.size(); track++) {
        if (trackPlayback[track].canResume) {
            // Restore position from resumePosition
            trackPlayback[track].position = trackPlayback[track].resumePosition;
            trackPlayback[track].position.pulse = UINT16_MAX;

            // Clear playback state
            trackPlayback[track].nextClip = 255;
            trackPlayback[track].playing = true;
            trackPlayback[track].noteOffMap.clear();
            trackPlayback[track].noteOffQueue.clear();
            trackPlayback[track].canResume = false;  // Clear after resuming
        }
    }

    MatrixOS::MIDI::Send(MidiPacket::Continue(), MIDI_PORT_ALL);
}

bool Sequence::CanResume()
{
    if(playing) return false;
    for (uint8_t track = 0; track < trackPlayback.size(); track++) {
        if (trackPlayback[track].canResume) {
            return true;
        }
    }
    return false;
}

bool Sequence::Playing()
{
    return playing;
}

bool Sequence::Playing(uint8_t track)
{
    if (track >= trackPlayback.size()) return false;
    return trackPlayback[track].playing;
}

void Sequence::Stop()
{
    playing = false;
    record = false;
    uint8_t sessionLayer = currentRecordLayer;

    // Send note-off for all queued notes before clearing
    for (uint8_t track = 0; track < trackPlayback.size(); track++) {
        uint8_t channel = GetChannel(track);
        TerminateRecordedNotes(track);
        for (const auto& [note, tick] : trackPlayback[track].noteOffMap) {
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0), MIDI_PORT_ALL);
        }
        MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, 120, 0), MIDI_PORT_ALL); // All sound off per channel

        // Save current position to resume position and mark if it can be resumed
        trackPlayback[track].resumePosition = trackPlayback[track].position;
        trackPlayback[track].canResume = trackPlayback[track].playing;

        trackPlayback[track].noteOffMap.clear();
        trackPlayback[track].noteOffQueue.clear();
        trackPlayback[track].nextClip = 255;
        trackPlayback[track].playing = false;
    }
    
    MatrixOS::MIDI::Send(MidiPacket::Stop(), MIDI_PORT_ALL);

    if (sessionLayer > 127 && lastRecordLayer > 127)
    {
        for (auto& track : data.tracks)
        {
            for (auto& clipPair : track.clips)
            {
                for (auto& pattern : clipPair.second.patterns)
                {
                    for (auto& ev : pattern.events)
                    {
                        if (ev.second.eventType == SequenceEventType::NoteEvent)
                        {
                            int16_t rl = ev.second.recordLayer - 127;
                            ev.second.recordLayer = rl > 0 ? rl : 0;
                        }
                    }
                }
            }
        }
        lastRecordLayer = lastRecordLayer - 127;
    }
    currentRecordLayer = 0;
    clocksTillStart = 0;
}

void Sequence::Stop(uint8_t track)
{
    if (track >= trackPlayback.size()) return;

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
        clocksTillStart = 0;
        trackPlayback[track].resumePosition = trackPlayback[track].position;
        MatrixOS::MIDI::Send(MidiPacket::Stop(), MIDI_PORT_ALL);
    }
}

void Sequence::StopAfter(uint8_t track)
{
    if (track >= trackPlayback.size()) return;

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
    if (track > data.tracks.size()) return false;
    return data.tracks[track].clips.size();
}

bool Sequence::ClipExists(uint8_t track, uint8_t clip)
{   
    if (track > data.tracks.size()) return false;
    if (clip > 127) return false;
    return data.tracks[track].clips.find(clip) != data.tracks[track].clips.end();
}

bool Sequence::NewClip(uint8_t track, uint8_t clipId)
{
    if (ClipExists(track, clipId)) return false;

    data.tracks[track].clips[clipId] = SequenceClip();
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
    if (trackPlayback[track].position.clip == clip)
    {
        if(Playing(track))
        {
            Stop(track);
        }

        // Find lowest available clip or create clip 0
        if (data.tracks[track].clips.empty())
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
                if (clipIdx < lowestClip)
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

SequencePattern* Sequence::GetPattern(uint8_t track, uint8_t clip, uint8_t pattern)
{
    if (!ClipExists(track, clip)) return nullptr;
    auto& pats = data.tracks[track].clips[clip].patterns;
    if (pattern >= pats.size()) return nullptr;
    return &pats[pattern];
}

int8_t Sequence::NewPattern(uint8_t track, uint8_t clip, uint8_t steps)
{
    if (!ClipExists(track, clip)) return -1;
    if (data.tracks[track].clips[clip].patterns.size() >= SEQUENCE_MAX_PATTERN_COUNT) {return -1;}

    // If length is 0, use patternLength as default
    uint8_t actualLength = (steps == 0) ? data.patternLength : steps;

    data.tracks[track].clips[clip].patterns.emplace_back();
    data.tracks[track].clips[clip].patterns.back().steps = actualLength;
    dirty = true;
    return data.tracks[track].clips[clip].patterns.size() - 1;
}

void Sequence::ClearAllStepsInClip(uint8_t track, uint8_t clip)
{
    if (!ClipExists(track, clip)) return;

    auto& patterns = data.tracks[track].clips[clip].patterns;
    for (auto& pattern : patterns)
    {
        pattern.Clear();
    }
    dirty = true;
}

bool Sequence::PatternClearAll(SequencePattern* pattern)
{
    if (!pattern) return false;
    if (!pattern->events.empty())
    {
        pattern->events.clear();
        dirty = true;
    }
    return true;
}

bool Sequence::PatternAddEvent(SequencePattern* pattern, uint16_t timestamp, const SequenceEvent& event)
{
    if (!pattern) return false;
    uint32_t patternLimit = pattern->steps * pulsesPerStep;
    if (timestamp >= patternLimit) return false;
    pattern->events.insert({timestamp, event});
    dirty = true;
    return true;
}

bool Sequence::PatternHasEventInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime, SequenceEventType type)
{
    if (!pattern) return false;
    auto it = pattern->events.lower_bound(startTime);
    while (it != pattern->events.end() && it->first <= endTime)
    {
        if (type == SequenceEventType::Invalid || it->second.eventType == type)
        {
            return true;
        }
        ++it;
    }
    return false;
}

bool Sequence::PatternClearNotesInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime, uint8_t note)
{
    if (!pattern) return false;
    bool removed = false;
    for (auto it = pattern->events.lower_bound(startTime); it != pattern->events.end() && it->first <= endTime; )
    {
        if (it->second.eventType == SequenceEventType::NoteEvent)
        {
            const SequenceEventNote& n = std::get<SequenceEventNote>(it->second.data);
            if (n.note == note)
            {
                it = pattern->events.erase(it);
                removed = true;
                continue;
            }
        }
        ++it;
    }
    if (removed) { dirty = true; }
    return removed;
}

bool Sequence::PatternOffsetNotesInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime, int8_t offset)
{
    if (!pattern || offset == 0) return false;

    std::vector<std::pair<uint16_t, SequenceEvent>> eventsToModify;
    std::vector<decltype(pattern->events)::iterator> eventsToRemove;

    // Collect events that need modification
    for (auto it = pattern->events.lower_bound(startTime); it != pattern->events.end() && it->first <= endTime; ++it)
    {
        if (it->second.eventType == SequenceEventType::NoteEvent)
        {
            SequenceEventNote noteData = std::get<SequenceEventNote>(it->second.data);
            int16_t newNote = noteData.note + offset;

            // Delete notes that go out of valid MIDI range (0-127)
            if (newNote < 0 || newNote > 127)
            {
                eventsToRemove.push_back(it);
            }
            else
            {
                // Update note value
                noteData.note = (uint8_t)newNote;
                SequenceEvent modifiedEvent = it->second;
                modifiedEvent.data = noteData;
                eventsToModify.push_back({it->first, modifiedEvent});
                eventsToRemove.push_back(it);
            }
        }
    }

    if (eventsToModify.empty() && eventsToRemove.empty()) return false;

    // Remove old events
    for (auto it : eventsToRemove)
    {
        pattern->events.erase(it);
    }

    // Insert modified events
    for (const auto& [timestamp, event] : eventsToModify)
    {
        pattern->events.insert({timestamp, event});
    }

    dirty = true;
    return true;
}

bool Sequence::PatternClearEventsInRange(SequencePattern* pattern, uint16_t startTime, uint16_t endTime)
{
    if (!pattern) return false;
    bool removed = false;
    for (auto it = pattern->events.lower_bound(startTime); it != pattern->events.end() && it->first <= endTime; )
    {
        it = pattern->events.erase(it);
        removed = true;
    }
    if (removed) { dirty = true; }
    return removed;
}

bool Sequence::PatternClearStepEvents(SequencePattern* pattern, uint8_t step, uint16_t pulsesPerStep)
{
    if (!pattern) return false;
    uint16_t startTime = step * pulsesPerStep;
    uint16_t endTime = startTime + pulsesPerStep - 1;
    return PatternClearEventsInRange(pattern, startTime, endTime);
}

bool Sequence::PatternCopyStepEvents(SequencePattern* pattern, uint8_t src, uint8_t dest, uint16_t pulsesPerStep)
{
    if (!pattern || src == dest) return false;
    uint16_t sourceStartTime = src * pulsesPerStep;
    uint16_t destStartTime = dest * pulsesPerStep;
    PatternCopyEventsInRange(pattern, sourceStartTime, destStartTime, pulsesPerStep);
    return true;
}

bool Sequence::PatternCopyEventsInRange(SequencePattern* pattern, uint16_t sourceStart, uint16_t destStart, uint16_t length)
{
    if (!pattern) return false;
    vector<std::pair<uint16_t, SequenceEvent>> eventsToCopy;

    uint16_t sourceEnd = sourceStart + length - 1;

    // Collect events in source range
    auto it = pattern->events.lower_bound(sourceStart);
    while (it != pattern->events.end() && it->first <= sourceEnd)
    {
        uint16_t offset = it->first - sourceStart;
        uint16_t newTimestamp = destStart + offset;
        eventsToCopy.push_back({newTimestamp, it->second});
        ++it;
    }

    if (eventsToCopy.empty()) { return true; }

    // Add copied events to destination
    for (const auto& [timestamp, event] : eventsToCopy)
    {
        pattern->events.insert({timestamp, event});
    }
    dirty = true;
    return true;
}

bool Sequence::PatternSetLength(SequencePattern* pattern, uint8_t steps)
{
    if (!pattern) return false;

    // Remove any events that now fall beyond the new pattern length
    uint32_t maxPulse = steps * pulsesPerStep;
    for (auto it = pattern->events.begin(); it != pattern->events.end(); )
    {
        if (it->first >= maxPulse)
        {
            it = pattern->events.erase(it);
        }
        else
        {
            ++it;
        }
    }

    pattern->steps = steps;
    dirty = true;
    return true;
}

bool Sequence::PatternQuantize(SequencePattern* pattern, SequencePattern* patternNext, uint16_t stepPulse)
{
    if (!pattern || stepPulse == 0) return false;

    int32_t patternLen = pattern->steps * pulsesPerStep;
    
    std::multimap<uint16_t, SequenceEvent> currentQuantized;
    bool changed = false;

    auto quantizeVal = [stepPulse](uint16_t val) -> uint16_t {
        return (val + stepPulse / 2) / stepPulse * stepPulse;
    };

    for (const auto& [timestamp, ev] : pattern->events)
    {
        uint16_t newTimestamp = quantizeVal(timestamp);
        SequenceEvent newEvent = ev;

        if (ev.eventType == SequenceEventType::NoteEvent)
        {
            auto& note = std::get<SequenceEventNote>(newEvent.data);
            uint16_t newLen = quantizeVal(note.length);
            if (newLen == 0) newLen = stepPulse; 
            if (newLen != note.length) { note.length = newLen; changed = true; }
        }

        if (newTimestamp != timestamp) changed = true;

        if (newTimestamp >= patternLen)
        {
            if (patternNext)
            {
                uint16_t overflowTimestamp = newTimestamp - patternLen;

                if (patternNext == pattern)
                {
                    // Loop to Self: Insert into TEMP map
                    currentQuantized.insert({overflowTimestamp, newEvent});
                }
                else
                {
                    // Distinct patternNext: Safe to insert directly
                    patternNext->events.insert({overflowTimestamp, newEvent});
                }
            }
        }
        else
        {
            currentQuantized.insert({newTimestamp, newEvent});
        }
    }

    if (changed)
    {
        pattern->events.swap(currentQuantized);
        dirty = true;
    }
    
    return true;
}

bool Sequence::DualPatternQuantize(SequencePattern* pattern1, SequencePattern* pattern2, SequencePattern* patternNext, uint16_t stepPulse)
{
    if (!pattern2) return PatternQuantize(pattern1, patternNext, stepPulse);
    if (!pattern1 || stepPulse == 0) return false;
    PatternQuantize(pattern1, pattern2, stepPulse);
    PatternQuantize(pattern2, patternNext, stepPulse);
    return true;
}

bool Sequence::PatternNudge(SequencePattern* pattern, int16_t offsetPulse)
{
    if (!pattern) return false;

    // Total pulses in the pattern
    int32_t patternLengthPulses = pattern->steps * pulsesPerStep;
    if (patternLengthPulses == 0) return true;

    // Normalize offset to [0, patternLengthPulses)
    int32_t normalized = offsetPulse % patternLengthPulses;
    if (normalized == 0) return true;
    if (normalized < 0) { normalized += patternLengthPulses; }

    std::multimap<uint16_t, SequenceEvent> shifted;
    for (const auto& [timestamp, ev] : pattern->events)
    {
        int32_t shiftedTs = (timestamp + normalized) % patternLengthPulses;
        shifted.insert({shiftedTs, ev});
    }

    pattern->events.swap(shifted);
    dirty = true;
    return true;
}

bool Sequence::DualPatternNudge(SequencePattern* pattern1, SequencePattern* pattern2, int16_t offsetPulse)
{
    if (!pattern1) return false;

    // Fallback to single pattern logic if pattern2 is missing
    if (!pattern2) {
        return PatternNudge(pattern1, offsetPulse);
    }

    // Calculate lengths
    int32_t len1 = pattern1->steps * pulsesPerStep;
    int32_t len2 = pattern2->steps * pulsesPerStep;
    int32_t totalLen = len1 + len2;

    if (totalLen == 0) return true;

    // Normalize offset to [0, totalLen)
    int32_t normalized = offsetPulse % totalLen;
    if (normalized == 0) return true;
    if (normalized < 0) { normalized += totalLen; }

    // Temporary storage for the new state of both patterns
    std::multimap<uint16_t, SequenceEvent> newEvents1;
    std::multimap<uint16_t, SequenceEvent> newEvents2;

    // --- Process Pattern 1 Events ---
    // These exist virtually from [0 to len1)
    for (const auto& [timestamp, ev] : pattern1->events)
    {
        // Calculate new absolute position in the combined timeline
        int32_t newAbsTs = (timestamp + normalized) % totalLen;

        if (newAbsTs < len1) {
            // It lands back inside Pattern 1
            newEvents1.insert({(uint16_t)newAbsTs, ev});
        } else {
            // It lands inside Pattern 2 (subtract len1 to get local P2 time)
            newEvents2.insert({(uint16_t)(newAbsTs - len1), ev});
        }
    }

    // --- Process Pattern 2 Events ---
    // These exist virtually from [len1 to totalLen)
    for (const auto& [timestamp, ev] : pattern2->events)
    {
        // Current absolute position is local timestamp + len1
        int32_t currentAbsTs = timestamp + len1;
        
        // Calculate new absolute position
        int32_t newAbsTs = (currentAbsTs + normalized) % totalLen;

        if (newAbsTs < len1) {
            // It wraps around to Pattern 1
            newEvents1.insert({(uint16_t)newAbsTs, ev});
        } else {
            // It stays/lands in Pattern 2
            newEvents2.insert({(uint16_t)(newAbsTs - len1), ev});
        }
    }

    // Apply changes
    pattern1->events.swap(newEvents1);
    pattern2->events.swap(newEvents2);
    dirty = true;

    return true;
}

bool Sequence::PatternNudgeInRange(SequencePattern* pattern, uint16_t startTime, uint16_t length, int16_t offsetPulse, SequencePattern* prevPattern, SequencePattern* nextPattern)
{
    if (!pattern || offsetPulse == 0 || length == 0) return false;

    int32_t patternLengthPulses = pattern->steps * pulsesPerStep;
    if (patternLengthPulses == 0) return false;

    uint16_t endTime = startTime + length - 1;
    std::vector<std::pair<uint16_t, SequenceEvent>> eventsToMove;
    std::vector<decltype(pattern->events)::iterator> eventsToRemove;

    // Collect events in the specified range
    for (auto it = pattern->events.lower_bound(startTime); it != pattern->events.end() && it->first <= endTime; ++it)
    {
        int32_t newTimestamp = (int32_t)it->first + offsetPulse;

        // Check if event stays within current pattern
        if (newTimestamp >= 0 && newTimestamp < patternLengthPulses)
        {
            eventsToMove.push_back({(uint16_t)newTimestamp, it->second});
            eventsToRemove.push_back(it);
        }
        // Check if event goes to previous pattern
        else if (newTimestamp < 0 && prevPattern != nullptr)
        {
            int32_t prevPatternLength = prevPattern->steps * pulsesPerStep;
            int32_t prevTimestamp = prevPatternLength + newTimestamp;
            if (prevTimestamp >= 0 && prevTimestamp < prevPatternLength)
            {
                prevPattern->events.insert({(uint16_t)prevTimestamp, it->second});
                eventsToRemove.push_back(it);
            }
            // Discard if can't fit in previous pattern
            else
            {
                eventsToRemove.push_back(it);
            }
        }
        // Check if event goes to next pattern
        else if (newTimestamp >= patternLengthPulses && nextPattern != nullptr)
        {
            int32_t nextTimestamp = newTimestamp - patternLengthPulses;
            int32_t nextPatternLength = nextPattern->steps * pulsesPerStep;
            if (nextTimestamp >= 0 && nextTimestamp < nextPatternLength)
            {
                nextPattern->events.insert({(uint16_t)nextTimestamp, it->second});
                eventsToRemove.push_back(it);
            }
            // Discard if can't fit in next pattern
            else
            {
                eventsToRemove.push_back(it);
            }
        }
        // Discard if overflow and no prev/next pattern
        else
        {
            eventsToRemove.push_back(it);
        }
    }

    if (eventsToMove.empty() && eventsToRemove.empty()) return false;

    // Remove old events
    for (auto it : eventsToRemove)
    {
        pattern->events.erase(it);
    }

    // Insert moved events back into current pattern
    for (const auto& [timestamp, event] : eventsToMove)
    {
        pattern->events.insert({timestamp, event});
    }

    dirty = true;
    return true;
}

void Sequence::DeletePattern(uint8_t track, uint8_t clip, uint8_t pattern)
{
    if (!ClipExists(track, clip)) return;

    auto& patterns = data.tracks[track].clips[clip].patterns;
    if (pattern >= patterns.size()) return;

    if (patterns.size() == 1)
    {
        // Clear instead of delete (keep at least 1 pattern)
        patterns[0].Clear();
    }
    else
    {
        patterns.erase(patterns.begin() + pattern);

        // Update position if pointing to this clip and pattern
        if (trackPlayback[track].position.clip == clip && trackPlayback[track].position.pattern >= pattern)
        {
            if (trackPlayback[track].position.pattern > 0)
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
    if (sourcePattern >= sourcePatterns.size()) return;

    SequencePattern& source = sourcePatterns[sourcePattern];

    if (destPattern == 255)
    {
        // Create new pattern
        auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;
        if (destPatterns.size() >= SEQUENCE_MAX_PATTERN_COUNT) {return;}
        destPatterns.emplace_back();
        SequencePattern& dest = destPatterns.back();
        dest.steps = source.steps;
        dest.events = source.events;
    }
    else
    {
        // Overwrite existing pattern
        auto& destPatterns = data.tracks[destTrack].clips[destClip].patterns;
        if (destPattern >= destPatterns.size()) return;
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
    if (data.tracks[track].channel != channel)
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
    if (bpm != data.bpm)
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
    if (swing != data.swing)
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
    if (stepLen == 0) { return; }
    if (stepLen != stepDivision)
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
    if (patternLength != data.patternLength)
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

    if (beatsPerBar > 16)
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
    if (track >= trackPlayback.size()) return false;
    return (data.solo >> track) & 1;
}

void Sequence::SetSolo(uint8_t track, bool val)
{
    if (track >= trackPlayback.size()) return;
    uint32_t mask = 1 << track;
    bool currentVal = (data.solo >> track) & 1;

    if (currentVal != val)
    {
        if (val)
            data.solo = mask;
        else
            data.solo = 0;

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
    if (track >= trackPlayback.size()) return false;
    return (data.mute >> track) & 1;
}

void Sequence::SetMute(uint8_t track, bool val)
{
    if (track >= trackPlayback.size()) return;

    uint32_t mask = 1 << track;
    bool currentVal = (data.mute >> track) & 1;

    if (currentVal != val)
    {
        if (val)
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
    if (track >= trackPlayback.size()) return false;
    return GetEnabled(track) && GetRecord(track);
}

bool Sequence::GetRecord(uint8_t track)
{
    if (track >= trackPlayback.size()) return false;
    return (data.record >> track) & 1;
}

void Sequence::SetRecord(uint8_t track, bool val)
{
    if (track >= trackPlayback.size()) return;
    
    uint32_t mask = 1 << track;
    bool currentVal = (data.record >> track) & 1;

    if (currentVal != val)
    {
        if (val)
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
    if (GetMute(track))
        return false;

    if (data.solo != 0 && !GetSolo(track))
        return false;

    return true;
}

bool Sequence::IsNoteActive(uint8_t track, uint8_t note) const
{   
    if (track >= trackPlayback.size()) return false;
    auto it = trackPlayback[track].noteOffMap.find(note);
    return it != trackPlayback[track].noteOffMap.end();
}

SequencePosition* Sequence::GetPosition(uint8_t track)
{
    if (track >= trackPlayback.size()) return nullptr;
    return &trackPlayback[track].position;
}

void Sequence::SetPosition(uint8_t track, uint8_t clip, uint8_t pattern, uint8_t step)
{
    if (track >= trackPlayback.size()) return;
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.step = step;
    // data.tracks[track].activeClip = clip;
    // dirty = true;
}

void Sequence::SetClip(uint8_t track, uint8_t clip)
{
    if (track >= trackPlayback.size()) return;
    trackPlayback[track].position.clip = clip;
    trackPlayback[track].position.pattern = 0;
    trackPlayback[track].position.step = 0;
    data.tracks[track].activeClip = clip;
    dirty = true;
}

void Sequence::SetPattern(uint8_t track, uint8_t pattern)
{
    if (track >= trackPlayback.size()) return;
    trackPlayback[track].position.pattern = pattern;
    trackPlayback[track].position.step = 0;
}

uint8_t Sequence::GetNextClip(uint8_t track)
{
    if (track >= trackPlayback.size()) return 255;
    return trackPlayback[track].nextClip;
}

void Sequence::SetNextClip(uint8_t track, uint8_t clip)
{
    if (track >= trackPlayback.size()) return;
    trackPlayback[track].nextClip = clip;
}

uint32_t Sequence::GetLastEventTime(uint8_t track)
{
    if (track >= trackPlayback.size()) return 0;
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

    if (track != 0xFF && track >= trackPlayback.size()) return;

    EMidiStatus status = packet.Status();
    if (status != EMidiStatus::NoteOn && status != EMidiStatus::NoteOff)
    {
        return;
    }

    const uint16_t recordGraceClocks = 24; // one Quartter Note
    bool isCountIn = clocksTillStart > 0;
    bool clampToStart = isCountIn && clocksTillStart <= recordGraceClocks;
    if (isCountIn && !clampToStart) {
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

        SequencePattern* pattern = GetPattern(t, clipIdx, patIdx);
        if (!pattern) continue;
        uint32_t currentTick = 0;
        uint16_t pulse = trackPlayback[t].position.pulse;
        if (pulse == UINT16_MAX) {
            pulse = 0;
        }
        if (clampToStart) {
            currentTick = trackPlayback[t].position.step * pulsesPerStep;
        } else {
            currentTick = trackPlayback[t].position.step * pulsesPerStep + pulse;
        }
        auto& pending = trackPlayback[t].recordedNotes;

        if (status == EMidiStatus::NoteOn && velocity > 0)
        {
            if (currentRecordLayer == 0)
            {
                lastRecordLayer++;
                if (lastRecordLayer == 0) { lastRecordLayer = 1; }
                currentRecordLayer = lastRecordLayer;
            }
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

            auto evIt = pattern->events.insert({(uint16_t)currentTick, SequenceEvent::Note(note, velocity, false, 1)});
            SequenceEvent& evRef = evIt->second;
            if (evRef.eventType == SequenceEventType::NoteEvent)
            {
                evRef.recordLayer = currentRecordLayer;
            }
            Sequence::TrackPlayback::RecordedNote info;
            info.startPulse = clampToStart ? 0 : pulseSinceStart;
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

bool Sequence::CanUndoLastRecord()
{
    return lastRecordLayer > 0;
}

void Sequence::UndoLastRecorded()
{
    if (lastRecordLayer == 0)
    {
        return;
    }

    bool removed = false;
    for (auto& track : data.tracks)
    {
        for (auto& clipPair : track.clips)
        {
            for (auto& pattern : clipPair.second.patterns)
            {
                for (auto it = pattern.events.begin(); it != pattern.events.end();)
                {
                    if (it->second.recordLayer == lastRecordLayer)
                    {
                        it = pattern.events.erase(it);
                        removed = true;
                        continue;
                    }
                    ++it;
                }
            }
        }
    }

    if (removed)
    {
        if (lastRecordLayer > 0) { lastRecordLayer--; }
        currentRecordLayer = 0;
        dirty = true;
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
    if (track >= trackPlayback.size()) return;

    bool trackEnabled = GetEnabled(track);

    SequencePosition& pos = trackPlayback[track].position;

    // Check for queued clip change at bar boundary (currentStep == 0)
    if (trackPlayback[track].nextClip != 255 && currentStep == 0) {
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
    else
    {
        // Only process tracks that are currently playing
        if (!trackPlayback[track].playing) return;

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
                if (SequencePattern* pat = GetPattern(track, clip, pattern)) {
                    patternSteps = pat->steps;
                }
            }
            if (pos.step >= patternSteps) {
                pos.step = 0;
                pos.pattern++;
                
                if (ClipExists(track, clip) && pos.pattern >= GetPatternCount(track, clip)) {
                    pos.pattern = 0;
                }
            }
        }
    }

    // 2. Fire events at current tick position
    uint8_t clip = pos.clip;
    uint8_t pattern = pos.pattern;
    if (trackEnabled && ClipExists(track, clip)) {
        SequencePattern* currentPattern = GetPattern(track, clip, pattern);
        if (!currentPattern) return;

        // Calculate the absolute tick position within the pattern
        uint16_t currentTick = pos.step * pulsesPerStep + pos.pulse;

        // Fire all events at exactly this tick (there can be multiple at the same timestamp)
        auto range = currentPattern->events.equal_range(currentTick);
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
