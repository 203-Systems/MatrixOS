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

static void SerializeEvent(vector<uint8_t>& out, const std::pair<uint16_t, SequenceEvent>& evPair)
{
    const SequenceEvent& ev = evPair.second;
    cb_write_uint(out, CB0R_ARRAY, 3);
    cb_write_uint(out, CB0R_INT, evPair.first);                                  // timestamp
    cb_write_uint(out, CB0R_INT, static_cast<uint8_t>(ev.eventType));            // type

    if (ev.eventType == SequenceEventType::NoteEvent)
    {
        const SequenceEventNote& n = std::get<SequenceEventNote>(ev.data);
        cb_write_uint(out, CB0R_ARRAY, 4);
        cb_write_uint(out, CB0R_INT, n.note);
        cb_write_uint(out, CB0R_INT, n.velocity);
        cb_write_uint(out, CB0R_INT, n.length);
        cb_write_bool(out, n.aftertouch);
    }
    else if (ev.eventType == SequenceEventType::ControlChangeEvent)
    {
        const SequenceEventCC& cc = std::get<SequenceEventCC>(ev.data);
        cb_write_uint(out, CB0R_ARRAY, 2);
        cb_write_uint(out, CB0R_INT, cc.param);
        cb_write_uint(out, CB0R_INT, cc.value);
    }
    else
    {
        cb_write_uint(out, CB0R_ARRAY, 0);
    }
}

static void SerializePattern(vector<uint8_t>& out, const SequencePattern& pat)
{
    size_t startSize = out.size();
    cb_write_uint(out, CB0R_ARRAY, 2);
    cb_write_uint(out, CB0R_INT, pat.steps);
    cb_write_uint(out, CB0R_ARRAY, pat.events.size());
    for (const auto& ev : pat.events)
    {
        SerializeEvent(out, ev);
    }
    size_t bytes = out.size() - startSize;
    MLOGD("SequenceData", "Serialized pattern steps=%u events=%zu bytes=%zu", pat.steps, pat.events.size(), bytes);
}

static void SerializeClip(vector<uint8_t>& out, const SequenceClip& clip)
{
    size_t startSize = out.size();
    cb_write_uint(out, CB0R_ARRAY, 2);
    cb_write_bool(out, clip.enabled);
    cb_write_uint(out, CB0R_ARRAY, clip.patterns.size());
    for (const auto& pat : clip.patterns)
    {
        SerializePattern(out, pat);
    }
    size_t bytes = out.size() - startSize;
    MLOGD("SequenceData", "Serialized clip patterns=%zu bytes=%zu", clip.patterns.size(), bytes);
}

static void SerializeTrack(vector<uint8_t>& out, const SequenceTrack& track)
{
    size_t startSize = out.size();
    cb_write_uint(out, CB0R_ARRAY, 3);
    cb_write_uint(out, CB0R_INT, track.channel);
    cb_write_uint(out, CB0R_INT, track.activeClip);
    cb_write_uint(out, CB0R_ARRAY, track.clips.size());
    for (const auto& [clipId, clip] : track.clips)
    {
        cb_write_uint(out, CB0R_ARRAY, 2);
        cb_write_uint(out, CB0R_INT, clipId);
        SerializeClip(out, clip);
    }
    size_t bytes = out.size() - startSize;
    MLOGD("SequenceData", "Serialized track channel=%u clips=%zu bytes=%zu", track.channel, track.clips.size(), bytes);
}

bool SerializeSequenceData(const SequenceData& data, std::vector<uint8_t>& out)
{
    out.clear();
    size_t startSize = out.size();
    // Top-level map with entries
    cb_write_uint(out, CB0R_MAP, 10);
    cb_write_text(out, "ver"); cb_write_uint(out, CB0R_INT, data.version);
    cb_write_text(out, "bpm"); cb_write_uint(out, CB0R_INT, data.bpm);
    cb_write_text(out, "swing"); cb_write_uint(out, CB0R_INT, data.swing);
    cb_write_text(out, "patternLen"); cb_write_uint(out, CB0R_INT, data.patternLength);
    cb_write_text(out, "beats"); cb_write_uint(out, CB0R_INT, data.beatsPerBar);
    cb_write_text(out, "beatUnit"); cb_write_uint(out, CB0R_INT, data.beatUnit);
    cb_write_text(out, "solo"); cb_write_uint(out, CB0R_INT, data.solo);
    cb_write_text(out, "mute"); cb_write_uint(out, CB0R_INT, data.mute);
    cb_write_text(out, "rec"); cb_write_uint(out, CB0R_INT, data.record);
    cb_write_text(out, "tracks");
    cb_write_uint(out, CB0R_ARRAY, data.tracks.size());
    for (const auto& track : data.tracks)
    {
        SerializeTrack(out, track);
    }
    size_t bytes = out.size() - startSize;
    MLOGD("SequenceData", "Serialized sequence tracks=%zu bytes=%zu", data.tracks.size(), bytes);
    return true;
}

// --- Deserialization helpers ---
static bool expect_type(cb0r_t parent, uint32_t index, cb0r_e type, cb0r_s& out)
{
    if (!cb0r_get(parent, index, &out)) return false;
    return out.type == type;
}

