
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

double TimeZone2 = 0;
double Timezone = 0;
double Latitude = 0;
double Longitude = 0;
double Timezone3;    // timezone for the specified date (including daylighyt saving time)


SUN sun;

unsigned short sun_azimuth[N_10SEC];
short sun_elevation[N_10SEC];
short sun_intensity[N_PANEL_GROUPS][N_10SEC];

PANEL_GROUP panel_groups[N_PANEL_GROUPS];

#define RADIANS(x)  (x)*M_PI/180
#define DEGREES(x)  (x)*180/M_PI

double CalcSun2(int date, int time_ix, int control)
{//================================================
    int ix;
    double Time;
    double jDay;
    double jCentury;
    double GeomMeanLongSun;  // (deg)
    double GeomMeanAnomSun;   // (rad)
    double EccentEarthOrbit;
    double SunEqOfCtr;
    double SunTrueLong;  // (deg)
    //double SunTrueAnom;  // (rad)
    //double SunRadVector; // (AUs)
    double SunAppLong;  // (deg)
    double MeanObliqEcliptic;  // (deg)
    double ObliqCorr;  // (deg)
//    double SunRtAscen;  // (deg)
    double SunDeclinRad;  //  (rad)
    double y;
    double EqOfTime;  // (mins)
    double HASunrise;  // (mins)
    double SolarNoon;  // (LST)
    double SunriseTime; // (LST)
    double SunsetTime;  // (LST)
    double SunlightDuration;  // (mins)
    double TrueSolarTime; // (mins)
    double HourAngle;   // (deg)
    double SolarZenith;  // (rad)
    double SolarElevation;  // (deg)
    double AtmosRefract;  // (deg)
    double SolarElevCorr;  // (deg)
    double SolarAzimuth;  // deg cw from N

    double LatitudeRad = RADIANS(Latitude);

    Time = (double)(time_ix) / N_10SEC;

    jDay = date + 2415018.5 + (Time - Timezone);

    jCentury = (jDay -2451545)/36525;
    GeomMeanLongSun = fmod(280.46646 + jCentury*(36000.76983 + jCentury*0.0003032), 360.0);
    GeomMeanAnomSun = RADIANS(357.52911 + jCentury*(35999.05029-0.0001537*jCentury));
    EccentEarthOrbit = 0.016708634 - jCentury*(0.000042037+0.0001537*jCentury);
    SunEqOfCtr = sin(GeomMeanAnomSun)*(1.914602-jCentury*(0.004817+0.000014*jCentury))+sin(2*GeomMeanAnomSun)*(0.019993-0.000101*jCentury)+sin(3*GeomMeanAnomSun)*0.000289;
    SunTrueLong = GeomMeanLongSun + SunEqOfCtr;
//    SunTrueAnom = GeomMeanAnomSun + SunEqOfCtr;
//    SunRadVector = (1.000001018*(1-EccentEarthOrbit*EccentEarthOrbit))/(1+EccentEarthOrbit*cos(SunTrueAnom));
    SunAppLong = SunTrueLong-0.00569-0.00478*sin(RADIANS(125.04-1934.136*jCentury));
    MeanObliqEcliptic = 23+(26+((21.448-jCentury*(46.815+jCentury*(0.00059-jCentury*0.001813))))/60)/60;
    ObliqCorr = MeanObliqEcliptic+0.00256*cos(RADIANS(125.04-1934.136*jCentury));
//    SunRtAscen = DEGREES(atan2(cos(RADIANS(SunAppLong)), cos(RADIANS(ObliqCorr))*sin(RADIANS(SunAppLong))));
    SunDeclinRad = asin(sin(RADIANS(ObliqCorr))*sin(RADIANS(SunAppLong)));
    y = tan(RADIANS(ObliqCorr/2))*tan(RADIANS(ObliqCorr/2));
    EqOfTime = 4*DEGREES(y*sin(2*RADIANS(GeomMeanLongSun))-2*EccentEarthOrbit*sin(GeomMeanAnomSun)+4*EccentEarthOrbit*y*sin(GeomMeanAnomSun)*cos(2*RADIANS(GeomMeanLongSun))-0.5*y*y*sin(4*RADIANS(GeomMeanLongSun))-1.25*EccentEarthOrbit*EccentEarthOrbit*sin(2*GeomMeanAnomSun));
    HASunrise = DEGREES(acos(cos(RADIANS(90.833))/(cos(LatitudeRad)*cos(SunDeclinRad))-tan(LatitudeRad)*tan(SunDeclinRad)));

    SolarNoon = (720-4* Longitude -EqOfTime+ Timezone3 *60)/1440;
    SunriseTime = SolarNoon - HASunrise * 4/1440;
    SunsetTime = SolarNoon + HASunrise * 4/1440;
    SunlightDuration = 8 * HASunrise;
    TrueSolarTime = fmod(Time*1440+EqOfTime+4*Longitude -60*Timezone3,1440);

    HourAngle = TrueSolarTime/4;
    if(HourAngle < 0)
        HourAngle += 180;
    else
        HourAngle -= 180;

    SolarZenith = acos(sin(LatitudeRad)*sin(SunDeclinRad)+cos(LatitudeRad)*cos(SunDeclinRad)*cos(RADIANS(HourAngle)));
    SolarElevation = 90 - DEGREES(SolarZenith);


    if(SolarElevation > 85)
        AtmosRefract = 0;
    else
    {
        if(SolarElevation > 5)
        {
            AtmosRefract = 58.1/tan(RADIANS(SolarElevation))-0.07/pow(tan(RADIANS(SolarElevation)),3)+0.000086/pow(tan(RADIANS(SolarElevation)),5);
        }
        else
        {
            if(SolarElevation > -0.575)
            {
                AtmosRefract = 1735+SolarElevation*(-518.2+SolarElevation*(103.4+SolarElevation*(-12.79+SolarElevation*0.711)));
            }
            else
            {
                AtmosRefract = -20.772/tan(RADIANS(SolarElevation));
            }
        }
    }
    AtmosRefract /= 3600;

    SolarElevCorr = SolarElevation + AtmosRefract;

    if(HourAngle > 0)
        SolarAzimuth = fmod(DEGREES(acos(((sin(LatitudeRad)*cos(SolarZenith))-sin(SunDeclinRad))/(cos(LatitudeRad)*sin(SolarZenith))))+180,360);
    else
        SolarAzimuth = fmod(540-DEGREES(acos(((sin(LatitudeRad)*cos(SolarZenith))-sin(SunDeclinRad))/(cos(LatitudeRad)*sin(SolarZenith)))),360);

    if(control & 1)
    {
        double cos_theta;
        double elevation;
        double diff_azimuth;
        double tilt;
        double airmass;
        double x;
        double intensity;
        double diffuse_factor = 0.15;  // this is a guess

        sun_elevation[time_ix] = (int)(SolarElevCorr * 100);
        elevation = RADIANS(SolarElevCorr);
        if(SolarElevCorr > sun.max_elev_day)
            sun.max_elev_day = SolarElevCorr;

        sun_azimuth[time_ix] = (int)(SolarAzimuth * 100);

        for(ix=0; ix<N_PANEL_GROUPS; ix++)
        {
            tilt = RADIANS(panel_groups[ix].tilt);
            diff_azimuth = panel_groups[ix].facing - double(sun_azimuth[time_ix])/100.0;

            // Angle of incidence from:
            // http://www.powerfromthesun.net/Book/chapter04/chapter04.html
            cos_theta = sin(elevation) * cos(tilt) + sin(tilt) * cos(elevation) * cos(RADIANS(diff_azimuth));

            x = 708 * cos(SolarZenith);

            // airmass and solar intensity from:
            // http://en.wikipedia.org/wiki/Air_mass_%28solar_energy%29  Spherical Shell model
            airmass = sqrt(x*x + 2*708 + 1) - x;

            x = pow(airmass, 0.678);
            intensity = 1.1 * 1353 * pow(0.7, x);
            sun_intensity[ix][time_ix] = (int)(intensity * (cos_theta + diffuse_factor) * 10);   // also apply factor for incidence angle

            // note, an additional factor would be temperature derating at about -0.45% per degree
        }
    }
    if(time_ix == N_10SEC/2)
    {
        sun.sunrise = (int)(SunriseTime * N_10SEC + 0.5);
        sun.sunset = (int)(SunsetTime * N_10SEC + 0.5);
        sun.sunnoon = (int)(SolarNoon * N_10SEC + 0.5);
        sun.noon_hrs = (int)(SolarNoon * 24);
        sun.noon_mins = (int)(SolarNoon * 24*60 + 0.5) % 60;
        sun.duration = SunlightDuration;
    }

    switch(control)
    {
    case 2:
        return(SunriseTime);
    case 3:
        return(SunsetTime);
    case 4:
        return(SolarAzimuth);
    }
    return(0);
}


