
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


#include <wx/dcbuffer.h>
#include "auroramon.h"

#define N_SCROLL    8   //scroll by 8 pixels

wxFont Font_GraphDate(24, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
wxFont Font_ChartName(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
wxFont Font_SunElevation(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);


CHART_PAGE chart_pages[N_PAGES];

DATAREC *graph_data[N_INV];
unsigned short *dsp_data[N_INV][N_DSP];



char *ns[2] = {NULL, NULL};
unsigned short *points_vt1[2] = {NULL, NULL};
unsigned short *points_vt2[2] = {NULL, NULL};
unsigned short *points_pw0[2] = {NULL, NULL};
unsigned short *points_pw1[2] = {NULL, NULL};
unsigned short *points_pw2[2] = {NULL, NULL};
short *points_tmpr[2] = {NULL, NULL};
unsigned short points_pwi[N_10SEC+1];

unsigned short *points_dsp[2][N_DSP];

wxPoint points[N_10SEC+3];
static int draw_width = 0;
static int graph_date_width = 0;

const double range_adjust[11] = {0.125, 0.1768, 0.25, 0.3536, 0.5, 0.7071, 1.0, 1.4142, 2.0, 2.8284, 4.0};

#define CLIPPING_WIDTH 8

GraphPanel::GraphPanel(wxWindow *parent, const wxPoint& pos, const wxSize &size):
    wxScrolledWindow(parent,-1,pos,size,wxSUNKEN_BORDER | wxHSCROLL)
{//============================================================
    int ix;

    newday = 1;
    page = 0;
	histogram_page = 0;
	top_ix = 0;
	start_ix = N_10SEC;
    chart_mode = 0;
	x_scale = 0;   // not yet set
	hist_xscale_ix = 4;
	hist_range_ix = 6;
	energy_n_days = 0;
	enable_draw = 0;
    SetColours(0);

	for(ix=0; ix<N_INV; ix++)
        graph_data[ix] = NULL;
	SetFocus();

    EnableScrolling(false, false);
}  //  end of GraphPanel::GraphPanel



void GraphPanel::SetColours(int white)
{//===================================
    if(white)
    {
        colour_backg.Set((unsigned long)0xffffff);
        colour_grid1.Set((unsigned long)0x707070);
        colour_grid2.Set((unsigned long)0xd0d0d0);
        colour_text1.Set((unsigned long)0x000000);
        colour_text2.Set((unsigned long)0x800000);
    }
    else
    {
        colour_backg.Set((unsigned long)0x000000);
        colour_grid1.Set((unsigned long)0xc0c0c0);
        colour_grid2.Set((unsigned long)0x606060);
        colour_text1.Set((unsigned long)0xffffff);
        colour_text2.Set((unsigned long)0xa8f4ff);
    }
    SetBackgroundColour(colour_backg);
}



DlgCoords::DlgCoords(wxWindow *parent)
    : wxDialog(parent, -1, _T("Coords"), wxPoint(0, 116), wxSize(320,280+DLG_HX))
{//====================================
    int x, y;
    int ix;

    x = 80;
    y = 0;

    new wxStaticText(this, -1, _T("Time"), wxPoint(8, 0));
    t_time = new wxStaticText(this, -1, wxEmptyString, wxPoint(x, 0));
    for(ix=0; ix<5; ix++)
    {
        y += 16;
        t_label[ix] = new wxStaticText(this, -1, wxEmptyString, wxPoint(8, y));
        t_value[ix] = new wxStaticText(this, -1, wxEmptyString, wxPoint(x, y), wxSize(100, 200));
    }
}


void DlgCoords::ShowDlg(CHART_PAGE *chart_page, int period)
{//========================================================
    int graph_num;
    int graphtype;
    int inv;
    double value=0;
    int v;
    int x;
    int ix;
    int sum;
    wxString label_string;
    wxString format_string;
    static int prev_period = 0;

    if(period < 0)
        period = prev_period;
    prev_period = period;

    if(IsShown() == false)
    {
#ifdef __WXMSW__
        SetSize(0, 116, 150, 100+DLG_HX);
#else
        SetSize(0, 116, 150, 96+DLG_HX);
#endif
    }

    t_time->SetLabel(wxString::Format(_T("%d:%.2d:%.2d"), period/360, (period/6)%60, (period%6)*10));


    Show(true);

    ix = 0;
    for(graph_num=0; graph_num < N_PAGE_GRAPHS; graph_num++)
    {
        if(((graphtype = chart_page->graph[graph_num].type )== 0) || (graphtype >= n_graphtypes))
            continue;

        inv = chart_page->graph[graph_num].inverter;

        x = 0;
        value = 0;
        switch(graphtype)
        {
        case g_PowerTotal:
            if(inverter_address[1] == 0)
                continue;   // don't show if there is only one inverter

            if((graph_data[0] != NULL) && (graph_data[0][period].pw0 != 0xffff))
                value = graph_data[0][period].pw0;
            if((graph_data[1] != NULL) && (graph_data[1][period].pw0 != 0xffff))
                value += graph_data[1][period].pw0;
            value /= X_PO;
            break;

        case g_PowerOut:
            if((graph_data[inv] != NULL) && (graph_data[inv][period].pw0 != 0xffff))
                value = (double)graph_data[inv][period].pw0/X_PO;
            break;

        case g_Power1:
            if(graph_data[inv] != NULL)
                value = (double)graph_data[inv][period].pw1/X_PI;
            break;

        case g_Power2:
            if(graph_data[inv] != NULL)
                value = (double)graph_data[inv][period].pw2/X_PI;
            break;

        case g_PowerMin12:
            continue;   // don't include

        case g_PowerBalance:
            if(graph_data[inv] != NULL)
            {
                if((sum = graph_data[inv][period].pw1 + graph_data[inv][period].pw2) > 0)
                {
                    value = (double)graph_data[inv][period].pw1 * 100 * X_PI/X_PO / sum;
                }
            }
            break;

        case g_Voltage1:
            if(graph_data[inv] != NULL)
                value = (double)graph_data[inv][period].vt1/X_VI;
            break;

        case g_Voltage2:
            if(graph_data[inv] != NULL)
                value = (double)graph_data[inv][period].vt2/X_VI;
            break;

        case g_Current1:
            if((graph_data[inv] != NULL) && ((v = graph_data[inv][period].vt1) > 0))
                value = (double)graph_data[inv][period].pw1 * X_VI/X_PI/ v;
            break;

        case g_Current2:
            if((graph_data[inv] != NULL) && ((v = graph_data[inv][period].vt2) > 0))
                value = (double)graph_data[inv][period].pw2 * X_VI/X_PI/ v;
            break;

        case g_SunElevation:
            value = (double)sun_elevation[period]/100;
            break;

        case g_SolarIntensity:
            value = (double)sun_intensity[inv][period]/10;
            break;

        case g_Temperature:
            if(graph_data[inv] != NULL)
                value = (double)graph_data[inv][period].tmpr/100;
            break;

        case g_Efficiency:
            if(graph_data[inv] != NULL)
            {
                if((sum = graph_data[inv][period].pw1 + graph_data[inv][period].pw2) > 0)
                {
                    value = (double)graph_data[inv][period].pw0 * 100 * X_PI/X_PO / sum;
                }
            }
            break;

        default:
            if(dsp_data[inv][graph_types[graphtype].dsp_code] != NULL)
            {
                x = dsp_data[inv][graph_types[graphtype].dsp_code][period];
                value = (double)x / graph_types[graphtype].multiply;
            }
            break;
        }

        t_label[ix]->SetLabel(wxString(graph_types[graphtype].mnemonic, wxConvLocal));

        if(x == 0xffff)
        {
            t_value[ix]->SetLabel(wxEmptyString);
        }
        else
        {
            format_string.Printf(_T("%%.%df"), graph_types[graphtype].decimalplaces);
            t_value[ix]->SetLabel(wxString::Format(format_string, value));
        }
        ix++;
    }

    while(ix < 5)
    {
        t_label[ix]->SetLabel(wxEmptyString);
        t_value[ix]->SetLabel(wxEmptyString);
        ix++;
    }
}


void GraphPanel::OnMouse(wxMouseEvent& event)
{//==========================================
    long x, y;
    int scroll_x, scroll_y;
    int period;
    wxString date;

    SetFocus();
    event.GetPosition(&x, &y);
    GetViewStart(&scroll_x, &scroll_y);
    scroll_x *= N_SCROLL;

    if(chart_mode == 0)
    {
        if(event.RightDown() || event.LeftDClick())
        {
            if(dlg_coords->IsShown())
            {
                dlg_coords->Show(false);
                return;
            }
        }
        else
        if(dlg_coords->IsShown() == false)
            return;

        x += (scroll_x * 2);
        period = (int)(x * x_scale + start_ix + 0.5);
        dlg_coords->ShowDlg(chart_page, period);
    }
    else
    if(chart_mode == 1)
    {
        if(event.LeftDClick())
        {
            date = DateAtCursor(x + scroll_x);
            DisplayDateGraphs(date, 0);
        }
    }
}



void GraphPanel::SetMode(int mode)
{//===============================
    if(mode >= 0)
        chart_mode = mode;

    if(chart_mode != 0)
    {
        dlg_coords->Show(false);
    }
    if(chart_mode == 0)
    {
        ShowPage(page);
    }
    if(chart_mode == 1)
    {
        hist_range_ix = 6;
        SetColours(histogram_flags & 0xf);
    }
    SetExtent();
    Refresh();
}


void GraphPanel::ResetGraph(void)
{//==============================
    int page;
    CHART_PAGE *chart_page;

    for(page=0; page < N_PAGES; page++)
    {
        chart_page = &chart_pages[page];
        chart_page->range_ix = 0;
    }
}

void GraphPanel::ShowPage(int pagenum)
{//===================================
    page = pagenum;
    SetColours(chart_pages[page].flags & gBACK_COLOUR);
    Refresh();
}



void GraphPanel::SetExtent()
{//=========================
    int extent = 0;
    int scroll_x, scroll_y;
    int width, height;
    int centre;
    static int extent_prev = 0;
    static int mode_prev = 0;

    GetViewStart(&scroll_x, &scroll_y);
    GetClientSize(&width, &height);
    centre = width/2 + scroll_x * N_SCROLL;

    if(chart_mode == 0)
    {
        extent = sun.sunset;
        if(top_ix > extent)
            extent = top_ix;
        extent = (extent - start_ix) / x_scale;
    }
    else
    if(chart_mode == 1)
    {
        extent = HistogramExtent();
    }

    if(extent != extent_prev)
    {
        if(extent_prev > 0)
        {
            centre = (centre * extent) / extent_prev;
            scroll_x = (centre - width/2) / N_SCROLL;
        }
        if(chart_mode != mode_prev)
        {
            if(chart_mode == 1)
                scroll_x = 0x7fff;   // start with histogram scrolled to right
        }

        SetScrollbars(N_SCROLL, N_SCROLL, extent/N_SCROLL, 0, 0, 0);
        Scroll(scroll_x, -1);
        extent_prev = extent;
    }
}


void GraphPanel::OnKey(wxKeyEvent& event)
{//=======================================
    if(OnKey2(event) < 0)
        event.Skip();
}


int GraphPanel::OnKey2(wxKeyEvent& event)
{//=====================================
    int key;
    int count=0;
    double prev_scale;
    int control=0;
    int prev_page = page;
    int prev_histogram_page = histogram_page;
    int scroll_x, scroll_y;

    key = event.GetKeyCode();
    if(event.GetModifiers() == wxMOD_CONTROL)
        control = 1;

	switch(key)
	{
    case ',':
    case '<':
        if(chart_mode == 1)
        {
            hist_xscale_ix--;
            if(hist_xscale_ix < 0)
                hist_xscale_ix = 0;
        }
        else
        {
            prev_scale = x_scale;
            x_scale *= 1.1;
            if(x_scale > 4)
                x_scale = prev_scale;
        }
        SetExtent();
        break;

    case '.':
    case '>':
        if(chart_mode == 1)
        {
            hist_xscale_ix++;
            if(hist_xscale_ix >= N_HIST_SCALE)
                hist_xscale_ix = N_HIST_SCALE-1;
        }
        else
        {
            prev_scale = x_scale;
            x_scale /= 1.1;
            if(x_scale < 1)
                x_scale = prev_scale;
        }
        SetExtent();
        break;

    case WXK_LEFT:
        GetViewStart(&scroll_x, &scroll_y);
        Scroll(scroll_x-1, -1);
        break;

    case WXK_RIGHT:
        GetViewStart(&scroll_x, &scroll_y);
        Scroll(scroll_x+1, -1);
        break;

    case WXK_DOWN:
        if(chart_mode == 1)
        {
            hist_range_ix++;
            if(hist_range_ix > 10)
                hist_range_ix = 10;
        }
        else
        {
            chart_pages[page].range_ix++;
            if(chart_pages[page].range_ix > 4)
                chart_pages[page].range_ix = 4;
        }
        break;

    case WXK_UP:
        if(chart_mode == 1)
        {
            hist_range_ix--;
            if(hist_range_ix < 1)
                hist_range_ix = 1;
        }
        else
        {
            chart_pages[page].range_ix--;
            if(chart_pages[page].range_ix < -6)
                chart_pages[page].range_ix = -6;
        }
        break;

    case WXK_HOME:
        if(chart_mode == 1)
        {
            hist_range_ix = 6;
            histogram_page = 0;
        }
        else
        {
            chart_pages[page].range_ix = 0;
            page = 0;
            x_scale = -1;   // reset to default scale
            Scroll(0,0);
            start_ix = 0;
        }
        SetExtent();
        break;

    case WXK_END:
        if(chart_mode == 1)
        {
            histogram_page = N_HISTOGRAM_PAGES-1;
        }
        else
        {
            for(count=0; count<N_PAGES; count++)
            {
                // find the last in-use page
                if((inverter_address[1] == 0) && (chart_pages[count].status_type == 3))
                {
                    // ignore an inverter-B page if we only have onw inverter
                }
                else
                if(chart_pages[count].flags & gINUSE)
                    page = count;
            }
        }
        break;

    case WXK_PAGEUP:
        if(chart_mode == 1)
        {
            histogram_page--;

            if(histogram_page < 0)
                histogram_page = 0;
        }
        else
        {
            while(page > 0)
            {
                page--;
                if((inverter_address[1] == 0) && (chart_pages[count].status_type == 3))
                {
                    // ignore an inverter-B page if we only have onw inverter
                }
                else
                 if(chart_pages[page].flags & gINUSE)
                    break;
            }
        }
        break;

    case WXK_PAGEDOWN:
        if(chart_mode == 1)
        {
            histogram_page++;
            if(histogram_page >= N_HISTOGRAM_PAGES)
                histogram_page = N_HISTOGRAM_PAGES-1;
        }
        else
        {
            for(count=page+1; count<N_PAGES; count++)
            {
                if((inverter_address[1] == 0) && (chart_pages[count].status_type == 3))
                {
                    // ignore an inverter-B page if we only have onw inverter
                }
                else
                 if(chart_pages[count].flags & gINUSE)
                    break;
            }
            if(count < N_PAGES)
                page = count;
        }
        break;

    case WXK_F2:
        OpenLogFiles(-1, 1);
        graph_panel->SetMode(0);  // show Today
        break;

    case WXK_F3:
        SetMode(1);  // show histogram
        break;

    case WXK_F4:
        if(dlg_alarms->IsShown())
            dlg_alarms->Close();
        else
            ShowInverterStatus();
        break;

    case WXK_F11:
        if(mainframe->IsFullScreen())
            mainframe->ShowFullScreen(false);
        else
            mainframe->ShowFullScreen(true);
        mainframe->Refresh();
        break;

    case WXK_ESCAPE:
        mainframe->ShowFullScreen(false);
        mainframe->Refresh();
        break;

    case 'N':
        if(control)
        {
            DisplayDateGraphs(NextDate(graph_date_ymd, 1), 1);
        }
        break;

    case 'P':
        if(control)
        {
            DisplayDateGraphs(PrevDate(graph_date_ymd, 1), 1);
        }
        break;

    case ']':
        DisplayDateGraphs(NextDate(graph_date_ymd, 0), 1);
        break;

    case '[':
        DisplayDateGraphs(PrevDate(graph_date_ymd, 0), 1);
        break;

    default:
        return(-1);
	}

    if(chart_mode == 0)
    {
        if(page != prev_page)
        {
            SetColours(chart_pages[page].flags & gBACK_COLOUR);
            mainframe->SetupStatusPanel(0);
            if(dlg_coords->IsShown())
                dlg_coords->ShowDlg(&chart_pages[page], -1);
        }
    }
    if(chart_mode == 1)
    {
        if(histogram_page != prev_histogram_page)
            SetExtent();
    }
    Refresh();
	return(0);
}


int GetYGrads(int range)
{
    int ix;
    const int rng[] =  {10, 20, 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 99999};
    const int grad[] = { 1,  2,  2,  5,  10,  20,  25,  50,  100,  200,  250,  500,  1000,  2000};

    for(ix=0; ix<13; ix++)
    {
        if(rng[ix] >= range)
            return(grad[ix]);
    }
    return(2000);
}




static void AllocGraphPoints(int width)
{//====================================
    int inv;
    int size = width * sizeof(unsigned short);
    int ix;

    for(inv=0; inv < 2; inv++)
    {
        ns[inv] = (char *)realloc(ns[inv], width * sizeof(char));
        points_vt1[inv] = (unsigned short *)realloc(points_vt1[inv], size);
        points_vt2[inv] = (unsigned short *)realloc(points_vt2[inv], size);
        points_pw0[inv] = (unsigned short *)realloc(points_pw0[inv], size);
        points_pw1[inv] = (unsigned short *)realloc(points_pw1[inv], size);
        points_pw2[inv] = (unsigned short *)realloc(points_pw2[inv], size);
        points_tmpr[inv] = (short *)realloc(points_tmpr[inv], size);

        for(ix=0; ix<N_EXTRA_READINGS; ix++)
        {
            if(extra_readings[ix].graph_slot > 0)
            {
                points_dsp[inv][extra_readings[ix].dsp_code] = (unsigned short *)realloc(points_dsp[inv][extra_readings[ix].dsp_code], size);
            }
        }
    }
}


void GraphPanel::PlotGraphs(wxDC &dc, int height, int pass)
{//========================================================
    int ix, j, x;
    int y;
    int v;
    int sum;
    int graph_num;
    int inv;
    double yscale;
    int base, top, range;
    int draw_flag;
    int dsp_code;
    int addition;
    int multiply;
    wxColour colour;
    double scale;
    int graph_type;

    scale = x_scale;

    for(graph_num=0; graph_num < N_PAGE_GRAPHS; graph_num++)
    {
        if(pass == 1)
        {
            if(chart_page->graph[graph_num].scaletype != gBACKG)
                continue;
        }
        else
        {
            if(chart_page->graph[graph_num].scaletype == gBACKG)
                continue;
        }

        inv = chart_page->graph[graph_num].inverter;
        base = chart_page->graph[graph_num].base;
        top = chart_page->graph[graph_num].top;
        if(chart_page->graph[graph_num].type <= N_POWERGRAPHS)
        {
            //  a power graph, allow change of scale
            top = (int)(top * range_adjust[chart_page->range_ix+6]);
        }
        if(top == 0)
        {
            // automatic scale
            if(chart_page->graph[graph_num].type == g_SunElevation)
                top = sun.max_elev_year+2;
        }
        range = top-base;
        yscale = (double)height / range;

        colour.Set(chart_page->graph[graph_num].colour);
        dc.SetPen(colour);
        dc.SetBrush(colour);
        draw_flag = 1;

        switch((graph_type = chart_page->graph[graph_num].type))
        {
        case 0:
            draw_flag = 0;
            break;

        case g_SunElevation:
            // draw sun elevation
            for(ix=0; ix<(N_10SEC-1); ix++)
            {
                if(ix > start_ix)
                {
                    x = (int)((ix-start_ix)/scale);
                    if(x < nx2)
                    {
                        dc.DrawLine(x-x_scroll, height-((sun_elevation[ix]-base*100)*yscale)/100, x+1-x_scroll, height-((sun_elevation[ix+1]-base*100)*yscale)/100);
                    }
                }
            }
            x = (int)((sun.sunnoon-start_ix)/scale) - x_scroll;
            y = height-((sun_elevation[sun.sunnoon]-base*100)*yscale)/100;
#ifdef deleted
            // background for the labels
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(x-12, y+22, 35, 11);
            dc.DrawRectangle(x+1, y-14, 35, 11);
#endif
            dc.SetFont(Font_SunElevation);
            dc.DrawLine(x, y+20, x, y-20);
            dc.SetTextForeground(colour);
            dc.DrawText(wxString::Format(_T("%.2d:%.2d"), sun.noon_hrs, sun.noon_mins), x-14, y+20);

            y = y-16;
            if(y < 0) y = 0;
            dc.SetTextForeground(colour);
            dc.DrawText(wxString::Format(_T("%.2f"), sun.max_elev_day), x+2, y);
            draw_flag = 0;
            break;

        case g_SolarIntensity:
            for(ix=0; ix<(N_10SEC-1); ix++)
            {
                if(ix > start_ix)
                {
                    x = (int)((ix-start_ix)/scale);
                    if((x < nx2) || (sun_intensity[inv][ix] > 0))
                    {
                        dc.DrawLine(x-x_scroll, height-((sun_intensity[inv][ix]-base*10)*yscale)/10, x+1-x_scroll, height-((sun_intensity[inv][ix+1]-base*10)*yscale)/10);
                    }
                }
            }
            draw_flag = 0;
            break;

        case g_PowerTotal:
            if(inverter_address[1] == 0)
            {
                draw_flag = 0;
                break;   // don't draw Power Total if there's only 1 inverter
            }
            for(ix=0; ix<=nx; ix++)
            {
                y = 0;
                for(j=0; j<N_INV; j++)
                {
                    if((inverter_address[j] != 0) && (ns[j][ix] & 2))
                    {
                        y += points_pw0[j][ix];
                    }
                }
                points[ix].y = height-((y-base*X_PO) * yscale)/X_PO;
            }
            break;

        case g_PowerOut:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_pw0[inv][ix];
                if(ns[inv][ix] & 2)
                    points[ix].y = height - ((y - base*X_PO)*yscale)/X_PO;
                else
                    points[ix].y = height;
            }
            break;

        case g_Power1:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_pw1[inv][ix];
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*X_PI)*yscale)/X_PI;
                else
                    points[ix].y = height;
            }
            break;

        case g_Power2:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_pw2[inv][ix];
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*X_PI)*yscale)/X_PI;
                else
                    points[ix].y = height;
            }
            break;

        case g_PowerMin12:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_pw1[inv][ix];
                if(points_pw2[inv][ix] < y)
                    y = points_pw2[inv][ix];
                if(y < 0) y = -y;
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*X_PI)*yscale)/X_PI;
                else
                    points[ix].y = height;
            }
