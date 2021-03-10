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

#include <ncurses.h>
#include <curses.h>
#include "functions.h"

static const uint8_t line_loc[NUM_DISPLAY_LOCATIONS] = {3, 15, 18, 31};

void redraw_edit_screen(char*** edit_view, int16_t y_offset, uint16_t num_lines, uint16_t max_y, uint16_t max_x){
uint16_t j;
erase(); refresh();
uint16_t divider = line_loc[NUM_BARS]+6>2*max_x/5?line_loc[NUM_BARS]+6:2*max_x/5;
for (j=0; j<max_y; j++){
      mvprintw(j, divider, "|");
}
mvprintw(0, 0, "NUM BARS     TIME SIGNATURE    BPM");
mvprintw(0, divider +2, "           for navigation use arrow keys");
mvprintw(1, divider +2, "            to insert a line press 'i'");
mvprintw(2, divider +2, "            to delete a line press 'd'");
mvprintw(3, divider +2, "            to exit this mode press 'e'");
mvprintw(5, divider +2, "             to save to file press 's'");
mvprintw(6, divider +2, "             to write audio press 'w'");
mvprintw(9, divider +2, "             A / B produces A pulses");
mvprintw(11, divider +2, "      (A)/ B produces one pulse of A duration");
mvprintw(13, divider +2, " (A+B)/ C produces two pulses of A and B durations");
mvprintw(15, divider +2, " A+B / C produces A+B pulses with accents after A and B");
mvprintw(17, divider +2, "      a . in the BPM slot will change BPMs");
mvprintw(18, divider +2, "     so as to match the smallest note values");
mvprintw(19, divider +2, "        < or > indicate accel. and rit.");
 for (j=y_offset; j<=num_lines+y_offset; j++){
      mvprintw((j-y_offset)*2+2, line_loc[NUM_BARS], edit_view[j][NUM_BARS]);
      mvprintw((j-y_offset)*2+2, line_loc[NUMERATOR] - strlen(edit_view[j][NUMERATOR]), edit_view[j][NUMERATOR]);
      mvprintw((j-y_offset)*2+2, line_loc[DENOMINATOR], edit_view[j][DENOMINATOR]);
      mvprintw((j-y_offset)*2+2, line_loc[BPM_IN], edit_view[j][BPM_IN]);
 }
 for (j=0; j<=num_lines; j++){
     mvprintw(j*2+2, line_loc[NUMERATOR]+2, "/");
} refresh();
}

