#include "SequenceData.h"
#include "cb0r.h"
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

// --- CBOR serialization helpers ---
static void write_uint(vector<uint8_t>& out, cb0r_e type, uint64_t value)
{
    uint8_t buf[9];
    uint8_t len = cb0r_write(buf, type, value);
    out.insert(out.end(), buf, buf + len);
}

static void write_bytes(vector<uint8_t>& out, const uint8_t* data, size_t len)
{
    write_uint(out, CB0R_BYTE, len);
    out.insert(out.end(), data, data + len);
}

static void write_text(vector<uint8_t>& out, const std::string& s)
{
    write_uint(out, CB0R_UTF8, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

static void write_bool(vector<uint8_t>& out, bool v)
{
    out.push_back(v ? 0xF5 : 0xF4); // CBOR true/false
}

static void serialize_event(vector<uint8_t>& out, const std::pair<uint16_t, SequenceEvent>& evPair)
{
    const SequenceEvent& ev = evPair.second;
    // map with 3 keys: "t","type","data"
    write_uint(out, CB0R_MAP, 3);
    write_text(out, "t"); write_uint(out, CB0R_INT, evPair.first);
    write_text(out, "type"); write_uint(out, CB0R_INT, static_cast<uint8_t>(ev.eventType));
    write_text(out, "data");
    if (ev.eventType == SequenceEventType::NoteEvent)
    {
        const SequenceEventNote& n = std::get<SequenceEventNote>(ev.data);
        write_uint(out, CB0R_MAP, 4);
        write_text(out, "note"); write_uint(out, CB0R_INT, n.note);
        write_text(out, "vel"); write_uint(out, CB0R_INT, n.velocity);
        write_text(out, "len"); write_uint(out, CB0R_INT, n.length);
        write_text(out, "aft"); write_bool(out, n.aftertouch);
    }
    else if (ev.eventType == SequenceEventType::ControlChangeEvent)
    {
        const SequenceEventCC& cc = std::get<SequenceEventCC>(ev.data);
        write_uint(out, CB0R_MAP, 2);
        write_text(out, "param"); write_uint(out, CB0R_INT, cc.param);
        write_text(out, "val"); write_uint(out, CB0R_INT, cc.value);
    }
    else
    {
        write_uint(out, CB0R_MAP, 0);
    }
}

static void serialize_pattern(vector<uint8_t>& out, const SequencePattern& pat)
{
    // map with 2 keys: "steps","events"
    write_uint(out, CB0R_MAP, 2);
    write_text(out, "steps"); write_uint(out, CB0R_INT, pat.steps);
    write_text(out, "events");
    write_uint(out, CB0R_ARRAY, pat.events.size());
    for (const auto& ev : pat.events)
    {
        serialize_event(out, ev);
    }
}

static void serialize_clip(vector<uint8_t>& out, const SequenceClip& clip)
{
    // map with 2 keys: "en","patterns"
    write_uint(out, CB0R_MAP, 2);
    write_text(out, "en"); write_bool(out, clip.enabled);
    write_text(out, "patterns");
    write_uint(out, CB0R_ARRAY, clip.patterns.size());
    for (const auto& pat : clip.patterns)
    {
        serialize_pattern(out, pat);
    }
}

static void serialize_track(vector<uint8_t>& out, const SequenceTrack& track)
{
    // map with 3 keys: "ch","clip","clips"
    write_uint(out, CB0R_MAP, 3);
    write_text(out, "ch"); write_uint(out, CB0R_INT, track.channel);
    write_text(out, "clip"); write_uint(out, CB0R_INT, track.activeClip);
    write_text(out, "clips");
    write_uint(out, CB0R_MAP, track.clips.size());
    for (const auto& [clipId, clip] : track.clips)
    {
        write_uint(out, CB0R_INT, clipId);
        serialize_clip(out, clip);
    }
}

bool SerializeSequenceData(const SequenceData& data, std::vector<uint8_t>& out)
{
    out.clear();
    // Top-level map with 8 entries
    write_uint(out, CB0R_MAP, 8);
    write_text(out, "ver"); write_uint(out, CB0R_INT, data.version);
    write_text(out, "bpm"); write_uint(out, CB0R_INT, data.bpm);
    write_text(out, "swing"); write_uint(out, CB0R_INT, data.swing);
    write_text(out, "bar"); write_uint(out, CB0R_INT, data.barLength);
    write_text(out, "solo"); write_uint(out, CB0R_INT, data.solo);
    write_text(out, "mute"); write_uint(out, CB0R_INT, data.mute);
    write_text(out, "rec"); write_uint(out, CB0R_INT, data.record);
    write_text(out, "tracks");
    write_uint(out, CB0R_ARRAY, data.tracks.size());
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
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bpm", &item)) out.bpm = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"swing", &item)) out.swing = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bar", &item)) out.barLength = item.value;
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
