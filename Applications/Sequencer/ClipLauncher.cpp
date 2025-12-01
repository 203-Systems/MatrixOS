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
            if(!sequencer->copySource.Selected())
            {
                // Only allow selecting existing clips as source
                if(sequencer->sequence.ClipExists(track, clip))
                {
                    sequencer->copySource.type = SequenceSelectionType::CLIP;
                    sequencer->copySource.track = track;
                    sequencer->copySource.clip = clip;
                }
                return true;
            }
            else
            {
                // Copy from existing source to this location
                uint8_t sourceTrack = sequencer->copySource.track;
                uint8_t sourceClip = sequencer->copySource.clip;

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
            if(sequencer->sequence.Playing())
            {   
                if(sequencer->sequence.GetNextClip(track) == 254)
                {
                    sequencer->sequence.PlayClipForAllTracks(clip);
                }
                else
                {
                    sequencer->sequence.PlayClip(track, 254); // Force into wait to stop
                }
            }
            else
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

    Color baseColor = Color::Black;

    if(sequencer->ClearActive())
    {
        uint8_t scale = ColorEffects::Breath();
        baseColor = Color::Crossfade(Color(0xFF0080).Dim(32), Color::White.Dim(32), Fract16(scale / 2, 8));
    }
    else if(sequencer->CopyActive())
    {
        uint8_t scale = ColorEffects::Breath();
        baseColor = Color::Crossfade(Color(0x0080FF).Dim(32), Color::White.Dim(32), Fract16(scale / 2, 8));
    }

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
                color = baseColor;
            }
            else if(!sequencer->sequence.ClipExists(track, clip))
            {
                // Clip doesn't exist - show black if track is playing, otherwise dim gray
                if(sequencer->ClearActive())
                {
                    color = baseColor;
                }
                else if(sequencer->CopyActive())
                {
                    if(sequencer->copySource.Selected())
                    {
                        Color trackColor = sequencer->meta.tracks[track].color;
                        color = trackColor.Dim(32);
                    }
                    else
                    {
                        color = baseColor;
                    }
                }
                else if(sequencer->sequence.Playing())
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
                bool isCopySourceSelected = sequencer->CopyActive() && sequencer->copySource.Selected();
                bool isCopySource = isCopySourceSelected && (sequencer->copySource.track == track && sequencer->copySource.clip == clip);
                bool isWrongCopyType = sequencer->CopyActive() && sequencer->copySource.Selected() && !sequencer->copySource.IsType(SequenceSelectionType::CLIP);


                    if(isWrongCopyType)
                    {
                        // Gray out when wrong copy type is selected
                        color = Color::White.Dim(32);
                    }
                    else if(isCopySource)
                    {
                        color = Color::White;
                    }
                    else if (isCopySourceSelected)
                    {
                        color = trackColor;
                    }
                    else if(isPlaying)
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
