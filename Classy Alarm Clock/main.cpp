//
//  main.cpp
//  Classy Alarm Clock
//
//  Created by Farouk Mishak on 5/16/19.
//  Copyright © 2019 Farouk Mishak. All rights reserved.
//

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include<unistd.h>
#include <time.h>
#include<iomanip>
#include <stdlib.h> // getenv
#ifdef WIN32
// Visual Studio will automatically "define" WIN32.
// This will trigger the code to include conio.h instead of curses.h
// TODO: Make sure WIN32 is defined
#include <conio.h>
#else
#include <ncurses.h> // for unix-related systems (likely not MS Windows... for Windows, try <conio.h>)
#endif
// Get a copy of the Cereal library with:
// git clone https://github.com/USCiLab/cereal.git cereal
// Include Cereal's support for C++ string and vector
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
// Cereal supports archives in several formats.
// Tried JSON since it is compact and human-readable, but it was making compiler warnings:
//#include <cereal/archives/json.hpp>
// Using Cereal's "binary" format instead:
#include <cereal/archives/binary.hpp>

using namespace std;

class tm_from_time : public tm { // struct tm is from time.h
public:
    tm_from_time(const time_t &the_time) {
        localtime_r(&the_time, this); // man localtime_r
    }
};


void start_ncurses() {
    // Finish setting up our screen since we know that TERM is set:
    initscr(); // tell ncurses to initialize itself.
    clear(); // clear the screen. see "man clear"
    noecho(); // If a user presses a key, don't automatically let the terminal show it. see "man noecho"
    cbreak(); // see "man cbreak"
    timeout(100); // getch() should wait 100 ms for a key to be pressed.  See "man timeout"
}

void stop_ncurses() {
    // turn off ncurses to let cin work properly
    clear();
    refresh(); // send the echo and clear commands to the terminal
    endwin();  // reset ncurses and return terminal to normal
}

/**
 UserProfile class is for saving the user info.
 **/
class UserProfile
{
private:
    string userName;
    int color;
public:
    UserProfile(string, int);
    UserProfile(){
        userName = "";
        color = 7;
    }
    void setUserName(string);
    void setColor(int);
    string getUserName()const;
    int getColor() const;
};
UserProfile::UserProfile(string un, int c)
{
    userName = un;
    color = c;
}
void UserProfile::setUserName(string un)
{
    userName = un;
}
void UserProfile::setColor(int c)
{
    color = c;
}
string UserProfile::getUserName() const
{
    return userName;
}
int UserProfile::getColor() const
{
    return color;
}

class ALARM { //: public UserProfile{
private:
    unsigned hour, minute;
    string name;
    
public:
    ALARM()
    {
        hour = 00;
        minute = 00;
        name = "";
    }
    ALARM (unsigned, unsigned, string);
    void setHour(unsigned);
    void setMinute(unsigned);
    void setName(string);
    unsigned getHour() const;
    unsigned getMinute() const;
    string getName() const;
    
    // Cereal library needs this to know what variables we want to save and load:
    template<class Archive> // For details, see http://uscilab.github.io/cereal/
    void serialize(Archive & archive) {
        archive(hour, minute, name); // list of our class's variables
    }
};

ALARM::ALARM (unsigned h, unsigned m, string n)
{
    hour = h;
    minute = m;
    name = n;
}
void ALARM::setHour(unsigned h)
{
    hour = h;
}
void ALARM::setMinute(unsigned m)
{
    minute = m;
}
void ALARM::setName(string n)
{
    name = n;
}
unsigned ALARM::getHour() const
{
    return hour;
}
unsigned ALARM::getMinute() const
{
    return minute;
}
string ALARM::getName() const
{
    return name;
}

int wrap_24_hours(int seconds_since_midnight) {
    // This function is used by secnonds_between().
    // Does C/C++ % operator handle negative numbers correctly?
    // until we verify, let's manually add a day at a time until the number is positive:
    while(seconds_since_midnight < 0)
        seconds_since_midnight += 24 * 60 * 60;
    // Now divide and keep the remainder so that we naturally wrap every 24 hours:
    seconds_since_midnight %= 24 * 60 * 60;
    return seconds_since_midnight;
}

int seconds_between(unsigned alarm, unsigned start_time, unsigned end_time) {
    // This function returns true if alarm is between start_time and end_time.
    // start_time is the last time this program checked time().
    // end_time is right now.
    // Normally start_time and end_time are veryr close together, since main() checks every 100ms.  But it can be slowed down if the user presses A to add or D to delete an alarm and then takes a while to finish.
    
    // wrap_24_hours() helps handles edge case when user has an alarm set for 12:00 midnight, but at 11:59pm, they press "A" to add an alarm and don't finish adding it until 12:01am.  start_time would be 11:59pm and end_time would be 12:01am.  end-start would be a large negative number.  (actually 120 - 86400)
    int duration = wrap_24_hours(end_time - start_time);
    int x = wrap_24_hours(alarm - start_time);
    if (x>=0 && x<=duration)
        return 1;
    else
        return 0;
}


