#include "MidiPipeline.h"

void MidiPipeline::Send(const MidiPacket& packet) {
    inputQueue.push_back(packet);
}

bool MidiPipeline::Get(MidiPacket& packet) {
    if (outputQueue.empty()) {
        return false;
    }
    packet = outputQueue.front();
    outputQueue.pop_front();
    return true;
}

void MidiPipeline::Tick() {
    if (effects.empty()) {
        // No effects, direct passthrough
        outputQueue.insert(outputQueue.end(),
            std::make_move_iterator(inputQueue.begin()),
            std::make_move_iterator(inputQueue.end()));
        inputQueue.clear();
        return;
    }

    std::deque<MidiPacket> effect_input;
    std::deque<MidiPacket> effect_output;

    // Start with inputQueue as effect_output
    effect_output.swap(inputQueue);

    for (MidiEffect* effect : effects) {
        if (effect->IsEnabled()) {
            effect_input.swap(effect_output);  // take previous output as new input
            effect_output.clear();             // prepare output buffer
            effect->Tick(effect_input, effect_output);
        }
    }

    // Update note states and move final output to main queue
    for (const MidiPacket& packet : effect_output) {
        // Track note on/off states
        if (packet.status == NoteOn && packet.Velocity() > 0) {
            SetNoteState(packet.Note(), true);
        }
        else if (packet.status == NoteOff || (packet.status == NoteOn && packet.Velocity() == 0)) {
            SetNoteState(packet.Note(), false);
        }
    }

    outputQueue.insert(outputQueue.end(),
        std::make_move_iterator(effect_output.begin()),
        std::make_move_iterator(effect_output.end()));
    effect_output.clear();
}

int32_t MidiPipeline::AddEffect(const string& key, MidiEffect* effect, const string& addAfter) {
    if (addAfter == "head") {
        // Add to front
        effectKeys.insert(effectKeys.begin(), key);
        effects.insert(effects.begin(), effect);
        return 0;
    } else if (addAfter == "tail" || addAfter.empty()) {
        // Add to back (default)
        effectKeys.push_back(key);
        effects.push_back(effect);
        return static_cast<int32_t>(effectKeys.size() - 1);
    } else {
        // Add after specific key
        for (size_t i = 0; i < effectKeys.size(); ++i) {
            if (effectKeys[i] == addAfter) {
                effectKeys.insert(effectKeys.begin() + i + 1, key);
                effects.insert(effects.begin() + i + 1, effect);
                return static_cast<int32_t>(i + 1);
            }
        }
        // If key not found, return -1
        return -1;
    }
}

void MidiPipeline::RemoveEffect(const string& key) {
    for (size_t i = 0; i < effectKeys.size(); ++i) {
        if (effectKeys[i] == key) {
            effectKeys.erase(effectKeys.begin() + i);
            effects.erase(effects.begin() + i);
            break;
        }
    }
}

void MidiPipeline::Reset() {
    for (MidiEffect* effect : effects) {
        if (effect) {
            effect->Reset();
        }
    }
}

void MidiPipeline::Clear() {
    effectKeys.clear();
    effects.clear();
    inputQueue.clear();
    outputQueue.clear();
    memset(noteStates, 0, sizeof(noteStates));
}

size_t MidiPipeline::GetEffectCount() const {
    return effects.size();
}

void MidiPipeline::SetNoteState(uint8_t note, bool on) {
    if (note >= 128) return;
    uint8_t byteIndex = note / 8;
    uint8_t bitIndex = note % 8;
    if (byteIndex < 16) {
        if (on) {
            noteStates[byteIndex] |= (1 << bitIndex);
        } else {
            noteStates[byteIndex] &= ~(1 << bitIndex);
        }
    }
}

bool MidiPipeline::IsNoteActive(uint8_t note) const {
    if (note >= 128) return false;
    uint8_t byteIndex = note / 8;
    uint8_t bitIndex = note % 8;
    if (byteIndex < 16) {
        return (noteStates[byteIndex] & (1 << bitIndex)) != 0;
    }
    return false;
}