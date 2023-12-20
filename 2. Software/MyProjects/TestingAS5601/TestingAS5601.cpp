#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::seed;

DaisySeed hw;
dsy_gpio  pin_[2];

int oldA, newA, oldB, newB;


int main(void)
{
    hw.Init();
    hw.StartLog(true);
    pin_[0].pin  = D11;
    pin_[0].mode = DSY_GPIO_MODE_INPUT;
    pin_[0].pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&pin_[0]);

    pin_[1].pin  = D12;
    pin_[1].mode = DSY_GPIO_MODE_INPUT;
    pin_[1].pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&pin_[1]);

    while(1)
    {
        newA = dsy_gpio_read(&pin_[0]);
        newB = dsy_gpio_read(&pin_[0]);
        if(oldA != newA)
        {
            hw.PrintLine("%lu, A changed", (unsigned long)System::GetNow());
			oldA = newA;
        }
		 if(oldB != newB)
        {
            hw.PrintLine("%lu, B changed", (unsigned long)System::GetNow());
			oldB = newB;
        }
    }
}
