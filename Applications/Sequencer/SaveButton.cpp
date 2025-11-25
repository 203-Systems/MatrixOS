#include "SaveButton.h"
#include "ColorEffects.h"

SaveButton::SaveButton(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

Dimension SaveButton::GetSize() { return Dimension(4, 4); }

bool SaveButton::Render(Point origin)
{
    if (!MatrixOS::FileSystem::Available())
    {
        Color color = Color::Red.Dim();
        for (uint8_t i = 0; i < 4; ++i)
        {
            MatrixOS::LED::SetColor(origin + Point(i, i), color);
            MatrixOS::LED::SetColor(origin + Point(3 - i, i), color);
        }
        return true;
    }

    bool dirty = sequencer->sequence.GetDirty();
    Color color = dirty ? ColorEffects::ColorBreathLowBound(sequencer->meta.color)
                        : sequencer->meta.color;

    if (dirty)
    {
        // Dirty: Down arrow
        for (uint8_t x = 0; x < 4; ++x)
        {
            for (uint8_t y = 0; y < 4; ++y)
            {
                bool arrow = (x == 1 || x == 2 || y == 2);
                if (!arrow) continue;
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
        }
    }
    else
    {
        // Clean: outline frame, leave 2x2 center empty
        for (uint8_t x = 0; x < 4; ++x)
        {
            for (uint8_t y = 0; y < 4; ++y)
            {
                if (x > 0 && x < 3 && y > 0 && y < 3) continue;
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
        }
    }
    return true;
}

bool SaveButton::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(!MatrixOS::FileSystem::Available())
    {
      if (keyInfo->state == PRESSED)
        {
            MatrixOS::UIUtility::TextScroll("No SD Card", Color::Red);
        }
    }
    else
    {
        if (sequencer->sequence.GetDirty())
        {
            bool arrow = (xy.x == 1 || xy.x == 2 || xy.y == 2);
            if (!arrow) return false;
        }

        if (keyInfo->state == HOLD)
        {
            MatrixOS::UIUtility::TextScroll("Save", sequencer->meta.color);
        }

        if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
        {
            if(sequencer->sequence.GetDirty())
            {
                sequencer->ConfirmSaveUI();
            }
        }
    }

    return true;
}
