
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
#include "auroramon.h"



BEGIN_EVENT_TABLE(DlgChart, wxDialog)
	EVT_BUTTON(dlgOK, DlgChart::OnButton)
	EVT_BUTTON(dlgAccept, DlgChart::OnButton)
	EVT_BUTTON(dlgCancel, DlgChart::OnButton)
	EVT_BUTTON(dlgInsert, DlgChart::OnButton)
	EVT_BUTTON(dlgDelete, DlgChart::OnButton)
	EVT_BUTTON(dlgDefault, DlgChart::OnButton)
    EVT_SPINCTRL(idPageSpin, DlgChart::OnPageSpin)
	EVT_BUTTON(idGraphA_1, DlgChart::OnColourButton)
	EVT_BUTTON(idGraphA_2, DlgChart::OnColourButton)
	EVT_BUTTON(idGraphA_3, DlgChart::OnColourButton)
	EVT_BUTTON(idGraphA_4, DlgChart::OnColourButton)
	EVT_BUTTON(idGraphA_5, DlgChart::OnColourButton)

	EVT_CHOICE(-1, DlgChart::OnGraphChoice)
    EVT_TEXT_ENTER(-1, DlgChart::OnTextEnter)
END_EVENT_TABLE()

#define gNULL {0,0,0,0,0xffd073,0,0}

CHART_PAGE chart_pages_default[N_PAGES] = {
    {0x3, 0, 0, "Total Power",
        {{g_PowerTotal,   0, gLEFT,  gSOLID, 0x80eaff, 0, 3500},
         {g_PowerOut,     0, gNONE,  gSOLID, 0x64c8ff, 0, 3500},
         {g_SunElevation, 0, gRIGHT, gTHICK, 0x8000ff, 0, 0},
        gNULL, gNULL}},

    {0x3, 2, 0, "Input Power",
        {{g_Power1,       0, gLEFT,  gSOLID, 0x5f4080, 0, 1750},
         {g_Power2,       0, gNONE,  gSOLID, 0x3b00ff, 0, 1750},
         {g_PowerMin12,   0, gNONE,  gSOLID, 0xcdc0ff, 0, 1750},
         {g_SunElevation, 0, gNONE,  gTHICK, 0x8000ff, 0, 0},
         {g_SolarIntensity,0,gRIGHT, gLINE,  0xffd073, 0, 1000},
        }},

    {0x3, 2, 0, "Temperature / Efficiency",
        {{g_PowerOut,     0, gBACKG, gSOLID, 0x282828, 0, 3500},
        {g_Temperature,   0, gLEFT,  gTHICK, 0x5500ff, 0, 75},
        {g_Efficiency,    0, gRIGHT, gLINE,  0xffaa00, 0, 150},
        gNULL, gNULL}},

    {0x3, 2, 0, "Power: string 1 v string 2",
        {{g_PowerOut,     0, gBACKG, gSOLID, 0x282828, 0, 3500},
        {g_PowerBalance,  0, gLEFT,  gLINE,  0x00ffff, 13, 87},
        gNULL, gNULL, gNULL}},

    {0x3, 2, 0, "Input Voltages / Grid Voltage",
        {{g_PowerOut,     0, gBACKG, gSOLID, 0x282828, 0,  3500},
        {g_Voltage1,      0, gLEFT,  gLINE,  0xff00ff, 100, 500},
        {g_Voltage2,      0, gNONE,  gLINE,  0xcdc0ff, 100, 500},
        {g_GridVoltage,      0, gRIGHT, gLINE,  0xff8888, 215, 255},
        gNULL}},

    {0, 0, 0, "", {gNULL, gNULL, gNULL, gNULL, gNULL}},

    {0x3, 3, 0, "#B  Input Power",
        {{g_Power1,       1, gLEFT,  gSOLID, 0x4fde24, 0, 1750},
         {g_Power2,       1, gNONE,  gSOLID, 0xff6b00, 0, 1750},
         {g_PowerMin12,   1, gNONE,  gSOLID, 0xb3ffe2, 0, 1750},
        gNULL, gNULL}},

    {0x3, 3, 0, "#B  Temperature / Efficiency",
        {{g_PowerOut,     1, gBACKG, gSOLID, 0x282828, 0, 3500},
        {g_Temperature,   1, gLEFT,  gTHICK, 0x0000ff, 0, 75},
        {g_Efficiency,    1, gRIGHT, gLINE,  0xffff00, 0, 150},
        gNULL, gNULL}},

    {0x3, 3, 0, "#B  Power: string 1 v string 2",
        {{g_PowerOut,     1, gBACKG, gSOLID, 0x282828, 0, 3500},
        {g_PowerBalance,  1, gLEFT,  gLINE,  0x00aaff, 13, 87},
        {g_SunElevation,  1, gRIGHT, gLINE,  0x8000ff, 0, 0},
        gNULL, gNULL}},

    {0x3, 3, 0, "#B  Input Voltages / Grid Voltage",
        {{g_PowerOut,     1, gBACKG, gSOLID, 0x282828, 0,  3500},
        {g_Voltage1,      1, gLEFT,  gLINE,  0xff0080, 100, 500},
        {g_Voltage2,      1, gNONE,  gLINE,  0xffc0ff, 100, 500},
        {g_GridVoltage,      1, gRIGHT, gLINE,  0xffc488, 215, 255},
        gNULL}},

    {0, 0, 0, "", {gNULL, gNULL, gNULL, gNULL, gNULL}},
    {0, 0, 0, "", {gNULL, gNULL, gNULL, gNULL, gNULL}},
    {0, 0, 0, "", {gNULL, gNULL, gNULL, gNULL, gNULL}},
    {0, 0, 0, "", {gNULL, gNULL, gNULL, gNULL, gNULL}},
};

