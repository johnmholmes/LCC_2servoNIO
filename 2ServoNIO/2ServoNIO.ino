/* LCC ESP32 2 servo N IO Basic series

  Report bugs if anything is found


This is my test version for demonstration CAN Bus use only by John Holmes
  - Pins 15 RX and 2 TX for the transceiver module
  - Pins 14,27,26,25,16,17,18,19  are used for input or output
  - Pins 32,33 servos

*/
//==============================================================
// AVR 2Servos NIO using ESPcan
//
// Coprright 2024 David P Harris
// derived from work by Alex Shepherd and David Harris
// Updated 2024.11.14 DPH
// Updated 2026 6 24 John Holmes
//==============================================================

#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board defintions, see "Boards.h"
#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.


#define OLCB_NO_BLUE_GOLD // Do not delete

extern "C" {
    #define N(x) xN(x)     
    #define xN(x) #x       
const char configDefInfo[] PROGMEM =
    CDIheader R"(
    <name>Application Configuration</name>
    <group>
        <name>Turnout Servo Configuration</name>
        <int size='1'>
          <name>Speed 5-50 (delay between steps)</name>
          <min>5</min><max>50</max>
          <hints><slider tickSpacing='15' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <int size='1'><name>Save servo positions every x*x= seconds</name></int>
    </group>
    <group replication=')" N(NUM_SERVOS) R"('>
        <name>Servos</name>
        <repname>Servo Pin 32</repname>
        <repname>Servo Pin 33</repname>
        <string size='8'><name>Description</name></string>
        <group replication=')" N(NUM_POS) R"('>
        <name>  Closed     Midpoint    Thrown</name>
            <repname>Position</repname>
            <eventid><name>EventID</name></eventid>
            <int size='1'>
                <name>Servo Position in Degrees</name>
                <min>0</min><max>180</max>
                <hints><slider tickSpacing='45' immediate='yes' showValue='yes'> </slider></hints>
            </int>
        </group>
    </group>
    <group replication=')" N(NUM_IO) R"('>
        <name>Input/Output</name> 
        <repname>Pin 14</repname>
        <repname>Pin 27</repname>
        <repname>Pin 26</repname>
        <repname>Pin 25</repname>
        <repname>Pin 16</repname>
        <repname>Pin 17</repname>
        <repname>Pin 18</repname>
        <repname>Pin 19</repname>

        <string size='8'><name>Description</name></string>
        <int size='1'>
            <name>Channel type</name>
            <map>
                <relation><property>0</property><value>None</value></relation> 
                <relation><property>1</property><value>Input</value></relation> 
                <relation><property>2</property><value>Input, Inverted</value></relation> 
                <relation><property>3</property><value>Input with pull-up</value></relation>
                <relation><property>4</property><value>Input with pull-up, Inverted</value></relation>
                <relation><property>5</property><value>Toggle</value></relation>
                <relation><property>6</property><value>Toggle with pull-up</value></relation>
                <relation><property>7</property><value>Output Phase A</value></relation>
                <relation><property>8</property><value>Output Phase A, Inverted</value></relation>
                <relation><property>9</property><value>Output Phase B</value></relation>
                <relation><property>10</property><value>Output Phase B, Inverted</value></relation>
            </map>
        </int>
        <int size='1'>
            <name>On-Duration/On-delay 1-255 = 100ms-25.5s, 0=steady-state</name>
            <hints><slider tickSpacing='85' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <int size='1'>
            <name>Off-Period/Off-delay 1-255 = 100ms-25.5s, 0=No repeat</name>
            <hints><slider tickSpacing='85' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <eventid><name>Pin HIGH State Event</name></eventid>
        <eventid><name>Pin LOW State Event</name></eventid>
    </group>
    )" CDIfooter;
} 

typedef struct {
      EVENT_SPACE_HEADER eventSpaceHeader; 
      char nodeName[20];  
      char nodeDesc[24];  
      uint8_t servodelay; 
      uint8_t saveperiod; 
      struct {
        char desc[8];        
        struct {
          EventID eid;       
          uint8_t angle;     
        } pos[NUM_POS];
      } servos[NUM_SERVOS];
      struct {
        char desc[8];
        uint8_t type;
        uint8_t duration;    
        uint8_t period;      
        EventID onEid;
        EventID offEid;
      } io[NUM_IO];
  uint8_t curpos[NUM_SERVOS];  
} MemStruct;                 

uint8_t curpos[NUM_SERVOS]; 

extern "C" {
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        SERVOEID(NUM_SERVOS),
        IOEID(NUM_IO)
    };
    extern const char SNII_const_data[] PROGMEM = "\001" MANU "\000" MODEL "\000" HWVERSION "\000" OlcbCommonVersion ; 
} 