class AlarmList {
private:
    vector<ALARM> alarms;
public:
    void save(string filename) {
        // This function will erase the file and write a new one.
        // Let Cereal write our vector of ALARM structs to the file:
        ofstream out(filename, ios::binary); // create the file
        cereal::BinaryOutputArchive archive(out); // set up Cereal
        archive(alarms); // tell Cereal to write our vector to the file
    }
    
    void load(string &filename) {
        ifstream in(filename, ios::binary); // try to open the file
        if (!in.is_open()) // the file doesn't exist
            return; // we could not open the file.
        // Let Cereal read in the file for us:
        cereal::BinaryInputArchive archive(in); // set up Cereal
        archive(alarms); // tell Cereal to read the file into our vector
    }
    
    void show(char *term_type, int start_y, int start_x) {
        clear();
        // bounceBubbles();
        //drawFrame();
        if (term_type) // If we're running in a normal terminal:
            mvprintw(start_y++, start_x, "#  HH:MM  Name %i", (int)alarms.size()); // ncurses
        else // If we're running in Xcode, we can't do fancy stuff:
            cout <<"#  HH:MM  Name\n"; // plain cout
        
        for (int n = 0; n<alarms.size(); n++) {
            const ALARM &alarm = alarms[n];
            if (term_type) // fancy ncurses: (uses old C style like printf)
                mvprintw(start_y++, start_x, "%i  % 2u:%02u  %s\n", n, alarm.getHour(), alarm.getMinute(), alarm.getName().c_str());
            else // plain cout for Xcode:
                cout << n << "  " << setw(2) << alarm.getHour() << ':' << setfill('0') << setw(2) << alarm.getMinute() << setfill(' ') << "  " << alarm.getName() << "\n";
        }
    }
    
    void add(char *term_type) {
        if(term_type) { // turn off ncurses for cin to work
            stop_ncurses();
        }
        // TODO:
        // Ask user for hour, minute, and description:
        // TODO:  put some cout's in here to be friendly
        int h, m;
        string n;
        cout << "\nPlease enter an alarm (hour, minute, name)\n";
        cout << "Hour: ";
        cin >> h;
        cout << "Minute: " ;
        cin >> m;
        cout << "Name: ";
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // need this for getline to work.
        // See https://stackoverflow.com/questions/25475384/when-and-why-do-i-need-to-use-cin-ignore-in-c
        // And http://www.cplusplus.com/reference/istream/istream/ignore/
        getline(cin, n);
        ALARM alarm(h, m, n);
        cout << "OK: " << alarm.getHour() << ":" << alarm.getMinute() << " " << alarm.getName() << "\n";
        
        cout << "alarms.size = " << alarms.size() << "\n";
        alarms.push_back(alarm);
        cout << "alarms.size = " << alarms.size() << "\n";
        if(term_type) { // turn on ncurses again
            start_ncurses();
        }
    }
    
    void del(char *term_type) {
        if(term_type) { // turn off ncurses for cin to work
            stop_ncurses();
        }
        cout << "Which alarm # do you want to delete?\n";
        show(0, 3, 10); // using 0 instead of term_type, because we turned off ncurses for cin to work.
        unsigned number;
        cin >> number;
        if (number>=alarms.size()) {
            cout << "Invalid alarm # " << number << endl;
            sleep(3); // Let user see our message for 3 seconds before going on.
        }
        if(term_type) { // turn on ncurses again
            start_ncurses();
        }
        // Tell C++ vector to erase the alarm:
        alarms.erase(alarms.begin() + number);
    }
    
    string get_new_ringing_alarm(time_t old_time, time_t current_time, int *alarm_is_ringing) {
        // Search through vector of ALARM's for any that are active.
        // old_time is the last time we were called and current_time is now.
        
        // old_time and current_time measure seconds since Jan 1, 1970.
        // Since our alarms repeat every day, we convert these times to be
        // seconds since midnight every day.
        
        // localtime() converts seconds since 1970 into a struct tm with
        // separate variables for hours, minutes, seconds, and the date.
        // (But we don't need the date here.)
        tm_from_time old_date_time(old_time);
        tm_from_time current_date_time(current_time);
        // Now convert hours, minutes, seconds to seconds since midnight:
        unsigned old_seconds_since_midnight =
        old_date_time.tm_hour * 3600 +
        old_date_time.tm_min * 60 +
        old_date_time.tm_sec; // ignoring tm_year, tm_mon, and tm_mday
        unsigned current_seconds_since_midnight =
        current_date_time.tm_hour * 3600 +
        current_date_time.tm_min * 60 +
        current_date_time.tm_sec; // ignoring tm_year, tm_mon, and tm_mday
        
        // This for loop searches vector of alarms for any alarm set between old and current:
        string name_of_ringing_alarm;
        for (int n = 0; n<alarms.size(); n++) {
            ALARM alarm = alarms[n];
            
            // Convert this alarm to seconds since midnight:
            unsigned alarm_seconds_since_midnight = alarm.getHour() * 3600 + alarm.getMinute() * 60;
            
            // seconds_between() tests if alarm_seconds_since_midnight is between old_seconds_since_midnight and current_seconds_since_midnight
            if (seconds_between(alarm_seconds_since_midnight, old_seconds_since_midnight, current_seconds_since_midnight)) {
                // This is an active alarm!
                *alarm_is_ringing = 1; // set a flag for main()
                name_of_ringing_alarm = alarm.getName(); // save the name for main()
            }
        }
        return name_of_ringing_alarm;
    }
    //get_new_ringing_alarm will set alarm_is_ringing=1 if a new alarm is ringing.
    
};


