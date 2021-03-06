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
///@deprecated
void write_out(float* sampleout, FILE* fp){
    int i;
    for (i=0; i<BLOCKSIZE; i++){
        fwrite(&(sampleout[i]), sizeof(float), 1, fp);
    }
}
