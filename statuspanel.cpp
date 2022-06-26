
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

#include <wx/aboutdlg.h>
#include <wx/colordlg.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include "auroramon.h"


wxFont font_big(24, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
wxFont font_medium(19, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);

#ifdef __WXMSW__
#define TE_BIG   wxTE_RICH | wxTE_DONTWRAP
#define HT_PW0  33
#else
#define TE_BIG   wxTE_MULTILINE | wxTE_DONTWRAP
#define HT_PW0  34
#endif



void DrawStatusPulse(int pulse)
{//============================
    int state;
    int colour;
    int inv;
    int y;
    wxClientDC dc(status_panel);
    static wxColour red(0x0000d0);
    static wxColour red2(0x0010ff);
    static wxColour yellow(0x00a0c0);
    static wxColour yellow2(0x00d0ff);
    static wxColour green(0x087808);
    static wxColour green2(0x00e800);

    static int colour_on[3] =  {0x0020ff, 0x00e800, 0x12c0ff};  // red, green, yellow
    static int colour_off[3] = {0x0000c0, 0x087808, 0x00aacc};

    static int x = 124;
    static int y_inv[2] = {4, 26};

    for(inv = 0; inv < 2; inv++)
    {
        if(inverter_address[inv] > 0)
        {
            y = y_inv[inv];
            if((state = inverters[inv].alive) > 0)
            {
                if(inverters[inv].alarm != 0)
                    state = 2;
            }

            if(pulse & (1 << inv))
            {
                colour = colour_on[state];
                if(inverters[inv].comms_error)
                {
                    if(state == 1)
                        colour = 0x12c0ff;   // green, flashes yellow = retry
                    inverters[inv].comms_error = 0;
                }
                else
                if((retrieving_data > 0) && (retrieving_inv == inv))
                {
                    if(state == 1)
                        colour = 0xffe838;  // green, flashes cyan = retrieving data
                }
                dc.SetBrush(wxColour(colour));
                dc.DrawRectangle(x+1, y+1, 16, 16);
            }
            else
            {
                dc.SetBrush(wxColour(colour_off[state]));
                dc.DrawRectangle(x, y, 18, 18);
            }
        }
    }
}


void Mainframe::OnPulseTimer(wxTimerEvent& WXUNUSED(event))
{//==============================================
    DrawStatusPulse(0);
}


void Mainframe::OnLogStatusTimer(wxTimerEvent& WXUNUSED(event))
{//==============================================
    wxLogStatus(wxEmptyString);
}


void Mainframe::PlaceReadings(int type, int inv, int x, bool show)
{//===============================================================
// type: 1  Power output
// type: 2  Power, voltage, current, input. Temperature
// type: 3  Energy totals
// type: 4  Grid volts, frequency
// type: 5  Today energy, both inverters

    int ix;
    int posn;
    wxString label;

    if(type == 1)
    {
        if(show == true)
        {
            txt_power[inv]->SetSize(x, wxDefaultCoord, 82, HT_PW0);
            txt_static[inv][0]->SetSize(x+15, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
        }
        txt_power[inv]->Show(show);
        txt_static[inv][0]->Show(show);
    }
    if(type == 2)
    {
        if(show == true)
        {
            txt_power1[inv]->SetSize(x, wxDefaultCoord, 46, 20);
            txt_power2[inv]->SetSize(x, wxDefaultCoord, 46, 20);
            txt_volts1[inv]->SetSize(x+66, wxDefaultCoord, 41, 20);
            txt_volts2[inv]->SetSize(x+66, wxDefaultCoord, 41, 20);
            txt_current1[inv]->SetSize(x+126, wxDefaultCoord, 41, 20);
            txt_current2[inv]->SetSize(x+126, wxDefaultCoord, 41, 20);
            txt_efficiency[inv]->SetSize(x+186, wxDefaultCoord, 41, 20);
            txt_temperature[inv]->SetSize(x+186, wxDefaultCoord, 41, 20);
            txt_static[inv][2]->SetSize(x+49, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][3]->SetSize(x+49, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][4]->SetSize(x+110, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][5]->SetSize(x+110, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][6]->SetSize(x+170, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][7]->SetSize(x+170, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][8]->SetSize(x+230, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][9]->SetSize(x+230, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
        }

        txt_power1[inv]->Show(show);
        txt_power2[inv]->Show(show);
        txt_volts1[inv]->Show(show);
        txt_volts2[inv]->Show(show);
        txt_current1[inv]->Show(show);
        txt_current2[inv]->Show(show);
        txt_efficiency[inv]->Show(show);
        txt_temperature[inv]->Show(show);
        txt_static[inv][2]->Show(show);
        txt_static[inv][3]->Show(show);
        txt_static[inv][4]->Show(show);
        txt_static[inv][5]->Show(show);
        txt_static[inv][6]->Show(show);
        txt_static[inv][7]->Show(show);
        txt_static[inv][8]->Show(show);
        txt_static[inv][9]->Show(show);
    }
    if(type == 3)
    {
        if(show == true)
        {
            txt_energy[inv][0]->SetSize(x+24, 1, 53, 20);
            txt_energy[inv][1]->SetSize(x+24, wxDefaultCoord, 53, 20);
            txt_energy[inv][2]->SetSize(x+117, wxDefaultCoord, 53, 20);
            txt_energy[inv][3]->SetSize(x+117, wxDefaultCoord, 53, 20);
            txt_energy[inv][4]->SetSize(x+210, wxDefaultCoord, 53, 20);
            txt_energy[inv][5]->SetSize(x+210, wxDefaultCoord, 53, 20);
            txt_static[inv][10]->SetSize(x, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][11]->SetSize(x+79, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][12]->SetSize(x+79, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][13]->SetSize(x+172, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][14]->SetSize(x+172, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][15]->SetSize(x+265, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][16]->SetSize(x+265, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
        }

        txt_energy[inv][0]->Show(show);
        txt_energy[inv][1]->Show(show);
        txt_energy[inv][2]->Show(show);
        txt_energy[inv][3]->Show(show);
        txt_energy[inv][4]->Show(show);
        txt_energy[inv][5]->Show(show);
        txt_static[inv][10]->Show(show);
        txt_static[inv][11]->Show(show);
        txt_static[inv][12]->Show(show);
        txt_static[inv][13]->Show(show);
        txt_static[inv][14]->Show(show);
        txt_static[inv][15]->Show(show);
        txt_static[inv][16]->Show(show);
    }
    if(type == 4)
    {
        // any extra readings ?
        posn = 0;
        for(ix=0; ix < N_EXTRA_READINGS; ix++)
        {
            if(extra_readings[ix].status_slot != 0)
            {
                txt_dsp_param[inv][posn]->SetSize(x+92*(posn/2), wxDefaultCoord, 46, 20);
                txt_dsp_label[inv][posn]->SetSize(x+48+92*(posn/2), wxDefaultCoord, wxDefaultCoord, wxDefaultCoord);
                label = wxString(extra_reading_types[extra_readings[ix].type].mnemonic, wxConvLocal);
                label.Replace(_T("_"), _T(" "));
                txt_dsp_label[inv][posn]->SetLabel(label);
                txt_dsp_param[inv][posn]->Show(show);
                txt_dsp_label[inv][posn]->Show(show);

                extra_readings[ix].status_slot = posn+1;
                posn++;
            }
        }
        while(posn < N_EXTRA_READINGS)
        {
            txt_dsp_param[inv][posn]->Show(false);
            txt_dsp_label[inv][posn]->Show(false);
            posn++;
        }
    }
    if(type == 5)
    {
        if(show == true)
        {
            txt_energy[0][0]->SetSize(x,  1, 52, 20);
            txt_energy[1][0]->SetSize(x, 23, 52, 20);
        }
        txt_energy[0][0]->Show(show);
        txt_energy[1][0]->Show(show);
    }
}  // end of PlaceReadings


void Mainframe::SetupStatusPanel(int control)
{//=========================================
// control bit 0: inverter addresses may have changed

// style 0:  show all
//         1:  show some
//         2:  show all inverter 0
//         3:  show all inverter 1

    int ix;
    int style;

    if(control & 1)
    {
        for(ix=0; ix<N_INV; ix++)
        {
            if(inverter_address[ix] == 0)
            {
                power_total[ix] = 0;
                inverters[ix].alive = 0;
            }
            else
            {
                txt_static[ix][0]->SetLabel(wxString::Format(_T("Inverter #%d"), inverter_address[ix]));
            }
        }
    }

    if(control & 2)
    {
        for(ix=0; ix < N_EXTRA_READINGS; ix++)
        {
            txt_dsp_param[0][ix]->Clear();
            txt_dsp_param[1][ix]->Clear();
        }
    }

    style = chart_pages[graph_panel->page].status_type;

    if(inverter_address[1] == 0)
    {
        // only one inverter;
        PlaceReadings(1, 0,   0, false);
        PlaceReadings(1, 1,   0, false);
        PlaceReadings(2, 0, 149, true);
        PlaceReadings(2, 1,   0, false);
        PlaceReadings(3, 0, 423, true);
        PlaceReadings(3, 1,   0, false);
        PlaceReadings(4, 0, 749, true);
        PlaceReadings(4, 1,   0, false);
    }
    else
    {
        PlaceReadings(1, 0, 149, true);
        if(style == 0)
        {
            // all
            PlaceReadings(2, 0, 239, true);
            PlaceReadings(1, 1, 518, true);
            PlaceReadings(2, 1, 608, true);
            PlaceReadings(3, 0, 887, true);
            PlaceReadings(3, 1, 1207, true);
            PlaceReadings(4, 0, 1527, true);
            PlaceReadings(4, 1,   0, false);
        }
        if(style == 1)
        {
            // some
            PlaceReadings(2, 0, 239, true);
            PlaceReadings(1, 1, 509, true);
            PlaceReadings(2, 1, 599, true);
            PlaceReadings(3, 0,   0, false);
            PlaceReadings(3, 1,   0, false);
            PlaceReadings(4, 0, 931, true);
            PlaceReadings(4, 1,   0, false);
            PlaceReadings(5, 0, 861, true);
        }
        if(style == 2)
        {
            // inverter A
            PlaceReadings(2, 0, 239, true);
            PlaceReadings(1, 1, 831, true);
            PlaceReadings(2, 1,   0, false);
            PlaceReadings(3, 0, 511, true);
            PlaceReadings(3, 1,   0, false);
            PlaceReadings(4, 0, 931, true);
            PlaceReadings(4, 1,   0, false);
        }
        if(style == 3)
        {
            // inverter B
            PlaceReadings(2, 0,   0, false);
            PlaceReadings(1, 1, 259, true);
            PlaceReadings(2, 1, 349, true);
            PlaceReadings(3, 0,   0, false);
            PlaceReadings(3, 1, 611, true);
            PlaceReadings(4, 0,   0, false);
            PlaceReadings(4, 1, 931, true);
        }
    }

    for(ix=0; ix<2; ix++)
    {
        ShowEnergy(ix);
        txt_energy[ix][1]->ChangeValue(wxString::Format(_T("%d"), inverters[ix].peak));
    }
}


void Mainframe::MakeStatusPanel(void)
{//==================================
    int ix;
    int inv;
    int y;

    txt_powertot = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0,2), wxSize(116,42), TE_BIG | wxTE_READONLY | wxTE_CENTRE);
    txt_powertot->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font_big));

    for(inv=0; inv<2; inv++)
    {
        txt_power[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0,1), wxSize(82,HT_PW0), TE_BIG | wxTE_READONLY | wxTE_CENTRE);
        txt_power[inv]->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font_medium));
        txt_static[inv][0] = new wxStaticText(status_panel, -1, _T(""), wxPoint(0, 33));

        txt_power1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_power2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_volts1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_volts2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_current1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_current2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_efficiency[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_temperature[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);

        txt_static[inv][2] = new wxStaticText(status_panel, -1, _T("W"), wxPoint(0, 4));
        txt_static[inv][3] = new wxStaticText(status_panel, -1, _T("W"), wxPoint(0, 26));
        txt_static[inv][4] = new wxStaticText(status_panel, -1, _T("V"), wxPoint(0, 4));
        txt_static[inv][5] = new wxStaticText(status_panel, -1, _T("V"), wxPoint(0, 26));
        txt_static[inv][6] = new wxStaticText(status_panel, -1, _T("A"), wxPoint(0, 4));
        txt_static[inv][7] = new wxStaticText(status_panel, -1, _T("A"), wxPoint(0, 26));
        txt_static[inv][8] = new wxStaticText(status_panel, -1, _T("%"), wxPoint(0, 4));
        txt_static[inv][9] = new wxStaticText(status_panel, -1, _T("deg"), wxPoint(0, 26));

        txt_static[inv][10] = new wxStaticText(status_panel, -1, _T("kWh"), wxPoint(0, 4));
        for(ix=0; ix<6; ix++)
        {
            static const wxString energy_names[] = {_T("today"), _T("peakW"), _T("month"), _T("year"), _T("total"), _T("partial")};
            y = 1;
            if(ix & 1)
                y = 23;
            txt_energy[inv][ix] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, y), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
            txt_static[inv][ix+11] = new wxStaticText(status_panel, -1, energy_names[ix], wxPoint(0, y+3));
       }

        for(ix=0; ix<N_EXTRA_READINGS; ix++)
        {
            y = 1;
            if(ix & 1)
                y = 23;
            txt_dsp_param[inv][ix] = new wxTextCtrl(status_panel, -1, wxEmptyString,  wxPoint(0, y), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
            txt_dsp_label[inv][ix] = new wxStaticText(status_panel, -1, wxEmptyString, wxPoint(0, y+3));
        }
    }

}  // end of MakeStatusPanal

