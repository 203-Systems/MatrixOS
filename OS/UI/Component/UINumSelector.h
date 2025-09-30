#include "MatrixOS.h"
#include "UISelectorBase.h"

// Difference between NumSelector and Item Selector is that everything lower than Output will be lit instead of single item being lit
template <class T>
class UINumItemSelector : public UIComponent {
 public:
  Color color;
  T* output;
  T* items;
  Dimension dimension;
  uint16_t count;
  UISelectorDirection direction;
  std::unique_ptr<std::function<void(T)>> changeCallback;

  UINumItemSelector() {
    this->dimension = Dimension(1, 1);
    this->color = Color(0);
    this->output = nullptr;
    this->count = 0;
    this->items = nullptr;
    this->direction = UISelectorDirection::RIGHT_THEN_DOWN;
    this->changeCallback = nullptr;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return dimension; }

  void SetDimension(Dimension dimension) { this->dimension = dimension; }
  void SetColor(Color color) { this->color = color; }
  void SetOutput(T* output) { this->output = output; }
  void SetCount(uint16_t count) { this->count = count; }
  void SetItems(T* items) { this->items = items; }
  void SetDirection(UISelectorDirection direction) { this->direction = direction; }
  void OnChange(std::function<void(T)> changeCallback) { this->changeCallback = std::make_unique<std::function<void(T)>>(changeCallback); }

  virtual bool OnChangeCallback(T value) {
    if (changeCallback) {
      (*changeCallback)(value);
      return true;
    }
    return false;
  }

  virtual bool Render(Point origin) {
    for (uint16_t item = 0; item < dimension.Area(); item++)
    {
      Point xy = origin + IndexToPoint(item, dimension, direction);
      if (item > count)
      { MatrixOS::LED::SetColor(xy, Color(0)); }
      else
      { MatrixOS::LED::SetColor(xy, color.DimIfNot(items[item] <= *output)); }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint16_t id = PointToIndex(xy, dimension, direction);
    if (id > count){return false;}
    if (keyInfo->State() == PRESSED)
    { 
      *output = items[id]; 
      OnChangeCallback(*output);
    }
    return true;
  }
}; 