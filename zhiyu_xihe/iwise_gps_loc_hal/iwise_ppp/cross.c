#include "rtklib.h"
#define ERR_BRDCI   0.5         /* broadcast iono model error factor */
const static double leaps[][7] = { /* leap seconds {y,m,d,h,m,s,utc-gpst,...} */
	{ 2012, 7, 1, 0, 0, 0, -16 },
	{ 2009, 1, 1, 0, 0, 0, -15 },
	{ 2006, 1, 1, 0, 0, 0, -14 },
	{ 1999, 1, 1, 0, 0, 0, -13 },
	{ 1997, 7, 1, 0, 0, 0, -12 },
	{ 1996, 1, 1, 0, 0, 0, -11 },
	{ 1994, 7, 1, 0, 0, 0, -10 },
	{ 1993, 7, 1, 0, 0, 0, -9 },
	{ 1992, 7, 1, 0, 0, 0, -8 },
	{ 1991, 1, 1, 0, 0, 0, -7 },
	{ 1990, 1, 1, 0, 0, 0, -6 },
	{ 1988, 1, 1, 0, 0, 0, -5 },
	{ 1985, 7, 1, 0, 0, 0, -4 },
	{ 1983, 7, 1, 0, 0, 0, -3 },
	{ 1982, 7, 1, 0, 0, 0, -2 },
	{ 1981, 7, 1, 0, 0, 0, -1 }
};
const prcopt_t prcopt_default = { /* defaults processing options */
	PMODE_SINGLE, 0, 2, SYS_GPS,   /* mode,soltype,nf,navsys */
	15.0*D2R, { { 0, 0 } },           /* elmin,snrmask */
	0, 1, 1, 5, 0, 10,               /* sateph,modear,glomodear,maxout,minlock,minfix */
	0, 0, 0, 0,                    /* estion,esttrop,dynamics,tidecorr */
	1, 0, 0, 0, 0,                  /* niter,codesmooth,intpref,sbascorr,sbassatsel */
	0, 0,                        /* rovpos,refpos */
	{ 100.0, 100.0 },              /* eratio[] */
	{ 100.0, 0.003, 0.003, 0.0, 1.0 }, /* err[] */
	{ 30.0, 0.03, 0.3 },            /* std[] */
	{ 1E-4, 1E-3, 1E-4, 1E-1, 1E-2 }, /* prn[] */
	5E-12,                      /* sclkstab */
	{ 3.0, 0.9999, 0.20 },          /* thresar */
	0.0, 0.0, 0.05,               /* elmaskar,almaskhold,thresslip */
	30.0, 30.0, 30.0,             /* maxtdif,maxinno,maxgdop */
	{ 0 }, { 0 }, { 0 },                /* baseline,ru,rb */
	{ "", "" },                    /* anttype */
	{ { 0 } }, { { 0 } }, { 0 }             /* antdel,pcv,exsats */
	, 1                          /*sys of compatation*/
};
const solopt_t solopt_default = { /* defaults solution output options */
	SOLF_LLH, TIMES_GPST, 1, 3,    /* posf,times,timef,timeu */
	0, 1, 0, 0, 0, 0,                /* degf,outhead,outopt,datum,height,geoid */
	0, 0, 0,                      /* solstatic,sstat,trace */
	{ 0.0, 0.0 },                  /* nmeaintv */
	" ", ""                      /* separator/program name */
	, 1                          /*sys of compatation*/
};
const static double gpst0[] = { 1980, 1, 6, 0, 0, 0 }; /* gps time reference */
//const static double gst0[] = { 1999, 8, 22, 0, 0, 0 }; /* galileo system time reference */
//const static double bdt0[] = { 2006, 1, 1, 0, 0, 0 }; /* beidou time reference */
/* coordinate rotation matrix ------------------------------------------------*/
#define Rx(t,X) do { \
	(X)[0] = 1.0; (X)[1] = (X)[2] = (X)[3] = (X)[6] = 0.0; \
	(X)[4] = (X)[8] = cos(t); (X)[7] = sin(t); (X)[5] = -(X)[7]; \
} while (0)

#define Ry(t,X) do { \
	(X)[4] = 1.0; (X)[1] = (X)[3] = (X)[5] = (X)[7] = 0.0; \
	(X)[0] = (X)[8] = cos(t); (X)[2] = sin(t); (X)[6] = -(X)[2]; \
} while (0)

#define Rz(t,X) do { \
	(X)[8] = 1.0; (X)[2] = (X)[5] = (X)[6] = (X)[7] = 0.0; \
	(X)[0] = (X)[4] = cos(t); (X)[3] = sin(t); (X)[1] = -(X)[3]; \
} while (0)
#define SQR(x)   ((x)*(x))
/* inner product ---------------------------------------------------------------
* inner product of vectors
* args   : double *a,*b     I   vector a,b (n x 1)
*          int    n         I   size of vector a,b
* return : a'*b
*-----------------------------------------------------------------------------*/
extern double dot(const double *a, const double *b, int n)
{
	double c = 0.0;

	while (--n >= 0) c += a[n] * b[n];
	return c;
}
/* euclid norm -----------------------------------------------------------------
* euclid norm of vector
* args   : double *a        I   vector a (n x 1)
*          int    n         I   size of vector a
* return : || a ||
*-----------------------------------------------------------------------------*/
extern double norm(const double *a, int n)
{
	return sqrt(dot(a, a, n));
}