wxColourData ChartPalette;

int palette_colours[16] = {
    0x0000ff,
    0xcdc0ff, 0x80a4ff, 0x64c8ff, 0x80eaff, 0x20ffff,
    0x70ffc8, 0x00ff00, 0x90ff90, 0xc7ff7d, 0xffff20,
    0xff8888, 0xff20ff, 0x8000ff, 0x894dff, 0x282828,
};

#define N_GRAPHTYPES  25
#define N_GRAPHTYPES_PRESET 15
int n_graphtypes = N_GRAPHTYPES_PRESET;   // initialized to the number of preset graph types

/*
typedef struct {
    int dsp_code;
    const char *name;
    const char *mnemonic;
    const char *mnem2;
    int per_inverter;
    unsigned char decimalplaces;
    int default_range;
    int addition;
    int multiply;
} GRAPH_TYPE;
*/
GRAPH_TYPE graph_types[N_GRAPHTYPES+1] = {
    { 0, "none",          "none",       "none", 0, 0,    0,  0, 1},
    { 0, "Power Total",   "Power_tot",  "ptot", 0, 0, 3500,  0, X_PO},
    { 3, "Power Output",  "Power_out",  "pout", 1, 0, 3500,  0, X_PO},
    { 0, "Power Input 1", "Power_in1",  "pw1",  1, 1, 2000,  0, X_PI},
    { 0, "Power Input 2", "Power_in2",  "pw2",  1, 1, 2000,  0, X_PI},
    { 0, "Power min of 1&2", "p12",     "p12",  1, 0, 2000,  0, X_PI},
    { 0, "Power Balance", "Power_bal",  "pbal", 1, 2,  100,  0, 100},
    {23, "Voltage 1",     "Voltage_1",  "vt1",  1, 1,  600,  0, X_VI},
    {26, "Voltage 2",     "Voltage_2",  "vt2",  1, 1,  600,  0, X_VI},
    {25, "Current 1",     "Current_1",  "ct1",  1, 2,   20,  0, 0},
    {27, "Current 2",     "Current_2",  "ct2",  1, 2,   20,  0, 0},
    { 0, "Sun Elevation", "Elevation",  "sun",  0, 2,   90,  0, 0},
    { 0, "Solar Intensity","Intensity", "intn", 1, 1, 1000,  0, 0},
    { 0, "Temperature",   "Tempr",      "tmpr", 1, 2,  80,   0, 100},
    { 0, "Efficiency",    "Efficiency", "eff",  1, 2,  100,  0, 100},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
    { 0, NULL, NULL, NULL, 0,0,0,0,0},
};


/*
typedef struct {
    int dsp_code;
    const char *name;
    const char *mnemonic;
    unsigned char decimalplaces;
    int multiplier;
    int default_range;
    int addition;
    int multiply;
} EXTRA_READING_TYPE;
*/

