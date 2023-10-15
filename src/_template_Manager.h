/// TODO: To be used for e.g. new data sources

#ifndef _template_Manager_h
#define _template_Manager_h

#include <Arduino.h>

class _template_Manager_
{
private:
public:
    static _template_Manager_ &getInstance();
    void setup();
    void tick();
};

extern _template_Manager_ &_template_Manager;

#endif