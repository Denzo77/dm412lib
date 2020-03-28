/**
 * SP-DM412 lights
 * 
 * SPI like:
 * - 20ns max clock pulse? The driver we have clocks at 5us pulses
 * - Clocked on rising clock edge
 * - MSBFirst
 * - End message with 8 times high pulse on serial, with clock held high.
 *   (20us pulses on driver)
 * 
 */
#define MAX_N_LIGHTS 10

#include "fastDigitalIO.h"

struct DM412DriverConfig
{
    uint8_t clock_pin;
    uint8_t data_pin;
    uint8_t clock_rate;
};

struct LightLevels
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
};

class DM412Driver
{
    const DM412DriverConfig config;
    const int n_lights;
    LightLevels lightArray[MAX_N_LIGHTS];

    inline void clock_delay() {
        delayMicroseconds(config.clock_rate);
    }

    void set_clock_pin(bool is_high) {
        fastDigitalWrite(config.clock_pin, is_high);
    }

    void set_data_pin(bool is_high) {
        fastDigitalWrite(config.data_pin, is_high);
    }
    
    // FIXME: Switch to using set pin fns instead of registers.
    // + actually use val.
    void write_channel(const uint16_t val) {
        // no interupts
        cli();
        
        set_clock_pin(LOW);
        // PORTB &= ~(1 << PB0);
        
        // i starts as 2^15 then is shifted one to the right each time
        // 
        for (auto i = (0x8000); i != 0; i >>= 1) {
            set_clock_pin(LOW);
            // set bit at position i
            set_data_pin((i & val));
            clock_delay();
            set_clock_pin(HIGH);
            clock_delay();
        }
        // add interupts
        sei();
    }

public:
    DM412Driver(DM412DriverConfig config, int n_lights) : config(config), n_lights(n_lights) {
        // initialise all lights to zero to start
        for (int i = 0; i < n_lights; i++) {
            lightArray[i] = {0, 0, 0};
        }
    } 

    void begin() {
        // TODO: Swap for faster functions
        set_clock_pin(HIGH);
        set_data_pin(HIGH);
        pinMode(config.clock_pin, OUTPUT);
        pinMode(config.data_pin, OUTPUT);
    }

    void end() {
        // TODO: Swap for faster functions
        pinMode(config.clock_pin, INPUT_PULLUP);
        pinMode(config.data_pin, INPUT_PULLUP);
    }

    void write(LightLevels level) {
        write_channel(level.r);
        write_channel(level.g);
        write_channel(level.b);
    }

    void set_light(int light, LightLevels level){
        lightArray[light] = level;
    }
    void write_lights(){
        for (int i = 0; i < n_lights; i++) {
            write(lightArray[i]);
        }
        latch();
    }

    int get_n_lights(){
        return n_lights;
    }

    void latch() {
        set_clock_pin(HIGH);
        set_data_pin(LOW);
        clock_delay();
        cli();

        for (auto i=0; i != 8; ++i) {
            set_data_pin(LOW);
            clock_delay();
            set_data_pin(HIGH);
            clock_delay();
        }
        set_data_pin(LOW);
        sei();

    }
};


DM412Driver leds {
    {
        8, // clock_pin
        9, // data_pin
        1, // Clock speed in us per pulse
    },
    4 // n_lights
};
int i = 0;
void setup() {
    leds.begin();
}

void loop() {
    leds.set_light(i, {0x0, 0x0, 0xffff});
    leds.write_lights();
    leds.set_light(i, {0x0, 0x0, 0x0});
    i = (i + 1) % leds.get_n_lights();
    delay(200);
}