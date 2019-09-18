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
	#define BLEEPHNAME 				"elora"
    #define NUMCOLORS               10
	
	

    #define MAX_TWIST_DELAY         2000
    
    #define MODECODESWIRL           1
    #define MODECODEREVERSE         2
    #define MODECODETWIST           3
    #define MODECODERAINBOW         4
    #define MODECODEMARQUEE         5
    

    #define REVERSE_FLAG            0x01

    #define VBATPIN A9
   
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
uint32_t vbatcountdown;
float last_battery;
float colorValue;
uint8_t colorValueByte;
uint8_t rainbowStart;


typedef struct {
    float r;       // a fraction between 0 and 1
    float g;       // a fraction between 0 and 1
    float b;       // a fraction between 0 and 1
} rgb_t;

typedef struct {
    float h;       // angle in degrees
    float s;       // a fraction between 0 and 1
    float v;       // a fraction between 0 and 1
} hsv_t;

static hsv_t   rgb2hsv(rgb_t in);
static rgb_t   hsv2rgb(hsv_t in);

hsv_t rgb2hsv(rgb_t in)
{
    hsv_t         out;
    float      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb_t hsv2rgb(hsv_t in)
{
    float      hh, p, q, t, ff;
    long        i;
    rgb_t         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}
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
  ble_readpacket_timeout = 5;
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
    pixel.setPixelColor(i, pixel.Color(1,0,0)); // off
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

  if( !ble.sendCommandCheckOK("AT+GAPDEVNAME=bleeph_"BLEEPHNAME) )
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
uint16_t lowbyte;
uint16_t vbatInt;
/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/ 
void loop(void)
{
  if(vbatcountdown == 0) {
    vbatcountdown = 2000;
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    // measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    // measuredvbat /= 1024; // convert to voltage
    //Serial.print("VBat: " ); Serial.print(measuredvbat); Serial.println(" whatevers");
	vbatInt = (int)measuredvbat;
    Serial.print("VBatInt: " ); Serial.print(vbatInt); Serial.println(" whatevers");
    char inputs[3];
	
    memset(inputs, 0, 16);
	inputs[0] = 1;
    inputs[1] = vbatInt % 256;
	inputs[2] = vbatInt / 256;
	//Serial.print("inputs[0] = " ); Serial.println((uint8_t)inputs[0]);
	//Serial.print("inputs[1] = " ); Serial.println((uint8_t)inputs[1]);
	//Serial.print("inputs[2] = " ); Serial.println((uint8_t)inputs[2]);
    ble.print(inputs);
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
    case MODECODERAINBOW:
      rainbow();
    default:
      left_right();
      break;
  }
  uint8_t len = readPacket(&ble, ble_readpacket_timeout);
  /* Got a packet! */
  if (len == 0) {
    return;
  }
  Serial.print("Got packet(s) packetbuffer[1] = ");Serial.println(packetbuffer[1]);
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
	} else if ( packetbuffer[1] == 'I' ) {
		int i = packetbuffer[2];
		Serial.print("color index = "); Serial.println(i);
		color[i] = pixel.Color(packetbuffer[3], packetbuffer[4], packetbuffer[5]);
		Serial.print("color[");
		Serial.print(i);
		Serial.print("] = ");
		printHex(color[i], 3);
		Serial.println(" new?");
	} else if ( packetbuffer[1] == 'N' ) {
		int i = packetbuffer[2];
		Serial.print("new numColors = "); Serial.println(i);
		numColors = i;
	} else if ( packetbuffer[1] == 'V' ) {
		Serial.println("Value!");
		colorValueByte = packetbuffer[2];
		colorValue = (float)colorValueByte / (float)256.0;
		Serial.print("colorValueByte = ");Serial.println(colorValueByte);
		Serial.print("colorValue = ");Serial.println(colorValue);
	} else if ( packetbuffer[1] == 'M' ) {
		Serial.print("Mode:");   Serial.println(packetbuffer[2]);
		if ( packetbuffer[2] == MODECODESWIRL ) {
				Serial.println("Swirl");
				Serial.print("old numColors = "); Serial.println(numColors);
				for (uint8_t i = 0; i < numColors; i++) {
					Serial.print("color[");
					Serial.print(i);
					Serial.print("] = ");
					printHex(color[i], 3);
					Serial.println(" k?");
				}
				for ( int i = 0; i < numColors ; i++ ) {
					colorChangePoint[i] = ((i * NUMPIXELS) / numColors);
				}
				if(mode != 1) {
					ss_1 = 0;
					ss_4 = 0;
					mode = 1;
				}
		} else if ( packetbuffer[2] == MODECODEREVERSE ) {
				flags ^= REVERSE_FLAG;
				if(flags & REVERSE_FLAG) {
					Serial.println("Reverse");
				} else {
					Serial.println("Forward");
				}
		} else if ( packetbuffer[2] == MODECODETWIST ) {
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
		} else if ( packetbuffer[2] == MODECODERAINBOW ) {
				Serial.println("Rainbow");
				if (mode != MODECODERAINBOW) {
					mode = MODECODERAINBOW;
					rainbowStart = 0;
			}
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
    ble_readpacket_timeout = 5;
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
      pixel.setPixelColor(i, Darken(onColor));
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
      pixel.setPixelColor(i, Darken(color[0]));
    }
    for(uint8_t i=NUMPIXELS/2; i<NUMPIXELS; i++) {
      pixel.setPixelColor(i, Darken(color[1]));
    }
}

void rainbow() {
	//Serial.print("colorValue = "); Serial.println(colorValue);
	for ( uint32_t i = 0; i < NUMPIXELS; i++ ) {
		uint16_t hue = (((rainbowStart+i)*256)/NUMPIXELS)%256;
		uint32_t color = DarkWheel(hue);
		//uint32_t color = Wheel(hue);
		pixel.setPixelColor(i, color);
	}
	pixel.show();
    if(flags & REVERSE_FLAG) {
		rainbowStart++;
		if (rainbowStart > NUMPIXELS) {
			rainbowStart = 0;
		}
	} 
	else {
		if (rainbowStart == 0) {
			rainbowStart = NUMPIXELS;
		}
		rainbowStart--;
	}
		
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t Darken(uint32_t color) {
  rgb_t rgb;
  hsv_t hsv;
  rgb.r = (float)((color & 0xFF0000)/0x010000)/256.0;
  rgb.g = (float)((color & 0x00FF00)/0x000100)/256.0;
  rgb.b = (float)((color & 0x0000FF)/0x000001)/256.0;
  hsv = rgb2hsv(rgb);
  hsv.v = colorValue;
  rgb = hsv2rgb(hsv);
  return pixel.Color(rgb.r * 255, rgb.g * 255, rgb.b * 255);
}	

uint32_t DarkWheel(byte WheelPos) {
  uint32_t color = Wheel(WheelPos);
  return Darken(color);
}