/* get meterological parameters ----------------------------------------------*/
static void getmet(double lat, double *met)
{
	static const double metprm[][10] = { /* lat=15,30,45,60,75 */
		{ 1013.25, 299.65, 26.31, 6.30E-3, 2.77, 0.00, 0.00, 0.00, 0.00E-3, 0.00 },
		{ 1017.25, 294.15, 21.79, 6.05E-3, 3.15, -3.75, 7.00, 8.85, 0.25E-3, 0.33 },
		{ 1015.75, 283.15, 11.66, 5.58E-3, 2.57, -2.25, 11.00, 7.24, 0.32E-3, 0.46 },
		{ 1011.75, 272.15, 6.78, 5.39E-3, 1.81, -1.75, 15.00, 5.36, 0.81E-3, 0.74 },
		{ 1013.00, 263.65, 4.11, 4.53E-3, 1.55, -0.50, 14.50, 3.39, 0.62E-3, 0.30 }
	};
	int i, j;
	double a;
	lat = fabs(lat);
	if (lat <= 15.0) for (i = 0; i < 10; i++) met[i] = metprm[0][i];
	else if (lat >= 75.0) for (i = 0; i < 10; i++) met[i] = metprm[4][i];
	else {
		j = (int)(lat / 15.0); a = (lat - j*15.0) / 15.0;
		for (i = 0; i < 10; i++) met[i] = (1.0 - a)*metprm[j - 1][i] + a*metprm[j][i];
	}
}
/* convert calendar day/time to time -------------------------------------------
* convert calendar day/time to gtime_t struct
* args   : double *ep       I   day/time {year,month,day,hour,min,sec}
* return : gtime_t struct
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
extern gtime_t epoch2time(const double *ep)
{
	const int doy[] = { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
	gtime_t time = { 0 };
	int days, sec, year = (int)ep[0], mon = (int)ep[1], day = (int)ep[2];

	if (year < 1970 || 2099 < year || mon < 1 || 12 < mon) return time;

	/* leap year if year%4==0 in 1901-2099 */
	days = (year - 1970) * 365 + (year - 1969) / 4 + doy[mon - 1] + day - 2 + (year % 4 == 0 && mon >= 3 ? 1 : 0);
	sec = (int)floor(ep[5]);
	time.time = (time_t)days * 86400 + (int)ep[3] * 3600 + (int)ep[4] * 60 + sec;
	time.sec = ep[5] - sec;
	return time;
}
/* time to calendar day/time ---------------------------------------------------
* convert gtime_t struct to calendar day/time
* args   : gtime_t t        I   gtime_t struct
*          double *ep       O   day/time {year,month,day,hour,min,sec}
* return : none
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
extern void time2epoch(gtime_t t, double *ep)
{
	const int mday[] = { /* # of days in a month */
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	int days, sec, mon, day;

	/* leap year if year%4==0 in 1901-2099 */
	days = (int)(t.time / 86400);
	sec = (int)(t.time - (time_t)days * 86400);
	for (day = days % 1461, mon = 0; mon < 48; mon++) {
		if (day >= mday[mon]) day -= mday[mon]; else break;
	}
	ep[0] = 1970 + days / 1461 * 4 + mon / 12; ep[1] = mon % 12 + 1; ep[2] = day + 1;
	ep[3] = sec / 3600; ep[4] = sec % 3600 / 60; ep[5] = sec % 60 + t.sec;
}
/* time difference -------------------------------------------------------------
* difference between gtime_t structs
* args   : gtime_t t1,t2    I   gtime_t structs
* return : time difference (t1-t2) (s)
*-----------------------------------------------------------------------------*/
extern double timediff(gtime_t t1, gtime_t t2)
{
	return difftime(t1.time, t2.time) + t1.sec - t2.sec;
}
/* time to day of year ---------------------------------------------------------
* convert time to day of year
* args   : gtime_t t        I   gtime_t struct
* return : day of year (days)
*-----------------------------------------------------------------------------*/
extern double time2doy(gtime_t t)
{
	double ep[6];

	time2epoch(t, ep);
	ep[1] = ep[2] = 1.0; ep[3] = ep[4] = ep[5] = 0.0;
	return timediff(t, epoch2time(ep)) / 86400.0 + 1.0;
}
/* tropospheric delay correction -----------------------------------------------
* compute sbas tropospheric delay correction (mops model)
* args   : gtime_t time     I   time
*          double   *pos    I   receiver position {lat,lon,height} (rad/m)
*          double   *azel   I   satellite azimuth/elavation (rad)
*          double   *var    O   variance of troposphric error (m^2)
* return : slant tropospheric delay (m)
*-----------------------------------------------------------------------------*/
extern double sbstropcorr(gtime_t time, const double *pos, const double *azel,
	double *var)
{
	const double k1 = 77.604, k2 = 382000.0, rd = 287.054, gm = 9.784, g = 9.80665;
	static double pos_[3] = { 0 }, zh = 0.0, zw = 0.0;
	int i;
	double c, met[10], sinel = sin(azel[1]), h = pos[2], m;

	if (pos[2]<-100.0 || 10000.0<pos[2] || azel[1] <= 0) {
		*var = 0.0;
		return 0.0;
	}
	if (zh == 0.0 || fabs(pos[0] - pos_[0])>1E-7 || fabs(pos[1] - pos_[1])>1E-7 ||
		fabs(pos[2] - pos_[2]) > 1.0) {
		getmet(pos[0] * R2D, met);
		c = cos(2.0*PI*(time2doy(time) - (pos[0] >= 0.0 ? 28.0 : 211.0)) / 365.25);
		for (i = 0; i < 5; i++) met[i] -= met[i + 5] * c;
		zh = 1E-6*k1*rd*met[0] / gm;
		zw = 1E-6*k2*rd / (gm*(met[4] + 1.0) - met[3] * rd)*met[2] / met[1];
		zh *= pow(1.0 - met[3] * h / met[1], g / (rd*met[3]));
		zw *= pow(1.0 - met[3] * h / met[1], (met[4] + 1.0)*g / (rd*met[3]) - 1.0);
		for (i = 0; i < 3; i++) pos_[i] = pos[i];
	}
	m = 1.001 / sqrt(0.002001 + sinel*sinel);
	*var = 0.12*0.12*m*m;
	return (zh + zw)*m;
}
/* time to gps time ------------------------------------------------------------
* convert gtime_t struct to week and tow in gps time
* args   : gtime_t t        I   gtime_t struct
*          int    *week     IO  week number in gps time (NULL: no output)
* return : time of week in gps time (s)
*-----------------------------------------------------------------------------*/
extern double time2gpst(gtime_t t, int *week)
{
	gtime_t t0 = epoch2time(gpst0);
	time_t sec = t.time - t0.time;
	int w = (int)(sec / (86400 * 7));

	if (week) *week = w;
	return (double)(sec - w * 86400 * 7) + t.sec;
}
/* ionosphere model ------------------------------------------------------------
* compute ionospheric delay by broadcast ionosphere model (klobuchar model)
* args   : gtime_t t        I   time (gpst)
*          double *ion      I   iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3}
*          double *pos      I   receiver position {lat,lon,h} (rad,m)
*          double *azel     I   azimuth/elevation angle {az,el} (rad)
* return : ionospheric delay (L1) (m)
*-----------------------------------------------------------------------------*/
extern double ionmodel(gtime_t t, const double *ion, const double *pos,
	const double *azel)
{
	const double ion_default[] = { /* 2004/1/1 */
		0.1118E-07, -0.7451E-08, -0.5961E-07, 0.1192E-06,
		0.1167E+06, -0.2294E+06, -0.1311E+06, 0.1049E+07
	};
	double tt, f, psi, phi, lam, amp, per, x;
	int week;

	if (pos[2]<-1E3 || azel[1] <= 0) return 0.0;
	if (norm(ion, 8) <= 0.0) ion = ion_default;

	/* earth centered angle (semi-circle) */
	psi = 0.0137 / (azel[1] / PI + 0.11) - 0.022;

	/* subionospheric latitude/longitude (semi-circle) */
	phi = pos[0] / PI + psi*cos(azel[0]);
	if (phi> 0.416) phi = 0.416;
	else if (phi < -0.416) phi = -0.416;
	lam = pos[1] / PI + psi*sin(azel[0]) / cos(phi*PI);

	/* geomagnetic latitude (semi-circle) */
	phi += 0.064*cos((lam - 1.617)*PI);

	/* local time (s) */
	tt = 43200.0*lam + time2gpst(t, &week);
	tt -= floor(tt / 86400.0)*86400.0; /* 0<=tt<86400 */

	/* slant factor */
	f = 1.0 + 16.0*pow(0.53 - azel[1] / PI, 3.0);

	/* ionospheric delay */
	amp = ion[0] + phi*(ion[1] + phi*(ion[2] + phi*ion[3]));
	per = ion[4] + phi*(ion[5] + phi*(ion[6] + phi*ion[7]));
	amp = amp < 0.0 ? 0.0 : amp;
	per = per < 72000.0 ? 72000.0 : per;
	x = 2.0*PI*(tt - 50400.0) / per;

	return CLIGHT*f*(fabs(x) < 1.57 ? 5E-9 + amp*(1.0 + x*x*(-0.5 + x*x / 24.0)) : 5E-9);
}
/* ionospheric correction ------------------------------------------------------
* compute ionospheric correction
* args   : gtime_t time     I   time
*          nav_t  *nav      I   navigation data
*          int    sat       I   satellite number
*          double *pos      I   receiver position {lat,lon,h} (rad|m)
*          double *azel     I   azimuth/elevation angle {az,el} (rad)
*          int    ionoopt   I   ionospheric correction option (IONOOPT_???)
*          double *ion      O   ionospheric delay (L1) (m)
*          double *var      O   ionospheric delay (L1) variance (m^2)
* return : status(1:ok,0:error)
*-----------------------------------------------------------------------------*/
extern int ionocorr(gtime_t time, const nav_t *nav, int sat, const double *pos,
                    const double *azel, int ionoopt, double *ion, double *var)
{

	/* broadcast model */
	*ion = ionmodel(time, nav->ion_gps, pos, azel);
	*var = SQR(*ion*ERR_BRDCI);
	return 1;
}
/* troposphere model -----------------------------------------------------------
* compute tropospheric delay by standard atmosphere and saastamoinen model
* args   : gtime_t time     I   time
*          double *pos      I   receiver position {lat,lon,h} (rad,m)
*          double *azel     I   azimuth/elevation angle {az,el} (rad)
*          double humi      I   relative humidity
* return : tropospheric delay (m)
*-----------------------------------------------------------------------------*/
extern double tropmodel(gtime_t time, const double *pos, const double *azel,
	double humi)
{
	const double temp0 = 15.0; /* temparature at sea level */
	double hgt, pres, temp, e, z, trph, trpw;

	if (pos[2] < -100.0 || 1E4 < pos[2] || azel[1] <= 0) return 0.0;

	/* standard atmosphere */
	hgt = pos[2] < 0.0 ? 0.0 : pos[2];

	pres = 1013.25*pow(1.0 - 2.2557E-5*hgt, 5.2568);
	temp = temp0 - 6.5E-3*hgt + 273.16;
	e = 6.108*humi*exp((17.15*temp - 4684.0) / (temp - 38.45));

	/* saastamoninen model */
	z = PI / 2.0 - azel[1];
	trph = 0.0022768*pres / (1.0 - 0.00266*cos(2.0*pos[0]) - 0.00028*hgt / 1E3) / cos(z);
	trpw = 0.002277*(1255.0 / temp + 0.05)*e / cos(z);
	return trph + trpw;
}
static double interpc(const double coef[], double lat)
{
	int i = (int)(lat / 15.0);
	if (i<1) return coef[0]; else if (i>4) return coef[4];
	return coef[i - 1] * (1.0 - lat / 15.0 + i) + coef[i] * (lat / 15.0 - i);
}
static double mapf(double el, double a, double b, double c)
{
	double sinel = sin(el);
	return (1.0 + a / (1.0 + b / (1.0 + c))) / (sinel + (a / (sinel + b / (sinel + c))));
}
static double nmf(gtime_t time, const double pos[], const double azel[],
	double *mapfw)
{
	/* ref [5] table 3 */
	/* hydro-ave-a,b,c, hydro-amp-a,b,c, wet-a,b,c at latitude 15,30,45,60,75 */
	const double coef[][5] = {
		{ 1.2769934E-3, 1.2683230E-3, 1.2465397E-3, 1.2196049E-3, 1.2045996E-3 },
		{ 2.9153695E-3, 2.9152299E-3, 2.9288445E-3, 2.9022565E-3, 2.9024912E-3 },
		{ 62.610505E-3, 62.837393E-3, 63.721774E-3, 63.824265E-3, 64.258455E-3 },

		{ 0.0000000E-0, 1.2709626E-5, 2.6523662E-5, 3.4000452E-5, 4.1202191E-5 },
		{ 0.0000000E-0, 2.1414979E-5, 3.0160779E-5, 7.2562722E-5, 11.723375E-5 },
		{ 0.0000000E-0, 9.0128400E-5, 4.3497037E-5, 84.795348E-5, 170.37206E-5 },

		{ 5.8021897E-4, 5.6794847E-4, 5.8118019E-4, 5.9727542E-4, 6.1641693E-4 },
		{ 1.4275268E-3, 1.5138625E-3, 1.4572752E-3, 1.5007428E-3, 1.7599082E-3 },
		{ 4.3472961E-2, 4.6729510E-2, 4.3908931E-2, 4.4626982E-2, 5.4736038E-2 }
	};
	const double aht[] = { 2.53E-5, 5.49E-3, 1.14E-3 }; /* height correction */

	double y, cosy, ah[3], aw[3], dm, el = azel[1], lat = pos[0] * R2D, hgt = pos[2];
	int i;

	if (el <= 0.0) {
		if (mapfw) *mapfw = 0.0;
		return 0.0;
	}
	/* year from doy 28, added half a year for southern latitudes */
	y = (time2doy(time) - 28.0) / 365.25 + (lat < 0.0 ? 0.5 : 0.0);

	cosy = cos(2.0*PI*y);
	lat = fabs(lat);

	for (i = 0; i < 3; i++) {
		ah[i] = interpc(coef[i], lat) - interpc(coef[i + 3], lat)*cosy;
		aw[i] = interpc(coef[i + 6], lat);
	}
	/* ellipsoidal height is used instead of height above sea level */
	dm = (1.0 / sin(el) - mapf(el, aht[0], aht[1], aht[2]))*hgt / 1E3;

	if (mapfw) *mapfw = mapf(el, aw[0], aw[1], aw[2]);

	return mapf(el, ah[0], ah[1], ah[2]) + dm;
}
/* troposphere mapping function ------------------------------------------------
* compute tropospheric mapping function by NMF
* args   : gtime_t t        I   time
*          double *pos      I   receiver position {lat,lon,h} (rad,m)
*          double *azel     I   azimuth/elevation angle {az,el} (rad)
*          double *mapfw    IO  wet mapping function (NULL: not output)
* return : dry mapping function
* note   : see ref [5] (NMF) and [9] (GMF)
*          original JGR paper of [5] has bugs in eq.(4) and (5). the corrected
*          paper is obtained from:
*          ftp://web.haystack.edu/pub/aen/nmf/NMF_JGR.pdf
*-----------------------------------------------------------------------------*/
extern double tropmapf(gtime_t time, const double pos[], const double azel[],
	double *mapfw)
{
#ifdef IERS_MODEL
	const double ep[] = { 2000, 1, 1, 12, 0, 0 };
	double mjd, lat, lon, hgt, zd, gmfh, gmfw;
#endif
	if (pos[2]<-1000.0 || pos[2]>20000.0) {
		if (mapfw) *mapfw = 0.0;
		return 0.0;
	}
#ifdef IERS_MODEL
	mjd = 51544.5 + (timediff(time, epoch2time(ep))) / 86400.0;
	lat = pos[0];
	lon = pos[1];
	hgt = pos[2] - geoidh(pos); /* height in m (mean sea level) */
	zd = PI / 2.0 - azel[1];

	/* call GMF */
	gmf_(&mjd, &lat, &lon, &hgt, &zd, &gmfh, &gmfw);

	if (mapfw) *mapfw = gmfw;
	return gmfh;
#else
	return nmf(time, pos, azel, mapfw); /* NMF */
#endif
}
/* normalize 3d vector ---------------------------------------------------------
* normalize 3d vector
* args   : double *a        I   vector a (3 x 1)
*          double *b        O   normlized vector (3 x 1) || b || = 1
* return : status (1:ok,0:error)
*-----------------------------------------------------------------------------*/
extern int normv3(const double *a, double *b)
{
	double r;
	if ((r = norm(a, 3)) <= 0.0) return 0;
	b[0] = a[0] / r;
	b[1] = a[1] / r;
	b[2] = a[2] / r;
	return 1;
}
/* interpolate antenna phase center variation --------------------------------*/
static double interpvar(double ang, const double *var)
{
	double a = ang / 5.0; /* ang=0-90 */
	int i = (int)a;
	if (i < 0) return var[0]; else if (i >= 18) return var[18];
	return var[i] * (1.0 - a + i) + var[i + 1] * (a - i);
}
/* satellite antenna model ------------------------------------------------------
* compute satellite antenna phase center parameters
* args   : pcv_t *pcv       I   antenna phase center parameters
*          double nadir     I   nadir angle for satellite (rad)
*          double *dant     O   range offsets for each frequency (m)
* return : none
*-----------------------------------------------------------------------------*/
extern void antmodel_s(const pcv_t *pcv, double nadir, double *dant)
{
	int i;

	for (i = 0; i < NFREQ; i++) {
		dant[i] = interpvar(nadir*R2D*5.0, pcv->var[i]);
	}
}
/* satellite number to satellite system ----------------------------------------
* convert satellite number to satellite system
* args   : int    sat       I   satellite number (1-MAXSAT)
*          int    *prn      IO  satellite prn/slot number (NULL: no output)
* return : satellite system (SYS_GPS,SYS_GLO,...)
*-----------------------------------------------------------------------------*/
extern int satsys(int sat, int *prn)
{
	int sys = SYS_NONE;
	//printf("2.0.1\n");
	if (sat <= 0 || MAXSAT < sat) sat = 0;
	else if (sat <= NSATGPS) {
		sys = SYS_GPS; sat += MINPRNGPS - 1;
	}
	else if ((sat -= NSATGPS) <= NSATGLO) {
		sys = SYS_GLO; sat += MINPRNGLO - 1;
	}
	else if ((sat -= NSATGLO) <= NSATGAL) {
		sys = SYS_GAL; sat += MINPRNGAL - 1;
	}
	else if ((sat -= NSATGAL) <= NSATQZS) {
		sys = SYS_QZS; sat += MINPRNQZS - 1;
	}
	else if ((sat -= NSATQZS) <= NSATCMP) {
		sys = SYS_CMP; sat += MINPRNCMP - 1;
	}
	else if ((sat -= NSATCMP) <= NSATLEO) {
		sys = SYS_LEO; sat += MINPRNLEO - 1;
	}
	else if ((sat -= NSATLEO) <= NSATSBS) {
		sys = SYS_SBS; sat += MINPRNSBS - 1;
	}
	else sat = 0;
	//printf("2.0.2\n");
	//if (prn) *prn = sat;
	//printf("2.0.3\n");
	return sys;
}
/* satellite system+prn/slot number to satellite number ------------------------
* convert satellite system+prn/slot number to satellite number
* args   : int    sys       I   satellite system (SYS_GPS,SYS_GLO,...)
*          int    prn       I   satellite prn/slot number
* return : satellite number (0:error)
*-----------------------------------------------------------------------------*/
extern int satno(int sys, int prn)
{
	if (prn <= 0) return 0;
	switch (sys) {
	case SYS_GPS:
		if (prn < MINPRNGPS || MAXPRNGPS < prn) return 0;
		return prn - MINPRNGPS + 1;
	case SYS_GLO:
		if (prn < MINPRNGLO || MAXPRNGLO < prn) return 0;
		return NSATGPS + prn - MINPRNGLO + 1;
	case SYS_GAL:
		if (prn < MINPRNGAL || MAXPRNGAL < prn) return 0;
		return NSATGPS + NSATGLO + prn - MINPRNGAL + 1;
	case SYS_QZS:
		if (prn < MINPRNQZS || MAXPRNQZS < prn) return 0;
		return NSATGPS + NSATGLO + NSATGAL + prn - MINPRNQZS + 1;
	case SYS_CMP:
		if (prn < MINPRNCMP || MAXPRNCMP < prn) return 0;
		return NSATGPS + NSATGLO + NSATGAL + NSATQZS + prn - MINPRNCMP + 1;
	case SYS_LEO:
		if (prn < MINPRNLEO || MAXPRNLEO < prn) return 0;
		return NSATGPS + NSATGLO + NSATGAL + NSATQZS + NSATCMP + prn - MINPRNLEO + 1;
	case SYS_SBS:
		if (prn < MINPRNSBS || MAXPRNSBS < prn) return 0;
		return NSATGPS + NSATGLO + NSATGAL + NSATQZS + NSATCMP + NSATLEO + prn - MINPRNSBS + 1;
	}
	return 0;
}
/* gpstime to utc --------------------------------------------------------------
* convert gpstime to utc considering leap seconds
* args   : gtime_t t        I   time expressed in gpstime
* return : time expressed in utc
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t gpst2utc(gtime_t t)
{
	gtime_t tu;
	int i;

	for (i = 0; i < (int)sizeof(leaps) / (int)sizeof(*leaps); i++) {
		tu = timeadd(t, leaps[i][6]);
		if (timediff(tu, epoch2time(leaps[i])) >= 0.0) return tu;
	}
	return t;
}
/* astronomical arguments: f={l,l',F,D,OMG} (rad) ----------------------------*/
static void ast_args(double t, double *f)
{
	static const double fc[][5] = { /* coefficients for iau 1980 nutation */
		{ 134.96340251, 1717915923.2178, 31.8792, 0.051635, -0.00024470 },
		{ 357.52910918, 129596581.0481, -0.5532, 0.000136, -0.00001149 },
		{ 93.27209062, 1739527262.8478, -12.7512, -0.001037, 0.00000417 },
		{ 297.85019547, 1602961601.2090, -6.3706, 0.006593, -0.00003169 },
		{ 125.04455501, -6962890.2665, 7.4722, 0.007702, -0.00005939 }
	};
	double tt[4];
	int i, j;

	for (tt[0] = t, i = 1; i < 4; i++) tt[i] = tt[i - 1] * t;
	for (i = 0; i < 5; i++) {
		f[i] = fc[i][0] * 3600.0;
		for (j = 0; j < 4; j++) f[i] += fc[i][j + 1] * tt[j];
		f[i] = fmod(f[i] * AS2R, 2.0*PI);
	}
}
/* sun and moon position in eci (ref [4] 5.1.1, 5.2.1) -----------------------*/
static void sunmoonpos_eci(gtime_t tut, double *rsun, double *rmoon)
{
	const double ep2000[] = { 2000, 1, 1, 12, 0, 0 };
	double t, f[5], eps, Ms, ls, rs, lm, pm, rm, sine, cose, sinp, cosp, sinl, cosl;

	t = timediff(tut, epoch2time(ep2000)) / 86400.0 / 36525.0;

	/* astronomical arguments */
	ast_args(t, f);

	/* obliquity of the ecliptic */
	eps = 23.439291 - 0.0130042*t;
	sine = sin(eps*D2R); cose = cos(eps*D2R);

	/* sun position in eci */
	if (rsun) {
		Ms = 357.5277233 + 35999.05034*t;
		ls = 280.460 + 36000.770*t + 1.914666471*sin(Ms*D2R) + 0.019994643*sin(2.0*Ms*D2R);
		rs = AU*(1.000140612 - 0.016708617*cos(Ms*D2R) - 0.000139589*cos(2.0*Ms*D2R));
		sinl = sin(ls*D2R); cosl = cos(ls*D2R);
		rsun[0] = rs*cosl;
		rsun[1] = rs*cose*sinl;
		rsun[2] = rs*sine*sinl;
	}
	/* moon position in eci */
	if (rmoon) {
		lm = 218.32 + 481267.883*t + 6.29*sin(f[0]) - 1.27*sin(f[0] - 2.0*f[3]) +
			0.66*sin(2.0*f[3]) + 0.21*sin(2.0*f[0]) - 0.19*sin(f[1]) - 0.11*sin(2.0*f[2]);
		pm = 5.13*sin(f[2]) + 0.28*sin(f[0] + f[2]) - 0.28*sin(f[2] - f[0]) -
			0.17*sin(f[2] - 2.0*f[3]);
		rm = RE_WGS84 / sin((0.9508 + 0.0518*cos(f[0]) + 0.0095*cos(f[0] - 2.0*f[3]) +
			0.0078*cos(2.0*f[3]) + 0.0028*cos(2.0*f[0]))*D2R);
		sinl = sin(lm*D2R); cosl = cos(lm*D2R);
		sinp = sin(pm*D2R); cosp = cos(pm*D2R);
		rmoon[0] = rm*cosp*cosl;
		rmoon[1] = rm*(cose*cosp*sinl - sine*sinp);
		rmoon[2] = rm*(sine*cosp*sinl + cose*sinp);
	}
}
/* multiply matrix -----------------------------------------------------------*/
extern void matmul(const char *tr, int n, int k, int m, double alpha,
	const double *A, const double *B, double beta, double *C)
{
	double d;
	int i, j, x, f = tr[0] == 'N' ? (tr[1] == 'N' ? 1 : 2) : (tr[1] == 'N' ? 3 : 4);

	for (i = 0; i < n; i++)
	for (j = 0; j < k; j++) {
		//if (i==4&&j==0)
		//{
		//	int kkk=0;
		//}

		d = 0.0;
		switch (f) {
		case 1: for (x = 0; x < m; x++)
			d += A[i + x*n] * B[x + j*m]; break;
		case 2: for (x = 0; x < m; x++)
			d += A[i + x*n] * B[j + x*k]; break;
		case 3: for (x = 0; x < m; x++) d += A[x + i*m] * B[x + j*m]; break;
		case 4: for (x = 0; x < m; x++) d += A[x + i*m] * B[j + x*k]; break;
		}
		if (beta == 0.0) C[i + j*n] = alpha*d; else C[i + j*n] = alpha*d + beta*C[i + j*n];
	}
}
/* sun and moon position -------------------------------------------------------
* get sun and moon position in ecef
* args   : gtime_t tut      I   time in ut1
*          double *erpv     I   erp value {xp,yp,ut1_utc,lod} (rad,rad,s,s/d)
*          double *rsun     IO  sun position in ecef  (m) (NULL: not output)
*          double *rmoon    IO  moon position in ecef (m) (NULL: not output)
*          double *gmst     O   gmst (rad)
* return : none
*-----------------------------------------------------------------------------*/
extern void sunmoonpos(gtime_t tutc, const double *erpv, double *rsun,
	double *rmoon, double *gmst)
{
	gtime_t tut;
	double rs[3], rm[3], U[9], gmst_;

	tut = timeadd(tutc, erpv[2]); /* utc -> ut1 */

	/* sun and moon position in eci */
	sunmoonpos_eci(tut, rsun ? rs : NULL, rmoon ? rm : NULL);

	/* eci to ecef transformation matrix */
	eci2ecef(tutc, erpv, U, &gmst_);

	/* sun and moon postion in ecef */
	if (rsun) matmul("NN", 3, 1, 3, 1.0, U, rs, 0.0, rsun);
	if (rmoon) matmul("NN", 3, 1, 3, 1.0, U, rm, 0.0, rmoon);
	if (gmst) *gmst = gmst_;
}
/* copy matrix -----------------------------------------------------------------
* copy matrix
* args   : double *A        O   destination matrix A (n x m)
*          double *B        I   source matrix B (n x m)
*          int    n,m       I   number of rows and columns of matrix
* return : none
*-----------------------------------------------------------------------------*/
extern void matcpy(double *A, const double *B, int n, int m)
{
	memcpy(A, B, sizeof(double)*n*m);
}

