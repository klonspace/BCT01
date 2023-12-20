#include <algorithm>
#include "hc165.h"
#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

void ShiftRegister165::Init(dsy_gpio_pin *pin_cfg, size_t num_daisy_chained)
{
    // Initialize Pins as outputs
    for(size_t i = 0; i < NUM_PINS; i++)
    {
        pin_[i].pin = pin_cfg[i];
        pin_[i].mode
            = i == PIN_DATA ? DSY_GPIO_MODE_INPUT : DSY_GPIO_MODE_OUTPUT_PP;
        pin_[i].pull = DSY_GPIO_NOPULL;
        dsy_gpio_init(&pin_[i]);
    }

    dsy_gpio_write(&pin_[PIN_CLK], 0);
    dsy_gpio_write(&pin_[PIN_LOAD], 1);

    // std::fill(state_, state_ + kMaxSr165DaisyChain, 0x00);
    num_devices_ = num_daisy_chained;
    // Set to 1 device if out of range.
    if(num_devices_ == 0 || num_devices_ > kMaxSr165DaisyChain)
        num_devices_ = 1;
}


long ShiftRegister165::Read()
{
    dsy_gpio_write(&pin_[PIN_CLK_ENABLE], 0);
    System::DelayUs(50);
    dsy_gpio_write(&pin_[PIN_LOAD], 1);
    dsy_gpio_write(&pin_[PIN_CLK], 1);

    // Get data from 74HC165
    // dsy_gpio_write(&pin_[PIN_CLK], 1);

    long value = 0;
    long i;

    for(i = 0; i < 8*num_devices_; ++i)
    {
        value |= dsy_gpio_read(&pin_[PIN_DATA]) << (i);
        dsy_gpio_write(&pin_[PIN_CLK], 1);
        System::DelayUs(50);
        dsy_gpio_write(&pin_[PIN_CLK], 0);
    }
    // Write pulse to load pin
    dsy_gpio_write(&pin_[PIN_LOAD], 0);
    dsy_gpio_write(&pin_[PIN_CLK_ENABLE], 1);


    return value;
    // return 0;
}