EXTRA_READING_TYPE extra_reading_types[] = {
    { 0, "none", "none", 0, 0, 0, 0, 1},
    {64, "Energy Today",    "Energy",  2,    1,  25,  0, 500},
    { 1, "Grid Voltage",    "Grid_V",  1,    1, 300,  0, 100},
    { 2, "Grid Current",    "Grid_I",  2,    1,  20,  0, 100},
    { 4, "Grid Frequency",  "Grid_Hz", 2,    1,  70,  0, 100},
    { 8, "Pin 1 output  W", "W_Pin1",  0,    1, 2000,  0, X_PI},
    { 9, "Pin 2 output  W", "W_Pin2",  0,    1, 2000,  0, X_PI},

    {34, "Peak Power",      "Peak",    0,    1, 4000,  0, X_PO},
    {35, "Peak Today",      "PkToday", 0,    1, 4000,  0, X_PO},

    { 6, "I-leak Dc/Dc  mA", "LeakDc",  1, 1000,   1,  0, 10000},
    { 7, "I-leak Inverter mA ","LeakInv", 1, 1000,   1,  0, 10000},
    {30, "R-iso  MOhm",      "Riso",    1,    1,  30,  0, 100},

    { 5, "V-bulk",          "Vbulk",   1,    1,  600,  0, X_VI},
    {33, "V-bulk Mid",      "VblkMid", 1,    1,  600,  0, X_VI},
    {31, "V-bulk Dc/Dc",    "VblkDc",  1,    1,  600,  0, X_VI},
    {45, "V-bulk +",        "Vblk+",   1,    1,  600,  0, X_VI},
    {46, "V-bulk -",        "Vblk-",   1,    1,  600,  0, X_VI},

    {28, "Grid V Dc/Dc",    "Vg_Dc",   1,    1,  600,  0, 100},
    {32, "Grid V Average",  "Vg_Av",   1,    1,  600,  0, 100},
    {36, "Grid V Neutral",  "Vg_N",    1,    1,  600,  0, 100},
    {29, "Grid Freq Dc/Dc", "Hz_Dc",   2,    1,  100,  0, 100},
//    {38, "? 38",            "?_38",    1,    1,  100,  0, 100},

    {21, "Tempr Booster",   "TmprBo",  1,    1,   70, 50, 100},  // allow for negative temperatures
    {22, "Tempr Inverter",  "TmprIv",  1,    1,   70, 50, 100},
    {47, "Tempr Supervisor","TmprSu",  1,    1,   70, 50, 100},
    {48, "Tempr Alim",      "TmprAl",  1,    1,   70, 50, 100},
    {49, "Tempr Heatsink",  "TmprHs",  1,    1,   70, 50, 100},
    {53, "Fan 1 Speed",     "Fan_1",   0,    1, 1000,  0, 10},
    {54, "Fan 2 Speed",     "Fan_2",   0,    1, 1000,  0, 10},
    {55, "Fan 3 Speed",     "Fan_3",   0,    1, 1000,  0, 10},

    {37, "Wind gen. freq",  "Wind_Hz",  1,    1, 100,  0, 100},
    {0, NULL, NULL, 0, 0}
};


int n_extra_readings=0;
EXTRA_READING extra_readings[N_EXTRA_READINGS];


typedef struct {
    wxChoice *t_graphtype;
    wxChoice *t_inv;
    wxButton *t_colour;
    wxTextCtrl *t_base;
    wxTextCtrl *t_top;
    wxChoice *t_scaletype;
    wxChoice *t_style;
    wxStaticText *t_text_ptot;
} GRAPHFIELDS;

GRAPHFIELDS gfields[N_PAGE_GRAPHS];

wxArrayString InverterNames;
wxArrayString ScaleNames;
wxArrayString GraphTypes;
wxArrayString StyleNames;
wxArrayString StatusNames;
wxString BackColours[N_BACK_COLOURS] = {_T("black"), _T("white")};

CHART_PAGE chart_pages_copy[N_PAGES];


void MakeGraphTypesList()
{//======================
    int ix;

    GraphTypes.Empty();
    for(ix=0; ix<n_graphtypes; ix++)
    {
        GraphTypes.Add(wxString(graph_types[ix].name, wxConvLocal));
    }
}

/************************************************************
                Extra Readings Dialog
*************************************************************/

int AddExtraReading(int extratype)
{//===============================
    int ix;

    for(ix=1; ix < n_graphtypes; ix++)
    {
        if(graph_types[ix].mnemonic != NULL)
        {
            if(strcmp(graph_types[ix].mnemonic, extra_reading_types[extratype].mnemonic) == 0)
                return(ix);   // already in the graph types menu
        }
    }

    if(n_graphtypes >= N_GRAPHTYPES)
    {
        wxLogError(_T("No more extra readings"));
        return(0);
    }

    graph_types[n_graphtypes].dsp_code = extra_reading_types[extratype].dsp_code;
    graph_types[n_graphtypes].name = extra_reading_types[extratype].name;
    graph_types[n_graphtypes].mnemonic = extra_reading_types[extratype].mnemonic;
    graph_types[n_graphtypes].mnem2 = extra_reading_types[extratype].mnemonic;
    graph_types[n_graphtypes].decimalplaces = extra_reading_types[extratype].decimalplaces;
    graph_types[n_graphtypes].per_inverter = 1;
    graph_types[n_graphtypes].default_range = extra_reading_types[extratype].default_range;
    graph_types[n_graphtypes].addition = extra_reading_types[extratype].addition;
    graph_types[n_graphtypes].multiply = extra_reading_types[extratype].multiply;
    n_graphtypes++;

    MakeGraphTypesList();
    return(n_graphtypes-1);
}

