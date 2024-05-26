#include <stdio.h>
#include <math.h>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <string>

#include "pico/stdlib.h"
#include "pico/rand.h"

#include "plasma2040.hpp"

#include "common/pimoroni_common.hpp"
#include "rgbled.hpp"
#include "button.hpp"
#include "analog.hpp"

#include "lights.hpp"

using namespace pimoroni;
using namespace plasma;

// How many times the LEDs will be updated per second
const uint UPDATES = 60;

const float gravity = 0.00005;
const float bounce = 0.9;

RGBLED led(plasma2040::LED_R, plasma2040::LED_G, plasma2040::LED_B);
Analog sense(plasma2040::CURRENT_SENSE, plasma2040::ADC_GAIN, plasma2040::SHUNT_RESISTOR);

float rand_0_1() {
    return ((float)get_rand_32()) / float(0xffffffffu);
}

class Pixel {
    public:
        Pixel() : h(0.0), s(0.0), v(0.0) {}

        void set(float h_, float s_, float v_) {
            h = h_;
            s = s_;
            v = v_;
        }

        void add(float h_, float s_, float v_) {
            h = (h + h_) / 2.0;
            s = (s + s_) / 2.0;
            v = std::max(v, v_);
        }

        float h;
        float s;
        float v;
};

Grid::Grid(std::vector<uint32_t> & heights) :
    heights(heights),
    rows(*std::max_element(heights.begin(), heights.end())),
    led_strip(NULL)
{
    // WS28X-style LEDs with a single signal line. AKA NeoPixel
    // by default the WS2812 LED strip will be 400KHz, RGB with no white element
    led_strip = new WS2812(std::accumulate(heights.begin(), heights.end(), 0), pio0, 0, plasma2040::DAT);
    led_strip->start(UPDATES);

    for (auto j = heights.begin(); j != heights.end(); j++) {
        pixels.push_back(std::vector<Pixel>());
        std::vector<Pixel> & column = pixels.back();
        for (uint32_t i = 0; i < rows; i++) {
            column.push_back(Pixel());
        }
    }
}

Grid::~Grid() {
    delete led_strip;
    led_strip = NULL;
}

uint32_t Grid::columns() {
    return heights.size();
}

uint32_t Grid::height(uint32_t column) {
    return heights[column];
}

void Grid::set(uint32_t column, uint32_t row, float h, float s, float v) {
    if (column < 0 || column >= columns() || row < 0 || row >= rows) {
        return;
    }
    pixels[column][row].set(h, s, v);
}

void Grid::add(uint32_t column, uint32_t row, float h, float s, float v) {
    if (column < 0 || column >= columns() || row < 0 || row >= rows) {
        return;
    }
    pixels[column][row].add(h, s, v);
    // printf("Pixel %d %d = %f %f %f\n", column, row, h, s, v);
}

void Grid::show() {
    uint32_t start_of_row = 0;
    for (uint32_t column = 0; column < columns(); column++) {
        auto & col_pixels = pixels[column];
        // printf("Column %d, height %d, rowstart %d, rowend %d\n", column, heights[column], rows - heights[column], col_pixels.size());
        for (uint32_t row = rows - heights[column]; row < col_pixels.size(); row++) {
            auto & pixel = col_pixels[row];

            uint32_t i = start_of_row;
            if (column % 2 == 0) {
                i += row;
            } else {
                i += col_pixels.size() - 1 - row;
            }

            //printf("Setting %d %d i=%d/%d to %f %f %f\n", column, row, i, led_strip->num_leds, pixel.h, pixel.s, pixel.v);
            led_strip->set_hsv(i, pixel.h, pixel.s, pixel.v);
        }
        start_of_row += heights[column];
    }
    // printf("Done\n");
}

void Grid::fade() {
    for(auto i = pixels.begin(); i != pixels.end(); ++i) {
        for(auto j = i->begin(); j != i->end(); ++j) {
            Pixel & pixel = *j;
            pixel.v = pixel.v * 0.98;
            if (pixel.v < 0.0001) {
                pixel.h = 0;
                pixel.s = 0;
            }

        }
    }
}

void Grid::fill(float h, float s, float v) {
    for(auto i = pixels.begin(); i != pixels.end(); ++i) {
        for(auto j = i->begin(); j != i->end(); ++j) {
            Pixel & pixel = *j;
            pixel.h = h;
            pixel.s = s;
            pixel.v = v;
        }
    }
}

class Spark {
    public:
        Spark(float pos, uint32_t column, float dpos, float spread, float h, float s, float v, uint32_t age)
              : pos(pos),
	        column(column),
		dpos(dpos),
	       	spread(spread),
	       	h(h),
	       	s(s),
	       	v(v),
	       	finished(false),
	       	age(age * UPDATES)
	{}

	bool finished;
	float age;

	void show(Grid & grid) {
	    uint32_t height = grid.height(column);
	    uint32_t y = floor(pos * height);
	    if (y < 0) {
	        y = 0;
	    }
	    else if (y >= height) {
		y = height - 1;
	    }

	    uint32_t offset = floor(spread * height);
	    // printf("pos=%f col=%d y=%d to %f %f %f offset=%d\n", pos, column, y, h, s, v, offset);

	    grid.add(column, y, h, s, v);
	    for (;
                 offset--;
		 offset > 0) {
	        grid.add(column, y-offset, h, s, v * 0.5);
	        grid.add(column, y+offset, h, s, v * 0.5);
	    }
	}

	void update() {
    	    // printf("Move to %f at %f\n", pos, dpos);
	    pos += dpos;
	    dpos -= gravity;
	    if (pos > 1.0) {
		pos = 1.0;
		dpos = -dpos * bounce;
	    }
	    if (pos < 0.0) {
		pos = 0.0;
		dpos = -dpos * bounce;
	    }

	    if (age <= 0) {
		finished = true;
	    } else {
	        age -= 1;
	    }
	}