/* geometric distance ----------------------------------------------------------
* compute geometric distance and receiver-to-satellite unit vector
* args   : double *rs       I   satellilte position (ecef at transmission) (m)
*          double *rr       I   receiver position (ecef at reception) (m)
*          double *e        O   line-of-sight vector (ecef)
* return : geometric distance (m) (0>:error/no satellite position)
* notes  : distance includes sagnac effect correction
*-----------------------------------------------------------------------------*/
extern double geodist(const double *rs, const double *rr, double *e)
{
	double r;
	int i;

	if (norm(rs, 3) < RE_WGS84) return -1.0;
	for (i = 0; i < 3; i++)
		e[i] = rs[i] - rr[i];
	r = norm(e, 3);
	for (i = 0; i < 3; i++)
		e[i] /= r;
	return r + OMGE*(rs[0] * rr[1] - rs[1] * rr[0]) / CLIGHT;
}

/* satellite azimuth/elevation angle -------------------------------------------
* compute satellite azimuth/elevation angle
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *e        I   receiver-to-satellilte unit vevtor (ecef)
*          double *azel     IO  azimuth/elevation {az,el} (rad) (NULL: no output)
*                               (0.0<=azel[0]<2*pi,-pi/2<=azel[1]<=pi/2)
* return : elevation angle (rad)
*-----------------------------------------------------------------------------*/
extern double satazel(const double *pos, const double *e, double *azel)
{
	double az = 0.0, el = PI / 2.0, enu[3];

	if (pos[2] > -RE_WGS84) {
		ecef2enu(pos, e, enu);
		az = dot(enu, enu, 2) < 1E-12 ? 0.0 : atan2(enu[0], enu[1]);
		if (az < 0.0) az += 2 * PI;
		el = asin(enu[2]);
	}
	if (azel) { azel[0] = az; azel[1] = el; }
	return el;
}
/* convert degree to deg-min-sec -----------------------------------------------
* convert degree to degree-minute-second
* args   : double deg       I   degree
*          double *dms      O   degree-minute-second {deg,min,sec}
* return : none
*-----------------------------------------------------------------------------*/
extern void deg2dms(double deg, double *dms)
{
	double sign = deg < 0.0 ? -1.0 : 1.0, a = fabs(deg);
	dms[0] = floor(a); a = (a - dms[0])*60.0;
	dms[1] = floor(a); a = (a - dms[1])*60.0;
	dms[2] = a; dms[0] *= sign;
}
/* convert deg-min-sec to degree -----------------------------------------------
* convert degree-minute-second to degree
* args   : double *dms      I   degree-minute-second {deg,min,sec}
* return : degree
*-----------------------------------------------------------------------------*/
extern double dms2deg(const double *dms)
{
	double sign = dms[0] < 0.0 ? -1.0 : 1.0;
	return sign*(fabs(dms[0]) + dms[1] / 60.0 + dms[2] / 3600.0);
}
/* transform ecef to geodetic postion ------------------------------------------
* transform ecef position to geodetic position
* args   : double *r        I   ecef position {x,y,z} (m)
*          double *pos      O   geodetic position {lat,lon,h} (rad,m)
* return : none
* notes  : WGS84, ellipsoidal height
*-----------------------------------------------------------------------------*/
extern void ecef2pos(const double *r, double *pos)
{
	double e2 = FE_WGS84*(2.0 - FE_WGS84), r2 = dot(r, r, 2), z, zk, v = RE_WGS84, sinp;

	for (z = r[2], zk = 0.0; fabs(z - zk) >= 1E-4;) {
		zk = z;
		sinp = z / sqrt(r2 + z*z);
		v = RE_WGS84 / sqrt(1.0 - e2*sinp*sinp);
		z = r[2] + v*e2*sinp;
	}
	pos[0] = r2 > 1E-12 ? atan(z / sqrt(r2)) : (r[2] > 0.0 ? PI / 2.0 : -PI / 2.0);
	pos[1] = r2 > 1E-12 ? atan2(r[1], r[0]) : 0.0;
	pos[2] = sqrt(r2 + z*z) - v;
}
/* transform geodetic to ecef position -----------------------------------------
* transform geodetic position to ecef position
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *r        O   ecef position {x,y,z} (m)
* return : none
* notes  : WGS84, ellipsoidal height
*-----------------------------------------------------------------------------*/
extern void pos2ecef(const double *pos, double *r)
{
	double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = cos(pos[1]);
	double e2 = FE_WGS84*(2.0 - FE_WGS84), v = RE_WGS84 / sqrt(1.0 - e2*sinp*sinp);

	r[0] = (v + pos[2])*cosp*cosl;
	r[1] = (v + pos[2])*cosp*sinl;
	r[2] = (v*(1.0 - e2) + pos[2])*sinp;
}
/* ecef to local coordinate transfromation matrix ------------------------------
* compute ecef to local coordinate transfromation matrix
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *E        O   ecef to local coord transformation matrix (3x3)
* return : none
* notes  : matirix stored by column-major order (fortran convention)
*-----------------------------------------------------------------------------*/
extern void xyz2enu(const double *pos, double *E)
{
	double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = cos(pos[1]);

	E[0] = -sinl;      E[3] = cosl;       E[6] = 0.0;
	E[1] = -sinp*cosl; E[4] = -sinp*sinl; E[7] = cosp;
	E[2] = cosp*cosl;  E[5] = cosp*sinl;  E[8] = sinp;
}
/* transform ecef vector to local tangental coordinate -------------------------
* transform ecef vector to local tangental coordinate
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *r        I   vector in ecef coordinate {x,y,z}
*          double *e        O   vector in local tangental coordinate {e,n,u}
* return : none
*-----------------------------------------------------------------------------*/
extern void ecef2enu(const double *pos, const double *r, double *e)
{
	double E[9];

	xyz2enu(pos, E);
	matmul("NN", 3, 1, 3, 1.0, E, r, 0.0, e);
}
/* test excluded satellite -----------------------------------------------------
* test excluded satellite
* args   : int    sat       I   satellite number
*          int    svh       I   sv health flag
*          prcopt_t *opt    I   processing options (NULL: not used)
* return : status (1:excluded,0:not excluded)
*-----------------------------------------------------------------------------*/
extern int satexclude(int sat, int svh, const prcopt_t *opt)
{
	int sys = satsys(sat, NULL);

	if (svh < 0) return 1; /* ephemeris unavailable */

	if (opt) {
		if (opt->exsats[sat - 1] == 1) return 1; /* excluded satellite */
		if (opt->exsats[sat - 1] == 2) return 0; /* included satellite */
		if (!(sys&opt->navsys)) return 1; /* unselected sat sys */
	}
	if (sys == SYS_QZS) svh &= 0xFE; /* mask QZSS LEX health */
	if (svh) {
		return 1;
	}
	return 0;
}
/* phase windup correction -----------------------------------------------------
* phase windup correction (ref [7] 5.1.2)
* args   : gtime_t time     I   time (GPST)
*          double  *rs      I   satellite position (ecef) {x,y,z} (m)
*          double  *rr      I   receiver  position (ecef) {x,y,z} (m)
*          double  *phw     IO  phase windup correction (cycle)
* return : none
* notes  : the previous value of phase windup correction should be set to *phw
*          as an input. the function assumes windup correction has no jump more
*          than 0.5 cycle.
*-----------------------------------------------------------------------------*/
extern void windupcorr(gtime_t time, const double *rs, const double *rr,
	double *phw)
{
	double ek[3], exs[3], eys[3], ezs[3], ess[3], exr[3], eyr[3], eks[3], ekr[3], E[9];
	double dr[3], ds[3], drs[3], r[3], pos[3], rsun[3], cosp, ph, erpv[5] = { 0 };
	int i;

	/* sun position in ecef */
	sunmoonpos(gpst2utc(time), erpv, rsun, NULL, NULL);

	/* unit vector satellite to receiver */
	for (i = 0; i < 3; i++) r[i] = rr[i] - rs[i];
	if (!normv3(r, ek)) return;

	/* unit vectors of satellite antenna */
	for (i = 0; i < 3; i++) r[i] = -rs[i];
	if (!normv3(r, ezs)) return;
	for (i = 0; i < 3; i++) r[i] = rsun[i] - rs[i];
	if (!normv3(r, ess)) return;
	cross3(ezs, ess, r);
	if (!normv3(r, eys)) return;
	cross3(eys, ezs, exs);

	/* unit vectors of receiver antenna */
	ecef2pos(rr, pos);
	xyz2enu(pos, E);
	exr[0] = E[1]; exr[1] = E[4]; exr[2] = E[7]; /* x = north */
	eyr[0] = -E[0]; eyr[1] = -E[3]; eyr[2] = -E[6]; /* y = west  */

	/* phase windup effect */
	cross3(ek, eys, eks);
	cross3(ek, eyr, ekr);
	for (i = 0; i < 3; i++) {
		ds[i] = exs[i] - ek[i] * dot(ek, exs, 3) - eks[i];
		dr[i] = exr[i] - ek[i] * dot(ek, exr, 3) + ekr[i];
	}
	cosp = dot(ds, dr, 3) / norm(ds, 3) / norm(dr, 3);
	if (cosp<-1.0) cosp = -1.0;
	else if (cosp> 1.0) cosp = 1.0;
	ph = acos(cosp) / 2.0 / PI;
	cross3(ds, dr, drs);
	if (dot(ek, drs, 3) < 0.0) ph = -ph;

	*phw = ph + floor(*phw - ph + 0.5); /* in cycle */
}
/* identity matrix -------------------------------------------------------------
* generate new identity matrix
* args   : int    n         I   number of rows and columns of matrix
* return : matrix pointer (if n<=0, return NULL)
*-----------------------------------------------------------------------------*/
extern int eye(double *p,int n)
{
	int i = 0;; int j = 0;

	for (i = 0; i < n; i++){
		for (j = 0; j < n; j++){
			if (i == j){
				p[i + j*n] = 1.0;
			}
			else{
				p[i + j*n] = 0;
			}
		}
	}
	return 0;
}
/* kalman filter ---------------------------------------------------------------
* kalman filter state update as follows:
*
*   K=P*H*(H'*P*H+R)^-1, xp=x+K*v, Pp=(I-K*H')*P
*
* args   : double *x        I   states vector (n x 1)
*          double *P        I   covariance matrix of states (n x n)
*          double *H        I   transpose of design matrix (n x m)
*          double *v        I   innovation (measurement - model) (m x 1)
*          double *R        I   covariance matrix of measurement error (m x m)
*          int    n,m       I   number of states and measurements
*          double *xp       O   states vector after update (n x 1)
*          double *Pp       O   covariance matrix of states after update (n x n)
* return : status (0:ok,<0:error)
* notes  : matirix stored by column-major order (fortran convention)
*          if state x[i]==0.0, not updates state x[i]/P[i+i*n]
*-----------------------------------------------------------------------------*/
static int filter_(const double *x, const double *P, const double *H,
	const double *v, const double *R, int n, int m,
	double *xp, double *Pp)
{
	double F[MAXsat_ * MAXsat_];
	double Q[MAXsat_ * MAXsat_];
	double K[MAXsat_ * MAXsat_];
	double I[MAXsat_ * MAXsat_];
	eye(I,n);
	int info;

	matcpy(Q, R, m, m);
	matcpy(xp, x, n, 1);
	matmul("NN", n, m, n, 1.0, P, H, 0.0, F);       /* Q=H'*P*H+R *///������������ʽ��;
	matmul("TN", m, m, n, 1.0, H, F, 1.0, Q);
	if (!(info = matinv(Q, m))) {
		matmul("NN", n, m, m, 1.0, F, Q, 0.0, K);   /* K=P*H*Q^-1 */
		matmul("NN", n, 1, m, 1.0, K, v, 1.0, xp);  /* xp=x+K*v */
		matmul("NT", n, n, m, -1.0, K, H, 1.0, I);  /* Pp=(I-K*H')*P *///������������ʽ��;
		matmul("NN", n, n, n, 1.0, I, P, 0.0, Pp);
	}
	return info;
}
extern int filter(double *x, double *P, const double *H, const double *v,
	const double *R, int n, int m)
{
	int i, j, k, info/*, *ix*/;
	int ix[50];
	for (i = k = 0; i<n; i++){
		if (x[i] != 0.0&&P[i + i*n]>0.0)
			ix[k++] = i;//��ȡ��x�в�Ϊ0�Ĳ��֣�Ҳ������ȡ���е���������ͽ��ջ��Ӳ�����������㣬ģ���ȵ�;
	}
	double x_[50];
	double xp_[50];
	double P_[50 * 50];
	double Pp_[50 * 50];
	double H_[135 * MAXOBS_];
	if (135 < n){
		return 0;
	}
	for (i = 0; i < k; i++) {
		x_[i] = x[ix[i]];
		for (j = 0; j < k; j++) P_[i + j*k] = P[ix[i] + ix[j] * n];
		for (j = 0; j < m; j++) H_[i + j*k] = H[ix[i] + j*n];
	}
	info = filter_(x_, P_, H_, v, R, k, m, xp_, Pp_);
	for (i = 0; i < k; i++) {
		x[ix[i]] = xp_[i];
		for (j = 0; j < k; j++)
			P[ix[i] + ix[j] * n] = Pp_[i + j*k];
	}
	return info;
}


