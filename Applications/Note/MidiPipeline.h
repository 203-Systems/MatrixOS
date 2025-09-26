#pragma once

#include "MatrixOS.h"
#include "MidiEffect.h"

// Pipeline manager class - minimal and efficient
class MidiPipeline {
private:
    vector<string> effectKeys;   // Effect keys in order
    vector<MidiEffect*> effects;      // Effects in same order
    deque<MidiPacket> inputQueue;     // Input queue
    deque<MidiPacket> outputQueue;    // Output queue
    uint8_t noteStates[16] = {0};     // Bitmap tracking active notes (each bit represents a note)

    // Private note state management
    void SetNoteState(uint8_t note, bool on);

public:
    // Send packet to input queue
    void Send(const MidiPacket& packet);

    // Get packet from output queue
    bool Get(MidiPacket& packet);

    // Process all input queue packets and call Tick on all effects
    void Tick();

    // Add effect to chain with a key
    // addAfter: key to add after, or "head" for front, "tail" or "" for back (default)
    // Returns index where effect was inserted, or -1 if addAfter key is invalid
    int32_t AddEffect(const string& key, MidiEffect* effect, const string& addAfter = "");

    // Remove effect from chain by key
    void RemoveEffect(const string& key);

    // Get effect by key
    MidiEffect* GetEffect(const string& key) const;

    // Reset pipeline - calls Reset() on all effects
    void Reset();

    // Clear all effects
    void Clear();

    // Get effect count
    size_t GetEffectCount() const;

    // Public note state queries
    bool IsNoteActive(uint8_t note) const;
};