uint8_t protocolIdentValue[6] = {   
        pSimple | pDatagram | pMemConfig | pPCEvents | !pIdent    | pTeach     | !pStream   | !pReservation, 
        pACDI   | pSNIP     | pCDI       | !pRemote  | !pDisplay  | !pTraction | !pFunction | !pDCC        , 
        0, 0, 0, 0                                                                                           
};

Servo servo[NUM_SERVOS];
uint8_t servoActual[NUM_SERVOS];
uint8_t servoTarget[NUM_SERVOS];
uint8_t servopin[]  = { SERVOPINS };
uint8_t iopin[] = { IOPINS };

bool iostate[NUM_IO] = {0};  
bool logstate[NUM_IO] = {0}; 
unsigned long next[NUM_IO] = {0};

// Exact data byte layout locations discovered via serial debugging maps
const uint32_t SERVO_DELAY_OFFSET = 48; 
const uint32_t SAVE_PERIOD_OFFSET  = 49;
bool posdirty = false;

void reportConfig() {
  dP("\n 2Servo8IO");
  dP("\nFile: " __FILE__);
  dP("\nUsing " BOARD);
  dP("\nNode ID="); dP(TOSTRING((NODE_ADDRESS)));
  dP("\nServo pins:"); for(int i=0; i<2; i++) { dP(" "); dP(servopin[i]); }
  dP("\nIO pins:"); for(int i=0; i<sizeof(iopin); i++) { dP(" "); dP(iopin[i]); }
  dP("\nCAN pins: Tx="); dP(CAN_TX_PIN); dP(" RX="); dP(CAN_RX_PIN);
}

void userInitAll()
{ 
  NODECONFIG.put(EEADDR(nodeName), ESTRING("Esp32"));
  NODECONFIG.put(EEADDR(nodeDesc), ESTRING("2Servos8IO"));
  NODECONFIG.update(SERVO_DELAY_OFFSET, 20);
  NODECONFIG.update(SAVE_PERIOD_OFFSET, 190);   
  for(uint8_t i = 0; i < NUM_SERVOS; i++) {
    NODECONFIG.put(EEADDR(servos[i].desc), ESTRING(""));
    for(int p=0; p<NUM_POS; p++) {
      NODECONFIG.update(EEADDR(servos[i].pos[p].angle), 90);
      NODECONFIG.update(EEADDR(curpos[i]), 0);
    }
  }
  for(uint8_t i = 0; i < NUM_IO; i++) {
    NODECONFIG.put(EEADDR(io[i].desc), ESTRING(""));
    NODECONFIG.update(EEADDR(io[i].type), 0);
    NODECONFIG.update(EEADDR(io[i].duration), 0);
    NODECONFIG.update(EEADDR(io[i].period), 0);
  }  
  EEPROMcommit;
}

enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
  dP("\n userState "); dP((uint16_t) index);
    if(index < NUM_SERVOS*NUM_POS) {
      int ch = index/3;
      int pos = index%3;
      if( curpos[ch]==pos ) return VALID;
                else return INVALID;
    } else {
      int ch = (index-NUM_SERVOS*NUM_POS)/2;
      if( NODECONFIG.read(EEADDR(io[ch].type))==0) return UNKNOWN;
      int evst = index % 2;
      if( iostate[ch]==evst ) return VALID;
    }
    return INVALID;
}
    
#ifndef PV
  #define PV(x) { dP(" " #x "="); dP(x); }
#endif

void pceCallback(uint16_t index) {
  dP("\npceCallback, index="); dP((uint16_t)index);
    if(index<NUM_SERVOS*NUM_POS) {
      uint8_t n = index / 3;
      uint8_t p = index % 3;
      curpos[n] = p;
      servoTarget[n] = NODECONFIG.read( EEADDR(servos[n].pos[p].angle) );
      dP("\n servo#"); dP(n); dP(" position#"); dP(p); dP(" target angle="); dP(servoTarget[n]); 
    } else {
      uint8_t n = index-NUM_SERVOS*NUM_POS;
      uint8_t type = NODECONFIG.read(EEADDR(io[n/2].type));
      dP("\nevent"); dP(" n="); dP(n); dP(" type="); dP(type);
      if(type>=7) {
        bool inv = !(type&1);       
        if(n%2) {
          digitalWrite( iopin[n/2], inv);
          next[n/2] = 0;
        } else {
          bool pha = type<9;       
          digitalWrite( iopin[n/2], pha ^ inv);
          iostate[n/2] = 1;
          uint8_t durn = NODECONFIG.read(EEADDR(io[n/2].duration));
          if(durn) next[n/2] = millis() + 100*durn; 
          else next[n/2]=0;
        }
      }
    }
}
void printMem();

