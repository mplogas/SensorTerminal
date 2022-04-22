#include <Arduino.h>
#include <LinkedList.h>

#ifndef models
#define models

class Entry {
  public:
    String title;
    String unit;
    float value;
};

struct Module {
  String title;
  LinkedList<Entry*> entries;
};

#endif
