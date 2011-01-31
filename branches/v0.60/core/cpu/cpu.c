/**************************************************************************/
/*! 
    @file     cpu.c
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

    @section DESCRIPTION

    Initialises the CPU and any core clocks.  By default, the core clock
    is set to run at 12MHz.  In order to reduce power consumption all pins
    are set to GPIO and input by cpuInit.

    @section EXAMPLE
    @code
    #include "lpc111x.h"
    #include "core/cpu/cpu.h"

    int main (void)
    {
      // Initialise the CPU and setup the PLL
      cpuInit();
      
      while(1)
      {
      }
    }
    @endcode 
    
    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2010, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "cpu.h"
#include "core/gpio/gpio.h"

/**************************************************************************/
/*! 
    @brief Configures the PLL and main system clock

    The speed at which the MCU operates is set here using the SCB_PLLCTRL
    register, and the SCB_PLLCLKSEL register can be used to select which
    oscillator to use to generate the system clocks (the internal 12MHz
    oscillator or an external crystal).
*/
/**************************************************************************/
void cpuPllSetup (cpuMultiplier_t multiplier)
{
  uint32_t i;

  // Power up system oscillator
  SCB_PDRUNCFG &= ~(SCB_PDRUNCFG_SYSOSC_MASK);

  // Setup the crystal input (bypass disabled, 1-20MHz crystal)
  SCB_SYSOSCCTRL = (SCB_SYSOSCCTRL_BYPASS_DISABLED | SCB_SYSOSCCTRL_FREQRANGE_1TO20MHZ);

  for (i = 0; i < 200; i++)
  {
    __asm volatile ("NOP");
  }

  // Configure PLL
  SCB_PLLCLKSEL = SCB_CLKSEL_SOURCE_MAINOSC;    // Use the external crystal
  SCB_PLLCLKUEN = SCB_PLLCLKUEN_UPDATE;         // Update clock source
  SCB_PLLCLKUEN = SCB_PLLCLKUEN_DISABLE;        // Toggle update register once
  SCB_PLLCLKUEN = SCB_PLLCLKUEN_UPDATE;         // Update clock source again
  
  // Wait until the clock is updated
  while (!(SCB_PLLCLKUEN & SCB_PLLCLKUEN_UPDATE));

  // Set clock speed
  switch (multiplier)
  {
    case CPU_MULTIPLIER_2:
      SCB_PLLCTRL = (SCB_PLLCTRL_MULT_2 | (1 << SCB_PLLCTRL_DIV_BIT));
      break;
    case CPU_MULTIPLIER_3:
      SCB_PLLCTRL = (SCB_PLLCTRL_MULT_3 | (1 << SCB_PLLCTRL_DIV_BIT));
      break;
    case CPU_MULTIPLIER_4:
      SCB_PLLCTRL = (SCB_PLLCTRL_MULT_4 | (1 << SCB_PLLCTRL_DIV_BIT));
      break;
    case CPU_MULTIPLIER_1:
    default:
      SCB_PLLCTRL = (SCB_PLLCTRL_MULT_1 | (1 << SCB_PLLCTRL_DIV_BIT));
      break;
  }

  // Enable system PLL
  SCB_PDRUNCFG &= ~(SCB_PDRUNCFG_SYSPLL_MASK);

  // Wait for PLL to lock
  while (!(SCB_PLLSTAT & SCB_PLLSTAT_LOCK));
  
  // Setup main clock
  SCB_MAINCLKSEL = SCB_MAINCLKSEL_SOURCE_SYSPLLCLKOUT;
  SCB_MAINCLKUEN = SCB_MAINCLKUEN_UPDATE;       // Update clock source
  SCB_MAINCLKUEN = SCB_MAINCLKUEN_DISABLE;      // Toggle update register once
  SCB_MAINCLKUEN = SCB_MAINCLKUEN_UPDATE;

  // Wait until the clock is updated
  while (!(SCB_MAINCLKUEN & SCB_MAINCLKUEN_UPDATE));

  // Set system AHB clock
  SCB_SYSAHBCLKDIV = SCB_SYSAHBCLKDIV_DIV1;

  // Enabled IOCON clock for I/O related peripherals
  SCB_SYSAHBCLKCTRL |= SCB_SYSAHBCLKCTRL_IOCON;
}