void produceFromInputs() {
    const uint8_t base = NUM_SERVOS*NUM_POS;
    static uint8_t c = 0;
    static unsigned long last = 0;
    if((millis()-last)<(50/NUM_IO)) return;
    last = millis();
    uint8_t type = NODECONFIG.read(EEADDR(io[c].type));
    uint8_t d;
    if(type==5 || type==6) {
      bool s = digitalRead(iopin[c]);
      if(s != iostate[c]) {
        iostate[c] = s;
        if(!s) {
          logstate[c] ^= 1;
          if(logstate[c]) d = NODECONFIG.read(EEADDR(io[c].duration));
          else            d = NODECONFIG.read(EEADDR(io[c].period));
          if(d==0) OpenLcb.produce( base+c*2 + logstate[c] ); 
          else next[c] = millis() + (uint16_t)d*100;          
        }
      }
    }
    if(type>0 && type<5) {
      bool s = digitalRead(iopin[c]);
      if(s != iostate[c]) {
        iostate[c] = s;
        if(!iostate[c]) d = NODECONFIG.read(EEADDR(io[c].duration)); 
        else d = NODECONFIG.read(EEADDR(io[c].period));
        if(d==0) OpenLcb.produce( base+c*2 + (!s^(type&1)) ); 
        else {
          next[c] = millis() + (uint16_t)d*100;                   
        }
      }
    }
    if(++c>=NUM_IO) c = 0;
}

void processProducer() {
  const uint8_t base = NUM_SERVOS*NUM_POS;
  static unsigned long last = 0;
  unsigned long now = millis();
  if( (now-last) < 50 ) return;
  for(int c=0; c<NUM_IO; c++) {
    if(next[c]==0) continue;
    if(now<next[c]) continue; 
    uint8_t type = NODECONFIG.read(EEADDR(io[c].type));
    if(type>6) return; 
    uint8_t s = iostate[c];
    if(type<5)  OpenLcb.produce( base+c*2 + (!s^(type&1)) ); 
    else OpenLcb.produce( base+c*2 + logstate[c] );          
    next[c] = 0;
  }
}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);  
#include "OpenLCBMid.h"    

#ifndef P
  #define P(...) Serial.print( __VA_ARGS__)
#endif
#ifndef PVL
  #define PVL(x) { P("\n"); P(#x  "="); P(x); }
#endif

void printMem() {
  PVL(NODECONFIG.read(SERVO_DELAY_OFFSET));
  PVL(NODECONFIG.read(SAVE_PERIOD_OFFSET));
  for(int s=0;s<NUM_SERVOS;s++) {
    P("\nServo "); P(s); 
    uint8_t cp = NODECONFIG.read(EEADDR(curpos[s])); 
    P(" cp="); P(cp);
    uint8_t angle1 = NODECONFIG.read(EEADDR(servos[s].pos[0].angle)); 
    P(" angle1="); P(angle1);
    uint8_t angle2 = NODECONFIG.read(EEADDR(servos[s].pos[1].angle)); 
    P(" angle2="); P(angle2);
    uint8_t angle3 = NODECONFIG.read(EEADDR(servos[s].pos[2].angle)); 
    P(" angle3="); P(angle3);
  }
}

void userConfigWritten(uint32_t address, uint16_t length, uint16_t func)
{
  dPS("\nuserConfigWritten: Addr: ", (uint32_t)address);
  dPS(" Len: ", (uint16_t)length);
  dPS(" Func: ", (uint8_t)func);
  
  EEPROMcommit;
  setupIOPins();
  servoSet();
}

uint32_t getSavePeriod() {
  uint32_t saveperiod = NODECONFIG.read( SAVE_PERIOD_OFFSET );
  return saveperiod * saveperiod * 1000;
}

