#pragma once
#include "UIComponent.h"
#include "../UIUtilities.h"

enum UISelectorColorMode {
  COLOR_MODE_SINGLE,
  COLOR_MODE_FUNCTION,
  COLOR_MODE_INDIVIDUAL,
};

enum class UISelectorDirection {
  RIGHT_THEN_DOWN,
  DOWN_THEN_RIGHT,
  LEFT_THEN_DOWN,
  DOWN_THEN_LEFT,
  UP_THEN_RIGHT,
  RIGHT_THEN_UP,
  UP_THEN_LEFT,
  LEFT_THEN_UP,
};

enum class UISelectorLitMode {
  LIT_EQUAL,
  LIT_LESS_EQUAL_THAN,
  LIT_GREATER_EQUAL_THAN,
  LIT_ALWAYS,
};

class UISelectorBase : public UIComponent {
 public:
  string name;
  Dimension dimension;
  uint16_t count;
  UISelectorDirection direction;
  UISelectorColorMode colorMode;
  Color color;  
  std::unique_ptr<std::function<Color()>> colorFunc;
  std::unique_ptr<std::function<Color(uint16_t)>> individualColorFunc;
  std::unique_ptr<std::function<string(uint16_t)>> nameFunc;
  
  UISelectorBase() {
    this->dimension = Dimension(1, 1);
    this->name = "";
    this->color = Color(0);
    this->count = UINT16_MAX;
    this->direction = UISelectorDirection::RIGHT_THEN_DOWN;
  }

  void SetDimension(Dimension dimension) { this->dimension = dimension; }
  void SetName(string name) { this->name = name; }
  void SetCount(uint16_t count) { this->count = count; }

  void SetColor(Color color) {
    this->colorFunc.reset();
    this->individualColorFunc.reset();
    colorMode = UISelectorColorMode::COLOR_MODE_SINGLE;
    this->color = color; 
  }

  void SetColorFunc(std::function<Color()> colorFunc) {
    this->individualColorFunc.reset();
    colorMode = UISelectorColorMode::COLOR_MODE_FUNCTION;
    this->colorFunc = std::make_unique<std::function<Color()>>(colorFunc);
  }

  void SetIndividualColorFunc(std::function<Color(uint16_t)> colorFunc) { // UINT16_MAX is called when the color is used for text scroll
    this->colorFunc.reset();
    colorMode = UISelectorColorMode::COLOR_MODE_INDIVIDUAL;
    this->individualColorFunc = std::make_unique<std::function<Color(uint16_t)>>(colorFunc);
  }

  void SetDirection(UISelectorDirection direction) { this->direction = direction; }
  void SetIndividualNameFunc(std::function<string(uint16_t)> nameFunc) { this->nameFunc = std::make_unique<std::function<string(uint16_t)>>(nameFunc); }

  // Internal functions  
  protected:
  virtual Color GetColor() { 
    if(colorMode == UISelectorColorMode::COLOR_MODE_FUNCTION)
    {
      return (*colorFunc)();
    }
    else if(colorMode == UISelectorColorMode::COLOR_MODE_INDIVIDUAL)
    {
      return (*individualColorFunc)(UINT16_MAX);
    }
    else
    {
      return color;
    }
  }

  virtual Dimension GetSize() { return dimension; }
  
  virtual Color GetIndividualColor(uint16_t index) {
    if(colorMode == UISelectorColorMode::COLOR_MODE_INDIVIDUAL && individualColorFunc != nullptr)
    {
      return (*individualColorFunc)(index);
    }
    return GetColor();
  }

  virtual string GetIndividualName(uint16_t index) {
    if(nameFunc != nullptr)
    {
      return (*nameFunc)(index);
    }
    return name;
  }

  virtual bool ShouldLit(uint16_t index) {return false;}  

  virtual void Selected(uint16_t value) {}

