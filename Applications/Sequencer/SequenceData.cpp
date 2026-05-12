#include "SequenceData.h"
#include "cb0r.h"
#include "cb0rHelper.h"
#include <string>
#include <cstring>

using std::string;
using std::vector;

static constexpr uint8_t SEQUENCE_MAX_TRACK_COUNT = 8;
static constexpr uint8_t SEQUENCE_MAX_PATTERN_LENGTH = 64;

static bool IsValidStepDivision(uint8_t stepDivision) {
  return stepDivision == 1 || stepDivision == 2 || stepDivision == 4 || stepDivision == 8 || stepDivision == 16 ||
         stepDivision == 32 || stepDivision == 64;
}

static bool ValidateSequenceData(const SequenceData& data) {
  if (data.tracks.empty() || data.tracks.size() > SEQUENCE_MAX_TRACK_COUNT)
  {
    MLOGW("SequenceData", "Invalid track count %u", (unsigned)data.tracks.size());
    return false;
  }
  if (data.bpm < 20 || data.bpm > 299 || data.swing < 20 || data.swing > 80 || data.patternLength == 0 ||
      data.patternLength > SEQUENCE_MAX_PATTERN_LENGTH || data.beatsPerBar == 0 || data.beatsPerBar > 16 ||
      !(data.beatUnit == 1 || data.beatUnit == 2 || data.beatUnit == 4 || data.beatUnit == 8 || data.beatUnit == 16) ||
      !IsValidStepDivision(data.stepDivision))
  {
    MLOGW("SequenceData", "Invalid header values bpm=%u swing=%u patternLen=%u beats=%u beatUnit=%u stepDiv=%u", data.bpm,
          data.swing, data.patternLength, data.beatsPerBar, data.beatUnit, data.stepDivision);
    return false;
  }

  uint16_t pulsesPerStep = (96 * 4) / data.stepDivision;
  for (size_t trackIdx = 0; trackIdx < data.tracks.size(); trackIdx++)
  {
    const SequenceTrack& track = data.tracks[trackIdx];
    if (track.channel > 15 || track.clips.empty())
    {
      MLOGW("SequenceData", "Invalid track %u channel=%u clips=%u", (unsigned)trackIdx, track.channel, (unsigned)track.clips.size());
      return false;
    }

    for (const auto& [clipId, clip] : track.clips)
    {
      if (clipId > 127 || clip.patterns.empty() || clip.patterns.size() > SEQUENCE_MAX_PATTERN_COUNT)
      {
        MLOGW("SequenceData", "Invalid clip t=%u c=%u patterns=%u", (unsigned)trackIdx, clipId, (unsigned)clip.patterns.size());
        return false;
      }

      for (size_t patternIdx = 0; patternIdx < clip.patterns.size(); patternIdx++)
      {
        const SequencePattern& pattern = clip.patterns[patternIdx];
        if (pattern.steps == 0 || pattern.steps > SEQUENCE_MAX_PATTERN_LENGTH)
        {
          MLOGW("SequenceData", "Invalid pattern length t=%u c=%u p=%u steps=%u", (unsigned)trackIdx, clipId,
                (unsigned)patternIdx, pattern.steps);
          return false;
        }

        uint32_t maxPulse = pattern.steps * pulsesPerStep;
        for (const auto& [timestamp, event] : pattern.events)
        {
          if (timestamp >= maxPulse)
          {
            MLOGW("SequenceData", "Invalid event timestamp t=%u c=%u p=%u ts=%u max=%u", (unsigned)trackIdx, clipId,
                  (unsigned)patternIdx, timestamp, (unsigned)maxPulse);
            return false;
          }
          if (event.eventType != SequenceEventType::NoteEvent && event.eventType != SequenceEventType::ControlChangeEvent &&
              event.eventType != SequenceEventType::Invalid)
          {
            MLOGW("SequenceData", "Invalid event type t=%u c=%u p=%u type=%u", (unsigned)trackIdx, clipId, (unsigned)patternIdx,
                  (unsigned)event.eventType);
            return false;
          }
        }
      }
    }
  }

  return true;
}

void SequencePattern::Clear() {
  events.clear();
}

void SequencePattern::ClearStepEvents(uint8_t step, uint16_t pulsesPerStep) {
  uint16_t startTime = step * pulsesPerStep;
  uint16_t endTime = startTime + pulsesPerStep - 1;
  auto it = events.lower_bound(startTime);
  while (it != events.end() && it->first <= endTime)
  {
    it = events.erase(it);
  }
}

