#include <DaisyDuino.h>
#include <Arduino.h>
#include <Wire.h>
#include "b3ReadWavFile.h"
#include <STM32SD.h>
#include <AS5601.h>
#include "STM32TimerInterrupt.h"

#define SD_DETECT_PIN SD_DETECT_NONE
File root;

#define kMaxFiles 8

//64 bytes should be enough
char buf[64];

size_t file_cnt_ = 0, file_sel_ = 0;

int file_sizes[kMaxFiles];
int file_frames[kMaxFiles];
int file_beats[kMaxFiles];
MemoryDataSource dataSources[kMaxFiles];
#define WAV_FILENAME_MAX \
  256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */
int selected_file_index = 0;
int num_frames = -1;


double currtime = 0.0;
float samplerate = 100.f;

b3ReadWavFile wavFileReaders[kMaxFiles];

size_t pool_index = 0;
int allocation_count = 0;
#define CUSTOM_POOL_SIZE (32 * 1024 * 1024)

static char DSY_SDRAM_BSS custom_pool[CUSTOM_POOL_SIZE];
static float DSY_SDRAM_BSS posData[CUSTOM_POOL_SIZE / 4];
// static float DSY_SDRAM_BSS timeData[CUSTOM_POOL_SIZE / 8];
// static float DSY_SDRAM_BSS custom_pool[10];



char basePath[100];



static DaisyHardware hw;



double lastEcho = 0;
int sample = 0;
static int32_t inc;

float bpm = 120.0;
float currSpeed = 120.0;


double beatTime = 0;
double currNote = 0;
double timeSinceNote = 0;


Parameter speedknob;

Parameter samplespeedcontrol;

int baseSequence[] = { 0, -1, -2, -1, 0, -1, -2, -1, 0, -1, -2, -1, 0, -1, -2, -1 };

int sequence[64];


int sequence_length = 16;
int beatDivision = 4;
int sequenceDivision = 4;


int _currNoteInSequence = 0;
int _lastPlayedNote = 0;
int stepsSinceLastNote = 0;
float stepLength = 0;
float sequenceLengthInBeats = 0;

float adjustedTime = 0;
float offsetTime = 0;

int currSearch = 0;
int maxSearch = 64;
int prevNote = 0;

int chance = 0;

double currFrame = 0;

int sampleSpeedIndex = 3;
double sampleSpeed = 1;
AS5601 Sensor;

float smoothspeed = 0.0;
float smoothpos = 0.0;


double sensorReadings[4] = { 0.f, 0.f, 0.f, 0.f };
double currMeasure = 0.0;

float deltas[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };
float averageDelta = 0.f;


double lastStamp = 0;



int loopN = 0;
double delta = 0;
double angle = 0.f;
double lastAngle = 0.f;
double angleoffset = 0.f;

float speed = 0.0;

float sensorInterval = 4.0f;

int numPositions = 0;



bool getCurrentNote(double *outputNote,
                    double *timeSinceLastNote,
                    int sequence[],
                    int sequence_length,
                    int sequenceDivision,
                    double inputTime) {
  // *outputBeat = fmod(inputTime, 1.0) + floor(inputTime / 3.0) * 3;

  stepLength = 1.0 / sequenceDivision;
  sequenceLengthInBeats = sequence_length * stepLength;

  adjustedTime = fmod(inputTime, sequenceLengthInBeats);

  _currNoteInSequence = floor(adjustedTime / stepLength);

  offsetTime = adjustedTime - _currNoteInSequence * stepLength;

  currSearch = -1;
  _lastPlayedNote = -1;
  while (_lastPlayedNote == -1 && currSearch < sequence_length) {
    currSearch++;
    _lastPlayedNote = sequence[int(
      fmod(2 * sequence_length + _currNoteInSequence - currSearch,
           sequence_length))];
  }


  if (_currNoteInSequence != prevNote) {
    prevNote = _currNoteInSequence;
  }

  if (_lastPlayedNote == -2) {
    return false;
  }

  *outputNote = _lastPlayedNote;
  *timeSinceLastNote = currSearch * stepLength + offsetTime;
  return true;
}

double CubicHermite(double A, double B, double C, double D, double t) {
  double a = -A / 2.0f + (3.0f * B) / 2.0f - (3.0f * C) / 2.0f + D / 2.0f;
  double b = A - (5.0f * B) / 2.0f + 2.0f * C - D / 2.0f;
  double c = -A / 2.0f + C / 2.0f;
  double d = B;

  return a * t * t * t + b * t * t + c * t + d;
}

double interpolateCubic(double t) {
  return CubicHermite(sensorReadings[0], sensorReadings[1], sensorReadings[2], sensorReadings[3], t);
}

double interpolate(double t) {
  return sensorReadings[1] + (sensorReadings[2] - sensorReadings[1]) * t;
}

