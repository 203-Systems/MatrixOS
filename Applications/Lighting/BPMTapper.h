#include <cmath>
#include <deque>
#include "MatrixOS.h"
#include "ui/UI.h"

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
    
    // Calculate average interval from last 3 taps (2 intervals)
    size_t size = tap_times.size();
    uint64_t newest = tap_times[size - 1];
    uint64_t middle = tap_times[size - 2];
    uint64_t oldest = tap_times[size - 3];
    
    uint64_t interval1 = newest - middle;
    uint64_t interval2 = middle - oldest;
    uint64_t avg_interval = (interval1 + interval2) / 2;
    
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
    if (keyInfo->state == RELEASED)
    {
      tap_times.push_back(MatrixOS::SYS::Micros());
      if (tap_times.size() > TAP_HISTORY_SIZE) {
        tap_times.pop_front();
      }
      CalculateBPM();
    }
    else if(keyInfo->state == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(name, color);
      tap_times.clear();
    }
    return true;
  }
};