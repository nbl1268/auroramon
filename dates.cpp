

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


#include <wx/filefn.h>
#include <wx/filename.h>
#include "auroramon.h"

#define YEAR_FIRST  2000
#define N_YEARS     100

int year_today;
int yearix_first;
wxArrayString years;

static int collected_dates = 0;
unsigned char *yeartab_flags[N_YEARS];   // bit 0 - we have some power data, bits 4,5 inv0,inv1 energy has an average value
unsigned short *yeartab_energy[N_INV][N_YEARS];

static short monthstart1[14] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 396};  // non-leap years
static short monthstart2[14] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366, 397};  // leap years

wxFont Font_Year(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
wxFont Font_Default(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);


void InitDates()
{//=============
    struct tm *btime;
    time_t current_time;

    time(&current_time);
    btime = localtime(&current_time);
    year_today = btime->tm_year+1900;
    yearix_first = year_today;

    collected_dates = 0;
    memset(yeartab_flags, 0, sizeof(yeartab_flags));
    memset(yeartab_energy, 0, sizeof(yeartab_energy));
}


int NYearDays(int year, short **m)
{//===============================
    if((year % 4) == 0)
    {
        if(m != NULL)
            *m = monthstart2;
        return(366);
    }
    if(m != NULL)
        *m = monthstart1;
    return(365);
}

int DayOfYear2(int year, int month, int day)
{//=========================================
    // allow month 13
    short *monthstart;

    NYearDays(year, &monthstart);

    if((month <= 0) || (month > 13) || (day <= 0) || (day > 31))
        return(-1);

    return(monthstart[month-1] + (day-1));
}


int DayOfYear(const wxString date_str, int *out_yearix)
{//====================================================
    int yearix, doy;
    long int year, month, day;
    wxString year_str, month_str, day_str;

    if(date_str == wxEmptyString)
        return(-1);

    year_str = date_str.Left(4);
    month_str = date_str.Mid(4, 2);
    day_str = date_str.Right(2);

    if(year_str.ToLong(&year) && month_str.ToLong(&month) && day_str.ToLong(&day))
    {
        yearix = year - YEAR_FIRST;
        if((yearix < 0) || (yearix >= N_YEARS) || ((doy = DayOfYear2(year, month, day)) < 0) || (doy > 365))
        {
            return(-1);
        }

        if(out_yearix != NULL)
            *out_yearix = yearix;
        return(doy);
    }
    return(-1);
}

wxString DoyToDate(int doy, int yearix)
{//====================================
    int ix;
    int year;
    static wxString date;

    short *monthstart;

    year = yearix + YEAR_FIRST;
    if((year % 4) == 0)
        monthstart = monthstart2;  //NOTE, next non-leap (year%4) is 2100
    else
        monthstart = monthstart1;

    for(ix=1; ix<13; ix++)
    {
        if(doy < monthstart[ix])
        {
            date.Printf(_T("%.4d%.2d%.2d"), year, ix, doy-monthstart[ix-1]+1);
            return(date);
        }
    }
    return(wxEmptyString);
}


void FindDates()
{//=============
    unsigned int year_count;
    int doy;  // 0 to 364 (or 365 in leap years)
    int yearix;
    long inverter;  // inverter slot, 0 or 1
    long value;
    int n;
    int energy1;
    wxString fname;
    wxString leaf;
    wxString extension;
    wxString s;
    FILE *f;
    int prev_doy = 999;
    int prev_yearix = -1;
    float prev_energy_total=0;
    float energy[6];
    char date_str[20];
    char buf[256];

    for(year_count=0; year_count<years.Count(); year_count++)
    {
        fname = wxFindFirstFile(data_dir+_T("/")+years[year_count]+_T("/d*_2*"), wxFILE);
        while(!fname.empty())
        {
            leaf = wxFileName(fname).GetName();   // without the extension

            doy = DayOfYear(leaf.Right(8), &yearix);

            if(doy >= 0)
            {
                if(yeartab_flags[yearix] == NULL)
                    break;

                yeartab_flags[yearix][doy] = 1;   // bit 0: we have some power data for this day
            }
            fname = wxFindNextFile();
        }

        // file names are of the for:  inverterN_A_yyyy
        // Where N is 0 or 1,  A is the inverter address, yyyy is the year
        fname = wxFindFirstFile(data_dir+_T("/")+years[year_count]+_T("/inverter*_2*"), wxFILE);
        while(!fname.empty())
        {
            leaf = wxFileName(fname).GetName();
            extension = wxFileName(fname).GetExt();

            s = leaf.Right(4);
            value = 0;
            s.ToLong(&value);
            if((value >= 2000) && (value < 3000) && (extension == _T("txt")))
            {
                // filename must end with a year
                s = leaf.Mid(8, 1);
                s.ToLong(&inverter);

                if((f = fopen(fname.mb_str(wxConvLocal), "r")) != NULL)
                {
                    while(fgets(buf, sizeof(buf), f) != NULL)
                    {
                        if(buf[0] == '#') continue;

                        n = sscanf(buf, "%s %f %f %f %f %f %f", date_str, &energy[0], &energy[1], &energy[2], &energy[3], &energy[4], &energy[5]);
                        if(n < 2)
                           continue;

                        doy = DayOfYear(wxString(date_str, wxConvLocal), &yearix);

                        if((doy >= 0) && (yeartab_energy[inverter][yearix] != NULL))
                        {
#ifdef deleted
                            if((n >= 6) && (yearix == prev_yearix) && ((doy - prev_doy) > 1))
                            {
                                // There is a gap in the records of daily energy output.
                                // Fill with the average.
                                // Note: not implemented across different years.
                                int j;
                                int ndays;
                                ndays = doy - prev_doy;
                                energy1 = (int)((energy[4]-prev_energy_total)/ndays * 1000);
                                if(energy1 < 0)
                                    energy1 = 0;
                                for(j=1; j <= ndays; j++)
                                {
                                    yeartab_energy[inverter][yearix][prev_doy+j] = energy1;
                                    yeartab_flags[yearix][prev_doy+j] |= (1 << (inverter+4));   // bits 4,5 this is an average value
                               }
                            }
                            else
#endif
                            {
                                energy1 = (int)(energy[0] * 1000);
                                if(energy1 < 0)
                                    energy1 = 0;
                                yeartab_energy[inverter][yearix][doy] = energy1;
                            }

                        }
                        prev_yearix = yearix;
                        prev_doy = doy;
                        prev_energy_total = energy[4];
                    }

                    fclose(f);
                }
            }
            fname = wxFindNextFile();
        }
    }
}


void FindYears()
{//=============
    int yearix;
    long int year;
    wxString dir;
    wxString leaf;

    years.Empty();

    dir = wxFindFirstFile(data_dir+_T("/2*"), wxDIR);
    while(!dir.empty())
    {
        leaf = wxFileName(dir).GetFullName();
        if(leaf.Len() == 4)
        {
            if(leaf.ToLong(&year) == true)
            {
                if((year >= YEAR_FIRST) && (year < (YEAR_FIRST + N_YEARS)))
                {
                    years.Add(leaf);
                    yearix = year - YEAR_FIRST;

                    if(yearix < yearix_first)
                        yearix_first = yearix;

                    yeartab_flags[yearix] = (unsigned char *)realloc(yeartab_flags[yearix], 366 * sizeof(unsigned char));
                    yeartab_energy[0][yearix] = (unsigned short *)realloc(yeartab_energy[0][yearix], 366 * sizeof(unsigned short));
                    yeartab_energy[1][yearix] = (unsigned short *)realloc(yeartab_energy[1][yearix], 366 * sizeof(unsigned short));

                    memset(yeartab_flags[yearix], 0, 366 * sizeof(unsigned char));
                    memset(yeartab_energy[0][yearix], 0, 366 * sizeof(unsigned short));
                    memset(yeartab_energy[1][yearix], 0, 366 * sizeof(unsigned short));
                }
            }
        }
        dir = wxFindNextFile();
    }
    years.Sort();
}



void CollectDates(int force)
{//=========================
    if((collected_dates==1) && (force==0))
        return;  // already done

    FindYears();
    FindDates();
    collected_dates = 0;

}


wxString NextDate(const wxString date_str, int skip)
{//=================================================
    int doy;
    int yearix;

    CollectDates(0);

    if((doy = DayOfYear(date_str, &yearix)) < 0)
        return(wxEmptyString);  // not a valid date_str

    doy++;

    if(skip == 0)
    {
        if(doy >= NYearDays(yearix + YEAR_FIRST, NULL))
        {
            doy = 0;
            yearix++;
            if(yeartab_flags[yearix] == NULL)
                return(wxEmptyString);

        }
        return(DoyToDate(doy, yearix));
    }

    while(yearix <= (year_today - YEAR_FIRST))
    {
        if(yeartab_flags[yearix] != NULL)
        {
            while(doy < 366)
            {
                if(yeartab_flags[yearix][doy] & 1)
                {
                    return(DoyToDate(doy, yearix));
                }
                doy++;
            }
        }
        yearix++;
        doy = 0;
    }
    return(wxEmptyString);
}


wxString YesterdayDate(const wxString date_str)
{//============================================
    int doy;
    int yearix;

    if((doy = DayOfYear(date_str, &yearix)) < 0)
        return(wxEmptyString);  // not a valid date_str

    doy--;
    if(doy < 0)
    {
        yearix--;
        doy = NYearDays(yearix + YEAR_FIRST, NULL) - 1;
    }

    return(DoyToDate(doy, yearix));
}


wxString PrevDate(const wxString date_str, int skip)
{//=================================================
    int doy;
    int yearix;

    CollectDates(0);


    if((doy = DayOfYear(date_str, &yearix)) < 0)
        return(wxEmptyString);  // not a valid date_str

    doy--;

    if(skip == 0)
    {
        if(doy < 0)
        {
            yearix--;
            if(yeartab_flags[yearix] == NULL)
                return(wxEmptyString);

            doy = NYearDays(yearix + YEAR_FIRST, NULL) - 1;
        }

        return(DoyToDate(doy, yearix));
    }

    while(yearix >= 0)
    {
        if(yeartab_flags[yearix] != NULL)
        {
            while(doy >= 0)
            {
                if(yeartab_flags[yearix][doy] & 1)
                {
                    return(DoyToDate(doy, yearix));
                }
                doy--;
            }
        }
        yearix--;
        doy = 365;   // Note, 365 only has data in a leap-year
    }
    return(wxEmptyString);
}



int DateEnergy(const wxString date_str)
{//====================================
    int doy;
    int yearix;
    int energy = 0;

    CollectDates(0);

    doy = DayOfYear(date_str, &yearix);
    if(doy < 0)
        return(0);

    if(yeartab_energy[0][yearix] != NULL)
        energy += yeartab_energy[0][yearix][doy];
    if(yeartab_energy[1][yearix] != NULL)
        energy += yeartab_energy[1][yearix][doy];

    return(energy);
}


int DateEnergy2(int yearix, int doy)
{//==================================
// doy may be < 0 or > 365
    int n = NYearDays(yearix + YEAR_FIRST, NULL);
    int energy = 0;

    if(doy >= n)
    {
        yearix++;
        doy -= n;
    }
    else
    if(doy < 0)
    {
        yearix--;
        doy = NYearDays(yearix + YEAR_FIRST, NULL) + doy;
    }

    if(yearix < 0)
        return(0);

    if(yeartab_energy[0][yearix] != NULL)
        energy += yeartab_energy[0][yearix][doy];
    if(yeartab_energy[1][yearix] != NULL)
        energy += yeartab_energy[1][yearix][doy];

    return(energy);
}




void UpdateDailyData(int inv, wxString fname_daily)
{//================================================
// Update daily energy data after retrieving it from an inverter
    FILE *f_in;
    FILE *f_out = NULL;
    FILE *f_inv = NULL;
    int year, month, day;
    int doy;
    int yearix;
    int current_year = -1;
    int n_changes = 0;
    int n_changes_year = 0;
    float energy;
    wxString fname;
    wxString fname_inv;
    wxString fname_upd;
    char buf[80];
    char buf_inv[80];
    char date[12];

    if((f_in = fopen(fname_daily.mb_str(wxConvLocal), "r")) == NULL)
        return;

    CollectDates(0);

    while(fgets(buf, sizeof(buf), f_in) != NULL)
    {
        if(sscanf(buf, "%d-%d-%d %f", &year, &month, &day, &energy) != 4)
            continue;

        if((doy = DayOfYear2(year, month, day)) < 0)
            continue;

        yearix = year - YEAR_FIRST;

        fname.Printf(_T("%s/%4d"), data_dir.c_str(), year);
        if(wxDirExists(fname) == false)
        {
            wxMkdir(fname);
            CollectDates(1);
        }

        if(yeartab_energy[inv][yearix] == NULL)
            continue;

        if(energy > 0)
        {
            if(yeartab_energy[inv][yearix][doy] == 0)
            {
                if((f_out == NULL) || (year != current_year))
                {
                    if(f_out !=  NULL)
                    {
                        if(f_inv != NULL)
                        {
                            do{
                                // copy the remainder of the inverter file
                                fprintf(f_out, "%s", buf_inv);
                            } while(fgets(buf_inv, sizeof(buf_inv), f_inv) != NULL);
                            fclose(f_inv);
                        }
                        fclose(f_out);
                        if(n_changes_year > 0)
                            wxRenameFile(fname_upd, fname_inv);
                    }

                    fname_inv.Printf(_T("%s/%d/inverter%d_%d_%d.txt"), data_dir.c_str(), year, inv, inverter_address[inv], year);
                    f_inv = fopen(fname_inv.mb_str(wxConvLocal), "r");
                    fname_upd.Printf(_T("%s/%d/update%d_%d_%d.txt"), data_dir.c_str(), year, inv, inverter_address[inv], year);
                    f_out = fopen(fname_upd.mb_str(wxConvLocal), "w");
                    current_year = year;
                    n_changes_year = 0;
                    buf_inv[0] = 0;
                    if(f_inv != NULL)
                        fgets(buf_inv, sizeof(buf_inv), f_inv);
                }

                if(f_out != NULL)
                {
                    if(f_inv != NULL)
                    {
                        sprintf(date, "%.4d%.2d%.2d", year, month, day);
                        while((buf_inv[0] != '2') || (strcmp(buf_inv, date) < 0))
                        {
                            fprintf(f_out, "%s", buf_inv);
                            buf_inv[0] = 0;
                            if(fgets(buf_inv, sizeof(buf_inv), f_inv) == NULL)
                               break;
                        }
                    }

                    fprintf(f_out, "%d%.2d%.2d %7.3f\n", year, month, day, energy);
                    n_changes++;
                    n_changes_year++;
                }
            }
        }
    }
    fclose(f_in);

    if(f_out != NULL)
    {
        if(f_inv != NULL)
        {
            do{
                // copy the remainder of the inverter file
                fprintf(f_out, "%s", buf_inv);
            } while(fgets(buf_inv, sizeof(buf_inv), f_inv) != NULL);
            fclose(f_inv);
        }
        fclose(f_out);
        if(n_changes_year > 0)
            wxRenameFile(fname_upd, fname_inv);
    }

    if(n_changes > 0)
        CollectDates(1);
}




int DisplayDateGraphs(const wxString date_str, int control)
{//========================================================
// Display the power data for a specified date
// control: bit 0: keep previous start_ix if it's less than the new value
    int doy;
    int yearix;
    int ix = 0;
    int j;
    long int addr;
    wxString dir;
    wxString fname;
    wxString fname2;
    wxString s;
    wxString not_placed[2] = {wxEmptyString, wxEmptyString};
    int done[2] = {0, 0};

    graph_panel->SetMode(0);

    doy = DayOfYear(date_str, &yearix);
    if(doy < 0)
        return(-1);

    graph_date_ymd = date_str;
    graph_date = MakeDateString(graph_date_ymd);

    if(date_str == date_ymd_string)
    {
        // This is today's date
        OpenLogFiles(-1, 1);
    }
    else
        display_today = 0;

    graph_panel->NewDay();

    dir = data_dir + _T("/") + date_str.Left(4);

    fname = wxFindFirstFile(dir+_T("/d*_*"), wxFILE);
    while(!fname.empty())
    {
        fname2 = fname;
        if((fname2.Right(4) == _T(".txt")) || (fname2.Right(4) == _T(".dat")))
            fname2 = wxFileName(fname).GetName();

        if(fname2.Right(8) == date_str)
        {
            s = fname2.AfterLast('d');
            s = s.BeforeFirst('_');
            if(s.ToLong(&addr) == false)
                continue;

            // current inverter addresses may differ from earlier dates
            // Fill matching addresses first
            if(addr == inverter_address[0])
            {
                graph_panel->ReadEnergyFile(0, fname, control, NULL);
                done[0] = 1;
            }
            else
            if(addr == inverter_address[1])
            {
                graph_panel->ReadEnergyFile(1, fname, control, NULL);
                done[1] = 1;
            }
            else
            {
                if(ix < 2)
                    not_placed[ix++] = fname;
            }
        }
        fname = wxFindNextFile();
    }

    // Find free slots for any mis-matched inverter addresses
    for(j=0; j<ix; j++)
    {
        if(done[0] == 0)
            graph_panel->ReadEnergyFile(0, not_placed[j], control, NULL);
        else
        if(done[1] == 0)
            graph_panel->ReadEnergyFile(1, not_placed[j], control, NULL);
    }

    CalcSun(graph_date_ymd);

    return(0);
}  // end of DisplayDateGraphs



//wxColour colour_energy1(0x64c8ff);
//wxColour colour_energy2(0x84c8ff);
wxColour month_colours[13] = {
    wxColour(0xccbf7a), wxColour(0xc0e673), wxColour(0x85F279),
    wxColour(0x77E0BA), wxColour(0x6ae6e6), wxColour(0x6bb5ff),
    wxColour(0x8080ff), wxColour(0xa373e6), wxColour(0x7566cc),
    wxColour(0x5a6bb3), wxColour(0x6382a6), wxColour(0xbfa39b), wxColour(0xbfa39b)};

wxColour colour_average(0x404040);
wxString month_names[13] = {_("Jan"),_("Feb"),_("Mar"),_("Apr"),_("May"),_("Jun"),_("Jul"),_("Aug"),_("Sep"),_("Oct"),_("Nov"),_("Dec"),_("")};

int hist_scale[N_HIST_SCALE] = {1, 2, 3, 4, 6, 9, 13, 18, 24, 30, 38, 50};


wxDateTime histogram_start_date;


int GraphPanel::HistogramExtent()
{//==============================
    int day_ix;
    int energy;
    int extent;
    int yearix;
    int n_yeardays;
    int doy;
    short *monthstart;

    if(energy_n_days == 0)
    {
        // first time called, examine the data
        CollectDates(0);
        day_ix = -1;
        for(yearix = yearix_first; yearix <= (year_today - YEAR_FIRST); yearix++)
        {
            n_yeardays = NYearDays(yearix+YEAR_FIRST, &monthstart);
            for(doy=0; doy < n_yeardays; doy++)
            {
                energy = 0;
                if(yeartab_energy[0][yearix] != NULL)
                    energy += yeartab_energy[0][yearix][doy];
                if(yeartab_energy[1][yearix] != NULL)
                    energy += yeartab_energy[1][yearix][doy];
                if((day_ix < 0) && (energy > 0))
                    day_ix = 0;   // the first day with an energy reading
                else
                if(day_ix >= 0)
                    day_ix++;

                if(energy > 0)
                    energy_n_days = day_ix;
            }
        }
    }

    xscale_hist = hist_scale[hist_xscale_ix];
    if(histogram_page == HIST_WEEKS)
    {
        extent = (energy_n_days+4) * xscale_hist/2 + 70;
    }
    else
    if(histogram_page == HIST_MONTHS)
    {
        xscale_hist /= 4;
        extent = (energy_n_days+16) * xscale_hist/4 + 70;
    }
    else
    {
        extent = (energy_n_days+2) * xscale_hist + 70;
    }
    return(extent);
}

static wxString histogram_titles[] = {_T("Daily"), _T("Smoothed"), _T("Weekly"), _T("Monthly")};

void GraphPanel::DrawEnergyHistogram(wxDC &dc)
{//============================================
    int ix;
    int width, height;
    int yearix;
    int doy;
    int dow;
    int dom=0;
    int n_yeardays;
    int energy;
    int day_ix;
    int initial_year;
    int yearix_last;
    int x, y;
    int month;
    short *monthstart;
    double yscale;
    int bar_width;
    int y_max;
    int y_grad;
    int y_grad2;
    int today;
    int av_energy;
    int total_energy = 0;
    int total_days = 0;
    wxColour colour;
    wxColour colour_week;
    wxColour colour_month;
    struct tm btime;
    int histogram_mode;
    int prev_x = 0;
    int today_doy;
    int today_yearix;
    int latest_doy;
    int latest_yearix;

    histogram_mode = histogram_page;

    xscale_hist = hist_scale[hist_xscale_ix];
    if(histogram_mode == HIST_WEEKS)
        xscale_hist /= 2;   // weeks
    if(histogram_mode == HIST_MONTHS)
        xscale_hist /= 4;   // months

    bar_width = xscale_hist - 1;
    if(bar_width < 1) bar_width = 1;

    CollectDates(0);
    GetClientSize(&width,&height);
    today_doy = DayOfYear(date_ymd_string, &today_yearix);
    latest_doy = DayOfYear(wxString(ymd_energy, wxConvLocal), &latest_yearix);


    dc.SetTextForeground(colour_text1);
    dc.SetTextBackground(colour_backg);

	dc.SetFont(Font_Year);
	dc.SetTextBackground(colour_backg);
	dc.SetTextForeground(colour_text1);
    dc.DrawText(histogram_titles[histogram_mode], x_scroll, 32);

    y_max = (int)(histogram_max * 1000 * range_adjust[hist_range_ix]);
    yscale = (double)(height)/y_max;

    y_grad = 1000;
    y_grad2 = 5000;
    if(y_max > 10000)
    {
        y_grad = 2000;
        y_grad2 = 10000;
    }
    if(y_max > 30000)
    {
        y_grad = 5000;
        y_grad2 = 10000;
    }
    // draw vertical scale
    for(ix=0; ix < y_max; ix+=y_grad)
    {
        y = height - (int)(ix * yscale);
        if((ix % y_grad2) == 0)
            dc.SetPen(colour_grid1);
        else
            dc.SetPen(colour_grid2);
        dc.DrawLine(0+x_scroll, y, width+x_scroll, y);
        dc.DrawText(wxString::Format(_T("%2dkW"), ix / 1000), x_scroll+width-30, y);
    }

    memset(&btime, 0, sizeof(btime));
    btime.tm_year = yearix_first + YEAR_FIRST - 1900;
    btime.tm_mday = 1;
    mktime(&btime);
    dow = btime.tm_wday-1;

    colour_month = colour_week = month_colours[0];
    day_ix = -1;
    initial_year = 1;
    yearix_last = year_today - YEAR_FIRST;
    if(today_doy > 334)
        yearix_last++;  // we're in December, so continue the loop until next year to ensure the December total is shown

    for(yearix = yearix_first; yearix <= yearix_last; yearix++)
    {
        month = 0;

        n_yeardays = NYearDays(yearix+YEAR_FIRST, &monthstart);
        for(doy=0; doy < n_yeardays; doy++)
        {
            dow++;
            if(dow == 7)
                dow = 0;

            energy = DateEnergy2(yearix, doy);

            if((energy > 0) && (histogram_mode == HIST_DAYS_AVG))
            {
                double e;
                int count;

              //  energy = 0;
                count = 1;
                for(ix=-2; ix<=2; ix++)
                {
                    if((e = DateEnergy2(yearix, doy+ix)) > 0)
                    {
                        energy += e;
                        count++;
                    }
                }
                if(count > 1)
                    energy = energy / count;
            }

            if((day_ix < 0) && (energy > 0))
            {
                wxString first_date = DoyToDate(doy, yearix);
                long first_month;
                long first_dom;
                first_date.Mid(4,2).ToLong(&first_month);
                first_date.Right(2).ToLong(&first_dom);
                day_ix = 0;   // the first day with an energy reading
                histogram_start_date.Set(first_dom, wxDateTime::Month(first_month-1), yearix+YEAR_FIRST);
            }
            else
            if(day_ix >= 0)
                day_ix++;

            today = 0;
            if((doy==latest_doy) && (yearix==latest_yearix) && (energy == 0))
            {
                time_t current_time;
                struct tm *btime;
                int period;

                energy = (int)((inverters[0].energy_total[0] + inverters[1].energy_total[0]) * 1000);

                time(&current_time);
                btime = localtime(&current_time);
                period = btime->tm_hour*360 + btime->tm_min*6 + btime->tm_sec/10;
                if((period > sun.sunset) || (today_yearix > latest_yearix) || (today_doy > latest_doy))
                {
                    today = 0;
                }
                else
                {
                    today = 1;
                }

                if(day_ix < 0)
                    day_ix = 0;
            }

            x = day_ix*xscale_hist + 30;

            if(doy >= monthstart[month])
            {
                // start of a month
                dom = 0;
                if(day_ix >= 0)
                {
                    // draw horizontal scale
                    if((month==0) || (initial_year && (month < 10) && (xscale_hist > 2)))
                    {
                        initial_year = 0;
                        dc.SetFont(Font_Year);
                        dc.SetTextBackground(colour_backg);
                        dc.SetTextForeground(colour_text1);
                        dc.DrawText(wxString::Format(_T("%4d"), yearix+YEAR_FIRST), x+4, 17);
                        dc.SetPen(colour_grid1);
                        dc.DrawLine(x, 0, x, height);
                    }
                    else
                    if(xscale_hist > 0.49)
                    {
                        dc.SetPen(colour_grid2);
                        dc.DrawLine(x, 0, x, height);
                    }

                    dc.SetFont(Font_Default);
                    if(xscale_hist < 1.0)
                        dc.DrawText(month_names[month][0], x+4, 2);
                    else
                    if(xscale_hist > 0.49)
                        dc.DrawText(month_names[month], x+4, 2);
                }

                colour_month = colour;
                colour = month_colours[month];
                if(Latitude < 0)
                {
                    colour = month_colours[(month + 6) % 12];
                }
                month++;
            }
            dom++;

            dc.SetPen(*wxTRANSPARENT_PEN);

            if((histogram_mode == HIST_WEEKS) && (dow == 0))
            {
                if(total_days > 0)
                {
                    dc.SetBrush(colour_week);
                    av_energy = total_energy / total_days;
                    y = (int)(av_energy * yscale);
                    if(xscale_hist < 1.0)
                        dc.DrawRectangle(prev_x, height-y, x - prev_x, y);
                    else
                        dc.DrawRectangle(prev_x, height-y, x - prev_x - 1, y);
                }
                colour_week = colour;
                prev_x = x;
                total_days = 0;
                total_energy = 0;
            }
            if((histogram_mode == HIST_MONTHS) && (dom == 1))
            {
                if(total_days > 0)
                {
                    dc.SetBrush(colour_month);
                    av_energy = total_energy / total_days;
                    y = (int)(av_energy * yscale);
                    dc.DrawRectangle(prev_x, height-y, x - prev_x - 1, y);
                }
                prev_x = x;
                total_days = 0;
                total_energy = 0;
            }

            if(energy > 0)
            {
                if(today == 0)
                {
                    total_energy += energy;
                    total_days++;
                }

                if((histogram_mode == HIST_DAYS) || (histogram_mode == HIST_DAYS_AVG))
                {
                    if((yeartab_flags[yearix][doy] & 0x30) && (histogram_mode != HIST_DAYS_AVG))
                        dc.SetBrush(colour_average);   // this value is an average
                    else
                        dc.SetBrush(colour);
                    y = (int)(energy * yscale);
                    if(today)
                    {
                        if(histogram_flags & 0xf)
                            dc.SetBrush(*wxBLACK_BRUSH);
                        else
                            dc.SetBrush(*wxWHITE_BRUSH);
                    }
                    dc.DrawRectangle(x, height-y, bar_width, y);
                    if(bar_width > 16)
                    {
                        dc.SetFont(Font_Default);
                        dc.SetTextForeground(*wxBLACK);
                        dc.SetTextBackground(colour);
                        dc.DrawText(wxString::Format(_T("%2d"), dom),x, height-16);
                    }
                }
            }

        }
    }
}


wxString GraphPanel::DateAtCursor(int x)
{//=====================================
    int day_ix;
    int jmd_start;
    wxDateTime date;

    jmd_start = histogram_start_date.GetModifiedJulianDayNumber();
    day_ix = (x-30) / xscale_hist;
    date = wxDateTime((double)(jmd_start + day_ix) + 2400000.5);
    return date.Format(_T("%Y%m%d"));
}



//=======================================================================================================================


/************************************************************
                Select Date Dialog
*************************************************************/

BEGIN_EVENT_TABLE(DlgSelectDate, wxDialog)
    EVT_CALENDAR_MONTH(-1, DlgSelectDate::OnMonth)
    EVT_CALENDAR_YEAR(-1, DlgSelectDate::OnMonth)
    EVT_CALENDAR(-1, DlgSelectDate::OnDate)
//	EVT_BUTTON(dlgOK, DlgSelectDate::OnButton)
//	EVT_BUTTON(dlgCancel, DlgSelectDate::OnButton)
END_EVENT_TABLE()



DlgSelectDate::DlgSelectDate(wxWindow *parent)
    : wxDialog(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(270,200+DLG_HX))
{//=================================
    int y;
    wxDateTime date;
    long year, month, day;

    graph_date_ymd.Left(4).ToLong(&year);
    graph_date_ymd.Mid(4, 2).ToLong(&month);
    graph_date_ymd.Right(2).ToLong(&day);
    date.Set(day, wxDateTime::Month(month-1), year);

    y = 12;
    t_calendar = new wxCalendarCtrl(this, -1, wxDefaultDateTime, wxPoint(10, y), wxDefaultSize, wxCAL_SEQUENTIAL_MONTH_SELECTION);

    t_calendar->SetDate(date);

    y = 160;
    button_OK = new wxButton(this, wxID_OK, _T("OK"), wxPoint(80,y));
    button_Cancel = new wxButton(this, wxID_CANCEL, _T("Cancel"), wxPoint(170,y));

    CollectDates(0);
    ShowAvailableDates();
}


wxString SelectDisplayDate()
{//=========================
    wxString string;

    DlgSelectDate *dlg = new DlgSelectDate(mainframe);
    if ( dlg->ShowModal() == wxID_OK )
    {
        string = dlg->t_calendar->GetDate().Format(_T("%Y%m%d"));
    }
    else
    {
        //else: dialog was cancelled or some another button pressed
        string = wxEmptyString;
    }

    dlg->Destroy();
    return(string);
}


void DlgSelectDate::ShowAvailableDates()
{//=====================================
    int ix;
    int year, month;
    int yearix;
    int doy_start;
    int n_days;
    wxCalendarDateAttr *attr;

    wxDateTime date;
    date = t_calendar->GetDate();

    year = date.GetYear();
    month = date.GetMonth();

    yearix = year - YEAR_FIRST;
    doy_start = DayOfYear2(year, month+1, 1);
    n_days = DayOfYear2(year, month+2, 1) - doy_start;

    for(ix=0; ix < n_days; ix++)
    {
        t_calendar->ResetAttr(ix+1);

        if((yearix < 0) || (yearix >= N_YEARS) || (yeartab_flags[yearix] == NULL) || ((yeartab_flags[yearix][doy_start + ix] & 1) == 0))
        {
            attr = new wxCalendarDateAttr(*wxLIGHT_GREY);
            t_calendar->SetAttr(ix+1, attr);
        }
    }
}



void DlgSelectDate::OnMonth(wxCalendarEvent& WXUNUSED(event))
{//================================================
    ShowAvailableDates();
}

void DlgSelectDate::OnDate(wxCalendarEvent& WXUNUSED(event))
{//===============================================
    wxString string;
    string = t_calendar->GetDate().Format(_T("%Y%m%d"));
    DisplayDateGraphs(string, 0);
}