int main(int argc, const char * argv[]) {
    //NCURSES RELATED
    WINDOW* wnd = initscr();
    uint16_t max_x, max_y;
    getmaxyx(wnd, max_y, max_x);
    if (max_y < 20 || max_x < 120){
          erase(); mvprintw(max_y/3, max_x/4, "PLEASE GO TO TERMINAL SETTINGS");
          mvprintw(max_y/3 +1, max_x/4, "AND SELECT A WINDOW SIZE OF AT LEAST");
          mvprintw(max_y/3 +2, max_x/4, "20 LINES AND 120 COLUMNS");
          refresh(); sleep(3); mvprintw(max_y/3 +4, max_x/4, "PRESS ANY KEY TO CONTINUE"); refresh();getch();
   }
    uint16_t display_loc[2];
    display_loc[0] = max_y/3; display_loc[1] = max_x/3;
    if ((display_loc[0]+16)>=max_y)
        display_loc[0] = max_y -16;
    //LICENSE INFO
    mvprintw(max_y/2-3, max_x/4,  "<BMETRO>  Copyright (C) <2021>  <Richard Schwennicke>");
    mvprintw(max_y/2-2, max_x/4,"This program comes with ABSOLUTELY NO WARRANTY.");
    mvprintw(max_y/2-1, max_x/4,"This is free software, and you are welcome to re-");
    mvprintw(max_y/2, max_x/4,"distribute it under certain conditions; for details");
    mvprintw(max_y/2+1, max_x/4,"see the LICENSE.md in the current directory.");
    mvprintw(max_y/2+3, max_x/4, "press any key to continue");
    getch();
    DISPLAY_MODE mode = WELCOME;
    char single_char; int single_int;
    uint16_t edit_view_length = 128;
    char*** edit_view = malloc(sizeof(char**)*edit_view_length);
    uint16_t i, j;
    uint16_t  length = 0;
    for (i=0; i<edit_view_length; i++){
        edit_view[i]= malloc(sizeof(char*)*NUM_DISPLAY_LOCATIONS);
        for (j=0; j<NUM_DISPLAY_LOCATIONS; j++)
        edit_view[i][j] = (char*) calloc(sizeof(char)*LEN_OF_EDIT_VIEW_LINES, 1);
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
    BMETRO_INFO* info = init_metro_info(32);
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
                mvprintw(display_loc[0] + 8, display_loc[1], "QUIT");
                move(display_loc[0] +2, display_loc[1]-1);
                refresh();
                    num_options = 3;
                }
                else {
                    mvprintw(display_loc[0] + 2, display_loc[1], "NEW");
                    mvprintw(display_loc[0] + 4, display_loc[1], "LOAD SAVED");
                    mvprintw(display_loc[0] + 6, display_loc[1], "QUIT");
                    move(display_loc[0] +2, display_loc[1]-1);
                    refresh();
                    num_options = 2;
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
                    memset(edit_view[i][j], '\0', LEN_OF_EDIT_VIEW_LINES);
                }
                length = 0;
                //deliberate fall-through
            case EDITING:
                  mode = EDITING;
                noecho();
                uint16_t num_lines = (max_y - 4)/2;
                uint16_t current_location[2] = {0, 0};
                int16_t location_y_offset = 0;
                redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                move(2, line_loc[NUM_BARS]+strlen(edit_view[0][0]));
                refresh();
                i = strlen(edit_view[0][0]);
                bool location_changed = false;;
                while(1){
                    single_int = getch(); single_char = (char) single_int;
                    if (single_char == 'e'){
                              mode = WELCOME;
                        break;
                    }
                    if (single_char == 's'){
                          char* error_text;
                          if (convert_strs_to_BMETRO(edit_view, length, &info, &error_text)==-1)
                                mode = SAVING;
                          else{
                                //USER ALERT
                                current_location[0] = convert_strs_to_BMETRO(edit_view, length, &info, &error_text);
                                erase(); refresh();
                                mvprintw(display_loc[0], display_loc[1], "PARSING YOUR DATA HAS CREATED AN ERROR ON LINE %d", current_location[0]+1);
                                mvprintw(display_loc[0]+1, display_loc[1],"THE ERROR WAS: %s", error_text);
                                mvprintw(display_loc[0]+3, display_loc[1], "PRESS ANY KEY TO CONTINUE");
                                getch();
                          }
                          break;
                    }
                    if (single_char == 'w'){
                          char* error_text;
                        if (convert_strs_to_BMETRO(edit_view, length, &info, &error_text)==-1)
                              mode = PREFERENCES;
                        else {
                              current_location[0] = convert_strs_to_BMETRO(edit_view, length, &info, &error_text);
                              erase(); refresh();
                              mvprintw(display_loc[0], max_x/4, "PARSING YOUR DATA HAS CREATED AN ERROR ON LINE %d", current_location[0]+1);
                              mvprintw(display_loc[0]+1, max_x/4,"THE ERROR WAS: %s", error_text);
                              mvprintw(display_loc[0]+3, max_x/4, "PRESS ANY KEY TO CONTINUE");
                              getch();
                        }
                        break;
                    }
                    if (single_int == KEY_LEFT && current_location[1]>0){
                        location_changed = true;
                        current_location[1]--;
                    }
                    else if (single_int == KEY_LEFT){
                          if (current_location[0]>0){
                          single_int = KEY_UP;
                          current_location[1] = NUM_DISPLAY_LOCATIONS-1;
                    }
                    }
                    //if current y_loc is at top of screen (&& bounds checking)
                    if (single_int == KEY_UP && current_location[0]-location_y_offset==0 && current_location[0]>0){
                          location_y_offset--;
                          current_location[0]--;
                          location_changed = true;
                          redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                    }
                    //else if it isn't at top of screen (but still within bounds)
                    else if (single_int == KEY_UP && current_location[0]>0){
                        location_changed = true;
                        current_location[0]--;
                    }
                    else if (single_int == KEY_DOWN && current_location[0]-location_y_offset<num_lines && current_location[0]<(edit_view_length-1)){
                        location_changed = true;
                        current_location[0]++;
                    }
                    else if (single_int == KEY_DOWN && current_location[0]-location_y_offset>=num_lines && current_location[0]<(edit_view_length-1)){
                          if ((length - location_y_offset) < num_lines -2)
                              continue;
                              location_y_offset++;
                              current_location[0]++;
                              location_changed = true;
                              redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                    }
                    else if (single_int == KEY_RIGHT && current_location[1]<NUM_DISPLAY_LOCATIONS-1){
                        location_changed = true;
                        current_location[1]++;
                    }
                    else if (single_int == KEY_RIGHT){
                          location_changed = true;
                          current_location[1] = 0;
                          if (current_location[0]-location_y_offset<num_lines && current_location[0]<(edit_view_length-1))
                          current_location[0]++;
                          else {
                                if (length < num_lines -2)
                                    continue;
                                    location_y_offset++;
                                    current_location[0]++;
                                    location_changed = true;
                                    redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                          }
                    }
                    else if (single_char == 'i'){
                          if (length>edit_view_length-1)
                              continue;
                          uint16_t jj;
                          for (j=edit_view_length-1; j>current_location[0]; j--){
                                for (jj=0; jj<NUM_DISPLAY_LOCATIONS; jj++)
                                    memcpy(edit_view[j][jj], edit_view[j-1][jj], LEN_OF_EDIT_VIEW_LINES);
                              }
                              for (j=0; j<NUM_DISPLAY_LOCATIONS; j++)
                              memset(edit_view[current_location[0]][j], '\0', LEN_OF_EDIT_VIEW_LINES);
                          location_changed = true;
                          redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                    }
                    else if (single_char == 'd'){
                          uint16_t jj;
                          for (j=current_location[0]; j<edit_view_length-1; j++){
                                for(jj=0; jj<NUM_DISPLAY_LOCATIONS; jj++)
                                    memcpy(edit_view[j][jj], edit_view[j+1][jj], LEN_OF_EDIT_VIEW_LINES);
                          }
                          for (j=0; j<NUM_DISPLAY_LOCATIONS; j++)
                                memset(edit_view[edit_view_length-1][j], '\0', LEN_OF_EDIT_VIEW_LINES);
                          location_changed = true;
                          redraw_edit_screen(edit_view, location_y_offset, num_lines, max_y, max_x);
                    }
                    else if (single_int == 127 && i>0/*backspace*/){
                        edit_view[current_location[0]][current_location[1]][--i]= '\0';
                        if (current_location[1]!=NUMERATOR)
                         mvprintw((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]], "%s  ", edit_view[current_location[0]][current_location[1]] );
                        else
                            mvprintw((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]] - strlen(edit_view[current_location[0]][current_location[1]])-2, "  %s", edit_view[current_location[0]][current_location[1]]);
                        //check whether current_line is empty
                        bool empty = line_is_empty(edit_view, current_location[0]);
                        if (empty == true){
                                  for (j=length; j>0; j--){
                                        if(line_is_empty(edit_view, j-1))
                                        length --;
                                        else
                                        break;
                                  }
                            //move lines
                        }
                    }
                    else {
                        //surely we can use ASCII to number and specify a number range?
                        if ((i<3 || (i<9 && current_location[1]==NUMERATOR)|| (i<5 && current_location[1]==BPM_IN)) && (isdigit(single_char) || single_char == '.' || single_char == '+' || single_char == '(' || single_char == ')' || single_char == '>' || single_char == '<')){
                    edit_view[current_location[0]][current_location[1]][i++] = single_char;
                            if (current_location[1]==NUMERATOR){
                                mvprintw((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]]-strlen(edit_view[current_location[0]][current_location[1]]), "%s", edit_view[current_location[0]][current_location[1]]);
                            }
                        else
                        mvprintw((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]], "%s", edit_view[current_location[0]][current_location[1]]);
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
                        move ((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]]);
                    else
                        move((current_location[0]-location_y_offset)*2+2, line_loc[current_location[1]]+i);
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
                    wgetnstr(wnd, save_locs[num_saves], 16);{
                          uint16_t index; bool to_break = false;
                          for (index = 0; index < strlen(save_locs[num_saves]); index++)
                          if (save_locs[num_saves][index]=='/'){
                                    erase(); mvprintw(display_loc[0], display_loc[1], "ILLEGAL CHAR"); refresh(); sleep(1); to_break = true;
                                    break;
                              }
                        if (to_break)
                        break;
                    }
                    strcat(save_locs[num_saves],".bm");
                    noecho();
                    {
                          uint8_t index = 0; bool already_exists = false;
                          while (index<num_saves){
                                if (strcmp(save_locs[num_saves], save_locs[index++])==0)
                                already_exists = true;
                          }
                    if (length){
                        save_file = open_file(save_locs[num_saves], "w", argv[0]);
                        if (already_exists == false)
                              num_saves++;
                    }
                    else {
                        erase();
                        mvprintw(display_loc[0], display_loc[1], "SEEMS LIKE YOU HAVEN'T ENTERED ANY DATA YET");
                        refresh(); sleep(1);
                        mode = EDITING;
                        break;
                    }
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
                mvprintw(display_loc[0]+2+(2*num_saves), display_loc[1], "If your file doesn't show up, place it");
                mvprintw(display_loc[0]+3+(2*num_saves), display_loc[1], "in this folder and choose this option");
                mvprintw(display_loc[0]+5+(2*num_saves), display_loc[1], "to exit press e");
                move(display_loc[0]+2, display_loc[1]-1);
                refresh();
                uint8_t chosen = 0;
                while (1){
                    single_int = getch(); single_char = (char) single_int;
                    if (single_int == KEY_UP && chosen > 0)
                        chosen--;
                    else if (single_int == KEY_DOWN && chosen<=num_saves)
                        chosen++;
                  else if (single_char == 'e'){
                        mode = WELCOME;
                        break;
                  }
                    else if (single_char == '\n')
                        break;
                    move(display_loc[0] + 2+ 2*chosen, display_loc[1]-1);
                    refresh();
                }
                if (chosen==num_saves && single_char != 'e'){
                      fclose(flog);
                      system("ls *.bm >> .log.txt");
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
                     flog = fopen(log_loc, "w");
                     erase(); refresh();
                     mvprintw(display_loc[0]+2, display_loc[1], "UPDATE SAVE STATES");
                     mvprintw(display_loc[0]+3, display_loc[1], "LET'S TRY AGAIN");
                     move(display_loc[0]+2, display_loc[1]-1)
                     refresh(); sleep(1);
                      break;
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
            case PREFERENCES:
                {
                uint8_t option = 0;
                erase(); refresh();
                mvprintw(display_loc[0], display_loc[1], "SETTINGS");
                mvprintw(display_loc[0]+8, display_loc[1], "TO CYCLE THROUGH SETTINGS HIT ENTER");
                mvprintw(display_loc[0]+9, display_loc[1], "TO CONTINUE PRESS SPACE");
                refresh();
                while (1){
                    if (info->count_in)
                        mvprintw(display_loc[0]+4, display_loc[1], "COUNTIN ENABLED ");
                    else
                        mvprintw(display_loc[0]+4, display_loc[1], "COUNTIN DISABLED");
                    if (info->mark_downbeat)
                        mvprintw(display_loc[0]+2, display_loc[1], "MARK DOWNBEAT ENABLED ");
                    else
                        mvprintw(display_loc[0]+2, display_loc[1], "MARK DOWNBEAT DISABLED");
                  #ifdef brew
                    switch(info->outfile_type){
                          case WAV_FILE:
                          mvprintw(display_loc[0]+6, display_loc[1], "OUTFILE FORMAT: WAV");
                          break;
                          case MP3_FILE:
                          mvprintw(display_loc[0]+6, display_loc[1], "OUTFILE FORMAT: MP3");
                          break;
                          case AIFF_FILE:
                          mvprintw(display_loc[0]+6, display_loc[1], "OUTFILE FORMAT: AIF");
                          break;
                          default:
                          break;
                    }
                    #endif
                    move(display_loc[0]+2+option*2, display_loc[1]-1);
                    refresh();
                    single_int = getch(); single_char = (char) single_int;
                    if (single_char == '\n'){
                         switch(option){
                               case 0:
                               info->mark_downbeat = !info->mark_downbeat;
                               info->hi = info->mark_downbeat;
                               break;
                               case 1:
                               info->count_in = !info->count_in;
                               break;
                               case 2:
                               if (info->outfile_type < NUM_OUT_FILETYPES -1)
                               info->outfile_type++;
                               else
                               info->outfile_type = 0;
                               break;
                               default:
                               break;
                         }
                    }
                    else if (single_int == KEY_UP && option > 0)
                        option--;
                  #ifdef brew
                    else if (single_int == KEY_DOWN && option < 2)
                  #else
                    else if (single_int == KEY_DOWN && option < 1)
                  #endif
                        option++;
                else if (single_char == ' ')
                    break;
                }
                mode = OUTPUT;
                break;
                }
            case OUTPUT:
                erase(); refresh();
                mvprintw(display_loc[0], display_loc[1], "ENTER FILE NAME (without extension)");
                move (display_loc[0]+2, display_loc[1]); refresh(); echo();
                char filename[32], mp3filename[32];
                memset(filename, '\0', 32);
                wgetnstr(wnd, filename, 28);
                memcpy(mp3filename, filename, 32);
                noecho();
                { // prevent terminal error
                     uint16_t index; bool to_break = false;
                     for (index = 0; index < strlen(filename); index++)
                     if (filename[index]=='/'){
                                erase(); mvprintw(display_loc[0], display_loc[1], "ILLEGAL CHAR"); refresh(); sleep(1); to_break = true;
                                break;
                          }
                    if (to_break)
                    break;
                }
                strcat(filename, ".wav");
                switch(info->outfile_type){
                      case MP3_FILE:
                      strcat(mp3filename, ".mp3");
                      break;
                      case WAV_FILE:
                      strcat(mp3filename, ".wav");
                      break;
                      case AIFF_FILE:
                      strcat(mp3filename, ".aif");
                      break;
                      default:
                      break;
                }
                int16_t filename_length = strlen(filename);
                int32_t phs = 0;
                info->current_line = info->current_bar = info->current_beat = info->current_subdv = 0;
                WavFile* fout = open_wav_file(filename, "wb", argv[0]);
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
                char* wav_loc = (char*) malloc(sizeof(char)*(loc_length+filename_length));
                char* mp3_loc = (char*) malloc(sizeof(char)*(loc_length+filename_length));
                char* sox_command = (char*) malloc(sizeof(char)*(loc_length*2+2*filename_length+10));
                char* rm_command = (char*) malloc(sizeof(char)*(loc_length+filename_length+4));
                memset(wav_loc, '\0', loc_length+filename_length);
                memset(mp3_loc, '\0', loc_length+filename_length);
                memset(sox_command, '\0', loc_length*2+2*filename_length+10);
                memset(rm_command, '\0', loc_length+filename_length+4);
                memcpy(wav_loc, argv[0], loc_length);
                strcat(wav_loc, filename);
                memcpy(mp3_loc, argv[0], loc_length);
                strcat(mp3_loc, mp3filename);
                memcpy(sox_command, "sox -V0 ", 8);
                strcat(sox_command, wav_loc);
                strcat(sox_command, " ");
                strcat(sox_command, mp3_loc);
                memcpy(rm_command, "rm ", 3);
                strcat(rm_command, wav_loc);
                system(sox_command);
                if (info->outfile_type != WAV_FILE)
                system(rm_command);
                  free(wav_loc);
                  free(mp3_loc);
                  free(sox_command);
                  free(rm_command);
                  #endif
                  erase();
                if (info->count_in)
                  mvprintw(4, 10, "wrote audio with one bar count_in");
                else
                  mvprintw(4, 10, "wrote audio");
                mvprintw(7, 10, "press any key to continue");
                refresh();
                getch();
                mode = WELCOME;
                break;
            default:
                break;
        }
        if (mode == QUIT){
            for (i=0; i<num_saves; i++)
            fprintf(flog, "%s\n",save_locs[i]);
        break;
        }
    }
    fclose(flog);
    endwin();
     return 0;
}
