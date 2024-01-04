/*PINOUT
 * //PADS
 * Nr 1  2   3  4  5  6  7  8  9 10 11 12 13 14 15
 * Pn 23 25 27 29 31 33 35 37 39 41 43 45 47 49 51
 * 
 * //MULTIPLEXER
 * Nr S0 S1 S2 S3 Z
 * Pn 17 16 15 14 A0
 *  0 1 2 - faders
 *  3 4   - YX
 *  5 ... - pots
 * 
 * //LCD
 * Nr SDA SCL RES CS DC
 * Pn SDA SCL RES 8  9
 * 
 * //ENCODER
 * Nr L R B
 * Pn 2 3 4
 * 
 * YX Switch  - 5
 * SW   0   1   2
 *     48  50  52
 * 
 * 
 * //LED
 * Pn 11 (ctr 12)
 */
//#define DEBUG 1
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <FastLED.h>
#include <Control_Surface.h> // Include the Control Surface library
//#include <Encoder.h>

#define NUM_LEDS 22 
#define DATA_PIN 11
#define CLOCK_PIN 12

#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5

#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        8
  #define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         9
#endif

class aLed
{
  public:
  // gdy podamy konstruktor
  aLed(byte aLedH, byte aLedS, byte aLedV)
  {
    h = aLedH; s = aLedS; v = aLedV;
  }
  // defaultowy konstruktor
  aLed() { h=0; s=255; v=64;}
  ~aLed();

  int h, s, v;
};

aLed *aLpt[NUM_LEDS]; // wskaznik na moje clasy ledow
aLed *matrixLpt[3][5]; // wskaznik na LEDY wg matrycy
int ledWave[3][5];
//int padKeyStatus[3][5];
int MPROG = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); // WLASCIWY OBIEKT 1.8!!!!!

CRGB leds[NUM_LEDS];
//Encoder enc(3, 2);

// Instantiate a MIDI over USB interface.
USBMIDI_Interface midi;
using namespace MIDI_Notes;

CD74HC4067 mux = { 
  A0,               // analog pin
  {17, 16, 15, 14},  // Address pins S0, S1, S2, S3
  // 13, // Optionally, specify the enable pin
};

CCPotentiometer potentiometers[] = {             //  moje muxy
  {mux.pin(0), 0x10},     // fader 0             // muxs 0    - F3
  {mux.pin(1), 0x11},     // fader 1             // muxs 1    - F2
  {mux.pin(2), 0x12},     // fader 2             // muxs 2    - F1
  {mux.pin(3), 0x13},     // xy 0                // muxs 3    -XY 2
  {mux.pin(4), 0x14},     // xy 1                // muxs 4    -XY 1
  {mux.pin(5), 0x15},     // poty ... 15 - 22    // muxs 5    - R 2
  {mux.pin(6), 0x16},                            // muxs 6    - R 1
  {mux.pin(7), 0x17},                            // muxs 7    - L5
  {mux.pin(8), 0x18},                            // muxs 8    - L4
  {mux.pin(10), 0x1A},                           // muxs 9    - L3
  {mux.pin(9), 0x19},                            // muxs 10   - L2
  {mux.pin(11), 0x1B},                           // muxs 11   - L1
};
//   RL 51 47 49 43 45 
//      41 33 39 37 35
//      31 27 29 23 25

// CUSTOM PADS
Button cPAD1 = {51},  cPAD2 = {47},  cPAD3 = {49},  cPAD4 = {43}, cPAD5 = {45};
Button cPAD6 = {41},  cPAD7 = {33}, cPAD8 = {39}, cPAD9 = {37}, cPAD10 = {35};
Button cPAD11 = {31}, cPAD12 = {27}, cPAD13 = {29}, cPAD14 = {23}, cPAD15 = {25};
Button *cPADptr[15];

///////////////////////////////////////////////////////////////////   ZMIENNE
float p = 3.1415926;            //
int muxs[12];                    // moje poty za muxem
int muxsPrev[12] = {-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127};
int muxsNoise = 35;
unsigned long ledMs = 0;        // -----
unsigned int ledDel = 3;        // del dla fade
byte padSet[15] = {51, 47, 49, 43, 45, 35, 37, 39, 33, 41, 31, 27, 29, 23, 25}; // PADSET ulozony wg LEDow P-L-P
byte padSetCurrNote[15] = {12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}; // zakres oktawy 12,13 - C3... 12+12 - C4,  wskazywany przez pointer: 0=C1, 12=C2;
                        //  C, C#,D,D#, E, F, F#,G,G#, A,A#, B.....repeat  IMPORTANT
                                    //  1   2  3  4  5  6  7  8  9 10 11 12 13 14 15
