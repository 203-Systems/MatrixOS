#include "Arpeggiator.h"
#include <algorithm>
#include <cstdlib>

Arpeggiator::Arpeggiator(ArpeggiatorConfig* cfg) : config(cfg) {
    CalculateStepDurations();
}

void Arpeggiator::Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) {
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

    // Track if notePool was empty at the start of this tick
    bool wasEmpty = notePool.empty();

    // Process input packets - always track notes even when division is OFF
    for (const MidiPacket& packet : input) {
        if (packet.status == NoteOn || packet.status == NoteOff || packet.status == AfterTouch) {
            if (packet.status == NoteOn && packet.Velocity() > 0) {
                ProcessNoteOn(packet, output);
            } else if (packet.status == AfterTouch) {
                ProcessAfterTouch(packet, output);
            } else {
                ProcessNoteOff(packet, output);
            }
        }

        // Pass through all packets when division is OFF
        if (division == DIV_OFF) {
            output.push_back(packet);
        } else if (packet.status != NoteOn && packet.status != NoteOff && packet.status != AfterTouch) {
            output.push_back(packet);
        }
    }

    // Only do arpeggiator logic when division is not OFF
    if (division != DIV_OFF) {
        // Check for gate off timing - process from front of queue
        while (!gateOffQueue.empty() && gateOffQueue.front().gateOffTime <= currentTime) {
            const GateOffEvent& event = gateOffQueue.front();
            output.push_back(MidiPacket::NoteOff(event.channel, event.note, 0));
            gateOffQueue.pop_front();
        }

        // Step arpeggiator if it's time (using swing timing)
        if (!notePool.empty() && (currentTime - lastStepTime) >= stepDuration[currentIndex % 2]) {
            StepArpeggiator(output);
            lastStepTime = currentTime;
        }

        // If notePool was empty but now has notes, force start
        if (wasEmpty && !notePool.empty()) {
            currentIndex = 0;
            StepArpeggiator(output);
            lastStepTime = currentTime;
        }

        // If notePool becomes empty, turn off any sustained notes (for gate=0 mode)
        if (!wasEmpty && notePool.empty()) {
            for (const auto& event : gateOffQueue) {
                output.push_back(MidiPacket::NoteOff(event.channel, event.note, 0)); // Turn off all sustained notes
            }
            gateOffQueue.clear(); // Clear all gate timers
        }
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
    }
}

void Arpeggiator::ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();

    // Remove from note pool
    notePool.erase(std::remove_if(notePool.begin(), notePool.end(),
        [note](const ArpNote& arpNote) { return arpNote.note == note; }),
        notePool.end());

    // If this note is being sustained by the arpeggiator, turn it off immediately
    auto it = std::find_if(gateOffQueue.begin(), gateOffQueue.end(),
        [note](const GateOffEvent& event) { return event.note == note; });
    if (it != gateOffQueue.end()) {
        output.push_back(MidiPacket::NoteOff(it->channel, note, 0));
        gateOffQueue.erase(it);
    }

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

    // Calculate gate off time based on gate percentage and current step duration
    if (config->gateTime == 0) {
        // Gate time 0 = always on until arp stops (don't add to map)
        // Note will be turned off when arp stops or note is released
    } else {
        // Calculate gate duration as percentage of step duration
        uint32_t currentStepDuration = stepDuration[currentIndex % 2];
        uint32_t gateDuration = (currentStepDuration * config->gateTime) / 100;
        uint64_t gateOffTime = MatrixOS::SYS::Micros() + gateDuration;

        // Add this note to the gate off queue
        gateOffQueue.push_back({gateOffTime, currentNote.note, currentNote.channel});
    }

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
    gateOffQueue.clear(); // Clear all gate timers
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
    ArpDivision oldDivision = division;
    division = div;
    CalculateStepDurations();

    // If turning on arpeggiator with notes already held, start immediately
    if (oldDivision == DIV_OFF && div != DIV_OFF && !notePool.empty()) {
        currentIndex = 0;
        lastStepTime = MatrixOS::SYS::Micros();
        // Note: We can't call StepArpeggiator here since we don't have output queue
        // The force start will happen on the next Tick()
    }
}