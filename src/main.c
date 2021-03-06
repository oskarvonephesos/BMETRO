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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <curses.h>
#include <unistd.h>
#include "../libwav/wav.h"
#define SR 44100
#define BLOCKSIZE 1024
#define TAB_LENGTH 2048
#define SILENCE 0.0f

#define LEN_OF_EDIT_VIEW 12
typedef struct {
    int16_t* bars;
    uint16_t* numerator;
    uint16_t* denominator;
    float* bpm;
    float** beat_length;
    uint16_t current_line;
    uint16_t current_beat;
    float* lo_click_tab;
    float* hi_click_tab;
    bool mark_downbeat;
    bool hi;
    bool count_in;
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
uint8_t line_loc[NUM_DISPLAY_LOCATIONS] = {3, 15, 18, 31};
//using x = a * exp(-k/T) and k = dur/nsteps
float* init_click(float freq){
    float* tab = (float*)malloc(sizeof(float)*TAB_LENGTH);
    uint16_t i;
    double wavel = SR/freq;
    double dur = 1.0, T = 0.5;
    double env = exp(-(dur/TAB_LENGTH)/T);
    double env_incr = 1.0;
    for (i=0; i<TAB_LENGTH; i++){
        tab[i]=0.5*env_incr*sin(M_PI*2.0*(double)i/wavel);
        env_incr *= env;
    }
    return tab;
}
BMETRO_INFO* init_metro_info(uint16_t num_bars){
    BMETRO_INFO* data = (BMETRO_INFO*) malloc(sizeof(BMETRO_INFO));
    data->bars        = (int16_t*)malloc(sizeof(int16_t)*num_bars);
    data->numerator   = (uint16_t*)malloc(sizeof(uint16_t)*num_bars);
    data->denominator = (uint16_t*)malloc(sizeof(uint16_t)*num_bars);
    data->bpm         = (float*)malloc(sizeof(float)*num_bars);
    data->beat_length = malloc(sizeof(float)*num_bars);
    int i;
    for (i=0; i<num_bars; i++)
        data->beat_length[i] = (float*)malloc(sizeof(float)*12);
    data->current_line  = 0;
    data->current_beat  = 0;
    data->mark_downbeat = false;
    data->lo_click_tab  = init_click(1000.0);
    data->hi_click_tab  = init_click(1750.0);
    data->count_in      = true;
    data->hi            = true;
    data->mark_downbeat = true;
    return data;
}
int16_t convert_strs_to_BMETRO(char*** input, uint16_t length, BMETRO_INFO* info){
    uint16_t i, j, k; bool is_regular, in_one;
    uint16_t numerators[12], num_numerators;
    char broken_down[32];
    memset(broken_down, '\0', 32);
    for (i=0; i<length; i++){
          for (j=0; j<NUM_DISPLAY_LOCATIONS; j++){
          if (strlen(input[i][j])==0)
            return i;
      }
          for (j=0; j<strlen(input[i][NUM_BARS]); j++){
                if (isdigit(input[i][NUM_BARS][j])==0)
                     return i;
            }
             for (j=0; j<strlen(input[i][NUMERATOR]); j++){
                if(isdigit(input[i][NUMERATOR][j])==0 && input[i][NUMERATOR][j]!='(' && input[i][NUMERATOR][j]!=')' && input[i][NUMERATOR][j]!='+')
                      return i;
           }
           for (j=0; j<strlen(input[i][DENOMINATOR]); j++){
                if(isdigit(input[i][DENOMINATOR][j])==0)
                      return i;
           }
           for (j=0; j<strlen(input[i][BPM_IN]); j++){
                if(isdigit(input[i][BPM_IN][j])==0 && input[i][BPM_IN][j]!='.')
                      return i;
          }
   }
    for (i=0; i<length; i++){
        info->bars[i]        = atoi(input[i][NUM_BARS]);
        info->denominator[i] = atoi(input[i][DENOMINATOR]);
        if (input[i][BPM_IN][0]== '.'){
            info->bpm[i] = info->bpm[i-1]*info->denominator[i]/info->denominator[i-1];
        }
        else
            info->bpm[i] = atof(input[i][BPM_IN]);
        if (input[i][NUMERATOR][0]== '('){
            info->numerator[i] = 1;
            in_one = true;
            is_regular = true;

        }
        else if (input[i][NUMERATOR][1]=='+'){
            is_regular = false;
            in_one = false;
            num_numerators =0;
            int ii = 0;
            for (ii=0; ii<LEN_OF_EDIT_VIEW; ii++){
            if (input[i][NUMERATOR][ii]==' ')
                input[i][NUMERATOR][ii]='\0';
            }
            ii=0;
            while(sscanf(&input[i][NUMERATOR][ii], "%hd", &numerators[num_numerators++])){
                while(input[i][NUMERATOR][ii++]!='+')
                    ;
            }
            info->numerator[i]=num_numerators-1;
        }
        else{
            info->numerator[i]   = atoi(input[i][NUMERATOR]);
            in_one = false;
            is_regular = true;
        }
        if (is_regular){
            if (!in_one){
            for (j=0; j<info->numerator[i]; j++)
            info->beat_length[i][j]=info->bpm[i];
            }
            else
                info->beat_length[i][0]=info->bpm[i]/atoi(&input[i][NUMERATOR][1]);
        }
        else {
            for (j=0; j<info->numerator[i]; j++)
            info->beat_length[i][j]=info->bpm[i]/numerators[j];
        }
    }
    info->bars[length] = -1;
    info->current_beat = 0;
    info->current_line = 0;
    return -1;
}
uint32_t bpm_to_samp(float bpm){
    float bps = bpm/60.0f;
    return (uint32_t) SR/bps;
}
int32_t write_sample_block(float* output, int32_t phs, BMETRO_INFO*info){
    uint16_t i;
    float* click_tab;
    if (info->hi)
        click_tab = info->hi_click_tab;
    else
        click_tab = info->lo_click_tab;
    uint32_t bpm_length = bpm_to_samp(info->beat_length[info->current_line][info->current_beat%info->numerator[info->current_line]]);
    for (i=0; i<BLOCKSIZE; i++){
        if (phs<TAB_LENGTH){
            output[i]=click_tab[phs];
        }
        else if (phs==TAB_LENGTH){
            output[i]= SILENCE;
            if (((info->current_beat+1)%info->numerator[info->current_line])==0)
                info->hi = true;
            else
                info->hi = false;
        }
        else {
            output[i]= SILENCE;
        }
        phs ++;
        if (phs>=bpm_length){
            phs = 0;
            info->current_beat++;
            bpm_length = bpm_to_samp(info->beat_length[info->current_line][info->current_beat%info->numerator[info->current_line]]);

            //end of section
            if (info->current_beat >= info->numerator[info->current_line] * info->bars[info->current_line]){
                info->current_line++;
                if (info->bars[info->current_line] == -1){
                    return -1;
                }
                info->current_beat = 0;
                bpm_length = bpm_to_samp(info->beat_length[info->current_line][0]);
            }
        }
    }
    return phs;
}
bool line_is_empty(char*** input, uint16_t line_number){
      uint8_t i;
      for (i=0; i<NUM_DISPLAY_LOCATIONS; i++){
            if (input[line_number][i][0]!='\0')
            return false;
      }
      return true;
}
///@deprecated
void write_out(float* sampleout, FILE* fp){
    int i;
    for (i=0; i<BLOCKSIZE; i++){
        fwrite(&(sampleout[i]), sizeof(float), 1, fp);
    }
}
FILE* open_file(char* name, const char* restrict_mode, const char* programm_loc){
    FILE* fp;
    long loc_length = strlen(programm_loc)-6;
    long name_length = strlen(name);
    char* file_path = (char*) malloc(sizeof(char*)*loc_length+name_length +1);
    memset(file_path, '\0', loc_length + name_length +1);
    memcpy(file_path, programm_loc, loc_length);
    strcat(file_path, name);
    fp = fopen(file_path, restrict_mode);
    return fp;
}
WavFile* open_wav_file(char* name, const char* restrict_mode, const char* programm_loc){
    WavFile* fp;
    long loc_length = strlen(programm_loc)-6;
    long name_length = strlen(name);
    char* file_path = (char*) malloc(sizeof(char*)*loc_length+name_length +1);
    memset(file_path, '\0', loc_length + name_length +1);
    memcpy(file_path, programm_loc, loc_length);
    strcat(file_path, name);
    fp = wav_open(file_path, restrict_mode);
    wav_set_format(fp, WAV_FORMAT_IEEE_FLOAT);
    wav_set_sample_size(fp, sizeof(float));
    wav_set_num_channels(fp, 1);
    wav_set_sample_rate(fp, SR);

    return fp;
}
int remove_file(char* name, const char* programm_loc){
    long loc_length = strlen(programm_loc)-6;
    long name_length = strlen(name);
    char* file_path = (char*) malloc(sizeof(char*)*loc_length+name_length +1);
    memset(file_path, '\0', loc_length + name_length +1);
    memcpy(file_path, programm_loc, loc_length);
    strcat(file_path, name);
    return remove(file_path);
}
int main(int argc, const char * argv[]) {
    //NCURSES RELATED
    WINDOW* wnd = initscr();
    uint16_t max_x, max_y;
    getmaxyx(wnd, max_y, max_x);
    uint16_t display_loc[2];
    display_loc[0] = max_y/3; display_loc[1] = max_x/3;
    if ((display_loc[0]+16)>=max_y)
        display_loc[0] = max_y -16;
    DISPLAY_MODE mode = WELCOME;
    char single_char; int single_int;
    char*** edit_view = malloc(sizeof(char**)*128);
    uint16_t i, j;
    uint16_t  length = 0;
    for (i=0; i<128; i++){
        edit_view[i]= malloc(sizeof(char*)*NUM_DISPLAY_LOCATIONS);
        for (j=0; j<NUM_DISPLAY_LOCATIONS; j++)
        edit_view[i][j] = (char*) calloc(sizeof(char)*LEN_OF_EDIT_VIEW, 1);
    }
    //FILE MANAGEMENT
    long loc_length = strlen(argv[0]) - 6;
    FILE *flog;
    char* log_loc = (char*)malloc(sizeof(char*)*(loc_length +16));
    memset(log_loc, '\0', loc_length + 16);
    memcpy(log_loc, argv[0], loc_length);
    strcat(log_loc, ".log.txt");
    char** save_locs = malloc(16*sizeof(char*));
    uint8_t num_saves = 0;
    for (i=0; i<16; i++){
        save_locs[i]=(char*)malloc(sizeof(char)*16);
        memset(save_locs[i], '\0', 16);
    }
    if (access(log_loc, F_OK) == 0){
        char scan_line[256];
        flog = fopen(log_loc, "r");
        i=0;
        while (fgets(scan_line, 256, flog)){
            sscanf(scan_line, "%s\n", save_locs[i]);
            i++;
        }
        num_saves = i;
        fclose(flog);
        remove_file(log_loc, argv[0]);
    }
    flog = fopen(log_loc, "w");
    //AUDIO-RELATED
    float* output = (float*)malloc(sizeof(float)*BLOCKSIZE);
    BMETRO_INFO* info = init_metro_info(16);
    while (1){
        switch (mode) {
            case WELCOME:
                noecho(); cbreak();
                keypad(wnd, TRUE);
                erase(); refresh();
                mvprintw(display_loc[0], display_loc[1], "HELLO! WELCOME TO BMETRO");
                //loop w/ navigation
                uint_fast8_t option = 0, num_options;
                if (length){
                mvprintw(display_loc[0] + 2, display_loc[1], "CONTINUE EDITING");
                mvprintw(display_loc[0] + 4, display_loc[1], "NEW");
                mvprintw(display_loc[0] + 6, display_loc[1], "LOAD SAVED");
                mvprintw(display_loc[0] + 8, display_loc[1], "OUTPUT");
                mvprintw(display_loc[0] + 10, display_loc[1], "HELP");
                mvprintw(display_loc[0] + 12, display_loc[1], "SETTINGS");
                mvprintw(display_loc[0] + 14, display_loc[1], "QUIT");
                move(display_loc[0] +2, display_loc[1]-1);
                refresh();
                    num_options = 6;
                }
                else {
                    mvprintw(display_loc[0] + 2, display_loc[1], "NEW");
                    mvprintw(display_loc[0] + 4, display_loc[1], "LOAD SAVED");
                    mvprintw(display_loc[0] + 6, display_loc[1], "HELP");
                    mvprintw(display_loc[0] + 8, display_loc[1], "SETTINGS");
                    mvprintw(display_loc[0] + 10, display_loc[1], "QUIT");
                    move(display_loc[0] +2, display_loc[1]-1);
                    refresh();
                    num_options = 4;
                }
                while (1){
                    single_int = getch(); single_char = (char) single_int;
                    if (single_int == KEY_DOWN){
                        if (option<num_options)
                            option++;
                    }
                    else if (single_int == KEY_UP){
                        if (option>0)
                            option--;
                    }
                    else if (single_char == '\n')
                        break;
                    move(display_loc[0] +2 + 2*option, display_loc[1]-1);
                    refresh();
                }
                if (length){
                switch (option){
                    case 0:
                        mode = EDITING;
                        break;
                    case 1:
                        mode = NEW;
                        break;
                    case 2:
                        mode = LOADING;
                        break;
                    case 3:
                        mode = OUTPUT;
                        break;
                    case 4:
                        mode = HELP;
                        break;
                    case 5:
                        mode = PREFERENCES;
                        break;
                    case 6:
                        mode = QUIT;
                        break;
                    default:
                        break;
                }
                }
                else {
                    switch (option){
                        case 0:
                            mode = NEW;
                            break;
                        case 1:
                            mode = LOADING;
                            break;
                        case 2:
                            mode = HELP;
                            break;
                        case 3:
                            mode = PREFERENCES;
                            break;
                        case 4:
                            mode = QUIT;
                            break;
                        default:
                            break;
                    }
                }
                break;
            case NEW:
                for (i=0; i<length; i++){
                    for (j=0; j<NUM_DISPLAY_LOCATIONS;j++)
                    memset(edit_view[i][j], '\0', LEN_OF_EDIT_VIEW);
                }
                length = 0;
                //deliberate fall-through
            case EDITING:
                noecho(); erase(); refresh();
                mvprintw(0, 0, "NUM BARS     TIME SIGNATURE    BPM");
                mvprintw(max_y-1, 0, "for navigation use arrow keys; to commit and leave this mode press 'e'; to save to file press 's'"); refresh();
                uint16_t num_lines = (max_y - 4)/2;
                for (i=0; i<=num_lines; i++){
                    mvprintw(i*2+2, line_loc[NUMERATOR]+2, "/");
                }
                for (i=0; i<length; i++){
                    mvprintw(i*2+2, line_loc[NUM_BARS], edit_view[i][NUM_BARS]);
                    mvprintw(i*2+2, line_loc[NUMERATOR] - strlen(edit_view[i][NUMERATOR]), edit_view[i][NUMERATOR]);
                    mvprintw(i*2+2, line_loc[DENOMINATOR], edit_view[i][DENOMINATOR]);
                    mvprintw(i*2+2, line_loc[BPM_IN], edit_view[i][BPM_IN]);
                }
                move(2, line_loc[NUM_BARS]+strlen(edit_view[0][0]));
                refresh();
                uint16_t current_location[2] = {0, 0};
                i = strlen(edit_view[0][0]);
                bool location_changed = false;;
                while(1){
                    single_int = getch(); single_char = (char) single_int;
                    if (single_char == 'e'){
                        if (convert_strs_to_BMETRO(edit_view, length, info)==-1)
                              mode = WELCOME;
                        else{
                              //USER ALERT
                              current_location[0] = convert_strs_to_BMETRO(edit_view, length, info);
                        }
                        break;
                    }
                    if (single_char == 's'){
                        mode = SAVING;
                        break;
                    }
                    else if (single_int == KEY_UP && current_location[0]>0){
                        location_changed = true;
                        current_location[0]--;
                    }
                    else if (single_int == KEY_DOWN && current_location[0]<num_lines){
                        location_changed = true;
                        current_location[0]++;
                    }
                    else if (single_int == KEY_RIGHT && current_location[1]<NUM_DISPLAY_LOCATIONS-1){
                        location_changed = true;
                        current_location[1]++;
                    }
                    else if (single_int == KEY_LEFT && current_location[1]>0){
                        location_changed = true;
                        current_location[1]--;
                    }
                    else if (single_int == 127 && i>0/*backspace*/){
                        edit_view[current_location[0]][current_location[1]][--i]= '\0';
                        if (current_location[1]!=NUMERATOR)
                         mvprintw(current_location[0]*2+2, line_loc[current_location[1]], "%s  ", edit_view[current_location[0]][current_location[1]] );
                        else
                            mvprintw(current_location[0]*2+2, line_loc[current_location[1]] - strlen(edit_view[current_location[0]][current_location[1]])-2, "  %s", edit_view[current_location[0]][current_location[1]]);
                        //check whether current_line is empty
                        bool empty = line_is_empty(edit_view, current_location[0]);
                        if (empty == true){
                                  for (j=length-1; j>0; j--){
                                        if(line_is_empty(edit_view, j))
                                        length --;
                                        else
                                        break;
                                  }
                            //move lines
                        }
                    }
                    else {
                        //surely we can use ASCII to number and specify a number range?
                        if ((i<3 || (i<9 && current_location[1]==NUMERATOR)|| (i<5 && current_location[1]==BPM_IN)) && (isdigit(single_char) || single_char == '.' || single_char == '+' || single_char == '(' || single_char == ')')){
                    edit_view[current_location[0]][current_location[1]][i++] = single_char;
                            if (current_location[1]==NUMERATOR){
                                mvprintw(current_location[0]*2+2, line_loc[current_location[1]]-strlen(edit_view[current_location[0]][current_location[1]]), "%s", edit_view[current_location[0]][current_location[1]]);
                            }
                        else
                        mvprintw(current_location[0]*2+2, line_loc[current_location[1]], "%s", edit_view[current_location[0]][current_location[1]]);
                        if (current_location[0]+1>length)
                            length = current_location[0]+1;
                        refresh();
                        }
                    }
                    if (location_changed){
                        i= strlen(edit_view[current_location[0]][current_location[1]]);
                        location_changed = false;
                    }
                    if (current_location[1]== NUMERATOR)
                        move (current_location[0]*2+2, line_loc[current_location[1]]);
                    else
                        move(current_location[0]*2+2, line_loc[current_location[1]]+i);
                    refresh();
                }
                break;
            case SAVING:
                //prompt user to enter name
                erase();
                mvprintw(display_loc[0], display_loc[1], "WOULD YOU LIKE TO OVERWRITE AN EXISTING FILE?");
                mvprintw(display_loc[0] +3, display_loc[1], "YES");
                mvprintw(display_loc[0] +3, display_loc[1]+10, "NO");
                bool overwrite = false;
                FILE* save_file;
                while (1){
                    single_int = getch(); single_char = (int) single_int;
                    if (single_char == '\n')
                        break;
                    else if (single_int == KEY_LEFT && !overwrite )
                            overwrite = true;
                    else if (single_int == KEY_RIGHT && overwrite )
                        overwrite = false;
                    move(display_loc[0]+3, display_loc[1]+(!overwrite)*10);
                    refresh();
                }
                if (overwrite){
                    erase();
                    //list all save names from save_locs
                    if (num_saves != 0){
                    for (i=0; i<num_saves; i++)
                        mvprintw(display_loc[0]+2*i, display_loc[1], "%s", save_locs[i]);
                        //give user option to choose
                        move(display_loc[0], display_loc[1]-1); refresh();
                        uint8_t chosen = 0;
                        while (1){
                            single_int = getch(); single_char = (char) single_int;
                            if (single_char == '\n')
                                break;
                            else if (single_int == KEY_UP && chosen> 0)
                                chosen--;
                            else if (single_int == KEY_DOWN && chosen<num_saves)
                                chosen ++;
                            move(display_loc[0]+chosen*2, display_loc[1]-1);
                            refresh();
                        }
                        if (remove_file(save_locs[chosen], argv[0])==0){
                        save_file = open_file(save_locs[chosen], "w", argv[0]);
                        }
                        else {
                            mvprintw(display_loc[0], display_loc[1], "COULD NOT DELETE FILE");
                            mvprintw(display_loc[0]+1, display_loc[1], "%s", save_locs[chosen]);
                            mvprintw(display_loc[0]+2, display_loc[1], "%ld", strlen(save_locs[chosen]));
                        refresh(); sleep(1);
                        mode = EDITING;
                        break;
                        }
                    }
                    else{
                        mvprintw(display_loc[0], display_loc[1], "SEEMS LIKE YOU HAVEN'T SAVED ANY FILES YET");
                    refresh(); sleep(1);
                    mode = EDITING;
                    break;
                    }
                }
                else {
                    erase();
                    mvprintw(display_loc[0], display_loc[1], "ENTER FILE NAME:");
                    move(display_loc[0]+3, display_loc[1]); refresh();
                    echo();
                    wgetnstr(wnd, save_locs[num_saves], 16);
                    strcat(save_locs[num_saves],".bm");
                    noecho();
                    if (length){
                        save_file = open_file(save_locs[num_saves++], "w", argv[0]);
                    }
                    else {
                        erase();
                        mvprintw(display_loc[0], display_loc[1], "SEEMS LIKE YOU HAVEN'T ENTERED ANY DATA YET");
                        refresh(); sleep(1);
                        mode = EDITING;
                        break;
                    }
                }
                //write to appropiate file
                    if (save_file){
                        for (i=0; i<length; i++){
                            for (j=0; j<NUM_DISPLAY_LOCATIONS; j++){
                                fprintf(save_file, "%s ", edit_view[i][j]);
                            }
                            fprintf(save_file, "\n");
                        }
                        fclose(save_file);
                    }
                    else {
                        erase(); mvprintw(display_loc[0], display_loc[1], "ERROR OPENING FILE");
                        refresh(); sleep(1);
                    }
                //save data
                mode = EDITING;
                break;
            case LOADING:
                //num_saves and save_locs tell us how many saves we have to care about...
                erase();
                mvprintw(display_loc[0], display_loc[1], "CHOOSE FILE TO LOAD");
                for (i=0; i<num_saves; i++){
                    mvprintw(display_loc[0]+2+2*i, display_loc[1], "%s", save_locs[i]);
                }
                move(display_loc[0]+2, display_loc[1]-1);
                refresh();
                uint8_t chosen = 0;
                while (1){
                    single_int = getch(); single_char = (char) single_int;
                    if (single_int == KEY_UP && chosen > 0)
                        chosen--;
                    else if (single_int == KEY_DOWN && chosen<num_saves)
                        chosen++;
                    else if (single_char == '\n')
                        break;
                    move(display_loc[0] + 2+ 2*chosen, display_loc[1]-1);
                    refresh();
                }
                FILE* load_file = open_file(save_locs[chosen], "r", argv[0]);
                if (load_file){
                char scan_line[256];
                    i=0; length =0;
                    while(fgets(scan_line, 256, load_file)){
                        sscanf(scan_line, "%s %s %s %s\n", edit_view[i][NUM_BARS], edit_view[i][NUMERATOR], edit_view[i][DENOMINATOR], edit_view[i][BPM_IN] );
                        i++; length++;
                    }
                }
                else{
                    //error message
                }
                mode = EDITING;
                break;
            case HELP:
                erase(); refresh();
                mvprintw(display_loc[0], max_x/4, "SYNTAX OF BAR SIGNATURES");
                mvprintw(display_loc[0]+2, max_x/4, "A BAR OF 2 / 4 WILL RESULT IN TWO PULSES ONE QUARTER NOTE APART");
                mvprintw(display_loc[0]+3, max_x/4, "\t(i.e. 2 / 4 IN TWO)");
                mvprintw(display_loc[0]+4, max_x/4, "A BAR OF (3) / 4 WILL RESULT IN ONE PULSE THAT TAKES THREE QUARTER NOTES");
                mvprintw(display_loc[0]+5, max_x/4, "\t(i.e. 3/ 4 IN ONE)");
                mvprintw(display_loc[0]+6, max_x/4, "A BAR OF 3 + 4 / 4 WILL RESULT IN TWO PULSES:");
                mvprintw(display_loc[0]+7, max_x/4, "\tTHE FIRST TAKING THREE QUARTER NOTES, THE SECOND FOUR");
                mvprintw(display_loc[0]+8, max_x/4, "\t(i.e. 7 / 4 IN TWO)");
                mvprintw(display_loc[0]+10, max_x/4, "'.' IN THE BPM SLOT TELLS BMETRO TO USE THE TEMPO FROM THE PRECEDING SECTION");
                mvprintw(display_loc[0]+11, max_x/4, "\tKEEPING THE SMALLEST NOTE VALUES CONSTANT");
                mvprintw(display_loc[0]+14, max_x/4, "PRESS ANY KEY TO RETURN TO THE MENU");
                refresh();
                getch();
                mode = WELCOME;
                    break;
            case PREFERENCES:
                erase(); refresh();
                mvprintw(display_loc[0], display_loc[1], "SETTINGS");
                mvprintw(display_loc[0]+6, display_loc[1], "TO CYCLE THROUGH SETTINGS HIT ENTER");
                mvprintw(display_loc[0]+7, display_loc[1], "TO EXIT BACK TO MENU HIT Q");
                refresh();
                while (1){
                    if (info->mark_downbeat)
                        mvprintw(display_loc[0]+2, display_loc[1], "MARK DOWNBEAT ENABLED ");
                    else
                        mvprintw(display_loc[0]+2, display_loc[1], "MARK DOWNBEAT DISABLED");
                        refresh();
                    single_int = getch(); single_char = (char) single_int;
                    if (single_char == '\n'){
                    info->mark_downbeat = !info->mark_downbeat;
                        info->hi = info->mark_downbeat;
                    }
                else if (single_char == 'q')
                    break;
                }
                mode = WELCOME;
                break;
            case OUTPUT:
                erase(); refresh();
                mvprintw(0, 10, "length: %d", length);
                mvprintw(2, 10, "bpms: %f, %f", info->beat_length[0][0], info->beat_length[0][1]);
                mvprintw(4, 10, "numerators: %d", info->numerator[0]);
                mvprintw(6, 10, "bpm 3: %f, bpm 4: %f", info->beat_length[info->current_line][2%info->numerator[info->current_line]], info->beat_length[info->current_line][3%info->numerator[info->current_line]]);
                refresh();
                int32_t phs = 0;
                WavFile* fout = open_wav_file("audio_out.wav", "wb", argv[0]);
                while (phs>=0){
                    phs = write_sample_block(output, phs, info);
                    if (wav_write(fout, output, BLOCKSIZE)!= BLOCKSIZE){
                        mvprintw(10, 10, "ERROR");
                        refresh();
                    }
                    //write_out(output, fout);
                }
                wav_close(fout);
                #ifdef brew
                char* wav_loc = (char*) malloc(sizeof(char)*(loc_length+13));
                char* mp3_loc = (char*) malloc(sizeof(char)*(loc_length+13));
                char* sox_command = (char*) malloc(sizeof(char)*(loc_length*2+2*13+6));
                char* rm_command = (char*) malloc(sizeof(char)*(loc_length+13+4));
                memset(wav_loc, '\0', loc_length+13);
                memset(mp3_loc, '\0', loc_length+13);
                memset(sox_command, '\0', loc_length*2+2*13+6);
                memset(rm_command, '\0', loc_length+13+4);
                memcpy(wav_loc, argv[0], loc_length);
                strcat(wav_loc, "audio_out.wav");
                memcpy(mp3_loc, argv[0], loc_length);
                strcat(mp3_loc, "audio_out.mp3");
                memcpy(sox_command, "sox ", 4);
                strcat(sox_command, wav_loc);
                strcat(sox_command, " ");
                strcat(sox_command, mp3_loc);
                memcpy(rm_command, "rm ", 3);
                strcat(rm_command, wav_loc);
                system(sox_command);
                system(rm_command);
                  free(wav_loc);
                  free(mp3_loc);
                  free(sox_command);
                  free(rm_command);
                  #endif
                  erase();
                mvprintw(4, 10, "wrote audio"); refresh();
                getch();
                mode = WELCOME;
                break;
            default:
                break;
        }
        if (mode == QUIT){
            for (i=0; i<num_saves; i++)
            fprintf(flog, "%s\n",save_locs[i]);
            //fprintf(flog, "%d", num_saves);
        break;
        }
    }
    fclose(flog);
    endwin();
    /*
    int32_t phs = 0;
    while (phs>=0){
        phs = write_sample_block(output, click_tab, phs, info);
        write_out(output, fout);
    }*/
    return 0;
}
