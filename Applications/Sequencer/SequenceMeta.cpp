#include "SequenceMeta.h"
#include "cb0r.h"
#include "cb0rHelper.h"
#include <string>

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

static void serialize_track_meta(vector<uint8_t>& out, const SequenceMetaTrack& t)
{
    // map with 4 keys: color, vel, mode, note/cc
    cb_write_uint(out, CB0R_MAP, 4);
    cb_write_text(out, "color"); cb_write_uint(out, CB0R_INT, t.color.RGB());
    cb_write_text(out, "vel"); cb_write_bool(out, t.velocitySensitive);
    cb_write_text(out, "mode"); cb_write_uint(out, CB0R_INT, static_cast<uint8_t>(t.mode));
    cb_write_text(out, "cfg");
    if (t.mode == SequenceTrackMode::NoteTrack || t.mode == SequenceTrackMode::DrumTrack)
    {
        const auto& n = t.config.note;
        cb_write_uint(out, CB0R_MAP, 7);
        cb_write_text(out, "type"); cb_write_uint(out, CB0R_INT, static_cast<uint8_t>(n.type));
        cb_write_text(out, "cscale"); cb_write_bool(out, n.customScale);
        cb_write_text(out, "enforce"); cb_write_bool(out, n.enforceScale);
        cb_write_text(out, "scale"); cb_write_uint(out, CB0R_INT, n.scale);
        cb_write_text(out, "root"); cb_write_uint(out, CB0R_INT, n.root);
        cb_write_text(out, "offset"); cb_write_uint(out, CB0R_INT, n.rootOffset);
        cb_write_text(out, "oct"); cb_write_uint(out, CB0R_INT, n.octave);
    }
    else // CC track
    {
        const auto& c = t.config.cc;
        cb_write_uint(out, CB0R_MAP, 1);
        cb_write_text(out, "param"); cb_write_uint(out, CB0R_INT, c.parameter);
    }
}

bool SerializeSequenceMeta(const SequenceMeta& meta, std::vector<uint8_t>& out)
{
    out.clear();
    // map with 4 keys: name,color,clock,trks
    cb_write_uint(out, CB0R_MAP, 4);
    cb_write_text(out, "name"); cb_write_text(out, meta.name);
    cb_write_text(out, "color"); cb_write_uint(out, CB0R_INT, meta.color.RGB());
    cb_write_text(out, "clock"); cb_write_bool(out, meta.clockOutput);
    cb_write_text(out, "trks");
    cb_write_uint(out, CB0R_ARRAY, meta.tracks.size());
    for (const auto& t : meta.tracks)
    {
        serialize_track_meta(out, t);
    }
    return true;
}

static bool parse_track_meta(cb0r_s m, SequenceMetaTrack& t)
{
    cb0r_s item;
    if (cb0r_find(&m, CB0R_UTF8, 5, (uint8_t*)"color", &item)) t.color = Color(item.value);
    if (cb0r_find(&m, CB0R_UTF8, 3, (uint8_t*)"vel", &item)) t.velocitySensitive = (item.type == CB0R_TRUE);
    if (cb0r_find(&m, CB0R_UTF8, 4, (uint8_t*)"mode", &item)) t.mode = static_cast<SequenceTrackMode>(item.value);
    if (cb0r_find(&m, CB0R_UTF8, 3, (uint8_t*)"cfg", &item) && item.type == CB0R_MAP)
    {
        if (t.mode == SequenceTrackMode::NoteTrack || t.mode == SequenceTrackMode::DrumTrack)
        {
            auto& n = t.config.note;
            cb0r_s d;
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
            auto& c = t.config.cc;
            cb0r_s d;
            if (cb0r_find(&item, CB0R_UTF8, 5, (uint8_t*)"param", &d)) c.parameter = d.value;
        }
    }
    return true;
}

bool DeserializeSequenceMeta(const uint8_t* in, size_t len, SequenceMeta& out)
{
    cb0r_s root;
    if (!cb0r_read(const_cast<uint8_t*>(in), len, &root) || root.type != CB0R_MAP) return false;

    cb0r_s item;
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"name", &item) && item.type == CB0R_UTF8)
    {
        out.name.assign((const char*)(item.start + item.header), item.length);
    }
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"color", &item)) out.color = Color(item.value);
    if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"clock", &item)) out.clockOutput = (item.type == CB0R_TRUE);
    if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"trks", &item) && item.type == CB0R_ARRAY)
    {
        out.tracks.clear();
        out.tracks.reserve(item.length);
        cb0r_s t = item;
        for (size_t i = 0; i < item.length; i++)
        {
            if (!cb0r_get(&item, i, &t) || t.type != CB0R_MAP) continue;
            SequenceMetaTrack mt{};
            parse_track_meta(t, mt);
            out.tracks.push_back(mt);
        }
    }
    return true;
}
