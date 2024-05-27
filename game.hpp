#pragma once
#include <vector>

#include "lights.hpp"

class GameEvent {
    public:
        GameEvent(uint type, uint32_t time, uint gpio) :
            type(type),
            time(time),
            gpio(gpio)
        {}

        uint type;
        uint32_t time;
        uint gpio;
};

class Game {
    public:
        Game(Grid * grid);
        ~Game();
        void start();
        void break_beam_event(uint gpio, uint32_t start_time, uint32_t end_time);

    private:
        Grid * grid;
        std::vector<GameEvent> events;

        uint32_t track1_start;
        uint32_t track2_start;
};


