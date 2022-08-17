
/***************************************************************************
 *   Copyright (C) 2012 by Jonathan Duddington                             *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see:                                 *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

#pragma region TODO
 // TODO My list of 'Fixes' and 'Enhancements' to Auroramon v1.07 code base
 // http://auroramonitor.sourceforge.net/
 // https://sourceforge.net/projects/auroramonitor/files/auroramon-1.07/
 // 
 // Planned changes (29 May 2022)
 // - Error Handling
 // - capture in log and handle so program continues rather than crashing out
 // 
 // - Improve Inverter status message form
 //   - fix so it can be hidden if needed (press F4 seems to hide it - need to check with live data)
 //   - 17/08 designed behavior, form shows if inverter has an alarm, eg Input Low on either (or both) channels...
 // 
 // Create Windows Installer Package (update to existing installer... CMake??)
 //
 // FUTURE items
 // - Message Log / List
 // - show raw and processed messages
 // - set default data path to %LOCALAPPDATA% eg C:\Users\{user}\Appdata\Local
 //       variable => data_dir
 //
#pragma endregion

#pragma region v1.08
 // List of changes contained in v1.08
 // 17/08/2022
 // forced position of dlg_alarm (eg Inverter Status) form to be top left of Main form (including when opened due to alarm state active)
 // revised trigger for display inverter status, removed channel alerts so only triggered by inverter alarms
 // 
 // 09/08/2022
 // fixed bug in QueueCommand() where inv parameter was missing from LogMessage resulting in an 'ArgType' error when Inverter 'Retrieve Daily Energy' and 'Retrieve 10sec Energy' requests were made
 // expanded logging to capture details for Communicat::Transmission_state_error message
 // 
 // 03/08/2022
 // Added new sytle 'All (v)' to allow inverter data in status panel to be 'stacked' vertically
 // Added PlaceReadings instuction to show extra readings fields for second inverter
 //
 // 02/08/2022
 //  - positioned each of the settings sub forms insert top left of Main form
 // 
 // 31/07/2022
 //  - positioned dlgCoords form bottom left of Main form
 //  - positioned dlg_alarm (eg Inverter Status) form top left of Main form
 // 29/07/2022
 // - correct error with LogFileOpen
 // 
 // 24/07/2022
 // - Expand size of Coords form to show all (5 max) variables
 // 
 // 19/7/2022
 // - Improve Inverter status message form
 // - expanded inverter status panel field size (height) when there are two inverters configured 

#pragma endregion


#include <wx/aboutdlg.h>
#include <wx/generic/aboutdlgg.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include <wx/html/htmlwin.h>
#include <wx/datetime.h>

#include "wx/socket.h"
#include "wx/url.h"
#include "wx/wfstream.h"

#include "auroramon.h"

#pragma region Declarations

DECLARE_EVENT_TYPE(wxEVT_MY_EVENT, -1)
DEFINE_EVENT_TYPE(wxEVT_MY_EVENT)
wxCommandEvent my_event(wxEVT_MY_EVENT);

BEGIN_EVENT_TABLE(Mainframe, wxFrame)
    EVT_CLOSE(Mainframe::OnClose)
    EVT_MENU(-1, Mainframe::OnCommand)
	EVT_KEY_DOWN(Mainframe::OnKey)
    // EVT_MOVE(Mainframe::OnMove)
    EVT_MOVE_END(Mainframe::OnMove)
	EVT_TIMER(idStartupTimer, Mainframe::OnStartupTimer)
    EVT_TIMER(idPulseTimer, Mainframe::OnPulseTimer)
    EVT_TIMER(idLogStatusTimer, Mainframe::OnLogStatusTimer)
    EVT_COMMAND(idInverterData, wxEVT_MY_EVENT, Mainframe::OnInverterEvent)
    EVT_COMMAND(idInverterFail, wxEVT_MY_EVENT, Mainframe::OnInverterEvent)
    EVT_COMMAND(idInverterModelError, wxEVT_MY_EVENT, Mainframe::OnInverterEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(GraphPanel,wxScrolledWindow)
	EVT_LEFT_DOWN(GraphPanel::OnMouse)
    EVT_RIGHT_DOWN(GraphPanel::OnMouse)
    EVT_LEFT_DCLICK(GraphPanel::OnMouse)

	EVT_KEY_DOWN(GraphPanel::OnKey)
END_EVENT_TABLE()

void SendCommand(int inv, int type);
void InitComms();

extern void MonitorOutput(int power);
extern int SavePvoutput(int inv);
extern void SunInfo();
extern void InitCharts();
extern void InitCharts2();
extern void InitDates();

GraphPanel *graph_panel;
wxPanel *status_panel;
DlgSetup *dlg_setup;
DlgLocation *dlg_location;
DlgChart *dlg_chart;
DlgInverter *dlg_inverter;
DlgHistogram *dlg_histogram;
DlgExtraR *dlg_extrareadings;
DlgRetrieveEnergy *dlg_retrieve_energy;
DlgPvoutput *dlg_pvoutput;
DlgSetTime *dlg_settime;
DlgCoords *dlg_coords;

Mainframe *mainframe;
int display_today = 0;
wxString graph_date;
wxString graph_date_ymd;
char date_ymd[12] = "";   // this is today, unless the next day has arrived and we haven't yet noticed. Set in OpenLogFiles()
wxString date_ymd_string = wxEmptyString;
wxString date_logchange = wxEmptyString;
char ymd_energy[12];
char ymd_alarm[12];

// TODO review to support more than 2 inverters
INVERTER inverters[N_INV];
int inverter_address[N_INV] = {2, 0};

int option_date_format;
//char current_timestring[N_INV][10];

#pragma endregion

#pragma region PowerMeter-type
PowerMeter::PowerMeter(wxWindow *parent)
    : wxFrame(parent, -1, _T("Power Meter"), wxPoint(0, 100), wxSize(330,280+DLG_HX), wxCAPTION | wxSTAY_ON_TOP)
{

}

PowerMeter::~PowerMeter()
{
}

PowerMeter *power_meter;
#pragma endregion

#pragma region MiscFunctions

int GetGmtOffset(time_t timeval)
{//=============================
    wxDateTime datetime;
    wxDateTime gmt;
    wxDateTime::Tm tm, tm2;
    wxTimeSpan timespan;

    if(timeval == 0)
        datetime = wxDateTime::Now();
    else
        datetime.Set(timeval);

#ifdef __WXMSW__
    gmt = datetime.ToTimezone(wxDateTime::UTC);  // ?? On Linux, this seems to give GMT1 not GMT0
#else
    tm = datetime.GetTm();
    tm2 = datetime.GetTm(wxDateTime::UTC);
    gmt.Set(tm2);
#endif

    timespan = datetime.Subtract(gmt);
    return(timespan.GetSeconds().ToLong());
}

int GetFileLength(const char *filename)
{//====================================
	struct stat statbuf;

	if(stat(filename,&statbuf) != 0)
		return(0);

	if((statbuf.st_mode & S_IFMT) == S_IFDIR)
		return(-2);  // a directory

	return(statbuf.st_size);
}  // end of GetFileLength

void LogMessage(wxString message, int control)
{//===========================================
// control:  bit 0 = save message to log file
    FILE *f;
    char buf[256];
    wxDateTime now = wxDateTime::Now();
    wxString string;

    string = now.FormatISOTime() + _T("  ") + message;

    if(control & 1)
    {
        strncpy0(buf, wxString(data_dir+_T("/system/message_log.txt")).mb_str(wxConvLocal), sizeof(buf));
        if((f = fopen(buf, "a")) != NULL)
        {
            strncpy0(buf, string.mb_str(wxConvLocal), sizeof(buf));
            fprintf(f, "%s\n", buf);
            fclose(f);
        }
    }
    wxLogStatus(string);
    mainframe->logstatus_timer.Start(300*1000, wxTIMER_ONE_SHOT);   // 5 minute timer to remove message from the status bar
}

void DisplayTextfile(wxString fname, wxString caption)
{//===================================================
    static wxDialog *textframe = NULL;
    static wxTextCtrl *textctrl;
    int w, h;

    if(textframe == NULL)
    {
        graph_panel->GetClientSize(&w, &h);
        textframe = new wxDialog(mainframe, -1, wxEmptyString, wxPoint(0,0), wxSize(600,h));
        textframe->GetClientSize(&w, &h);
        textctrl = new wxTextCtrl(textframe, -1, wxEmptyString, wxPoint(0,0), wxSize(w,h), wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|wxVSCROLL|wxALWAYS_SHOW_SB);
    }
    textframe->SetTitle(caption);
    textctrl->Clear();
    if(wxFileExists(fname))
    {
        textctrl->LoadFile(fname);
    }
    textframe->Show(true);
}

#pragma endregion

#define MAX_PVOUTPUT_BATCH  30
void ResendPvoutput(int timeval)
{//=============================
    FILE *f_in;
    FILE *f_out = NULL;
    wxString fname;
    wxString fname2;
    wxString url_string;
    wxString data_string;
    wxURLError url_err;
    wxDateTime date;
    wxDateTime today;
    int count;
    int flag_log;
    int age;
    static int prev_time = 0;
    char buf[80];

    if(prev_time == 0)
    {
        // ignore the first call after program start
        prev_time = timeval;
    }

    if(timeval == prev_time)
    {
        // only run this function every minute
        return;
    }
    prev_time = timeval;

    fname = data_dir + _T("/system/pvoutput_fail.txt");
    if((f_in = fopen(fname.mb_str(wxConvLocal), "r")) == NULL)
        return;

    url_string.Printf(_T("%saddbatchstatus.jsp?key=%s&sid=%s&data="), pvoutput.url.c_str(), pvoutput.key.c_str(), pvoutput.sid.c_str());

    today = wxDateTime::Now();

    count = 0;
    while((count < MAX_PVOUTPUT_BATCH) && (fgets(buf, sizeof(buf), f_in) != NULL))
    {
        if(buf[0] != '2')
            continue;


        data_string = wxString(buf, wxConvLocal);
        MakeDate(date, data_string.Left(8));
        age = today.Subtract(date).GetDays();
        if((age < 0) || (age > 13))
            continue;   // discard old data

        if(count > 0)
            url_string += _T(";");
        url_string += data_string.Trim();
        count++;
    }

    wxURL url(url_string);
    if((url_err = url.GetError()) != wxURL_NOERR)
    {
        flag_log = 0;
        if(pvoutput.failed == 0)
            flag_log = 1;
        LogMessage(_T("Bad URL: ")+ url_string, flag_log);
        pvoutput.failed = 1;
        fclose(f_in);
        return;
    }

    wxInputStream *input = url.GetInputStream();  // connect to the URL
    url_err = url.GetError();
    if(input == NULL)
    {
        flag_log = 0;
        if(pvoutput.failed == 0)
            flag_log = 1;
//        LogMessage(wxString::Format(_T("PVOutput bulk update failed: %d"), url_err), flag_log);
        fclose(f_in);
        pvoutput.failed = 1;
        return;
    }
    LogMessage(_T("PVOutput.org bulk update OK"), 1);
    pvoutput.failed = 0;
    delete input;

    // are there any lines remaining ?
    // Copy them to another file.
    while((fgets(buf, sizeof(buf), f_in)) != NULL)
    {
        if(buf[0] != '2')
            continue;

        if(f_out == NULL)
        {
            fname2 = fname + _T("2");
            if((f_out = fopen(fname2.mb_str(wxConvLocal), "w")) == NULL)
            {
                break;
            }
        }
        fprintf(f_out, "%s", buf);
    }
    fclose(f_in);
    wxRemoveFile(fname);
    if(f_out != NULL)
    {
        fclose(f_out);
        wxRenameFile(fname2, fname);
    }
}

#pragma region LogFiles

static int prev_period_5min = -1;

void CheckEnergyLog()
{//==================
    // On program start, find the last 5 minute period in the energy log
    FILE* f;
    wxString fname;
    int hours, minutes;
    int value;
    char buf[80];

    fname.Printf(_T("%s/e5min_%s.txt"), data_year_dir2.c_str(), date_ymd_string.c_str());
    if ((f = fopen(fname.mb_str(wxConvLocal), "r")) == NULL)
        return;

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (sscanf(buf, "%d:%d, %d", &hours, &minutes, &value) == 3)
        {
            prev_period_5min = (hours * 60 + minutes) / 5;
        }
    }
    fclose(f);
}

void LogEnergy(int seconds, const char *ymd)
{//=========================================
    wxString fname;
    wxString url_string;
    wxString err_string;
    wxURLError url_err;
    double total_e;
    int total_p;
    FILE *f;
    int inv;
    int ix;
    int age;
    int period_5min;
    int got_power = 0;
    int pvo_period;
    int hours, mins;
    int flag_log;
    float grid_voltage = -1;

    period_5min = seconds/300;
    if(period_5min == prev_period_5min)
        return;

    prev_period_5min = period_5min;

    total_e = 0;
    total_p = 0;
    for(inv=0; inv < N_INV; inv++)
    {
        if((inverter_address[inv] != 0) && (strcmp(ymd, inverters[0].today_done) == 0))
        {
            total_e += inverters[inv].energy_total[0];

            if(inverters[inv].alive > 0)
            {
                age = seconds - inverters[inv].last_power_out_time;
                if((age >= 0) && (age < 60))
                {
                    total_p += inverters[inv].last_power_out;
                    got_power = 1;
                }
            }
        }
    }

    if(total_e == 0)
        return;

    if(got_power == 0)
        return;

    hours = seconds/3600;
    mins =  (seconds/60) % 60;

    fname.Printf(_T("%s/e5min_%s.txt"), data_year_dir2.c_str(), date_ymd_string.c_str());
    if((f = fopen(fname.mb_str(wxConvLocal), "a")) == NULL)
        return;

    // do we have grid voltage readings ?
    for(ix=0; ix < N_EXTRA_READINGS; ix++)
    {
        if(extra_readings[ix].dsp_code == DSP_GRID_VOLTS)
        {
            if((extra_readings[ix].status_slot != 0) || (extra_readings[ix].graph_slot != 0))
            {
                // we are getting grid voltage reading
                if(inverters[0].alive)
                    grid_voltage = inverter_response[0].dsp[DSP_GRID_VOLTS];
                else
                if(inverters[1].alive)
                    grid_voltage = inverter_response[1].dsp[DSP_GRID_VOLTS];
                break;
            }
        }
    }

    fprintf(f, "%2d:%.2d,%6d,%5d,,,%5.2f",  hours, mins, (int)(total_e*1000), total_p, inverters[0].temperature);
    if(grid_voltage > 0)
    {
        fprintf(f, ",%5.1f", grid_voltage);
    }
    fputc('\n', f);

    fclose(f);

    pvo_period = pvoutput.live_updates & PVO_PERIOD;

    if(pvo_period > 0)
    {

        if((pvo_period == 1) || ((period_5min % pvo_period) == 0))
        {
            // Start of 5 minute, 10 minute, or 15 minute period
            url_string.Printf(_T("%saddstatus.jsp?key=%s&sid=%s&d=%s&t=%d:%.2d&v1=%d"), pvoutput.url.c_str(), pvoutput.key.c_str(), pvoutput.sid.c_str(),
                       date_ymd_string.c_str(), hours, mins, (int)(total_e*1000));

            if(pvoutput.live_updates & PVO_INSTANTANEOUS)
            {
                url_string += wxString::Format(_T("&v2=%d"), total_p);
            }

            if(pvoutput.live_updates & PVO_TEMPERATURE)
            {
                url_string += wxString::Format(_T("&v5=%.2f"), inverters[0].temperature);
            }

            if((pvoutput.live_updates & PVO_GRID_VOLTAGE) && (grid_voltage > 0))
            {
                url_string += wxString::Format(_T("&v6=%.1f"), grid_voltage);
            }

            wxURL url(url_string);
            if((url_err = url.GetError()) != wxURL_NOERR)
            {
                flag_log = 0;
                if(pvoutput.failed == 0)
                    flag_log = 1;
                LogMessage(_T("Bad URL: ") + url_string, flag_log);
                pvoutput.failed = 1;
                return;
            }

            url.GetProtocol().SetTimeout(30);  // change from default=10 mins to 30 seconds

            wxInputStream *input = url.GetInputStream();  // connect to the URL
            url_err = url.GetError();

            if(input == NULL)
            {
                flag_log = 0;
                if(pvoutput.failed)
                    flag_log = 1;
//                LogMessage(_T("PVOutput.org update failed"), flag_log);
                if(url_err == wxURL_NOHOST)
                    err_string = _T("No internet");
                else
                if(url_err == wxURL_NOPATH)
                    err_string = _T("No path");
                else
                if(url_err == wxURL_CONNERR)
                    err_string = _T("Connection error");
                else
                if(url_err == wxURL_PROTOERR)
                    err_string = _T("Protocol error");
                else
                    err_string = wxString::Format(_T("Error %d"), url_err);

                LogMessage(_T("PVOutput.org update failed: ") + err_string, flag_log);
                pvoutput.failed = 1;

                fname = data_dir + _T("/system/pvoutput_fail.txt");
                if((f = fopen(fname.mb_str(wxConvLocal), "a")) != NULL)
                {
                    fprintf(f, "%s,%d:%.2d,%d,%d,-1,-1,%.2f",  date_ymd, hours, mins, (int)(total_e*1000), total_p, inverters[0].temperature);
                    if(grid_voltage > 0)
                    {
                        fprintf(f, ",%5.1f", grid_voltage);
                    }
                    fputc('\n', f);
                    fclose(f);
                }
            }

            if(input != NULL)
                delete input;
        }
    }
}

void LogColumnHeaders(int inverter, int control)
{//=============================================
// control: 1= only if the header has changed
    int ix;
    int n_columns;
    int type;
    FILE *f;
    char columns[N_EXTRA_READINGS+1];
    static char prev_columns[N_EXTRA_READINGS+1] = {0};

    if((f = inverters[inverter].f_log) == NULL)
        return;

    for(ix=0, n_columns=0; ix<N_EXTRA_READINGS; ix++)
    {
        if(((type = extra_readings[ix].type) > 0) && (extra_readings[ix].graph_slot > 0))
        {
            columns[n_columns++] = type;
        }
    }
    columns[n_columns] = 0;

    if((control == 1) && (strcmp(columns, prev_columns) == 0))
        return;

    strcpy(prev_columns, columns);

    fprintf(f, "%s", "time\tpout\tpw1\tvt1\tpw2\tvt2\ttmpr");

    for(ix=0; ix<n_columns; ix++)
    {
        fprintf(f, "\t%s", extra_reading_types[(int)columns[ix]].mnemonic);
    }
    fputc('\n', f);
    fflush(f);
    inverters[inverter].logheaders_changed = 0;
}

void MakeDate(wxDateTime &date, wxString ymd)
{//==========================================
    long year, month, day;

    ymd.Left(4).ToLong(&year);
    ymd.Mid(4,2).ToLong(&month);
    ymd.Right(2).ToLong(&day);
    date.Set(day, wxDateTime::Month(month-1), year);
}

wxString MakeDateString(wxString ymd)
{//==================================
    wxDateTime date;
    const wxString fmt[N_DATE_FORMATS] = {
        _("%Y-%m-%d"), _("%x"), _("%d %b %Y"), _("%d.%m.%Y")};

    MakeDate(date, ymd);

    // default is ISO, yyyy-mm-dd
    if(option_date_format > 4)
        option_date_format = 0;
    return(date.Format(fmt[option_date_format]));
}

void OpenLogFiles(int create, int control)
{//=======================================
// open log file for power and voltage readings
// create = inverter number, or -1 (don't create log files)  -2 program start, also start new com_log
    int inv = 0;
    INVERTER *iv;
    struct tm *btime;
    time_t current_time;
    char fname[256];
    wxString fname_log;

    if(TimeZone2 > 24)
    {
        Timezone = (double)GetGmtOffset(0) / 3600.0;  // current timezone, including daylight saving time
    }
    else
    {
        Timezone = TimeZone2;
    }
    if(Longitude > 360)
    {
        // Longitude is not set, guess from TimeZone
        Longitude = Timezone * 360/24;
    }

    time(&current_time);
    btime = localtime(&current_time);
    sprintf(date_ymd, "%.4d%.2d%.2d", btime->tm_year+1900, btime->tm_mon+1, btime->tm_mday);
    date_ymd_string = wxString(date_ymd, wxConvLocal);
    graph_date_ymd = date_ymd_string;
    graph_date = MakeDateString(graph_date_ymd);

    if(date_logchange != date_ymd_string)
    {
        // change to a new com_log
        // 30/July/2022 - these two lines are not needed...
        // wxRemoveFile(data_dir+_T("/com_log.txt"));   // location used by previous version
        // wxRemoveFile(data_dir+_T("/com_log_yesterday.txt"));

        // back up com_log, overwrite 'com_log_yesterday' if exists
        fname_log = data_dir+_T("/system/com_log.txt");
        if(wxFileExists(fname_log))
        {
            wxRenameFile(fname_log, data_dir+_T("/system/com_log_yesterday.txt"), true);
        }
        // back up message_log, overwrite 'message_log_yesterday' if exists
        fname_log = data_dir+_T("/system/message_log.txt");
        if(wxFileExists(fname_log))
        {
            wxRenameFile(fname_log, data_dir+_T("/system/message_log_yesterday.txt"), true);
        }
        date_logchange = date_ymd_string;
        ConfigSave(1);
        pvoutput.failed = 0;
    }

    if (!wxDirExists(data_dir))
        wxMkdir(data_dir);

    data_system_dir = data_dir + _T("/system");
    if (!wxDirExists(data_system_dir))
        wxMkdir(data_system_dir);

    data_year_dir = data_dir + wxString::Format(_T("/%.4d"), btime->tm_year+1900);
    data_year_dir2 = data_year_dir + _T("_out");

    if (!wxDirExists(data_year_dir))
    {
        wxMkdir(data_year_dir);
    }
    if (!wxDirExists(data_year_dir2))
    {
        wxMkdir(data_year_dir2);
    }


    graph_panel->NewDay();
    for(inv=0; inv<N_INV; inv++)
    {
        iv = &inverters[inv];
        if(iv->f_log != NULL)
        {
            fclose(iv->f_log);
            iv->f_log = NULL;
        }

        if(inverter_address[inv] <= 0)
            continue;

        fname_log.Printf(_T("%s/d%d_%s"), data_year_dir.c_str(), inverter_address[inv], date_ymd_string.c_str());

        // load any data from today's log file
        graph_panel->ReadEnergyFile(inv, fname_log, control, NULL);

        strcpy(fname, fname_log.mb_str(wxConvLocal));
        strcat(fname, ".txt");

        if(GetFileLength(fname))
        {
            iv->f_log = fopen(fname, "a");
            if(create == -2)
                iv->logheaders_changed = 2;   // program start, write column headers when the first log entry is written
        }
        else
        if(create == inv)
        {
            if((iv->f_log = fopen(fname, "w")) == NULL)
            {
                wxLogError(_T("Can't write to log file: ") + fname_log);
            }
            else
            {
                fprintf(iv->f_log, "AUR1 %s #%d\n", date_ymd, inverter_address[inv]);
                iv->logheaders_changed = 2;   // write column headers when the first log entry is written
            }
        }
    }

    CalcSun(graph_date_ymd);   // called after we set the time zone
    graph_panel->ResetGraph();
    display_today = 1;
}  // end of OpenLogFiles

#pragma endregion

#pragma region InverterInfo

int CounterString(char *string, int inv, int counter)
{//==================================================
    unsigned long duration;
    int len;
    int ix;
    char *p;
    char buf[20];
    char padding[20];
    static const char *counter_name[4] = {
        "Total Running Time   (Lifetime)      ",
        "Partial Running Time (since reset)",
        "Total Time With Grid Connection   "};

    duration = inverters[inv].counters[counter];
    sprintf(buf, "%.2f", (double)duration/3600);
    len = strlen(buf);

    p = padding;
    ix = (8 - len);
    while(ix-- > 0)
    {
        *p++ = ' ';
    }
    *p = 0;

    sprintf(string, "%s\t%s%9.2f hrs\n", counter_name[counter], padding, (double)duration/3600);
    return(strlen(string));
}

void GotInverterInfo(int inv, int ok)
{//==================================
    char *data = NULL;
    int length;
    int ix;
    char *p;
    FILE *f;
    wxString fname = data_system_dir + wxString::Format(_T("/InverterInfo%d"), inverter_address[inv]);
    wxString noresponse = wxEmptyString;


    if(ok <= 0)
    {
        noresponse = _T(" (no response)");
    }

    length = GetFileLength(fname.mb_str(wxConvLocal));
    if(length == 0)
    {
        wxLogError(wxString::Format(_T("No response from inverter #%d"), inverter_address[inv]));
    }
    else
    {
        if((data = (char *)malloc(length + 300)) == NULL)
            return;

        if((f = fopen(fname.mb_str(wxConvLocal), "r")) != NULL)
            length = fread(data, 1, length, f);
        else
            length = 0;

        data[length] = 0;

        if(ok == 1)
        {
            p = &data[length];
            for(ix=0; ix<3; ix++)
            {
                p += CounterString(p, inv, ix);
            }

            sprintf(p, "\nInverter-computer time difference: %4d seconds", inverters[inv].time_offset);
        }

        wxMessageBox(wxString(data, wxConvLocal), wxString::Format(_T("Inverter address %d") + noresponse, inverter_address[inv]), wxOK, mainframe);
        free(data);
    }
}  // end of GotInverterInfo

#pragma endregion

#pragma region Mainframe

void Mainframe::ShowEnergy(int inv)
{//================================
    int ix;
    double *e;
    wxString format;

    e = inverters[inv].energy_total;
    if(e[4] == 0)
        return;   // total energy, zero implies that data is not available

    for(ix=0; ix < 6; ix++)
    {
        if(ix==1) continue;  // skip 'week'

        txt_energy[inv][ix]->Clear();
        if(e[ix] >= 10000)
            format = _T("%6.0f");
        else
        if(e[ix] >= 1000)
            format = _T("%6.1f");
        else
        if((e[ix] >= 100) || (ix > 0))
            format = _T("%6.2f");
        else
            format = _T("%6.3f"); // only show 3 decimal places for "today" energy
        txt_energy[inv][ix]->WriteText(wxString::Format(format, e[ix]));
    }
}

int Mainframe::DataResponse(int data_ok, INVERTER_RESPONSE *ir)
{//============================================================
    int ix;
    int hrs, mins;
    int seconds;
    int computer_seconds;
    DATAREC datarec;
    INVERTER *iv;
    struct tm btime;
    time_t current_time;
    wxString fname_daily;
    static float tmpr_max = 0;
    int inv = 0;
    int period_type=0;  // 0=other 1= 10sec, 2= 1 minute
    int power_value;
    double energy_today;
    static double prev_energy_today = -1;
    double *etot;
    char ymd[20];
    char format[20];
    static int first_energy = 1;

    time(&current_time);
    btime = *localtime(&current_time);
    computer_seconds = btime.tm_hour*3600 + btime.tm_min*60 + btime.tm_sec;
    sprintf(ymd, "%.4d%.2d%.2d", btime.tm_year+1900, btime.tm_mon+1, btime.tm_mday);

    // Check for any unsent data for pvoutput.org
    ResendPvoutput(computer_seconds / 120);

    inv = command_inverter;
    iv = &inverters[inv];
    etot = iv->energy_total;

    if(data_ok == 1)
    {
        if(ir->flags & IR_HAS_STATE)
        {
            GotInverterStatus(command_inverter);
        }

        if((iv->f_log == NULL) || (memcmp(ymd, date_ymd, 8) != 0))
        {
            // The first data of a new day.
            // Open a new log file and clear the graphs
            OpenLogFiles(command_inverter, 0);
            iv->need_time_offset = 1;
        }

        seconds = computer_seconds - 1;  // allow for delay in receiving the response from the inverter
        if(ir->flags & IR_HAS_TIME)
        {
            // inverters[inv].time_offset has been set
            //wxLogStatus(wxString::Format(_T("time offset %d"), inverters[inv].time_offset));
        }

        if((ir->flags & IR_HAS_PEAK) && (strcmp(ymd, inverters[inv].today_done) == 0))
        {
            inverters[inv].peak = ir->dsp[DSP_PEAK_TODAY];
            txt_energy[inv][1]->ChangeValue(wxString::Format(_T("%d"), inverters[inv].peak));
        }

        if(ir->flags & IR_HAS_ENERGY_TODAY)
        {
            etot[0] = ir->energy[0];
            LogEnergy(seconds, ymd);
        }

        if(ir->flags & IR_HAS_ENERGY)
        {
            etot[0] = ir->energy[0];
            etot[1] = ir->energy[1];
            etot[2] = ir->energy[2];
            etot[3] = ir->energy[3];
            etot[4] = ir->energy[4];
            etot[5] = ir->energy[5];

            if(first_energy)
            {
                first_energy = 0;
                LogCommMsg(wxString::Format(_T("Program start: Energy %7.3f  %8.3f %8.3f %9.3f %10.3f %9.3f"), etot[0],etot[1],etot[2],etot[3],etot[4],etot[5]));
            }

            if((strcmp(ymd, iv->today_done) != 0) && (ir->pw[0] > 0))
            {
                // daily and weekly seem to get reset before monthly and yearly, so
                // wait until the inverter is generating power before recording
                // the start-of-day totals.
                FILE *f_daily;
                double e;
                double yesterday;
                bool exists;
                char yesterday_str[20];

                if(iv->auto_retrieve)
                {
                    iv->do_retrieve_data = 1;
                }

                e = etot[0];  // energy today
                if(iv->previous_total == 0)
                    yesterday = 0;
                else
                    yesterday = etot[4] - e - iv->previous_total;

                fname_daily = data_year_dir + wxString::Format(_T("/inverter%d_%d_") + data_year_dir.Right(4) + _T(".txt"), inv, inverter_address[inv]);
                exists = wxFileExists(fname_daily);
                if((f_daily = fopen(fname_daily.mb_str(wxConvLocal), "a")) != NULL)
                {
                    if(!exists)
                    {
                        // write column headers at the start of the file
                        fprintf(f_daily,"#end of    today     week     month     year      total     partial   peakW\n");
                    }

                    // print the yesterday, week, month, year, total, partial totals for the start of the day
                    strcpy(yesterday_str, YesterdayDate(wxString(ymd, wxConvLocal)).mb_str(wxConvLocal));
                    fprintf(f_daily, "%s %7.3f  %8.3f %8.3f %9.3f %10.3f %9.3f %6d\n", yesterday_str, yesterday,
                            etot[1]-e, etot[2]-e, etot[3]-e, etot[4]-e, etot[5]-e, iv->peak);
                    fclose(f_daily);
                }
                iv->previous_total = etot[4] - e;
                strcpy(iv->today_done, ymd);
                strcpy(ymd_energy, ymd);
                ConfigSave(1);

                // remove old entries from the alarms file
                PruneAlarmFile(inv);

                // set inverter time to computer time (if it differs by more than ??)
//                QueueCommand(inv, cmdSetInvTime2);
            }
        }


        period_type = 0;

        if((seconds / 10) != iv->current_period)
        {
            // this is the first response in this 10 second period
            period_type = 1;   // start of  10 second period
            iv->period_in_minute++;

            if((seconds / 60) != iv->previous_minute)
            {
                period_type = 2;   // start of a minute period
                iv->period_in_minute = 0;
                iv->previous_minute = seconds / 60;

                if((iv->previous_minute & 5) == 0)
                {
                    period_type = 3;    // 5 minute period

                    if((inv==0) || (inverters[0].alive == 0))
                        ConfigSave(1);    // save energy totals every 5 minutes
                }


                // do we want to retrieve data from this inverter?
                if((iv->do_retrieve_data != 0) && (retrieving_data == 0))
                {
                    retrieve_message = _T("10 second energy data");
                    if(QueueCommand(inv, cmdInverter10SecEnergy) == 0)
                    {
                        iv->do_retrieve_data = 0;
                        retrieving_fname.Printf(_T("%s/r%d_%s"), data_year_dir2.c_str(), inverter_address[inv], date_ymd_string.c_str());
                        retrieving_pvoutput = 1;
                        retrieving_progress = 0;
                    }
                }
            }

            if(inverters[inv].current_period >= 0)
            {
                // get extra reading values
                for(ix=0; ix<N_EXTRA_READINGS; ix++)
                {
                    int dsp_code;
                    int j;
                    EXTRA_READING_TYPE *ptype;
                    unsigned short *p;
                    if(((dsp_code = extra_readings[ix].dsp_code) > 0) && (extra_readings[ix].graph_slot > 0))
                    {
                        if(dsp_data[inv][dsp_code] == NULL)
                        {
                            dsp_data[inv][dsp_code] = p = (unsigned short *)malloc(sizeof(unsigned short) * N_10SEC);
                            for(j=0; j<N_10SEC; j++)
                                p[j] = 0xffff;
                        }
                        ptype = &extra_reading_types[extra_readings[ix].type];
                        dsp_data[inv][dsp_code][inverters[inv].current_period] = (int)((ir->dsp[dsp_code] + ptype->addition) * ptype->multiply);
                    }
                }

                if(ir->n_av_pwin > 0)
                {
                    // write out the average values for the previous 10 second period
                    if(iv->f_log != NULL)
                    {
                        double pw0, pw1, pw2, vt1, vt2;
                        static double tmpr = 0;
                        pw0 = iv->averages[0] / ir->n_av_pw0;
                        pw1 = iv->averages[1] / ir->n_av_pwin;
                        pw2 = iv->averages[3] / ir->n_av_pwin;
                        vt1 = iv->averages[2] / ir->n_av_pwin;
                        vt2 = iv->averages[4] / ir->n_av_pwin;
                        if(ir->n_av_tmpr > 0)
                        {
                            tmpr = iv->averages[5] / ir->n_av_tmpr;
                            iv->temperature = tmpr;
                        }

                        iv->last_power_out = (int)(pw0 + 0.05);
                        iv->last_power_out_time = seconds;

                        if((pw1 >= 5.0) || (pw2 >= 5.0))
                        {
                            // Only log when input power is more than 5 watts (to avoid logging useless data
                            // Ensure all power and voltage values are not negative

                            if(iv->logheaders_changed != 0)
                            {
                                LogColumnHeaders(inv, iv->logheaders_changed);   // column headers for extra readings may have changed
                            }

                            if(pw0 < 0) pw0 = 0;
                            if(vt1 < 0) vt1 = 0;
                            if(vt2 < 0) vt2 = 0;
                            fprintf(iv->f_log, "%s\t%6.1f\t%6.1f\t%6.2f\t%6.1f\t%6.2f\t%5.2f", iv->current_timestring,
                                pw0, pw1, vt1, pw2, vt2, tmpr);

                            // write out any extra readings
                            for(ix=0; ix<N_EXTRA_READINGS; ix++)
                            {
                                if((extra_readings[ix].type > 0) && (extra_readings[ix].graph_slot > 0))
                                {
                                    sprintf(format, "\t%%.%df", extra_reading_types[extra_readings[ix].type].decimalplaces+1);
                                    fprintf(iv->f_log, format, ir->dsp[extra_readings[ix].dsp_code]);
                                }
                            }
                            // write out total energy
                            fprintf(iv->f_log, "\t%9.3f", etot[4]);

                            fputc('\n', iv->f_log);
                            if(iv->period_in_minute == 0)
                            {
#ifdef FLASH_MEMORY
                                if((iv->previous_minute & 5) == 0)
                                    fflush(iv->f_log);
#else
                                fflush(iv->f_log);
#endif
                            }
                        }
                    }
                }
            }
            inverters[inv].current_period = seconds / 10;
            hrs = seconds/3600;
            mins = (seconds % 3600) /60;
            sprintf(iv->current_timestring, "%.2d:%.2d:%.2d", hrs, mins, seconds % 60);

            iv->averages[0] = 0;
            iv->averages[1] = 0;
            iv->averages[2] = 0;
            iv->averages[3] = 0;
            iv->averages[4] = 0;
            iv->averages[5] = 0;
            iv->averages[6] = 0;
            ir->n_av_pw0 = 0;
            ir->n_av_pwin = 0;
            ir->n_av_tmpr = 0;
        }  // end of new 10 second period


        // add the new data values to the averages for this 10 second period
        iv->averages[0] += ir->pw[0];  // power output reading is always present
        ir->n_av_pw0++;
        if(ir->flags & IR_HAS_POWERIN)
        {
            iv->averages[1] += ir->pw[1];
            iv->averages[2] += ir->vt[1];
            iv->averages[3] += ir->pw[2];
            iv->averages[4] += ir->vt[2];
            iv->averages[6] += ir->vt[0];
            ir->n_av_pwin++;
        }
        if(ir->flags & IR_HAS_TEMPR)
        {
           tmpr_max = ir->tempr[0];
            if(ir->tempr[1] > tmpr_max)
                tmpr_max = ir->tempr[1];
            iv->averages[5] += tmpr_max;
            ir->n_av_tmpr++;
        }


        if(display_today)
        {
            if(ir->n_av_pwin > 0)
            {
                double value;

                // update the power graph display
                if(ir->n_av_pw0 > 0)
                {
                    value = iv->averages[0] * X_PO / ir->n_av_pw0;
                    datarec.pw0 = (unsigned short)(value + 0.5);
                }

                if(ir->n_av_pwin > 0)
                {
                    datarec.pw1 = (unsigned short)(iv->averages[1] * X_PI / ir->n_av_pwin);
                    datarec.vt1 = (unsigned short)(iv->averages[2] * X_VI / ir->n_av_pwin);
                    datarec.pw2 = (unsigned short)(iv->averages[3] * X_PI / ir->n_av_pwin);
                    datarec.vt2 = (unsigned short)(iv->averages[4] * X_VI / ir->n_av_pwin);
                }

                if(ir->n_av_tmpr > 0)
                {
                    datarec.tmpr= (short)(iv->averages[5] * 100 / ir->n_av_tmpr);
                }
                else
                {
                    datarec.tmpr = (unsigned short)(tmpr_max * 100);
                }
                datarec.pwi = 0;

                energy_today = inverters[0].energy_total[0] + inverters[1].energy_total[0];
                if(energy_today != prev_energy_today)
                {
                    graph_panel->AddPoint(inv, inverters[inv].current_period, &datarec, 1);
                    prev_energy_today = energy_today;
                }
                else
                {
                    graph_panel->AddPoint(inv, inverters[inv].current_period, &datarec, 0);
                }
            }
        }
        else
        {
            graph_panel->AddPoint(inv, inverters[inv].current_period, NULL, 1);
        }
    }


    if((data_ok == 1) || (inverters[inv ^ 1].alive == 0))
    {
        // don't clear if these will be updated by the other inverter
        txt_powertot->Clear();
    }

    txt_power[inv]->Clear();

    if((ir->flags & IR_HAS_POWERIN) || (data_ok == 0))
    {
        txt_power1[inv]->Clear();
        txt_power2[inv]->Clear();
        txt_volts1[inv]->Clear();
        txt_volts2[inv]->Clear();
        txt_current1[inv]->Clear();
        txt_current2[inv]->Clear();
        txt_efficiency[inv]->Clear();
        txt_temperature[inv]->Clear();
    }

    if(data_ok == 1)
    {

        txt_power[inv]->WriteText(wxString::Format(_T("%4d"), (int)(ir->pw[0] + 0.5)));
        power_total[inv] = ir->pw[0];
        power_value = (int)(power_total[0] + power_total[1] + 0.5);

        SetTitle(wxString::Format(_T("Aurora %2d"), power_value));
        txt_powertot->WriteText(wxString::Format(_T("%4d"), power_value));
        MonitorOutput(power_value);

        if(ir->flags & IR_HAS_ENERGY_TODAY)
        {
            ShowEnergy(inv);
        }

        if(ir->flags & IR_HAS_POWERIN)
        {
            txt_power1[inv]->WriteText(wxString::Format(_T("%4d"), (int)(ir->pw[1] + 0.5)));
            txt_power2[inv]->WriteText(wxString::Format(_T("%4d"), (int)(ir->pw[2] + 0.5)));
            txt_volts1[inv]->WriteText(wxString::Format(_T("%3d"), (int)(ir->vt[1] + 0.5)));
            txt_volts2[inv]->WriteText(wxString::Format(_T("%3d"), (int)(ir->vt[2] + 0.5)));
            txt_current1[inv]->WriteText(wxString::Format(_T("%.2f"), ir->ct[1]));
            txt_current2[inv]->WriteText(wxString::Format(_T("%.2f"), ir->ct[2]));
            txt_efficiency[inv]->WriteText(wxString::Format(_T("%.1f"), ir->effic));
            txt_temperature[inv]->WriteText(wxString::Format(_T("%.1f"), tmpr_max));
        }

        if(ir->flags & IR_HAS_EXTRA)
        {
            int ix;
            int slot;
            int code;
            wxString format;
            double value;
            EXTRA_READING_TYPE *ex;

            for(ix=0; ix<N_EXTRA_READINGS; ix++)
            {
                if((slot = extra_readings[ix].status_slot) > 0)
                {
                    ex = &extra_reading_types[extra_readings[ix].type];
                    format.Printf(_T("%%.%df"), ex->decimalplaces);
                    code = extra_readings[ix].dsp_code;
                    value = ir->dsp[code] * ex->multiplier;
                    txt_dsp_param[inv][slot-1]->ChangeValue(wxString::Format(format, value));
                }
            }
        }
   }
   else
   {
        inverters[inv].alive = 0;
        power_total[inv] = 0;
        if(inverters[inv ^ 1].alive == 0)
            SetTitle(_T("Aurora"));  // neither inverter is alive
   }

    return(period_type);
}  // end of DataResponse

void Mainframe::OnKey(wxKeyEvent &event)
{//=====================================
    if(graph_panel->OnKey2(event) >= 0)
        return;
    event.Skip();
}

void Mainframe::OnMove(wxMoveEvent &event)
{//=====================================
    // Get current position and size of main form once move has stopped.
    mainframe->MainPosX = GetPosition().x;
    mainframe->MainPosY = GetPosition().y;
    mainframe->MainWidth = GetSize().GetWidth();
    mainframe->MainHeight = GetSize().GetHeight();
    event.Skip();
}

const wxString key_info = _T(""
    "F2     \tToday's charts.\n"
    "F3     \tHistogram of daily energy totals.\n"
    "F4     \tInverter status and alarm state.\n"
    "F11    \tToggle full-screen display.\n"
    "Escape \tEnd full-screen display.\n"
    " [     \tShow the charts for the previous day.\n"
    " ]     \tShow the charts for the next day.\n"
    "\n"
    "Page Up\tDisplay the next chart page.\n"
    "Page Down\tDisplay the previous chart page.\n"
    "Home   \tDisplay the first chart page.\n"
    "End    \tDisplay the last chart page.\n"
    " <   > \tChange the x-scale.\n"
    "Up,  Down\tChange the y-scale (power graphs only).\n"
    "Left,  Right\tHorizontal scroll.");

wxHtmlWindow *html_help = NULL;
wxDialog *dlg_help = NULL;

void Mainframe::ShowHelp()
{//=======================
    int width, height;
    wxString fname;

#ifdef __WXMSW__
    fname = _T("help.html");
#else
    fname = data_system_dir + _T("/help.html");
    if(wxFileExists(fname) == false)
    {
        wxMessageBox(key_info, _T("Key commands"), wxOK, this);
        return;
    }
#endif

    mainframe->GetClientSize(&width, &height);
    if(dlg_help == NULL)
    {
        dlg_help = new wxDialog(mainframe, -1, wxString(_T("Aurora Monitor Help")));
        html_help = new wxHtmlWindow(dlg_help);
    }
    dlg_help->SetSize(width-800, 0, 800, height);
    html_help->LoadPage(fname);
    dlg_help->Show(true);
}

void Mainframe::OnAbout()
{
    // https://docs.wxwidgets.org/3.0/classwx_about_dialog_info.html
    wxAboutDialogInfo info;
    info.SetName(_("Aurora Monitor"));
    info.SetVersion(_("1.08"));
    info.SetDescription(_("Receives and displays data from up to 2 Aurora power inverters.\n"));
    info.SetCopyright(_T("(C) 2012 Jonathan Duddington <jonsd@users.sourceforge.net>"));
    info.AddDeveloper(wxT("2012 Jonathan Duddington"));
    info.AddDeveloper(wxT("\n2022 nbl1268"));
    info.SetLicence(_T("GNU GENERAL PUBLIC LICENSE Version 3\nhttp://www.gnu.org/licenses/gpl.html"));
    info.SetWebSite(wxT("https://github.com/nbl1268"));

    wxAboutBox(info);
}

void Mainframe::OnCommand(wxCommandEvent &event)
{//=============================================
    int id;
    int inv;
    wxString string;

    id = event.GetId();
    switch(id)
    {
    case idMenuQuit:
        Destroy();
        break;

    case idMenuAbout:
        OnAbout();
        break;

    case idDisplayToday:
        OpenLogFiles(-1, 1);
        graph_panel->SetMode(0);
        break;

    case idDisplayDate:
        string = SelectDisplayDate();
        if(string != wxEmptyString)
            DisplayDateGraphs(string, 0);
        break;

    case idNextDate:
        DisplayDateGraphs(NextDate(graph_date_ymd, 0), 1);
        break;

    case idPrevDate:
        DisplayDateGraphs(PrevDate(graph_date_ymd, 0), 1);
        break;

    case idImportStored:
#ifdef deleted
        fname = wxFileSelector(_T("Select energy file"), data_dir, wxEmptyString, _T(""), _T("q_*"));
        if(fname == wxEmptyString)
            return;

        date = wxGetTextFromUser(_T("Search file for date (yyyy-mm-dd):"), _T("Aurora"), graph_date);
        result = graph_panel->ReadEnergyFile(-1, fname, 0, date.mb_str(wxConvLocal));
        if(result == -2)
        {
            wxLogError(_T("Data for ") + date + _T(" not found in this file"));
        }
        else
        if(result != 0)
        {
            wxLogError(_T("Failed to read data file: ") + fname);
        }
        display_today = 0;
#endif
        break;

    case idDisplayEnergy:
        graph_panel->SetMode(1);
        break;

    case idMenuSetup:
        dlg_setup->ShowDlg();
        break;

    case idMenuLocation:
        dlg_location->ShowDlg();
        break;

    case idMenuSetInverter:
    case idMenuSetInverter+1:
        inv = id - idMenuSetInverter;
        dlg_inverter->ShowDlg(inv);
        break;

    case idMenuEditCharts:
        dlg_chart->ShowDlg();
        break;

    case idMenuEditHistogram:
        dlg_histogram->ShowDlg();
        break;

    case idMenuExtraReadings:
        dlg_extrareadings->ShowDlg();
        break;

    case idMenuPvoutput:
        dlg_pvoutput->ShowDlg();
        break;

    case idMenuExportPvo:
        SavePvoutput(0);
        break;

    case idMenuSunInfo:
        SunInfo();
        break;

    case idMenuHelp:
        ShowHelp();
        break;

    case idMenuInverterState:
        if (dlg_alarms->IsShown())
            dlg_alarms->Close();
        else
            ShowInverterStatus();
            this->SetFocus();
        break;

    case idMenuInverterInfo:
    case idMenuInverterInfo+1:
        QueueCommand(id - idMenuInverterInfo, cmdInverterInfo);
        break;

    case idMenuInverterSetTime:
    case idMenuInverterSetTime+1:
        dlg_settime->ShowDlg(id - idMenuInverterSetTime);
        break;

    case idMenuInverterResetPartial:
    case idMenuInverterResetPartial+1:
        QueueCommand(id - idMenuInverterResetPartial, cmdResetPartial);
        break;

    case idMenuInverterDailyEnergy:
    case idMenuInverterDailyEnergy+1:
        dlg_retrieve_energy->ShowDlg(id - idMenuInverterDailyEnergy, cmdInverterDailyEnergy);
        break;

    case idMenuInverter10SecEnergy:
    case idMenuInverter10SecEnergy+1:
        dlg_retrieve_energy->ShowDlg(id - idMenuInverter10SecEnergy, cmdInverter10SecEnergy);
        break;

    case idMenuFullscreen:
        if(mainframe->IsFullScreen())
            mainframe->ShowFullScreen(false);
        else
            mainframe->ShowFullScreen(true);
        Refresh();
        break;

    case idMenuMessageLog:
        DisplayTextfile(data_dir + _T("/system/message_log.txt"), _T("Message Log  ")+MakeDateString(date_logchange));
        break;
    case idMenuCommLog:
        DisplayTextfile(data_dir + _T("/system/com_log.txt"), _T("Communication Log  ")+MakeDateString(date_logchange));
        break;
    case idMenuAlarmLog:
    case idMenuAlarmLog+1:
        inv = id - idMenuAlarmLog;
        DisplayTextfile(wxString::Format(_T("%s/system/alarms%d_%d.txt"), data_dir.c_str(), inv, inverter_address[inv]),
                        wxString::Format(_T("Alarm log, inverter address %d"), inverter_address[inv]));
        break;
    case idMenuPvoLog:
        string.Printf(_T("%s/e5min_%s.txt"), data_year_dir2.c_str(), date_ymd_string.c_str());
        DisplayTextfile(string, _T("5 Minute Data (for pvoutput.org)  ")+MakeDateString(date_logchange));
        break;
    case idMenu10secLog:
    case idMenu10secLog+1:
        inv = id - idMenu10secLog;
        DisplayTextfile(wxString::Format(_T("%s/d%d_%s.txt"), data_year_dir.c_str(), inverter_address[inv], date_ymd_string.c_str()),
                        wxString::Format(_T("10 Second Data, inverter address %d"), inverter_address[inv]));
        break;
    }
}

wxMenuBar *mbar;

void Mainframe::MakeInverterMenu(int control)
{//==========================================
    wxMenu *invMenu;

    if(control == 1)
    {
        invMenu = mbar->GetMenu(2);
        mbar->Remove(2);
        delete invMenu;
    }

    invMenu = new wxMenu(_T(""));
    if(inverter_address[1] == 0)
    {
        invMenu->Append(idMenuInverterInfo, _("Inverter information"));
        invMenu->Append(idMenuInverterSetTime, _("Set inverter time"));
        invMenu->Append(idMenuInverterResetPartial, _("Reset partial energy and time"));
        invMenu->Append(idMenuInverterDailyEnergy, _("Retrieve daily energy"));
        invMenu->Append(idMenuInverter10SecEnergy, _("Retrieve 10sec energy"));
    }
    else
    {
        invMenu->Append(idMenuInverterInfo, _("A:  Inverter information"));
        invMenu->Append(idMenuInverterSetTime, _("A:  Set inverter time"));
        invMenu->Append(idMenuInverterResetPartial, _("A:  Reset partial energy and time"));
        invMenu->Append(idMenuInverterDailyEnergy, _("A:  Retrieve daily energy"));
        invMenu->Append(idMenuInverter10SecEnergy, _("A:  Retrieve 10sec energy"));
        invMenu->AppendSeparator();
        invMenu->Append(idMenuInverterInfo+1, _("B:  Inverter information"));
        invMenu->Append(idMenuInverterSetTime+1, _("B:  Set inverter time"));
        invMenu->Append(idMenuInverterResetPartial+1, _("B:  Reset partial energy and time"));
        invMenu->Append(idMenuInverterDailyEnergy+1, _("B:  Retrieve daily energy"));
        invMenu->Append(idMenuInverter10SecEnergy+1, _("B:  Retrieve 10sec energy"));
    }
//    dataMenu->Append(idMenuExportPvo, _("Export data for pvoutput"));

    mbar->Insert(2, invMenu, _T("&Inverter"));
}

void Mainframe::MakeMenus()
{//========================
    // create a menu bar
    mbar = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tAlt-F4"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu *displayMenu = new wxMenu(_T(""));
    displayMenu->Append(idDisplayToday, _T("Today\tF2"));
    displayMenu->Append(idDisplayDate, _T("Select day"));
    displayMenu->Append(idNextDate, _T("Next day\t]"));
    displayMenu->Append(idPrevDate, _T("Previous day\t["));
    displayMenu->AppendSeparator();
//    displayMenu->Append(idImportStored, _T("Import -q energy data"));
//    displayMenu->AppendSeparator();
    displayMenu->Append(idDisplayEnergy, _T("Energy histogram\tF3"));
    displayMenu->AppendSeparator();
    displayMenu->Append(idMenuInverterState, _T("Inverter status\tF4"));
    displayMenu->AppendSeparator();
    displayMenu->Append(idMenuFullscreen, _("Fullscreen\tF11"));
    mbar->Append(displayMenu, _T("&Display"));

    MakeInverterMenu(0);

    wxMenu *settingsMenu = new wxMenu(_T(""));
    settingsMenu->Append(idMenuSetup, _T("Setup"));
    settingsMenu->Append(idMenuLocation, _T("Location"));
    if(inverter_address[1] == 0)
    {
        settingsMenu->Append(idMenuSetInverter, _T("Inverter"));
    }
    else
    {
        settingsMenu->Append(idMenuSetInverter, _T("Inverter A"));
        settingsMenu->Append(idMenuSetInverter, _T("Inverter B"));
    }
    settingsMenu->Append(idMenuPvoutput, _T("PVOutput.org"));
    settingsMenu->AppendSeparator();
    settingsMenu->Append(idMenuEditCharts, _T("Charts"));
    settingsMenu->Append(idMenuEditHistogram, _T("Histograms"));
    settingsMenu->Append(idMenuExtraReadings, _T("Extra readings"));
    mbar->Append(settingsMenu, _T("&Settings"));

    wxMenu *logsMenu = new wxMenu(_T(""));
    logsMenu->Append(idMenuMessageLog, _T("Message log"));
    if(inverter_address[1] == 0)
    {
        logsMenu->Append(idMenuAlarmLog, _("Alarm Log"));
    }
    else
    {
        logsMenu->Append(idMenuAlarmLog, _("A:  Alarm Log"));
        logsMenu->Append(idMenuAlarmLog+1, _("B:  Alarm Log"));
    }
    logsMenu->Append(idMenuPvoLog, _T("5 minute data"));
    if(inverter_address[1] == 0)
    {
        logsMenu->Append(idMenu10secLog, _("10 second data"));
    }
    else
    {
        logsMenu->Append(idMenu10secLog, _("A:  10 second data"));
        logsMenu->Append(idMenu10secLog+1, _("B:  10 second data"));
    }
    logsMenu->Append(idMenuCommLog, _T("Communication log"));
    mbar->Append(logsMenu, _T("&Logs"));


    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuSunInfo, _("Sun information"));
    helpMenu->Append(idMenuHelp, _("Help information\tF1"));
    helpMenu->Append(idMenuAbout, _("&About"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    SetMenuBar(mbar);
}

Mainframe::Mainframe(wxFrame *frame, const wxString& title)
    // : wxFrame(frame, -1, title, wxDefaultPosition, wxSize(1024,768), wxDEFAULT_FRAME_STYLE)
    : wxFrame(frame, -1, title, wxDefaultPosition, wxSize(1058, 768), wxDEFAULT_FRAME_STYLE)
{//========================================================
    int inv;

    mainframe = this;
    CreateStatusBar(1);
    MakeMenus();

	startuptimer.SetOwner(this->GetEventHandler(), idStartupTimer);
    pulsetimer.SetOwner(this->GetEventHandler(), idPulseTimer);
    logstatus_timer.SetOwner(this->GetEventHandler(), idLogStatusTimer);

    // Default size of the status_panel for single inverter
    // status_panel = new wxPanel(this, -1, wxDefaultPosition, wxSize(100, 52), wxSUNKEN_BORDER);  // Single Row - Single Inverter
    status_panel = new wxPanel(this, -1, wxDefaultPosition, wxSize(100, 104), wxSUNKEN_BORDER); // Double Row - Dual Inverters

    graph_panel = new GraphPanel(this, wxDefaultPosition, wxSize(100,100));
    wxBoxSizer *frame_sizer = new wxBoxSizer(wxVERTICAL);
    frame_sizer->Add(graph_panel, 1, wxEXPAND);
    frame_sizer->Add(status_panel, 0, wxEXPAND);
    SetSizer(frame_sizer);

    MakeStatusPanel();
	// Maximize();

    dlg_setup = new DlgSetup(this);
    dlg_location = new DlgLocation(this);
    dlg_inverter = new DlgInverter(this);
    dlg_chart = new DlgChart(this);
    dlg_histogram = new DlgHistogram(this);
    dlg_extrareadings = new DlgExtraR(this);
    dlg_alarms = new DlgAlarms(this);
    dlg_retrieve_energy = new DlgRetrieveEnergy(this);
    dlg_pvoutput = new DlgPvoutput(this);
    dlg_settime = new DlgSetTime(this);
    dlg_coords = new DlgCoords(this);
    power_meter = new PowerMeter(this);

    pvoutput.failed = 0;

    memset(inverter_response, 0, sizeof(inverter_response));
    memset(dsp_data, 0, sizeof(dsp_data));

    for(inv=0; inv<N_INV; inv++)
    {
        inverters[inv].time_offset = INT_MAX;
        inverters[inv].need_time_offset = 1;
        power_total[inv] = 0;
        inverters[inv].logheaders_changed = 2;

        ShowEnergy(inv);
    }

    InitDates();
    OpenLogFiles(-2, 0);  // Read today's data if it exists (call before InitCharts() in order to set data_system_dir)
    InitCharts();
    InitCharts2();
    SetupStatusPanel(1);   // call this after InitCharts()

    // Get current position and size of main form once move has stopped.
    mainframe->MainPosX = GetPosition().x;
    mainframe->MainPosY = GetPosition().y;
    mainframe->MainWidth = GetSize().GetWidth();
    mainframe->MainHeight = GetSize().GetHeight();

    InitComms();
    CheckEnergyLog();

	startuptimer.Start(500, wxTIMER_ONE_SHOT);

    SendCommand(0,5);  // start the inverter commands
}

void Mainframe::OnStartupTimer(wxTimerEvent& WXUNUSED(event))
{//===========================================================
	graph_panel->enable_draw = 1;
	graph_panel->Refresh();
//	my_event.SetId(idRefreshGraph);
//  wxPostEvent(graph_panel->GetEventHandler(), my_event);
}

Mainframe::~Mainframe()
{//====================
    int ix;
    SerialClose();

    for(ix=0; ix<N_INV; ix++)
    {
        if(inverters[ix].f_log != NULL)
            fclose(inverters[ix].f_log);
    }
}

void Mainframe::OnClose(wxCloseEvent& WXUNUSED(event))
{
    Destroy();
}

#pragma endregion
