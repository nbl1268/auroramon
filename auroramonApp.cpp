
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

#include <wx/fileconf.h>
#include "auroramon.h"


class auroraApp : public wxApp
{
    public:
        virtual bool OnInit();
        int OnExit();
};


IMPLEMENT_APP(auroraApp);

wxString path_home;
wxString serial_port;
wxString data_dir = wxGetCwd();
wxString data_system_dir = data_dir;
wxString data_year_dir = data_dir;
wxString data_year_dir2 = data_dir;
wxString path_retrieve;

// NOTE reads from 'aurora.ini' which is located in '%USERPROFILE%\AppData\Roaming' eg %APPDATA% folder
void ConfigInit()
{
    int ix;
    int inv;
    double *e;
    char buf[100];
    wxString string;
    wxFileConfig *pConfig = new wxFileConfig(_T("aurora"));     // this creates new 'default' config which has data path within... 
	wxFileConfig::Set(pConfig);
#ifdef __WXMSW__
	pConfig->Read(_T("/data"), &data_dir, path_home+_T("\\aurora"));
	pConfig->Read(_T("/serial_port"),&serial_port,_T("COM1"));
#else
	pConfig->Read(_T("/data"), &data_dir, path_home+_T("/aurora"));
	pConfig->Read(_T("/serial_port"),&serial_port,_T("/dev/ttyS0"));
#endif
	pConfig->Read(_T("/longitude"), &Longitude, 500);
	pConfig->Read(_T("/latitude"), &Latitude, 45);
	pConfig->Read(_T("/timezone"), &TimeZone2, 99);
	pConfig->Read(_T("/histogram"), &histogram_flags, (long)0);
	pConfig->Read(_T("/histogram_max"), &histogram_max, 25.0);
    pConfig->Read(_T("/logchange"), &date_logchange, wxEmptyString);
    pConfig->Read(_T("/energydate"), &string, wxEmptyString);
    strncpy0(ymd_energy, string.mb_str(wxConvLocal), sizeof(ymd_energy));
    pConfig->Read(_T("/alarmdate"), &string, wxEmptyString);
    strncpy0(ymd_alarm, string.mb_str(wxConvLocal), sizeof(ymd_alarm));

    pConfig->Read(_T("/retrievepath"), &path_retrieve, wxEmptyString);
    pConfig->Read(_T("/date_format"), &option_date_format, (long)0);

    pConfig->Read(_T("/extra_readings"), &string, _T("3001 2004"));  // default is Grid_V Grid_Hz
    SetExtraReadings(string);

    pConfig->SetPath(_T("/pvoutput"));
    pConfig->Read(_T("url"), &pvoutput.url, _T("http://pvoutput.org/service/r2/"));
    pConfig->Read(_T("systemid"), &pvoutput.sid, wxEmptyString);
    pConfig->Read(_T("apikey"), &pvoutput.key, wxEmptyString);
    pConfig->Read(_T("live_updates"), &pvoutput.live_updates, (long)0);

	for(inv=0; inv<N_INV; inv++)
	{
        memset(&inverters[inv], 0, sizeof(inverters[inv]));
	    pConfig->SetPath(wxString::Format(_T("/inverter%d"), inv));
	    if(inv==0)
            inverter_address[inv] = pConfig->Read(_T("address"), (long)2);
        else
            inverter_address[inv] = pConfig->Read(_T("address"), (long)0);
        inverters[inv].auto_retrieve = pConfig->Read(_T("auto_retrieve"), (long)0);
        pConfig->Read(_T("today"), &string, _T(""));
        strncpy0(inverters[inv].today_done, string.mb_str(wxConvLocal), sizeof(inverters[inv].today_done));
        pConfig->Read(_T("previous_total"), &(inverters[inv].previous_total), 0);
        pConfig->Read(_T("energy"), &string, wxEmptyString);
        strncpy0(buf, string.mb_str(wxConvLocal), sizeof(buf));
        e = inverters[inv].energy_total;
        sscanf(buf, "%lf %lf %lf %lf %lf %lf %d", &e[0], &e[1], &e[2], &e[3], &e[4], &e[5], &inverters[inv].peak);
	}

    for(ix=0; ix<N_PANEL_GROUPS; ix++)
    {
	    pConfig->SetPath(wxString::Format(_T("/panelgroup%d"), ix));
	    pConfig->Read(_T("tilt"), &panel_groups[ix].tilt, 0);
	    pConfig->Read(_T("facing"), &panel_groups[ix].facing, 0);
    }

}  // end of ConfigInit


