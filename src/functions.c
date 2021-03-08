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
#include "functions.h"
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
    data->numerator   = malloc(sizeof(int16_t*)*num_bars);
    data->denominator = (uint16_t*)malloc(sizeof(uint16_t)*num_bars);
    data->bpm         = (float*)malloc(sizeof(float)*num_bars);
    data->bpm_incr    = (float*)calloc(sizeof(float)*num_bars, 1);
    data->in_one      = (bool*)malloc(sizeof(bool)*num_bars);
    data->is_regular  = (bool*)malloc(sizeof(bool)*num_bars);
    int i;
    for (i=0; i<num_bars; i++)
        data->numerator[i] = (int16_t*)malloc(sizeof(int16_t)*16);
    data->current_line  = 0;
    data->current_beat  = 0;
    data->current_bar   = 0;
    data->current_subdv = 0;
    data->mark_downbeat = false;
    data->lo_click_tab  = init_click(1000.0);
    data->hi_click_tab  = init_click(1750.0);
    data->count_in      = true;
    data->hi            = true;
    data->mark_downbeat = true;
    data->length        = num_bars;
    return data;
}
char error_strings[9][64]={
      "EMPTY STRING",
      "LEADING ZERO / SONDERZEICHEN",
      "EXPECTED DIGIT",
      "FINAL + / (",
      "SUCCESSION OF SONDERZEICHEN",
      "FIRST BPM IS . < >",
      "MISC ERROR",
      "UNTERMINATED ACCEL./RIT.",
      "CHANGING DENOMINATOR DURING ACCEL./RIT CURRENTLY SUPPORTED"
};
int16_t convert_strs_to_BMETRO(char*** input, uint16_t length, BMETRO_INFO** info, char** error_text){
    uint16_t i, j, k; bool is_regular, in_one;
    uint16_t numerators[12], num_numerators;
    char broken_down[32];
    if (length+1 > (*info)->length){
          free(*info);
      *info = init_metro_info(length*2);
}
    memset(broken_down, '\0', 32);
    for (i=0; i<length; i++){
          //prevent empty strings
          for (j=0; j<NUM_DISPLAY_LOCATIONS; j++){
          if (strlen(input[i][j])==0){
                *error_text = error_strings[0];
                  return i;
          }
      }
      //prevent leading zero
      for (j=0; j<NUM_DISPLAY_LOCATIONS; j++){
      if (input[i][j][0]=='0' ){
           *error_text = error_strings[1];
             return i;
      }
      }
      //prevent leading SONDERZEICHEN
      for (j=0; j<NUM_DISPLAY_LOCATIONS; j++){
      if (input[i][j][0] == '+' || input[i][j][0] == ')' ){
           *error_text = error_strings[1];
             return i;
      }
      }
      //( and + have to be followed by a non-zero digit
      for (j=0; j<strlen(input[i][NUMERATOR]); j++){
            if (input[i][NUMERATOR][j] == '(' || input[i][NUMERATOR][j] == '+'){
                  if (isdigit(input[i][NUMERATOR][j+1])==0){
                        *error_text = error_strings[2];
                        return i;
                  }
                  else if (input[i][NUMERATOR][j+1]=='0'){
                        *error_text = error_strings[1];
                        return i;
                  }
            }
      }
      //check for appropriate chracters
          for (j=0; j<strlen(input[i][NUM_BARS]); j++){
                if (isdigit(input[i][NUM_BARS][j])==0){
                     *error_text = error_strings[2];
                       return i;
               }
            }
             for (j=0; j<strlen(input[i][NUMERATOR]); j++){
                if(isdigit(input[i][NUMERATOR][j])==0 && input[i][NUMERATOR][j]!='(' && input[i][NUMERATOR][j]!=')' && input[i][NUMERATOR][j]!='+'){
                     *error_text = error_strings[6];
                       return i;
               }
           }
           //two SONDERZEICHEN may not succeed each other
           for (j=0; j<strlen(input[i][NUMERATOR]); j++){
                 if(input[i][NUMERATOR][j]=='(' || input[i][NUMERATOR][j]==')' || input[i][NUMERATOR][j]=='+')
                 if (input[i][NUMERATOR][j+1] == '(' || input[i][NUMERATOR][j+1] == ')' || input[i][NUMERATOR][j+1] == '+'){
                       *error_text = error_strings[4];
                         return i;
                 }
          }
          //no final +
          for (j=0; j<strlen(input[i][NUMERATOR]); j++){
                if(input[i][NUMERATOR][j]=='+' || input[i][NUMERATOR][j]=='(')
                if (input[i][NUMERATOR][j+1] == '\0'){
                     *error_text = error_strings[3];
                       return i;
               }
         }
         //appropiate chars again
           for (j=0; j<strlen(input[i][DENOMINATOR]); j++){
                if(isdigit(input[i][DENOMINATOR][j])==0){
                     *error_text = error_strings[2];
                       return i;
               }
           }
           //prevent first BPM from referencing previous lines
           if (input[0][BPM_IN][0] == '.' || input[0][BPM_IN][0] == '<' || input[0][BPM_IN][0] == '>'){
                *error_text = error_strings[5];
                  return i;
          }
          // prevent denominator changes during accel/rit.
          if (input[i][BPM_IN][0] == '<' || input[i][BPM_IN][0] == '>'){
                      if (strcmp(input[i][DENOMINATOR], input[i-1][DENOMINATOR]) != 0){
                           *error_text = error_strings[8];
                           return i;
                     }
          }
          //prevent unterminated accel./rit.
          if (input[length-1][BPM_IN][0]== '>' || input[length-1][BPM_IN][0]=='<'){
                      *error_text = error_strings[7];
                      return i;
          }
           //appropriate chars
           for (j=0; j<strlen(input[i][BPM_IN]); j++){
                if(isdigit(input[i][BPM_IN][j])==0 && input[i][BPM_IN][j]!='.' && input[i][BPM_IN][j]!='>' && input[i][BPM_IN][j]!='<'){
                     *error_text = error_strings[2];
                       return i;
               }
          }
   }
    for (i=0; i<length; i++){
        (*info)->bars[i]        = atoi(input[i][NUM_BARS]);
        (*info)->denominator[i] = atoi(input[i][DENOMINATOR]);
        if (input[i][NUMERATOR][0]== '('){
             in_one = true;
             if (input[i][NUMERATOR][2]=='+'||input[i][NUMERATOR][3]=='+'||input[i][NUMERATOR][4]=='+'){
                   num_numerators = 0;
                   uint32_t ii = 1; //this has to be a very large data type, so that sscanf fails
                   while(sscanf(&input[i][NUMERATOR][ii], "%hd", &numerators[num_numerators++])){
                        while(input[i][NUMERATOR][ii++]!='+')
                        ;
                  }
                  ii = 0;
                  uint32_t jj =0;
                  while (jj<strlen(input[i][NUMERATOR])){
                        if (input[i][NUMERATOR][jj++]=='+')
                         ii++;
                  }
                  num_numerators = ii+1;
                  for (ii=0; ii<num_numerators; ii++)
                  (*info)->numerator[i][ii]=numerators[ii];
                  (*info)->numerator[i][num_numerators]=-1;
             }
             else {
            (*info)->numerator[i][0] = atoi(&input[i][NUMERATOR][1]);
            (*info)->numerator[i][1] = -1;
            is_regular = true;
      }
        }
        else if (input[i][NUMERATOR][1]=='+'||input[i][NUMERATOR][2]=='+'||input[i][NUMERATOR][3]=='+'){
            is_regular = false;
            in_one = false;
            num_numerators = 0;
            uint32_t ii = 0; //this has to be a very large data type, so that sscanf fails
            while(sscanf(&input[i][NUMERATOR][ii], "%hd", &numerators[num_numerators++])){
                while(input[i][NUMERATOR][ii++]!='+')
                    ;
            }
            ii = 0;
            uint32_t jj =0;
            while (jj<strlen(input[i][NUMERATOR])){
                  if (input[i][NUMERATOR][jj++]=='+')
                   ii++;
            }
            num_numerators = ii+1;
            for (ii=0; ii<num_numerators; ii++)
                  (*info)->numerator[i][ii]=numerators[ii];
            (*info)->numerator[i][num_numerators]=-1;
        }
        else{
            (*info)->numerator[i][0]   = atoi(input[i][NUMERATOR]);
            (*info)->numerator[i][1]   = -1;
            in_one = false;
            is_regular = true;
        }
        (*info)->in_one[i] = in_one;
        (*info)->is_regular[i]= is_regular;
    }
    //bpm parsing is done last, because it may depend on denominator and numerator parsing
    for (i=0; i<length;){
    if (input[i][BPM_IN][0]== '.'){
        (*info)->bpm[i] = (*info)->bpm[i-1]*(*info)->denominator[i]/(*info)->denominator[i-1];
        (*info)->bpm_incr[i] = 0;
        i++;
    }
    else if (input[i][BPM_IN][0]== '>'){
         int16_t jj = i, accel_len = 0, bar_length;
         while (input[jj][BPM_IN][0]== '>'){
               bar_length = get_bar_length(*info, jj);
               accel_len += bar_length* (*info)->bars[jj++];
      }
        float bpm_difference = atof(input[jj][BPM_IN]) - atof(input[i-1][BPM_IN]);
        bpm_difference /= accel_len; int16_t ii;
        for (ii=i; ii<jj; ii++){
        (*info)->bpm_incr[ii] = bpm_difference;
        (*info)->bpm[ii] = (*info)->bpm[ii-1];
        }
        i = ii;
   }
    else{
        (*info)->bpm[i] = atof(input[i][BPM_IN]);
        (*info)->bpm_incr[i] = 0;
        i++;
 }
 }
    (*info)->bars[length] = -1;
    (*info)->current_beat = 0;
    (*info)->current_line = 0;
    if ((*info)->count_in)
    (*info)->bars[0] +=1;
    return -1;
}
int16_t get_bar_length(BMETRO_INFO* info, uint16_t line){
      int16_t length=0, i=0;
      while (info->numerator[line][i]!=-1){
            length += info->numerator[line][i++];
      }
      return length;
}
uint32_t bpm_to_samp(float bpm){
    float bps = bpm/60.0f;
    return (uint32_t) SR/bps;
}
int32_t write_sample_block(float* output, int32_t phs, BMETRO_INFO*info){
    uint16_t i;
    float* click_tab;
    if (info->hi && info->mark_downbeat){
          click_tab = info->hi_click_tab;
   }
   else {
         click_tab = info->lo_click_tab;
   }
    static float volume = 1.0f;
    uint32_t bpm_length = bpm_to_samp(info->bpm[info->current_line]);
    for (i=0; i<BLOCKSIZE; i++){
        if (phs<TAB_LENGTH){
            output[i]=volume*click_tab[phs];
        }
        else if (phs==TAB_LENGTH){
            output[i]= SILENCE;
            if (((info->current_beat+1)%info->numerator[info->current_line][info->current_subdv])==0 && info->numerator[info->current_line][info->current_subdv+1]==-1){
                info->hi = true;
                volume = 1.0f;
          }
            else if (((info->current_beat+1)%info->numerator[info->current_line][info->current_subdv])==0 ){
              volume = 1.0f;
              info->hi = false;
            }
            else{
                if (!info->is_regular[info->current_line]&& ! info->in_one[info->current_line])
                  volume = 0.4f;
                else if (info->in_one[info->current_line]){
                      volume = 0.0f;
                }
                 else{
                 volume = 1.0f;
           }
                info->hi = false;
               }
        }
        else {
            output[i]= SILENCE;
        }
        phs ++;
        if (phs>=bpm_length){
            phs = 0;
            info->current_beat++;
            info->bpm[info->current_line]+=info->bpm_incr[info->current_line];
            if (info->current_beat >= info->numerator[info->current_line][info->current_subdv]){
                  info->current_beat = 0;
                  info->current_subdv++;
                  if (info->numerator[info->current_line][info->current_subdv] ==-1){
                        info->current_subdv = 0;
                        info->current_bar++;
                  }
            }
            bpm_length = bpm_to_samp(info->bpm[info->current_line]);
            //end of section
            if (info->current_bar >= info->bars[info->current_line]){
                info->current_line++;
                if (info->bars[info->current_line] == -1){
                    return -1;
                }
                if (info->bpm_incr[info->current_line] != 0.0f)
                info->bpm[info->current_line] = info->bpm[info->current_line-1];
                info->current_beat  = 0;
                info->current_subdv = 0;
                info->current_bar   = 0;
                bpm_length = bpm_to_samp(info->bpm[info->current_line]);
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
