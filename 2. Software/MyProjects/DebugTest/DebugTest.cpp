/// NEED TO WORK OUT HOW TO APPEND FILE PATHS TOGETHER< THAT IS THE BUG CURRENTLY


#include "daisy_pod.h"
#include "daisysp.h"
#include "DebugTest.h"
#include "b3ReadWavFile.h"


#include "fatfs.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;
uint8_t  dir_count = 0;


SdmmcHandler   sd;
FatFSInterface fsi;
FIL SDFile; // Needs to be global to work https://forum.electro-smith.com/t/fatfs-f-read-returns-fr-disk-err/2883

#define kMaxFiles 64

//64 bytes should be enough
char buf[64];

size_t file_cnt_ = 0, file_sel_ = 0;

WavFileInfo      file_info_[kMaxFiles];
int              file_sizes[kMaxFiles];
int              file_frames[kMaxFiles];
int              file_beats[kMaxFiles];
MemoryDataSource dataSources[kMaxFiles];
#define WAV_FILENAME_MAX \
    256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */
int         selected_file_index = 0;
int         num_frames          = -1;
std::string sd_debug_msg        = "no sdcard";


b3ReadWavFile wavFileReaders[kMaxFiles];


double         lastEcho = 0;
int            sample   = 0;
static int32_t inc;

float bpm       = 120.0;
float currSpeed = 120.0;


double beatTime      = 0;
double currNote      = 0;
double timeSinceNote = 0;


Parameter speedknob;

Parameter samplespeedcontrol;

int baseSequence[]
    = {0, -1, -2, -1, 0, -1, -2, -1, 0, -1, -2, -1, 0, -1, -2, -1};

int sequence[64];


int sequence_length  = 16;
int beatDivision     = 4;
int sequenceDivision = 4;


int   _currNoteInSequence   = 0;
int   _lastPlayedNote       = 0;
int   stepsSinceLastNote    = 0;
float stepLength            = 0;
float sequenceLengthInBeats = 0;

float adjustedTime = 0;
float offsetTime   = 0;

int currSearch = 0;
int maxSearch  = 64;
int prevNote   = 0;

int chance = 0;

double currFrame = 0;

int    sampleSpeedIndex = 3;
double sampleSpeed      = 1;


bool getCurrentNote(double *outputNote,
                    double *timeSinceLastNote,
                    int     sequence[],
                    int     sequence_length,
                    int     sequenceDivision,
                    double  inputTime)
{
    // *outputBeat = fmod(inputTime, 1.0) + floor(inputTime / 3.0) * 3;

    stepLength            = 1.0 / sequenceDivision;
    sequenceLengthInBeats = sequence_length * stepLength;

    adjustedTime = fmod(inputTime, sequenceLengthInBeats);

    _currNoteInSequence = floor(adjustedTime / stepLength);

    offsetTime = adjustedTime - _currNoteInSequence * stepLength;

    currSearch      = -1;
    _lastPlayedNote = -1;
    while(_lastPlayedNote == -1 && currSearch < sequence_length)
    {
        currSearch++;
        _lastPlayedNote = sequence[int(
            fmod(2 * sequence_length + _currNoteInSequence - currSearch,
                 sequence_length))];
    }


    if(_currNoteInSequence != prevNote)
    {
        prevNote = _currNoteInSequence;
    }

    if(_lastPlayedNote == -2)
    {
        return false;
    }

    *outputNote        = _lastPlayedNote;
    *timeSinceLastNote = currSearch * stepLength + offsetTime;
    return true;
}
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.encoder.Debounce();
    inc = hw.encoder.Increment();
    sample += inc;
    sample = (sample + file_cnt_) % file_cnt_;

    if(hw.encoder.RisingEdge())
    {
        for(int i = 0; i < sequence_length; i++)
        {
            chance = rand() % 100;
            if(chance > 50)
            {
                sequence[i] = 2 * (rand() % 8);
            }
            else if(chance > 25)
            {
                sequence[i] = -2;
            }
            else
            {
                sequence[i] = -1;
            }
        }
    }

    hw.ProcessDigitalControls();
    bpm         = speedknob.Process();
    sampleSpeed = samplespeedcontrol.Process();


    // currBeat = fmod(beatTime, 1.0) + floor(beatTime / 3.0) * 3;

    // currBeat = 2.0+sin(beatTime)*2.0;
    // currSpeed = cos(beatTime);


    if(getCurrentNote(&currNote,
                      &timeSinceNote,
                      sequence,
                      sequence_length,
                      sequenceDivision,
                      beatTime))
    {
        currSpeed = bpm * sampleSpeed;
        currFrame = currNote / beatDivision + timeSinceNote * sampleSpeed;

        for(size_t i = 0; i < size; i++)
        {
            out[0][i] = wavFileReaders[sample].interpolate(
                currFrame + float(i) * (currSpeed / (48000.0 * 60.0)),
                0,
                dataSources[sample]);
            out[1][i] = wavFileReaders[sample].interpolate(
                currFrame + float(i) * (currSpeed / (48000.0 * 60.0)),
                1,
                dataSources[sample]);
        }
    }
    else
    {
        for(size_t i = 0; i < size; i++)
        {
            out[0][i] = 0;
            out[1][i] = 0;
        }
    }


    beatTime += bpm * (4.0 / (60.0 * 48000.0));
}