void ConfigSave(int control)
{
    int ix;
    int inv;
    double *e;
    wxString string;
    long value;
	wxFileConfig *pConfig = (wxFileConfig *)(wxConfigBase::Get());

    pConfig->Write(_T("/data"), data_dir);
	pConfig->Write(_T("/serial_port"), serial_port);
	pConfig->Write(_T("/longitude"), Longitude);
	pConfig->Write(_T("/latitude"), Latitude);
	pConfig->Write(_T("/timezone"), TimeZone2);
	pConfig->Write(_T("/histogram"), histogram_flags);
	pConfig->Write(_T("/histogram_max"), histogram_max);
	pConfig->Write(_T("/logchange"), date_logchange);
	pConfig->Write(_T("/energydate"), wxString(ymd_energy, wxConvLocal));
	pConfig->Write(_T("/alarmdate"), wxString(ymd_alarm, wxConvLocal));
	pConfig->Write(_T("/retrievepath"), path_retrieve);
    pConfig->Write(_T("/date_format"), (long)option_date_format);

    string = wxEmptyString;

    for(ix=0; ix<N_EXTRA_READINGS; ix++)
    {
        value = extra_readings[ix].dsp_code;
        if(value > 0)
        {
            if(extra_readings[ix].graph_slot > 0)
                value += 1000;
            if(extra_readings[ix].status_slot > 0)
                value += 2000;
        }
        string += wxString::Format(_T("%d "), value);
    }
    pConfig->Write(_T("/extra_readings"), string);
    pConfig->SetPath(_T("/pvoutput"));
    pConfig->Write(_T("url"), pvoutput.url);
    pConfig->Write(_T("systemid"), pvoutput.sid);
    pConfig->Write(_T("apikey"), pvoutput.key);
    pConfig->Write(_T("live_updates"), pvoutput.live_updates);

	for(inv=0; inv<N_INV; inv++)
	{
	    e = inverters[inv].energy_total;
	    pConfig->SetPath(wxString::Format(_T("/inverter%d"), inv));
        pConfig->Write(_T("address"),inverter_address[inv]);
        pConfig->Write(_T("auto_retrieve"), inverters[inv].auto_retrieve);
        pConfig->Write(_T("today"), wxString(inverters[inv].today_done, wxConvLocal));
        if(inverter_address[inv] == 0)
        {
            inverters[inv].previous_total = 0;
            memset(e, 0, sizeof(inverters[inv].energy_total));
        }
        pConfig->Write(_T("previous_total"), inverters[inv].previous_total);
        pConfig->Write(_T("energy"), wxString::Format(_T("%f %f %f %f %f %f %d"),
                e[0],e[1],e[2],e[3],e[4],e[5], inverters[inv].peak));
//        pConfig->Write(_T("logperiod"), logging_period_index[inv]);
//        pConfig->Write(_T("logpath"), logging_path[inv]);
	}

    for(ix=0; ix<N_PANEL_GROUPS; ix++)
    {
	    pConfig->SetPath(wxString::Format(_T("/panelgroup%d"), ix));
	    pConfig->Write(_T("tilt"), panel_groups[ix].tilt);
	    pConfig->Write(_T("facing"), panel_groups[ix].facing);
    }


//	if(control == 0)
//		delete wxFileConfig::Set((wxFileConfig *)NULL);  // program exit
//    else
        pConfig->Flush();
}

bool auroraApp::OnInit()
{
#ifdef __WXMSW__
    wxString home_drive;
    wxString home_path;
    wxGetEnv(_T("HOMEPATH"), &home_path);
    wxGetEnv(_T("HOMEDRIVE"), &home_drive);
    path_home = home_drive + home_path;
#else
    wxGetEnv(_T("HOME"), &path_home);
#endif
    SetAppName(_T("Aurora Monitor"));
    ConfigInit();
    Mainframe* frame = new Mainframe(0L, _("Aurora"));
    frame->Show();

    return true;
}

int auroraApp::OnExit()
{
	ConfigSave(0);
	return(0);
}


void strncpy0(char *to, const char* from, int size)
{
    strncpy(to, from, size);
    to[size-1] = 0;
}
