#ifndef NOTES_H
#define NOTES_H

extern const char *noteNames[];
extern const float noteFrequencies[45];
const int get_closestNoteIndex(double frequency);
int get_noteOctave(int noteIndex);
const double get_noteDiff(double frequency, int closestIndex);

#endif
