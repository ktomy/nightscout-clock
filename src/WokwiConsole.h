#ifndef WOKWICONSOLE_H
#define WOKWICONSOLE_H

// Serial commands for tests in the Wokwi simulator, which cannot reach the clock over HTTP and
// cannot take screenshots of its NeoPixel canvas. Built only with -DWOKWI (the "wokwi"
// PlatformIO environment). Send one command per line:
//
//   bg <sgv> [trend] [minutes_ago]  Add a reading to the "API" data source. The trend uses the
//                                   Nightscout numbers: 1 double up ... 4 flat ... 7 double down.
//                                   Default: 4, 0 minutes ago.
//   clear                           Delete all readings from the "API" data source.
//   screen                          Print "WOKWI_SCREEN " and one RRGGBB hex value per LED, row by
//                                   row from the top left, before brightness is applied.
//
// Each command answers with a line starting with "WOKWI_OK" or "WOKWI_ERROR".

void wokwiConsoleTick();

#endif  // WOKWICONSOLE_H
