#define MAX_BYTES    200

// define three track TTL reader inputs and card present pins
#define CARD_PRESENT 2
#define TRACK1_CLOCK 3
#define TRACK1_DATA  4
#define TRACK2_CLOCK 5
#define TRACK2_DATA  6
#define TRACK3_CLOCK 7
#define TRACK3_DATA  8

// pending flags are used to detect clock edge
bool pending1;
bool pending2;
bool pending3;

// index and data allow us to store the captured bits
int bits1;
int bits2;
int bits3;
uint8_t data1[MAX_BYTES];
uint8_t data2[MAX_BYTES];
uint8_t data3[MAX_BYTES];

void setup() { 
  Serial.begin(9600);

  pinMode(CARD_PRESENT, INPUT);
  pinMode(TRACK1_CLOCK, INPUT);
  pinMode(TRACK1_DATA,  INPUT);
  pinMode(TRACK2_CLOCK, INPUT);
  pinMode(TRACK2_DATA,  INPUT);
  pinMode(TRACK3_CLOCK, INPUT);
  pinMode(TRACK3_DATA,  INPUT);
} 

void loop() {
  if (digitalRead(CARD_PRESENT)) {
    return;
  }

  // clear out any old or uninitialized values
  pending1 = pending2 = pending3 = true;
  bits1 = bits2 = bits3 = 0;  
  memset(data1, 0, MAX_BYTES);
  memset(data2, 0, MAX_BYTES);
  memset(data3, 0, MAX_BYTES);

  while (!digitalRead(CARD_PRESENT)) {
    // scan each track trying to capture bits on falling clock edge
    scanTrack(TRACK1_CLOCK, TRACK1_DATA, &pending1, &bits1, data1);
    scanTrack(TRACK2_CLOCK, TRACK2_DATA, &pending2, &bits2, data2);
    scanTrack(TRACK3_CLOCK, TRACK3_DATA, &pending3, &bits3, data3);
  }
  
  // output what we found
  Serial.print("\n");
  dump(1, data1, bits1);
  dump(2, data2, bits2);
  dump(3, data3, bits3);
}

void scanTrack(int CLOCK, int DATA, bool* pending, int* bits, uint8_t* data)
{ 
  // if the clock is high set pending to be true and carry on
  // if the track is low and pending is true (so we just transitioned) read data and clear pending
  if (digitalRead(CLOCK)) {
    *pending = true;
  } else if (*pending) {
    *pending    = false;
    int index   = (*bits)++ / 8;
    data[index] = (data[index] << 1) + !digitalRead(DATA);
  }    
}

#define GET_BIT(byte, bit) (((byte) >> (7 - (bit))) & 1)
void dump(int track, uint8_t* data, int bits)
{
  int skip=0, b=0, i;
  int byte, bit, width, firstBad;
  bool parity;
  
  Serial.print("\nTrack ");
  Serial.print(track);
  Serial.print(" ");
  Serial.print(bits);
  Serial.print(" bits \nRaw Hex: ");
  
  // print out the raw version in hex. will drop some trailing zeros 
  // if not evenly divisible by 8, meh
  b=0;
  while (b < bits) {
    Serial.print("\\x");
    byte = data[b/8];
    if (byte <= 0x0F) {
      Serial.print("0");
    }
    Serial.print(byte, HEX);
    b += 8;
  }

  // skip over leading 0 bits
  for (b = 0; b < bits && !GET_BIT(data[b/8], b%8); b++);
  skip = b;
  
  Serial.print("\n");
  Serial.print(skip);
  Serial.print(" zero/clocking bits skipped ");
  
  // detect if its 5 or 7 bit text (4 or 6 data bits plus odd parity)
  for (width=0, i=0, parity=0; b < bits && i < 7; b++, i++) {
    bit = GET_BIT(data[b/8], b%8);
    if ((i == 4 || i == 6) && bit != parity) {
      width = i;
      break;
    }
    
    // update our parity to date
    parity ^= bit;
  }
  
  if (!width) {
    Serial.print("based on parity, data isn't 5 or 7 bit text; decode skipped\n");
    return;
  }
  
  Serial.print("detected ");
  Serial.print(width+1);
  Serial.print(" bit data, decoded:\n"); 
  
  // convert to ascii and ouptut so long as we have bits to give and no issues detected
  b = skip;
  while (b < bits) {
    // try to read out data bits and track parity
    for (parity=0, byte=0, i=0; b < bits && i < width; i++, b++) {
      bit     = GET_BIT(data[b/8], b%8);
      parity ^= bit;
      byte   += (bit << i);
    }
    
    // jump ship, silently, on incomplete byte
    if (i < width) {
      break;
    }
    
    // skip over parity bit but flag failures
    b++;
    if (!firstBad && parity != GET_BIT(data[b/8], b%8)) {
      firstBad = b/8;
      break;
    }
    
    // offset byte for 7 or 5 bit text and print value
    Serial.print((char) (byte + (width == 6 ? 0x20 : 0x30)));
  }
  
  if (firstBad) {
   Serial.print("\nOne or more bad parity detected starting at byte ");
   Serial.print(firstBad);
  }
  
  Serial.print("\n");
}
