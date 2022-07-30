
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

#include "auroramon.h"

#define N_GLOBAL_STATES   102
#define N_INVERTER_STATES  48
#define N_DCDC_STATES      20
#define N_ALARM_STATES     65

#pragma region Declarations

static const char *GlobalStates[N_GLOBAL_STATES] = {
"Sending Parameters",
"Wait Sun/Grid",
"Checking Grid",
"Measuring Riso",
"DcDc Start",
"Inverter Turn-On",
"Run",
"Recovery",
"Pause",
"Ground Fault",
"OTH Fault",                // 10
"Address Setting",
"Self Test",
"Self Test Fail",
"Sensor Test + Measure Riso",
"Leak Fault",
"Waiting for manual reset",
"Internal Error E026",
"Internal Error E027",
"Internal Error E028",
"Internal Error E029",      // 20
"Internal Error E030",
"Sending Wind Table",
"Failed Sending Table",
"UTH Fault",
"Remote Off",
"Interlock Fail",
"Executing Autotest",
NULL,
NULL,
"Waiting Sun",              // 30
"Temperature Fault",
"Fan Stuck",
"Int. Com. Fail",
"Slave Insertion",
"DC Switch Open",
"TRAS Switch Open",
"MASTER Exclusion",
"Auto Exclusion",
NULL,						// 39 These are all placeholders for the last 4
NULL,						// 40
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,                       // 50
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,						// 60
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,						// 70
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,						// 80
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,						// 90
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
"Erasing Internal EEprom",  // 98
"Erasing External EEprom",	// 99 was Erasing EEPROM */
"Counting EEprom",		    // 100
"Freeze"					// 101
};

static const char *InverterStates[N_INVERTER_STATES] = {
"Stand By",                 // 0
"Checking Grid",            // 1
"Run",                      // 2
"Bulk Over Voltage",        // 3
"Out Over Current",         // 4
"IGBT Sat",                 // 5
"Bulk Under Voltage",       // 6
"Degauss Error",            // 7
"No Parameters",            // 8
"Bulk Low",                 // 9
"Grid Over Voltage",        // 10
"Communication Error",      // 11
"Degaussing",               // 12
"Starting",                 // 13
"Bulk Cap Fail",            // 14
"Leak Fail",                // 15
"DcDc Fail",                // 16
"I-leak Sensor Fail",       // 17
"SelfTest: relay inverter", // 18
"SelfTest: wait for sensor test",  // 19
"SelfTest: test relay DcDc + sensor",  // 20
"SelfTest: relay inverter fail",  // 21
"SelfTest: timeout fail",   // 22
"SelfTest: relay DcDc fail", // 23
"Self Test 1",              // 24
"Waiting self test start",  // 25
"Dc Injection",             // 26
"Self Test 2",              // 27
"Self Test 3",              // 28
"Self Test 4",              // 29
"Internal Error (30)",      // 30
"Internal Error (31)",      // 31
NULL,						// 32
NULL,						// 33
NULL,						// 34
NULL,						// 35
NULL,						// 36
NULL,						// 37
NULL,						// 38
NULL,						// 39
"Forbidden State",          // 40
"Input UC",                 // 41
"Zero Power",               // 42
"Grid Not Present",	        // 43
"Waiting Start",			// 44
"MPPT",					    // 45
"Grid Fail",				// 46
"Input OC", 			    // 47
};

static const char *DcDcStates[N_DCDC_STATES] = {
"DcDc OFF",					// 0
"Ramp Start",			    // 1
"MPPT",					    // 2
"not used",					// 3
"Input Over Current",		// 4
"Input Under Voltage",		// 5
"Input Over Voltage",		// 6
"Input Low",				// 7
"No Parameters",			// 8
"Bulk Over Voltage",		// 9
"Communication Error",		// 10
"Ramp Fail",				// 11
"Internal Error",			// 12
"Input mode Error",			// 13
"Ground Fault",				// 14
"Inverter Fail",			// 15
"DcDc IGBT Sat",			// 16
"DcDc ILEAK Fail",			// 17
"DcDc Grid Fail",			// 18
"DcDc Comm. Error",			// 19
};

static char alarm_types[N_ALARM_STATES] = {
    0, 0, 2, 0, 2, 0, 2, 2, 2, 2,
    2, 0, 2, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 1, 1, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2};