const byte padSetCurrNoteChroma[15] =  {12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}; // Chromatyczna od C2
const byte padSetCurrNote_C[15] =      {12,14,16,17,19,21,23,24,26,28,29,31,33,35,36}; // C maj
const byte padSetCurrNote_c[15] =      {13,15,17,18,20,22,24,25,27,29,30,32,34,36,37}; // C# maj
const byte padSetCurrNote_D[15] =      {14,16,18,19,21,23,25,26,28,30,31,33,35,37,38}; // D maj
const byte padSetCurrNote_d[15] =      {15,17,19,20,22,24,26,27,29,31,32,34,36,38,39}; // D# ...
const byte padSetCurrNote_E[15] =      {16,18,20,21,23,25,27,28,30,32,33,35,37,39,40};
const byte padSetCurrNote_F[15] =      {17,19,21,22,24,26,28,29,31,33,34,36,38,40,41};
const byte padSetCurrNote_f[15] =      {18,20,22,23,25,27,29,30,32,34,35,37,39,41,42};
const byte padSetCurrNote_G[15] =      {19,21,23,24,26,28,30,31,33,35,36,38,40,42,43};
const byte padSetCurrNote_g[15] =      {20,22,24,25,27,29,31,32,34,36,37,39,41,43,44};
const byte padSetCurrNote_A[15] =      {21,23,25,26,28,30,32,33,35,37,38,40,42,44,45};
const byte padSetCurrNote_a[15] =      {22,24,26,27,29,31,33,34,36,38,39,41,43,45,46};
const byte padSetCurrNote_B[15] =      {23,25,27,28,30,32,34,35,37,39,40,42,44,46,47}; // B maj 

byte padClr[15] ={};

  int encoder=0;
  int encoderPrev=0;
 //volatile byte ISRfreeFlag=1;
uint16_t encDel = 130;
unsigned long encMs = 0;

int sw1,sw2,sw3,sw1p,sw2p,sw3p;
int sw3out=0, sw3outPrev=0;

//MIDI CUSTOM STUFF --
int padLcdStartX = 5;
int padLcdStartY = 84;
int padLcdCoefX = 25;
int padLcdCoefY = 14;

int octModNote;
byte cNoteP[6] =  { 1,  2,   3,  4,   5,  6};                                           // Zakres uzywalnych oktaw - 0=C1
//char *cNote[12] = {"C","c","D","d","E","F","f","G","g","A","a","B"};
const char *cNote[12] = {" C","C#"," D","D#"," E"," F","F#"," G","G#"," A","A#"," B"}; // Zapis oktawy (nutowo) - tablica 12-ele wskaznikow
//                  0   1    2    3   4   5   6    7    8   9   10   11                 // Zapis oktawy (numerycznie)

const char *cScalesName[13] = {"SC: Chromatic",
                              "SC: C Maj / (A min)",
                              "SC: C# Maj / (A# min)",
                              "SC: D Maj / (B min)",
                              "SC: D# Maj / (C min)",
                              "SC: E Maj / (C# min)",
                              "SC: F Maj / (D min)",
                              "SC: F# Maj / (D# min)",
                              "SC: G Maj / (E min)",
                              "SC: G# Maj / (F min)",
                              "SC: A Maj / (F# min)",
                              "SC: A# Maj / (G min)",
                              "SC: B Maj / (G# min)"};


 // WSKAZNIKI NA ADESY MIDI DLA SKAL ---  Wskaznik na adresy zakresu skali CHROMATYCZNEJ (6-oktaw, 12-poltonow) 1 c                  
MIDIAddress *notePtrChroma[72];          // CHROMATYCZNA
  
uint8_t padVel = 0x7F;

enum updateCodes {nn=0, f1=1, f2=2, f3=3,
                  xyX=4, xyY=5,
                  p1=6, p2=7, p3=8, p4=9, p5=10,
                  pbt=11, pbb=12,
                  encRot=13, encB=14, xyB=15};

updateCodes uCodes  = nn;  

//LED STUFF
int ledOverallBrightness = 128;
int ledVfadeoutSpeed = 5;
int ledHfadeoutSpeed = 3;

int padLedBlight = 32;
int potsLedBlight = 192;
bool firstUpdate = 1;

int padLedBaseColor = 160;     // Niebieski
int padLedBasePushCol = 255;   // Czerwony
unsigned long LedWaveIntervalMs = 0;
unsigned int LedWIDel = 50;

const int LtoRAssign [15] = {0, 1, 2, 3, 4,
                             9, 8, 7, 6, 5,
                            10,11,12,13,14};
byte padIterator1=0;

int currScaleIter = 0;
int currScaleIterPrev = 0;
///////////////////////////////////////////////////////////////////   FUNCKJE
void fadeall() { for(int i = 0; i < 15; i++) { leds[i].nscale8(250); } }


