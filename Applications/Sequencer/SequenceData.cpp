#include "SequenceData.h"
#include "cb0r.h"
#include "cb0rHelper.h"
#include <string>
#include <cstring>

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
    if (src == dest)
    {
        return;
    }

    uint16_t sourceStartTime = src * pulsesPerStep;
    uint16_t destStartTime = dest * pulsesPerStep;
    CopyEventsInRange(sourceStartTime, destStartTime, pulsesPerStep);
}

// --- Serialization helpers ---
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

bool SerializeSequenceData(const SequenceData& data, File& file)
{
    std::vector<uint8_t> buffer;

    // Header item (map)
    buffer.clear();
    cb_write_uint(buffer, CB0R_MAP, 10);
    cb_write_text(buffer, "ver"); cb_write_uint(buffer, CB0R_INT, data.version);
    cb_write_text(buffer, "bpm"); cb_write_uint(buffer, CB0R_INT, data.bpm);
    cb_write_text(buffer, "swing"); cb_write_uint(buffer, CB0R_INT, data.swing);
    cb_write_text(buffer, "patternLen"); cb_write_uint(buffer, CB0R_INT, data.patternLength);
    cb_write_text(buffer, "beats"); cb_write_uint(buffer, CB0R_INT, data.beatsPerBar);
    cb_write_text(buffer, "beatUnit"); cb_write_uint(buffer, CB0R_INT, data.beatUnit);
    cb_write_text(buffer, "stepDiv"); cb_write_uint(buffer, CB0R_INT, data.stepDivision);
    cb_write_text(buffer, "solo"); cb_write_uint(buffer, CB0R_INT, data.solo);
    cb_write_text(buffer, "mute"); cb_write_uint(buffer, CB0R_INT, data.mute);
    cb_write_text(buffer, "rec"); cb_write_uint(buffer, CB0R_INT, data.record);
    if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;

    uint8_t trackId = 0;
    for (const auto& track : data.tracks)
    {
        MLOGD("SequenceData", "Serializing track %u ch=%u activeClip=%u clips=%zu", trackId, track.channel, track.activeClip, track.clips.size());
        // Track item: ["t", channel, activeClip]
        buffer.clear();
        cb_write_uint(buffer, CB0R_ARRAY, 3);
        cb_write_text(buffer, "t");
        cb_write_uint(buffer, CB0R_INT, track.channel);
        cb_write_uint(buffer, CB0R_INT, track.activeClip);
        if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;

        // Clip meta + patterns
        for (const auto& clipPair : track.clips)
        {
            uint8_t clipId = clipPair.first;
            const SequenceClip& clip = clipPair.second;

            MLOGD("SequenceData", "Serializing clip t=%u id=%u patterns=%zu", trackId, clipId, clip.patterns.size());
            // Clip item: ["c", clipId]
            buffer.clear();
            cb_write_uint(buffer, CB0R_ARRAY, 2);
            cb_write_text(buffer, "c");
            cb_write_uint(buffer, CB0R_INT, clipId);
            if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;

            // Patterns
            for (size_t patIdx = 0; patIdx < clip.patterns.size(); patIdx++)
            {
                const SequencePattern& pat = clip.patterns[patIdx];
                buffer.clear();
                // ["p", steps, events[]]
                cb_write_uint(buffer, CB0R_ARRAY, 3);
                cb_write_text(buffer, "p");
                cb_write_uint(buffer, CB0R_INT, pat.steps);
                cb_write_uint(buffer, CB0R_ARRAY, pat.events.size());
                MLOGD("SequenceData", "Serializing pattern t=%u c=%u p=%zu steps=%u events=%zu", trackId, clipId, patIdx, pat.steps, pat.events.size());
                for (const auto& ev : pat.events)
                {
                    SerializeEvent(buffer, ev);
                }
                if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;
            }
        }
        trackId++;
    }

    return true;
}

