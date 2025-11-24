#include "SequenceMeta.h"
#include "cb0r.h"
#include "cb0rHelper.h"
#include <string>
#include <cstring>

using std::vector;
using std::string;

void SequenceMeta::New(uint8_t tracks)
{
    const Color colors[8]
    {
        Color(0x00FFFF),
        Color(0x0000FF),
        Color(0x8000FF),
        Color(0xFF00FF),
        Color(0xFF0080),
        Color(0xFF4000),
        Color(0xFFFF00),
        Color(0x00FF40)
    };

    this->tracks.reserve(tracks);
    constexpr float hueStep = 1.0f / 16.0f;
    uint8_t hueIndex = MatrixOS::SYS::Millis() % 16;
    float hue = hueIndex * hueStep;
    color = Color::HsvToRgb(hue, 1.0f, 1.0f);
    clockOutput = false;
    for(uint8_t i = 0; i < tracks; i++)
    {
        SequenceMetaTrack track;
        track.color = colors[i];
        track.mode = SequenceTrackMode::NoteTrack;
        track.config.note.type = SequenceNoteType::Scale;
        track.config.note.customScale = false;
        track.config.note.enforceScale = true;
        track.config.note.scale = (uint16_t)Scale::MINOR;
        track.config.note.root = 0;
        track.config.note.rootOffset = 0;
        track.config.note.octave = 3;
        this->tracks.push_back(track);
    }
}

bool SerializeSequenceMeta(const SequenceMeta& meta, File& file)
{
    vector<uint8_t> buffer;

    // Header map (no track count stored; we stream until EOF)
    buffer.clear();
    cb_write_uint(buffer, CB0R_MAP, 2);
    cb_write_text(buffer, "color"); cb_write_uint(buffer, CB0R_INT, meta.color.RGB());
    cb_write_text(buffer, "clock"); cb_write_bool(buffer, meta.clockOutput);
    MLOGD("SequenceMeta", "Serialize header color=0x%06X clock=%d tracks=%u", meta.color.RGB(), meta.clockOutput ? 1 : 0, (unsigned)meta.tracks.size());
    if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;

    // Track items
    for (size_t i = 0; i < meta.tracks.size(); ++i)
    {
        const SequenceMetaTrack& t = meta.tracks[i];
        buffer.clear();
        // ["t", color, vel, mode, cfg]
        cb_write_uint(buffer, CB0R_ARRAY, 5);
        cb_write_text(buffer, "t");
        cb_write_uint(buffer, CB0R_INT, t.color.RGB());
        cb_write_bool(buffer, t.velocitySensitive);
        cb_write_uint(buffer, CB0R_INT, static_cast<uint8_t>(t.mode));
        if (t.mode == SequenceTrackMode::NoteTrack || t.mode == SequenceTrackMode::DrumTrack)
        {
            const auto& n = t.config.note;
            cb_write_uint(buffer, CB0R_MAP, 7);
            cb_write_text(buffer, "type"); cb_write_uint(buffer, CB0R_INT, static_cast<uint8_t>(n.type));
            cb_write_text(buffer, "cscale"); cb_write_bool(buffer, n.customScale);
            cb_write_text(buffer, "enforce"); cb_write_bool(buffer, n.enforceScale);
            cb_write_text(buffer, "scale"); cb_write_uint(buffer, CB0R_INT, n.scale);
            cb_write_text(buffer, "root"); cb_write_uint(buffer, CB0R_INT, n.root);
            cb_write_text(buffer, "offset"); cb_write_uint(buffer, CB0R_INT, n.rootOffset);
            cb_write_text(buffer, "oct"); cb_write_uint(buffer, CB0R_INT, n.octave);
            MLOGD("SequenceMeta", "Serialize track %u mode=note color=0x%06X vel=%d type=%u scale=%u root=%u off=%u oct=%u", (unsigned)i, t.color.RGB(), t.velocitySensitive ? 1 : 0, (unsigned)n.type, (unsigned)n.scale, (unsigned)n.root, (unsigned)n.rootOffset, (unsigned)n.octave);
        }
        else // CC track
        {
            const auto& c = t.config.cc;
            cb_write_uint(buffer, CB0R_MAP, 1);
            cb_write_text(buffer, "param"); cb_write_uint(buffer, CB0R_INT, c.parameter);
            MLOGD("SequenceMeta", "Serialize track %u mode=cc color=0x%06X vel=%d param=%u", (unsigned)i, t.color.RGB(), t.velocitySensitive ? 1 : 0, (unsigned)c.parameter);
        }
        if (file.Write(buffer.data(), buffer.size()) != buffer.size()) return false;
    }
    return true;
}

