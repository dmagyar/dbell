
// Audio

#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>


bool newAudioSource = false;
char audioSource[256] = "";
uint8_t msgPriority = 0;
float onceGain = 0;
float defaultGain = .3;

AudioFileSourceHTTPStream *stream = nullptr;
AudioFileSourceSPIFFS *file = nullptr;
AudioFileSourceBuffer *buff = nullptr;
AudioGenerator *mp3 = nullptr;
AudioOutputI2S *out = nullptr;

const int preallocateBufferSize = 5*1024;
const int preallocateCodecSize = 29192; // MP3 codec max mem needed

void *preallocateBuffer = NULL;
void *preallocateCodec = NULL;

bool stopPlaying() {
    bool stopped = false;
    if (mp3) {
        mp3->stop();
        delete mp3;
        mp3 = nullptr;
        stopped = true;
    }
    if (file) {
        file->close();
        delete file;
        file = nullptr;
    }
    if (buff) {
        buff->close();
        delete buff;
        buff = nullptr;
    }
    if (stream) {
        stream->close();
        delete stream;
        stream = nullptr;
    }
    return stopped;
}

void playAudio() {
    newAudioSource = false;

    if (audioSource[0] == 0) {
        return;
    }

    stopPlaying();

    Serial.printf_P(PSTR("\nFree heap: %d\n"), ESP.getFreeHeap());

    if (!out) {
        out = new AudioOutputI2S();
        out->SetOutputModeMono(true);
    }
    out->SetGain(onceGain ?: defaultGain);
    onceGain = 0;

    if (strncmp("http", audioSource, 4) == 0) {
        // Get MP3 from stream
        Serial.printf_P(PSTR("**MP3FASZ stream: %s\n"), audioSource);

        //stream = new AudioFileSourceICYStream(audioSource);
        stream = new AudioFileSourceHTTPStream(audioSource);
        if (stream && stream->isOpen()) {
          Serial.printf_P(PSTR("**STREAM OPEN: %s\n"), audioSource);
        } else {
          Serial.printf_P(PSTR("**STREAM CLOSED: %p\n"), stream);  
        }

        buff = new AudioFileSourceBuffer(stream, preallocateBuffer, preallocateBufferSize); 
        if (buff && buff->isOpen()) {
          Serial.printf_P(PSTR("**BUFF OPEN: %s\n"), audioSource);
        } else {
          Serial.printf_P(PSTR("**BUFF CLOSED: %p\n"), buff);  
        }

        mp3 = (AudioGenerator*) new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
        Serial.printf_P("Decoder start...\n");
        mp3->begin(buff, out);
        if (!mp3->isRunning()) {
            Serial.printf_P(PSTR("Can't connect to URL"));
            stopPlaying();
        }     
  } else if (strncmp("/mp3/", audioSource, 5) == 0) {
        // Get MP3 from SPIFFS
        Serial.printf_P(PSTR("**MP3 file: %s\n"), audioSource);
        file = new AudioFileSourceSPIFFS(audioSource);
        if (file && file->isOpen()) {
          Serial.printf_P(PSTR("**OPENED: %s\n"), audioSource);
        }
        mp3 = (AudioGenerator*) new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
        mp3->begin(file, out);
        if (!mp3->isRunning()) {
            Serial.println(F("Unable to play MP3"));
            stopPlaying();
        }
    }
}