/* outer product of 3d vectors -------------------------------------------------
* outer product of 3d vectors
* args   : double *a,*b     I   vector a,b (3 x 1)
*          double *c        O   outer product (a x b) (3 x 1)
* return : none
*-----------------------------------------------------------------------------*/
extern void cross3(const double *a, const double *b, double *c)
{
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}

/* add time --------------------------------------------------------------------
* add time to gtime_t struct
* args   : gtime_t t        I   gtime_t struct
*          double sec       I   time to add (s)
* return : gtime_t struct (t+sec)
*-----------------------------------------------------------------------------*/
extern gtime_t timeadd(gtime_t t, double sec)
{
	double tt;

	t.sec += sec; tt = floor(t.sec); t.time += (int)tt; t.sec -= tt;
	return t;
}
/* iau 1980 nutation ---------------------------------------------------------*/
static void nut_iau1980(double t, const double *f, double *dpsi, double *deps)
{
	static const double nut[106][10] = {
		{ 0, 0, 0, 0, 1, -6798.4, -171996, -174.2, 92025, 8.9 },
		{ 0, 0, 2, -2, 2, 182.6, -13187, -1.6, 5736, -3.1 },
		{ 0, 0, 2, 0, 2, 13.7, -2274, -0.2, 977, -0.5 },
		{ 0, 0, 0, 0, 2, -3399.2, 2062, 0.2, -895, 0.5 },
		{ 0, -1, 0, 0, 0, -365.3, -1426, 3.4, 54, -0.1 },
		{ 1, 0, 0, 0, 0, 27.6, 712, 0.1, -7, 0.0 },
		{ 0, 1, 2, -2, 2, 121.7, -517, 1.2, 224, -0.6 },
		{ 0, 0, 2, 0, 1, 13.6, -386, -0.4, 200, 0.0 },
		{ 1, 0, 2, 0, 2, 9.1, -301, 0.0, 129, -0.1 },
		{ 0, -1, 2, -2, 2, 365.2, 217, -0.5, -95, 0.3 },
		{ -1, 0, 0, 2, 0, 31.8, 158, 0.0, -1, 0.0 },
		{ 0, 0, 2, -2, 1, 177.8, 129, 0.1, -70, 0.0 },
		{ -1, 0, 2, 0, 2, 27.1, 123, 0.0, -53, 0.0 },
		{ 1, 0, 0, 0, 1, 27.7, 63, 0.1, -33, 0.0 },
		{ 0, 0, 0, 2, 0, 14.8, 63, 0.0, -2, 0.0 },
		{ -1, 0, 2, 2, 2, 9.6, -59, 0.0, 26, 0.0 },
		{ -1, 0, 0, 0, 1, -27.4, -58, -0.1, 32, 0.0 },
		{ 1, 0, 2, 0, 1, 9.1, -51, 0.0, 27, 0.0 },
		{ -2, 0, 0, 2, 0, -205.9, -48, 0.0, 1, 0.0 },
		{ -2, 0, 2, 0, 1, 1305.5, 46, 0.0, -24, 0.0 },
		{ 0, 0, 2, 2, 2, 7.1, -38, 0.0, 16, 0.0 },
		{ 2, 0, 2, 0, 2, 6.9, -31, 0.0, 13, 0.0 },
		{ 2, 0, 0, 0, 0, 13.8, 29, 0.0, -1, 0.0 },
		{ 1, 0, 2, -2, 2, 23.9, 29, 0.0, -12, 0.0 },
		{ 0, 0, 2, 0, 0, 13.6, 26, 0.0, -1, 0.0 },
		{ 0, 0, 2, -2, 0, 173.3, -22, 0.0, 0, 0.0 },
		{ -1, 0, 2, 0, 1, 27.0, 21, 0.0, -10, 0.0 },
		{ 0, 2, 0, 0, 0, 182.6, 17, -0.1, 0, 0.0 },
		{ 0, 2, 2, -2, 2, 91.3, -16, 0.1, 7, 0.0 },
		{ -1, 0, 0, 2, 1, 32.0, 16, 0.0, -8, 0.0 },
		{ 0, 1, 0, 0, 1, 386.0, -15, 0.0, 9, 0.0 },
		{ 1, 0, 0, -2, 1, -31.7, -13, 0.0, 7, 0.0 },
		{ 0, -1, 0, 0, 1, -346.6, -12, 0.0, 6, 0.0 },
		{ 2, 0, -2, 0, 0, -1095.2, 11, 0.0, 0, 0.0 },
		{ -1, 0, 2, 2, 1, 9.5, -10, 0.0, 5, 0.0 },
		{ 1, 0, 2, 2, 2, 5.6, -8, 0.0, 3, 0.0 },
		{ 0, -1, 2, 0, 2, 14.2, -7, 0.0, 3, 0.0 },
		{ 0, 0, 2, 2, 1, 7.1, -7, 0.0, 3, 0.0 },
		{ 1, 1, 0, -2, 0, -34.8, -7, 0.0, 0, 0.0 },
		{ 0, 1, 2, 0, 2, 13.2, 7, 0.0, -3, 0.0 },
		{ -2, 0, 0, 2, 1, -199.8, -6, 0.0, 3, 0.0 },
		{ 0, 0, 0, 2, 1, 14.8, -6, 0.0, 3, 0.0 },
		{ 2, 0, 2, -2, 2, 12.8, 6, 0.0, -3, 0.0 },
		{ 1, 0, 0, 2, 0, 9.6, 6, 0.0, 0, 0.0 },
		{ 1, 0, 2, -2, 1, 23.9, 6, 0.0, -3, 0.0 },
		{ 0, 0, 0, -2, 1, -14.7, -5, 0.0, 3, 0.0 },
		{ 0, -1, 2, -2, 1, 346.6, -5, 0.0, 3, 0.0 },
		{ 2, 0, 2, 0, 1, 6.9, -5, 0.0, 3, 0.0 },
		{ 1, -1, 0, 0, 0, 29.8, 5, 0.0, 0, 0.0 },
		{ 1, 0, 0, -1, 0, 411.8, -4, 0.0, 0, 0.0 },
		{ 0, 0, 0, 1, 0, 29.5, -4, 0.0, 0, 0.0 },
		{ 0, 1, 0, -2, 0, -15.4, -4, 0.0, 0, 0.0 },
		{ 1, 0, -2, 0, 0, -26.9, 4, 0.0, 0, 0.0 },
		{ 2, 0, 0, -2, 1, 212.3, 4, 0.0, -2, 0.0 },
		{ 0, 1, 2, -2, 1, 119.6, 4, 0.0, -2, 0.0 },
		{ 1, 1, 0, 0, 0, 25.6, -3, 0.0, 0, 0.0 },
		{ 1, -1, 0, -1, 0, -3232.9, -3, 0.0, 0, 0.0 },
		{ -1, -1, 2, 2, 2, 9.8, -3, 0.0, 1, 0.0 },
		{ 0, -1, 2, 2, 2, 7.2, -3, 0.0, 1, 0.0 },
		{ 1, -1, 2, 0, 2, 9.4, -3, 0.0, 1, 0.0 },
		{ 3, 0, 2, 0, 2, 5.5, -3, 0.0, 1, 0.0 },
		{ -2, 0, 2, 0, 2, 1615.7, -3, 0.0, 1, 0.0 },
		{ 1, 0, 2, 0, 0, 9.1, 3, 0.0, 0, 0.0 },
		{ -1, 0, 2, 4, 2, 5.8, -2, 0.0, 1, 0.0 },
		{ 1, 0, 0, 0, 2, 27.8, -2, 0.0, 1, 0.0 },
		{ -1, 0, 2, -2, 1, -32.6, -2, 0.0, 1, 0.0 },
		{ 0, -2, 2, -2, 1, 6786.3, -2, 0.0, 1, 0.0 },
		{ -2, 0, 0, 0, 1, -13.7, -2, 0.0, 1, 0.0 },
		{ 2, 0, 0, 0, 1, 13.8, 2, 0.0, -1, 0.0 },
		{ 3, 0, 0, 0, 0, 9.2, 2, 0.0, 0, 0.0 },
		{ 1, 1, 2, 0, 2, 8.9, 2, 0.0, -1, 0.0 },
		{ 0, 0, 2, 1, 2, 9.3, 2, 0.0, -1, 0.0 },
		{ 1, 0, 0, 2, 1, 9.6, -1, 0.0, 0, 0.0 },
		{ 1, 0, 2, 2, 1, 5.6, -1, 0.0, 1, 0.0 },
		{ 1, 1, 0, -2, 1, -34.7, -1, 0.0, 0, 0.0 },
		{ 0, 1, 0, 2, 0, 14.2, -1, 0.0, 0, 0.0 },
		{ 0, 1, 2, -2, 0, 117.5, -1, 0.0, 0, 0.0 },
		{ 0, 1, -2, 2, 0, -329.8, -1, 0.0, 0, 0.0 },
		{ 1, 0, -2, 2, 0, 23.8, -1, 0.0, 0, 0.0 },
		{ 1, 0, -2, -2, 0, -9.5, -1, 0.0, 0, 0.0 },
		{ 1, 0, 2, -2, 0, 32.8, -1, 0.0, 0, 0.0 },
		{ 1, 0, 0, -4, 0, -10.1, -1, 0.0, 0, 0.0 },
		{ 2, 0, 0, -4, 0, -15.9, -1, 0.0, 0, 0.0 },
		{ 0, 0, 2, 4, 2, 4.8, -1, 0.0, 0, 0.0 },
		{ 0, 0, 2, -1, 2, 25.4, -1, 0.0, 0, 0.0 },
		{ -2, 0, 2, 4, 2, 7.3, -1, 0.0, 1, 0.0 },
		{ 2, 0, 2, 2, 2, 4.7, -1, 0.0, 0, 0.0 },
		{ 0, -1, 2, 0, 1, 14.2, -1, 0.0, 0, 0.0 },
		{ 0, 0, -2, 0, 1, -13.6, -1, 0.0, 0, 0.0 },
		{ 0, 0, 4, -2, 2, 12.7, 1, 0.0, 0, 0.0 },
		{ 0, 1, 0, 0, 2, 409.2, 1, 0.0, 0, 0.0 },
		{ 1, 1, 2, -2, 2, 22.5, 1, 0.0, -1, 0.0 },
		{ 3, 0, 2, -2, 2, 8.7, 1, 0.0, 0, 0.0 },
		{ -2, 0, 2, 2, 2, 14.6, 1, 0.0, -1, 0.0 },
		{ -1, 0, 0, 0, 2, -27.3, 1, 0.0, -1, 0.0 },
		{ 0, 0, -2, 2, 1, -169.0, 1, 0.0, 0, 0.0 },
		{ 0, 1, 2, 0, 1, 13.1, 1, 0.0, 0, 0.0 },
		{ -1, 0, 4, 0, 2, 9.1, 1, 0.0, 0, 0.0 },
		{ 2, 1, 0, -2, 0, 131.7, 1, 0.0, 0, 0.0 },
		{ 2, 0, 0, 2, 0, 7.1, 1, 0.0, 0, 0.0 },
		{ 2, 0, 2, -2, 1, 12.8, 1, 0.0, -1, 0.0 },
		{ 2, 0, -2, 0, 1, -943.2, 1, 0.0, 0, 0.0 },
		{ 1, -1, 0, -2, 0, -29.3, 1, 0.0, 0, 0.0 },
		{ -1, 0, 0, 1, 1, -388.3, 1, 0.0, 0, 0.0 },
		{ -1, -1, 0, 2, 1, 35.0, 1, 0.0, 0, 0.0 },
		{ 0, 1, 0, 1, 0, 27.3, 1, 0.0, 0, 0.0 }
	};
	double ang;
	int i, j;

	*dpsi = *deps = 0.0;

	for (i = 0; i < 106; i++) {
		ang = 0.0;
		for (j = 0; j < 5; j++) ang += nut[i][j] * f[j];
		*dpsi += (nut[i][6] + nut[i][7] * t)*sin(ang);
		*deps += (nut[i][8] + nut[i][9] * t)*cos(ang);
	}
	*dpsi *= 1E-4*AS2R; /* 0.1 mas -> rad */
	*deps *= 1E-4*AS2R;
}
/* bdt to gpstime --------------------------------------------------------------
* convert bdt (beidou navigation satellite system time) to gpstime
* args   : gtime_t t        I   time expressed in bdt
* return : time expressed in gpstime
* notes  : see gpst2bdt()
*-----------------------------------------------------------------------------*/
extern gtime_t bdt2gpst(gtime_t t)
{
	return timeadd(t, 14.0);
}
/* time to day and sec -------------------------------------------------------*/
static double time2sec(gtime_t time, gtime_t *day)
{
	double ep[6], sec;
	time2epoch(time, ep);
	sec = ep[3] * 3600.0 + ep[4] * 60.0 + ep[5];
	ep[3] = ep[4] = ep[5] = 0.0;
	*day = epoch2time(ep);
	return sec;
}
/* utc to gmst -----------------------------------------------------------------
* convert utc to gmst (Greenwich mean sidereal time)
* args   : gtime_t t        I   time expressed in utc
*          double ut1_utc   I   UT1-UTC (s)
* return : gmst (rad)
*-----------------------------------------------------------------------------*/
extern double utc2gmst(gtime_t t, double ut1_utc)
{
	const double ep2000[] = { 2000, 1, 1, 12, 0, 0 };
	gtime_t tut, tut0;
	double ut, t1, t2, t3, gmst0, gmst;

	tut = timeadd(t, ut1_utc);
	ut = time2sec(tut, &tut0);
	t1 = timediff(tut0, epoch2time(ep2000)) / 86400.0 / 36525.0;
	t2 = t1*t1; t3 = t2*t1;
	gmst0 = 24110.54841 + 8640184.812866*t1 + 0.093104*t2 - 6.2E-6*t3;
	gmst = gmst0 + 1.002737909350795*ut;

	return fmod(gmst, 86400.0)*PI / 43200.0; /* 0 <= gmst <= 2*PI */
}
/* utc to gpstime --------------------------------------------------------------
* convert utc to gpstime considering leap seconds
* args   : gtime_t t        I   time expressed in utc
* return : time expressed in gpstime
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t utc2gpst(gtime_t t)
{
	int i;

	for (i = 0; i < (int)sizeof(leaps) / (int)sizeof(*leaps); i++) {
		if (timediff(t, epoch2time(leaps[i])) >= 0.0) return timeadd(t, -leaps[i][6]);
	}
	return t;
}
/* eci to ecef transformation matrix -------------------------------------------
* compute eci to ecef transformation matrix
* args   : gtime_t tutc     I   time in utc
*          double *erpv     I   erp values {xp,yp,ut1_utc,lod} (rad,rad,s,s/d)
*          double *U        O   eci to ecef transformation matrix (3 x 3)
*          double *gmst     IO  greenwich mean sidereal time (rad)
*                               (NULL: no output)
* return : none
* note   : see ref [3] chap 5
*          not thread-safe
*-----------------------------------------------------------------------------*/
extern void eci2ecef(gtime_t tutc, const double *erpv, double *U, double *gmst)
{
	const double ep2000[] = { 2000, 1, 1, 12, 0, 0 };
	static gtime_t tutc_;
	static double U_[9], gmst_;
	gtime_t tgps;
	double eps, ze, th, z, t, t2, t3, dpsi, deps, gast, f[5];
	double R1[9], R2[9], R3[9], R[9], W[9], N[9], P[9], NP[9];
	int i;

	if (fabs(timediff(tutc, tutc_)) < 0.01) { /* read cache */
		for (i = 0; i < 9; i++) U[i] = U_[i];
		if (gmst) *gmst = gmst_;
		return;
	}
	tutc_ = tutc;

	/* terrestrial time */
	tgps = utc2gpst(tutc_);
	t = (timediff(tgps, epoch2time(ep2000)) + 19.0 + 32.184) / 86400.0 / 36525.0;
	t2 = t*t; t3 = t2*t;

	/* astronomical arguments */
	ast_args(t, f);

	/* iau 1976 precession */
	ze = (2306.2181*t + 0.30188*t2 + 0.017998*t3)*AS2R;
	th = (2004.3109*t - 0.42665*t2 - 0.041833*t3)*AS2R;
	z = (2306.2181*t + 1.09468*t2 + 0.018203*t3)*AS2R;
	eps = (84381.448 - 46.8150*t - 0.00059*t2 + 0.001813*t3)*AS2R;
	Rz(-z, R1); Ry(th, R2); Rz(-ze, R3);
	matmul("NN", 3, 3, 3, 1.0, R1, R2, 0.0, R);
	matmul("NN", 3, 3, 3, 1.0, R, R3, 0.0, P); /* P=Rz(-z)*Ry(th)*Rz(-ze) */

	/* iau 1980 nutation */
	nut_iau1980(t, f, &dpsi, &deps);
	Rx(-eps - deps, R1); Rz(-dpsi, R2); Rx(eps, R3);
	matmul("NN", 3, 3, 3, 1.0, R1, R2, 0.0, R);
	matmul("NN", 3, 3, 3, 1.0, R, R3, 0.0, N); /* N=Rx(-eps)*Rz(-dspi)*Rx(eps) */

	/* greenwich aparent sidereal time (rad) */
	gmst_ = utc2gmst(tutc_, erpv[2]);
	gast = gmst_ + dpsi*cos(eps);
	gast += (0.00264*sin(f[4]) + 0.000063*sin(2.0*f[4]))*AS2R;

	/* eci to ecef transformation matrix */
	Ry(-erpv[0], R1); Rx(-erpv[1], R2); Rz(gast, R3);
	matmul("NN", 3, 3, 3, 1.0, R1, R2, 0.0, W);
	matmul("NN", 3, 3, 3, 1.0, W, R3, 0.0, R); /* W=Ry(-xp)*Rx(-yp) */
	matmul("NN", 3, 3, 3, 1.0, N, P, 0.0, NP);
	matmul("NN", 3, 3, 3, 1.0, R, NP, 0.0, U_); /* U=W*Rz(gast)*N*P */

	for (i = 0; i < 9; i++) U[i] = U_[i];
	if (gmst) *gmst = gmst_;
}
/* LU decomposition ----------------------------------------------------------*/
static int ludcmp(double *A, int n, int *indx, double *d)
{
	double big, s, tmp, vv[50];
	int i, imax = 0, j, k;

	*d = 1.0;
	for (i = 0; i<n; i++) {
		big = 0.0; for (j = 0; j<n; j++)
		if ((tmp = fabs(A[i + j*n]))>big)
			big = tmp;
		if (big>0.0) vv[i] = 1.0 / big;
		else { /*free(vv); */return -1; }
	}
	for (j = 0; j < n; j++) {
		for (i = 0; i < j; i++) {
			s = A[i + j*n]; for (k = 0; k < i; k++) s -= A[i + k*n] * A[k + j*n]; A[i + j*n] = s;
		}
		big = 0.0;
		for (i = j; i < n; i++) {
			s = A[i + j*n]; for (k = 0; k < j; k++) s -= A[i + k*n] * A[k + j*n]; A[i + j*n] = s;
			if ((tmp = vv[i] * fabs(s)) >= big) { big = tmp; imax = i; }
		}
		if (j != imax) {
			for (k = 0; k < n; k++) {
				tmp = A[imax + k*n]; A[imax + k*n] = A[j + k*n]; A[j + k*n] = tmp;
			}
			*d = -(*d); vv[imax] = vv[j];
		}
		indx[j] = imax;
		if (A[j + j*n] == 0.0) { /*free(vv);*/ return -1; }
		if (j != n - 1) {
			tmp = 1.0 / A[j + j*n]; for (i = j + 1; i < n; i++) A[i + j*n] *= tmp;
		}
	}
	//free(vv);
	return 0;
}
/* LU back-substitution ------------------------------------------------------*/
static void lubksb(const double *A, int n, const int *indx, double *b)
{
	double s;
	int i, ii = -1, ip, j;

	for (i = 0; i < n; i++) {
		ip = indx[i]; s = b[ip]; b[ip] = b[i];
		if (ii >= 0) for (j = ii; j < i; j++) s -= A[i + j*n] * b[j]; else if (s) ii = i;
		b[i] = s;
	}
	for (i = n - 1; i >= 0; i--) {
		s = b[i]; for (j = i + 1; j < n; j++) s -= A[i + j*n] * b[j]; b[i] = s / A[i + i*n];
	}
}
/* inverse of matrix ---------------------------------------------------------*/
extern int matinv(double *A, int n)
{
	double d, B[MAXOBS_*MAXOBS_];
	int i, j, indx[50];

	//indx = imat(n, 1);
	//B = mat(n, n);
	matcpy(B, A, n, n);
	if (ludcmp(B, n, indx, &d))
	{
		/*free(indx); free(B);*/ return -1;
	}
	for (j = 0; j < n; j++) {
		for (i = 0; i < n; i++) A[i + j*n] = 0.0; A[j + j*n] = 1.0;
		lubksb(B, n, indx, A + j*n);
	}
	//free(indx); free(B);
	return 0;
}