int CalcSun(wxString date_ymd)
{//===========================
	struct tm btime;
	long year, month, day;
	int date;
    double timeofday;
    wxDateTime DateTime;

    date_ymd.Left(4).ToLong(&year);
    date_ymd.Mid(4,2).ToLong(&month);
    date_ymd.Right(2).ToLong(&day);
	memset(&btime, 0, sizeof(btime));
	btime.tm_year = year-1900;
	btime.tm_mon = month-1;
	btime.tm_mday = day;
	mktime(&btime);

    date = (btime.tm_year * 1461)/4 + btime.tm_yday + 2;

    Timezone3 = TimeZone2;
    if(TimeZone2 > 24)
    {
        //'auto' timezone
        DateTime.Set((double)date + 2415019 - Timezone);
        Timezone3 = GetGmtOffset(DateTime.GetTicks()) /3600;
    }


    sun.max_elev_day = -90;
    memset(sun_elevation, 0, sizeof(sun_elevation));
    for(timeofday = 0; timeofday < N_10SEC; timeofday ++)
    {
        CalcSun2(date, timeofday, 1);
    }

    sun.max_elev_year = 90 + 23.5 - abs(Latitude);
    if(sun.max_elev_year > 90)
        sun.max_elev_year = 90;

    sun.sunrise_time = CalcSun2(date, sun.sunrise, 2);
    sun.sunrise_azim = CalcSun2(date, (int)(sun.sunrise_time * N_10SEC + 0.5), 4);
    sun.sunset_time = CalcSun2(date, sun.sunset, 3);
    sun.sunset_azim = CalcSun2(date, (int)(sun.sunset_time * N_10SEC + 0.5), 4);
    sun.noon_azim = CalcSun2(date, sun.sunnoon, 4);

    if(sun.sunset >= N_10SEC)
    {
        wxLogError(_T("Longitude is set wrongly for this Timezone"));
        sun.sunset = N_10SEC - 1;
        return(-1);
    }
    return(0);
}


