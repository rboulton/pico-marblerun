#include <stdio.h>

#include "game.hpp"
#include "sensors.hpp"

Game * active_game = NULL;

const uint FRAMES_PER_SECOND = 100;

void break_beam_event_callback(uint gpio, uint32_t start_time, uint32_t end_time) {
    if (active_game) {
        active_game->break_beam_event(gpio, start_time, end_time);
    }
}

Game::Game(Grid * grid) :
    grid(grid),
    track1_start(0),
    track2_start(0)
{}

Game::~Game()
{
    if (active_game == this) {
        active_game = NULL;
    }
}

void Game::start() {
    active_game = this;
    const uint START1 = 10;
    const uint END1 = 12;
    const uint START2 = 13;
    const uint END2 = 11;

    const float HUE_BLUE = 192.0 / 360;
    const float HUE_GOLD = 57.0 / 360;
    const float HUE_PURPLE = 293.0 / 360;
    const float HUE_GREEN = 120.0 / 360;

    BreakBeamSensor beam_1(START1);
    BreakBeamSensor beam_2(END1);
    BreakBeamSensor beam_3(START2);
    BreakBeamSensor beam_4(END2);
    beam_1.on_event(break_beam_event_callback);
    beam_2.on_event(break_beam_event_callback);
    beam_3.on_event(break_beam_event_callback);
    beam_4.on_event(break_beam_event_callback);

    while (true) {
        grid->fade();
        auto i = events.begin();
        for (; i != events.end(); i++) {
            printf("GameEvent %d\n", i->gpio);
            switch (i->gpio) {
                case START1:
                    track1_start = i->time;
                    grid->fill(HUE_GREEN, 1, 0.5);
                    break;
                case END1:
                    grid->fill(HUE_GOLD, 1, 0.5);
                    break;
                case START2:
                    track2_start = i->time;
                    grid->fill(HUE_BLUE, 1, 0.5);
                    break;
                case END2:
                    grid->fill(HUE_PURPLE, 1, 0.5);
                    break;
            }
        }
        events.erase(events.begin(), i);

        grid->show();
        sleep_ms(1000 / FRAMES_PER_SECOND);
    }
}

void Game::break_beam_event(uint gpio, uint32_t start_time, uint32_t end_time)
{
    // printf("beam %d broken at %d for %d\n", gpio, start_time, end_time - start_time);
    events.push_back(GameEvent(0, start_time, gpio));
}
