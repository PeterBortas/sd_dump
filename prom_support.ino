#include <CRC32.h>

CRC32 crc32;

#define MM1702A    1 /* NS MM1702A  2048-Bit (256x8) UV Erasable PROM as found on the Original Lys16 with 4KW BIOS */
#define I2716      2 /* As found on the 8KW BIOS boards */
#define TBP18S030  3 /* 256-bit (32x8) 3-state PROM */
#define HM7603     4 /* Harris HM7602 or HM7603 */
#define N82S129    5 /* Signetics N82S129N */
#define PROM HM7603

// NOTE: Each address will take DATASIZExREREADSxSTABDELAY to read. So that is 8s per byte with 8x10x100
#if PROM == I2716
#    define PROMSIZE 1024 // Size of PROM in adressable units. The size of that unit depends on the PROM
#    define DATASIZE 8 // Width of the data bus
#    define ADDRSIZE 11 // Width of the address bus (NOTE: Not enough pins on Uno, no longer supported)
#    define STABDELAY 25 // Time in milliseconds between setting the address and reading the data. 2716 needs 450ns from setting thte addresses, so theoreticly "1" would be OK.
#    define REREADS 8 // Times each bit should be retried with STABDELAY in between, for verifying flakey pins/addresses. Must be at least 1 to get any reads.
#    error "I2716 untested in new code"
#elif PROM == MM1702A  // National semiconductor clone of Intel 1702A. Should work with the original too.
#    define PROMSIZE 256 // Size of PROM in adressable units. The size of that unit depends on the PROM
#    define DATASIZE 8 // Width of the data bus
#    define ADDRSIZE 8 // Width of the address bus
#    define STABDELAY 25 // Time in milliseconds between setting the address and reading the data. MM1702A datasheet specifies exactly 1us, so 1ms should be good really
#    define REREADS 8 // Times each bit should be retried with STABDELAY in between, for verifying flakey pins/addresses. Must be at least 1 to get any reads.
#    error "MM1702A untested in new code"
#elif (PROM == TBP18S030) || (PROM == HM7603)
// http://hxc2001.free.fr/Squale/datasheet/TBP18S030.pdf
// http://pdf1.alldatasheet.com/datasheet-pdf/view/109677/HARRIS/HM-7602.html
#    define PROMSIZE 32 // Size of PROM in adressable units. The size of that unit depends on the PROM
#    define DATASIZE 8 // Width of the data bus
#    define ADDRSIZE 5 // Width of the address bus
#    define STABDELAY 1 // Time in milliseconds between setting the address and reading the data. datasheet says max 50ns 
#    define REREADS 1 // Times each bit should be retried with STABDELAY in between, for verifying flakey pins/addresses. Must be at least 1 to get any reads.
#elif PROM == N82S129
#    define PROMSIZE 256 // Size of PROM in adressable units. The size of that unit depends on the PROM
#    define DATASIZE 4 // Width of the data bus
#    define ADDRSIZE 8 // Width of the address bus
#    define STABDELAY 1 // Time in milliseconds between setting the address and reading the data. datasheet says max 50ns 
#    define REREADS 2 // Times each bit should be retried with STABDELAY in between, for verifying flakey pins/addresses. Must be at least 1 to get any reads.
#endif

#ifdef ARDUINO_AVR_MEGA2560
#    define ADDRSTARTPIN A8
#    define DATASTARTPIN 22
#    define CEPIN 53
#    define ADDRSKIP 1
#    define DATASKIP 2  //Just use the even ports on Mega
#else
#    error "No pin setup for this board"
#endif

int address = 0x0000; // Address to dump next

// the setup function runs once when you press reset or power the board
void prom_setup() {
  Serial.print("Setting pins to address output: ");
  for(int pin=ADDRSTARTPIN; pin != (ADDRSTARTPIN + (ADDRSIZE*ADDRSKIP)); pin += ADDRSKIP) {
    Serial.print(pin, DEC);
    Serial.print(" ");
    pinMode(pin, OUTPUT);
  }
  Serial.println();

  Serial.print("Setting pins to data input: ");
  for(int pin=DATASTARTPIN; pin != (DATASTARTPIN + DATASIZE); pin++) {
    Serial.print(pin, DEC);
    Serial.print(" ");
    pinMode(pin, INPUT);
  }
  Serial.println();

  Serial.print("Setting CE to output: ");
  Serial.print(CEPIN, DEC);
  Serial.println();
  pinMode(CEPIN, OUTPUT);
}

byte verifyRead(int pin)
{
  byte reads[REREADS];
  byte i, bad_reads=0;
  
  for(i = 0; i < REREADS; i++) {
    delay(STABDELAY);
    reads[i] = digitalRead(pin);
    if(i > 0)
      if(reads[i-1] != reads[i])
        bad_reads++;
  }
  
  if(bad_reads) {
    Serial.print("Bad reads on pin ");
    Serial.print(pin, DEC);
    Serial.print(":\t");
    for(i = 0; i < REREADS; i++)
      Serial.print(reads[i], DEC);
    Serial.println();
  }

  // Just return the last measurement we took regardless of failures
  return reads[REREADS-1];
}

int bitReverse(int num)
{
  return ((num & 0x01) << 7)
       | ((num & 0x02) << 5)
       | ((num & 0x04) << 3)
       | ((num & 0x08) << 1)
       | ((num & 0x10) >> 1)
       | ((num & 0x20) >> 3)
       | ((num & 0x40) >> 5)
       | ((num & 0x80) >> 7);
}

uint32_t prom_debug_print_data(int data)
{
    Serial.print(address, HEX);
    Serial.print("\t");
    Serial.print(data, DEC);
    Serial.print("\t");
    Serial.print(data, HEX);
    Serial.print("\t");
    if(data > 32 && data != 127)
      Serial.write(data);
    Serial.print("\t");
    if(bitReverse(data) > 32 && bitReverse(data) != 127)
      Serial.write(bitReverse(data));
    Serial.print("\trev: ");
    Serial.print(bitReverse(data), HEX);    
    Serial.println();   
}

uint32_t prom_dump(File sd_fd)
{
#if DATASIZE > 8
# error "Code needs to be fixed for 9+ bits
#endif

  Serial.print("Enabling chip\r\n");
  digitalWrite(CEPIN, LOW);
  
  Serial.print("Dump starting\r\n");
  Serial.print("addr\tdec\thex\tascii\trascii\tbitereversed hex\r\n");

  while(address < PROMSIZE) {
    uint8_t data = 0;

    for(int bit=0; bit<ADDRSIZE; bit++) {
      if((address>>bit)&1)
        digitalWrite(ADDRSTARTPIN+(bit*ADDRSKIP), HIGH);
      else
        digitalWrite(ADDRSTARTPIN+(bit*ADDRSKIP), LOW);
    }
    for(int bit=0; bit<DATASIZE; bit++) { 
      data = (data<<1) | verifyRead(DATASTARTPIN+(bit*DATASKIP));
    }

    prom_debug_print_data(data);
    sd_fd.write(data);
    crc32.update(data);
    
    address++;
  }

  // Set all address lines to low just to be on the safe side
  for(int bit=0; bit<ADDRSIZE; bit++) {
    digitalWrite(ADDRSTARTPIN+(bit*ADDRSKIP), LOW);
  }
  
  uint32_t checksum = crc32.finalize();
  Serial.print("Dump completed, CRC32: ");
  Serial.print(checksum, HEX);
  Serial.println();
  
  return checksum;
}

