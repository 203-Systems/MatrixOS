#pragma once
#include "MatrixOS.h"
#include <functional>

struct Application_Info {
  uint32_t id;
  string name;
  string author;
  Color color;
  uint32_t version;
  std::function<Application*()> factory;
  bool visibility;

  Application_Info(uint32_t id, string name, string author, Color color, uint32_t version,
                   std::function<Application*()> factory, bool visibility = true) {
    this->id = id;
    this->name = name;
    this->author = author;
    this->color = color;
    this->version = version;
    this->factory = factory;
    this->visibility = visibility;
  }
  Application_Info() {}
};

class Application {
 public:
  string name;
  string author;
  uint32_t version;

  void* args;

  void Start(void* args = NULL);

  virtual void Setup(){};
  virtual void Loop(){};
  virtual void End(){};

  void Exit();
};