// Background FreeRTOS Execution Thread for precise hardware timing controls
// Background FreeRTOS Execution Thread using variable step sizing
void servoBackgroundTask(void * parameter) {
  for(;;) {
    // Read the slider value (typically 5 to 50)
    uint8_t sliderVal = NODECONFIG.read( SERVO_DELAY_OFFSET );
    if (sliderVal < 1) sliderVal = 1; 

    // Calculate a dynamic step increment based on the slider
    // Adjust the division factor if you want it faster or slower overall
    uint8_t stepSize = sliderVal / 5; 
    if (stepSize < 1) stepSize = 1;

    // Run the loop at a fixed, smooth 20ms refresh rate
    vTaskDelay(pdMS_TO_TICKS(20));

    static long lastmove = 0;
    
    for(int i=0; i<NUM_SERVOS; i++) {
      if(servoTarget[i] == servoActual[i] ) continue;
      
      // Move by the calculated step size instead of just 1 degree
      if(servoTarget[i] > servoActual[i]) {
        if ((servoTarget[i] - servoActual[i]) > stepSize) {
          servoActual[i] += stepSize;
        } else {
          servoActual[i] = servoTarget[i]; // Close enough, snap to target
        }
      }
      else if(servoTarget[i] < servoActual[i]) {
        if ((servoActual[i] - servoTarget[i]) > stepSize) {
          servoActual[i] -= stepSize;
        } else {
          servoActual[i] = servoTarget[i]; // Close enough, snap to target
        }
      }
      
      if(!servo[i].attached()) { 
        servo[i].attach(servopin[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      servo[i].write(servoActual[i]);
      lastmove = millis();
      posdirty = true;
    }

    if( lastmove && (millis()-lastmove)>4000) {
      for(int i=0; i<NUM_SERVOS; i++) servo[i].detach();
      lastmove = 0;
    }
  }
}

void servoStartUp() {
  for(int i=0; i<NUM_SERVOS; i++) {
    if(getSavePeriod()==0) curpos[i] = 1; 
    else curpos[i] = NODECONFIG.read( EEADDR( curpos[i] ) );
    if( USE_90_ON_STARTUP ) servoActual[i] = 90;
    else servoActual[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
    servo[i].attach(servopin[i]);
    servo[i].write(servoActual[i]);
    delay(100);
  }
  servoSet();
}

void servoSet() {
  for(int i=0; i<NUM_SERVOS; i++) {
    servoTarget[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
  }
}

void handleServoSave() {
  static long lastsave = 0;
  uint32_t saveperiod = getSavePeriod();
  if(saveperiod && posdirty && (millis()-lastsave) > saveperiod ) {
    lastsave = millis();
    posdirty = false;
    for(int i=0; i<NUM_SERVOS; i++) NODECONFIG.update( EEADDR(curpos[i]), curpos[i]);
    EEPROMcommit;
  }
}

void setupIOPins() {
  dP("\nPins: ");
  for(uint8_t i=0; i<NUM_IO; i++) {
    uint8_t type = NODECONFIG.read( EEADDR(io[i].type));
    switch (type) {
      case 1: case 2: case 5:
        dP(" IN:");
        pinMode(iopin[i], INPUT); 
        iostate[i] = type&1;
        if(type==5) iostate[i] = 0;
        break;
      case 3: case 4: case 6:
        dP(" INP:");
        pinMode(iopin[i], INPUT_PULLUP); 
        iostate[i] = type&1;
        break;
      case 7: case 8: case 9: case 10:
        dP(" OUT:");
        pinMode(iopin[i], OUTPUT); 
        iostate[i] = !type&1;
        digitalWrite(iopin[i], !type&1);
        break;
    }
    dP(iopin[i]); dP(":"); dP(type); dP(", ");
  }
}

void appProcess() {
  uint8_t base = NUM_SERVOS * NUM_POS;
  unsigned long now = millis();
  for(int i=0; i<NUM_IO; i++) {
    uint8_t type = NODECONFIG.read(EEADDR(io[i].type));
    if(type >= 7) {
      if( next[i] && now>next[i] ) {
        bool inv = !(type&1);
        bool phb = type>8;
        if(iostate[i]) {
          digitalWrite(iopin[i], phb ^ inv);
          iostate[i] = 0;
          if( NODECONFIG.read(EEADDR(io[i].period)) > 0 ) 
            next[i] = now + 100*NODECONFIG.read(EEADDR(io[i].period));
          else next[i] = 0;
        } else {
          digitalWrite(iopin[i], !phb ^ inv);
          iostate[i] = 1;
          if( NODECONFIG.read(EEADDR(io[i].duration)) > 0 )
            next[i] = now + 100*NODECONFIG.read(EEADDR(io[i].duration));
          else next[i] = 0;
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200); while(!Serial);
  delay(2000);
  dP("\n HiHo");

  EEPROMbegin;
  NodeID nodeid(NODE_ADDRESS);       
  Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
  reportConfig();

  servoStartUp();
  setupIOPins();

  // Spawns structural execution framework thread on Core 0 natively
  xTaskCreatePinnedToCore(
    servoBackgroundTask,   
    "ServoTask",           
    4096,                  
    NULL,                  
    1,                     
    NULL,                  
    0                      
  );
}

void loop() {
  Olcb_process();        
  produceFromInputs();  
  appProcess();         
  processProducer();    
  handleServoSave();    
}