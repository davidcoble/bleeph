/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

#include "BluefruitConfig.h"


/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    PIN                       Which pin on the Arduino is connected to the NeoPixels?
    NUMPIXELS                 How many NeoPixels are attached to the Arduino?
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE     0

    #define PIN                     5
    #define NUMPIXELS               144
    #define NUMCOLORS               10

    #define MAX_TWIST_DELAY         2000
    
    #define MODECODESWIRL           1
    #define MODECODEREVERSE         2
    #define MODECODETWIST           3
    #define MODECODERAINBOW         4
    

    #define REVERSE_FLAG            0x01

    #define VBATPIN 9
   
/*=========================================================================*/

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN);

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

uint8_t mode;

uint8_t numColors;
uint32_t color[NUMCOLORS];
uint32_t colorChangePoint[NUMCOLORS];
//swirl state vars
uint8_t ss_1; // starting color
uint8_t ss_2; // first switch point
uint8_t ss_3; // second switch point
uint8_t ss_4; // next color
uint8_t flags;
uint16_t twist_delay;
int twist_delay_direction;
uint8_t twist_fast_prolong;
uint8_t twist_delay_delta;
uint16_t ble_readpacket_timeout;
uint8_t vbatcountdown;
float last_battery;

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  for(uint8_t i = 0; i < NUMCOLORS; i++) {
    color[i] = pixel.Color(0,0,0);
    colorChangePoint[i] = 0;
  }
  ble_readpacket_timeout = 20;
  mode = 0;
  flags = 0;
  numColors = 0;
  twist_delay = 0;
  twist_delay_direction = 1;
  twist_delay_delta = 0;
  twist_fast_prolong = 0;
  vbatcountdown = 0;
  
  //while (!Serial);  // required for Flora & Micro
  delay(5000);

  // turn off neopixel
  pixel.begin(); // This initializes the NeoPixel library.
  for(uint8_t i=0; i<NUMPIXELS; i++) {
    pixel.setPixelColor(i, pixel.Color(0,0,0)); // off
  }
  pixel.show();

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Neopixel Color Picker Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  //Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  if( !ble.sendCommandCheckOK("AT+GAPDEVNAME=bleeph") )
  {
    Serial.println("Error setting dev name");
  }
  if( !ble.sendCommandCheckOK("ATZ") )
  {
    Serial.println("Resetting failed.");
  }

  /* Disable command echo from Bluefruit */
  ble.echo(true);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("***********************"));

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("***********************"));

}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/ 
void loop(void)
{
  if(vbatcountdown == 0) {
    vbatcountdown = 100;
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    Serial.print("VBat: " ); Serial.println(measuredvbat);
  } else {
    vbatcountdown--;
  }

  /* Wait for new data to arrive */
  switch ( mode ) { 
    case MODECODESWIRL:
      swirl();
      break;
    case MODECODETWIST:
      twist();
      break;
    default:
      left_right();
      break;
  }
  uint8_t len = readPacket(&ble, ble_readpacket_timeout);
  /* Got a packet! */
  if (len == 0) {
    return;
  }
  printHex(packetbuffer, len);
  // Color
  if (packetbuffer[1] == 'C') {
    uint8_t red = packetbuffer[2];
    uint8_t green = packetbuffer[3];
    uint8_t blue = packetbuffer[4];
    color[0] = color[1];
    color[1] = pixel.Color(red,green,blue);
    Serial.println("Color:");
    for(uint8_t i=0; i<NUMPIXELS/2; i++) {
      pixel.setPixelColor(i, color[0]);
    }
    for(uint8_t i=NUMPIXELS/2; i<NUMPIXELS; i++) {
      pixel.setPixelColor(i, color[1]);
    }
    pixel.show(); // This sends the updated pixel color to the hardware.
  } else if (packetbuffer[1] == 'I') { //index and color
    int i = packetbuffer[2];
    Serial.print("color index = "); Serial.println(i);
    color[i] = pixel.Color(packetbuffer[3], packetbuffer[4], packetbuffer[5]);

  } else if (packetbuffer[1] == 'M') {
    Serial.print("Mode:");
    Serial.println(packetbuffer[2]);
    switch(packetbuffer[2]) {
      case 1:
        Serial.println("Swirl");
        numColors = packetbuffer[3];
        Serial.print("numColors = "); Serial.println(numColors);
        for ( int i = 0; i < numColors ; i++ ) {
          colorChangePoint[i] = ((i * NUMPIXELS) / numColors);
        }
        if(mode != 1) {
          ss_1 = 0;
          ss_4 = 0;
          mode = 1;
        }
        break;
      case 2:
        flags ^= REVERSE_FLAG;
        if(flags & REVERSE_FLAG) {
          Serial.println("Reverse");
        } else {
          Serial.println("Forward");
        }
        break;
      case 3:
        Serial.println("Twist");
        numColors = packetbuffer[3];
        Serial.print("numColors = "); Serial.println(numColors);
        for ( int i = 0; i < numColors ; i++ ) {
          colorChangePoint[i] = ((i * NUMPIXELS) / numColors);
        }
        if(mode != 3) {
          ss_1 = 0;
          ss_4 = 0;
          mode = 3;
        }
        break;

      default:
        mode = 0;
        break;
    }
  }

}