static const char *AlarmStates[N_ALARM_STATES] = {
"No Alarm",					/* #  0 */
"Sun Low W001 (1)",				/* #  1 */
"Input Over Current E001",			/* #  2 */
"Input Under Voltage W002",			/* #  3 */
"Input Over Voltage E002",			/* #  4 */
"Sun Low W001 (5)",				/* #  5 */
"No Parameters E003",				/* #  6 was Internal error E003 */
"Bulk Over Voltage E004",			/* #  7 */
"Comm. Error E005",				/* #  8 was Internal error E005 */
"Output Over Current E006",			/* #  9 */
"IGBT Sat E007",				/* # 10 was Internal error E007 */
"Sun Low - bulk undervoltage W011",				/* # 11 was Internal error E008 */
"Internal error E009",				/* # 12 was Internal error E009 */
"Grid Fail W003",				/* # 13 */
"Bulk Low E010",				/* # 14 was Internal error E010 */
"Ramp Fail E011",				/* # 15 was Internal error E011 */
"Dc/Dc Fail E012",				/* # 16 */
"Wrong Mode E013",				/* # 17 */
"Ground Fault (18)",				/* # 18 */
"Over Temp. E014",				/* # 19 */
"Bulk Cap Fail E015",				/* # 20 */
"Inverter Fail E016",				/* # 21 */
"Start Timeout E017",				/* # 22 was Internal error E017 */
"Ground Fault E018 (23)",			/* # 23 */
"Degauss error (24)",				/* # 24 */
"I-leak Sens. fail E019",			/* # 25 was Internal error E019 */
"DcDc Fail E012",				/* # 26 */
"Self  Test Error 1 E020",			/* # 27 was Internal error E020 */
"Self  Test Error 2 E021",			/* # 28 was Internal error E021 */
"Self  Test Error 3 E019",			/* # 29 was Internal error E019 */
"Self  Test Error 4 E022",			/* # 30 was Internal error E022 */
"DC inj error E023",				/* # 31 was Internal error E023 */
"Grid Over Voltage W004",			/* # 32 */
"Grid Under Voltage W005",			/* # 33 */
"Grid OF W006",				    /* # 34 */
"Grid UF W007",				    /* # 35 */
"Z grid Hi W008",				/* # 36 */
"Internal error E024",		    /* # 37 */
"Riso Low E025",				/* # 38 */
"Vref Error E026",				/* # 39 was Internal error E026 */
"Error Meas V E027",				/* # 40 was Internal error E027 */
"Error Meas F E028",				/* # 41 was Internal error E028 */
"Error Meas I E029",				/* # 42 was Internal error E029 */
"Error Meas Ileak E030",			/* # 43 was Internal error E030 */
"Read Error V E031",				/* # 44 was Internal error E031 */
"Read Error I E032",				/* # 45 was Internal error E032 */
"Table fail W009",				/* # 46 was Empty Wind Table W009 */
"Fan Fail W010",				/* # 47 */
"UTH E033",					     /* # 48 was Internal error E033 */
"Interlock Fail",				/* # 49 */
"Remote Off",					/* # 50 */
"Vout Avg error",				/* # 51 */
"Battery low",					/* # 52 */
"Clk fail",					    /* # 53 */
"Input UC",					    /* # 54 */
"Zero Power",					/* # 55 */
"Fan Stuck",					/* # 56 */
"DC Switch Open",				/* # 57 */
"Tras Switch Open",				/* # 58 */
"AC Switch Open",				/* # 59 */
"Bulk UV",					    /* # 60 */
"Autoexclusion",				/* # 61 */
"Grid df/dt",					/* # 62 */
"Den switch Open",				/* # 63 */
"Jbox fail",					/* # 64 */
};

const char **message_array[5] = {
    GlobalStates, InverterStates, DcDcStates, DcDcStates, AlarmStates };

const int message_range[5] = {
    N_GLOBAL_STATES, N_INVERTER_STATES, N_DCDC_STATES, N_DCDC_STATES, N_ALARM_STATES};

static int keep_alarm_display = 0;
static unsigned long time_last_alarm = 0;

BEGIN_EVENT_TABLE(DlgAlarms, wxDialog)
	EVT_BUTTON(-1, DlgAlarms::OnButton)
END_EVENT_TABLE()

DlgAlarms *dlg_alarms = NULL;

static const wxString labels[5] = {_T("Global state"), _T("Inverter state"), _T("Chnl 1 DC/DC"), _T("Chnl 2 DC/DC"), _T("Alarm state")};

#pragma endregion


// Done 19/07/2022 - fix form size to show information for multiple inverters
// FUTURE add scrollbar to support more then 2 inverters
// https://forums.wxwidgets.org/viewtopic.php?t=44435


