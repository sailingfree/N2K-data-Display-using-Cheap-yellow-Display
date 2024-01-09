// NMEA headers
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <NMEA0183.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Msg.h>

// Input/Output stream
extern Stream *Console;

// Main message handler
void handlePGN(tN2kMsg & msg);