  Point IndexToPoint(uint16_t index) {
    switch (direction) {
      case UISelectorDirection::RIGHT_THEN_DOWN:
        return Point(index % dimension.x, index / dimension.x);
      case UISelectorDirection::DOWN_THEN_RIGHT:
        return Point(index / dimension.y, index % dimension.y);
      case UISelectorDirection::LEFT_THEN_DOWN:
        return Point((dimension.x - 1) - (index % dimension.x), index / dimension.x);
      case UISelectorDirection::DOWN_THEN_LEFT:
        return Point((dimension.x - 1) - (index / dimension.y), index % dimension.y);
      case UISelectorDirection::UP_THEN_RIGHT:
        return Point(index % dimension.x, (dimension.y - 1) - (index / dimension.x));
      case UISelectorDirection::RIGHT_THEN_UP:
        return Point(index % dimension.x, (dimension.y - 1) - (index / dimension.x));
      case UISelectorDirection::UP_THEN_LEFT:
        return Point((dimension.x - 1) - (index % dimension.x), (dimension.y - 1) - (index / dimension.x));
      case UISelectorDirection::LEFT_THEN_UP:
        return Point((dimension.x - 1) - (index / dimension.y), (dimension.y - 1) - (index % dimension.y));
      default:
        return Point(index % dimension.x, index / dimension.x);
    }
  }
  
   uint16_t PointToIndex(Point point) {
    switch (direction) {
      case UISelectorDirection::RIGHT_THEN_DOWN:
        return point.x + point.y * dimension.x;
      case UISelectorDirection::DOWN_THEN_RIGHT:
        return point.y + point.x * dimension.y;
      case UISelectorDirection::LEFT_THEN_DOWN:
        return ((dimension.x - 1) - point.x) + point.y * dimension.x;
      case UISelectorDirection::DOWN_THEN_LEFT:
        return point.y + ((dimension.x - 1) - point.x) * dimension.y;
      case UISelectorDirection::UP_THEN_RIGHT:
        return point.x + ((dimension.y - 1) - point.y) * dimension.x;
      case UISelectorDirection::RIGHT_THEN_UP:
        return point.x + ((dimension.y - 1) - point.y) * dimension.x;
      case UISelectorDirection::UP_THEN_LEFT:
        return ((dimension.x - 1) - point.x) + ((dimension.y - 1) - point.y) * dimension.x;
      case UISelectorDirection::LEFT_THEN_UP:
        return ((dimension.y - 1) - point.y) + ((dimension.x - 1) - point.x) * dimension.y;
      default:
        return point.x + point.y * dimension.x;
    }
  }

  // UI Driven functions
  public:
  virtual bool Render(Point origin) {
    Color color = Color(0);
    if(colorMode == UISelectorColorMode::COLOR_MODE_SINGLE)
    {
      color = this->color;
    }
    else if(colorMode == UISelectorColorMode::COLOR_MODE_FUNCTION)
    {
      color = (*colorFunc)();
    }

    for (uint16_t item = 0; item < dimension.Area(); item++)
    {
      if (colorMode == UISelectorColorMode::COLOR_MODE_INDIVIDUAL)
      {
        color = (*individualColorFunc)(item);
      }

      Point xy = origin + IndexToPoint(item);
      if (item >= count)
      { MatrixOS::LED::SetColor(xy, Color(0)); }
      else
      { 
        MatrixOS::LED::SetColor(xy, color.DimIfNot(ShouldLit(item))); 
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint16_t id = PointToIndex(xy);
    if (id >= count)
    { 
      return false;
    }
    else if (keyInfo->State() == RELEASED)
    {
      Selected(id);
    }
    else if (keyInfo->State() == HOLD)
    {
      if(nameFunc == nullptr)
      {
          MatrixOS::UIUtility::TextScroll(name, GetColor());
      }
      else
      {
          MatrixOS::UIUtility::TextScroll(GetIndividualName(id), GetIndividualColor(id));
      }
    }
    return true;
  }
};