void show_help(int start_y, int start_x) {
    clear();
    mvprintw(start_y++, start_x, "Key  Action");
    mvprintw(start_y++, start_x, " A   Add alarm");
    mvprintw(start_y++, start_x, " D   Delete alarm");
    mvprintw(start_y++, start_x, " L   List alarms");
    mvprintw(start_y++, start_x, " ?   Show this help screen");
    mvprintw(start_y++, start_x, "SPACE  Silence an active alarm");
    refresh(); // actually send the text to the screen!
}

int main() {
    time_t old_time;
    time(&old_time); // when we first start, skip any alarm that has already passed.
    // we'll only care about alarms that happen while we're running.
    
    char *term_type = getenv("TERM"); // This will return 0 if TERM isn't defined in the "environment"
    
    if(term_type){
        start_ncurses();
        
    }
    
    string filename = "alarms.cereal";
    AlarmList alarms;
    alarms.load(filename);
    //alarms.show(term_type, 5, 10);
    //return 0;
    
    int alarm_is_ringing = 0;
    string name_of_ringing_alarm;
    
    int done = 0;
    while (!done){
        time_t current_time;
        time(&current_time); // get time in seconds since Jan 1, 1970, midnight.  See "man 3 time"
        tm_from_time current_date_time(current_time); // man localtime
        // get a copy of the hours, minutes, and seconds from the struct to make our code easier below:
        int hour = current_date_time.tm_hour;
        int minute = current_date_time.tm_min;
        int second = current_date_time.tm_sec;
        
        {
            // Check if any new alarm is going off
            time(&current_time);
            int new_alarm_is_ringing = 0;
            string name_of_new_ringing_alarm = alarms.get_new_ringing_alarm(old_time, current_time, &new_alarm_is_ringing);
            if (new_alarm_is_ringing) {
                // Replace any alarm already ringing with the new one we just found:
                alarm_is_ringing = new_alarm_is_ringing;
                name_of_ringing_alarm = name_of_new_ringing_alarm;
                old_time = current_time;
            }
            //get_new_ringing_alarm will set alarm_is_ringing=1 if a new alarm is ringing.
        }
        
        if(term_type) {
            if (alarm_is_ringing) {
                cerr << "\x07";
                // TODO make flashing color;
                mvprintw(2, 10, "%2i:%02i:%02i", hour, minute, second); // man mvprintw
                mvprintw(3, 10, "ALARM!  name=\"%s\"", name_of_ringing_alarm.c_str()); // man mvprintw
                clrtoeol();
            } else {
                mvprintw(2, 10, "%2i:%02i:%02i", hour, minute, second); // man mvprintw
                clrtoeol();
            }
            refresh(); // actually send the text to the screen!
        } else
            cout << setfill(' ') << setw(2) << hour << ":" << setfill('0') << setw(2) << minute << ":" << setw(2) << second << setfill(' ') << endl;
        
        int key = getch(); // ncurses function to get a character from cin.  see "man getch"
        // getch() will wait for timeout (above) before returning.
        if(!term_type) {
            // getch() will return immediately when $TERM isn't set (such as when running accidentally in Xcode) so we haven't initialized ncurses with initscr().
            // We added this usleep to slow down our code, so that we don't run up the CPU and drain the laptop battery.
            usleep(100000);   //make the program sleep for 100,000 mircoseconds (0.1 seconds)
        }
        
        switch (key) {
            case 'a':
            case 'A':
                alarms.add(term_type);
                alarms.save(filename);
                break;
            case 'd':
            case 'D':
                alarms.del(term_type);
                alarms.save(filename);
                break;
            case 'l':
            case 'L':
                alarms.show(term_type, 5, 10);
                break;
            case ' ':
                // silence alarm:
                alarm_is_ringing = 0;
                break;
            case 'h':
            case 'H':
            case '?':
                show_help(5, 10);
                break;
            case 'q':
            case 'Q':
                done = 1; // we'll exit when the user presses q
                break;
            case ERR:
                // Nothing was pressed. So do nothing.
                break;
            default:
                // Unknown key...?   Do nothing.
                clear();
                mvprintw(3, 10, "Press ? for help.");
                clrtoeol();
                refresh();
                break;
        }
        
        
    }
    if(term_type)
        stop_ncurses();
    
    return 0;
}
