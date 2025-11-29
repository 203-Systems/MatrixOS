#include "ClipLauncher.h"

ClipLauncher::ClipLauncher(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

std::pair<uint8_t, uint8_t> ClipLauncher::XY2Clip(Point xy) const
{
    uint8_t track, clip;

    if (sequencer->wideClipMode) {
        track = sequencer->clipWindow * 4 + xy.x / 2;
        clip = xy.y * 2 + xy.x % 2;

    } else {
        track = xy.x;
        clip = xy.y + (sequencer->clipWindow * 7);
    }

    return std::make_pair(track, clip);
}

Dimension ClipLauncher::GetSize() { return Dimension(8, 7); }

bool ClipLauncher::IsEnabled()
{
    return sequencer->currentView == Sequencer::ViewMode::Session;
}

bool ClipLauncher::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(keyInfo->State() == PRESSED)
    {
        std::pair<uint8_t, uint8_t> pair = XY2Clip(xy);
        uint8_t track = pair.first;
        uint8_t clip = pair.second;

        // Handle copy mode
        if(sequencer->CopyActive())
        {
            // Source selection if none yet
            if(sequencer->copySourceClip < 0)
            {
                // Only allow selecting existing clips as source
                if(sequencer->sequence.ClipExists(track, clip))
                {
                    sequencer->copySourceClip = clip;
                    sequencer->copySourceTrack = track;
                }
                return true;
            }
            else
            {
                // Copy from existing source to this location
                uint8_t sourceTrack = sequencer->copySourceTrack;
                uint8_t sourceClip = sequencer->copySourceClip;

                // Copy clip (includes all patterns and enabled state)
                sequencer->sequence.CopyClip(sourceTrack, sourceClip, track, clip);
            }
        }
        // If clip exists and Clear is active, delete it
        else if(sequencer->ClearActive())
        {
            sequencer->sequence.DeleteClip(track, clip);

            // Update selection if we deleted the current clip
            if(sequencer->sequence.GetPosition(track)->clip == clip)
            {
                // Find first available clip or default to 0
                sequencer->sequence.SetClip(track, 0);
            }
        }
        // If clip doesn't exist, create it or stop track
        else if(!sequencer->sequence.ClipExists(track, clip))
        {
            if(sequencer->ClearActive())
            {
                // Don't create clip if Clear is active
                return true;
            }

            // If track is playing, stop it when clicking empty slot
            if(sequencer->sequence.Playing(track))
            {
                sequencer->sequence.StopAfter(track);
            }
            else if(sequencer->sequence.Playing() == false)
            {
                sequencer->sequence.NewClip(track, clip);

                // Select the newly created clip
                sequencer->track = track;
                sequencer->sequence.SetClip(track, clip);
                sequencer->ClearActiveNotes();
                sequencer->ClearSelectedNotes();
                sequencer->stepSelected.clear();
            }
        }
        // Select clip
        else
        {
            sequencer->track = track;
            if(sequencer->sequence.Playing())
            {
                if(clip == sequencer->sequence.GetNextClip(track))
                {
                    sequencer->sequence.PlayClipForAllTracks(clip);
                }
                else
                {
                    sequencer->sequence.PlayClip(track, clip);
                }
            }
            else
            {   
                if(sequencer->ShiftActive())
                {
                    sequencer->sequence.PlayClip(track, clip);
                }
                else
                {
                    sequencer->sequence.SetClip(track, clip);
                }   
            }

            sequencer->ClearActiveNotes();
            sequencer->ClearSelectedNotes();
            sequencer->stepSelected.clear();
        }
    }
    return true;
}

bool ClipLauncher::Render(Point origin)
{
    uint8_t trackCount = sequencer->sequence.GetTrackCount();

    Fract16 quarterNoteProgress = sequencer->sequence.GetQuarterNoteProgress();
    uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();

    vector<uint8_t> activeClips;
    vector<uint8_t> nextClips;
    activeClips.reserve(trackCount);
    nextClips.reserve(trackCount);

    for(uint8_t track = 0; track < trackCount; track++)
    {
        uint8_t activeClip = sequencer->sequence.GetPosition(track)->clip;
        activeClips.push_back(activeClip);
        uint8_t nextClip = sequencer->sequence.GetNextClip(track);
        nextClips.push_back(nextClip);
    }

    // Render all clip slots
    for(uint8_t x = 0; x < GetSize().x; x++)
    {
        for(uint8_t y = 0; y < GetSize().y; y++)
        {
            Point xy = Point(x, y);
            
            std::pair<uint8_t, uint8_t> pair = XY2Clip(xy);
            uint8_t track = pair.first;
            uint8_t clip = pair.second;

            Point point = origin + xy;
            Color color;

            if(track >= trackCount)
            {
                // No track at this position
                color = Color::Black;
            }
            else if(sequencer->CopyActive())
            {
                // Copy mode rendering
                if(sequencer->copySourceClip >= 0 && sequencer->copySourceTrack == track && sequencer->copySourceClip == clip)
                {
                    // Highlight selected source in white
                    color = Color::White;
                }
                else if(sequencer->sequence.ClipExists(track, clip))
                {
                    // Show existing clips in track color
                    color = sequencer->meta.tracks[track].color;
                }
                else if(sequencer->copySourceClip >= 0)
                {
                    // Show empty slots in dimmed track color when source selected
                    color = sequencer->meta.tracks[track].color.Dim(32);
                }
                else
                {
                    // Show empty slots in dim gray when no source selected
                    color = Color::Black;
                }
            }
            else if(!sequencer->sequence.ClipExists(track, clip))
            {
                // Clip doesn't exist - show black if track is playing, otherwise dim gray
                if(sequencer->sequence.Playing())
                {
                    if(nextClips[track] == 254) // About to stop
                    {
                        // color = Color(0x100000);
                        color = (quarterNoteProgress < 0x8000) ? Color(0x600000) : Color::Black;
                    }
                    else
                    {
                        color = Color::Black;
                    }
                }
                else if(sequencer->ClearActive())
                {
                    color = Color::Black;
                }
                else
                {
                    Color trackColor = sequencer->meta.tracks[track].color;
                    color = trackColor.Dim(32);
                }
            }
            else
            {
                Color trackColor = sequencer->meta.tracks[track].color;
                bool isCurrentTrack = (sequencer->track == track);
                bool isSelectedInTrack = (activeClips[track] == clip);
                bool isPlaying = sequencer->sequence.Playing(track) && isSelectedInTrack;
                bool isNextClip = (nextClips[track] == clip);

                if(isPlaying)
                {
                    // Playing clip pulse toward white
                    color = Color::Crossfade(trackColor, Color::White, Fract16(breathingScale / 2, 8));
                }
                else if(isNextClip)
                {
                    color = (quarterNoteProgress < 0x8000) ? Color(0x00FF00) : trackColor;
                }
                else if(isCurrentTrack && isSelectedInTrack)
                {
                    color = Color::Crossfade(trackColor, Color::White, Fract16(0x9000));
                }
                else if(isSelectedInTrack)
                {
                    // Selected clip in other tracks shows red
                    uint16_t brightness = breathingScale / 2 + 128;
                    if(brightness > 255) { brightness = 255; }
                    color = trackColor.Scale(brightness);
                }
                else
                {
                    // Clip exists - show track color
                    color = trackColor;
                }
            }

            MatrixOS::LED::SetColor(point, color);
        }
    }

    return true;
}