void incrementStartingColorIndex() {
    ss_1++;
    if(ss_1 >= numColors) { ss_1 = 0; }
}

void decrementStartingColorIndex() {
  if(ss_1 == 0) {
    ss_1 = numColors;
  }
  ss_1--;
}

void incrementNextColorIndex() {
    ss_4++;
    if(ss_4 >= numColors) { ss_4 = 0; }
}

void incrementSwitchPoints() {
    for(uint8_t j = 0; j < numColors; j++) {
      colorChangePoint[j] = colorChangePoint[j] + 1;
      if(colorChangePoint[j] >= NUMPIXELS) {  colorChangePoint[j] = 0; }
    }
}
void decrementSwitchPoints() {
    for(uint8_t j = 0; j < numColors; j++) {
      if(colorChangePoint[j] == 0) {
        colorChangePoint[j] = NUMPIXELS;
      }
      colorChangePoint[j] = colorChangePoint[j] - 1;
    }
}

void twist() {
    if(twist_fast_prolong) {
      twist_fast_prolong--;
    } else {
      if(twist_delay < 40) {
        twist_fast_prolong = 40 - twist_delay;
      }
      if(twist_delay >= 2000) {
        flags ^= REVERSE_FLAG;
        twist_delay_direction *= -1;
      }
      if(twist_delay > 1000) {
        twist_delay_delta = 200;
      } else if(twist_delay > 500) {
        twist_delay_delta = 100;
      } else if(twist_delay > 200) {
        twist_delay_delta = 50;
      } else if(twist_delay > 100) {
        twist_delay_delta = 10;
      } else {
        twist_delay_delta = 1;
      }
      if(twist_delay == 0) {
        twist_fast_prolong = 100;
        twist_delay_direction *= -1;
      }
      if(twist_delay_direction != -1 || twist_delay != 0){
        twist_delay += twist_delay_direction * twist_delay_delta;
      }
    }
    Serial.print("delay= ");Serial.print(twist_delay);
    Serial.print("delta= ");Serial.println(twist_delay_delta);
    ble_readpacket_timeout = twist_delay;
    uint32_t onColor = color[ss_1];
    ss_4 = ss_1;
    incrementNextColorIndex();
    uint32_t nextColor = color[ss_4];
    for(uint8_t i = 0; i < NUMPIXELS; i++) {
      for(uint8_t j = 0; j < numColors; j++) {
        if(colorChangePoint[j] == i) {
          onColor = nextColor;
          incrementNextColorIndex();
          nextColor = color[ss_4];
        }
      }
      pixel.setPixelColor(i, onColor);
    }
    pixel.show();
    if(flags & REVERSE_FLAG) {
      decrementSwitchPoints();
    } else {
      incrementSwitchPoints();
    }
    for(uint8_t j = 0; j < numColors; j++) {
      if(flags & REVERSE_FLAG) {
        if(colorChangePoint[j] == NUMPIXELS-1) {
          incrementStartingColorIndex();
        }
      } else {
        if(colorChangePoint[j] == 0) {
          decrementStartingColorIndex();
        }
      }
    }
}

void swirl() {
//    Serial.print("ss_1 = ");     Serial.println(ss_1);
//    Serial.print("ss_2 = ");     Serial.println(ss_2);
//    Serial.print("ss_3 = ");     Serial.println(ss_3);
//    Serial.println("HHHHHHHHHHHHHHHHH");
    
//    Serial.print("starting color index = ");
//    Serial.println(ss_1);
    ble_readpacket_timeout = 20;
    uint32_t onColor = color[ss_1];
    ss_4 = ss_1;
    incrementNextColorIndex();
    uint32_t nextColor = color[ss_4];
    for(uint8_t i = 0; i < NUMPIXELS; i++) {
      for(uint8_t j = 0; j < numColors; j++) {
        if(colorChangePoint[j] == i) {
          onColor = nextColor;
          incrementNextColorIndex();
          nextColor = color[ss_4];
        }
      }
      pixel.setPixelColor(i, onColor);
    }
    pixel.show();
    if(flags & REVERSE_FLAG) {
      decrementSwitchPoints();
    } else {
      incrementSwitchPoints();
    }
    for(uint8_t j = 0; j < numColors; j++) {
//      Serial.print("colorChangePoint[");
//      Serial.print(j);
//      Serial.print("] = ");
//      Serial.println(colorChangePoint[j]);
    if(flags & REVERSE_FLAG) {
        if(colorChangePoint[j] == NUMPIXELS-1) {
          incrementStartingColorIndex();
        }
    } else {
        if(colorChangePoint[j] == 0) {
          decrementStartingColorIndex();
        }
      }
    }
}

void left_right() {
    for(uint8_t i=0; i<NUMPIXELS/2; i++) {
      pixel.setPixelColor(i, color[0]);
    }
    for(uint8_t i=NUMPIXELS/2; i<NUMPIXELS; i++) {
      pixel.setPixelColor(i, color[1]);
    }
}