DlgAlarms::DlgAlarms(wxWindow *parent)
    : wxDialog(parent, -1, _T("Inverter Status"), wxPoint(0, 116), wxSize(320,280+DLG_HX))
{//====================================
    int x, y;
    int inv;
    int ix;

    x = 100;
    y = 0;

    for(inv=0; inv < 2; inv++)
    {
        inverter_name[inv] = new wxStaticText(this, -1, wxEmptyString, wxPoint(0, 0));
        for(ix=0; ix<5; ix++)
        {
            label[inv][ix] = new wxStaticText(this, -1, labels[ix], wxPoint(8, y));
            message[inv][ix] = new wxStaticText(this, -1, wxEmptyString, wxPoint(x, y), wxSize(100, 200));
            y += 16;
        }
    }
}

void DlgAlarms::Layout(int which_inverter)
{//=======================================
    // which_inverter:  bit 0,  show data for inverter 0
    //                  bit 1,  show data for inverter 1
    int y;
    int ix;
    int inv;
    int show[2] = { 0,0 };

    if(inverter_address[1] == 0)
    {
        // only one inverter
        // SetSize(0, 116, 320, 86+DLG_HX);
        SetSize(0, 116, 320, 106+DLG_HX);
        y =4;
        for(ix=0; ix<5; ix++)
        {
            label[0][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            message[0][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            y += 16;

            label[0][ix]->Show(true);
            message[0][ix]->Show(true);
            label[1][ix]->Show(false);
            message[1][ix]->Show(false);
        }
        inverter_name[0]->Show(false);
        inverter_name[1]->Show(false);
        return;
    }

    for(inv=0; inv<2; inv++)
    {
        show[inv] = 0;
        if((inverter_address[inv] != 0) && (inverters[inv].alarm || (which_inverter & (1 << inv))))
            show[inv] = 1;
        inverter_name[inv]->SetLabel(wxString::Format(_T("Inverter #%d"), inverter_address[inv]));
        inverter_name[inv]->Show(true);
    }

    // Set form size for number of inverters configured
    if(show[0] && show[1])
        SetSize(0, 116, 320, 219+DLG_HX);
    else
        SetSize(0, 116, 320, 106+DLG_HX);

    y = 0;
    inverter_name[0]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
    y += 16;
    if(show[0])
    {
        for(ix=0; ix<5; ix++)
        {
            label[0][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            message[0][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            y += 16;

            label[0][ix]->Show(true);
            message[0][ix]->Show(true);
        }
        y += 4;
    }
    else
    {
        for(ix=0; ix<5; ix++)
        {
            label[0][ix]->Show(false);
            message[0][ix]->Show(false);
        }
    }

    inverter_name[1]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
    if(show[1])
    {
        y += 16;
        for(ix=0; ix<5; ix++)
        {
            label[1][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            message[1][ix]->SetSize(wxDefaultCoord, y, wxDefaultCoord, wxDefaultCoord);
            y += 16;

            label[1][ix]->Show(true);
            message[1][ix]->Show(true);
        }
    }
    else
    {
        for(ix=0; ix<5; ix++)
        {
            label[1][ix]->Show(false);
            message[1][ix]->Show(false);
        }
    }
}

void DlgAlarms::Close(void)
{//========================
    keep_alarm_display = 0;
    Show(false);
}

void DlgAlarms::OnButton(wxCommandEvent &event)
{//===========================================
    int id;

    id = event.GetId();
    if(id == dlgOK)
    {
    }
    else
    {
    }
    Close();
}

void DisplayState(int inv)
{//=======================
    int n;
    int ix;
    const char *p;
    INVERTER_RESPONSE *ir;
    char buf[100];

    if(inverter_address[inv] == 0)
        return;

    ir = &inverter_response[inv];

    if(ir->state[0] == 0xff)
    {
        // not yet received state data
        for(ix=0; ix<5; ix++)
        {
            dlg_alarms->message[inv][ix]->SetLabel(wxEmptyString);
        }
        return;
    }
    for(ix=0; ix<5; ix++)
    {
        p = NULL;
        if((n = ir->state[ix]) < message_range[ix])
        {
            p = message_array[ix][n];
        }
        if(p == NULL)
        {
            sprintf(buf, "code %d", n);
            p = buf;
        }
        dlg_alarms->message[inv][ix]->SetLabel(wxString(p, wxConvLocal));
    }
}

void GotInverterStatus(int inv)
{//============================
    INVERTER_RESPONSE *ir;
    int prev_state;
    int alarm;
    time_t now;
    struct tm *btime;
    FILE *f_out;
    wxString fname;
    const char *p;
    const char *alarm_type;
    char buf[20];
    char buf_date[20];
    static int prev_alarm[N_INV] = {0, 0};
    static time_t prev_alarm_time[N_INV] = {0, 0};
    static time_t prev_alarm_state[N_INV] = {0, 0};

    ir = &inverter_response[inv];
    alarm = ir->state[4];

    if(ir->state[0] == 0xff)
    {
        // not yet received state data
        inverters[inv].alarm = 0;
        return;
    }

    if((inverters[0].alarm == 0) && (inverters[1].alarm == 0))
        prev_state = 0;
    else
        prev_state = 1;

    if((ir->state[0] == GLOBAL_STATE_RUN) && (ir->state[1] == INVERTER_STATE_RUN) && (alarm == 0) &&
        ((ir->state[2] == 2) || (ir->state[2] == 3)) && ((ir->state[3] == 2) || (ir->state[3] == 3)))
    {
        inverters[inv].alarm = 0;
    }
    else
    {
        inverters[inv].alarm = 1;

        if((alarm != 0) && (alarm != prev_alarm_state[inv]))
        {
            // the alarm indication has changed (and is on)
            time(&now);
            if((alarm != prev_alarm[inv]) || (difftime(now, prev_alarm_time[inv]) > 300))
            {
                // a different alarm, or more than 5 minutes since the previous alarm
                fname = data_system_dir + wxString::Format(_T("/alarms%d_%d.txt"), inv, inverter_address[inv]);
                if((f_out = fopen(fname.mb_str(wxConvLocal), "a")) != NULL)
                {
                    btime = localtime(&now);

                    alarm_type = "E";
                    if((alarm > 0) && (alarm < message_range[4]))
                    {
                        p = message_array[4][alarm];
                        if(alarm_types[alarm] == 0)
                            alarm_type = "";
                        else
                        if(alarm_types[alarm] == 1)
                            alarm_type = "W";
                    }
                    else
                    {
                        sprintf(buf, "?? Alarm %d", alarm);
                        p = buf;
                    }
                    sprintf(buf_date, "%.4d-%.2d-%.2d", btime->tm_year+1900, btime->tm_mon+1, btime->tm_mday);
                    if(strcmp(buf_date, ymd_alarm) != 0)
                    {
                        // the first alarm of a new date, add a blank line
                        fputc('\n', f_out);
                        strncpy0(ymd_alarm, buf_date, sizeof(ymd_alarm));
                    }
                    fprintf(f_out, "%s\t%s  %.2d:%.2d:%.2d   %s\n", alarm_type, buf_date, btime->tm_hour, btime->tm_min, btime->tm_sec, p);
                    fclose(f_out);
                }
            }
            prev_alarm_time[inv] = now;
            prev_alarm[inv] = alarm;
        }
    }
    prev_alarm_state[inv] = alarm;

    if((inverters[0].alarm == 0) && (inverters[1].alarm == 0))
    {
        if(dlg_alarms->IsShown())
        {
            DisplayState(inv);
            if(keep_alarm_display == 0)
            {
                if((wxGetLocalTime() - time_last_alarm) > 20)
                    dlg_alarms->Show(false);
            }
        }
    }
    else
    {
        if(prev_state == 0)
        {
            wxBell();  // previously there was no alarm
        }
        dlg_alarms->Layout(1 << inv);
        DisplayState(inv);
        dlg_alarms->Show(true);

        time_last_alarm = wxGetLocalTime();
    }
}

void ShowInverterStatus()
{//======================
    dlg_alarms->Layout(3);
    DisplayState(0);
    DisplayState(1);

    keep_alarm_display = 1;  // don't remove the display automatically
    dlg_alarms->Show(true);
}

void PruneAlarmFile(int inv)
{//=========================
    FILE *f_in;
    FILE *f_out;
    wxString fname;
    wxString fname2;
    int count;
    int skip;
    char buf[200];

    fname = data_system_dir + wxString::Format(_T("/alarms%d_%d.txt"), inverter_address[inv]);
    if((f_in = fopen(fname.mb_str(wxConvLocal), "r")) == NULL)
        return;

    count = 0;
    while(fgets(buf, sizeof(buf), f_in) != NULL)
    {
        // count the number of lines in the alarm file
        count++;
    }

    if(count <= 250)
    {
        // not too many
        fclose(f_in);
        return;
    }
    skip = count - 220;

    fname2 = data_dir + _T("/alarm_tmp");
    if((f_out = fopen(fname2.mb_str(wxConvLocal), "w")) == NULL)
    {
        fclose(f_in);
        return;
    }

    count = 0;
    rewind(f_in);
    while(fgets(buf, sizeof(buf), f_in) != NULL)
    {
        if(count++ < skip)
            continue;

        fputs(buf, f_out);
    }
    fclose(f_in);
    fclose(f_out);
    wxRenameFile(fname2, fname, true);
}