void RemoveExtraReading(int code)
{//==============================
    int ix;
    int graphtype = 0;
    int page;
    CHART_PAGE *cp;

    for(ix=0; ix < n_graphtypes; ix++)
    {
        if((graph_types[ix].dsp_code != 0) && (graph_types[ix].dsp_code == code))
        {
            graphtype = ix;
            break;
        }
    }
    if(graphtype == 0)
        return;  // not found

    for(ix = graphtype+1; ix < n_graphtypes; ix++)
    {
        // remove this entry from the graph_types table
        memcpy(&graph_types[ix-1], &graph_types[ix], sizeof(graph_types[0]));
    }
    n_graphtypes--;

    // remove this type from chart pages
    for(page=0; page < N_PAGES; page++)
    {
        cp = &chart_pages[page];
        for(ix=0; ix<N_PAGE_GRAPHS; ix++)
        {
            if(cp->graph[ix].type == graphtype)
            {
                memset(&cp->graph[ix], 0, sizeof(cp->graph[ix]));
                cp->graph[ix].colour = 0x2080ff;
            }
        }
        for(ix=0; ix<N_PAGE_GRAPHS; ix++)
        {
            if(cp->graph[ix].type > graphtype)
            {
                cp->graph[ix].type--;
            }
        }
    }

    MakeGraphTypesList();
    graph_panel->Refresh();
}  // end of RemoveExtraReading


void SetExtraReadings(wxString string)
{//===================================
// Called on program start
    int n;
    int ix;
    int j;
    int flags;
    int code;
    int x[8];
    char buf[200];

    strcpy(buf, string.mb_str(wxConvLocal));
    n = sscanf(buf, "%d %d %d %d %d %d %d %d", &x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &x[7]);

    n_extra_readings = 0;
    for(ix=0; ix<n; ix++)
    {
        flags = x[ix] / 1000;
        code = x[ix] % 1000;

        for(j=1; extra_reading_types[j].name != NULL; j++)
        {
            if(extra_reading_types[j].dsp_code == code)
            {
                extra_readings[n_extra_readings].type = j;
                extra_readings[n_extra_readings].dsp_code = code;
                extra_readings[n_extra_readings].graph_slot = 0;
                extra_readings[n_extra_readings].status_slot = 0;
                if(flags & 1)
                {
                    extra_readings[ix].graph_slot = AddExtraReading(j);
                }
                if(flags & 2)
                {
                    extra_readings[n_extra_readings].status_slot = 1;
                }
                n_extra_readings++;
                break;
            }
        }
    }
    // Note, SetupStatusPanel() will be called later.  Can't do it here because it's not initialized.
}



BEGIN_EVENT_TABLE(DlgExtraR, wxDialog)
	EVT_BUTTON(dlgOK, DlgExtraR::OnButton)
	EVT_BUTTON(dlgCancel, DlgExtraR::OnButton)
END_EVENT_TABLE()



DlgExtraR::DlgExtraR(wxWindow *parent)
    : wxDialog(parent, -1, _T("Extra Readings"), wxPoint(200, 150), wxSize(320,320+DLG_HX))
{//=================================
    int x;
    int y;
    int ix;
    x = 160;
    y = 20;

    ExtraRNames.Clear();
    for(ix=0; extra_reading_types[ix].name != NULL; ix++)
    {
        ExtraRNames.Add(wxString(extra_reading_types[ix].name, wxConvLocal));
    }

    new wxStaticText(this, -1, _T("Reading"), wxPoint(8, y));
    new wxStaticText(this, -1, _T("Graph"), wxPoint(x, y));
    new wxStaticText(this, -1, _T("Numeric"), wxPoint(x+50, y));

    y = 38;
    for(ix=0; ix < N_EXTRA_READINGS; ix++)
    {
        t_type[ix] = new wxChoice(this, -1, wxPoint(8, y+2), wxDefaultSize, ExtraRNames);
        t_type[ix]->SetSelection(0);
#ifdef __WXMSW__
        t_graph[ix] = new wxCheckBox(this, -1, wxEmptyString, wxPoint(x+6, y+6));
        t_numeric[ix] = new wxCheckBox(this, -1, wxEmptyString, wxPoint(x+56, y+6));
#else
        t_graph[ix] = new wxCheckBox(this, -1, wxEmptyString, wxPoint(x+6, y));
        t_numeric[ix] = new wxCheckBox(this, -1, wxEmptyString, wxPoint(x+56, y));
#endif
        y += 28;
    }


    y = 280;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(130,y));
    button_OK->SetDefault();
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(220,y));

}

void DlgExtraR::ShowDlg()
{//=====================
    int ix;

    for(ix=0; ix < N_EXTRA_READINGS; ix++)
    {
        t_type[ix]->SetSelection(extra_readings[ix].type);
        t_graph[ix]->SetValue((bool)extra_readings[ix].graph_slot);
        t_numeric[ix]->SetValue((bool)extra_readings[ix].status_slot);

//        if(extra_readings[ix].type == 0)
//            t_graph[ix]->SetValue(true);   // preset 'graph' by default
    }
    SetSize(200, 150, wxDefaultCoord, wxDefaultCoord, wxSIZE_USE_EXISTING);
    ShowModal();
}


