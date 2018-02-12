const int chipSelect = 4; //Modify this if you are not using the official eth shield

void sd_setup()
{
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    myexit(1);
  }
  Serial.println("card initialized.");  
}

// Use the base filename and get the next unused filename
File sd_get_next_free(String base, String suffix)
{
  String fname;
  bool found = false;
  int n = 0;
  while(!found) {
    if(!n) {
      fname = base;
    } else {
      fname = base +"-"+ n;
    }
    curBaseName = fname;
    fname = fname +"."+ suffix;
    if( !SD.exists(fname) ) {
      Serial.println("filename: "+ fname);
      return SD.open(fname, FILE_WRITE);
    }
    n++;
  }
}

File sd_get_crcfile(String base)
{
  String fname = base +".crc";
  if( SD.exists(fname) ) {
    Serial.println(fname +" already exists! Not writing a new one. Clean up your SD!");
    myexit(1);
  }

  return SD.open(fname, FILE_WRITE);
}

