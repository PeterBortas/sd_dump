#include <SPI.h>
#include <SD.h>

// Change this
const String fileNameBase = "PROM2";
String curBaseName;

void setup() {
  serial_setup();
  sd_setup();

  // make a string for assembling the data to log:
  String dataString = "";

/*
  // read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2) {
      dataString += ",";
    }
  }
*/

  prom_setup();

  // allocate an unused filename
  File dataFile = sd_get_next_free(fileNameBase, "bin");

  if (dataFile) {
    uint32_t crc = prom_dump(dataFile);
    dataFile.close();
    File crcFile = sd_get_crcfile(curBaseName);
    crcFile.println(crc, HEX);
    crcFile.close();
  } else {
    Serial.println("error opening '"+ curBaseName +".bin");
  }
}

void myexit(int code)
{
  delay(1000); // Give serial a chance to complete
  exit(code);
}

void loop() {
  myexit(0);
}

