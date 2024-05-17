#include <stdio.h>
#include <math.h>
#include <cstdint>
#include <vector>

#include "pico/stdlib.h"
#include "pico/rand.h"

#include "plasma2040.hpp"

#include "common/pimoroni_common.hpp"
#include "rgbled.hpp"
#include "button.hpp"
#include "analog.hpp"

using namespace pimoroni;
using namespace plasma;

// The speed that the LEDs will start cycling at
const uint DEFAULT_SPEED = 10;

// How many times the LEDs will be updated per second
const uint UPDATES = 60;


Button user_sw(plasma2040::USER_SW, Polarity::ACTIVE_LOW, 0);
Button button_a(plasma2040::BUTTON_A, Polarity::ACTIVE_LOW, 50);
Button button_b(plasma2040::BUTTON_B, Polarity::ACTIVE_LOW, 50);
RGBLED led(plasma2040::LED_R, plasma2040::LED_G, plasma2040::LED_B);
Analog sense(plasma2040::CURRENT_SENSE, plasma2040::ADC_GAIN, plasma2040::SHUNT_RESISTOR);

float rand_0_1() {
    return ((float)get_rand_32()) / float(0xffffffffu);
}

class Spark {
    public:
        Spark(float pos, float dpos, float h, float s, float v, uint32_t age) : pos(pos), dpos(dpos), h(h), s(s), v(v), finished(false), age(age) {
	}

	bool finished;
	uint32_t age;

	void show(WS2812 & led_strip) {
	    uint32_t i = floor(pos * led_strip.num_leds);
	    if (i < 0) {
	        i = 0;
	    }
	    else if (i >= led_strip.num_leds) {
		i = led_strip.num_leds - 1;
	    }
	    // printf("pos = %f. dpos = %f. set %d to %f %f %f\n", pos, i, h, s, v);
	    led_strip.set_hsv(i, h, s, v);
	}

	void update(std::vector<Spark> & newsparks) {
    	    // printf("Move to %f at %f\n", pos, dpos);
	    pos += dpos;
	    dpos -= 0.0005;
	    if (pos > 1.0) {dpos = -dpos;}
	    if (pos < 0.0) {dpos = -dpos;}
	    if (age == 0) {
		finished = true;
	    } else {
	        age -= 1;
	    }
	}

    private:
	float pos;
	float dpos;
	float h;
	float s;
	float v;
};

Spark newspark() {
    float h = rand_0_1();
    float s = rand_0_1();
    float v = rand_0_1() * 0.5 + 0.5;
    float d = rand_0_1() * 0.03;
    float age = rand_0_1() * 300;
    return Spark(0, d, h, s, v, age);
}

int main() {
  uint32_t leds = 192;

  stdio_init_all();

  std::vector<Spark> sparks;
  std::vector<Spark> newsparks;

  newsparks.push_back(newspark());

  led.set_rgb(0, 0, 0);

  // WS28X-style LEDs with a single signal line. AKA NeoPixel
  // by default the WS2812 LED strip will be 400KHz, RGB with no white element
  WS2812 led_strip(leds, pio0, 0, plasma2040::DAT);

  led_strip.start(UPDATES);

  uint count = 0;
  while(true) {
    bool sw_pressed = user_sw.read();
    bool a_pressed = button_a.read();
    bool b_pressed = button_b.read();

    if (sw_pressed) {
      while(!sparks.empty()) {
        sparks.pop_back();
      }
    }
    if (a_pressed) {
      newsparks.push_back(newspark());
    }
    if (b_pressed) {}

    for(auto i = 0u; i < led_strip.num_leds; ++i) {
      led_strip.set_rgb(i, 0, 0, 0);
    }

    for(auto i = sparks.begin(); i != sparks.end(); ++i) {
      auto & spark = *i;
      spark.show(led_strip);
      spark.update(newsparks);
    }

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
    if(count >= UPDATES) {
      // Display the current value once every second
      printf("Current = %f A. sparks = %d\n", sense.read_current(), sparks.size());
      count = 0;
    }

    // Sleep time controls the rate at which the LED buffer is updated
    // but *not* the actual framerate at which the buffer is sent to the LEDs
    sleep_ms(1000 / UPDATES);
  }
}
