#pragma once
#ifndef DSY_DEV_SR_165_H
#define DSY_DEV_SR_165_H

#include "daisy_core.h"
#include "daisy_seed.h"
#include "daisysp.h"
#include "per/gpio.h"

using namespace daisy;
using namespace daisysp;

const size_t kMaxSr165DaisyChain
    = 16; /**< Maximum Number of chained devices Connect device's QH' pin to the next chips serial input*/

/** @addtogroup shiftregister
    @{
    */

/**
   @brief Device Driver for 8-bit shift register. \n 
   CD74HC165 - 8-bit serial to parallel output shift
   @author shensley
   @date May 2020
*/
class ShiftRegister165
{
  public:
    /** The following pins correspond to the hardware connections
    to the 165. 
  */
    enum Pins
    {
        PIN_LOAD, /** LOAD corresonds to Pin 1 "SH/LD" */
        PIN_CLK,   /** CLK corresponds to Pin 2 "CLK" */
        PIN_DATA,  /** DATA corresponds to Pin 7 "QH*" */
        PIN_CLK_ENABLE, /** CLK_ENABLE corresponds to Pin 15 "CLK/INH" */
        NUM_PINS, /** _SRCLR_ is not added here, but is tied to 3v3 on test hardware. */
    };
    ShiftRegister165() {}
    ~ShiftRegister165() {}

    /** 
    Initializes the GPIO, and data for the ShiftRegister
     * \param pin_cfg is an array of dsy_gpio_pin corresponding the the Pins enum above.
     * \param num_daisy_chained (default = 1) is the number of 165 devices daisy chained together.
     */
    void Init(dsy_gpio_pin *pin_cfg, size_t num_daisy_chained = 1);

    /** Writes the states of shift register out to the connected devices.
     */
    long Read();

  private:
    dsy_gpio pin_[NUM_PINS];
    size_t   num_devices_;
};

#endif
/** @} */
