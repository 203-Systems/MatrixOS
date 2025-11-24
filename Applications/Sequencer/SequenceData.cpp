#include "SequenceData.h"
#include "cb0r.h"
#include "cb0rHelper.h"
#include <string>

using std::vector;
using std::string;

void SequencePattern::Clear()
{
    events.clear();
}

void SequencePattern::AddEvent(uint16_t timestamp, const SequenceEvent& event)
{
    events.insert({timestamp, event});
}

bool SequencePattern::HasEventInRange(uint16_t startTime, uint16_t endTime, SequenceEventType eventType)
{
    auto it = events.lower_bound(startTime);
    while (it != events.end() && it->first <= endTime)
    {
        if (eventType == SequenceEventType::Invalid || it->second.eventType == eventType)
        {
            return true;
        }
        ++it;
    }
    return false;
}

bool SequencePattern::RemoveNoteEventsInRange(uint16_t startTime, uint16_t endTime, uint8_t note)
{
    bool removed = false;
    auto it = events.lower_bound(startTime);
    while (it != events.end() && it->first <= endTime)
    {
        if (it->second.eventType == SequenceEventType::NoteEvent)
        {
            const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
            if (noteData.note == note)
            {
                it = events.erase(it);
                removed = true;
                continue;
            }
        }
        ++it;
    }
    return removed;
}

bool SequencePattern::RemoveAllEventsInRange(uint16_t startTime, uint16_t endTime)
{
    bool removed = false;
    auto it = events.lower_bound(startTime);
    while (it != events.end() && it->first <= endTime)
    {
        it = events.erase(it);
        removed = true;
    }
    return removed;
}

void SequencePattern::CopyEventsInRange(uint16_t sourceStart, uint16_t destStart, uint16_t length)
{
    vector<std::pair<uint16_t, SequenceEvent>> eventsToCopy;

    uint16_t sourceEnd = sourceStart + length - 1;

    // Collect events in source range
    auto it = events.lower_bound(sourceStart);
    while (it != events.end() && it->first <= sourceEnd)
    {
        uint16_t offset = it->first - sourceStart;
        uint16_t newTimestamp = destStart + offset;
        eventsToCopy.push_back({newTimestamp, it->second});
        ++it;
    }

    // Add copied events to destination
    for (const auto& [timestamp, event] : eventsToCopy)
    {
        events.insert({timestamp, event});
    }
}

void SequencePattern::ClearStepEvents(uint8_t step, uint16_t pulsesPerStep)
{
    uint16_t startTime = step * pulsesPerStep;
    uint16_t endTime = startTime + pulsesPerStep - 1;
    RemoveAllEventsInRange(startTime, endTime);
}

void SequencePattern::CopyStepEvents(uint8_t src, uint8_t dest, uint16_t pulsesPerStep)
{
    uint16_t sourceStartTime = src * pulsesPerStep;
    uint16_t destStartTime = dest * pulsesPerStep;
    CopyEventsInRange(sourceStartTime, destStartTime, pulsesPerStep);
}

static void serialize_event(vector<uint8_t>& out, const std::pair<uint16_t, SequenceEvent>& evPair)
{
    const SequenceEvent& ev = evPair.second;
    // map with 3 keys: "t","type","data"
    cb_write_uint(out, CB0R_MAP, 3);
    cb_write_text(out, "t"); cb_write_uint(out, CB0R_INT, evPair.first);
    cb_write_text(out, "type"); cb_write_uint(out, CB0R_INT, static_cast<uint8_t>(ev.eventType));
    cb_write_text(out, "data");
    if (ev.eventType == SequenceEventType::NoteEvent)
    {
        const SequenceEventNote& n = std::get<SequenceEventNote>(ev.data);
        cb_write_uint(out, CB0R_MAP, 4);
        cb_write_text(out, "note"); cb_write_uint(out, CB0R_INT, n.note);
        cb_write_text(out, "vel"); cb_write_uint(out, CB0R_INT, n.velocity);
        cb_write_text(out, "len"); cb_write_uint(out, CB0R_INT, n.length);
        cb_write_text(out, "aft"); cb_write_bool(out, n.aftertouch);
    }
    else if (ev.eventType == SequenceEventType::ControlChangeEvent)
    {
        const SequenceEventCC& cc = std::get<SequenceEventCC>(ev.data);
        cb_write_uint(out, CB0R_MAP, 2);
        cb_write_text(out, "param"); cb_write_uint(out, CB0R_INT, cc.param);
        cb_write_text(out, "val"); cb_write_uint(out, CB0R_INT, cc.value);
    }
    else
    {
        cb_write_uint(out, CB0R_MAP, 0);
    }
}