void DlgExtraR::OnButton(wxCommandEvent &event)
{//===========================================
    int ix;
    int id;
    int type;
    int prev_code;

    id = event.GetId();
    if(id == dlgOK)
    {
        n_extra_readings = 0;
        for(ix=0; ix < N_EXTRA_READINGS; ix++)
        {
            prev_code = extra_readings[ix].dsp_code;
            extra_readings[ix].type = type = t_type[ix]->GetSelection();
            extra_readings[ix].dsp_code = extra_reading_types[type].dsp_code;
            extra_readings[ix].status_slot = t_numeric[ix]->GetValue();
            extra_readings[ix].graph_slot = 0;

            if(type == 0)
            {
                RemoveExtraReading(prev_code);
                extra_readings[ix].dsp_code = 0;
                extra_readings[ix].status_slot = 0;
                extra_readings[ix].graph_slot = 0;
            }
            else
            {
                if(t_graph[ix]->GetValue() == true)
                {
                    extra_readings[ix].graph_slot = AddExtraReading(type);
                }
                else
                {
                    RemoveExtraReading(prev_code);
                }

                if((extra_readings[ix].status_slot != 0) || (extra_readings[ix].graph_slot != 0))
                {
                    n_extra_readings++;
                }
            }
        }


        ConfigSave(1);
        mainframe->SetupStatusPanel(2);   // allocates status_slot
        inverters[0].logheaders_changed |= 1;
        inverters[1].logheaders_changed |= 1;
        inverters[0].get_extra_dsp = 1;
        inverters[1].get_extra_dsp = 1;
    }
    Show(false);
}


//=======================================================================================================================




int SaveCharts(const wxString fname)
{//=================================
    FILE *f;
    int page;
    int gnum;
    CHART_PAGE *cpage;
    GRAPH *g;
    unsigned int ix;
    char name[80];

    if((f = fopen(fname.mb_str(wxConvLocal), "w")) == NULL)
    {
        return(-1);
    }

    for(page=0; page < N_PAGES; page++)
    {
        cpage = &chart_pages[page];
        if((cpage->flags & gINUSE) == 0)
            continue;

        for(ix=0; ix<sizeof(name); ix++)
        {
            if((name[ix] = cpage->name[ix]) == 0)
                break;
            if(isspace(name[ix]))
                name[ix] = '_';
        }
        name[sizeof(name)-1] = 0;
        if(name[0] == 0)
            sprintf(name, "Page_%d", page+1);

        fprintf(f, "Page  %.2x %d %d %s\n", cpage->flags, cpage->status_type, 0, name);
        for(gnum=0; gnum < N_PAGE_GRAPHS; gnum++)
        {
            g = &cpage->graph[gnum];
            fprintf(f, "%s %d %d %d %.6x %d %d\n", graph_types[g->type].mnem2, g->inverter, g->scaletype, g->style, g->colour, g->base, g->top);
        }
        fputc('\n', f);
    }

    fclose(f);
    return(0);
}


int LoadCharts(const wxString fname)
{//=================================
    FILE *f;
    int ix;
    int n;
    int page=0;
    int gnum=0;
    CHART_PAGE *cpage=NULL;
    GRAPH *g;
    int linenum = 0;
    int error = 0;
    char name[80];
    char buf[100];

    int flags;
    int status_type;
    int inverter;
    int scaletype;
    int style;
    int colour;
    int base;
    int top;
    int spare;

    if((f = fopen(fname.mb_str(wxConvLocal), "r")) == NULL)
    {
        return(-1);
    }

    memset(chart_pages, 0, sizeof(chart_pages));

    while(fgets(buf, sizeof(buf), f) != NULL)
    {
        linenum++;
        if(memcmp(buf, "Page ", 5) == 0)
        {
            n = sscanf(&buf[5], "%x %d %d %s", &flags, &status_type, &spare, name);
            if(n == 4)
            {
                gnum = 0;
                cpage = &chart_pages[page++];
                cpage->flags = flags | gINUSE;
                cpage->status_type = status_type;
                for(ix=0; ix<(int)sizeof(cpage->name); ix++)
                {
                    if(name[ix] == '_')
                        name[ix] = ' ';
                    if((cpage->name[ix] = name[ix]) == 0)
                        break;
                }
                cpage->name[ix] = 0;
            }
            else
            {
                error = linenum;
                break;
            }
        }
        else
        if(isalpha(buf[0]))
        {
            // a graph within a page
            if(cpage == NULL)
            {
                error = linenum;
                break;
            }
            g = &cpage->graph[gnum];
            n = sscanf(buf, "%s %d %d %d %x %d %d", name, &inverter, &scaletype, &style, &colour, &base, &top);
            if(n != 7)
            {
                error = linenum;
                break;
            }
            g->inverter = inverter;
            g->scaletype = scaletype;
            g->style = style;
            g->colour = colour;
            g->base = base;
            g->top = top;

            for(ix=0; ix<n_graphtypes; ix++)
            {
                if((graph_types[ix].mnemonic != NULL) && (strcmp(name, graph_types[ix].mnemonic) == 0))
                {
                    g->type = ix;
                    break;
                }
                if((graph_types[ix].mnem2 != NULL) && (strcmp(name, graph_types[ix].mnem2) == 0))
                {
                    g->type = ix;
                    break;
                }
            }
            if(ix == n_graphtypes)
            {
                // not found, so ignore this entry
                continue;
            }

            gnum++;
        }
    }
    fclose(f);

    return(error);
}



