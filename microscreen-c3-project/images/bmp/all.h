#pragma once
#include <Arduino.h>  // for PROGMEM on ESP32

// Define bitmaps from raw header data
static const unsigned char angy[] PROGMEM = {
#include "angy.h"
};

static const unsigned char smudge[] PROGMEM = {
#include "smudge.h"
};

static const unsigned char korby[] PROGMEM = {
#include "korby.h"
};

static const unsigned char aw[] PROGMEM = {
#include "aw.h"
};

static const unsigned char concorned[] PROGMEM = {
#include "concorned.h"
};

static const unsigned char wat[] PROGMEM = {
#include "wat.h"
};

// ...add more as needed for each images/bmp/*.h file...