void assignLedsToMy() {
    for(int i = 0; i < NUM_LEDS; i++)
    { 
      leds[i] = CHSV(aLpt[i]->h,aLpt[i]->s,aLpt[i]->v);
    }
   }
void assignLedsToMyMatrix(){
    padIterator1=0;
    for (byte wier=0; wier<3; wier++)
    {
     for (byte kol=0; kol<5; kol++)
     {
      leds[LtoRAssign[padIterator1]] = CHSV(matrixLpt[wier][kol]->h,
                                            matrixLpt[wier][kol]->s,
                                            matrixLpt[wier][kol]->v);
      padIterator1++;
     }
    }
   for(int i = 15; i < NUM_LEDS; i++)
    { 
   leds[i] = CHSV(aLpt[i]->h,aLpt[i]->s,aLpt[i]->v);
    }
  }
void fadeAllToMy()
{
  for (byte i=0; i<15; i++)
  {
    if (aLpt[i]->v > padLedBlight)
    {
      aLpt[i]->v-=ledVfadeoutSpeed;
      if (aLpt[i]->v<0) aLpt[i]->v=0;
    }
    else if (aLpt[i]->v < padLedBlight)
    {
      aLpt[i]->v+=ledVfadeoutSpeed;
      if (aLpt[i]->v>255) aLpt[i]->v=255;
    }
  }
}
void fadeAllToMyColor()
{
  byte padIterator2=0;
    for (byte wier=0; wier<3; wier++)
    {
     for (byte kol=0; kol<5; kol++)
     {  // WYROWNAJ KOLOR
           if (matrixLpt[wier][kol]->h > padLedBaseColor)
           {
            matrixLpt[wier][kol]->h-=ledHfadeoutSpeed;
            if (matrixLpt[wier][kol]->h<0) matrixLpt[wier][kol]->h=0;
           }
           
      else if (matrixLpt[wier][kol]->h < padLedBaseColor)
      {
        matrixLpt[wier][kol]->h+=ledHfadeoutSpeed;
        if (matrixLpt[wier][kol]->h>255) matrixLpt[wier][kol]->h=255;
      }
        // WYROWNAJ JASNOSC
           if (matrixLpt[wier][kol]->v > padLedBlight)
           {
            matrixLpt[wier][kol]->v-=ledVfadeoutSpeed;
            if (matrixLpt[wier][kol]->v<0) matrixLpt[wier][kol]->v=0;
           }
      else if (matrixLpt[wier][kol]->v < padLedBlight)
      {
        matrixLpt[wier][kol]->v+=ledVfadeoutSpeed;
        if (matrixLpt[wier][kol]->v>255) matrixLpt[wier][kol]->v=255;
      }
      padIterator2++;
     }
    }
}
word ConvertRGB( byte R, byte G, byte B)
{
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}

void customPadUpdate()
{

//loop aproach

    for (int pad=0; pad<15; pad++)
      {
       cPADptr[pad]->update();
        if (cPADptr[pad]->getState() == Button::Falling)
         {
            midi.sendNoteOn(*notePtrChroma[padSetCurrNote[pad]], padVel);         // wysylaj NOTE ON na CH 1 
            ledWave[pad/5][pad%5]=1;
         }
        else if (cPADptr[pad]->getState() == Button::Rising) 
         {
            midi.sendNoteOff(*notePtrChroma[padSetCurrNote[pad]], padVel);         // wysylaj NOTE ON na CH 1
            if (ledWave[pad/5][pad%5]==1)
            {
              ledWave[pad/5][pad%5]=2; 
              LedWaveIntervalMs=millis();
            }
         }
      } // petla
} // 

void BtnChk()   //// SZYBKIE FUNKCJE
{
      if (digitalRead(4)==LOW)
      {
        potsLedBlight = map(muxs[1],0,4095,0,255);
        padLedBlight  = map(muxs[2],0,4095,0,255);
        LedWIDel =      map(muxs[5],0,4095,10,500);
        padLedBaseColor=map(muxs[6],0,4095,32,192);
      }
      
}

void encChk()   // -------------- GLOWNA METODA DLA ENCODERA ------- //
{
  if (digitalRead(3)==LOW && (millis()>encMs+encDel))   // debounce dla encodera
  {
    if (digitalRead(4)==HIGH)
    {
      if (digitalRead(2)==LOW)
      {
        if (encoder<35) encoder++;                 // obracanie w prawo
      }
      else 
      {
        if(encoder>-12)encoder--;                                     // obracanie w lewo
      }
      encMs = millis();  
    }
    else if (digitalRead(4)==LOW)
    {
      if (digitalRead(2)==LOW)
      {
        if (currScaleIter<12) currScaleIter++;                 // obracanie w prawo
      }
      else
      {
        if (currScaleIter>0) currScaleIter--;                                     // obracanie w lewo
      }
      encMs = millis(); 
    }
                                 // zapisz nowy czas dla debounca 
  }
}
// -------------- GLOWNA METODA DLA ENCODERA END ------------------- //

