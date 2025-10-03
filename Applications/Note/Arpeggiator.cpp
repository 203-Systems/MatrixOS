#include "Arpeggiator.h"
#include <algorithm>
#include <cstdlib>

Arpeggiator::Arpeggiator(ArpeggiatorConfig* cfg) : config(cfg) {
    CalculateTicksPerStep();
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
        // Increment tick counter (Tick() is called on each clock pulse)
        tickCounter++;

        // Check for gate off timing - process from front of queue
        while (!gateOffQueue.empty() && gateOffQueue.front().gateOffTick <= tickCounter &&
               gateOffQueue.front().gateOffTick != UINT32_MAX) {
            const GateOffEvent& event = gateOffQueue.front();
            output.push_back(MidiPacket::NoteOff(event.channel, event.note, 0));
            gateOffQueue.pop_front();
        }

        // Step arpeggiator if it's time (using swing timing) and not exceeded repeat limit
        if (!notePool.empty() && tickCounter >= nextStepTick &&
            (config->repeat == 0 || currentRepeat < config->repeat)) {
            StepArpeggiator(output);
            nextStepTick = tickCounter + ticksPerStep[currentIndex % 2];
        }

        // If notePool was empty but now has notes, force start
        if (wasEmpty && !notePool.empty()) {
            currentIndex = 0;
            currentRepeat = 0;  // Reset repeat counter when starting fresh
            lastSequenceIndex = 0;
            if (config->repeat == 0 || currentRepeat < config->repeat) {
                StepArpeggiator(output);
                nextStepTick = tickCounter + ticksPerStep[currentIndex % 2];
            }
        }

        // If notePool becomes empty, turn off any sustained notes (for inf gate mode)
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

    // Add to note pool if not already present
    bool found = false;
    for (const ArpNote& arpNote : notePool) {
        if (arpNote.note == note) {
            found = true;
            break;
        }
    }

    if (!found) {
        ArpNote arpNote = {note, velocity, channel, tickCounter};
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
    // Find all instances of this note (there may be multiple with different gate times)
    auto it = gateOffQueue.begin();
    while (it != gateOffQueue.end()) {
        if (it->note == note) {
            output.push_back(MidiPacket::NoteOff(it->channel, note, 0));
            it = gateOffQueue.erase(it);
        } else {
            ++it;
        }
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

    // For RANDOM mode, just copy the pool - we'll pick randomly in StepArpeggiator
    if (config->direction == ARP_RANDOM) {
        // Build sequence with octave steps and offset for random selection
        uint8_t actualSteps = config->step == 0 ? 1 : config->step;
        for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
            for (const ArpNote& note : notePool) {
                ArpNote stepNote = note;
                stepNote.note += stepNum * config->stepOffset;
                if (stepNote.note < 128 && stepNote.note >= 0) {
                    arpSequence.push_back(stepNote);
                }
            }
        }
        return;
    }

    // Sort notes based on direction
    vector<ArpNote> sortedNotes = notePool;

    switch (config->direction) {
        case ARP_UP:
        case ARP_UP_DOWN:
        case ARP_UP_N_DOWN:
        case ARP_CONVERGE:
        case ARP_CON_DIVERGE:
        case ARP_PINKY_UP:
        case ARP_PINKY_UP_DOWN:
        case ARP_THUMB_UP:
        case ARP_THUMB_UP_DOWN:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.note < b.note; });
            break;
        case ARP_DOWN:
        case ARP_DOWN_UP:
        case ARP_DOWN_N_UP:
        case ARP_DIVERGE:
        case ARP_DIV_CONVERGE:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.note > b.note; });
            break;
        case ARP_PLAY_ORDER:
            std::sort(sortedNotes.begin(), sortedNotes.end(),
                [](const ArpNote& a, const ArpNote& b) { return a.timestamp < b.timestamp; });
            break;
    }

    // Build base sequence based on direction
    uint8_t actualSteps = config->step == 0 ? 1 : config->step;

    switch (config->direction) {
        case ARP_UP:
        case ARP_DOWN:
        case ARP_PLAY_ORDER:
            // Simple sequential patterns
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            break;

        case ARP_UP_DOWN:
            // Up then down without repeating endpoints
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            if (arpSequence.size() > 1) {
                for (int i = arpSequence.size() - 2; i > 0; i--) {
                    arpSequence.push_back(arpSequence[i]);
                }
            }
            break;

        case ARP_DOWN_UP:
            // Down then up without repeating endpoints
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            if (arpSequence.size() > 1) {
                for (int i = arpSequence.size() - 2; i > 0; i--) {
                    arpSequence.push_back(arpSequence[i]);
                }
            }
            break;

        case ARP_UP_N_DOWN:
            // Up then down WITH repeating endpoints
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            if (arpSequence.size() > 0) {
                for (int i = arpSequence.size() - 1; i >= 0; i--) {
                    arpSequence.push_back(arpSequence[i]);
                }
            }
            break;

        case ARP_DOWN_N_UP:
            // Down then up WITH repeating endpoints
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            if (arpSequence.size() > 0) {
                for (int i = arpSequence.size() - 1; i >= 0; i--) {
                    arpSequence.push_back(arpSequence[i]);
                }
            }
            break;

        case ARP_CONVERGE:
            // Play from outside to inside
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                vector<ArpNote> stepNotes;
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        stepNotes.push_back(stepNote);
                    }
                }
                // Add notes alternating from ends toward middle
                int left = 0, right = stepNotes.size() - 1;
                while (left <= right) {
                    arpSequence.push_back(stepNotes[left++]);
                    if (left <= right) {
                        arpSequence.push_back(stepNotes[right--]);
                    }
                }
            }
            break;

        case ARP_DIVERGE:
            // Play from inside to outside
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                vector<ArpNote> stepNotes;
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        stepNotes.push_back(stepNote);
                    }
                }
                // Add notes from middle toward ends
                int mid = stepNotes.size() / 2;
                for (int i = 0; i <= mid && i < stepNotes.size(); i++) {
                    if (mid - i >= 0) {
                        arpSequence.push_back(stepNotes[mid - i]);
                    }
                    if (mid + i + 1 < stepNotes.size() && i > 0) {
                        arpSequence.push_back(stepNotes[mid + i]);
                    }
                }
            }
            break;

        case ARP_CON_DIVERGE:
            // Converge then diverge
            {
                vector<ArpNote> convergeSeq;
                for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                    vector<ArpNote> stepNotes;
                    for (const ArpNote& note : sortedNotes) {
                        ArpNote stepNote = note;
                        stepNote.note += stepNum * config->stepOffset;
                        if (stepNote.note < 128 && stepNote.note >= 0) {
                            stepNotes.push_back(stepNote);
                        }
                    }
                    int left = 0, right = stepNotes.size() - 1;
                    while (left <= right) {
                        convergeSeq.push_back(stepNotes[left++]);
                        if (left <= right) {
                            convergeSeq.push_back(stepNotes[right--]);
                        }
                    }
                }
                // Add converge sequence
                arpSequence = convergeSeq;
                // Add diverge sequence (reverse of converge)
                for (int i = convergeSeq.size() - 2; i > 0; i--) {
                    arpSequence.push_back(convergeSeq[i]);
                }
            }
            break;

        case ARP_DIV_CONVERGE:
            // Diverge then converge
            {
                vector<ArpNote> divergeSeq;
                for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                    vector<ArpNote> stepNotes;
                    for (const ArpNote& note : sortedNotes) {
                        ArpNote stepNote = note;
                        stepNote.note += stepNum * config->stepOffset;
                        if (stepNote.note < 128 && stepNote.note >= 0) {
                            stepNotes.push_back(stepNote);
                        }
                    }
                    int mid = stepNotes.size() / 2;
                    for (int i = 0; i <= mid && i < stepNotes.size(); i++) {
                        if (mid - i >= 0) {
                            divergeSeq.push_back(stepNotes[mid - i]);
                        }
                        if (mid + i + 1 < stepNotes.size() && i > 0) {
                            divergeSeq.push_back(stepNotes[mid + i]);
                        }
                    }
                }
                // Add diverge sequence
                arpSequence = divergeSeq;
                // Add converge sequence (reverse of diverge)
                for (int i = divergeSeq.size() - 2; i > 0; i--) {
                    arpSequence.push_back(divergeSeq[i]);
                }
            }
            break;

        case ARP_PINKY_UP:
            // Play lowest note, then rest ascending
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                bool first = true;
                for (const ArpNote& note : sortedNotes) {
                    ArpNote stepNote = note;
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        if (first) {
                            arpSequence.push_back(stepNote);
                            first = false;
                        }
                    }
                }
                for (size_t i = 1; i < sortedNotes.size(); i++) {
                    ArpNote stepNote = sortedNotes[i];
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            break;

        case ARP_PINKY_UP_DOWN:
            // Play lowest, then up, then down
            {
                vector<ArpNote> baseSeq;
                for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                    bool first = true;
                    for (const ArpNote& note : sortedNotes) {
                        ArpNote stepNote = note;
                        stepNote.note += stepNum * config->stepOffset;
                        if (stepNote.note < 128 && stepNote.note >= 0) {
                            if (first) {
                                baseSeq.push_back(stepNote);
                                first = false;
                            }
                        }
                    }
                    for (size_t i = 1; i < sortedNotes.size(); i++) {
                        ArpNote stepNote = sortedNotes[i];
                        stepNote.note += stepNum * config->stepOffset;
                        if (stepNote.note < 128 && stepNote.note >= 0) {
                            baseSeq.push_back(stepNote);
                        }
                    }
                }
                arpSequence = baseSeq;
                // Add reverse without endpoints
                for (int i = baseSeq.size() - 2; i > 0; i--) {
                    arpSequence.push_back(baseSeq[i]);
                }
            }
            break;

        case ARP_THUMB_UP:
            // Play highest note, then rest ascending from lowest
            for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                // Add highest note first
                if (!sortedNotes.empty()) {
                    ArpNote highNote = sortedNotes.back();
                    highNote.note += stepNum * config->stepOffset;
                    if (highNote.note < 128 && highNote.note >= 0) {
                        arpSequence.push_back(highNote);
                    }
                }
                // Add rest ascending except the highest
                for (size_t i = 0; i < sortedNotes.size() - 1; i++) {
                    ArpNote stepNote = sortedNotes[i];
                    stepNote.note += stepNum * config->stepOffset;
                    if (stepNote.note < 128 && stepNote.note >= 0) {
                        arpSequence.push_back(stepNote);
                    }
                }
            }
            break;

        case ARP_THUMB_UP_DOWN:
            // Play highest, then up from lowest, then down
            {
                vector<ArpNote> baseSeq;
                for (uint8_t stepNum = 0; stepNum < actualSteps; stepNum++) {
                    // Add highest note first
                    if (!sortedNotes.empty()) {
                        ArpNote highNote = sortedNotes.back();
                        highNote.note += stepNum * config->stepOffset;
                        if (highNote.note < 128 && highNote.note >= 0) {
                            baseSeq.push_back(highNote);
                        }
                    }
                    // Add rest ascending except the highest
                    for (size_t i = 0; i < sortedNotes.size() - 1; i++) {
                        ArpNote stepNote = sortedNotes[i];
                        stepNote.note += stepNum * config->stepOffset;
                        if (stepNote.note < 128 && stepNote.note >= 0) {
                            baseSeq.push_back(stepNote);
                        }
                    }
                }
                arpSequence = baseSeq;
                // Add reverse without endpoints
                for (int i = baseSeq.size() - 2; i > 0; i--) {
                    arpSequence.push_back(baseSeq[i]);
                }
            }
            break;
    }
}