extern wxString degrees_to_string(double degrees);

void SunInfo()
{//===========
    wxString info;
    int sunr;
    int suns;

    sunr = (int)(sun.sunrise_time * 24*3600 + 0.5);
    suns = (int)(sun.sunset_time * 24*3600 + 0.5);
    info = wxString::Format(_T("Sunrise\t\t%.2d:%.2d  at %3d degrees\nSunset\t\t%.2d:%.2d  at %3d degrees\n\n"
                               "Solar Noon\t%.2d:%.2d  at %3d degrees\nMax Elevation\t%.2f degrees\n"),
            sunr/3600, (sunr/60) % 60, (int)(sun.sunrise_azim + 0.5),
            suns/3600, (suns/60) % 60, (int)(sun.sunset_azim + 0.5),
            sun.noon_hrs, sun.noon_mins, (int)(sun.noon_azim + 0.5),
            sun.max_elev_day) +
            _T("\nLongitude \t") + degrees_to_string(Longitude) + _T("\nLatitude    \t") + degrees_to_string(Latitude);
    wxMessageBox(info, _T("Sun  ") + graph_date, wxOK, mainframe);
}



void MonitorOutput(int power)
{//==========================
    static int power_ok = 700;
    static int power_low = 600;
    static int power_high = 800;
    static int state = 0;
    static int timer_low = 0;
    static int timer_high = 0;
    static int timer_high2 = 0;

    if(power > power_high)
    {
        if((state == 0) && (timer_high2 == 0))
            timer_high2 = 5;
        timer_low = 0;
    }
    else
    if(power >= power_ok)
    {
        if((state == 0) && (timer_high == 0))
            timer_high = 10;
        timer_high2 = 0;
        timer_low = 0;
    }
    else
    if((state == 1) && (power < power_low))
    {
        timer_high = 0;
        timer_high2 = 0;
        if(state == 1)
        {
            // action low
            state = 0;
        }
    }
    else
    {
        if((state == 1) && (timer_low == 0))
            timer_low = 5;
        timer_high = 0;
        timer_high2 = 0;
    }

}