//            colour2.Set(0x2090ff);
            break;

        case g_Voltage1:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_vt1[inv][ix];
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*X_VI)*yscale)/X_VI;
                else
                    points[ix].y = height;
            }
            break;

        case g_Voltage2:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_vt2[inv][ix];
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*X_VI)*yscale)/X_VI;
                else
                    points[ix].y = height;
            }
            break;

        case g_Current1:
            for(ix=0; ix<=nx; ix++)
            {
                if((v = points_vt1[inv][ix]) <= 0)
                    y = 0;
                else
                    y = points_pw1[inv][ix] * 1000 * X_VI/X_PI/ v;
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*1000)*yscale)/1000;
                else
                    points[ix].y = height;
            }
            break;

        case g_Current2:
            for(ix=0; ix<=nx; ix++)
            {
                if((v = points_vt2[inv][ix]) <= 0)
                    y = 0;
                else
                    y = points_pw2[inv][ix] * 1000 * X_VI/X_PI / v;
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*1000)*yscale)/1000;
                else
                    points[ix].y = height;
            }
            break;

        case g_Temperature:
            for(ix=0; ix<=nx; ix++)
            {
                y = points_tmpr[inv][ix];
                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*100)*yscale)/100;
                else
                    points[ix].y = height;
            }
            break;

        case g_Efficiency:
            for(ix=0; ix<=nx; ix++)
            {
                if((sum = points_pw1[inv][ix] + points_pw2[inv][ix]) == 0)
                    y = 0;
                else
                    y = (points_pw0[inv][ix] * 10000 * X_PI/X_PO) / sum;  // percentage * 100

                if(ns[inv][ix] & 3)
                    points[ix].y = height - ((y - base*100)*yscale)/100;
                else
                    points[ix].y = height;
            }
            break;

        case g_PowerBalance:
            for(ix=0; ix<=nx; ix++)
            {
                if((sum = points_pw1[inv][ix] + points_pw2[inv][ix]) <= 0)
                    y = 0;
                else
                    y = (points_pw1[inv][ix] * 10000) / sum;  // percentage * 100

                if(ns[inv][ix] & 1)
                    points[ix].y = height - ((y - base*100)*yscale)/100;
                else
                    points[ix].y = height;
            }
            break;

        default:
            if(graph_type >= n_graphtypes)
            {
                draw_flag = 0;
                break;
            }

            dsp_code = graph_types[graph_type].dsp_code;
            addition = graph_types[graph_type].addition;   // offset for temperatures to keep them positive
            multiply = graph_types[graph_type].multiply;
            if(points_dsp[inv][dsp_code] == NULL)
            {
                draw_flag = 0;
                break;
            }
            for(ix=0; ix<=nx; ix++)
            {
                y = points_dsp[inv][dsp_code][ix];
                if(y != 0xffff)
                    points[ix].y = height - ((y - (base+addition)*multiply)*yscale)/multiply;
                else
                    points[ix].y = 0x8000;
            }
            draw_flag = 2;
            break;
        }

        if(draw_flag)
        {
            colour.Set(chart_page->graph[graph_num].colour);
            dc.SetPen(colour);
            if(chart_page->graph[graph_num].style == gSOLID)
            {
                dc.SetBrush(colour);
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.DrawPolygon(nx+3, points);
            }
            else
            if(chart_page->graph[graph_num].style == gHATCH)
            {
                wxBrush hatch_brush(colour, wxFDIAGONAL_HATCH);
                dc.SetBrush(hatch_brush);
                dc.DrawPolygon(nx+3, points);
            }
            else
            {
                if(chart_page->graph[graph_num].style == gTHICK)
                {
                    wxPen thick_pen(colour, 2);
                    dc.SetPen(thick_pen);
                }
                else
                if(chart_page->graph[graph_num].style == gTHICK2)
                {
                    wxPen thick_pen(colour, 3);
                    dc.SetPen(thick_pen);
                }

                if(draw_flag == 2)
                {
                    // data is from dsp_data
                    for(ix=0; ix<nx; ix++)
                    {
                        if((points[ix].y != 0x8000) && (points[ix+1].y != 0x8000))
                            dc.DrawLine(points[ix].x, points[ix].y, points[ix+1].x, points[ix+1].y);
                    }
               }
                else
                {
                    for(ix=0; ix<nx; ix++)
                    {
                        if((ns[inv][ix] & 1) && (ns[inv][ix+1] & 1))
                            dc.DrawLine(points[ix].x, points[ix].y, points[ix+1].x, points[ix+1].y);
                    }
                }
            }

#ifdef deleted
            if((chart_page->graph[graph_num].scaletype == gLEFT) && ((graph_type == g_PowerOut) || (graph_type == g_PowerTotal)))
            {
                // draw stored data imported from the inverter ( aurora -q )
                dc.SetPen(*wxRED_PEN);
                for(ix=0; ix<nx; ix++)
                {
                    if((ns[inv][ix] & 1) && (ns[inv][ix+1] & 1))
                        dc.DrawLine(points[ix].x, height - ((points_pwi[ix] - base*10)*yscale)/10, points[ix+1].x, height - ((points_pwi[ix+1] - base*10)*yscale)/10);
                }
            }
#endif
        }
    }
}  // end of PlotGraphs