// --- Serialization helpers ---
static void SerializeEvent(vector<uint8_t>& out, const std::pair<uint16_t, SequenceEvent>& evPair) {
  const SequenceEvent& ev = evPair.second;
  cb_write_uint(out, CB0R_ARRAY, 3);
  cb_write_uint(out, CB0R_INT, evPair.first);                       // timestamp
  cb_write_uint(out, CB0R_INT, static_cast<uint8_t>(ev.eventType)); // type

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

bool SerializeSequenceData(const SequenceData& data, File& file) {
  std::vector<uint8_t> buffer;

  // Header item (map)
  buffer.clear();
  cb_write_uint(buffer, CB0R_MAP, 10);
  cb_write_text(buffer, "ver");
  cb_write_uint(buffer, CB0R_INT, data.version);
  cb_write_text(buffer, "bpm");
  cb_write_uint(buffer, CB0R_INT, data.bpm);
  cb_write_text(buffer, "swing");
  cb_write_uint(buffer, CB0R_INT, data.swing);
  cb_write_text(buffer, "patternLen");
  cb_write_uint(buffer, CB0R_INT, data.patternLength);
  cb_write_text(buffer, "beats");
  cb_write_uint(buffer, CB0R_INT, data.beatsPerBar);
  cb_write_text(buffer, "beatUnit");
  cb_write_uint(buffer, CB0R_INT, data.beatUnit);
  cb_write_text(buffer, "stepDiv");
  cb_write_uint(buffer, CB0R_INT, data.stepDivision);
  cb_write_text(buffer, "solo");
  cb_write_uint(buffer, CB0R_INT, data.solo);
  cb_write_text(buffer, "mute");
  cb_write_uint(buffer, CB0R_INT, data.mute);
  cb_write_text(buffer, "rec");
  cb_write_uint(buffer, CB0R_INT, data.record);
  if (file.Write(buffer.data(), buffer.size()) != buffer.size())
    return false;

  uint8_t trackId = 0;
  for (const auto& track : data.tracks)
  {
    MLOGD("SequenceData", "Serializing track %u ch=%u activeClip=%u clips=%zu", trackId, track.channel, track.activeClip,
          track.clips.size());
    // Track item: ["t", channel, activeClip]
    buffer.clear();
    cb_write_uint(buffer, CB0R_ARRAY, 3);
    cb_write_text(buffer, "t");
    cb_write_uint(buffer, CB0R_INT, track.channel);
    cb_write_uint(buffer, CB0R_INT, track.activeClip);
    if (file.Write(buffer.data(), buffer.size()) != buffer.size())
      return false;

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
      if (file.Write(buffer.data(), buffer.size()) != buffer.size())
        return false;

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
        MLOGD("SequenceData", "Serializing pattern t=%u c=%u p=%zu steps=%u events=%zu", trackId, clipId, patIdx, pat.steps,
              pat.events.size());
        for (const auto& ev : pat.events)
        {
          SerializeEvent(buffer, ev);
        }
        if (file.Write(buffer.data(), buffer.size()) != buffer.size())
          return false;
      }
    }
    trackId++;
  }

  return true;
}

// --- Deserialization helpers ---
static bool ParseHeader(cb0r_s root, SequenceData& out) {
  cb0r_s item;
  if (!cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"ver", &item))
  {
    MLOGW("SequenceData", "Header missing 'ver'");
    return false;
  }
  uint8_t fileVersion = item.value;
  out.version = fileVersion;
  if (fileVersion < MIN_SUPPORTED_SEQUENCE_VERSION || fileVersion > SEQUENCE_VERSION)
  {
    MLOGW("SequenceData", "Unsupported version %u", fileVersion);
    return false;
  }
  if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"bpm", &item) && item.type == CB0R_INT)
    out.bpm = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"swing", &item) && item.type == CB0R_INT)
    out.swing = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 10, (uint8_t*)"patternLen", &item) && item.type == CB0R_INT)
    out.patternLength = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 5, (uint8_t*)"beats", &item) && item.type == CB0R_INT)
    out.beatsPerBar = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 8, (uint8_t*)"beatUnit", &item) && item.type == CB0R_INT)
    out.beatUnit = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 7, (uint8_t*)"stepDiv", &item) && item.type == CB0R_INT)
    out.stepDivision = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"solo", &item) && item.type == CB0R_INT)
    out.solo = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 4, (uint8_t*)"mute", &item) && item.type == CB0R_INT)
    out.mute = item.value;
  if (cb0r_find(&root, CB0R_UTF8, 3, (uint8_t*)"rec", &item) && item.type == CB0R_INT)
    out.record = item.value;

  // Sequencer Data Migration
  if (fileVersion == 3)
  {
    uint16_t migratedStepDivision = static_cast<uint16_t>(out.stepDivision) * 4;
    if (migratedStepDivision > UINT8_MAX)
    {
      MLOGW("SequenceData", "V3 stepDiv migration overflow stepDiv=%u", out.stepDivision);
      return false;
    }
    out.stepDivision = static_cast<uint8_t>(migratedStepDivision);
    out.version = SEQUENCE_VERSION;
    MLOGD("SequenceData", "Migrated v3 stepDiv to %u", out.stepDivision);
  }
  out.tracks.clear();
  return true;
}