static void serialize_pattern(vector<uint8_t>& out, const SequencePattern& pat)
{
    // map with 2 keys: "steps","events"
    cb_write_uint(out, CB0R_MAP, 2);
    cb_write_text(out, "steps"); cb_write_uint(out, CB0R_INT, pat.steps);
    cb_write_text(out, "events");
    cb_write_uint(out, CB0R_ARRAY, pat.events.size());
    for (const auto& ev : pat.events)
    {
        serialize_event(out, ev);
    }
}

static void serialize_clip(vector<uint8_t>& out, const SequenceClip& clip)
{
    // map with 2 keys: "en","patterns"
    cb_write_uint(out, CB0R_MAP, 2);
    cb_write_text(out, "en"); cb_write_bool(out, clip.enabled);
    cb_write_text(out, "patterns");
    cb_write_uint(out, CB0R_ARRAY, clip.patterns.size());
    for (const auto& pat : clip.patterns)
    {
        serialize_pattern(out, pat);
    }
}

static void serialize_track(vector<uint8_t>& out, const SequenceTrack& track)
{
    // map with 3 keys: "ch","clip","clips"
    cb_write_uint(out, CB0R_MAP, 3);
    cb_write_text(out, "ch"); cb_write_uint(out, CB0R_INT, track.channel);
    cb_write_text(out, "clip"); cb_write_uint(out, CB0R_INT, track.activeClip);
    cb_write_text(out, "clips");
    cb_write_uint(out, CB0R_MAP, track.clips.size());
    for (const auto& [clipId, clip] : track.clips)
    {
        cb_write_uint(out, CB0R_INT, clipId);
        serialize_clip(out, clip);
    }
}

bool SerializeSequenceData(const SequenceData& data, std::vector<uint8_t>& out)
{
    out.clear();
    // Top-level map with 8 entries
    cb_write_uint(out, CB0R_MAP, 10);
    cb_write_text(out, "ver"); cb_write_uint(out, CB0R_INT, data.version);
    cb_write_text(out, "bpm"); cb_write_uint(out, CB0R_INT, data.bpm);
    cb_write_text(out, "swing"); cb_write_uint(out, CB0R_INT, data.swing);
    cb_write_text(out, "plen"); cb_write_uint(out, CB0R_INT, data.patternLength);
    cb_write_text(out, "beats"); cb_write_uint(out, CB0R_INT, data.beatsPerBar);
    cb_write_text(out, "beatUnit"); cb_write_uint(out, CB0R_INT, data.beatUnit);
    cb_write_text(out, "solo"); cb_write_uint(out, CB0R_INT, data.solo);
    cb_write_text(out, "mute"); cb_write_uint(out, CB0R_INT, data.mute);
    cb_write_text(out, "rec"); cb_write_uint(out, CB0R_INT, data.record);
    cb_write_text(out, "tracks");
    cb_write_uint(out, CB0R_ARRAY, data.tracks.size());
    for (const auto& track : data.tracks)
    {
        serialize_track(out, track);
    }
    return true;
}

// --- Deserialization helpers ---
static bool expect_type(cb0r_t parent, uint32_t index, cb0r_e type, cb0r_s& out)
{
    if (!cb0r_get(parent, index, &out)) return false;
    return out.type == type;
}

static bool parse_event(cb0r_s evMap, SequenceEvent& outEv)
{
    cb0r_s item;
    cb0r_find(&evMap, CB0R_UTF8, 1, (uint8_t*)"t", &item);
    uint16_t t = item.value;
    cb0r_find(&evMap, CB0R_UTF8, 4, (uint8_t*)"type", &item);
    uint8_t type = item.value;
    cb0r_find(&evMap, CB0R_UTF8, 4, (uint8_t*)"data", &item);
    if (type == static_cast<uint8_t>(SequenceEventType::NoteEvent))
    {
        SequenceEventNote note{};
        cb0r_s d;
        if (cb0r_find(&item, CB0R_UTF8, 4, (uint8_t*)"note", &d)) note.note = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 3, (uint8_t*)"vel", &d)) note.velocity = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 3, (uint8_t*)"len", &d)) note.length = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 3, (uint8_t*)"aft", &d)) note.aftertouch = (d.type == CB0R_TRUE);
        outEv = SequenceEvent{SequenceEventType::NoteEvent, note};
    }
    else if (type == static_cast<uint8_t>(SequenceEventType::ControlChangeEvent))
    {
        SequenceEventCC cc{};
        cb0r_s d;
        if (cb0r_find(&item, CB0R_UTF8, 5, (uint8_t*)"param", &d)) cc.param = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 3, (uint8_t*)"val", &d)) cc.value = d.value;
        outEv = SequenceEvent{SequenceEventType::ControlChangeEvent, cc};
    }
    else
    {
        outEv = SequenceEvent{SequenceEventType::Invalid, SequenceEventNote{}};
    }
    // time is handled by caller
    return true;
}