void GraphPanel::OnDraw(wxDC &dc)
{//=============================
    int inv=0;
    int width, height;
    int ix, j, k, ex;
    int x, y;
    int pw0, pw1, pw2, pwi, vt1, vt2, tmpr;
    int n_samples;
    int n_samples_pw0;
    int extras[N_EXTRA_READINGS];
    int n_samples_extras[N_EXTRA_READINGS];
    double scale;
    int iscale;
    double remainder;
    int graph_num;
    double yscale;
    int base, top, range;
    wxString string;
    wxColour colour;
    int x_grads;
    int margin = 20;

    if(enable_draw == 0)
        return;  // don't draw until startup is complete

    GetViewStart(&x_scroll, &y_scroll);
    x_scroll *= N_SCROLL;

    if(chart_mode == 1)
    {
        DrawEnergyHistogram(dc);
        return;
    }

    GetClientSize(&width,&height);
    if((width + 2) > draw_width)
    {
        draw_width = width + 2;
        AllocGraphPoints(draw_width);
    }
    for(ix=0; ix < N_EXTRA_READINGS; ix++)
    {
        if(extra_readings[ix].graph_slot > 0)
        {
            for(inv=0; inv<2; inv++)
            {
                if(points_dsp[inv][extra_readings[ix].dsp_code] == NULL)
                    points_dsp[inv][extra_readings[ix].dsp_code] = (unsigned short *)malloc(width * sizeof(unsigned short));

            }
        }
    }

    memset(ns[0], 0, draw_width * sizeof(char));
    memset(ns[1], 0, draw_width * sizeof(char));

    if(x_scale < 1)
    {
        // the scale has not yet been set
        j = sun.sunset - sun.sunrise + margin;
        x_scale = (double)j / width;
    }
    if(x_scale < 1)
        x_scale = 1;

    chart_page = &chart_pages[page];

    scale = x_scale;

    if((start_ix > (sun.sunrise - margin)) || (start_ix == 0))
        start_ix = sun.sunrise - margin;

    if(top_ix < start_ix)
        top_ix = start_ix;
    ix = top_ix;
    nx = (int)((ix - start_ix)/scale);
    if(ix < sun.sunset)
        ix = sun.sunset;
    nx2 = (int)((ix +1 - start_ix)/scale);

    SetExtent();

    dc.SetTextForeground(colour_text1);
    dc.SetTextBackground(colour_backg);

    nx += x_scroll;
    if(nx < 0)
        nx = 0;
    if(nx >= width)
        nx = width-1;
    for(inv=0; inv<N_INV; inv++)
    {
        if((inverter_address[inv] <= 0) || (graph_data[inv] == NULL))
            continue;

        remainder = 0;
        for(ix=0; ix<=nx; ix++)
        {
            points[ix].x = ix + x_scroll;
            pw0 = pw1 = pw2 = pwi = 0;
            vt1 = vt2 = tmpr = 0;
            for(ex=0; ex<N_EXTRA_READINGS; ex++)
            {
                extras[ex] = 0;
                n_samples_extras[ex] = 0;
            }
            n_samples = 0;
            n_samples_pw0 = 0;

            iscale = (int)(scale + remainder);
            remainder = (scale + remainder) - iscale;
            for(j=0; j<scale; j++)
            {
                k = ((ix+x_scroll*2)*scale + j) + start_ix;
                if(k <= top_ix)
                {
                    if(graph_data[inv][k].pw0 != 0xffff)
                    {
                        pw0 += graph_data[inv][k].pw0;
                        n_samples_pw0++;
                    }
                    if(graph_data[inv][k].pw1 != 0xffff)
                    {
                        pw1 += graph_data[inv][k].pw1;
                        pw2 += graph_data[inv][k].pw2;
                        vt1 += graph_data[inv][k].vt1;
                        vt2 += graph_data[inv][k].vt2;
                        tmpr += graph_data[inv][k].tmpr;
                        pwi += graph_data[0][k].pwi;
                        n_samples++;
                    }

                    for(ex=0; ex<N_EXTRA_READINGS; ex++)
                    {
                        if(extra_readings[ex].graph_slot > 0)
                        {
                            if(dsp_data[inv][extra_readings[ex].dsp_code] != NULL)
                            {
                                unsigned short value;
                                if((value = dsp_data[inv][extra_readings[ex].dsp_code][k]) != 0xffff)
                                {
                                    extras[ex] += value;
                                    n_samples_extras[ex]++;
                                }
                            }
                        }
                    }
                }
            }

            points_pw0[inv][ix] = 0;
            ns[inv][ix] = 0;

            if(n_samples_pw0 > 0)
            {
                points_pw0[inv][ix] = pw0 / n_samples_pw0;
                ns[inv][ix] |= 2;
            }

            if(n_samples > 0)
            {
                points_pw1[inv][ix] = pw1 / n_samples;
                points_pw2[inv][ix] = pw2 / n_samples;
                points_vt1[inv][ix] = vt1 / n_samples;
                points_vt2[inv][ix] = vt2 / n_samples;
                points_tmpr[inv][ix] = tmpr / n_samples;
                points_pwi[ix] = pwi / n_samples;
                ns[inv][ix] |= 1;
            }

            for(ex=0; ex<N_EXTRA_READINGS; ex++)
            {
                if(extra_readings[ex].graph_slot > 0)
                {
                    unsigned short *p;
                    if((p = points_dsp[inv][extra_readings[ex].dsp_code]) != NULL)
                    {
                        if(n_samples_extras[ex] == 0)
                            p[ix] = 0xffff;
                        else
                            p[ix] = extras[ex] / n_samples_extras[ex];
                    }
                }
            }

        }
    }

    // This is just for solid graphs, ensure the edges of gaps are vertical
    for(ix=1; ix<nx; ix++)
    {
        if((ns[0][ix] == 0) && (ns[1][ix] == 0))
        {
            // No data for this period.  Any data for the previous period?
            if((ns[0][ix-1] > 0) || (ns[1][ix-1] > 0))
                points[ix].x = points[ix-1].x;
        }
        else
        {
            // Data for this period.  Any data for the previous period?
            if((ns[0][ix-1] == 0) && (ns[1][ix-1] == 0))
                points[ix-1].x = points[ix].x;
        }
    }

    if(nx >= 0)
    {
        points[nx+1].x = nx + x_scroll;
        points[nx+2].x = 0 + x_scroll;
        points[nx+1].y = height+1;
        points[nx+2].y = height+1;
    }

    // draw graphs which are behind the grid
    PlotGraphs(dc, height, 1);

    //draw horizontal scale
    x_grads = 360;
    if(scale < 2)
		x_grads = 90;
	else
	if(scale < 3)
		x_grads = 180;
    for(ix=0; ix <= N_10SEC; ix += x_grads)
    {
        x = (int)((ix - start_ix) /scale);
        if(x < -2)
            continue;

        if((ix % 360) == 0)
            dc.SetPen(colour_grid1);
        else
            dc.SetPen(colour_grid2);
        dc.DrawLine(x-x_scroll, 0, x-x_scroll, height);
        if((scale < 1.4) || ((ix % 180) == 0))
            dc.DrawText(wxString::Format(_("%2d:%.2d"), ix/360, (ix % 360)/6), x-x_scroll+2, 1);
    }

    // draw vertical scale
    for(graph_num=0; graph_num < N_PAGE_GRAPHS; graph_num++)
    {
        int grads;
        int scaletype;
        int graphtype;

        if(((graphtype = chart_page->graph[graph_num].type )== 0) || (graphtype >= n_graphtypes))
            continue;

        if((scaletype = chart_page->graph[graph_num].scaletype) != 0)
        {
            dc.SetTextForeground(colour_text1);
            base = chart_page->graph[graph_num].base;
            top = chart_page->graph[graph_num].top;
            if(graphtype <= N_POWERGRAPHS)
            {
                //  a power graph, allow change of scale
                top = (int)(top * range_adjust[chart_page->range_ix+6]);
            }
            if(top == 0)
            {
                // automatic scale
                if(graphtype == g_SunElevation)
                    top = sun.max_elev_year+2;
            }
            range = top-base;
            grads = GetYGrads(range);
            yscale = (double)height / range;
            for(ix = (base / grads) * grads; ix<top; ix+=grads)
            {
                if(ix > base)
                {
                    y = height - (ix-base)*yscale;

                    if(graphtype == g_PowerBalance)
                        string = wxString::Format(_T("%3d%%"), ix);
                    else
                        string = wxString::Format(_T("%3d"), ix);

                    if(scaletype == gLEFT)
                    {
                        dc.SetPen(colour_grid1);
                        dc.DrawLine(x_scroll, y, x_scroll+width, y);
                        dc.DrawText(string, x_scroll, y);
                    }
                    else
                    if(scaletype == gRIGHT)
                    {
                        colour.Set(chart_page->graph[graph_num].colour);
                        dc.SetTextForeground(colour);
                        dc.SetPen(colour);
                        dc.DrawLine(x_scroll+width-30, y, x_scroll+width, y);
                        dc.DrawText(string, x_scroll+width-26, y);
                    }
                }
            }
        }
    }
    // draw graphs which are in front of the grid
    PlotGraphs(dc, height, 2);

    // draw the date and chart name
    {
        wxCoord h;
        double energy;

        y = 16;
        dc.SetTextForeground(colour_text1);
        dc.SetFont(Font_GraphDate);
        dc.GetTextExtent(graph_date, &graph_date_width, &h);
        dc.DrawText(graph_date, x_scroll+16, y);
        dc.SetFont(Font_ChartName);
        dc.DrawText(wxString(chart_page->name, wxConvLocal), x_scroll+16, y+32);

        if(strcmp(ymd_energy, graph_date_ymd.mb_str(wxConvLocal)) == 0)
        {
            dc.SetTextForeground(colour_text2);
            dc.DrawText(wxString::Format(_T("%5.3f kWh"), inverters[0].energy_total[0] + inverters[1].energy_total[0]), x_scroll+graph_date_width+31, y+7);
        }
        else
        {
            // showing a previous date, show the energy total for that date
            energy = (double)DateEnergy(graph_date_ymd) / 1000.0;
            if(energy > 0)
            {
                dc.SetTextForeground(colour_text1);
//                dc.DrawText(wxString::Format(_T("%5.2f kWh"), energy), graph_date_width+24, y+7);
                dc.DrawText(wxString::Format(_T("%5.3f kWh"), energy), x_scroll+graph_date_width+31, y+7);
            }
        }
    }
}  // end of GraphPanel::OnDraw



