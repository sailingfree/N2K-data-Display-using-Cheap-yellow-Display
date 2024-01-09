// Handle the N2K PGNS and update the displays

#include <Arduino.h>
// NMEA headers
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <handlePGN.h>

// Display handlers
#include <tftscreen.h>

void handlePGN(tN2kMsg& msg) {
    switch (msg.PGN) {
        case 127508: {
            // Battery Status
            unsigned char instance = 0xff;
            double voltage = 0.0;
            double current = 0.0;
            double temp = 273.0;
            unsigned char SID = 0xff;
            bool s = ParseN2kPGN127508(msg, instance, voltage, current, temp, SID);

            switch (instance) {
                case 0:
                    setMeter(SCR_ENGINE, HOUSEV, voltage, "V");
                    setMeter(SCR_ENGINE, HOUSEI, current, "A");
                    break;
                case 1:
                    setMeter(SCR_ENGINE, ENGINEV, voltage, "V");
                    ;
                    break;
            }
        } break;

        case 127513:
            // Not interested
            break;

        case 127488: {
            // Engine Rapid
            unsigned char instance;
            double speed;
            double boost;
            int8_t trim;
            bool s = ParseN2kPGN127488(msg, instance, speed, boost, trim);
            setGauge(SCR_ENGINE, speed / 100);
            String es(speed, 0);
            es += "rpm";
            setVlabel(SCR_ENGINE, es);
        } break;

        case 130306: {
            // Wind
            double windSpeed;
            double windAngle;
            unsigned char instance;
            tN2kWindReference ref;
            bool s = ParseN2kPGN130306(msg, instance, windSpeed, windAngle, ref);
            String ws(msToKnots(windSpeed));
            ws += "kts";
            setVlabel(SCR_NAV, ws);
            setGauge(SCR_NAV, RadToDeg(windAngle));

            setMeter(SCR_ENV, WINDSP, msToKnots(windSpeed), "kts");
            setMeter(SCR_ENV, WINDANGLE, RadToDeg(windAngle), "°");
        } break;

        case 129026: {
            // COG/SOG
            unsigned char instance;
            tN2kHeadingReference ref;
            double hdg;
            double sog;
            bool s = ParseN2kPGN129026(msg, instance, ref, hdg, sog);
            setMeter(SCR_NAV, SOG, msToKnots(sog), "kts");
            setMeter(SCR_NAV, HDG, RadToDeg(hdg), "°");
        } break;

        case 128267: {
            // depth;
            unsigned char instance;
            double depth;
            double offset;
            double range;
            bool s = ParseN2kPGN128267(msg, instance, depth, offset, range);
            setMeter(SCR_NAV, DEPTH, depth, "m");
        } break;

        case 129029: {
            // GNSS
            unsigned char instance;
            uint16_t DaysSince1970;
            double SecondsSinceMidnight;
            double Latitude;
            double Longitude;
            double Altitude;
            tN2kGNSStype GNSStype;
            tN2kGNSSmethod GNSSmethod;
            unsigned char nSatellites;
            double Hdop;
            double PDOP;
            double GeoidalSeparation;
            unsigned char nReferenceStations;
            tN2kGNSStype ReferenceStationType;
            uint16_t ReferenceSationID;
            double AgeOfCorrection;

            bool s = ParseN2kPGN129029(msg, instance, DaysSince1970, SecondsSinceMidnight, Latitude,
                                       Longitude, Altitude, GNSStype, GNSSmethod, nSatellites, Hdop, PDOP, GeoidalSeparation,
                                       nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection);

            setMeter(SCR_GNSS, HDOP, Hdop, "");
        } break;

        case 129540: {
            // GNSS satellites in view

            unsigned char instance;
            tN2kRangeResidualMode Mode;
            uint8_t NumberOfSVs;

            // First get the number of satellites in view
            bool s = ParseN2kPGN129540(msg, instance, Mode, NumberOfSVs);

            initGNSSSky(NumberOfSVs);
            initGNSSSignal(NumberOfSVs);
            // Now for each satellite index get the details
            for (int i = 0; i < NumberOfSVs; i++) {
                tSatelliteInfo SatelliteInfo;

                s = ParseN2kPGN129540(msg, i, SatelliteInfo);
                Console->printf("RET %d Sat %d PRN %d AZ %f EL %f SNR %f\n", s, i, SatelliteInfo.PRN,
                                RadToDeg(SatelliteInfo.Azimuth), RadToDeg(SatelliteInfo.Elevation),
                                SatelliteInfo.SNR);
                setGNSSSignal(i, SatelliteInfo.SNR);
                setGNSSSky(i, RadToDeg(SatelliteInfo.Azimuth), RadToDeg(SatelliteInfo.Elevation));
            }

            setMeter(SCR_GNSS, SATS, (double)NumberOfSVs, "");

        } break;

        case 130310: {
            // Outside Environmental
            unsigned char instance;
            double WaterTemperature;
            double OutsideAmbientAirTemperature;
            double AtmosphericPressure;

            bool s = ParseN2kPGN130310(msg, instance, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure);

            setMeter(SCR_ENV, SEATEMP, KelvinToC(WaterTemperature), "°C");
        } break;

        case 130312: {
            // Temperature
            unsigned char instance;
            unsigned char TempInstance;
            tN2kTempSource TempSource;
            double ActualTemperature;
            double SetTemperature;

            bool s = ParseN2kPGN130312(msg, instance, TempInstance, TempSource, ActualTemperature, SetTemperature);

            setMeter(SCR_ENV, AIRTEMP, KelvinToC(ActualTemperature), "°C");
        } break;

        case 130313: {
            // Humidity
            unsigned char instance;
            unsigned char HumidityInstance;
            tN2kHumiditySource HumiditySource;
            double ActualHumidity;

            bool s = ParseN2kPGN130313(msg, instance, HumidityInstance, HumiditySource, ActualHumidity);

            setMeter(SCR_ENV, HUM, ActualHumidity, "%");
        } break;

        case 130314: {
            // Pressure
            unsigned char instance;
            unsigned char PressureInstance;
            tN2kPressureSource PressureSource;
            double Pressure;

            bool s = ParseN2kPGN130314(msg, instance, PressureInstance, PressureSource, Pressure);

            setMeter(SCR_ENV, PRESSURE, Pressure / 100, "");
        } break;

        default:
            // Catch any messages we don't expect
            break;
    }
}