/**************************************************************************/
/*! 
    @brief Initialises the CPU, setting up the PLL, etc.
*/
/**************************************************************************/
void cpuInit (void)
{
  gpioInit();

  // Set all GPIO pins to input by default
  GPIO_GPIO0DIR &= ~(GPIO_IO_ALL);
  GPIO_GPIO1DIR &= ~(GPIO_IO_ALL);
  GPIO_GPIO2DIR &= ~(GPIO_IO_ALL);
  GPIO_GPIO3DIR &= ~(GPIO_IO_ALL);

  // Alternatively, you may want to set all pins to output and driven low
  // for the lowest possible power consumption.  Uncommenting the code
  // below will accomplish this.  (Note: Special care will need to be
  // taken with the SWD/JTAG pins if you are using a HW debuggers.

  // /* Disable all internal pull-ups */
  // gpioSetPullup(&IOCON_nRESET_PIO0_0, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_1, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_2, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_3, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_4, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_5, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_6, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_7, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_8, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO0_9, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_JTAG_TCK_PIO0_10, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_JTAG_TDI_PIO0_11, gpioPullupMode_Inactive);
  //
  // gpioSetPullup(&IOCON_JTAG_TMS_PIO1_0, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_JTAG_TDO_PIO1_1, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_JTAG_nTRST_PIO1_2, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_SWDIO_PIO1_3, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_4, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_5, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_6, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_7, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_8, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_9, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_10, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO1_11, gpioPullupMode_Inactive);
  //
  // gpioSetPullup(&IOCON_PIO2_0, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_1, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_2, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_3, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_4, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_5, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_6, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_7, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_8, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_9, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_10, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO2_11, gpioPullupMode_Inactive);
  //
  // gpioSetPullup(&IOCON_PIO3_0, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO3_1, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO3_2, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO3_3, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO3_4, gpioPullupMode_Inactive);
  // gpioSetPullup(&IOCON_PIO3_5, gpioPullupMode_Inactive);
  
  // /* Set all GPIO pins to output */
  // GPIO_GPIO0DIR &= ~(GPIO_IO_ALL);
  // GPIO_GPIO1DIR &= ~(GPIO_IO_ALL);
  // GPIO_GPIO2DIR &= ~(GPIO_IO_ALL);
  // GPIO_GPIO3DIR &= ~(GPIO_IO_ALL);
  
  // /* Set all GPIO pins low */
  // GPIO_GPIO0DATA &= ~(GPIO_IO_ALL);
  // GPIO_GPIO1DATA &= ~(GPIO_IO_ALL);
  // GPIO_GPIO2DATA &= ~(GPIO_IO_ALL);
  // GPIO_GPIO3DATA &= ~(GPIO_IO_ALL);

  // Configure PLL and main system clock
  cpuPllSetup(CPU_MULTIPLIER_3);
}

/**************************************************************************/
/*! 
    @brief Get's the CPU Device ID
*/
/**************************************************************************/
cpuDeviceID_t cpuGetDeviceID (void)
{
  switch (SCB_DEVICEID)
  {
    case (SCB_DEVICEID_LPC1111_101):
      return cpuDeviceID_LPC1111;
    case (SCB_DEVICEID_LPC1111_201):
      return cpuDeviceID_LPC1111;
    case (SCB_DEVICEID_LPC1112_101):
      return cpuDeviceID_LPC1112;
    case (SCB_DEVICEID_LPC1112_201):
      return cpuDeviceID_LPC1112;
    case (SCB_DEVICEID_LPC1113_201):
      return cpuDeviceID_LPC1113;
    case (SCB_DEVICEID_LPC1113_301):
      return cpuDeviceID_LPC1113;
    case (SCB_DEVICEID_LPC1114_201):
      return cpuDeviceID_LPC1114;
    case (SCB_DEVICEID_LPC1114_301):
      return cpuDeviceID_LPC1114;
    default:
      return cpuDeviceID_Unknown;
  }
  return cpuDeviceID_Unknown;
}