void GraphPanel::AddPoint(int inverter, int time, DATAREC *datarec, int control)
{//=============================================================================
    // control:  bit 0  update energy total
    int width, height;
    int x_scroll, y_scroll;
    int x;
    int w;
    static int first = 1;

    if(datarec == NULL)
    {
        // only refresh the energy total at the top left
        RefreshRect(wxRect(graph_date_width, 23, 144, 24));  // update the energy total
        return;
    }

    if(graph_data[inverter] == NULL)
    {
        graph_data[inverter] = (DATAREC *)calloc(N_10SEC, sizeof(DATAREC));
        start_ix = time;
        for(x=0; x<N_10SEC; x++)
        {
            graph_data[inverter][x].pw0 = 0xffff;
            graph_data[inverter][x].pw1 = 0xffff;
        }
    }

    memcpy(&graph_data[inverter][time], datarec, sizeof(DATAREC));

    if(time >= start_ix)
        top_ix = time;
    GetClientSize(&width,&height);
    GetViewStart(&x_scroll, &y_scroll);
    x_scroll *= N_SCROLL;
    x = (int)((top_ix - start_ix)/x_scale) - (x_scroll*2);

    if(first)
    {
        Refresh();   // ensure grid is drawn
        first = 0;
    }
    else
    {
        if(control & 1)
        {
            RefreshRect(wxRect(graph_date_width, 23, 144, 24));  // update the energy total
        }
        w = ((double)CLIPPING_WIDTH + 0.5)/ x_scale;
        RefreshRect(wxRect(x-w, 0, w+1, height));
    }
}  // end of AddPoint



