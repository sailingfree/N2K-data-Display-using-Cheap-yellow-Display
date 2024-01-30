// Log functions
// basically wrappers for stdio

#include <Arduino.h>
#include <stdio.h>
#include <logger.h>


FILE *  openLog(String logname) {
    FILE * result = NULL;

    result = fopen(logname.c_str(), "a");
    return result;
}

void logClose(FILE * fptr) {
    fflush(fptr);
    fclose(fptr);
}


void logAdd(FILE * fptr, String & msg) {
    fwrite(msg.c_str(), strlen(msg.c_str()), 1, fptr);
    fflush(fptr);
}