void InitCharts2()
{//==============
    if(LoadCharts(data_system_dir + _T("/chartinfo")) != 0)
    {
        // Failed to load the saved graph layout information.
        // Use the default
        memcpy(chart_pages, chart_pages_default, sizeof(chart_pages));
    }
    graph_panel->SetColours(chart_pages[0].flags & gBACK_COLOUR);
}


void MakeInverterNames(int control)
{//=====================
    InverterNames.Empty();

    if(control == 0)
    {
        // for chart setup dialog
        InverterNames.Add(_T("A "));
        InverterNames.Add(_T("B "));
    }
    else
    {
        InverterNames.Add(wxString::Format(_T("A  (address #%d) "), inverter_address[0]));
        if(inverter_address[1] != 0)
            InverterNames.Add(wxString::Format(_T("B  (address #%d) "), inverter_address[1]));
    }
}


DlgChart::DlgChart(wxWindow *parent)
    : wxDialog(parent, -1, _T("Charts Settings"), wxDefaultPosition, wxSize(600,290+DLG_HX))
{//==============================================
    int x, y;
    int ix;

    MakeInverterNames(0);

    ScaleNames.Clear();
    ScaleNames.Add(_T("none"));
    ScaleNames.Add(_T("left"));
    ScaleNames.Add(_T("right"));
    ScaleNames.Add(_T("backg"));

    MakeGraphTypesList();

    StyleNames.Clear();
    StyleNames.Add(_T("line"));
    StyleNames.Add(_T("solid"));
    StyleNames.Add(_T("thick"));
    StyleNames.Add(_T("thick2"));

    StatusNames.Clear();
    StatusNames.Add(_T("all"));
    StatusNames.Add(_T("some"));
    StatusNames.Add(_T("inv A"));
    StatusNames.Add(_T("inv B"));

    memset(gfields, 0, sizeof(gfields));

    for(ix=0; ix<16; ix++)
        ChartPalette.SetCustomColour(ix, palette_colours[ix]);


    y = 20;
    new wxStaticText(this, -1, _T("Page number"), wxPoint(8, y+3));
    t_pagenum = new wxSpinCtrl(this, idPageSpin, wxEmptyString, wxPoint(82, y-1), wxSize(50, 22), wxSP_ARROW_KEYS, 1, N_PAGES, 1);
    cpage = &chart_pages[0];
    t_pagename = new wxTextCtrl(this, idPageName, wxString(cpage->name, wxConvLocal), wxPoint(140, y), wxSize(200, 20), wxTE_PROCESS_ENTER);
    new wxStaticText(this, -1, _T("Status bar"), wxPoint(351, y+3));
    t_statustype = new wxChoice(this, idStatusType, wxPoint(410, y-1), wxDefaultSize, StatusNames);
    new wxStaticText(this, -1, _T("Backg"), wxPoint(487, y+3));
    t_back_colour = new wxChoice(this, idBackColour, wxPoint(523, y-1), wxDefaultSize, N_BACK_COLOURS, BackColours);

    y = 62;
    new wxStaticText(this, -1, _T("Inverter"), wxPoint(140, y));
    new wxStaticText(this, -1, _T("Colour"), wxPoint(195, y));
    new wxStaticText(this, -1, _T("Min"), wxPoint(247, y));
    new wxStaticText(this, -1, _T("Max"), wxPoint(297, y));
    new wxStaticText(this, -1, _T("Scale"), wxPoint(346, y));
    new wxStaticText(this, -1, _T("Style"), wxPoint(415, y));

    y = 82;
    for(ix=0; ix<N_PAGE_GRAPHS; ix++)
    {
        gfields[ix].t_graphtype = new wxChoice(this, idGraphA_1+ix, wxPoint(8, y), wxDefaultSize, GraphTypes);
        y += 32;
    }

    y = 250;
    x = 326;
    button_OK = new wxButton(this, dlgOK, _T("OK"), wxPoint(x,y));
    button_OK->SetDefault();
    button_Accept = new wxButton(this, dlgAccept, _T("Accept"), wxPoint(x+90,y));
    button_Cancel = new wxButton(this, dlgCancel, _T("Cancel"), wxPoint(x+180,y));

    y = 82;
    x = 506;
    button_Insert = new wxButton(this, dlgInsert, _T("Insert Page"), wxPoint(x,y));
    button_Delete = new wxButton(this, dlgDelete, _T("Delete Page"), wxPoint(x,y+32));
    button_Default = new wxButton(this, dlgDefault, _T("Defaults"), wxPoint(x,y+100));

    fname_chartinfo = data_system_dir + _T("/chartinfo");
    page = 0;
    ShowPage(page);
}

void DlgChart::ShowDlg()
{//=====================
    memcpy(chart_pages_copy, chart_pages, sizeof(chart_pages_copy));
    ShowPage(graph_panel->page);
    Show();
}