// --- Deserialization ---
static bool ParseTrackMeta(cb0r_s m, SequenceMetaTrack& t)
{
    cb0r_s item;
    // ["t", color, vel, mode, cfg]
    if (m.type != CB0R_ARRAY || m.length < 5) return false;
    if (!cb0r_get(&m, 1, &item) || item.type != CB0R_INT) return false;
    t.color = Color(item.value);
    if (!cb0r_get(&m, 2, &item)) return false;
    t.velocitySensitive = (item.type == CB0R_TRUE) || (item.type == CB0R_INT && item.value != 0);
    if (!cb0r_get(&m, 3, &item) || item.type != CB0R_INT) return false;
    t.mode = static_cast<SequenceTrackMode>(item.value);
    if (!cb0r_get(&m, 4, &item) || item.type != CB0R_MAP) return false;

    if (t.mode == SequenceTrackMode::NoteTrack || t.mode == SequenceTrackMode::DrumTrack)
    {
        cb0r_s d;
        auto& n = t.config.note;
        if (cb0r_find(&item, CB0R_UTF8, 4, (uint8_t*)"type", &d)) n.type = static_cast<SequenceNoteType>(d.value);
        if (cb0r_find(&item, CB0R_UTF8, 6, (uint8_t*)"cscale", &d)) n.customScale = (d.type == CB0R_TRUE);
        if (cb0r_find(&item, CB0R_UTF8, 7, (uint8_t*)"enforce", &d)) n.enforceScale = (d.type == CB0R_TRUE);
        if (cb0r_find(&item, CB0R_UTF8, 5, (uint8_t*)"scale", &d)) n.scale = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 4, (uint8_t*)"root", &d)) n.root = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 6, (uint8_t*)"offset", &d)) n.rootOffset = d.value;
        if (cb0r_find(&item, CB0R_UTF8, 3, (uint8_t*)"oct", &d)) n.octave = d.value;
    }
    else
    {
        cb0r_s d;
        auto& c = t.config.cc;
        if (cb0r_find(&item, CB0R_UTF8, 5, (uint8_t*)"param", &d)) c.parameter = d.value;
    }
    return true;
}