void Arpeggiator::StepArpeggiator(deque<MidiPacket>& output) {
    if (arpSequence.empty()) {
        return;
    }

    // Check if currentIndex is out of bounds (sequence changed) or needs wrapping
    if (currentIndex >= arpSequence.size()) {
        // Detect sequence completion when wrapping
        // lastSequenceIndex holds the last valid index we played
        if (config->repeat != 0 && lastSequenceIndex < arpSequence.size()) {
            currentRepeat++;

            // If we've reached the repeat limit, stop
            if (currentRepeat >= config->repeat) {
                // Turn off all sustained notes
                for (const auto& gateEvent : gateOffQueue) {
                    output.push_back(MidiPacket::NoteOff(gateEvent.channel, gateEvent.note, 0));
                }
                gateOffQueue.clear();
                return;
            }
        }

        currentIndex = 0;
    }

    // For random mode, pick a random note from the sequence
    if (config->direction == ARP_RANDOM) {
        currentIndex = rand() % arpSequence.size();
    }

    // Play current note
    const ArpNote& currentNote = arpSequence[currentIndex];
    output.push_back(MidiPacket::NoteOn(currentNote.channel, currentNote.note, currentNote.velocity));


    // Calculate gate off time based on gate percentage and BASE step time (not swing-modified)
    if (config->gateTime == 0) {
        // Gate time 0 = always on until arp stops
        // Use UINT32_MAX to indicate infinite gate time
        gateOffQueue.push_back({UINT32_MAX, currentNote.note, currentNote.channel});
    } else {
        // Gate duration is based on baseTicksPerStep (consistent note length regardless of swing)
        uint32_t gateTicks = (baseTicksPerStep * config->gateTime) / 100;
        uint32_t gateOffTick = tickCounter + gateTicks;

        // Add this note to the gate off queue
        gateOffQueue.push_back({gateOffTick, currentNote.note, currentNote.channel});
    }

    // Store last index before advancing
    lastSequenceIndex = currentIndex;

    // Advance to next note (just increment, will wrap on next call if needed)
    currentIndex++;
}

