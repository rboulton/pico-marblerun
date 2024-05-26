#include <vector>
#include "plasma2040.hpp"

class Pixel;
class Grid {
    public:
	Grid(std::vector<uint32_t> & heights);
	~Grid();
	uint32_t columns();
	uint32_t height(uint32_t column);
        void set(uint32_t column, uint32_t row, float h, float s, float v);
        void add(uint32_t column, uint32_t row, float h, float s, float v);
	void show();
	void fade();
	void fill(float h, float s, float v);

    private:
	plasma::WS2812 * led_strip;
	std::vector<uint32_t> heights;
	uint32_t rows;

	std::vector<std::vector<Pixel>> pixels;
};
