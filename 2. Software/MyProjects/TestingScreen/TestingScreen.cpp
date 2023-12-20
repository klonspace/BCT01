#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"
#include "hc165.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::seed;

#define PIN_OLED_DC 9
#define PIN_OLED_RESET 30

DaisySeed               hw;
static ShiftRegister165 shr;


OledDisplay<SSD130x4WireSpi128x64Driver>         display; /**< & */
OledDisplay<SSD130x4WireSpi128x64Driver>::Config display_config;

int numDevices = 1;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}


void printbinary(long x)
{
    char str[numDevices * 8 + 1];
    str[numDevices * 8] = '\0';
    int i               = numDevices * 8;
    while(i)
    {
        i--;
        str[i] = (x & 1) + '0';
        x >>= 1;
    }
    hw.PrintLine("%s", str);
}

int  y         = 0;
long readValue = 0;

int main(void)
{
    hw.Init();
    // hw.SetAudioBlockSize(4); // number of samples handled per callback
    // hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    // hw.StartAudio(AudioCallback);

    display_config.driver_config.transport_config.pin_config.dc
        = hw.GetPin(PIN_OLED_DC);
    display_config.driver_config.transport_config.pin_config.reset
        = hw.GetPin(PIN_OLED_RESET);
    display.Init(display_config);

    dsy_gpio_pin pin_cfg[] = {D0, D1, D2, D3};
    shr.Init(pin_cfg, 1);


    while(1)
    {
        readValue = shr.Read();
        y         = 0;
        display.Fill(false);
        char str[numDevices * 8 + 1];
        display.SetCursor(0, 0);
        int i = numDevices * 8;
        while(i)
        {
            i--;
            str[i] = (readValue & 1) + '0';
            readValue >>= 1;
        }
        char* cstr = &str[0];
        display.WriteString(cstr, Font_7x10, true);
        display.Update();
    }
}