void GraphPanel::NewDay(void)
{//==========================
    int inv;
    int ix;
    int j;
    unsigned short *p;

    newday = 1;

    for(inv=0; inv<2; inv++)
    {
        if(graph_data[inv] != NULL)
            memset(graph_data[inv], 0, N_10SEC * sizeof(DATAREC));

        for(ix=0; ix<N_DSP; ix++)
        {
            if((p = dsp_data[inv][ix]) != NULL)
            {
                for(j=0; j<N_10SEC; j++)
                    p[j] = 0xffff;
            }

            if((p = points_dsp[inv][ix]) != NULL)
            {
                for(j=0; j<draw_width; j++)
                    p[j] = 0xffff;
            }
        }
    }
}


typedef struct {
    float power[3];
    float voltage[3];
    float current[3];
    float temperature;
    float temperature2;
    float extra[8];
} DATA_IN;

int GraphPanel::ReadEnergyFile(int inv, wxString filename, int control, const char *date_str)
{//==========================================================================================
// control: bit 0: keep previous start_ix if it's less than the new value

    FILE *f_in;
    int ix;
    int hours, mins, secs;
    int timeix;
    int prev_ix;
    int prev_ix_pw0;
    int first_ix;
    int any_data = 0;
    int data_type = 0;  // 1=log, 2= binary log, 3=logfull, 4= aurora communicator, 5= aurora -q,
    int gap;
    char *p_time;
    DATA_IN d;
    DATAREC *gd;
    DATAREC *gp;
    BINARY_LOG bdata;
    char buf[256];
    char date_str2[20];
    char date_match[20];
    float data_pw0[N_10SEC];   // 10sec periods in 1day
    float data_pw1[N_10SEC];
    float data_vt1[N_10SEC];
    float data_pw2[N_10SEC];
    float data_vt2[N_10SEC];
    float data_tmpr[N_10SEC];
    float data_pwi[N_10SEC];
    unsigned char n_samples[N_10SEC];
    unsigned char n_samples_pw0[N_10SEC];
    int column_types[16];
    int column_dsp[16];

    memset(column_types, 0, sizeof(column_types));
    column_types[6] = 1;  // default is Grid_V only, extra_reading_types index for Grid_V
    column_dsp[6] = DSP_GRID_VOLTS;    // dsp number for Grid_V

    if(date_str == NULL)
    {
        date_str2[0] = 0;
    }
    else
    if(date_str[4] == '-')
    {
        memcpy(date_str2, date_str, 4);
        date_str2[4] = date_str[5];
        date_str2[5] = date_str[6];
        date_str2[6] = date_str[8];
        date_str2[7] = date_str[9];
    }
    else
    {
        strcpy(date_str2, date_str);
    }

    if((inv >= 0) && (inv < N_INV) && graph_data[inv] != NULL)
    {
        if(graph_data[inv] != NULL)
        {
            free(graph_data[inv]);
            graph_data[inv] = NULL;
        }
    }


    strcpy(buf, filename.mb_str(wxConvLocal));
    if((f_in = fopen(buf, "rb")) == NULL)
    {
        // also look for file with .txt or .dat extension ('filename' may already have an extension)
        strcat(buf, ".txt");
        if((f_in = fopen(buf, "rb")) == NULL)
        {
            buf[strlen(buf)-4] = 0;
            strcat(buf,".dat");
            if((f_in = fopen(buf, "rb")) == NULL)
            {
                Refresh();
                return(-1);
            }
        }
    }

    memset(data_pw0, 0, sizeof(data_pw0));
    memset(data_pw1, 0, sizeof(data_pw1));
    memset(data_vt1, 0, sizeof(data_vt1));
    memset(data_pw2, 0, sizeof(data_pw2));
    memset(data_vt2, 0, sizeof(data_vt2));
    memset(data_tmpr, 0, sizeof(data_tmpr));
    memset(data_pwi, 0, sizeof(data_pwi));
    memset(n_samples, 0, sizeof(n_samples));
    memset(n_samples_pw0, 0, sizeof(n_samples_pw0));

    data_type = 0;

    if(fgets(buf, sizeof(buf), f_in) == NULL)
    {
        buf[0] = 0;
    }
    if(memcmp(buf, "AUR1 ", 5) == 0)
    {
        data_type = 1;
        p_time = buf;
        sscanf(&buf[5], "%s", date_str2);
        memset(data_pwi, 0, sizeof(data_pwi));
    }
    else
    if(memcmp(buf, "AUR2 ", 5) == 0)
    {
        data_type = 2;
        p_time = buf;
        sscanf(&buf[5], "%s", date_str2);
        memset(data_pwi, 0, sizeof(data_pwi));
    }
    else
    if(memcmp(&buf[3], "Time\tPower (W)", 14) == 0)
    {
        // Aurora Communicator exported data
        data_type = 4;
        p_time = &buf[11];
        date_match[0] = date_str2[6];
        date_match[1] = date_str2[7];
        date_match[2] = '/';
        date_match[3] = date_str2[4];
        date_match[4] = date_str2[5];
        date_match[5] = '/';
        date_match[6] = date_str2[0];
        date_match[7] = date_str2[1];
        date_match[8] = date_str2[2];
        date_match[9] = date_str2[3];
        date_match[10] = 0;
    }
    else
    {
        if(strlen(buf) > 40)
        {
            data_type = 3;  // logfull
            memcpy(date_str2, &buf[0], 8);
            date_str2[8] = 0;
        }
        else
        {
            data_type = 5;  // aurora -q
            strcpy(date_match, date_str2);
        }
        rewind(f_in);
        p_time = &buf[9];
    }

    graph_date_ymd = wxString(date_str2, wxConvLocal);
    graph_date = MakeDateString(graph_date_ymd);

    first_ix = start_ix;
    if((data_type == 1) || (data_type == 2) || (data_type == 3))
    {
        if(newday == 1)
        {
            newday = 0;
            first_ix = N_10SEC*2;  // larger than any max value for first_ix
            top_ix = 0;
        }
    }

    if(data_type == 2)
    {
        // binary log data
        if(inv >= 0)
        {
            if((graph_data[inv] = (DATAREC *)realloc(graph_data[inv], N_10SEC * sizeof(DATAREC))) == NULL)
            {
                return(-3);
            }
        }
        for(ix=0; ix<N_10SEC; ix++)
        {
            graph_data[inv][ix].pw0 = 0xffff;   // indicates no value
        }

        while(!feof(f_in))
        {
            if(fread(&bdata, sizeof(bdata), 1, f_in) == 1)
            {
                if((timeix = bdata.time) >= N_10SEC)
                    continue;

                gd = &graph_data[inv][timeix];
                gd->pw0 = bdata.pw0;
                gd->pw1 = bdata.pw1;
                gd->vt1 = bdata.vt1;
                gd->pw2 = bdata.pw2;
                gd->vt2 = bdata.vt2;
                gd->tmpr = bdata.tmpr;

                if((bdata.pw1 > 0) || (bdata.pw2 > 0))
                {
                    if(timeix < first_ix)
                        first_ix = timeix;
                    if(timeix > top_ix)
                        top_ix = timeix;
                }
            }
        }
        fclose(f_in);
        if(((control & 1) == 0) || (first_ix < start_ix))
        {
            start_ix = first_ix;
        }
        Refresh();
        return(0);
    }


    while(fgets(buf, sizeof(buf), f_in) != NULL)
    {
        int n;
        int column;
        float discard;
        char cn[16][20];
        int dsp_code;
        int type;
        EXTRA_READING_TYPE *ptype;

        if((data_type == 1) || (data_type == 3))
        {
            memset(&d, 0, sizeof(d));

            if(data_type == 1)
            {
                if(buf[0] == '#')
                    continue;

                if(memcmp(buf, "time\t", 5) == 0)
                {
                    // column headers
                    n = sscanf(&buf[5], "%s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                               cn[0], cn[1], cn[2], cn[3], cn[4], cn[5],
                               cn[6], cn[7], cn[8], cn[9], cn[10], cn[11], cn[12], cn[13]);
                    for(column=6; column<n; column++)
                    {
                        column_types[column] = 0;
                        for(ix=0; ix<N_EXTRA_READINGS; ix++)
                        {
                            if((type = extra_readings[ix].type) > 0)
                            {
                                int j;
                                unsigned short *p;
                                if(strcmp(cn[column], extra_reading_types[type].mnemonic) == 0)
                                {
                                    column_types[column] = extra_readings[ix].type;
                                    column_dsp[column] = dsp_code = extra_readings[ix].dsp_code;
                                    if(dsp_data[inv][dsp_code] == NULL)
                                    {
                                        dsp_data[inv][dsp_code] = p = (unsigned short *)malloc(sizeof(unsigned short) * N_10SEC);
                                        for(j=0; j<N_10SEC; j++)
                                            p[j] = 0xffff;
                                    }
                                }
                            }
                        }
                    }
                    continue;
                }

                n = sscanf(p_time, "%d:%d:%d %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                            &hours, &mins, &secs, &d.power[0], &d.power[1], &d.voltage[1], &d.power[2], &d.voltage[2], &d.temperature,
                            &d.extra[0], &d.extra[1], &d.extra[2], &d.extra[3], &d.extra[4], &d.extra[5], &d.extra[6], &d.extra[7]);

            }
            else
            {
                n = sscanf(p_time, "%d:%d:%d %f %f %f %f %f %f %f %f %f %f %f %f %f", &hours, &mins, &secs,
                            &d.voltage[1], &d.current[1], &d.power[1], &d.voltage[2], &d.current[2], &d.power[2],
                            &d.voltage[0], &d.current[0], &d.power[0], &discard, &discard, &d.temperature, &d.temperature2);
                if(d.temperature2 > d.temperature)
                    d.temperature = d.temperature2;
                if(n >= 6) n = 6;
            }

            if(n >= 4)
            {
                timeix = (hours * 3600) + (mins * 60) + secs;
                timeix = timeix /10;
                if(timeix < N_10SEC)
                {
                    data_pw0[timeix] += d.power[0];
                    n_samples_pw0[timeix]++;

                    if((d.power[0] > 0) || (d.power[1] > 0))
                    {
                        if(timeix < first_ix)
                            first_ix = timeix;
                        if(timeix > top_ix)
                            top_ix = timeix;
                        any_data++;
                    }

                    if(n > 4)
                    {
                        n_samples[timeix]++;
                        data_pw1[timeix] += d.power[1];
                        data_vt1[timeix] += d.voltage[1];
                        data_pw2[timeix] += d.power[2];
                        data_vt2[timeix] += d.voltage[2];
                        data_tmpr[timeix] += d.temperature;

                        for(column=6; column<(n-3); column++)
                        {
                            if(column_types[column] > 0)
                            {
                                ptype = &extra_reading_types[column_types[column]];
                                dsp_data[inv][column_dsp[column]][timeix] = (int)((d.extra[column-6] + ptype->addition) * ptype->multiply);
                            }
                        }
                    }
                }
            }
        }
        else
        if((memcmp(buf, date_match, 8) == 0) || (memcmp(buf, date_match, 10) == 0))
        {
            // this is for the required date
            d.power[0] = 0;
            if(sscanf(p_time, "%d:%d:%d %f", &hours, &mins, &secs, &d.power[0]) == 4)
            {
                timeix = (hours * 3600) + (mins * 60) + secs;
                timeix = timeix /10;
                if(timeix < N_10SEC)
                {
                    data_pwi[timeix] = d.power[0];

                    if(d.power[0] > 0.01)
                    {
                        if(timeix < first_ix)
                            first_ix = timeix;
                        if(timeix > top_ix)
                            top_ix = timeix;
                        any_data++;
                    }
                }
            }
        }
    }

    fclose(f_in);

    if(any_data == 0)
    {
        Refresh();
        return(-2);
    }
    if(inv >= 0)
    {
        if((graph_data[inv] = (DATAREC *)realloc(graph_data[inv], N_10SEC * sizeof(DATAREC))) == NULL)
        {
            Refresh();
            return(-3);
        }
    }

    if((data_type == 1) || (data_type == 3))
    {
        int j;
        int dsp_code;
        unsigned short *p;
        double diff;

        prev_ix = -1;
        prev_ix_pw0 = -1;

        for(dsp_code=1; dsp_code < N_DSP; dsp_code++)
        {
            if(dsp_data[inv][dsp_code] == NULL)
                continue;

            p = dsp_data[inv][dsp_code];

            for(ix=0; ix<N_10SEC; ix++)
            {
                if(p[ix] != 0xffff)
                {
                    gap = ix - prev_ix;
                    if((prev_ix > 0) & (gap > 1) && (gap < MAX_GAP))
                    {
                        diff = (double)(p[ix] - p[prev_ix]) / gap;
                        for(j=1; j < gap; j++)
                        {
                            p[prev_ix+j] = (unsigned short)(p[prev_ix] + diff * j);
                        }
                    }
                    prev_ix = ix;
                }
            }
        }

        for(ix=0; ix<N_10SEC; ix++)
        {
            double diff_pw0, diff_pw1, diff_vt1, diff_pw2, diff_vt2, diff_tmpr;
            gd = &graph_data[inv][ix];
            memset(&gd[0], 0, sizeof(gd[0]));
            gd[0].pw0 = 0xffff;
            gd[0].pw1 = 0xffff;

            if(n_samples_pw0[ix] > 0)
            {
                gd[0].pw0 = (unsigned int)(data_pw0[ix] * X_PO / n_samples_pw0[ix]);

                gap = ix - prev_ix_pw0;
                if((prev_ix_pw0 > 0) && (gap > 1) && (gap < MAX_GAP))
                {

                    gp = &graph_data[inv][prev_ix_pw0];

                    diff_pw0 = (double)(gd[0].pw0 - gp[0].pw0) / gap;

                    for(j=1; j < gap; j++)
                    {
                        gp[j].pw0 = gp[0].pw0 + diff_pw0*j;
                    }
                }
                prev_ix_pw0 = ix;
            }

            if(n_samples[ix] > 0)
            {
                gd[0].pw1 = (unsigned int)(data_pw1[ix] * X_PI / n_samples[ix]);
                gd[0].vt1 = (unsigned int)(data_vt1[ix] * X_VI / n_samples[ix]);
                gd[0].pw2 = (unsigned int)(data_pw2[ix] * X_PI / n_samples[ix]);
                gd[0].vt2 = (unsigned int)(data_vt2[ix] * X_VI / n_samples[ix]);
                gd[0].tmpr = (int)(data_tmpr[ix] * 100 / n_samples[ix]);
                gd[0].pwi = 0;

                gap = ix - prev_ix;
                if((prev_ix > 0) && (gap > 1) && (gap < MAX_GAP))
                {
                    gp = &graph_data[inv][prev_ix];

                    diff_pw1 = (double)(gd[0].pw1 - gp[0].pw1) / gap;
                    diff_vt1 = (double)(gd[0].vt1 - gp[0].vt1) / gap;
                    diff_pw2 = (double)(gd[0].pw2 - gp[0].pw2) / gap;
                    diff_vt2 = (double)(gd[0].vt2 - gp[0].vt2) / gap;
                    diff_tmpr = (double)(gd[0].tmpr - gp[0].tmpr) / gap;

                    for(j=1; j < gap; j++)
                    {
                        gp[j].pw1 = gp[0].pw1 + diff_pw1*j;
                        gp[j].vt1 = gp[0].vt1 + diff_vt1*j;
                        gp[j].pw2 = gp[0].pw2 + diff_pw2*j;
                        gp[j].vt2 = gp[0].vt2 + diff_vt2*j;
                        gp[j].tmpr = gp[0].tmpr + diff_tmpr*j;
                    }
                }
                prev_ix = ix;
            }
        }
    }
    else
    if(data_type == 4)
    {
        // Aurora Communicator exported data
        for(ix=0; ix<(N_10SEC-48); ix++)
        {
            graph_data[0][ix].pwi = (int)(data_pwi[ix] * 10);
        }
    }
    else
    if(data_type == 5)
    {
        //  aurora -q data
        for(ix=0; ix<(N_10SEC-48); ix++)
        {
            graph_data[0][ix].pwi = (int)(data_pwi[ix+8] * 10);
        }
    }

    if(((control & 1) == 0) || (first_ix < start_ix))
    {
        start_ix = first_ix;
    }
    Refresh();
    return(0);
}