static bool ParseTrack(cb0r_s node, uint8_t& channel, uint8_t& activeClip) {
  if (node.type != CB0R_ARRAY || node.length < 3)
  {
    MLOGW("SequenceData", "Track parse failed - invalid node");
    return false;
  }
  cb0r_s item;
  // ["t", channel, activeClip]
  if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Track parse failed - channel missing");
    return false;
  }
  if (item.value > 15)
    return false;
  channel = item.value;
  if (!cb0r_get(&node, 2, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Track parse failed - activeClip missing");
    return false;
  }
  if (item.value > 127)
    return false;
  activeClip = item.value;
  MLOGD("SequenceData", "Parsed Track ch=%u activeClip=%u", channel, activeClip);
  return true;
}

static bool ParseClip(cb0r_s node, uint8_t& clipId) {
  // Accept legacy 3-element clips but only require 2 (["c", clipId, <enabled>])
  if (node.type != CB0R_ARRAY || node.length < 2)
  {
    MLOGW("SequenceData", "Clip parse failed - invalid node");
    return false;
  }
  cb0r_s item;
  // ["c", clipId]
  if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Clip parse failed - id missing");
    return false;
  }
  if (item.value > 127)
    return false;
  clipId = item.value;
  MLOGD("SequenceData", "Parsed Clip id=%u", clipId);
  return true;
}

// --- Deserialization helpers ---
static bool ParseEvent(cb0r_s node, SequenceEvent& outEv, uint16_t& timestamp, uint8_t version) {
  (void)version;
  if (node.type != CB0R_ARRAY || node.length < 2)
  {
    MLOGW("SequenceData", "Event parse failed - invalid node");
    return false;
  }
  cb0r_s item;
  if (!cb0r_get(&node, 0, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Event parse failed - timestamp missing");
    return false;
  }
  if (item.value > UINT16_MAX)
    return false;
  timestamp = item.value;
  if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Event parse failed - type missing");
    return false;
  }
  uint8_t type = item.value;
  cb0r_s payload;
  if (!cb0r_get(&node, 2, &payload))
    payload.type = CB0R_ARRAY;

  if (type == static_cast<uint8_t>(SequenceEventType::NoteEvent))
  {
    SequenceEventNote note{};
    if (payload.type == CB0R_ARRAY)
    {
      cb0r_s d;
      if (cb0r_get(&payload, 0, &d) && d.type == CB0R_INT)
        note.note = d.value > 127 ? 127 : d.value;
      if (cb0r_get(&payload, 1, &d) && d.type == CB0R_INT)
        note.velocity = d.value > 127 ? 127 : d.value;
      if (cb0r_get(&payload, 2, &d) && d.type == CB0R_INT)
        note.length = d.value > UINT16_MAX ? UINT16_MAX : d.value;
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
      if (cb0r_get(&payload, 0, &d) && d.type == CB0R_INT)
        cc.param = d.value > 127 ? 127 : d.value;
      if (cb0r_get(&payload, 1, &d) && d.type == CB0R_INT)
        cc.value = d.value > 127 ? 127 : d.value;
    }
    outEv = SequenceEvent{SequenceEventType::ControlChangeEvent, cc};
  }
  else
  {
    outEv = SequenceEvent{SequenceEventType::Invalid, SequenceEventNote{}};
  }
  return true;
}

