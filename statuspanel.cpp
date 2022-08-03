
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

#pragma region Declarations

wxFont font_big(24, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
wxFont font_medium(19, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);

#ifdef __WXMSW__
#define TE_BIG   wxTE_RICH | wxTE_DONTWRAP
#define HT_PW0  33
#else
#define TE_BIG   wxTE_MULTILINE | wxTE_DONTWRAP
#define HT_PW0  34
#endif

#pragma endregion

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

// Done 03/08/2022 review layout, re-arrange so that inverter 2 readings are shown below 'stacked' (inverter 1 readings) rather than to the right side
//   replaced wxDefaultCoord for Y axis on all SetSize commands
//   Added new sytle 'All (v)' to allow inverter data in status panel to be 'stacked' vertically
//   Added PlaceReadings instuction to show extra readings fields for second inverter
// FUTURE Maybe make as scrollable field to allow for multiple inverters

void Mainframe::PlaceReadings(int type, int inv, int x, int y, bool show)
{//===============================================================
// type: 1  Inverter power output and Name
// type: 2  Inverter power, voltage, current, input. Temperature
// type: 3  Inverter energy totals
// type: 4  Inverter extra Readings eg Grid volts, frequency
// type: 5  Today energy, both inverters

    int ix;
    // y = wxDefaultCoord;
    int posn;
    wxString label;

    // type: 1  Inverter power output and Name
    if(type == 1)
    {
        if(show == true)
        {
            txt_power[inv]->SetSize(x, y, 82, HT_PW0);
            txt_static[inv][0]->SetSize(x+15, y+33, wxDefaultCoord, wxDefaultCoord);
        }
        txt_power[inv]->Show(show);
        txt_static[inv][0]->Show(show);
    }
    
    // type: 2  Inverter power, voltage, current, input. Temperature
    if(type == 2)
    {
        if(show == true)
        {
            txt_power1[inv]->SetSize(x, y, 46, 20);
            txt_power2[inv]->SetSize(x, y + 22, 46, 20);
            txt_volts1[inv]->SetSize(x+66, y, 41, 20);
            txt_volts2[inv]->SetSize(x+66, y + 22, 41, 20);
            txt_current1[inv]->SetSize(x+126, y, 41, 20);
            txt_current2[inv]->SetSize(x+126, y + 22, 41, 20);
            txt_efficiency[inv]->SetSize(x+186, y, 41, 20);
            txt_temperature[inv]->SetSize(x+186, y + 22, 41, 20);
            txt_static[inv][2]->SetSize(x+49, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][3]->SetSize(x+49, y + 22, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][4]->SetSize(x+110, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][5]->SetSize(x+110, y + 22, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][6]->SetSize(x+170, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][7]->SetSize(x+170, y + 22, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][8]->SetSize(x+230, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][9]->SetSize(x+230, y + 22, wxDefaultCoord, wxDefaultCoord);
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
    
    // type: 3  Inverter energy totals
    if(type == 3)
    {
        if(show == true)
        {
            txt_energy[inv][0]->SetSize(x+24, y, 53, 20);
            txt_energy[inv][1]->SetSize(x+24, y + 22, 53, 20);
            txt_energy[inv][2]->SetSize(x+117, y, 53, 20);
            txt_energy[inv][3]->SetSize(x+117, y + 22, 53, 20);
            txt_energy[inv][4]->SetSize(x+210, y, 53, 20);
            txt_energy[inv][5]->SetSize(x+210, y + 22, 53, 20);
            txt_static[inv][10]->SetSize(x, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][11]->SetSize(x+79, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][12]->SetSize(x+79, y + 22, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][13]->SetSize(x+172, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][14]->SetSize(x+172, y + 22, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][15]->SetSize(x+265, y, wxDefaultCoord, wxDefaultCoord);
            txt_static[inv][16]->SetSize(x+265, y + 22, wxDefaultCoord, wxDefaultCoord);
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
    
    // type: 4  Inverter extra Readings eg Grid volts, frequency
    if(type == 4)
    {
        posn = 0;
        for(ix=0; ix < N_EXTRA_READINGS; ix++)
        {
            if(extra_readings[ix].status_slot != 0)
            {
                if (ix & 1)
                {
                    txt_dsp_param[inv][posn]->SetSize(x + 92 * (posn / 2), y + 22, 46, 20);
                    txt_dsp_label[inv][posn]->SetSize(x + 48 + 92 * (posn / 2), y + 22, wxDefaultCoord, wxDefaultCoord);
                }
                else
                {
                    txt_dsp_param[inv][posn]->SetSize(x + 92 * (posn / 2), y, 46, 20);
                    txt_dsp_label[inv][posn]->SetSize(x + 48 + 92 * (posn / 2), y, wxDefaultCoord, wxDefaultCoord);
                }

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
    
    // type: 5  Today energy, both inverters
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
    // resize status_panel based on number of inverters configured
    if (inverter_address[1] == 0)
    {
        // only one inverter
        status_panel->SetSize(GetSize().GetWidth(), 52);
        status_panel->Layout();
        status_panel->Refresh();
        mainframe->Layout();
    }
    else
    {
        // two inverters
        status_panel->SetClientSize(GetSize().GetWidth(), 104);
        status_panel->Layout();
        status_panel->Refresh();
        mainframe->Layout();
    }

    // control bit 0: inverter addresses may have changed

    // style 0:  show all - horizontal
    //       1:  show all - stacked vertical
    //       2:  show all inverter 0
    //       3:  show all inverter 1
    //       4:  show some

    int ix;
    int style;

    // Show Inverter ID
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

    // set via settings | charts | Status Bar | {All, Some, Inv A, Inv B}
    style = chart_pages[graph_panel->page].status_type;

    if(inverter_address[1] == 0)
    {
        // only one inverter;
        // PlaceReadings(int type, int inv, int x, bool show
        PlaceReadings(1, 0,   0,   1, false);
        PlaceReadings(1, 1,   0,  23, false);
        PlaceReadings(2, 0, 149,   1, true);
        PlaceReadings(2, 1,   0,  23, false);
        PlaceReadings(3, 0, 423,   1, true);
        PlaceReadings(3, 1,   0,  23, false);
        PlaceReadings(4, 0, 749,   1, true);
        PlaceReadings(4, 1,   0,  23, false);
    }
    else
    {
        // PlaceReadings(int type, int inv, int x, bool show
        // PlaceReadings(1, 0, 149, true);

        if(style == 0)  
        {
            // all - horizontal
            PlaceReadings(1, 0, 149,   1, true);
            PlaceReadings(1, 1, 518,   1, true);
            PlaceReadings(2, 0, 239,   1, true);
            PlaceReadings(2, 1, 608,   1, true);
            PlaceReadings(3, 0, 887,   1, true);
            PlaceReadings(3, 1, 1207,   1, true);
            PlaceReadings(4, 0, 1527,   1, true);
            PlaceReadings(4, 1, 1727,   1, true);
        }
        if(style == 1)
        {
            // All - vertical eg stacked
            PlaceReadings(1, 0, 149, 1, true);
            PlaceReadings(1, 1, 149, 50, true);
            PlaceReadings(2, 0, 239, 1, true);
            PlaceReadings(2, 1, 239, 50, true);
            PlaceReadings(3, 0, 518, 1, true);
            PlaceReadings(3, 1, 518, 50, true);
            PlaceReadings(4, 0, 849, 1, true);
            PlaceReadings(4, 1, 849, 50, true);
        }
        if(style == 2)
        {
            // inverter A
            PlaceReadings(1, 0, 149,   1, true);
            PlaceReadings(1, 1,   0,   1, false);
            PlaceReadings(2, 0, 239,   1, true);
            PlaceReadings(2, 1,   0,   1, false);
            PlaceReadings(3, 0, 518,   1, true);
            PlaceReadings(3, 1,   0,   1, false);
            PlaceReadings(4, 0, 849,   1, true);
            PlaceReadings(4, 1,   0,   1, false);
        }
        if(style == 3)
        {
            // inverter B
            PlaceReadings(1, 0,   0,   1, false);
            PlaceReadings(1, 1, 149,   1, true);
            PlaceReadings(2, 0,   0,   1, false);
            PlaceReadings(2, 1, 239,   1, true);
            PlaceReadings(3, 0,   0,   1, false);
            PlaceReadings(3, 1, 518,   1, true);
            PlaceReadings(4, 0,   0,   1, false);
            PlaceReadings(4, 1, 849,   1, true);
        }
        if (style == 4)
        {
            // some
            PlaceReadings(1, 0, 149,   1, true);
            PlaceReadings(1, 1, 518,   1, true);
            PlaceReadings(2, 0, 239,   1, true);
            PlaceReadings(2, 1, 608,   1, true);
            PlaceReadings(3, 0,   0,   1, false);
            PlaceReadings(3, 1,   0,   1, false);
            PlaceReadings(4, 0,   0,   1, false);
            PlaceReadings(4, 1,   0,   1, false);
            PlaceReadings(5, 0, 961,   1, false);     // type: 5  Today energy, both inverters
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

    // Sets out the various controls to show Inverter data, only Y coord used, X over written in PlaceReadings() by data passed from SetupStatusPanel()
    // Show Total instantaneous energy, sum both inverters
    txt_powertot = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0,2), wxSize(116,42), TE_BIG | wxTE_READONLY | wxTE_CENTRE);
    txt_powertot->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font_big));

    // for each inverter
    for(inv=0; inv<2; inv++)
    {
        // Inverter output and name
        txt_power[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0,1), wxSize(82,HT_PW0), TE_BIG | wxTE_READONLY | wxTE_CENTRE);
        txt_power[inv]->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font_medium));
        txt_static[inv][0] = new wxStaticText(status_panel, -1, _T(""), wxPoint(0, 33));

        // adds controls to 'status_panel' show key values from inverters
        txt_power1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_power2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(46, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_volts1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_volts2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_current1[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_current2[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_efficiency[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 1), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);
        txt_temperature[inv] = new wxTextCtrl(status_panel, -1, _T(""), wxPoint(0, 23), wxSize(40, 20), wxTE_READONLY | wxTE_CENTRE);

        // Adds labels for respective fields
        txt_static[inv][2] = new wxStaticText(status_panel, -1, _T("W"), wxPoint(0, 4));
        txt_static[inv][3] = new wxStaticText(status_panel, -1, _T("W"), wxPoint(0, 26));
        txt_static[inv][4] = new wxStaticText(status_panel, -1, _T("V"), wxPoint(0, 4));
        txt_static[inv][5] = new wxStaticText(status_panel, -1, _T("V"), wxPoint(0, 26));
        txt_static[inv][6] = new wxStaticText(status_panel, -1, _T("A"), wxPoint(0, 4));
        txt_static[inv][7] = new wxStaticText(status_panel, -1, _T("A"), wxPoint(0, 26));
        txt_static[inv][8] = new wxStaticText(status_panel, -1, _T("%"), wxPoint(0, 4));
        txt_static[inv][9] = new wxStaticText(status_panel, -1, _T("deg"), wxPoint(0, 26));

        // adds Inverter Energy Summary
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

        // adds controls for defined extra readings
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