void xyBtn()
{
  
}

void sw3check()
{
  if (digitalRead(48)==HIGH) sw1=0;
  else  sw1=1;
  if (digitalRead(50)==HIGH) sw2=0;
  else sw2=1;
  if (digitalRead(52)==HIGH) sw3=0;
  else sw3=1;
  
    if (sw1p!=sw1 || sw2p!=sw2 || sw3p!=sw3)
     {
      sw3out=0;
      sw3out = ((((sw3out|sw1)<<1)|(sw3out|sw2))<<1)|(sw3out|sw3);
      sw1p=sw1;
      sw2p=sw2;
      sw3p=sw3;
      
  //LCD
  tft.setCursor(76, 20);

  tft.setTextColor(ConvertRGB(55,65,220));
  tft.setTextSize(1);
  tft.print("P:["+String(sw3outPrev)+"]");

  tft.setCursor(76, 20);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("P:["+String(sw3out)+"]");
  sw3outPrev=sw3out;
  firstUpdate=1;
     }

//chwilowa zmiana MPROG
MPROG = sw1;
}
void updatePotsLCD()
{
  static int pLcdSX = 42;
  static int pLcdSY = 50;
   static int pLcdCX = 12;
   static int pLcdW = 8;
   static int pLcdH = -14;
   //static int pLcdH = 24;

  // 5 gornych
  for (int p=0; p<5; p++)
  {
    if (muxsPrev[11-p] > muxs[11-p]+muxsNoise || muxsPrev[11-p] < muxs[11-p]-muxsNoise)
      {
     // stare
     tft.fillRect((pLcdSX+pLcdCX*p),pLcdSY,pLcdW,map(muxsPrev[11-p],0,4095,0,pLcdH),ConvertRGB(0,0,0));
     //  nowe
     tft.fillRect((pLcdSX+pLcdCX*p),pLcdSY,pLcdW,map(muxs[11-p],0,4095,0,pLcdH),ConvertRGB(255,32,16));
     //   ramki
     tft.drawRect((pLcdSX+pLcdCX*p),pLcdSY,pLcdW,pLcdH+2,ConvertRGB(255,200,200));
    }
  }

  // 2 boczne po prawej
    for (int p=0; p<2; p++)
  {
    if (muxsPrev[6-p] > muxs[6-p]+muxsNoise || muxsPrev[6-p] < muxs[6-p]-muxsNoise)
    {
     // stare
     tft.fillRect((pLcdSX+(pLcdCX-2)*p)+62,pLcdSY,pLcdW-2,map(muxsPrev[6-p],0,4095,0,pLcdH),ConvertRGB(0,0,0));
     //  nowe
      tft.fillRect((pLcdSX+(pLcdCX-2)*p)+62,pLcdSY,pLcdW-2,map(muxs[6-p],0,4095,0,pLcdH),ConvertRGB(16,32,255));
     //   ramki
      tft.drawRect((pLcdSX+(pLcdCX-2)*p)+62,pLcdSY,pLcdW-2,pLcdH,ConvertRGB(255,32,32));
    }
  }
  /*
  tft.fillRect(140,pLcdSY,pLcdW,map(muxsPrev[6],0,4095,0,pLcdH),ConvertRGB(0,0,0));
  tft.fillRect(140,pLcdSY,pLcdW,map(muxs[6],0,4095,0,pLcdH),ConvertRGB(255,32,128));
  tft.drawRect(140,pLcdSY,pLcdW,pLcdH+1,ConvertRGB(255,200,200));

  tft.fillRect(145,pLcdSY+30,pLcdW,map(muxsPrev[5],0,4095,0,pLcdH),ConvertRGB(0,0,0));
  tft.fillRect(145,pLcdSY+30,pLcdW,map(muxs[5],0,4095,0,pLcdH),ConvertRGB(255,32,128));
  tft.drawRect(145,pLcdSY+30,pLcdW,pLcdH+1,ConvertRGB(255,200,200));
*/
  // 3 fadery
  for (int p=0; p<3; p++)
  {
    if (muxsPrev[2-p] > muxs[2-p]+muxsNoise || muxsPrev[2-p] < muxs[2-p]-muxsNoise)
    {
      // stare
     tft.fillRect((pLcdSX+pLcdCX*p)-40,pLcdSY,pLcdW,map(muxsPrev[2-p],0,4095,0,pLcdH),ConvertRGB(0,0,0));
     //  nowe
     tft.fillRect((pLcdSX+pLcdCX*p)-40,pLcdSY,pLcdW,map(muxs[2-p],0,4095,0,pLcdH),ConvertRGB(16,32,255));
      //   ramki
      tft.drawRect((pLcdSX+pLcdCX*p)-40,pLcdSY,pLcdW,pLcdH,ConvertRGB(255,32,32));
    }
  }  

}

