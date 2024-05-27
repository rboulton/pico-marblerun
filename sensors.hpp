#pragma once
#include <map>

class BreakBeamSensor;
class BreakBeamSensors {
    public:
        BreakBeamSensors() :
            sensors()
        {
        }

        void add(uint gpio, BreakBeamSensor * sensor)
        {
            sensors[gpio] = sensor;
        }

        void remove(uint gpio)
        {
            sensors.erase(gpio);
        }

        void event(uint gpio, uint32_t event_mask);

    private:
        std::map<uint, BreakBeamSensor *> sensors;
};

typedef void (*break_beam_callback_t)(uint gpio, uint32_t start_time, uint32_t end_time);

class BreakBeamSensor {
    public:
        BreakBeamSensor(uint gpio);

        void on_event(break_beam_callback_t callback) {
            this->callback = callback;
        }

        ~BreakBeamSensor();

        /** Called once for each detection of an object moving through the beam
         *
         *  If another apparent event starts within break_beam_settle_time, it
         *  will be ignored. Events less than break_beam_blip_time will be
         *  ignored, unless there are following events also within a further
         *  break_beam_blip_time; this will only be called once such events have
         *  been happening for longer than break_beam_blip time.
         */
        void event(uint32_t start_time, uint32_t end_time) {
            // printf("beam %d broken at %d for %d\n", gpio, start_time, end_time - start_time);
            if (callback) {
                callback(gpio, start_time, end_time);
            }
        }

        // Called when the beam is broken
        void fall(uint32_t time);

        // Called when the beam is restored
        void rise(uint32_t time);

    private:
        uint gpio;
        uint32_t last_fall;
        uint32_t last_rise;
        uint32_t last_event;
        break_beam_callback_t callback;

        enum {
            NO_EVENT, // No event in progress.
            IN_EVENT, // Event in progress, not yet reported
            IN_REPORTED_EVENT, // Event in progress, but it's already been reported
            MAYBE_EVENT // Might be in an event (we've seen a fall quickly followed by a rise).
        } status;
};


