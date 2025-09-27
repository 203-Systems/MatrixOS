#include "Arpeggiator.h"
#include <algorithm>
#include <cstdlib>

Arpeggiator::Arpeggiator(ArpeggiatorConfig* cfg) : config(cfg) {
    CalculateStepDurations();
}

void Arpeggiator::Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) {
    if (!enabled || division == DIV_OFF) {
        output = std::move(input);
        return;
    }

    if (disableOnNextTick) {
        disableOnNextTick = false;
        enabled = false;
        Reset();

        for (const MidiPacket& packet : input) {
            output.push_back(packet);
        }
        return;
    }

    uint64_t currentTime = MatrixOS::SYS::Micros();

    // Process input packets
    for (const MidiPacket& packet : input) {
        if (packet.status == NoteOn || packet.status == NoteOff || packet.status == AfterTouch) {
            if (packet.status == NoteOn && packet.Velocity() > 0) {
                ProcessNoteOn(packet, output);
            } else if (packet.status == AfterTouch) {
                ProcessAfterTouch(packet, output);
            } else {
                ProcessNoteOff(packet, output);
            }
        } else {
            output.push_back(packet);
        }
    }

    // Step arpeggiator if it's time (using swing timing)
    if (!notePool.empty() && currentTime - lastStepTime >= stepDuration[currentIndex % 2]) {
        StepArpeggiator(output);
        lastStepTime = currentTime;
    }
}

void Arpeggiator::ProcessNoteOn(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();
    uint8_t velocity = packet.Velocity();
    uint8_t channel = packet.Channel();
    uint64_t timestamp = MatrixOS::SYS::Micros();

    // Add to note pool if not already present
    bool found = false;
    for (const ArpNote& arpNote : notePool) {
        if (arpNote.note == note) {
            found = true;
            break;
        }
    }

    if (!found) {
        ArpNote arpNote = {note, velocity, channel, (uint32_t)timestamp};
        notePool.push_back(arpNote);
        UpdateSequence();

        // If this is the first note, start arpeggiating immediately
        if (notePool.size() == 1) {
            currentIndex = 0;
            lastStepTime = timestamp;
            StepArpeggiator(output);
        }
    }
}

void Arpeggiator::ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();

    // Remove from note pool
    notePool.erase(std::remove_if(notePool.begin(), notePool.end(),
        [note](const ArpNote& arpNote) { return arpNote.note == note; }),
        notePool.end());

    UpdateSequence();

    // Reset index if we've gone beyond the sequence
    if (currentIndex >= arpSequence.size() && !arpSequence.empty()) {
        currentIndex = 0;
    }
}

void Arpeggiator::ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();
    uint8_t velocity = packet.Velocity();

    // Update velocity for the note in notePool
    for (ArpNote& arpNote : notePool) {
        if (arpNote.note == note) {
            arpNote.velocity = velocity;
            break;
        }
    }

    // Update velocity in arpSequence as well
    for (ArpNote& seqNote : arpSequence) {
        if (seqNote.note == note) {
            seqNote.velocity = velocity;
        }
    }
}

void Arpeggiator::UpdateSequence() {
    arpSequence.clear();

    if (notePool.empty()) {
        return;
    }

    // Sort notes based on direction
    vector<ArpNote> sortedNotes = notePool;

    switch (config->direction) {
        case ARP_UP:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.note < b.note; });
            break;
        case ARP_DOWN:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.note > b.note; });
            break;
        case ARP_PLAY_ORDER:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.timestamp < b.timestamp; });
            break;
        case ARP_RANDOM:
            // No sorting - will be randomized during playback
            break;
        default:
            // For other modes, sort ascending first
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.note < b.note; });
            break;
    }

    // Build sequence with octave steps and offset
    for (uint8_t stepNum = 0; stepNum < config->step; stepNum++) {
        for (const ArpNote& note : sortedNotes) {
            ArpNote stepNote = note;
            stepNote.note += (stepNum * 12) + config->stepOffset;
            if (stepNote.note < 128 && stepNote.note >= 0) {
                arpSequence.push_back(stepNote);
            }
        }
    }

    // Handle special direction modes
    if (config->direction == ARP_UP_DOWN && arpSequence.size() > 1) {
        // Add reverse without duplicating end notes
        for (int i = arpSequence.size() - 2; i > 0; i--) {
            arpSequence.push_back(arpSequence[i]);
        }
    } else if (config->direction == ARP_DOWN_UP && arpSequence.size() > 1) {
        // Reverse the entire sequence first, then add forward
        std::reverse(arpSequence.begin(), arpSequence.end());
        vector<ArpNote> temp = arpSequence;
        for (int i = temp.size() - 2; i > 0; i--) {
            arpSequence.push_back(temp[i]);
        }
    }
}

void Arpeggiator::StepArpeggiator(deque<MidiPacket>& output) {
    if (arpSequence.empty()) {
        return;
    }

    if (config->direction == ARP_RANDOM) {
        currentIndex = rand() % arpSequence.size();
    }

    // Play current note
    const ArpNote& currentNote = arpSequence[currentIndex];
    output.push_back(MidiPacket::NoteOn(currentNote.channel, currentNote.note, currentNote.velocity));

    // Advance to next note
    currentIndex = (currentIndex + 1) % arpSequence.size();
}

void Arpeggiator::CalculateStepDurations() {
    if (division == DIV_OFF || division == 0) {
        stepDuration[0] = stepDuration[1] = 1000000; // 1 second fallback in microseconds
        return;
    }

    // Calculate base duration in microseconds based on BPM and division
    uint32_t quarterNoteUs = (60 * 1000000) / config->bpm;
    uint32_t baseDuration = quarterNoteUs * 4 / division;

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (config->swingAmount - 50) / 100.0f;

    stepDuration[0] = (uint32_t)(baseDuration * (1.0f + swingRatio));  // On-beat
    stepDuration[1] = (uint32_t)(baseDuration * (1.0f - swingRatio));  // Off-beat
}

void Arpeggiator::Reset() {
    notePool.clear();
    arpSequence.clear();
    currentIndex = 0;
    lastStepTime = 0;
    disableOnNextTick = false;
}

void Arpeggiator::SetEnabled(bool state) {
    if (!state && enabled) {
        disableOnNextTick = true;
    } else {
        enabled = state;
        if (state) {
            disableOnNextTick = false;
        }
    }
}

void Arpeggiator::UpdateConfig(ArpeggiatorConfig* cfg) {
    config = cfg;
    CalculateStepDurations(); // Recalculate timings when config changes
}

void Arpeggiator::SetDivision(ArpDivision div) {
    division = div;
    CalculateStepDurations();
}