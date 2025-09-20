#ifndef TUNER_H
#define TUNER_H

#include "arm_math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern TaskHandle_t xHandleAudioAcq;
extern TaskHandle_t xHandleFFTProc;

int get_noteOctave(int noteIndex);

const int get_closestNoteIndex(double frequency);
const double get_noteDiff(double frequency, int closestIndex);
void display_result(int noteDiff, double frequency, int noteIndex);
void timer_init(void);
void tuner_init();
void vTaskAudioAcquisition(void *pvParameters);
void vTaskFFTProcessing(void *pvParameters);
void audio_start(void);
void audio_stop(void);
#endif