        std::string describe() {
	    return std::string("spark(") +
		   std::to_string(column) +
		   std::string(", ") +
		   std::to_string(pos) +
		   std::string(")");
        }

    private:
	float pos;
	uint32_t column;
	float dpos;
	float spread;
	float h;
	float s;
	float v;
};

Spark newspark(uint32_t column) {
    float h = rand_0_1();
    float s = rand_0_1();
    float v = rand_0_1() * 0.5 + 0.5;
    float dpos = rand_0_1() * 0.03;
    float spread = rand_0_1() * 0.05 + 0.02;
    float age = rand_0_1() * 10 + 10.0;
    age = 10;
    return Spark(1.0, column, dpos, spread, h, s, v, age);
}

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

static BreakBeamSensors break_beam_sensors;

class BreakBeamSensor {
    public:
        BreakBeamSensor(uint gpio);

        ~BreakBeamSensor() {
            break_beam_sensors.remove(gpio);
        }

        // Called when the beam is broken
        void fall(uint32_t time) {
            uint32_t since_last = time - last_fall;
            printf("BreakBeam %d broken at %d: %d since last, %d since unbroken\n", gpio, time, since_last, time - last_rise);
            last_fall = time;
        }

        // Called when the beam is restored
        void rise(uint32_t time) {
            uint32_t since_last = time - last_rise;
            printf("BreakBeam %d unbroken at %d: %d since last, %d since broken\n", gpio, time, since_last, time - last_fall);
            last_rise = time;
        }

    private:
        uint gpio;
        uint32_t last_fall;
        uint32_t last_rise;
};

void gpio_breakbeam_callback(uint gpio, uint32_t event_mask) {
    break_beam_sensors.event(gpio, event_mask);
}

BreakBeamSensor::BreakBeamSensor(uint gpio) :
    gpio(gpio),
    last_fall(0),
    last_rise(0)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, false);
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_callback(gpio_breakbeam_callback);
    break_beam_sensors.add(gpio, this);
    irq_set_enabled(IO_IRQ_BANK0, true);

}

void BreakBeamSensors::event(uint gpio, uint32_t event_mask) {
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

uint32_t count_fall = 0;
uint32_t count_rise = 0;
uint32_t count_both = 0;

void start_irq(uint gpio) {
    new BreakBeamSensor(gpio);
}

int main() {
    stdio_init_all();
    //stdio_usb_init();
    //stdout_uart_init();
    //setup_default_uart();

    //stdio_set_driver_enabled(stdio_usb, true);
    //stdio_set_driver_enabled(stdio_uart, true);

    BreakBeamSensor beam_10(10);
    BreakBeamSensor beam_11(11);
    BreakBeamSensor beam_12(12);
    BreakBeamSensor beam_13(13);

    std::vector<Spark> sparks;
    std::vector<Spark> newsparks;

    led.set_rgb(0, 0, 0);

    uint count = 0;
    bool a_was_pressed = false;
    uint32_t next_column = 0;

    std::vector<uint32_t> columns;
    columns.push_back(60);
    columns.push_back(60);
    columns.push_back(60);
    columns.push_back(59);
    columns.push_back(60);
    Grid grid(columns);

    while(true) {
        // bool sw_pressed = user_sw.read();
        bool a_pressed = false; //button_a.read();
        // bool b_pressed = button_b.read();

        // if (sw_pressed) { while(!sparks.empty()) { sparks.pop_back(); } }
        if(false) {
            newsparks.push_back(newspark(next_column++));
            next_column %= columns.size();
        }

        if (a_pressed != a_was_pressed) {
            printf("A_pressed %d\n", a_pressed);
            if (a_pressed) {
                newsparks.push_back(newspark(next_column++));
                next_column %= columns.size();
            }
        }
        a_was_pressed = a_pressed;

        grid.fade();
        //grid.fill(0, 0, 0);
        //grid.fill((count % 100) * 0.01, 1, 0.5);
        for (uint32_t c = 0; c < grid.columns(); c++) {
            uint32_t height = grid.height(c);
            for (uint32_t z = 0; z < height; z++) {
                float h = fmod((count % 100) * 0.01 + c * 0.5 / grid.columns(), 1.0);
                grid.set(c, z, h, 1, 0.5);
            }
        }

        for(auto i = sparks.begin(); i != sparks.end(); ++i) {
            auto & spark = *i;
            spark.show(grid);
            spark.update();
        }

        grid.show();

        for(auto i = newsparks.begin(); i != newsparks.end(); ++i) {
            sparks.push_back(*i);
        }
        while(!newsparks.empty()) {
            newsparks.pop_back();
        }
        for(auto i = sparks.begin(); i != sparks.end();) {
            if (i->finished) {
                i = sparks.erase(i);
            } else {
                i++;
            }
        }

        // led.set_rgb(speed, 0, 255 - sparks.length());

        count += 1;
        if(count % UPDATES * 2 == 0) {
            // Display the current value once every second
            // grid.fill(0, 1, 1);
            //printf("Current = %f A. sparks = %d\n", sense.read_current(), sparks.size());
            for(auto i = sparks.begin(); i != sparks.end(); ++i) {
                auto & spark = *i;
                auto desc = spark.describe();
                printf("Spark %s\n", desc.c_str());
            }
            // count = 0;
        }

        // Sleep time controls the rate at which the LED buffer is updated
        // but *not* the actual framerate at which the buffer is sent to the LEDs
        sleep_ms(1000 / UPDATES);
    }
}