void DlgChart::BuildGraphFields(int gnum)
{//======================================
    int gtype;
    int x, y;
    int ix;
    wxString string;
    GRAPHFIELDS *gf;

    MakeInverterNames(0);

    gf = &gfields[gnum];
    gtype = gfields[gnum].t_graphtype->GetSelection();

    // delete all the fields except graph-type
    delete gf->t_inv;
    delete gf->t_colour;
    delete gf->t_base;
    delete gf->t_top;
    delete gf->t_scaletype;
    delete gf->t_style;
    delete gf->t_text_ptot;
    gf->t_inv = NULL;
    gf->t_colour = NULL;
    gf->t_base = NULL;
    gf->t_top = NULL;
    gf->t_scaletype = NULL;
    gf->t_style = NULL;
    gf->t_text_ptot = NULL;

    gf->t_graphtype->Clear();
    gf->t_graphtype->Append(GraphTypes);
    gf->t_graphtype->SetSelection(gtype);

    if(gtype == 0)
        return;  // not in use

    y = 82 + gnum*32;

    x = 140;
    if(gtype == g_PowerTotal)
    {
        if(inverter_address[1] == 0)
        {
            gf->t_text_ptot = new wxStaticText(this, -1, _T("hidden"), wxPoint(x+3, y+4));
        }
    }
    else
    if(graph_types[gtype].per_inverter == 1)
    {
        gf->t_inv = new wxChoice(this, idGraphB_1+gnum, wxPoint(x, y), wxDefaultSize, InverterNames);
        gf->t_inv->SetSelection(cpage->graph[gnum].inverter);
    }

    x += 50;
#ifdef __WXMSW__
    gf->t_colour = new wxButton(this, idGraphA_1+gnum, wxEmptyString, wxPoint(x, y+2), wxSize(47, 20));
#else
    gf->t_colour = new wxButton(this, idGraphA_1+gnum, wxEmptyString, wxPoint(x, y+1), wxSize(47, 22));
#endif
    gf->t_colour->SetBackgroundColour(wxColour(cpage->graph[gnum].colour));
    x += 50;
    gf->t_base = new wxTextCtrl(this, idGraphA_1+gnum, wxString::Format(_T("%d"), cpage->graph[gnum].base), wxPoint(x, y+2), wxSize(47, 20), wxTE_PROCESS_ENTER);
    x += 50;
    gf->t_top = new wxTextCtrl(this, idGraphB_1+gnum, wxString::Format(_T("%d"), cpage->graph[gnum].top), wxPoint(x, y+2), wxSize(47, 20), wxTE_PROCESS_ENTER);
    x += 50;
    gf->t_scaletype = new wxChoice(this, idGraphC_1+gnum, wxPoint(x, y), wxDefaultSize, ScaleNames);
    gf->t_scaletype->SetSelection(cpage->graph[gnum].scaletype);
    x += 68;
    gf->t_style = new wxChoice(this, idGraphD_1+gnum, wxPoint(x, y), wxDefaultSize, StyleNames);
    gf->t_style->SetSelection(cpage->graph[gnum].style);

    cpage->flags &= ~gINUSE;
    for(ix=0; ix<N_PAGE_GRAPHS; ix++)
    {
        if(cpage->graph[ix].type != 0)
        {
            cpage->flags |= gINUSE;
            break;
        }
    }
}

void DlgChart::ShowPage(int pagenum)
{//=================================
    int ix;
    int background;

    page = pagenum;
    cpage = &chart_pages[page];

    t_pagenum->SetValue(page+1);
    t_pagename->SetValue(wxString(cpage->name, wxConvLocal));
    t_statustype->SetSelection(cpage->status_type);

    background = (cpage->flags & gBACK_COLOUR) >> 4;
    t_back_colour->SetSelection(background);

    for(ix=0; ix<N_PAGE_GRAPHS; ix++)
    {
        gfields[ix].t_graphtype->SetSelection(cpage->graph[ix].type);
        BuildGraphFields(ix);
    }
}


void DlgChart::OnColourButton(wxCommandEvent &event)
{//=================================================
    int id;
    int gnum;
    wxColour new_colour;

    id = event.GetId();
    gnum = id - idGraphA_1;


    ChartPalette.SetColour(cpage->graph[gnum].colour);

    wxColourDialog dialog(this, &ChartPalette);
    if (dialog.ShowModal() != wxID_OK)
        return;

    wxColourData retData = dialog.GetColourData();
    new_colour = retData.GetColour();
    cpage->graph[gnum].colour = (new_colour.Blue() << 16) + (new_colour.Green() << 8) + new_colour.Red();
    BuildGraphFields(gnum);
    graph_panel->Refresh();
}