size_t pool_index       = 0;
int    allocation_count = 0;
#define CUSTOM_POOL_SIZE (64 * 1024 * 1024)

DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];


char basePath[100];

void *custom_pool_allocate(size_t size)
{
    if(pool_index + size >= CUSTOM_POOL_SIZE)
    {
        return 0;
    }
    void *ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}

void loadFiles(void)
{
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    if(f_mount(&fsi.GetSDFileSystem(), basePath, 1) != FR_OK)
    {
        hw.seed.PrintLine("couldnt get file system");

        for(;;) {}
    }
    file_cnt_      = 0;
    FRESULT result = FR_OK;
    FILINFO fno;
    DIR     dir;
    char   *fn;
    file_sel_ = 0;
    strcpy(basePath, "/reexport");

    if(f_opendir(&dir, basePath) == FR_OK)
    {
        hw.seed.PrintLine("opened directory : %s", basePath);
        hw.seed.PrintLine("loading files :");
        strcat(basePath, "/");

        hw.seed.PrintLine("baspoath is now : %s", basePath);

        sd_debug_msg = "sd: no files";
        do
        {
            result = f_readdir(&dir, &fno);
            // Exit if bad read or NULL fname
            if(result != FR_OK || fno.fname[0] == 0)
                break;
            // Skip if its a directory or a hidden file.
            if(fno.fattrib & (AM_HID | AM_DIR))
                continue;
            // Now we'll check if its .wav and add to the list.
            fn = fno.fname;
            if(file_cnt_ < kMaxFiles - 1)
            {
                if((strstr(fn, ".wav") || strstr(fn, ".WAV"))
                   && !strstr(fn, ".asd"))
                {
                    char newpath[100];
                    strcpy(newpath, basePath);
                    strcat(newpath, fn);
                    // strcpy(newpath,
                    //        strcat(basePath, fn));

                    hw.seed.PrintLine(newpath);

                    size_t bytesread;
                    if(f_open(&SDFile, newpath, (FA_OPEN_EXISTING | FA_READ))
                       == FR_OK)
                    {
                        file_sizes[file_cnt_] = f_size(&SDFile);
                        char  *memoryBuffer   = 0;
                        int    memorySize     = 0;
                        UINT   size           = file_sizes[file_cnt_];
                        size_t bytesread;
                        memoryBuffer = (char *)custom_pool_allocate(size);
                        if(memoryBuffer)
                        {
                            UINT bytesRead;
                            // Read the whole WAV file
                            if(f_read(&SDFile,
                                      (void *)memoryBuffer,
                                      size,
                                      &bytesread)
                               == FR_OK)
                            {
                                memorySize             = size;
                                dataSources[file_cnt_] = MemoryDataSource(
                                    memoryBuffer, memorySize);
                                hw.seed.PrintLine("sample size is %i",
                                                  memorySize);
                                char *err
                                    = wavFileReaders[file_cnt_].getWavInfo(
                                        dataSources[file_cnt_]);
                                if(strcmp(err, "z") != 0)
                                {
                                    hw.seed.PrintLine(
                                        "error happened whilst getting the wav "
                                        ": %s ",
                                        err);
                                }
                                else
                                {
                                    char    *token     = strtok(newpath, ".");
                                    long int num_beats = strtol(
                                        &token[strlen(token) - 2], NULL, 10);
                                    hw.seed.PrintLine(
                                        "num beats is %i\n",
                                        num_beats); //printing the token

                                    wavFileReaders[file_cnt_].setNumBeats(
                                        num_beats);

                                    num_frames = wavFileReaders[file_cnt_]
                                                     .getNumFrames();
                                    file_frames[file_cnt_] = num_frames;

                                    hw.seed.PrintLine("sample length is %i",
                                                      wavFileReaders[file_cnt_]
                                                          .getNumFrames());
                                    wavFileReaders[file_cnt_].resize();
                                    file_cnt_++;
                                }
                            }
                            else
                            {
                                hw.seed.PrintLine("failed to read file");
                            }
                        }
                        else
                        {
                            hw.seed.PrintLine("no memorybuffer");
                        }
                        f_close(&SDFile);
                        hw.seed.PrintLine("num files is %i", file_cnt_);
                    }
                    else if(f_open(
                                &SDFile, newpath, (FA_OPEN_EXISTING | FA_READ))
                            == FR_NO_FILE)
                    {
                        hw.seed.PrintLine("failed to read file, doesnt exist");
                    }
                }
            }
            else
            {
                break;
            }
        } while(result == FR_OK);
    }
    else
    {
        hw.seed.PrintLine("couldnt open dir");
    }
    f_closedir(&dir);
}


int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAdc();

    speedknob.Init(hw.knob1, 60, 300, Parameter::LINEAR);
    samplespeedcontrol.Init(hw.knob2, -2, 2, Parameter::LINEAR);

    hw.seed.StartLog(true);
    loadFiles();

    for(int i = 0; i < sequence_length; i++)
    {
        sequence[i] = baseSequence[i];

        // hw.seed.PrintLine("sequence note %i is %i", i, sequence[i]);
    }


    hw.StartAudio(AudioCallback);

    // while(1)
    // {
    //     hw.DelayMs(1000);
    // }
}