static bool parse_pattern(cb0r_s patMap, SequencePattern& pat)
{
    cb0r_s item;
    if (cb0r_find(&patMap, CB0R_UTF8, 5, (uint8_t*)"steps", &item)) pat.steps = item.value;
    if (cb0r_find(&patMap, CB0R_UTF8, 6, (uint8_t*)"events", &item) && item.type == CB0R_ARRAY)
    {
        cb0r_s ev = item;
        for (size_t i = 0; i < item.length; i++)
        {
            if (!cb0r_get(&item, i, &ev) || ev.type != CB0R_MAP) continue;
            SequenceEvent e(SequenceEventType::Invalid, SequenceEventNote{});
            parse_event(ev, e);
            cb0r_s tfield;
            if (cb0r_find(&ev, CB0R_UTF8, 1, (uint8_t*)"t", &tfield))
            {
                pat.events.insert({static_cast<uint16_t>(tfield.value), e});
            }
        }
    }
    return true;
}

static bool parse_clip(cb0r_s clipMap, SequenceClip& clip)
{
    cb0r_s item;
    if (cb0r_find(&clipMap, CB0R_UTF8, 2, (uint8_t*)"en", &item))
    {
        clip.enabled = (item.type == CB0R_TRUE);
    }
    if (cb0r_find(&clipMap, CB0R_UTF8, 8, (uint8_t*)"patterns", &item) && item.type == CB0R_ARRAY)
    {
        clip.patterns.clear();
        clip.patterns.reserve(item.length);
        cb0r_s pat = item;
        for (size_t i = 0; i < item.length; i++)
        {
            if (!cb0r_get(&item, i, &pat) || pat.type != CB0R_MAP) continue;
            SequencePattern p;
            parse_pattern(pat, p);
            clip.patterns.push_back(p);
        }
    }
    return true;
}

static bool parse_track(cb0r_s trackMap, SequenceTrack& track)
{
    cb0r_s item;
    if (cb0r_find(&trackMap, CB0R_UTF8, 2, (uint8_t*)"ch", &item)) track.channel = item.value;
    if (cb0r_find(&trackMap, CB0R_UTF8, 4, (uint8_t*)"clip", &item)) track.activeClip = item.value;
    if (cb0r_find(&trackMap, CB0R_UTF8, 5, (uint8_t*)"clips", &item) && item.type == CB0R_MAP)
    {
        track.clips.clear();
        cb0r_s clipEntry = item;
        // map has pairs key/value
        for (uint32_t i = 0; i < item.count; i += 2)
        {
            cb0r_s key;
            cb0r_s val;
            if (!cb0r_get(&item, i, &key) || key.type != CB0R_INT) continue;
            if (!cb0r_get(&item, i + 1, &val) || val.type != CB0R_MAP) continue;
            SequenceClip clip;
            parse_clip(val, clip);
            track.clips[(uint8_t)key.value] = clip;
        }
    }
    return true;
}

bool DeserializeSequenceData(const uint8_t* in, size_t len, SequenceData& out)
{
    cb0r_s root;
    if (!cb0r_read(const_cast<uint8_t*>(in), len, &root) || root.type != CB0R_MAP) return false;

    cb0r_s item;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"ver", &item)) out.version = item.value;

    if (out.version > SEQUENCE_VERSION || out.version < MIN_SUPPORTED_SEQUENCE_VERSION) return false;

    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bpm", &item)) out.bpm = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"swing", &item)) out.swing = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"plen", &item)) out.patternLength = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"beats", &item)) out.beatsPerBar = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 8, (uint8_t*)"beatUnit", &item)) out.beatUnit = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"solo", &item)) out.solo = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"mute", &item)) out.mute = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"rec", &item)) out.record = item.value;

    if (cb0r_find(&root, CB0R_UTF8, 6, (uint8_t*)"tracks", &item) && item.type == CB0R_ARRAY)
    {
        out.tracks.clear();
        out.tracks.reserve(item.length);
        cb0r_s track = item;
        for (size_t i = 0; i < item.length; i++)
        {
            if (!cb0r_get(&item, i, &track) || track.type != CB0R_MAP) continue;
            SequenceTrack t;
            parse_track(track, t);
            out.tracks.push_back(t);
        }
    }
    return true;
}
