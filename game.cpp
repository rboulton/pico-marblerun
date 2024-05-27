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
    grid(grid)
{}

Game::~Game()
{
    if (active_game == this) {
        active_game = NULL;
    }
}

void Game::start() {
    active_game = this;
    uint START1 = 10;
    uint END1 = 12;
    uint START2 = 11;
    uint END2 = 13;

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
            grid->fill(0.3, 1, 0.5);
        }
        events.erase(events.begin(), i);

        grid->show();
        sleep_ms(1000 / FRAMES_PER_SECOND);
    }
}

void Game::break_beam_event(uint gpio, uint32_t start_time, uint32_t end_time)
{
    // printf("beam %d broken at %d for %d\n", gpio, start_time, end_time - start_time);
    events.push_back(GameEvent(0, gpio, start_time));
}