static bool ParsePattern(cb0r_s node, uint8_t trackId, uint8_t clipId, uint8_t patternId, SequenceData& data) {
  if (node.type != CB0R_ARRAY || node.length < 3)
  {
    MLOGW("SequenceData", "Pattern parse failed - invalid node");
    return false;
  }
  cb0r_s item;
  // ["p", steps, events[]]
  if (!cb0r_get(&node, 1, &item) || item.type != CB0R_INT)
  {
    MLOGW("SequenceData", "Pattern parse failed - steps missing");
    return false;
  }
  if (item.value == 0 || item.value > SEQUENCE_MAX_PATTERN_LENGTH)
  {
    MLOGW("SequenceData", "Pattern parse failed - invalid steps %u", (unsigned)item.value);
    return false;
  }
  uint8_t steps = item.value;
  if (!cb0r_get(&node, 2, &item) || item.type != CB0R_ARRAY)
  {
    MLOGW("SequenceData", "Pattern parse failed - events array missing");
    return false;
  }

  if (trackId >= data.tracks.size())
    data.tracks.resize(trackId + 1);
  SequenceTrack& track = data.tracks[trackId];
  SequenceClip& clip = track.clips[clipId];
  if (clip.patterns.size() <= patternId)
    clip.patterns.resize(patternId + 1);
  SequencePattern& pat = clip.patterns[patternId];
  pat.steps = steps;
  pat.events.clear();

  for (size_t i = 0; i < item.length; i++)
  {
    cb0r_s evNode;
    if (!cb0r_get(&item, (uint32_t)i, &evNode))
      continue;
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

bool DeserializeSequenceData(File& file, SequenceData& out) {
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

        if (itemReady)
          break;

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

      MLOGD("SequenceData", "Item parsed offset=%zu consumed=%zu type=%u len=%u first=%02X", fileOffset, consumed, (unsigned)root.type,
            (unsigned)root.length, buffer.empty() ? 0 : buffer[0]);

      if (!headerParsed)
      {
        if (root.type != CB0R_MAP)
        {
          MLOGW("SequenceData", "Header parse failed - not a map");
          return false;
        }
        if (!ParseHeader(root, out))
        {
          MLOGW("SequenceData", "Header parse failed");
          return false;
        }
        MLOGD("SequenceData", "Header parsed ver=%u bpm=%u swing=%u patternLen=%u beats=%u beatUnit=%u stepDiv=%u", out.version, out.bpm,
              out.swing, out.patternLength, out.beatsPerBar, out.beatUnit, out.stepDivision);
        headerParsed = true;
        currentTrack = 0xFF;
        currentClip = 0;
        nextPattern = 0;
      }
      else
      {
        if (root.type != CB0R_ARRAY || root.length < 1)
        {
          MLOGW("SequenceData", "Item parse failed - not array (type=%u len=%u)", (unsigned)root.type, (unsigned)root.length);
          return false;
        }
        cb0r_s tagNode;
        if (!cb0r_get(&root, 0, &tagNode) || tagNode.type != CB0R_UTF8)
        {
          MLOGW("SequenceData", "Item parse failed - tag missing");
          return false;
        }
        string tag((char*)(tagNode.start + tagNode.header), tagNode.length);

        if (tag == "t")
        {
          uint8_t channel = 0;
          uint8_t activeClip = 0;
          if (!ParseTrack(root, channel, activeClip))
          {
            MLOGW("SequenceData", "Failed to parse track item");
            return false;
          }
          currentTrack++;
          currentClip = 0;
          nextPattern = 0;
          if (currentTrack >= out.tracks.size())
            out.tracks.resize(currentTrack + 1);
          out.tracks[currentTrack].channel = channel;
          out.tracks[currentTrack].activeClip = activeClip;
        }
        else if (tag == "c")
        {
          if (currentTrack == 0xFF)
          {
            MLOGW("SequenceData", "Clip before track");
            return false;
          }
          uint8_t clipId = 0;
          if (!ParseClip(root, clipId))
          {
            MLOGW("SequenceData", "Failed to parse clip item");
            return false;
          }
          currentClip = clipId;
          nextPattern = 0;
          if (currentTrack >= out.tracks.size())
            out.tracks.resize(currentTrack + 1);
        }
        else if (tag == "p")
        {
          if (currentTrack == 0xFF)
          {
            MLOGW("SequenceData", "Pattern before track");
            return false;
          }
          if (!ParsePattern(root, currentTrack, currentClip, nextPattern, out))
          {
            MLOGW("SequenceData", "Failed to parse pattern item t=%u c=%u p=%u", currentTrack, currentClip, nextPattern);
            return false;
          }
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
        return headerParsed && ValidateSequenceData(out);
      }
      break;
    }
  }

  return headerParsed && ValidateSequenceData(out);
}
