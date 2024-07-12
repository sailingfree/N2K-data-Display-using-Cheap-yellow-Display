// Logging for te gateway

#include <Arduino.h>
#include <GwLogger.h>

static String logname;

void setup_logging(void) {
    logname = "logfile";
    logname += time(NULL);
    logname += ".txt";

    if(!hasSdCard()) {
      return;
    }

// create a file and write one line to the file
  if (!file.open(logname.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
    errorPrint("Creating logfile");
    return;
  }
  file.println(logname);
  file.close();
}

void append_log(const char * msg) {
    String txt("");
    txt += (uint32_t) millis();
    txt += msg;

    if(!hasSdCard()) {
      return;
    }

    if (!file.open(logname.c_str(), O_WRONLY | O_CREAT | O_APPEND)) {
        errorPrint("Updating logfile");
        return;
    }

  file.println(txt);
  file.close();
  file.sync();
}

void read_log(Stream & s) {
    if(!hasSdCard()) {
      return;
    }
    
    if (!file.open(logname.c_str(), O_RDWR)) {
        errorPrint("Updating logfile");
        return;
    }

    String str;
    do {
        str = file.readString();
        s.print(str);
    } while(str.length());
    file.close();
}

String & getLogname() {
  return logname;
}