void Arpeggiator::CalculateTicksPerStep() {
    if (division == DIV_OFF || division == 0) {
        baseTicksPerStep = UINT32_MAX;
        ticksPerStep[0] = ticksPerStep[1] = UINT32_MAX;
        return;
    }

    // Calculate base ticks based on TPQN and division
    // division = how many of this note per whole note
    // TPQN = ticks per quarter note
    // baseTicksPerStep = (TPQN * 4) / division
    baseTicksPerStep = (EFFECT_TPQN * 4) / division;

    // Apply swing based on 20-80 range with 50 as center (no swing)
    // Convert swing amount (20-80) to ratio (-0.3 to +0.3)
    float swingRatio = (config->swing - 50) / 100.0f;

    // Swing modifies the timing between note triggers (not gate length)
    // On-beat gets longer by swing amount, off-beat gets shorter
    // This maintains total duration: ticksPerStep[0] + ticksPerStep[1] = 2 * baseTicksPerStep
    uint32_t swingTicks = (uint32_t)(baseTicksPerStep * swingRatio);

    ticksPerStep[0] = baseTicksPerStep + swingTicks;  // On-beat (longer with positive swing)
    ticksPerStep[1] = baseTicksPerStep - swingTicks;  // Off-beat (shorter with positive swing)

    MLOGI("Arp", "CalculateTicksPerStep: baseTicksPerStep=%u, swing=%u, swingRatio=%.2f, swingTicks=%u, ticksPerStep[0]=%u, ticksPerStep[1]=%u",
          baseTicksPerStep, config->swing, swingRatio, swingTicks, ticksPerStep[0], ticksPerStep[1]);
}

void Arpeggiator::Reset() {
    notePool.clear();
    arpSequence.clear();
    currentIndex = 0;
    tickCounter = 0;
    nextStepTick = ticksPerStep[0];  // Initialize to first step duration, not 0
    gateOffQueue.clear(); // Clear all gate timers
    disableOnNextTick = false;
    currentRepeat = 0;  // Reset repeat counter
    lastSequenceIndex = 0;
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
    if(cfg != nullptr)
    {
        config = cfg;
    }
    CalculateTicksPerStep(); // Recalculate timings when config changes
}

void Arpeggiator::SetDivision(ArpDivision div) {
    ArpDivision oldDivision = division;
    division = div;
    CalculateTicksPerStep();

    // If turning on arpeggiator with notes already held, start immediately
    if (oldDivision == DIV_OFF && div != DIV_OFF && !notePool.empty()) {
        currentIndex = 0;
        currentRepeat = 0;  // Reset repeat counter when restarting
        lastSequenceIndex = 0;
        nextStepTick = tickCounter;
        // Note: We can't call StepArpeggiator here since we don't have output queue
        // The force start will happen on the next Tick()
    }
}