/* solve linear equation -----------------------------------------------------*/
extern int solve(const char *tr, const double *A, const double *Y, int n,
	int m, double *X)
{
	double B[20*20];
	//double *B = mat(n, n);
	int info;

	matcpy(B, A, n, n);
	if (!(info = matinv(B, n))) matmul(tr[0] == 'N' ? "NN" : "TN", n, m, n, 1.0, B, Y, 0.0, X);
	//free(B);
	return info;
}
/* least square estimation -----------------------------------------------------
* least square estimation by solving normal equation (x=(A*A')^-1*A*y)
* args   : double *A        I   transpose of (weighted) design matrix (n x m)
*          double *y        I   (weighted) measurements (m x 1)
*          int    n,m       I   number of parameters and measurements (n<=m)
*          double *x        O   estmated parameters (n x 1)
*          double *Q        O   esimated parameters covariance matrix (n x n)
* return : status (0:ok,0>:error)
* notes  : for weighted least square, replace A and y by A*w and w*y (w=W^(1/2))
*          matirix stored by column-major order (fortran convention)
*-----------------------------------------------------------------------------*/
extern int lsq(const double *A, const double *y, int n, int m, double *x,
	double *Q)
{
	double Ay[10];
	int info;

	if (m < n) return -1;
	//Ay = mat(n, 1);
	matmul("NN", n, 1, m, 1.0, A, y, 0.0, Ay); /* Ay=A*y */
	//for(int k=0;k<36;k++) 
	//Ay[k];//printf("%d %f\n",k,Ay[k]);
	matmul("NT", n, n, m, 1.0, A, A, 0.0, Q);  /* Q=A*A' */
	if (!(info = matinv(Q, n)))
		matmul("NN", n, 1, n, 1.0, Q, Ay, 0.0, x); /* x=Q^-1*Ay */
	//free(Ay);
	return info;
}
/* satellite carrier wave length -----------------------------------------------
* get satellite carrier wave lengths
* args   : int    sat       I   satellite number
*          int    frq       I   frequency index (0:L1,1:L2,2:L5/3,...)
*          nav_t  *nav      I   navigation messages
* return : carrier wave length (m) (0.0: error)
*-----------------------------------------------------------------------------*/
extern double satwavelen(int sat, int frq, const nav_t *nav)
{
	const double freq_glo[] = { FREQ1_GLO, FREQ2_GLO, FREQ3_GLO };
	const double dfrq_glo[] = { DFRQ1_GLO, DFRQ2_GLO, 0.0 };
	int i, sys = satsys(sat, NULL);

	if (sys == SYS_GLO) {
		if (0 <= frq&&frq <= 2) {
			for (i = 0; i < nav->ng; i++) {
				if (nav->geph[i].sat != sat) continue;
				return CLIGHT / (freq_glo[frq] + dfrq_glo[frq] * nav->geph[i].frq);
			}
		}
	}
	else if (sys == SYS_CMP) {
		if (frq == 0) return CLIGHT / FREQ1_CMP; /* B1 */
		else if (frq == 1) return CLIGHT / FREQ2_CMP; /* B3 */
		else if (frq == 2) return CLIGHT / FREQ3_CMP; /* B2 */
	}
	else {
		if (frq == 0) return CLIGHT / FREQ1; /* L1/E1 */
		else if (frq == 1) return CLIGHT / FREQ2; /* L2 */
		else if (frq == 2) return CLIGHT / FREQ5; /* L5/E5a */
		else if (frq == 3) return CLIGHT / FREQ6; /* L6/LEX */
		else if (frq == 4) return CLIGHT / FREQ7; /* E5b */
		else if (frq == 5) return CLIGHT / FREQ8; /* E5a+b */
	}
	return 0.0;
}