double numCyclesRead = 8.f;
double currCycle = 0;
double currframe = 0;
void AudioCallback(float **in, float **out, size_t size) {
  hw.DebounceControls();
  inc = hw.encoder.Increment();
  sample += inc;
  sample = (sample + file_cnt_) % file_cnt_;

  if (hw.encoder.RisingEdge()) {
    for (int i = 0; i < sequence_length; i++) {
      chance = rand() % 100;
      if (chance > 50) {
        sequence[i] = 2 * (rand() % 8);
      } else if (chance > 25) {
        sequence[i] = -2;
      } else {
        sequence[i] = -1;
      }
    }
  }

  bpm += 0.01 * ((60.f + 240.f * analogRead(PIN_POD_POT_1) / 1023.f) - bpm);
  sampleSpeed += 0.01 * ((-2.f + 4.f * analogRead(PIN_POD_POT_2) / 1023.f) - sampleSpeed);

  if (floor(currframe / (numCyclesRead * 32.f)) != currCycle) {
    currCycle = floor(currframe / (numCyclesRead * 32.f));
    sensorReadings[0] = sensorReadings[1];
    sensorReadings[1] = sensorReadings[2];
    sensorReadings[2] = sensorReadings[3];
    sensorReadings[3] = currMeasure;

    speed += (abs(sensorReadings[3] - sensorReadings[0]) - speed) * 0.2;
  }

  for (size_t i = 0; i < size; i++) {
    double nowt = interpolate(fmod(currframe, numCyclesRead * 32.f) / (numCyclesRead * 32.f));
    out[0][i] = 1.0 * wavFileReaders[sample].interpolate(nowt, 0, dataSources[sample]);
    out[1][i] = 1.0 * wavFileReaders[sample].interpolate(nowt, 1, dataSources[sample]);
    currframe++;
  }

  // currtime += 1000.f * (float)size / samplerate;
}




void *custom_pool_allocate(size_t size) {
  if (pool_index + size >= CUSTOM_POOL_SIZE) {
    return 0;
  }
  void *ptr = &custom_pool[pool_index];
  pool_index += size;
  return ptr;
}

void loadFiles(void) {

  delay(100);
  Serial.println(" connected to SD");


  strcpy(basePath, "/reexport");

  root = SD.open(basePath);

  file_sel_ = 0;

  if (root) {

    Serial.printf("opened directory : %s\n", basePath);
    Serial.println("loading files :");
    strcat(basePath, "/");

    Serial.printf("basepath is now : %s\n", basePath);


    UINT bytesRead = 0;

    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        // no more files
        break;
      }
      if (!entry.isDirectory()) {
        // files have sizes, directories do not
        if ((strstr(entry.name(), ".wav") || strstr(entry.name(), ".WAV"))
            && !strstr(entry.name(), ".asd")) {
          Serial.printf("found wav file: %s\n", entry.name());
          Serial.print("size is : ");
          Serial.println(entry.size(), DEC);
          file_sizes[file_cnt_] = entry.size();
          UINT size = file_sizes[file_cnt_];
          char *memoryBuffer = 0;
          memoryBuffer = (char *)custom_pool_allocate(size);
          if (memoryBuffer) {
            // Read the whole WAV file
            Serial.println("got membuffer, trying to read in");
            entry.read((char *)memoryBuffer, size);
            Serial.println("read in!");
            dataSources[file_cnt_] = MemoryDataSource(
              memoryBuffer, size);
            Serial.printf("sample size is %i\n",
                          size);
            char *err = wavFileReaders[file_cnt_].getWavInfo(dataSources[file_cnt_]);
            if (strcmp(err, "z") != 0) {
              Serial.printf(
                "error happened whilst getting the wav "
                ": %s \n",
                err);
            } else {
              char *token = strtok(entry.name(), ".");
              long int num_beats = strtol(
                &token[strlen(token) - 2], NULL, 10);
              Serial.printf(
                "num beats is %i\n",
                num_beats);  //printing the token

              wavFileReaders[file_cnt_].setNumBeats(
                num_beats);

              num_frames = wavFileReaders[file_cnt_]
                             .getNumFrames();
              file_frames[file_cnt_] = num_frames;

              Serial.printf("sample length is %i\n",
                            wavFileReaders[file_cnt_]
                              .getNumFrames());
              wavFileReaders[file_cnt_].resize();
              file_cnt_++;
            }
          } else {
            Serial.println("no memorybuffer");
          }
          Serial.printf("num files is %i\n", file_cnt_);
        }
      }
      entry.close();
    }
  } else {
    Serial.printf("couldnt open dir");
  }
  root.close();
}

File posDataFile;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  // while (!Serial) yield();
  Serial.println("Inited Serial");
  Serial.println("inting daisy");
  hw = DAISY.init(DAISY_POD, AUDIO_SR_48K);
  samplerate = DAISY.get_samplerate();
  DAISY.SetAudioBlockSize(32);
  Sensor.begin();
  Serial.println("inited daisy");
  while (!SD.begin(SD_DETECT_PIN)) {
    Serial.println("can't connect to SD");
    delay(10);
  }
  loadFiles();
  SD.remove("/posdata.txt");
  posDataFile = SD.open("/posdata.txt", FILE_WRITE);
  DAISY.begin(AudioCallback);
}

float lasttime = 0.f;
void loop() {
  angle = 1.0 - (Sensor.getAngle() / 4096.0);
  if (angle < 0.2 && lastAngle > 0.8) {
    angleoffset += 1.f;
  } else if (angle > 0.8 && lastAngle < 0.2) {
    angleoffset -= 1.f;
  }
  lastAngle = angle;

  // currMeasure += 0.05 * ((angle + angleoffset) - currMeasure);
  currMeasure = angle + angleoffset;
  Serial.print("speed :");
  Serial.println(speed);
  // lasttime = micros();
  // currMeasure = micros() / 1000000.f;
  // delay(20);
  // Serial.println(currframe);
}