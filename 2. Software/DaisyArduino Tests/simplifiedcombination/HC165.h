
#include "Arduino.h"

const int kMaxSr165DaisyChain
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
    void Init(uint32_t *pin_cfg, int num_daisy_chained = 1);

    /** Writes the states of shift register out to the connected devices.
     */
    long Read();

  private:
    uint32_t pin_[NUM_PINS];
    int   num_devices_;
};

