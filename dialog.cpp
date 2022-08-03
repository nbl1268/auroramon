
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


#include <wx/cmndata.h>
#include <wx/colordlg.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include "auroramon.h"

extern void MakeInverterNames(int control);
extern wxArrayString InverterNames;


/************************************************************
                Setup Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgSetup, wxDialog)
	EVT_BUTTON(dlgOK, DlgSetup::OnButton)
	EVT_BUTTON(dlgCancel, DlgSetup::OnButton)
END_EVENT_TABLE()

#define N_DATE_FORMATS  4
wxString date_formats[N_DATE_FORMATS] = {_T("2012-12-31"), _T("Local"), _T("31 Dec 2012"), _T("31.12.2012")};

DlgSetup::DlgSetup(wxWindow *parent)
    : wxDialog(parent, -1, _T("Setup"), wxDefaultPosition, wxSize(320,280+DLG_HX))
{//=================================
    int y;
    int x;
    x = 96;
    y = 22;
    t_serialport = new wxTextCtrl(this, dlgSerialPort, serial_port, wxPoint(x, y), wxSize(200, 20));
    new wxStaticText(this, -1, _T("Serial port"), wxPoint(8, y+2));
    y = 62;
    t_inverter1 = new wxTextCtrl(this, dlgInverter1, wxString::Format(_T("%d"), inverter_address[0]), wxPoint(x+90,y), wxSize(40, 20));
    t_inverter2 = new wxTextCtrl(this, dlgInverter2, wxString::Format(_T("%d"), inverter_address[1]), wxPoint(x+130,y), wxSize(40, 20));
    new wxStaticText(this, -1, _T("Inverter addresses (0 if not used)"), wxPoint(8, y+2));
    y = 96;
    new wxStaticText(this, -1, _T("Data directory"), wxPoint(8, y));
    t_data_dir = new wxTextCtrl(this, dlgDataDir, data_dir, wxPoint(6, y+16), wxSize(306, 20));

    y = 144;
    t_date_format = new wxChoice(this, -1, wxPoint(x, y), wxDefaultSize, N_DATE_FORMATS, date_formats);
    new wxStaticText(this, -1, _T("Date format"), wxPoint(8, y+4));

    y = 240;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(130,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(220,y));
}

void DlgSetup::ShowDlg()
{//=====================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    t_serialport->ChangeValue(serial_port);

    if(inverter_address[0] > 0)
        t_inverter1->ChangeValue(wxString::Format(_T("%d"), inverter_address[0]));
    else
        t_inverter1->ChangeValue(_T(""));

    if(inverter_address[1] > 0)
        t_inverter2->ChangeValue(wxString::Format(_T("%d"), inverter_address[1]));
    else
        t_inverter2->ChangeValue(_T(""));

    t_data_dir->ChangeValue(data_dir);
    t_date_format->SetSelection(option_date_format);
    ShowModal();
}

void DlgSetup::OnButton(wxCommandEvent &event)
{//===========================================
    int id;
    long int value;
    wxString dir;
    wxString s;

    id = event.GetId();
    if(id == dlgOK)
    {
        serial_port = t_serialport->GetValue();
        value = 0;
        t_inverter1->GetValue().ToLong(&value);
        inverter_address[0] = value;
        t_inverter2->GetValue().ToLong(&value);
        inverter_address[1] = value;
        data_dir = t_data_dir->GetValue();

        option_date_format = t_date_format->GetSelection();

        mainframe->SetupStatusPanel(1);
        status_panel->Refresh();

        if(!wxDirExists(data_dir))
        {
            if(wxMessageBox(_T("Create directory '") + data_dir + _T("' ?"), _T("Setup"), wxYES_NO, this) != wxYES)
            {
                return;
            }
            wxMkdir(data_dir);
        }

//        CalcSun(graph_date_ymd);  // done in OpenLogFiles()
//        graph_panel->Refresh();  // done in OpenLogFiles()
        OpenLogFiles(-1, 0);  // because the inverter address may have changed
        ConfigSave(1);
        mainframe->MakeInverterMenu(1);  // remake the inverter menu

    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                Location Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgLocation, wxDialog)
	EVT_BUTTON(dlgOK, DlgLocation::OnButton)
	EVT_BUTTON(dlgCancel, DlgLocation::OnButton)
END_EVENT_TABLE()

wxString degrees_to_string(double degrees)
{//=======================================
    int mins;
    int secs;
    int deg;

    deg = (int)degrees;
    if(degrees < 0) degrees = -degrees;
    mins = (int)(degrees*60) % 60;
    secs = (int)(degrees*3600) % 60;
    return(wxString::Format(_T("%2d:%.2d:%.2d"), deg, mins, secs));
}

double string_to_degrees(wxString string)
{//======================================
    int deg = 0;
    int mins = 0;
    float secs = 0;
    double degrees = 0;
    int sign;
    char buf[200];

    strncpy(buf, string.mb_str(wxConvLocal), sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    if(strchr(buf,':') == NULL)
    {
        string.ToDouble(&degrees);
    }
    else
    {
        sscanf(buf, "%d:%d:%f", &deg, &mins, &secs);
        sign = 1;
        if(deg < 0)
        {
            sign = -1;
            deg = -deg;
        }
        degrees = sign * (deg + (double)mins / 60 + secs/3600);
    }
    return(degrees);
}

DlgLocation::DlgLocation(wxWindow *parent)
    : wxDialog(parent, -1, _T("Location"), wxDefaultPosition, wxSize(320,280+DLG_HX))
{//======================================
    int y;
    int x;
    int ix;
    x = 110;
    y = 22;

    new wxStaticText(this, -1, _T("Longitude"), wxPoint(8, y+3));
    new wxStaticText(this, -1, _T("Latitude"), wxPoint(8, y+25));
    new wxStaticText(this, -1, _T("Time Zone (hrs E)"), wxPoint(8, y+47));
    t_longitude = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y), wxSize(90, 20));
    t_latitude = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+22), wxSize(90, 20));
    t_timezone = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+44), wxSize(90, 20));
#ifdef __WXMSW__
    t_tz_auto = new wxCheckBox(this, -1, _T("auto"), wxPoint(x+92, y+46));
#else
    t_tz_auto = new wxCheckBox(this, -1, _T("auto"), wxPoint(x+90, y+41));
#endif

    y = 104;
    new wxStaticText(this, -1, _T("Panel Group"), wxPoint(8, y));
    new wxStaticText(this, -1, _T("Tilt"), wxPoint(8, y+20));
    new wxStaticText(this, -1, _T("Facing CW from N"), wxPoint(8, y+40));

    x = 120;
    for(ix=0; ix<N_PANEL_GROUPS; ix++)
    {
        new wxStaticText(this, -1, wxString(wxChar('A' + ix)), wxPoint(x+20, y));
        t_tilt[ix] = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+18), wxSize(50, 20), wxTE_CENTRE);
        t_facing[ix] = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+38), wxSize(50, 20), wxTE_CENTRE);
        x += 50;
    }

    y = 240;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(130,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(220,y));

}

void DlgLocation::ShowDlg()
{//=====================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    int ix;

    t_longitude->ChangeValue(degrees_to_string(Longitude));
    t_latitude->ChangeValue(degrees_to_string(Latitude));
    t_timezone->ChangeValue(wxString::Format(_T("%.2f"), Timezone));
    if(TimeZone2 > 24)
        t_tz_auto->SetValue(true);
    else
        t_tz_auto->SetValue(false);

    for(ix=0; ix<N_PANEL_GROUPS; ix++)
    {
        t_tilt[ix]->ChangeValue(wxString::Format(_T("%.1f"), panel_groups[ix].tilt));
        t_facing[ix]->ChangeValue(wxString::Format(_T("%.1f"), panel_groups[ix].facing));
    }
    ShowModal();
}

void DlgLocation::OnButton(wxCommandEvent &event)
{//==============================================
    int id;
    int ix;
    wxString dir;
    wxString s;

    id = event.GetId();
    if(id == dlgOK)
    {
        s = t_longitude->GetValue();
        if(s.Find(':') == wxNOT_FOUND)
        {
            Longitude = 0;
            s.ToDouble(&Longitude);
        }
        else
        {
            Longitude = string_to_degrees(s);
        }

        s = t_latitude->GetValue();
        if(s.Find(':') == wxNOT_FOUND)
        {
            Latitude = 0;
            s.ToDouble(&Latitude);
        }
        else
        {
            Latitude = string_to_degrees(s);
        }

        t_timezone->GetValue().ToDouble(&Timezone);
        if(t_tz_auto->GetValue() == true)
            TimeZone2 = 99;
        else
            TimeZone2 = Timezone;

        for(ix=0; ix<N_PANEL_GROUPS; ix++)
        {
            t_tilt[ix]->GetValue().ToDouble(&panel_groups[ix].tilt);
            t_facing[ix]->GetValue().ToDouble(&panel_groups[ix].facing);
        }
        ConfigSave(1);
        if(CalcSun(graph_date_ymd) < 0)
            return;
        graph_panel->Refresh();
    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                Inverter Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgInverter, wxDialog)
	EVT_BUTTON(dlgOK, DlgInverter::OnButton)
	EVT_BUTTON(dlgCancel, DlgInverter::OnButton)
END_EVENT_TABLE()

static wxString inverter_retrieve_choices[] = {
    _T("no"), _T("5 minute data"), _T("10 second data") };

DlgInverter::DlgInverter(wxWindow *parent)
    : wxDialog(parent, -1, _T("Inverter Settings"), wxDefaultPosition, wxSize(324,200+DLG_HX))
{//=================================
    int y;

    y = 24;
    t_retrieve = new wxCheckBox(this, -1, _T("Auto retrieve 5 minute energy data"), wxPoint(8, y));

    y = 160;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(134,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(224,y));

}

void DlgInverter::ShowDlg(int inv)
{//===============================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    inverter = inv;
    t_retrieve->SetValue(inverters[inv].auto_retrieve);
    ShowModal();
}

void DlgInverter::OnButton(wxCommandEvent &event)
{//==============================================
    int id;

    id = event.GetId();
    if(id == dlgOK)
    {
        inverters[inverter].auto_retrieve = t_retrieve->GetValue();
    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                Histogram Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgHistogram, wxDialog)
	EVT_BUTTON(dlgOK, DlgHistogram::OnButton)
	EVT_BUTTON(dlgCancel, DlgHistogram::OnButton)
END_EVENT_TABLE()

extern wxString BackColours[N_BACK_COLOURS];
double histogram_max;
int histogram_flags;

DlgHistogram::DlgHistogram(wxWindow *parent)
    : wxDialog(parent, -1, _T("Energy Histogram"), wxDefaultPosition, wxSize(324,200+DLG_HX))
{//=================================
    int y;

    y = 24;
    new wxStaticText(this, -1, _T("Maximum daily energy"), wxPoint(10, y));
    t_dailymax = new wxTextCtrl(this, -1, data_dir, wxPoint(136, y-3), wxSize(46, 20));
    new wxStaticText(this, -1, _T("kW"), wxPoint(184, y));

    y = 52;
    new wxStaticText(this, -1, _T("Background"), wxPoint(10, y+3));
    t_back_colour = new wxChoice(this, idBackColour, wxPoint(136, y-1), wxDefaultSize, N_BACK_COLOURS, BackColours);

    y = 160;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(134,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(224,y));

}

void DlgHistogram::ShowDlg()
{//=========================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    t_dailymax->ChangeValue(wxString::Format(_T("%4.1f"), histogram_max));
    t_back_colour->SetSelection(histogram_flags);
    ShowModal();
}

void DlgHistogram::OnButton(wxCommandEvent &event)
{//=============================================
    int id;

    id = event.GetId();
    if(id == dlgOK)
    {
        t_dailymax->GetValue().ToDouble(&histogram_max);
        histogram_flags = t_back_colour->GetSelection();
        ConfigSave(1);
        graph_panel->SetMode(-1);
        graph_panel->Refresh();
    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                Retrieve Daily Energy Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgRetrieveEnergy, wxDialog)
	EVT_BUTTON(dlgOK, DlgRetrieveEnergy::OnButton)
	EVT_BUTTON(dlgCancel, DlgRetrieveEnergy::OnButton)
END_EVENT_TABLE()

wxProgressDialog *progress_retrieval = NULL;

DlgRetrieveEnergy::DlgRetrieveEnergy(wxWindow *parent)
    : wxDialog(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(470,200+DLG_HX))
{//=================================
    int y;

    MakeInverterNames(1);

    y = 12;
    t_title = new wxStaticText(this, -1, wxEmptyString, wxPoint(10, y));

    y = 46;
    new wxStaticText(this, -1, _T("Write to"), wxPoint(10, y));
    t_path = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(5, y+16), wxSize(464, 20));

    y = 90;
    t_option1 = new wxCheckBox(this, -1, _T("5 minute data for PVOutput.org "), wxPoint(10, y));

    y = 160;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(280,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(370,y));

    option_verbose = 0;
    option_pvoutput = 0;
}

void DlgRetrieveEnergy::ShowDlg(int inv, int type)
{//===============================================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    wxString fname;
    wxString path;
    int addr;

    retrieve_type = type;
    retrieve_inverter = inv;
    addr = inverter_address[inv];

    SetTitle(wxString::Format(_T("Retrieve Data from Inverter #%d"), inverter_address[inv]));
    if(retrieving_data != 0)
    {
        wxLogError(_T("Data retrieval is already in progress"));
        return;
    }

    if(type == cmdInverterDailyEnergy)
    {
        t_option1->SetLabel(_T("Verbose"));
        t_option1->SetValue(option_verbose);
        t_title->SetLabel(_T("Retrieve daily energy data"));
#ifdef __WXMSW__
        fname = wxString::Format(_T("\\daily%d_"), addr);
#else
        fname = wxString::Format(_T("/daily%d_"), addr);
#endif
    }
    else
    if(type == cmdInverter10SecEnergy)
    {
        t_option1->SetLabel(_T("5 minute data for PVOutput.org"));
        t_option1->SetValue(option_pvoutput);
        t_title->SetLabel(_T("Retrieve 10 second energy data"));
#ifdef __WXMSW__
        fname = wxString::Format(_T("\\r%d_"), addr);
#else
        fname = wxString::Format(_T("/r%d_"), addr);
#endif
    }

    if(path_retrieve == wxEmptyString)
        path = data_dir;
    else
        path = path_retrieve;

    t_path->ChangeValue(path + fname + date_ymd_string + _T(".txt"));

    ShowModal();
}

void DlgRetrieveEnergy::OnButton(wxCommandEvent &event)
{//=============================================
    wxString path;

    if(event.GetId() == dlgOK)
    {
        path = wxFileName(t_path->GetValue()).GetPath();
        if(wxDirExists(path) == false)
        {
            wxLogError(_T("Directory not found:\n") + path);
            return;
        }
        path_retrieve = path;
        ConfigSave(1);

        retrieving_progress = 0;
        retrieving_finished = 0;
        retrieving_fname = t_path->GetValue();

        if(retrieve_type == cmdInverterDailyEnergy)
        {
            retrieve_message = _T("daily energy data");
            option_verbose = t_option1->GetValue();
        }
        else
        if(retrieve_type == cmdInverter10SecEnergy)
        {
            retrieve_message = _T("10 second energy data");
            option_pvoutput = t_option1->GetValue();
            retrieving_pvoutput = option_pvoutput;
        }

        if(QueueCommand(retrieve_inverter, retrieve_type) == 0)
        {
            progress_retrieval = new wxProgressDialog(_T("Aurora Monitor"), _T("Retrieving ") + retrieve_message, 100, mainframe, wxPD_CAN_ABORT | wxSTAY_ON_TOP);
        }
    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                Pvoutput Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgPvoutput, wxDialog)
	EVT_BUTTON(dlgOK, DlgPvoutput::OnButton)
	EVT_BUTTON(dlgCancel, DlgPvoutput::OnButton)
END_EVENT_TABLE()

PVOUTPUT pvoutput;
wxString pvoutput_choices[4] = {_T("Off"), _T("5 minutes"), _T("10 minutes"), _T("15 minutes")};

DlgPvoutput::DlgPvoutput(wxWindow *parent)
    : wxDialog(parent, -1, _T("PVOutput.org"), wxDefaultPosition, wxSize(470,220+DLG_HX))
{//=================================
    int y = 20;
    int x = 70;
    t_url = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y), wxSize(350, 20));
    t_system_id = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+26), wxSize(80, 20));
    t_api_key = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y+52), wxSize(350, 20));

    new wxStaticText(this, -1, _T("URL"), wxPoint(8, y+2));
    new wxStaticText(this, -1, _T("System Id"), wxPoint(8, y+28));
    new wxStaticText(this, -1, _T("API Key"), wxPoint(8, y+54));

    y += 86;
    t_live_updates = new wxChoice(this, -1, wxPoint(84, y), wxDefaultSize, 4, pvoutput_choices);
    new wxStaticText(this, -1, _T("Live updates"), wxPoint(8, y+4));
    t_instantaneous = new wxCheckBox(this, -1, _T("Include Instantaneous Power"), wxPoint(200, y-2));
    t_grid_voltage = new wxCheckBox(this, -1, _T("Include Grid Voltage"), wxPoint(200, y+18));
    t_temperature = new wxCheckBox(this, -1, _T("Include Inverter Temperature"), wxPoint(200, y+38));

    y = 180;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(280,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(370,y));
}

void DlgPvoutput::ShowDlg()
{//========================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    t_url->ChangeValue(pvoutput.url);
    t_system_id->ChangeValue(pvoutput.sid);
    t_api_key->ChangeValue(pvoutput.key);
    t_live_updates->SetSelection(pvoutput.live_updates & PVO_PERIOD);
    t_instantaneous->SetValue(pvoutput.live_updates & PVO_INSTANTANEOUS);
    t_grid_voltage->SetValue(pvoutput.live_updates & PVO_GRID_VOLTAGE);
    t_temperature->SetValue(pvoutput.live_updates & PVO_TEMPERATURE);
    ShowModal();
}

void DlgPvoutput::OnButton(wxCommandEvent &event)
{//==============================================

    if(event.GetId() == dlgOK)
    {
        pvoutput.url = t_url->GetValue();
        if(pvoutput.url.Right(1) != _T("/"))
            pvoutput.url += _T("/");
        pvoutput.sid = t_system_id->GetValue();
        pvoutput.key = t_api_key->GetValue();
        pvoutput.live_updates = t_live_updates->GetSelection();
        if(t_instantaneous->GetValue())
            pvoutput.live_updates |= PVO_INSTANTANEOUS;
        if(t_grid_voltage->GetValue())
            pvoutput.live_updates |= PVO_GRID_VOLTAGE;
        if(t_temperature->GetValue())
            pvoutput.live_updates |= PVO_TEMPERATURE;
        pvoutput.failed = 0;
        ConfigSave(1);
    }
    Show(false);
}

//=======================================================================================================================


/************************************************************
                SetTime Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgSetTime, wxDialog)
	EVT_BUTTON(dlgOK, DlgSetTime::OnButton)
	EVT_BUTTON(dlgCancel, DlgSetTime::OnButton)
    EVT_TIMER(idSetTimeTimer, DlgSetTime::OnTimer)
END_EVENT_TABLE()

DlgSetTime::DlgSetTime(wxWindow *parent)
    : wxDialog(parent, -1, _T("Set Time"), wxDefaultPosition, wxSize(470,220+DLG_HX))
{//=================================
    int y = 20;
    int x = 106;

    static wxString radio_names[2] = {_T("Set to specified time "), _T("Set to computer time ")};

    new wxStaticText(this, -1, _T("Computer time"), wxPoint(10, y));
    t_computer_date = new wxStaticText(this, -1, wxEmptyString, wxPoint(x+8, y));
    t_computer_time = new wxStaticText(this, -1, wxEmptyString, wxPoint(x+98, y));
    y += 24;
    new wxStaticText(this, -1, _T("Inverter time"), wxPoint(10, y));
    t_inverter_date = new wxStaticText(this, -1, wxEmptyString, wxPoint(x+8, y));
    t_inverter_time = new wxStaticText(this, -1, wxEmptyString, wxPoint(x+98, y));

    y += 20;
    t_radiobox = new wxRadioBox(this, -1, wxEmptyString, wxPoint(10, y), wxSize(350, -1), 2, radio_names);

    y += 16;
    x = 180;
    t_new_date = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x, y), wxSize(90, 20));
    t_new_time = new wxTextCtrl(this, -1, wxEmptyString, wxPoint(x+90, y), wxSize(70, 20));

    y = 180;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(280,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(370,y));

    settime_timer.SetOwner(this->GetEventHandler(), idSetTimeTimer);

}

void DlgSetTime::OnTimer(wxTimerEvent& WXUNUSED(event))
{//====================================================

    UpdateData();

}

void DlgSetTime::UpdateData()
{//====================================================

    wxDateTime datetime;
    time_t computer_time;
    time_t inverter_time;

    if(this->IsShown() == false)
    {
        settime_timer.Stop();
    }

    computer_time = time(NULL);
    datetime.Set(computer_time);
    t_computer_date->SetLabel(datetime.FormatISODate());
    t_computer_time->SetLabel(datetime.FormatISOTime());
    if(t_new_date->GetValue() == wxEmptyString)
        t_new_date->ChangeValue(datetime.FormatISODate());

    if(inverters[inverter].need_time_offset != 0)
    {
        t_inverter_time->SetLabel(wxEmptyString);
        if(inverters[inverter].need_time_offset < 0)
            t_inverter_date->SetLabel(_T("No response"));
        else
            t_inverter_date->SetLabel(_T("Waiting..."));
    }
    else
    {
        inverter_time = computer_time + inverters[inverter].time_offset;
        datetime.Set(inverter_time);
        t_inverter_date->SetLabel(datetime.FormatISODate());
        t_inverter_time->SetLabel(datetime.FormatISOTime());
    }
}

void DlgSetTime::ShowDlg(int inv)
{//==============================
    // move dialog to adjacent to mainframe
    Move(mainframe->MainPosX + 10, mainframe->MainPosY + 10);

    inverter = inv;
    UpdateData();
    settime_timer.Start(250, wxTIMER_CONTINUOUS);
    QueueCommand(inv, cmdGetInvTime);
    inverters[inv].need_time_offset = 2;  // pending
    ShowModal();
}

void DlgSetTime::OnButton(wxCommandEvent &event)
{//=============================================
    int action;
    int n1, n2;
    int year, month;
    struct tm btime;
    time_t new_time;
    char buf[80];

    if(event.GetId() == dlgOK)
    {
        action = t_radiobox->GetSelection();
        if(action == 0)
        {
            memset(&btime, 0, sizeof(btime));
            strncpy0(buf, t_new_date->GetValue().mb_str(wxConvLocal), sizeof(buf));
            n1 = sscanf(buf, "%d-%d-%d", &year, &month, &btime.tm_mday);
            btime.tm_year = year-1900;
            btime.tm_mon = month-1;

            strncpy0(buf, t_new_time->GetValue().mb_str(wxConvLocal), sizeof(buf));
            n2 = sscanf(buf, "%d:%d:%d", &btime.tm_hour, &btime.tm_min, &btime.tm_sec);
            new_time = mktime(&btime);

            if((n1 != 3) || (n2 != 3) || (new_time == (time_t)(-1)))
            {
                wxLogError(_T("Invalid date and time"));
                return;
            }
            settime_offset = new_time - time(NULL);
        }
        else
        {
            settime_offset = 0;
        }
        QueueCommand(inverter, cmdSetInvTime2);

    }
    settime_timer.Stop();
    Show(false);
}

//=======================================================================================================================