bool DeserializeSequenceMeta(File& file, SequenceMeta& out)
{
    const size_t maxSize = 2048; // meta is small; keep under 2KB
    vector<uint8_t> buffer;
    buffer.reserve(512);

    // Load entire file (bounded)
    size_t fileSize = file.Size();
    if (fileSize == 0)
    {
        MLOGE("SequenceMeta", "Empty meta file");
        return false;
    }
    if (fileSize > maxSize)
    {
        MLOGE("SequenceMeta", "Meta file too large (%u bytes)", (unsigned)fileSize);
        return false;
    }

    buffer.resize(fileSize);
    size_t readBytes = file.Read(buffer.data(), buffer.size());
    if (readBytes != buffer.size())
    {
        MLOGE("SequenceMeta", "Meta read failed size=%u read=%u", (unsigned)buffer.size(), (unsigned)readBytes);
        return false;
    }
    MLOGD("SequenceMeta", "Meta file size=%u", (unsigned)buffer.size());

    size_t offset = 0;
    cb0r_s root{};

    // Header
    if (!cb0r_read(buffer.data() + offset, buffer.size() - offset, &root)) { MLOGE("SequenceMeta", "Header parse failed"); return false; }
    size_t consumed = (size_t)(root.end - (buffer.data() + offset));
    if (root.type != CB0R_MAP || consumed == 0 || offset + consumed > buffer.size()) { MLOGE("SequenceMeta", "Header invalid"); return false; }
    cb0r_s item;
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"color", &item)) out.color = Color(item.value);
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"clock", &item)) out.clockOutput = (item.type == CB0R_TRUE) || (item.type == CB0R_INT && item.value != 0);
    MLOGD("SequenceMeta", "Header parsed color=0x%06X", out.color.RGB());
    out.tracks.clear();
    offset += consumed;

    // Tracks
    uint8_t trackId = 0;
    while (offset < buffer.size())
    {
        if (!cb0r_read(buffer.data() + offset, buffer.size() - offset, &root)) { MLOGE("SequenceMeta", "Track %u parse failed", trackId); return false; }
        consumed = (size_t)(root.end - (buffer.data() + offset));
        if (consumed == 0 || offset + consumed > buffer.size()) { MLOGE("SequenceMeta", "Track %u consumed invalid", trackId); return false; }

        if (root.type != CB0R_ARRAY || root.length < 1)
        {
            char preview[128] = {0};
            size_t previewLen = (buffer.size() - offset < 16) ? buffer.size() - offset : 16;
            for (size_t i = 0; i < previewLen; ++i)
            {
                std::snprintf(preview + i * 3, sizeof(preview) - i * 3, "%02X ", buffer[offset + i]);
            }
            MLOGE("SequenceMeta", "Item not array type=%u len=%u (trackId=%u) preview=%s", root.type, (unsigned)root.length, trackId, preview);
            return false;
        }

        cb0r_s tagNode;
        if (!cb0r_get(&root, 0, &tagNode) || tagNode.type != CB0R_UTF8) { MLOGE("SequenceMeta", "Tag missing/invalid (track %u)", trackId); return false; }
        string tag((char*)(tagNode.start + tagNode.header), tagNode.length);
        if (tag != "t") { MLOGE("SequenceMeta", "Unknown tag %s (trackId=%u)", tag.c_str(), trackId); return false; }

        SequenceMetaTrack mt{};
        if (!ParseTrackMeta(root, mt)) { MLOGE("SequenceMeta", "ParseTrackMeta failed at %u", trackId); return false; }
        if (trackId >= out.tracks.size()) out.tracks.resize(trackId + 1);
        out.tracks[trackId] = mt;
        MLOGD("SequenceMeta", "Parsed track %u", trackId);
        MLOGD("SequenceMeta", "Track %u color=0x%06X vel=%d mode=%u", trackId, mt.color.RGB(), mt.velocitySensitive ? 1 : 0, (unsigned)mt.mode);
        if (mt.mode == SequenceTrackMode::NoteTrack || mt.mode == SequenceTrackMode::DrumTrack)
        {
            const auto& n = mt.config.note;
            MLOGD("SequenceMeta", "Track %u note cfg type=%u cscale=%d enforce=%d scale=%u root=%u offset=%u oct=%u",
                  trackId, (unsigned)n.type, n.customScale ? 1 : 0, n.enforceScale ? 1 : 0,
                  (unsigned)n.scale, (unsigned)n.root, (unsigned)n.rootOffset, (unsigned)n.octave);
        }
        else
        {
            const auto& c = mt.config.cc;
            MLOGD("SequenceMeta", "Track %u cc param=%u", trackId, (unsigned)c.parameter);
        }

        offset += consumed;
        trackId++;
    }

    if (trackId == 0)
    {
        MLOGE("SequenceMeta", "No tracks parsed");
        return false;
    }

    return true;
}
