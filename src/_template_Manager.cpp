#include "_template_Manager.h"
#include "globals.h"

// The getter for the instantiated singleton instance
_template_Manager_ &_template_Manager_::getInstance()
{
    static _template_Manager_ instance;
    return instance;
}

// Initialize the global shared instance
_template_Manager_ &_template_Manager = _template_Manager.getInstance();

void _template_Manager_::setup() {

}

void _template_Manager_::tick() {

}