static bool ParseEvent(cb0r_s node, SequenceEvent& outEv, uint16_t& timestamp, uint8_t version)
{
    (void)version;
    if (node.type != CB0R_ARRAY || node.length < 2) return false;
    cb0r_s item;
    if (!cb0r_get(&node, 0, &item) || item.type != CB0R_INT) return false;
    timestamp = item.value;
    if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT) return false;
    uint8_t type = item.value;
    cb0r_s payload;
    if (!cb0r_get(&node, 2, &payload)) payload.type = CB0R_ARRAY;

    if (type == static_cast<uint8_t>(SequenceEventType::NoteEvent))
    {
        SequenceEventNote note{};
        if (payload.type == CB0R_ARRAY)
        {
            cb0r_s d;
            if (cb0r_get(&payload, 0, &d) && d.type == CB0R_INT) note.note = d.value;
            if (cb0r_get(&payload, 1, &d) && d.type == CB0R_INT) note.velocity = d.value;
            if (cb0r_get(&payload, 2, &d) && d.type == CB0R_INT) note.length = d.value;
            if (cb0r_get(&payload, 3, &d))
            {
                note.aftertouch = (d.type == CB0R_TRUE) || (d.type == CB0R_INT && d.value != 0);
            }
        }
        outEv = SequenceEvent{SequenceEventType::NoteEvent, note};
    }
    else if (type == static_cast<uint8_t>(SequenceEventType::ControlChangeEvent))
    {
        SequenceEventCC cc{};
        if (payload.type == CB0R_ARRAY)
        {
            cb0r_s d;
            if (cb0r_get(&payload, 0, &d) && d.type == CB0R_INT) cc.param = d.value;
            if (cb0r_get(&payload, 1, &d) && d.type == CB0R_INT) cc.value = d.value;
        }
        outEv = SequenceEvent{SequenceEventType::ControlChangeEvent, cc};
    }
    else
    {
        outEv = SequenceEvent{SequenceEventType::Invalid, SequenceEventNote{}};
    }
    return true;
}

static bool ParsePattern(cb0r_s node, SequencePattern& pat, uint8_t version)
{
    (void)version;
    if (node.type != CB0R_ARRAY || node.length == 0) return false;
    cb0r_s item;
    if (cb0r_get(&node, 0, &item) && item.type == CB0R_INT) pat.steps = item.value;
    if (cb0r_get(&node, 1, &item) && item.type == CB0R_ARRAY)
    {
        pat.events.clear();
        for (size_t i = 0; i < item.length; i++)
        {
            cb0r_s evNode;
            if (!cb0r_get(&item, i, &evNode)) continue;
            SequenceEvent e(SequenceEventType::Invalid, SequenceEventNote{});
            uint16_t timestamp = 0;
            if (ParseEvent(evNode, e, timestamp, version))
            {
                pat.events.insert({timestamp, e});
            }
        }
    }
    return true;
}

static bool ParseClip(cb0r_s node, SequenceClip& clip, uint8_t version)
{
    (void)version;
    clip.enabled = true;
    clip.patterns.clear();
    if (node.type != CB0R_ARRAY || node.length < 1) return false;
    cb0r_s item;
    if (cb0r_get(&node, 0, &item))
    {
        if (item.type == CB0R_TRUE) clip.enabled = true;
        else if (item.type == CB0R_FALSE) clip.enabled = false;
        else if (item.type == CB0R_INT) clip.enabled = (item.value != 0);
    }
    if (cb0r_get(&node, 1, &item) && item.type == CB0R_ARRAY)
    {
        for (size_t i = 0; i < item.length; i++)
        {
            cb0r_s patNode;
            if (!cb0r_get(&item, i, &patNode)) continue;
            SequencePattern pattern;
                ParsePattern(patNode, pattern, version);
            clip.patterns.push_back(pattern);
        }
    }
    return true;
}

static bool ParseTrack(cb0r_s node, SequenceTrack& track, uint8_t version)
{
    (void)version;
    if (node.type != CB0R_ARRAY || node.length < 2) return false;
    cb0r_s item;
    if (cb0r_get(&node, 0, &item) && item.type == CB0R_INT) track.channel = item.value;
    if (cb0r_get(&node, 1, &item) && item.type == CB0R_INT) track.activeClip = item.value;
    track.clips.clear();
    if (cb0r_get(&node, 2, &item) && item.type == CB0R_ARRAY)
    {
        for (size_t i = 0; i < item.length; i++)
        {
            cb0r_s entry;
            if (!cb0r_get(&item, i, &entry) || entry.type != CB0R_ARRAY || entry.length < 2) continue;
            cb0r_s clipIdNode;
            cb0r_s clipNode;
            if (!cb0r_get(&entry, 0, &clipIdNode) || clipIdNode.type != CB0R_INT) continue;
            if (!cb0r_get(&entry, 1, &clipNode)) continue;
            SequenceClip clip;
                ParseClip(clipNode, clip, version);
            track.clips[(uint8_t)clipIdNode.value] = clip;
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
    else return false;
    if (out.version < 2 || out.version > SEQUENCE_VERSION) return false;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bpm", &item)) out.bpm = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"swing", &item)) out.swing = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 10, (uint8_t*)"patternLen", &item)) out.patternLength = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"beats", &item)) out.beatsPerBar = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 8, (uint8_t*)"beatUnit", &item)) out.beatUnit = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"solo", &item)) out.solo = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"mute", &item)) out.mute = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"rec", &item)) out.record = item.value;

    if (cb0r_find(&root, CB0R_UTF8, 6, (uint8_t*)"tracks", &item) && item.type == CB0R_ARRAY)
    {
        out.tracks.clear();
        out.tracks.reserve(item.length);
        cb0r_s trackNode = item;
        for (size_t i = 0; i < item.length; i++)
        {
            if (!cb0r_get(&item, i, &trackNode)) continue;
            if (trackNode.type != CB0R_ARRAY) continue;
            SequenceTrack t;
            if (ParseTrack(trackNode, t, out.version))
            {
                out.tracks.push_back(t);
            }
        }
    }
    return true;
}
