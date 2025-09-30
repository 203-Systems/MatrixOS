#include <cmath>
#include <deque>
#include "MatrixOS.h"
#include "UI/UI.h"

#define TAP_HISTORY_SIZE 10

class UIBPMTapper : public UIComponent {
 public:

  string name;
  Color color;
  Dimension dimension;
  std::function<void(uint16_t)> callback;

  std::deque<uint64_t> tap_times;
  
  UIBPMTapper() {
    this->name = "BPM";
  }

  virtual string GetName() { return this->name; }
  void SetName(string name) { this->name = name; }

  virtual Color GetColor() { return this->color; }
  void SetColor(Color color) { this->color = color; }

  virtual Dimension GetSize() { return this->dimension; }
  void SetSize(Dimension dimension) { this->dimension = dimension; }

  void OnChange(std::function<void(uint16_t)> callback) { this->callback = callback; } 

  void CalculateBPM() {
    if (tap_times.size() < 3) {
      return;
    }
    
    // Use last 5 taps or all available taps if fewer
    size_t taps_to_use = std::min(tap_times.size(), (size_t)5);
    size_t start_index = tap_times.size() - taps_to_use;
    
    // Calculate average interval
    uint64_t total_interval = tap_times[tap_times.size() - 1] - tap_times[start_index];
    uint64_t num_intervals = taps_to_use - 1;
    uint64_t avg_interval = total_interval / num_intervals;
    
    // BPM = 60 seconds * 1,000,000 microseconds / average interval
    uint16_t bpm = 60000000 / avg_interval;

    if(bpm > 299) {
      bpm = 299;
    }
    else if(bpm < 30) {
      bpm = 30;
    }
    if (callback) {
      callback(bpm);
    }
  }

  virtual bool Render(Point origin) {
    uint64_t now = MatrixOS::SYS::Micros();

    // Clear all taps if no tap for 3 seconds.
    // Prevent only 1/2 left after garbage collection, the TAP is showing again.
    if(!tap_times.empty() && now - tap_times.back() > 3000000)
    {
      tap_times.clear(); 
    }
    else
    {
      while (!tap_times.empty() && now - tap_times.front() > 6000000) {
        tap_times.pop_front(); // Remove taps older than 6 seconds
      }
    }

    if (tap_times.size() > 0 && tap_times.size() < 3) {

      // Fill with off
      for (int8_t y = 0; y < dimension.y; y++)
      {
        for (int8_t x = 0; x < dimension.x; x++)
        {
          MatrixOS::LED::SetColor(origin + Point(x, y), Color(0x000000));
        }
      }
      
      // T
      MatrixOS::LED::SetColor(origin + Point(0, 0), color);
      MatrixOS::LED::SetColor(origin + Point(1, 0), color);
      MatrixOS::LED::SetColor(origin + Point(2, 0), color);
      MatrixOS::LED::SetColor(origin + Point(1, 1), color);
      MatrixOS::LED::SetColor(origin + Point(1, 2), color);
      MatrixOS::LED::SetColor(origin + Point(1, 3), color);

      // A
      MatrixOS::LED::SetColor(origin + Point(3, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(4, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(4, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 3), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 3), Color(0xFFFFFF));

      // P
      MatrixOS::LED::SetColor(origin + Point(6, 0), color);
      MatrixOS::LED::SetColor(origin + Point(7, 0), color);
      MatrixOS::LED::SetColor(origin + Point(6, 1), color);
      MatrixOS::LED::SetColor(origin + Point(7, 1), color);
      MatrixOS::LED::SetColor(origin + Point(6, 2), color);
      MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == RELEASED)
    {
      tap_times.push_back(MatrixOS::SYS::Micros());
      if (tap_times.size() > TAP_HISTORY_SIZE) {
        tap_times.pop_front();
      }
      CalculateBPM();
    }
    else if(keyInfo->State() == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(name, color);
      tap_times.clear();
    }
    return true;
  }
};