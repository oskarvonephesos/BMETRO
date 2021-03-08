/**
      BMETRO
      a click track generator
    Copyright (C) <2021> <Richard Schwennicke>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    **/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "../libwav/wav.h"

#define SR 44100
#define BLOCKSIZE 1024
#define TAB_LENGTH 2048
#define SILENCE 0.0f
#define LEN_OF_EDIT_VIEW_LINES 12
typedef struct {
    int16_t* bars;
    int16_t** numerator;
    uint16_t* denominator;
    float* bpm;
    uint16_t current_line;
    uint16_t current_subdv;
    uint16_t current_beat;
    uint16_t current_bar;
    bool* in_one;
    bool* is_regular;
    float* lo_click_tab;
    float* hi_click_tab;
    bool mark_downbeat;
    bool hi;
    bool count_in;
    uint16_t length;
} BMETRO_INFO;
typedef enum {
    WELCOME,
    EDITING,
    NEW,
    LOADING,
    SAVING,
    PREFERENCES,
    HELP,
    OUTPUT,
    QUIT = -1
} DISPLAY_MODE;
typedef enum {
    NUM_BARS,
    NUMERATOR,
    DENOMINATOR,
    BPM_IN,
    NUM_DISPLAY_LOCATIONS
} DISPLAY_LOC;

float* init_click(float freq);
BMETRO_INFO* init_metro_info(uint16_t num_bars);
int16_t convert_strs_to_BMETRO(char*** input, uint16_t length, BMETRO_INFO** info, char** error_text);
uint32_t bpm_to_samp(float bpm);
int32_t write_sample_block(float* output, int32_t phs, BMETRO_INFO*info);
bool line_is_empty(char*** input, uint16_t line_number);
FILE* open_file(char* name, const char* restrict_mode, const char* programm_loc);
WavFile* open_wav_file(char* name, const char* restrict_mode, const char* programm_loc);
int remove_file(char* name, const char* programm_loc);
