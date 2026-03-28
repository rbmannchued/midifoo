#include "notes.h"

const char *noteNames[] = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};

const float noteFrequencies[57] = {
  // --- A0 octave (added for bass guitar support) ---
  27.50,  // A0
  29.14,  // A#0
  30.87,  // B0
  32.70,  // C1
  34.65,  // C#1
  36.71,  // D1
  38.89,  // D#1
  41.20,  // E1
  43.65,  // F1
  46.25,  // F#1
  49.00,  // G1
  51.91,  // G#1
  55.00,  // A1
  58.27,  // A#1
  61.74,  // B1
  65.41,  // C2
  69.30,  // C#2
  73.42,  // D2
  77.78,  // D#2
  82.41,  // E2  ← guitar low E
  87.31,  // F2
  92.50,  // F#2
  98.00,  // G2
  103.83, // G#2
  110.00, // A2
  116.54, // A#2
  123.47, // B2
  130.81, // C3
  138.59, // C#3
  146.83, // D3
  155.56, // D#3
  164.81, // E3
  174.61, // F3
  185.00, // F#3
  196.00, // G3
  207.65, // G#3
  220.00, // A3
  233.08, // A#3
  246.94, // B3
  261.63, // C4
  277.18, // C#4
  293.66, // D4
  311.13, // D#4
  329.63, // E4
  349.23, // F4
  369.99, // F#4
  392.00, // G4
  415.30, // G#4
  440.00, // A4
  466.16, // A#4
  493.88, // B4
  523.25, // C5
  554.37, // C#5
  587.33, // D5
  622.25, // D#5
  659.26, // E5
  698.46, // F5
};

int get_noteOctave(int noteIndex) {
    return (noteIndex + 9) / 12;
}

const double get_noteDiff(double frequency, int closestIndex) {
    double noteGap;
    double frequencyGap;
    int n = sizeof(noteFrequencies) / sizeof(noteFrequencies[0]);

    if (frequency < noteFrequencies[closestIndex]) {
        if (closestIndex > 0)
            noteGap = noteFrequencies[closestIndex] - noteFrequencies[closestIndex - 1];
        else
            noteGap = noteFrequencies[1] - noteFrequencies[0];
    } else {
        if (closestIndex < n - 1)
            noteGap = noteFrequencies[closestIndex + 1] - noteFrequencies[closestIndex];
        else
            noteGap = noteFrequencies[n - 1] - noteFrequencies[n - 2];
    }

    frequencyGap = fabs(frequency - noteFrequencies[closestIndex]) / noteGap * 100;
    if (frequency < noteFrequencies[closestIndex]) {
        frequencyGap = -frequencyGap;
    }
    return frequencyGap;
}

const int get_closestNoteIndex(double frequency) {
    float minDiff = 1e9;
    int closestIndex = 0;
    int n = sizeof(noteFrequencies) / sizeof(noteFrequencies[0]);

    for (int i = 0; i < n; i++) {
        float diff = fabs(frequency - noteFrequencies[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }
    return closestIndex;
}
