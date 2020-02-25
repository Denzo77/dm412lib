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
        cli();
        
        // set_clock_pin(LOW);
        PORTB &= ~(1 << PB0);

        for (auto i = (0x8000); i != 0; i >>= 1) {
            // set_clock_pin(LOW);
            // fastDigitalWrite(config.clock_pin, LOW);
            // set_data_pin((i & val));
            // fastDigitalWrite(config.data_pin, (i & val));
            // fastDigitalWrite(config.clock_pin, HIGH);

            // set data_pin
            const auto set_bit = (i & val) ? true : false;
            PORTB &= ~(1 << PB0);
            if (set_bit) {
                PORTB |= (1 << PB1);
            } else {
                PORTB &= ~(1 << PB1);
            }
            clock_delay();
            // clock signal
            PORTB |= (1 << PB0);
            clock_delay();
        }
        sei();
    }

public:
    DM412Driver(DM412DriverConfig config) : config(config) {}

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

    void latch() {
        set_clock_pin(HIGH);
        PORTB |= (1 << PB0);

        set_data_pin(LOW);

        clock_delay();
        cli();

        for (auto i=0; i != 8; ++i) {
            // set_data_pin(LOW);
            PORTB &= ~(1 << PB1);
            clock_delay();
            // set_data_pin(HIGH);
            PORTB |= (1 << PB1);

            clock_delay();
        }
        // set_data_pin(LOW);
        PORTB &= ~(1 << PB1);
        sei();

    }
};


DM412Driver leds {{
        8, // clock_pin
        9, // data_pin
        1, // Clock speed in us per pulse
}};

void setup() {
    Serial.begin(115200);
    Serial.println("hello");

    leds.begin();
}

void loop() {
    Serial.println("Go!");

    // leds.write({'a', 'b', 0x4325});
    // for (auto i = 0; i != 4; ++i) {
    //     // leds.write({(i+1) * 1000, 0, 0});
    //     // leds.latch();
    // }
    leds.write({0xffff, 0xffff, 0xffff});
    leds.write({0xffff, 0, 0});
    leds.write({0x0, 0xffff, 0});
    leds.write({0x0, 0, 0xffff});
    leds.latch();
    delay(1000);
}
