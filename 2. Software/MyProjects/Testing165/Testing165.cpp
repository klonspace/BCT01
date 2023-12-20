#include "daisy_seed.h"
#include "daisysp.h"
#include "hc165.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::seed;
#
DaisySeed               hw;
static ShiftRegister165 shr;

long readValue;
long oldValue;

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
    char str[numDevices*8+1];
    str[numDevices*8] = '\0';
    int i  = numDevices*8;
    while(i)
    {
        i--;
        str[i] = (x & 1) + '0';
        x >>= 1;
    }
    hw.PrintLine("%s", str);
}

int main(void)
{
    hw.Init();
    hw.StartLog(true);

    // hw.SetAudioBlockSize(4); // number of samples handled per callback
    // hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    // hw.StartAudio(AudioCallback);

    dsy_gpio_pin pin_cfg[] = {D0, D1, D2, D3};
    shr.Init(pin_cfg, 1);


    while(1)
    {
        readValue = shr.Read();
        if(readValue != oldValue)
        {
            printbinary(readValue);
            oldValue = readValue;
        }

        hw.DelayMs(1);
    }
}