void updateLCD()
{
  if (encoderPrev!=encoder || currScaleIterPrev != currScaleIter || firstUpdate==1)
  {
  tft.setCursor(8, 20);

if (MPROG==0) tft.setTextColor(ConvertRGB(55,65,220));
else if (MPROG==1) tft.setTextColor(ConvertRGB(190,50,240));
  tft.setTextSize(1);
  tft.print("Note's: "+String(encoderPrev));

  tft.setCursor(8, 20);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Note's: "+String(encoder));


  static byte ii=0;
  for (int y=0; y<3; y++)
  {
    for (int x=0; x<5; x++)
    {
        tft.setCursor(padLcdStartX+(padLcdCoefX*x), padLcdStartY+(padLcdCoefY*y));
        tft.fillRect((padLcdStartX+(padLcdCoefX*x)),padLcdStartY+(padLcdCoefY*y),20,20,ST77XX_BLACK);
        //tft.setTextColor(ST77XX_BLACK);
        //tft.setTextSize(1);
        //tft.print(String(*cNote[padSetCurrNote[ii]%12])+String(padSetCurrNote[ii]/12+1));
        ii++;
    }
  }

  // --------------------------- USTAWIENIE OKTAW i SKALI 
    for (int i=0; i<15; i++)
    {
      switch(currScaleIter)
      {
                case 0:
        {
          padSetCurrNote[i]=padSetCurrNoteChroma[i];
          break;
        }
                case 1:
        {
          padSetCurrNote[i]=padSetCurrNote_C[i];
          break;
        }
                case 2:
        {
          padSetCurrNote[i]=padSetCurrNote_c[i];
          break;
        }
                case 3:
        {
          padSetCurrNote[i]=padSetCurrNote_D[i];
          break;
        }
                case 4:
        {
          padSetCurrNote[i]=padSetCurrNote_d[i];
          break;
        }
                case 5:
        {
          padSetCurrNote[i]=padSetCurrNote_E[i];
          break;
        }
                case 6:
        {
          padSetCurrNote[i]=padSetCurrNote_F[i];
          break;
        }
                case 7:
        {
          padSetCurrNote[i]=padSetCurrNote_f[i];
          break;
        }
                case 8:
        {
          padSetCurrNote[i]=padSetCurrNote_G[i];
          break;
        }
                case 9:
        {
          padSetCurrNote[i]=padSetCurrNote_g[i];
          break;
        }
                case 10:
        {
          padSetCurrNote[i]=padSetCurrNote_A[i];
          break;
        }
                case 11:
        {
          padSetCurrNote[i]=padSetCurrNote_a[i];
          break;
        }
                case 12:
        {
          padSetCurrNote[i]=padSetCurrNote_B[i];
          break;
        }
        default: break;
      }  
    }
    
  for (int i=0; i<15; i++)
    {
      padSetCurrNote[i]+=encoder;
    }
  // --------------------------- USTAWIENIE OKTAW    
  ii=0;
  for (int y=0; y<3; y++)
  {
    for (int x=0; x<5; x++)
    {
        tft.setCursor(padLcdStartX+(padLcdCoefX*x), padLcdStartY+(padLcdCoefY*y));
        tft.setTextColor(ConvertRGB(random(120,255),random(120,255),random(120,255))); // padClr[p]
        tft.setTextSize(1);
        tft.print(cNote[padSetCurrNote[ii]%12]+String(padSetCurrNote[ii]/12+2));
        ii++;
    }
  }
    encoderPrev=encoder;
  }
  //Serial.println(encoder);
  // update POTOW ---->
  for (int i=0; i<12; i++)
  {
    if (muxsPrev[i] > muxs[i]+muxsNoise || muxsPrev[i] < muxs[i]-muxsNoise)
    {
      updatePotsLCD();
      muxsPrev[i]=muxs[i];
    }
  }

  // update skali ----->
  if ((currScaleIterPrev != currScaleIter) || firstUpdate==1)
  {
    tft.fillRect(0,60,130,14,ConvertRGB(0,0,0));
    tft.setCursor(4, 64);
    tft.setTextColor(ConvertRGB(16,220,96));
    tft.setTextSize(1);
    tft.print(cScalesName[currScaleIter]);
    currScaleIterPrev = currScaleIter;
  }

}