// --- Deserialization helpers ---
static bool ParseHeader(cb0r_s root, SequenceData& out)
{
    cb0r_s item;
    if (!cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"ver", &item)) { MLOGW("SequenceData", "Header missing 'ver'"); return false; }
    out.version = item.value;
    if (out.version < MIN_SUPPORTED_SEQUENCE_VERSION || out.version > SEQUENCE_VERSION) { MLOGW("SequenceData", "Unsupported version %u", out.version); return false; }
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bpm", &item)) out.bpm = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"swing", &item)) out.swing = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 10, (uint8_t*)"patternLen", &item)) out.patternLength = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"beats", &item)) out.beatsPerBar = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 8, (uint8_t*)"beatUnit", &item)) out.beatUnit = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 7, (uint8_t*)"stepDiv", &item)) out.stepDivision = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"solo", &item)) out.solo = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"mute", &item)) out.mute = item.value;
    if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"rec", &item)) out.record = item.value;
    out.tracks.clear();
    return true;
}

static bool ParseTrack(cb0r_s node, uint8_t& channel, uint8_t& activeClip)
{
    if (node.type != CB0R_ARRAY || node.length < 3) { MLOGW("SequenceData", "Track parse failed - invalid node"); return false; }
    cb0r_s item;
    // ["t", channel, activeClip]
    if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Track parse failed - channel missing"); return false; }
    channel = item.value;
    if (!cb0r_get(&node, 2, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Track parse failed - activeClip missing"); return false; }
    activeClip = item.value;
    MLOGD("SequenceData", "Parsed Track ch=%u activeClip=%u", channel, activeClip);
    return true;
}

static bool ParseClip(cb0r_s node, uint8_t& clipId)
{
    // Accept legacy 3-element clips but only require 2 (["c", clipId, <enabled>])
    if (node.type != CB0R_ARRAY || node.length < 2) { MLOGW("SequenceData", "Clip parse failed - invalid node"); return false; }
    cb0r_s item;
    // ["c", clipId]
    if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Clip parse failed - id missing"); return false; }
    clipId = item.value;
    MLOGD("SequenceData", "Parsed Clip id=%u", clipId);
    return true;
}

// --- Deserialization helpers ---
static bool ParseEvent(cb0r_s node, SequenceEvent& outEv, uint16_t& timestamp, uint8_t version)
{
    (void)version;
    if (node.type != CB0R_ARRAY || node.length < 2) { MLOGW("SequenceData", "Event parse failed - invalid node"); return false; }
    cb0r_s item;
    if (!cb0r_get(&node, 0, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Event parse failed - timestamp missing"); return false; }
    timestamp = item.value;
    if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Event parse failed - type missing"); return false; }
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

static bool ParsePattern(cb0r_s node, uint8_t trackId, uint8_t clipId, uint8_t patternId, SequenceData& data)
{
    if (node.type != CB0R_ARRAY || node.length < 3) { MLOGW("SequenceData", "Pattern parse failed - invalid node"); return false; }
    cb0r_s item;
    // ["p", steps, events[]]
    if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT) { MLOGW("SequenceData", "Pattern parse failed - steps missing"); return false; }
    uint8_t steps = item.value;
    if (!cb0r_get(&node, 2, &item) || item.type != CB0R_ARRAY) { MLOGW("SequenceData", "Pattern parse failed - events array missing"); return false; }

    if (trackId >= data.tracks.size()) data.tracks.resize(trackId + 1);
    SequenceTrack& track = data.tracks[trackId];
    SequenceClip& clip = track.clips[clipId];
    if (clip.patterns.size() <= patternId) clip.patterns.resize(patternId + 1);
    SequencePattern& pat = clip.patterns[patternId];
    pat.steps = steps;
    pat.events.clear();

    for (size_t i = 0; i < item.length; i++)
    {
        cb0r_s evNode;
        if (!cb0r_get(&item, (uint32_t)i, &evNode)) continue;
        SequenceEvent e(SequenceEventType::Invalid, SequenceEventNote{});
        uint16_t timestamp = 0;
        if (ParseEvent(evNode, e, timestamp, data.version))
        {
            pat.events.insert({timestamp, e});
        }
    }
    MLOGD("SequenceData", "Parsed Pattern t=%u c=%u p=%u steps=%u events=%zu", trackId, clipId, patternId, pat.steps, pat.events.size());
    return true;
}

bool DeserializeSequenceData(File& file, SequenceData& out)
{
    const size_t MAX_STREAMING_BUFFER = 8 * 1024;
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);
    uint8_t chunk[512];
    size_t fileOffset = 0; // bytes consumed from start of file
    bool eofReached = false;

    bool headerParsed = false;
    uint8_t currentTrack = 0xFF;
    uint8_t currentClip = 0;
    uint8_t nextPattern = 0;

    while (true)
    {
        cb0r_s root{};
        while (true)
        {
            size_t consumed = 0;
            bool itemReady = false;

            do
            {
                bool itemRead = cb0r_read(buffer.data(), buffer.size(), &root);
                consumed = itemRead ? (size_t)(root.end - buffer.data()) : 0;

                if (buffer.size() > 0 && (consumed == 0 || consumed > buffer.size()))
                {
                    MLOGW("SequenceData", "Item parse failed - consumed=%zu size=%zu", consumed, buffer.size());
                    return false;
                }

                itemReady = itemRead && (consumed < buffer.size() || eofReached);

                if (itemReady) break;

                // We need to read more
                size_t n = file.Read(chunk, 512);
 
                if (buffer.size() + n > MAX_STREAMING_BUFFER)
                {
                    MLOGE("SequenceData", "Deserialize - item too large (%zu bytes)", buffer.size() + n);
                    return false;
                }

                buffer.insert(buffer.end(), chunk, chunk + n);

                if (file.Position() >= file.Size())
                {
                    eofReached = true;
                }
            } while (!itemReady);

            MLOGD("SequenceData", "Item parsed offset=%zu consumed=%zu type=%u len=%u first=%02X",
                  fileOffset, consumed, (unsigned)root.type, (unsigned)root.length, buffer.empty() ? 0 : buffer[0]);

            if (!headerParsed)
            {
                if (root.type != CB0R_MAP) { MLOGW("SequenceData", "Header parse failed - not a map"); return false; }
                if (!ParseHeader(root, out)) { MLOGW("SequenceData", "Header parse failed"); return false; }
                MLOGD("SequenceData", "Header parsed ver=%u bpm=%u swing=%u patternLen=%u beats=%u beatUnit=%u stepDiv=%u", out.version, out.bpm, out.swing, out.patternLength, out.beatsPerBar, out.beatUnit, out.stepDivision);
                headerParsed = true;
                currentTrack = 0xFF;
                currentClip = 0;
                nextPattern = 0;
            }
            else
            {
                if (root.type != CB0R_ARRAY || root.length < 1) {
                    MLOGW("SequenceData", "Item parse failed - not array (type=%u len=%u)", (unsigned)root.type, (unsigned)root.length);
                    return false;
                }
                cb0r_s tagNode;
                if (!cb0r_get(&root, 0, &tagNode) || tagNode.type != CB0R_UTF8) { MLOGW("SequenceData", "Item parse failed - tag missing"); return false; }
                string tag((char*)(tagNode.start + tagNode.header), tagNode.length);

                if (tag == "t")
                {
                    uint8_t channel = 0;
                    uint8_t activeClip = 0;
                    if (!ParseTrack(root, channel, activeClip)) { MLOGW("SequenceData", "Failed to parse track item"); return false; }
                    currentTrack++;
                    currentClip = 0;
                    nextPattern = 0;
                    if (currentTrack >= out.tracks.size()) out.tracks.resize(currentTrack + 1);
                    out.tracks[currentTrack].channel = channel;
                    out.tracks[currentTrack].activeClip = activeClip;
                }
                else if (tag == "c")
                {
                    uint8_t clipId = 0;
                    if (!ParseClip(root, clipId)) { MLOGW("SequenceData", "Failed to parse clip item"); return false; }
                    currentClip = clipId;
                    nextPattern = 0;
                    if (currentTrack >= out.tracks.size()) out.tracks.resize(currentTrack + 1);
                }
                else if (tag == "p")
                {
                    if (!ParsePattern(root, currentTrack, currentClip, nextPattern, out)) { MLOGW("SequenceData", "Failed to parse pattern item t=%u c=%u p=%u", currentTrack, currentClip, nextPattern); return false; }
                    nextPattern++;
                }
                else
                {
                    MLOGW("SequenceData", "Unknown item tag");
                    return false;
                }
            }

            buffer.erase(buffer.begin(), buffer.begin() + consumed);
            fileOffset += consumed;

            if (buffer.empty() && eofReached)
            {
                return headerParsed;
            }
            break;
        }
    }

    return headerParsed;
}