int ConvertLogBinary(wxString fname)
{//=================================
    FILE *f_in;
    FILE *f_out;
    int n;
    int hrs, mins, secs;
    int time10sec;
    wxString fname_out;
    char buf[256];

    BINARY_LOG data;

    float d[8];

    if(fname.Right(4) != _T(".txt"))
    {
        return(1);
    }
    if((f_in = fopen(fname.mb_str(wxConvLocal), "r")) == NULL)
    {
        return(2);  // can't read file
    }
    if((fgets(buf, sizeof(buf), f_in) == NULL) || (memcmp(buf, "AUR1 2", 4) != 0))
    {
        return(3);  // not a textfile log
    }

    fname_out = fname.BeforeLast('.') + _T(".dat");
    if((f_out = fopen(fname_out.mb_str(wxConvLocal), "wb")) == NULL)
    {
        fclose(f_in);
        return(4);  // can't create binary log file
    }

    buf[3] = '2';   // change AUR1 to AUR2
    fwrite(buf, 1, strlen(buf), f_out);  // copy the first line of text

    while(fgets(buf, sizeof(buf), f_in) != NULL)
    {
        n = sscanf(buf, "%d:%d:%d %f %f %f %f %f %f %f", &hrs, &mins, &secs, &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7]);
        if(n < 10)
            continue;

        time10sec = hrs*360 + mins*6 + secs/10;
        if((time10sec < 0) || (time10sec >= N_10SEC))
            continue;

        data.time = time10sec;
        data.pw0 = (unsigned int)(d[1] * X_PO + 0.5);   // power output
        data.pw1 = (unsigned int)(d[2] * X_PI + 0.5);   // power input 1
        data.vt1 = (unsigned int)(d[3] * X_VI + 0.5);  // voltage input 1
        data.pw2 = (unsigned int)(d[4] * X_PI + 0.5);   // power input 2
        data.vt2 = (unsigned int)(d[5] * X_VI + 0.5);  // voltage input 2
        data.tmpr = (int)(d[6] * 100 + 0.5);  // temperature
        data.spare = 0;

        fwrite(&data, sizeof(data), 1, f_out);
    }
    fclose(f_in);
    fclose(f_out);
    return(0);
}  // end of ReadEnergyFile


int SavePvoutput(int inv)
{//======================
// Save the day's data for export to www.pvoutput.org
    double energy;
    double total;
    int ix;
    FILE *f;
    wxString fname;

    fname = data_year_dir2 + _T("/pvoutput_") + graph_date_ymd;
    f = fopen(fname.mb_str(wxConvLocal), "w");
    if(f == NULL)
    {
        return(-1);
    }

    total = 0;
    for(ix=graph_panel->start_ix; ix <= graph_panel->top_ix; ix++)
    {
        if(graph_data[inv][ix].pw0 != 0xffff)
        {
            energy = ((double)graph_data[inv][ix].pw0 / (X_PO * 360));
            total += energy;
        }
        if((total > 0) && ((ix % 30) == 0))
        {
            // 5 minute period
            fprintf(f, "%2d:%.2d\t%d\n", ix/360, (ix/6) % 60, (int)total);
        }
    }

    fclose(f);
    return(0);
}


void InitCharts()
{//==============
    memset(points_dsp, 0, sizeof(points_dsp));
}