void updateLogo()
{
  //INIT LCD // ------------------------------------------------BUDUJ MENU i LOGOTYP
  tft.setRotation(3);
   tft.fillScreen(ST77XX_BLACK);

 if (MPROG==0)
 {
  tft.fillRect(0, 0 , 160, 16, ConvertRGB(64,32,32));
  tft.fillRect(0, 17 , 160, 16, ConvertRGB(55,65,220));
  
  tft.setCursor(4, 4);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("RAG MIDI REACTOR");

  for (byte i=0; i<30; i++)
  {
      tft.fillTriangle(120+(2*i),0,160,128,160,0, ConvertRGB(32+(7*i),128-(4*i),255-(5*i)));

  }
  tft.drawLine(120,0,160,128,ConvertRGB(232,235,232));  
 } // if MPROG

 else if (MPROG==1)
 {
  tft.fillRect(0, 0 , 160, 16, ConvertRGB(244,160,74));
  tft.fillRect(0, 17 , 160, 16, ConvertRGB(190,50,240));
  
  tft.setCursor(4, 4);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.print("RAGNAROCK d(X_X)b");

  for (byte i=0; i<30; i++)
  {
      tft.fillTriangle(120+(2*i),0,160,128,160,0, ConvertRGB(210-(5*i),210-(7*i),0+(5*i)));

  }
  tft.drawLine(120,0,160,128,ConvertRGB(64,235,22));  
 } // if MPROG

  tft.setCursor(128, 4);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("V1.1");  
}

