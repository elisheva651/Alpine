#include "gps.h"
#include "config.h"
#include "shared.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "minmea.h"

#include <string.h>
#include <stdio.h>

// ============================================================================
// GPS Module — UART + NMEA parsing via minmea
// ============================================================================

#define NMEA_BUF_SIZE 128

static char s_nmea_buf[NMEA_BUF_SIZE];
static int s_nmea_pos = 0;

void gps_init(void) {
    uart_init(GPS_UART_ID, GPS_BAUD_RATE);
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);

    // Enable FIFO
    uart_set_fifo_enabled(GPS_UART_ID, true);
}

static void process_sentence(const char *sentence) {
    switch (minmea_sentence_id(sentence, false)) {
    case MINMEA_SENTENCE_GGA: {
        struct minmea_sentence_gga frame;
        if (minmea_parse_gga(&frame, sentence)) {
            SHARED_LOCK();
            g_shared.latitude = minmea_tocoord(&frame.latitude);
            g_shared.longitude = minmea_tocoord(&frame.longitude);
            g_shared.altitude_gps = minmea_tofloat(&frame.altitude);
            g_shared.satellites = frame.satellites_tracked;

            switch (frame.fix_quality) {
            case 0: g_shared.fix = GPS_FIX_NONE; break;
            case 1: g_shared.fix = GPS_FIX_3D; break;   // GPS fix
            case 2: g_shared.fix = GPS_FIX_3D; break;   // DGPS fix
            default: g_shared.fix = GPS_FIX_2D; break;
            }

            g_shared.hour = frame.time.hours;
            g_shared.minute = frame.time.minutes;
            g_shared.second = frame.time.seconds;
            g_shared.time_valid = true;
            SHARED_UNLOCK();
        }
        break;
    }

    case MINMEA_SENTENCE_RMC: {
        struct minmea_sentence_rmc frame;
        if (minmea_parse_rmc(&frame, sentence)) {
            SHARED_LOCK();
            if (frame.valid) {
                g_shared.latitude = minmea_tocoord(&frame.latitude);
                g_shared.longitude = minmea_tocoord(&frame.longitude);
                g_shared.speed_kmh = minmea_tofloat(&frame.speed) * 1.852f; // knots → km/h
                g_shared.course = minmea_tofloat(&frame.course);
            }
            g_shared.day = frame.date.day;
            g_shared.month = frame.date.month;
            g_shared.year = 2000 + frame.date.year;
            SHARED_UNLOCK();
        }
        break;
    }

    case MINMEA_SENTENCE_GSA: {
        struct minmea_sentence_gsa frame;
        if (minmea_parse_gsa(&frame, sentence)) {
            SHARED_LOCK();
            switch (frame.fix_type) {
            case 1: g_shared.fix = GPS_FIX_NONE; break;
            case 2: g_shared.fix = GPS_FIX_2D; break;
            case 3: g_shared.fix = GPS_FIX_3D; break;
            }
            SHARED_UNLOCK();
        }
        break;
    }

    default:
        break;
    }
}

void gps_poll(void) {
    while (uart_is_readable(GPS_UART_ID)) {
        char c = uart_getc(GPS_UART_ID);

        if (c == '$') {
            // Start of new sentence
            s_nmea_pos = 0;
        }

        if (s_nmea_pos < NMEA_BUF_SIZE - 1) {
            s_nmea_buf[s_nmea_pos++] = c;
        }

        if (c == '\n') {
            s_nmea_buf[s_nmea_pos] = '\0';
            if (s_nmea_pos > 6) {
                process_sentence(s_nmea_buf);
            }
            s_nmea_pos = 0;
        }
    }
}

bool gps_has_fix(void) {
    SHARED_LOCK();
    bool has = (g_shared.fix >= GPS_FIX_2D);
    SHARED_UNLOCK();
    return has;
}
