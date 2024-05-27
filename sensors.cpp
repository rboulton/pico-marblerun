#include "pico/stdlib.h"

#include "sensors.hpp"

static BreakBeamSensors break_beam_sensors;

// Any break less than this time is ignored.
const uint32_t break_beam_blip_time = 1000;

// Any breaks closer together than this are combined.
const uint32_t break_beam_settle_time = 60000;


void gpio_breakbeam_callback(uint gpio, uint32_t event_mask) {
    break_beam_sensors.event(gpio, event_mask);
}

void
BreakBeamSensors::event(uint gpio, uint32_t event_mask) {
    auto i = sensors.find(gpio);
    if (i != sensors.end()) {
        uint32_t t = time_us_32();
        if (event_mask & GPIO_IRQ_EDGE_FALL) {
            i->second->fall(t);
        }
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            i->second->rise(t);
        }
    }
}


BreakBeamSensor::BreakBeamSensor(uint gpio) :
    gpio(gpio),
    last_fall(0),
    last_rise(0),
    last_event(0),
    callback(NULL),
    status(NO_EVENT)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, false);
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_callback(gpio_breakbeam_callback);
    break_beam_sensors.add(gpio, this);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

BreakBeamSensor::~BreakBeamSensor() {
    break_beam_sensors.remove(gpio);
}

void
BreakBeamSensor::fall(uint32_t time) {
    uint32_t since_last_fall = time - last_fall;
    uint32_t since_last_rise = time - last_rise;
    uint32_t since_last_event = time - last_event;
    last_fall = time;
    // printf("BreakBeam %d broken at %d: status = %d, since_last_fall = %d, since_last_rise = %d, since_last_event = %d\n", gpio, time, status, since_last_fall, since_last_rise, since_last_event);

    switch (status) {
        case NO_EVENT:
            if (since_last_event < break_beam_settle_time) {
                // Continue the event - though we won't report it.
                // Don't change the event time
                status = IN_REPORTED_EVENT;
            } else {
                // Start an event
                last_event = time;
                status = IN_EVENT;
            }
            break;
        case IN_EVENT:
        case IN_REPORTED_EVENT:
            // Not expected to happen - but just continue processing the event
            break;
        case MAYBE_EVENT:
            if (since_last_rise < break_beam_blip_time) {
                // A small gap - continue the event
                status = IN_EVENT;
            } else {
                // A bigger gap than the "blip" time - restart the event.
                last_event = time;
                status = IN_EVENT;
            }
            break;
    }
    // printf("Status -> %d\n", status);
}

void
BreakBeamSensor::rise(uint32_t time) {
    uint32_t since_last_fall = time - last_fall;
    uint32_t since_last_rise = time - last_rise;
    uint32_t since_event_start = time - last_event;
    // printf("BreakBeam %d unbroken at %d: status = %d, since_last_fall = %d, since_last_rise = %d, since_event_start = %d\n", gpio, time, status, since_last_fall, since_last_rise, since_event_start);
    last_rise = time;

    switch (status) {
        case NO_EVENT:
            // Not expected to happen; ignore
            break;
        case IN_EVENT:
            if (since_event_start < break_beam_blip_time) {
                // Too short - mark as maybe
                status = MAYBE_EVENT;
            } else {
                // An event detected!
                status = NO_EVENT;
                event(last_event, time);
            }
            break;
        case IN_REPORTED_EVENT:
            // We've already reported this - don't care if it's a blip.
            status = NO_EVENT;
            break;
        case MAYBE_EVENT:
            // Not expected to happen; ignore
            break;
    }
    // printf("Status -> %d\n", status);
}