void ledWaveMode()
{
/*
 * ledWave table
 * 0 - czekam na nowe wcisniecie
 * 1 - wcisniety, przygotuj
 * 2 - zwolniony i fx
 *   3
 * 6   4
 *   5
 */
   // SPRAWDZENIE i TWORZENIE OGNISKA
   if (MPROG==1){
  for (int x=0; x<3; x++)
  {
    for (int y=0; y<5; y++)
    {
      if (ledWave[x][y]==2)
      {
        if (x>0) ledWave[x-1][y]=3;
        if (y<4) ledWave[x][y+1]=4;
        if (x<2) ledWave[x+1][y]=5;
        if (y>0) ledWave[x][y-1]=6;
        ledWave[x][y]=0;
      }
    }
  }}
// ZAPALANIE WAVE - poprawic
if ((millis()>LedWaveIntervalMs+LedWIDel) && MPROG==1)
{
  // ROZPALANIE
    // zapalaj kolejne zgodnie z zegarem 1c 2g 3p 4d 5l
  for (int x=0; x<3; x++)
  {
   for (int y=0; y<5; y++)
    {
      if (ledWave[x][y]==3)
      {
        if (x>0) {ledWave[x-1][y] = 3;}
        matrixLpt[x][y]->h=padLedBasePushCol;
        matrixLpt[x][y]->v=192;
        ledWave[x][y]=0;
       }  
      if (ledWave[x][y]==6)
      {
        if (y>0) {ledWave[x][y-1] = 6;}
        matrixLpt[x][y]->h=padLedBasePushCol;
        matrixLpt[x][y]->v=192;
        ledWave[x][y]=0;
       }
    } // loop x
  } // loop y
  for (int x=2; x>=0; x--)
  {
   for (int y=4; y>=0; y--)
    {
      if (ledWave[x][y]==4)
      {
        if (y<4) {ledWave[x][y+1] = 4;}
        matrixLpt[x][y]->h=padLedBasePushCol;
        matrixLpt[x][y]->v=192;
        ledWave[x][y]=0;
       } 
      if (ledWave[x][y]==5)
      {
        if (x<2) {ledWave[x+1][y] = 5;}
        matrixLpt[x][y]->h=padLedBasePushCol;
        matrixLpt[x][y]->v=192;
        ledWave[x][y]=0;
       }         
    }
  }
  #ifdef DEBUG
    for (int x=0; x<3; x++)
  {
    for (int y=0; y<5; y++)
    {
      Serial.print(ledWave[x][y]);
      Serial.print("  ");
    }
    Serial.println("");
  }
  #endif
  LedWaveIntervalMs = millis();
} // wave end  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP///////////////////////////////////+-+-+
void setup(void) {

  Serial.begin(115200);
  Serial.println("Startuje debug:");

  pinMode(13,INPUT_PULLUP); // zabezpieczenie
  pinMode(2,INPUT_PULLUP);  // ENC L (zostawic PULLUP)
  pinMode(3,INPUT_PULLUP);  // ENC R (zostawic PULLUP)
  pinMode(4,INPUT_PULLUP);         // ENC BTN
  pinMode(5,INPUT_PULLUP);         // YX BTN
  pinMode(48,INPUT_PULLUP); // SW 1
  pinMode(50,INPUT_PULLUP); // SW 2
  pinMode(52,INPUT_PULLUP); // SW 3
  //attachInterrupt(digitalPinToInterrupt(2), enc, RISING);
  //attachInterrupt(digitalPinToInterrupt(3), enc, RISING);
  //attachInterrupt(digitalPinToInterrupt(4), encBtn, RISING);
  //attachInterrupt(digitalPinToInterrupt(5), xyBtn, RISING);

//--------------------------------------------------------------------------------ZAPISYWANIE ADRESOW SKAL++++++++++++++++++++++++++++
//---------------------------------- Zapisanie adresow CHROMAYCZNEJ skali
   byte noteIterator=0;
for (byte i=1; i<7; i++)
{
  for (byte j=0; j<12; j++)
  {
      notePtrChroma[noteIterator] = new MIDIAddress;
      *notePtrChroma[noteIterator] = (MIDIAddress){note(j, i), CHANNEL_1};    // j - polton    0  11 --- i - oktawa  1  6
      // (poprawione, bez '()' )                                              //               C  B                  1  6 okt.
      noteIterator++;
  }
}

             // ----------- zapisze adresy na obiekty Button
cPADptr[0]=&cPAD1; cPADptr[1]=&cPAD2; cPADptr[2]=&cPAD3; cPADptr[3]=&cPAD4; cPADptr[4]=&cPAD5;
cPADptr[5]=&cPAD6; cPADptr[6]=&cPAD7; cPADptr[7]=&cPAD8; cPADptr[8]=&cPAD9; cPADptr[9]=&cPAD10;
cPADptr[10]=&cPAD11; cPADptr[11]=&cPAD12; cPADptr[12]=&cPAD13; cPADptr[13]=&cPAD14; cPADptr[14]=&cPAD15;

//---------------------------------- Zapisanie adresow obiektow nut (address) calej skali w wskaznikach // END

  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

  LEDS.addLeds<WS2812B,DATA_PIN,GRB>(leds,NUM_LEDS);
  LEDS.setBrightness(ledOverallBrightness);

  
for (byte i=0; i<NUM_LEDS; i++)
{
  aLpt[i]=new aLed(random(0,255),255,16);
}
padIterator1=0;
for (byte i=0; i<3; i++)
{
     for (byte j=0; j<5; j++)
      {
      matrixLpt[i][j]=aLpt[LtoRAssign[padIterator1]];
      padIterator1++;
      } 
}

  if (digitalRead(13)==HIGH) // -------------------------------------------- ZABEZPIECZENIE
  {
  Control_Surface.begin(); // Initialize Control Surface
  mux.begin(); // Initialize multiplexer
  
  cPAD1.begin(); cPAD2.begin(); cPAD3.begin(); cPAD4.begin(); cPAD5.begin();  // enables internal pull-up
  cPAD6.begin(); cPAD7.begin(); cPAD8.begin(); cPAD9.begin(); cPAD10.begin();
  cPAD11.begin(); cPAD12.begin(); cPAD13.begin(); cPAD14.begin(); cPAD15.begin();
  midi.begin();       // initialize the MIDI interface
  }

}

void loop() {

   Control_Surface.loop(); // Update dla potow, faderow
   //midi.update(); // read and handle or discard MIDI input
   customPadUpdate();

    //  ----------------------------------- ---------------------------KOLORY DLA POTOW 5-12
    int *muxpt = &muxs[0];
    for (byte m=0; m<12; m++)
    {
      (*muxpt)=mux.analogRead(m);
      muxpt++;
    }
    //--------------------Podmiana
    int buff = muxs[9];
    muxs[9]=muxs[10];
    muxs[10]=buff;
    //--------------------Zmapoj
    muxpt = &muxs[5];
    for (byte m=15; m<22; m++)
    {
      aLpt[m]->h = map((*muxpt),0,4095,0,192);
      aLpt[m]->s = 255;
      aLpt[m]->v = potsLedBlight;
      muxpt++;
    }
    // ------------------------------------ ---------------------------KOLORY DLA POTOW 5-12 /END

    
// * Pn 23 25 27 29 31 33 35 37 39 41 43 45 47 49 51
//      0  1  2   3 4  5  6  7  8  9  10 11 12 13 14
//   RL 51 47 49 43 45 35 37 39 33 41 31 27 29 23 25

if (MPROG==0) { // PROGRAM 0
    // ------------------------------------ ---------------------------KOLORY DLA PADOW 1-15
for (byte c=0; c<15; c++)
{
  if (digitalRead(padSet[c]) == HIGH) padClr[c]=random(0,255);
}
////////////////////////////////////////////////////////////////// PO WCISNIECIU
for (byte p=0; p<15; p++)
{
  if (digitalRead(padSet[p])==LOW)
  {
    aLpt[p]->h=padClr[p];
    aLpt[p]->s=255;
    aLpt[p]->v=255;
  }//{leds[p] = CHSV(padClr[p], 255, 255);}
}
    // -----------------------------------KOLORY DLA PADOW 1-15 /END
              } // PROGRAM 0    
////////////////////////////////////////////////////////////////// PO WCISNIECIU + MATRIX
else if (MPROG==1)   { // PROGRAM 1
padIterator1=0;
for (byte wier=0; wier<3; wier++)
{
  for (byte kol=0; kol<5; kol++)
  {
    if (digitalRead(padSet[LtoRAssign[padIterator1]])==LOW)
    {
      matrixLpt[wier][kol]->h=padLedBasePushCol;
      //matrixLpt[wier][kol]->s=255;
      matrixLpt[wier][kol]->v=255;
      ledWave[wier][kol] = 1;
    }
    padIterator1++;
  }
}
                } // PROGRAM 1
  // Glowne procedury drugiej kategori
if      (MPROG==0) assignLedsToMy();
else if (MPROG==1) assignLedsToMyMatrix();
FastLED.show();
sw3check();
if (firstUpdate==1) updateLogo();
encChk();
updateLCD();
BtnChk();
ledWaveMode();

if (firstUpdate==1)
{
  firstUpdate=0;
  for (int mx=0; mx<12; mx++) {muxsPrev[mx]=-127;}
}
//updateNotation();

  // Fade dla PAD LEDow -poprawic
if (millis()>ledMs+ledDel)
{
       if (MPROG==0)fadeAllToMy();
  else if (MPROG==1)fadeAllToMyColor();
  ledMs = millis();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////           
   /*
  for (pin_t pin = 0; pin < mux.getLength(); ++pin) {
    Serial.print(mux.analogRead(pin));
    Serial.print('\t');
  }
  Serial.println(); 
  */

  /* LEDS
    static uint8_t hue = 0;
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }

  // Now go in the other direction.  
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  */

  
   /*
   if (rot==3)
   {
    rot=0;
    tft.setRotation(rot);
   }
   else if (rot==0)
   {
    rot=1;
    tft.setRotation(rot);
   }
   else if (rot==1)
   {
    rot=2;
    tft.setRotation(rot);
   }
    else
   {
    rot=3;
    tft.setRotation(rot);
   }
   
    uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(500);

  // large block of text
  tft.fillScreen(ST77XX_BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
  delay(1000);

  // tft print function!
  tftPrintTest();
  delay(4000);

  // a single pixel
  tft.drawPixel(tft.width()/2, tft.height()/2, ST77XX_GREEN);
  delay(500);

  // line draw test
  testlines(ST77XX_YELLOW);
  delay(500);

  // optimized lines
  testfastlines(ST77XX_RED, ST77XX_BLUE);
  delay(500);

  testdrawrects(ST77XX_GREEN);
  delay(500);

  testfillrects(ST77XX_YELLOW, ST77XX_MAGENTA);
  delay(500);

  tft.fillScreen(ST77XX_BLACK);
  testfillcircles(10, ST77XX_BLUE);
  testdrawcircles(10, ST77XX_WHITE);
  delay(500);

  testroundrects();
  delay(500);

  testtriangles();
  delay(500);

  mediabuttons();
  delay(500);

  Serial.println("done");
  delay(1000);
  
  tft.invertDisplay(true);
  delay(500);
  tft.invertDisplay(false);
  delay(500);
}

void testlines(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, 0, x, tft.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, 0, tft.width()-1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, 0, 0, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, tft.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y=0; y < tft.height(); y+=5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x=0; x < tft.width(); x+=5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=tft.width()-1; x > 6; x-=6) {
    tft.fillRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color1);
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < tft.width(); x+=radius*2) {
    for (int16_t y=radius; y < tft.height(); y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < tft.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < tft.height()+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = tft.width()/2;
  int x = tft.height()-1;
  int y = 0;
  int z = tft.width();
  for(t = 0 ; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = tft.width()-2;
    int h = tft.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(4);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.print(p, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(" seconds.");
}

void mediabuttons() {
  // play
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_RED);
  delay(500);
  // pause
  tft.fillRoundRect(25, 90, 78, 60, 8, ST77XX_WHITE);
  tft.fillRoundRect(39, 98, 20, 45, 5, ST77XX_GREEN);
  tft.fillRoundRect(69, 98, 20, 45, 5, ST77XX_GREEN);
  delay(500);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_BLUE);
  delay(50);
  // pause color
  tft.fillRoundRect(39, 98, 20, 45, 5, ST77XX_RED);
  tft.fillRoundRect(69, 98, 20, 45, 5, ST77XX_RED);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_GREEN);
  */
}