void DlgChart::OnGraphChoice(wxCommandEvent &event)
{//================================================
    int id;
    int gnum;
    wxColour new_colour;

    id = event.GetId();

    if(id == idStatusType)
    {
        cpage->status_type = t_statustype->GetSelection();
        mainframe->SetupStatusPanel(0);
    }
    else
    if(id == idBackColour)
    {
        graph_panel->SetColours(t_back_colour->GetSelection());
    }
    else
    if(id >= idGraphD_1)
    {
        gnum = id - idGraphD_1;
        cpage->graph[gnum].style = (unsigned char)gfields[gnum].t_style->GetSelection();
    }
    else
    if(id >= idGraphC_1)
    {
        gnum = id - idGraphC_1;
        cpage->graph[gnum].scaletype = (unsigned char)gfields[gnum].t_scaletype->GetSelection();
    }
    else
    if(id >= idGraphB_1)
    {
        gnum = id - idGraphB_1;
        cpage->graph[gnum].inverter = gfields[gnum].t_inv->GetSelection();
    }
    else
    if(id >= idGraphA_1)
    {
        gnum = id-idGraphA_1;
        cpage->graph[gnum].type = gfields[gnum].t_graphtype->GetSelection();
        cpage->graph[gnum].base = 0;
        cpage->graph[gnum].top = graph_types[cpage->graph[gnum].type].default_range;
        BuildGraphFields(gnum);
    }
    graph_panel->Refresh();
}



void DlgChart::OnPageSpin(wxSpinEvent& WXUNUSED(event))
{//==========================================
    page = t_pagenum->GetValue()-1;
    ShowPage(page);
    graph_panel->ShowPage(page);
}


void DlgChart::OnTextEnter(wxCommandEvent &event)
{//==============================================
    int id;
    int gnum;
    long int value;

    id = event.GetId();
    if(id == idPageName)
    {
        strncpy0(chart_pages[page].name, t_pagename->GetValue().mb_str(wxConvLocal), sizeof(chart_pages[0].name));
    }
    else
    if(id >= idGraphB_1)
    {
        gnum = id - idGraphB_1;
        if(gfields[gnum].t_top->GetValue().ToLong(&value) == true)
        {
            cpage->graph[gnum].top = value;
        }
    }
    else
    if(id >= idGraphA_1)
    {
        gnum = id - idGraphA_1;
        if(gfields[gnum].t_base->GetValue().ToLong(&value) == true)
        {
            cpage->graph[gnum].base = value;
        }
    }

    graph_panel->Refresh();
}


void DlgChart::OnButton(wxCommandEvent &event)
{//===========================================
    int id;
    int gnum;
    int ix;
    long int value;
    int background;

    id = event.GetId();
    switch(id)
    {
    case dlgOK:
    case dlgAccept:
        for(gnum=0; gnum<N_PAGE_GRAPHS; gnum++)
        {
            if((gfields[gnum].t_top != NULL) && (gfields[gnum].t_top->GetValue().ToLong(&value) == true))
            {
                cpage->graph[gnum].top = value;
            }
            if((gfields[gnum].t_base != NULL) && (gfields[gnum].t_base->GetValue().ToLong(&value) == true))
            {
                cpage->graph[gnum].base = value;
            }
        }
        strncpy0(chart_pages[page].name, t_pagename->GetValue().mb_str(wxConvLocal), sizeof(chart_pages[0].name));
        memcpy(&chart_pages_copy[page], &chart_pages[page], sizeof(chart_pages_copy[page]));

        background = t_back_colour->GetSelection() << 4;
        chart_pages[page].flags = (chart_pages[page].flags & ~gBACK_COLOUR) | background;

        if(id == dlgOK)
        {
            Show(false);
        }

        fname_chartinfo = data_system_dir + _T("/chartinfo");

        if(SaveCharts(fname_chartinfo) != 0)
        {
            wxLogError(_T("Can't save to: ") + fname_chartinfo);
        }
        break;

    case dlgInsert:
        for(ix=N_PAGES-1; ix > page; ix--)
        {
            if((chart_pages[ix].flags & gINUSE) == 0)
                break;
        }
        if(ix == page)
        {
            wxLogError(_T("No more pages"));
            break;
        }
        for(; ix > page; ix--)
        {
            memcpy(&chart_pages[ix], &chart_pages[ix-1], sizeof(chart_pages[ix]));
        }
        memset(&chart_pages[page], 0, sizeof(chart_pages[0]));
        ShowPage(page);
        break;

    case dlgDelete:
        for(ix=page+1; ix<N_PAGES; ix++)
        {
            memcpy(&chart_pages[ix-1], &chart_pages[ix], sizeof(chart_pages[ix]));
        }
        memset(&chart_pages[N_PAGES-1], 0, sizeof(chart_pages[0]));
        ShowPage(page);
        break;

    case dlgDefault:
        wxRenameFile(fname_chartinfo, fname_chartinfo + _T("_backup"), true);
        memcpy(chart_pages, chart_pages_default, sizeof(chart_pages));
        ShowPage(page);
        break;

    default:
        memcpy(chart_pages, chart_pages_copy, sizeof(chart_pages));
        Show(false);
        break;
    }

    graph_panel->SetColours(chart_pages[page].flags & gBACK_COLOUR);
    graph_panel->Refresh();
}

