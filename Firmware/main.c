/*
* main.c
*
* Created: October 2014
* Author: Timothy Mitchell Thompkins
*
*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <stdio.h>

#include "pcd8544.h"
#include "images.h"
#include "DHT22.h"


int main()
{
  LcdInit();
  DHT22_Init();
  sei();

  DDRD = 0b11000000;
  DDRB = 0b00111101;
  TCCR1B |= (1 << CS12); //CS12 for prescale of 256

  DHT22_STATE_t state;
  DHT22_DATA_t sensor_data;

  uint8_t waitStatus = 0;
  uint16_t tempC, tempF, humid;
  uint8_t tempOffset = 2;
  uint8_t LED_State;
  uint8_t checkState = 0;

  unsigned char tempBufferC[20];
  unsigned char tempBufferF[20];
  unsigned char humidBuffer[20];

  unsigned char elapsedSeconds = 0; // Make a new counter variable and initialize to zero

  while (1){

    LED_State = (PIND & 1<<PD1) ? 1 : 0; //Check PD1, if high set LED_State to 1, else 0

    if(LED_State == 1)
    {
      if (checkState == 0) //if checkState == 0 then we have already checked the state while it was high
      {
        (PORTD ^= 1<<PD7); //Toggle pin PD7
        checkState = 1;
      }
    }
    else
    {
      checkState = 0;
    }

    PORTD |= 1<<PD6;  //Set pin PD6 high (VCC for DHT22)

    //display image
    //LcdContrast(0x00);
    if (waitStatus == 0)
    {
      LcdClear();
      LcdImage(waitImage);
      LcdUpdate();
      _delay_ms(2500);
      waitStatus = 1;

      //_delay_ms(2500);
      //LcdContrast(0x00);
      LcdUpdate();
      _delay_ms(500);
    }
    else
    {
      if (TCNT1 >= 31249) // Will become true at 1 second. 31249 is target prescale given 8Mhz uC Freq. and prescale of /256
      {
        TCNT1 = 0; // Reset timer value
        elapsedSeconds++;

        if (elapsedSeconds == 20) // Check to see if specified amount of time (in seconds) has passed
        {
          elapsedSeconds = 0; // Reset counter variable

          PORTD &= ~(1<<PD7); //PD7
          PORTD &= ~(1<<PD6); //PD6

          //Deinitialize LCD and DHT22 sensor to turn off power
          DHT22_Deinit();
          LcdDeinit();

          set_sleep_mode(SLEEP_MODE_PWR_DOWN);
          sleep_enable();
          sleep_mode();
        }
      }

      state = DHT22_StartReading();
      state = DHT22_CheckStatus(&sensor_data);

      if (state == DHT_DATA_READY){

        tempC = sensor_data.temperature_integral;
        tempF = (tempC*1.8)+32;
        humid = sensor_data.humidity_integral;

        itoa(tempC,tempBufferC,10);
        itoa(tempF,tempBufferF,10);
        itoa(humid,humidBuffer,10);

        LcdClear();

        LcdGotoXYFont(2,1);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("Temperature:"));

        LcdGotoXYFont((1+tempOffset),2);
        LcdStr(FONT_1X, tempBufferC);

        LcdGotoXYFont((4+tempOffset),2);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("C"));

        LcdGotoXYFont((7+tempOffset),2);
        LcdStr(FONT_1X, tempBufferF);

        LcdGotoXYFont((10+tempOffset),2);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("F"));

        LcdGotoXYFont(3,4);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("Humidity:"));

        LcdGotoXYFont(6,5);
        LcdStr(FONT_1X, humidBuffer);

        LcdGotoXYFont(9,5);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("%"));

        LcdUpdate();

      }

      else if (state == DHT_ERROR_CHECKSUM){

        LcdGotoXYFont(2,1);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("CHECKSUM ERROR"));

      }
      else if (state == DHT_ERROR_NOT_RESPOND){

        LcdGotoXYFont(2,1);
        LcdFStr(FONT_1X,(unsigned char*)PSTR("NO RESPONSE FROM SENSOR"));
      }


    }
  }
}
