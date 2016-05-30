/*------------------------------------------------------------------------------
* rinex.c : rinex functions
*
*          Copyright (C) 2007-2013 by T.TAKASU, All rights reserved.
*
* reference :
*     [1] W.Gurtner and L.Estey, RINEX The Receiver Independent Exchange Format
*         Version 2.11, December 10, 2007
*     [2] W.Gurtner and L.Estey, RINEX The Receiver Independent Exchange Format
*         Version 3.00, November 28, 2007
*     [3] IS-GPS-200D, Navstar GPS Space Segment/Navigation User Interfaces,
*         7 March, 2006
*     [4] W.Gurtner and L.Estey, RINEX The Receiver Independent Exchange Format
*         Version 2.12, June 23, 2009
*     [5] W.Gurtner and L.Estey, RINEX The Receiver Independent Exchange Format
*         Version 3.01, June 22, 2009
*     [6] J.Ray and W.Gurtner, RINEX extentions to handle clock information
*         version 3.02, September 2, 2010
*     [7] RINEX The Receiver Independent Exchange Format Version 3.02,
*         International GNSS Service (IGS), RINEX Working Group and Radio
*         Technical Commission for Maritime Services Special Committee 104
*         (RTCM-SC104), December 10, 2012
*
* version : $Revision:$
* history : 2006/01/16 1.0  new
*           2007/03/14 1.1  read P1 if no obstype of C1
*           2007/04/27 1.2  add readrnxt() function
*           2007/05/25 1.3  add support of file path with wild-card (*)
*                           add support of compressed files
*           2007/11/02 1.4  support sbas/geo satellite
*                           support doppler observables
*                           support rinex bug of week handover
*                           add rinex obs/nav output functions
*           2008/06/16 1.5  export readrnxf(), add compress()
*                           separate sortobs(), uniqeph(), screent()
*           2008/10/28 1.6  fix bug on reading rinex obs header types of observ
*           2009/04/09 1.7  support rinex 2.11
*                           change api of outrnxobsh(),outrnxobsb(),outrnxnavb()
*           2009/06/02 1.8  add api outrnxgnavb()
*           2009/08/15 1.9  support glonass
*                           add slip save/restore functions
*           2010/03/03 1.10 fix bug of array access by disabled satellite
*           2010/07/21 1.11 support rinex ver.2.12, 3.00
*                           support rinex extension for qzss
*                           support geo navigation messages
*                           added api:
*                               setrnxcodepri(),outrnxhnavh(),outrnxhnavb(),
*                           changed api:
*                               readrnx(),readrnxt(),outrnxnavh(),outrnxgnavh()
*           2010/05/29 1.12 fix bug on skipping invalid satellite data
*                           fix bug on frequency number overflow
*                           output P1 instead of C1 if rnxopt.rcvopt=-L1P
*                           output C2 instead of P2 if rnxopt.rcvopt=-L2C
*                           change api:
*                               outrnxgnavh(),outrnxhnavh(),readrnx(),
*                               readrnxt()
*                           add api:
*                               outrnxlnavh(), outrnxqnav()
*                           move uniqeph(),uniqgeph,uniqseph()
*           2010/08/19 1.13 suppress warning
*           2012/03/01 1.14 add function to read cnes widelane fcb in rnxclk
*                           support compass rinex nav
*                           change api: setcodepri()
*           2012/10/17 1.15 support ver.2.12, ver.3.01
*                           add api init_rnxctr(),free_rnxctr(),open_rnxctr(),
*                           input_rnxctr()
*                           change api readrnxt(),readrnx()
*                           delete api setrnxcodepri()
*                           fix bug on message frama time in v.3 glonass nav
*           2013/02/09 1.16 add reading geph.iode derived from toe
*           2013/02/23 1.17 support rinex 3.02 (ref [7])
*                           change api outrnxobsh()
*                           add api outrnxcnavh()
*                           fix bug on output of fit interval
*-----------------------------------------------------------------------------*/
#include "rtklib.h"
#include "gps_common.h"

static const char rcsid[]="$Id:$";

/* constants/macros ----------------------------------------------------------*/

#define SQR(x)      ((x)*(x))

#define NUMSYS      6                   /* number of systems */
#define MAXRNXLEN   (16*MAXOBSTYPE+4)   /* max rinex record length */
#define MAXPOSHEAD  1024                /* max head line position */
#define MINFREQ_GLO -7                  /* min frequency number glonass */
#define MAXFREQ_GLO 13                  /* max frequency number glonass */
#define NINCOBS     262144              /* inclimental number of obs data */

static const int navsys[]={             /* satellite systems */
    SYS_GPS,SYS_GLO,SYS_GAL,SYS_QZS,SYS_SBS,SYS_CMP,0
};
static const char syscodes[]="GREJSC";  /* satellite system codes */

static const char obscodes[]="CLDS";    /* obs type codes */

static const char frqcodes[]="125678";  /* frequency codes */

static const double ura_eph[]={         /* ura values (ref [3] 20.3.3.3.1.1) */
    2.4,3.4,4.85,6.85,9.65,13.65,24.0,48.0,96.0,192.0,384.0,768.0,1536.0,
    3072.0,6144.0,0.0
};
/* type definition -----------------------------------------------------------*/
typedef struct {                        /* signal index type */
    int n;                              /* number of index */
    int frq[MAXOBSTYPE];                /* signal frequency (1:L1,2:L2,...) */
    int pos[MAXOBSTYPE];                /* signal index in obs data (-1:no) */
    unsigned char pri [MAXOBSTYPE];     /* signal priority (15-0) */
    unsigned char type[MAXOBSTYPE];     /* type (0:C,1:L,2:D,3:S) */
    unsigned char code[MAXOBSTYPE];     /* obs code (CODE_L??) */
    double shift[MAXOBSTYPE];           /* phase shift (cycle) */
} sigind_t;

/* set string without tail space ---------------------------------------------*/
static void setstr(char *dst, const char *src, int n)
{
    char *p=dst;
    const char *q=src;
    while (*q&&q<src+n) *p++=*q++;
    *p--='\0';
    while (p>=dst&&*p==' ') *p--='\0';
}
/* adjust time considering week handover -------------------------------------*/
static gtime_t adjweek(gtime_t t, gtime_t t0)
{
    double tt=timediff(t,t0);
    if (tt<-302400.0) return timeadd(t, 604800.0);
    if (tt> 302400.0) return timeadd(t,-604800.0);
    return t;
}
/* adjust time considering week handover -------------------------------------*/
static gtime_t adjday(gtime_t t, gtime_t t0)
{
    double tt=timediff(t,t0);
    if (tt<-43200.0) return timeadd(t, 86400.0);
    if (tt> 43200.0) return timeadd(t,-86400.0);
    return t;
}
/* time string for ver.3 (yyyymmdd hhmmss UTC) -------------------------------*/
static void timestr_rnx(char *str)
{
    gtime_t time;
    double ep[6];
    time=timeget();
    time.sec=0.0;
    time2epoch(time,ep);
    sprintf(str,"%04.0f%02.0f%02.0f %02.0f%02.0f%02.0f UTC",ep[0],ep[1],ep[2],
            ep[3],ep[4],ep[5]);
}
/* satellite to satellite code -----------------------------------------------*/
static int sat2code(int sat, char *code)
{
    int prn;
    switch (satsys(sat,&prn)) {
        case SYS_GPS: sprintf(code,"G%2d",prn-MINPRNGPS+1); break;
        case SYS_GLO: sprintf(code,"R%2d",prn-MINPRNGLO+1); break;
        case SYS_GAL: sprintf(code,"E%2d",prn-MINPRNGAL+1); break;
        case SYS_SBS: sprintf(code,"S%2d",prn-100); break;
        case SYS_QZS: sprintf(code,"J%2d",prn-MINPRNQZS+1); break;
        case SYS_CMP: sprintf(code,"C%2d",prn-MINPRNCMP+1); break;
        default: return 0;
    }
    return 1;
}
/* ura index to ura value (m) ------------------------------------------------*/
static double uravalue(int sva)
{
    return 0<=sva&&sva<15?ura_eph[sva]:32767.0;
}
/* ura value (m) to ura index ------------------------------------------------*/
static int uraindex(double value)
{
    int i;
    for (i=0;i<15;i++) if (ura_eph[i]>=value) break;
    return i;
}
/* initialize station parameter ----------------------------------------------*/
static void init_sta(sta_t *sta)
{
    int i;
    *sta->name   ='\0';
    *sta->marker ='\0';
    *sta->antdes ='\0';
    *sta->antsno ='\0';
    *sta->rectype='\0';
    *sta->recver ='\0';
    *sta->recsno ='\0';
    sta->antsetup=sta->itrf=sta->deltype=0;
    for (i=0;i<3;i++) sta->pos[i]=0.0;
    for (i=0;i<3;i++) sta->del[i]=0.0;
    sta->hgt=0.0;
}
/*------------------------------------------------------------------------------
* input rinex functions
*-----------------------------------------------------------------------------*/

/* convert rinex obs type ver.2 -> ver.3 -------------------------------------*/
static void convcode(double ver, int sys, const char *str, char *type)
{
    strcpy(type,"   ");
    
    if      (!strcmp(str,"P1")) { /* ver.2.11 GPS L1PY,GLO L2P */
        if      (sys==SYS_GPS) sprintf(type,"%c1W",'C');
        else if (sys==SYS_GLO) sprintf(type,"%c1P",'C');
    }
    else if (!strcmp(str,"P2")) { /* ver.2.11 GPS L2PY,GLO L2P */
        if      (sys==SYS_GPS) sprintf(type,"%c2W",'C');
        else if (sys==SYS_GLO) sprintf(type,"%c2P",'C');
    }
    else if (!strcmp(str,"C1")) { /* ver.2.11 GPS L1C,GLO L1C/A */
        if      (sys==SYS_GPS) sprintf(type,"%c1C",'C');
        else if (sys==SYS_GLO) sprintf(type,"%c1C",'C');
        else if (sys==SYS_QZS) sprintf(type,"%c1C",'C');
        else if (sys==SYS_SBS) sprintf(type,"%c1C",'C');
    }
    else if (!strcmp(str,"C2")) { /* ver.2.11 GPS L2C,GLO L2C/A */
        if      (sys==SYS_GPS) sprintf(type,"%c2X",'C');
        else if (sys==SYS_GLO) sprintf(type,"%c2C",'C');
        else if (sys==SYS_QZS) sprintf(type,"%c2X",'C');
    }
    else if (ver>=2.12&&str[1]=='A') { /* ver.2.12 L1C/A */
        if      (sys==SYS_GPS) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_GLO) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_SBS) sprintf(type,"%c1C",str[0]);
    }
    else if (ver>=2.12&&str[1]=='B') { /* ver.2.12 GPS L1C */
        if      (sys==SYS_GPS) sprintf(type,"%c1X",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c1X",str[0]);
    }
    else if (ver>=2.12&&str[1]=='C') { /* ver.2.12 GPS L2C */
        if      (sys==SYS_GPS) sprintf(type,"%c2X",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c2X",str[0]);
    }
    else if (ver>=2.12&&str[1]=='D') { /* ver.2.12 GLO L2C/A */
        if      (sys==SYS_GLO) sprintf(type,"%c2C",str[0]);
    }
    else if (ver>=2.12&&str[1]=='1') { /* ver.2.12 GPS L1PY,GLO L1P */
        if      (sys==SYS_GPS) sprintf(type,"%c1W",str[0]);
        else if (sys==SYS_GLO) sprintf(type,"%c1P",str[0]);
        else if (sys==SYS_GAL) sprintf(type,"%c1X",str[0]); /* tentative */
    }
    else if (ver<2.12&&str[1]=='1') {
        if      (sys==SYS_GPS) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_GLO) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_GAL) sprintf(type,"%c1X",str[0]); /* tentative */
        else if (sys==SYS_QZS) sprintf(type,"%c1C",str[0]);
        else if (sys==SYS_SBS) sprintf(type,"%c1C",str[0]);
    }
    else if (str[1]=='2') {
        if      (sys==SYS_GPS) sprintf(type,"%c2W",str[0]);
        else if (sys==SYS_GLO) sprintf(type,"%c2P",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c2X",str[0]);
        else if (sys==SYS_CMP) sprintf(type,"%c2C",str[0]); /* tentative */
    }
    else if (str[1]=='5') {
        if      (sys==SYS_GPS) sprintf(type,"%c5X",str[0]);
        else if (sys==SYS_GAL) sprintf(type,"%c5X",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c5X",str[0]);
        else if (sys==SYS_SBS) sprintf(type,"%c5X",str[0]);
        else if (sys==SYS_CMP) sprintf(type,"%c5X",str[0]); /* tentative */
    }
    else if (str[1]=='6') {
        if      (sys==SYS_GAL) sprintf(type,"%c6X",str[0]);
        else if (sys==SYS_QZS) sprintf(type,"%c6X",str[0]);
        else if (sys==SYS_CMP) sprintf(type,"%c6X",str[0]); /* tentative */
    }
    else if (str[1]=='7') {
        if      (sys==SYS_GAL) sprintf(type,"%c7X",str[0]);
        else if (sys==SYS_CMP) sprintf(type,"%c7X",str[0]); /* tentative */
    }
    else if (str[1]=='8') {
        if      (sys==SYS_GAL) sprintf(type,"%c8X",str[0]);
    }
    trace(3,"convcode: ver=%.2f sys=%2d type= %s -> %s\n",ver,sys,str,type);
}
/* decode obs header ---------------------------------------------------------*/
static void decode_obsh(FILE *fp, char *buff, double ver, int *tsys,
                        char tobs[][MAXOBSTYPE][4], nav_t *nav, sta_t *sta)
{
    /* default codes for unknown code */
    const char *defcodes[]={
        "CWX   ",   /* GPS: L125___ */
        "CC    ",   /* GLO: L12____ */
        "X XXXX",   /* GAL: L1_5678 */
        "CXXX  ",   /* QZS: L1256__ */
        "C X   ",   /* SBS: L1_5___ */
        " C XX "    /* BDS: L_2_67_ */
    };
    double del[3];
    int i,j,k,n,nt,prn,fcn;
    char *label=buff+60,*p,str[4];
    
    trace(3,"decode_obsh: ver=%.2f\n",ver);
    
    if      (strstr(label,"MARKER NAME"         )) {
        if (sta) setstr(sta->name,buff,60);
    }
    else if (strstr(label,"MARKER NUMBER"       )) { /* opt */
        if (sta) setstr(sta->marker,buff,20);
    }
    else if (strstr(label,"MARKER TYPE"         )) ; /* ver.3 */
    else if (strstr(label,"OBSERVER / AGENCY"   )) ;
    else if (strstr(label,"REC # / TYPE / VERS" )) {
        if (sta) {
            setstr(sta->recsno, buff,   20);
            setstr(sta->rectype,buff+20,20);
            setstr(sta->recver, buff+40,20);
        }
    }
    else if (strstr(label,"ANT # / TYPE"        )) {
        if (sta) {
            setstr(sta->antsno,buff   ,20);
            setstr(sta->antdes,buff+20,20);
        }
    }
    else if (strstr(label,"APPROX POSITION XYZ" )) {
        if (sta) {
            for (i=0,j=0;i<3;i++,j+=14) sta->pos[i]=str2num(buff,j,14);
        }
    }
    else if (strstr(label,"ANTENNA: DELTA H/E/N")) {
        if (sta) {
            for (i=0,j=0;i<3;i++,j+=14) del[i]=str2num(buff,j,14);
            sta->del[2]=del[0]; /* h */
            sta->del[0]=del[1]; /* e */
            sta->del[1]=del[2]; /* n */
        }
    }
    else if (strstr(label,"ANTENNA: DELTA X/Y/Z")); /* opt ver.3 */
    else if (strstr(label,"ANTENNA: PHASECENTER")); /* opt ver.3 */
    else if (strstr(label,"ANTENNA: B.SIGHT XYZ")); /* opt ver.3 */
    else if (strstr(label,"ANTENNA: ZERODIR AZI")); /* opt ver.3 */
    else if (strstr(label,"ANTENNA: ZERODIR XYZ")); /* opt ver.3 */
    else if (strstr(label,"CENTER OF MASS: XYZ" )); /* opt ver.3 */
    else if (strstr(label,"SYS / # / OBS TYPES" )) { /* ver.3 */
        if (!(p=strchr(syscodes,buff[0]))) {
            trace(2,"invalid system code: sys=%c\n",buff[0]);
            return;
        }
        i=(int)(p-syscodes);
        n=(int)str2num(buff,3,3);
        for (j=nt=0,k=7;j<n;j++,k+=4) {
            if (k>58) {
                if (!fgets(buff,MAXRNXLEN,fp)) break;
                k=7;
            }
            if (nt<MAXOBSTYPE-1) setstr(tobs[i][nt++],buff+k,3);
        }
        *tobs[i][nt]='\0';
        
        /* if unknown code in ver.3, set default code */
        for (j=0;j<nt;j++) {
            if (tobs[i][j][2]) continue;
            if (!(p=strchr(frqcodes,tobs[i][j][1]))) continue;
            tobs[i][j][2]=defcodes[i][(int)(p-frqcodes)];
            trace(2,"set default for unknown code: sys=%c code=%s\n",buff[0],
                  tobs[i][j]);
        }
    }
    else if (strstr(label,"WAVELENGTH FACT L1/2")) ; /* opt ver.2 */
    else if (strstr(label,"# / TYPES OF OBSERV" )) { /* ver.2 */
        n=(int)str2num(buff,0,6);
        for (i=nt=0,j=10;i<n;i++,j+=6) {
            if (j>58) {
                if (!fgets(buff,MAXRNXLEN,fp)) break;
                j=10;
            }
            if (nt>=MAXOBSTYPE-1) continue;
            if (ver<=2.99) {
                setstr(str,buff+j,2);
                convcode(ver,SYS_GPS,str,tobs[0][nt]);
                convcode(ver,SYS_GLO,str,tobs[1][nt]);
                convcode(ver,SYS_GAL,str,tobs[2][nt]);
                convcode(ver,SYS_QZS,str,tobs[3][nt]);
                convcode(ver,SYS_SBS,str,tobs[4][nt]);
                convcode(ver,SYS_CMP,str,tobs[5][nt]);
            }
            nt++;
        }
        *tobs[0][nt]='\0';
    }
    else if (strstr(label,"SIGNAL STRENGTH UNIT")) ; /* opt ver.3 */
    else if (strstr(label,"INTERVAL"            )) ; /* opt */
    else if (strstr(label,"TIME OF FIRST OBS"   )) {
        if      (!strncmp(buff+48,"GPS",3)) *tsys=TSYS_GPS;
        else if (!strncmp(buff+48,"GLO",3)) *tsys=TSYS_UTC;
        else if (!strncmp(buff+48,"GAL",3)) *tsys=TSYS_GAL;
        else if (!strncmp(buff+48,"QZS",3)) *tsys=TSYS_QZS; /* ver.3.02 */
        else if (!strncmp(buff+48,"BDT",3)) *tsys=TSYS_CMP; /* ver.3.02 */
    }
    else if (strstr(label,"TIME OF LAST OBS"    )) ; /* opt */
    else if (strstr(label,"RCV CLOCK OFFS APPL" )) ; /* opt */
    else if (strstr(label,"SYS / DCBS APPLIED"  )) ; /* opt ver.3 */
    else if (strstr(label,"SYS / PCVS APPLIED"  )) ; /* opt ver.3 */
    else if (strstr(label,"SYS / SCALE FACTOR"  )) ; /* opt ver.3 */
    else if (strstr(label,"SYS / PHASE SHIFTS"  )) ; /* ver.3.01 */
    else if (strstr(label,"GLONASS SLOT / FRQ #")) { /* ver.3.02 */
        if (nav) {
            for (i=0,p=buff+4;i<8;i++,p+=8) {
                if (sscanf(p,"R%2d %2d",&prn,&fcn)<2) continue;
                if (1<=prn&&prn<=MAXPRNGLO) nav->glo_fcn[prn-1]=fcn+8;
            }
        }
    }
    else if (strstr(label,"GLONASS COD/PHS/BIS" )) { /* ver.3.02 */
        if (nav) {
            for (i=0,p=buff;i<4;i++,p+=13) {
                if      (strncmp(p+1,"C1C",3)) nav->glo_cpbias[0]=str2num(p,5,8);
                else if (strncmp(p+1,"C1P",3)) nav->glo_cpbias[1]=str2num(p,5,8);
                else if (strncmp(p+1,"C2C",3)) nav->glo_cpbias[2]=str2num(p,5,8);
                else if (strncmp(p+1,"C2P",3)) nav->glo_cpbias[3]=str2num(p,5,8);
            }
        }
    }
    else if (strstr(label,"LEAP SECONDS"        )) { /* opt */
        if (nav) nav->leaps=(int)str2num(buff,0,6);
    }
    else if (strstr(label,"# OF SALTELLITES"    )) ; /* opt */
    else if (strstr(label,"PRN / # OF OBS"      )) ; /* opt */
}
/* decode nav header ---------------------------------------------------------*/
static void decode_navh(char *buff, nav_t *nav)
{
    int i,j;
    char *label=buff+60;
    
    trace(3,"decode_navh:\n");
    
    if      (strstr(label,"ION ALPHA"           )) { /* opt ver.2 */
        if (nav) {
            for (i=0,j=2;i<4;i++,j+=12) nav->ion_gps[i]=str2num(buff,j,12);
        }
    }
    else if (strstr(label,"ION BETA"            )) { /* opt ver.2 */
        if (nav) {
            for (i=0,j=2;i<4;i++,j+=12) nav->ion_gps[i+4]=str2num(buff,j,12);
        }
    }
    else if (strstr(label,"DELTA-UTC: A0,A1,T,W")) { /* opt ver.2 */
        if (nav) {
            for (i=0,j=3;i<2;i++,j+=19) nav->utc_gps[i]=str2num(buff,j,19);
            for (;i<4;i++,j+=9) nav->utc_gps[i]=str2num(buff,j,9);
        }
    }
    else if (strstr(label,"IONOSPHERIC CORR"    )) { /* opt ver.3 */
        if (nav) {
            if (!strncmp(buff,"GPSA",4)) {
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_gps[i]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"GPSB",4)) {
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_gps[i+4]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"GAL",3)) {
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_gal[i]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"QZSA",4)) { /* v.3.02 */
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_qzs[i]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"QZSB",4)) { /* v.3.02 */
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_qzs[i+4]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"BDSA",4)) { /* v.3.02 */
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_cmp[i]=str2num(buff,j,12);
            }
            else if (!strncmp(buff,"BDSB",4)) { /* v.3.02 */
                for (i=0,j=5;i<4;i++,j+=12) nav->ion_cmp[i+4]=str2num(buff,j,12);
            }
        }
    }
    else if (strstr(label,"TIME SYSTEM CORR"    )) { /* opt ver.3 */
        if (nav) {
            if (!strncmp(buff,"GPUT",4)) {
                nav->utc_gps[0]=str2num(buff, 5,17);
                nav->utc_gps[1]=str2num(buff,22,16);
                nav->utc_gps[2]=str2num(buff,38, 7);
                nav->utc_gps[3]=str2num(buff,45, 5);
            }
            else if (!strncmp(buff,"GLUT",4)) {
                nav->utc_glo[0]=str2num(buff, 5,17);
                nav->utc_glo[1]=str2num(buff,22,16);
            }
            else if (!strncmp(buff,"GAUT",4)) { /* v.3.02 */
                nav->utc_gal[0]=str2num(buff, 5,17);
                nav->utc_gal[1]=str2num(buff,22,16);
                nav->utc_gal[2]=str2num(buff,38, 7);
                nav->utc_gal[3]=str2num(buff,45, 5);
            }
            else if (!strncmp(buff,"QZUT",4)) { /* v.3.02 */
                nav->utc_qzs[0]=str2num(buff, 5,17);
                nav->utc_qzs[1]=str2num(buff,22,16);
                nav->utc_qzs[2]=str2num(buff,38, 7);
                nav->utc_qzs[3]=str2num(buff,45, 5);
            }
            else if (!strncmp(buff,"BDUT",4)) { /* v.3.02 */
                nav->utc_cmp[0]=str2num(buff, 5,17);
                nav->utc_cmp[1]=str2num(buff,22,16);
                nav->utc_cmp[2]=str2num(buff,38, 7);
                nav->utc_cmp[3]=str2num(buff,45, 5);
            }
            else if (!strncmp(buff,"SBUT",4)) { /* v.3.02 */
                nav->utc_cmp[0]=str2num(buff, 5,17);
                nav->utc_cmp[1]=str2num(buff,22,16);
                nav->utc_cmp[2]=str2num(buff,38, 7);
                nav->utc_cmp[3]=str2num(buff,45, 5);
            }
        }
    }
    else if (strstr(label,"LEAP SECONDS"        )) { /* opt */
        if (nav) nav->leaps=(int)str2num(buff,0,6);
    }
}
/* decode gnav header --------------------------------------------------------*/
static void decode_gnavh(char *buff, nav_t *nav)
{
    char *label=buff+60;
    
    trace(3,"decode_gnavh:\n");
    
    if      (strstr(label,"CORR TO SYTEM TIME"  )) ; /* opt */
    else if (strstr(label,"LEAP SECONDS"        )) { /* opt */
        if (nav) nav->leaps=(int)str2num(buff,0,6);
    }
}
/* decode geo nav header -----------------------------------------------------*/
static void decode_hnavh(char *buff, nav_t *nav)
{
    char *label=buff+60;
    
    trace(3,"decode_hnavh:\n");
    
    if      (strstr(label,"CORR TO SYTEM TIME"  )) ; /* opt */
    else if (strstr(label,"D-UTC A0,A1,T,W,S,U" )) ; /* opt */
    else if (strstr(label,"LEAP SECONDS"        )) { /* opt */
        if (nav) nav->leaps=(int)str2num(buff,0,6);
    }
}
/* read rinex header ---------------------------------------------------------*/

/* decode obs epoch ----------------------------------------------------------*/
static int decode_obsepoch(FILE *fp, char *buff, double ver, gtime_t *time,
                           int *flag, int *sats)
{
    int i,j,n;
    char satid[8]="";
    
    trace(4,"decode_obsepoch: ver=%.2f\n",ver);
    
    if (ver<=2.99) { /* ver.2 */
        if ((n=(int)str2num(buff,29,3))<=0) return 0;
        
        /* epoch flag: 3:new site,4:header info,5:external event */
        *flag=(int)str2num(buff,28,1);
        
        if (3<=*flag&&*flag<=5) return n;
        
        if (str2time(buff,0,26,time)) {
            trace(2,"rinex obs invalid epoch: epoch=%26.26s\n",buff);
            return 0;
        }
        for (i=0,j=32;i<n;i++,j+=3) {
            if (j>=68) {
                if (!fgets(buff,MAXRNXLEN,fp)) break;
                j=32;
            }
            if (i<MAXOBS) {
                strncpy(satid,buff+j,3);
                sats[i]=satid2no(satid);
            }
        }
    }
    else { /* ver.3 */
        if ((n=(int)str2num(buff,32,3))<=0) return 0;
        
        *flag=(int)str2num(buff,31,1);
        
        if (3<=*flag&&*flag<=5) return n;
        
        if (buff[0]!='>'||str2time(buff,1,28,time)) {
            trace(2,"rinex obs invalid epoch: epoch=%29.29s\n",buff);
            return 0;
        }
    }
    trace(4,"decode_obsepoch: time=%s flag=%d\n",time_str(*time,3),*flag);
    return n;
}
/* decode obs data -----------------------------------------------------------*/
static int decode_obsdata(FILE *fp, char *buff, double ver, sigind_t *index,
                          obsd_t *obs)
{
    sigind_t *ind;
    double val[MAXOBSTYPE]={0};
    unsigned char lli[MAXOBSTYPE]={0};
    char satid[8]="";
    int i,j,n,m,stat=1,p[MAXOBSTYPE],k[16],l[16];
    
    trace(4,"decode_obsdata: ver=%d\n",ver);
    
    if (ver>2.99) { /* ver.3 */
        strncpy(satid,buff,3);
        obs->sat=(unsigned char)satid2no(satid);
    }
    if (!obs->sat) {
        trace(4,"decode_obsdata: unsupported sat sat=%s\n",satid);
        stat=0;
    }
    /* read obs data fields */
    switch (satsys(obs->sat,NULL)) {
        case SYS_GLO: ind=index+1; break;
        case SYS_GAL: ind=index+2; break;
        case SYS_QZS: ind=index+3; break;
        case SYS_SBS: ind=index+4; break;
        case SYS_CMP: ind=index+5; break;
        default:      ind=index  ; break;
    }
    for (i=0,j=ver<=2.99?0:3;i<ind->n;i++,j+=16) {
        
        if (ver<=2.99&&j>=80) { /* ver.2 */
            if (!fgets(buff,MAXRNXLEN,fp)) break;
            j=0;
        }
        if (stat) {
            val[i]=str2num(buff,j,14)+ind->shift[i];
            lli[i]=(unsigned char)str2num(buff,j+14,1)&3;
        }
    }
    if (!stat) return 0;
    
    for (i=0;i<NFREQ+NEXOBS;i++) {
        obs->P[i]=obs->L[i]=0.0; obs->D[i]=0.0f;
        obs->SNR[i]=obs->LLI[i]=obs->code[i]=0;
    }
    /* assign position in obs data */
    for (i=n=m=0;i<ind->n;i++) {
        
        p[i]=ver<=2.11?ind->frq[i]-1:ind->pos[i];
        
        if (ind->type[i]==0&&p[i]==0) k[n++]=i; /* C1? index */
        if (ind->type[i]==0&&p[i]==1) l[m++]=i; /* C2? index */
    }
    if (ver<=2.11) {
        
        /* if multiple codes (C1/P1,C2/P2), select higher priority */
        if (n>=2) {
            if (val[k[0]]==0.0&&val[k[1]]==0.0) {
                p[k[0]]=-1; p[k[1]]=-1;
            }
            else if (val[k[0]]!=0.0&&val[k[1]]==0.0) {
                p[k[0]]=0; p[k[1]]=-1;
            }
            else if (val[k[0]]==0.0&&val[k[1]]!=0.0) {
                p[k[0]]=-1; p[k[1]]=0;
            }
            else if (ind->pri[k[1]]>ind->pri[k[0]]) {
                p[k[1]]=0; p[k[0]]=NEXOBS<1?-1:NFREQ;
            }
            else {
                p[k[0]]=0; p[k[1]]=NEXOBS<1?-1:NFREQ;
            }
        }
        if (m>=2) {
            if (val[l[0]]==0.0&&val[l[1]]==0.0) {
                p[l[0]]=-1; p[l[1]]=-1;
            }
            else if (val[l[0]]!=0.0&&val[l[1]]==0.0) {
                p[l[0]]=1; p[l[1]]=-1;
            }
            else if (val[l[0]]==0.0&&val[l[1]]!=0.0) {
                p[l[0]]=-1; p[l[1]]=1; 
            }
            else if (ind->pri[l[1]]>ind->pri[l[0]]) {
                p[l[1]]=1; p[l[0]]=NEXOBS<2?-1:NFREQ+1;
            }
            else {
                p[l[0]]=1; p[l[1]]=NEXOBS<2?-1:NFREQ+1;
            }
        }
    }
    /* save obs data */
    for (i=0;i<ind->n;i++) {
        if (p[i]<0||val[i]==0.0) continue;
        switch (ind->type[i]) {
            case 0: obs->P[p[i]]=val[i]; obs->code[p[i]]=ind->code[i]; break;
            case 1: obs->L[p[i]]=val[i]; obs->LLI [p[i]]=lli[i];       break;
            case 2: obs->D[p[i]]=(float)val[i];                        break;
            case 3: obs->SNR[p[i]]=(unsigned char)(val[i]*4.0+0.5);    break;
        }
    }
    trace(4,"decode_obsdata: time=%s sat=%2d\n",time_str(obs->time,0),obs->sat);
    return 1;
}
/* save slips ----------------------------------------------------------------*/
static void saveslips(unsigned char slips[][NFREQ], obsd_t *data)
{
    int i;
    for (i=0;i<NFREQ;i++) {
        if (data->LLI[i]&1) slips[data->sat-1][i]|=1;
    }
}
/* restore slips -------------------------------------------------------------*/
static void restslips(unsigned char slips[][NFREQ], obsd_t *data)
{
    int i;
    for (i=0;i<NFREQ;i++) {
        if (slips[data->sat-1][i]&1) data->LLI[i]|=1;
        slips[data->sat-1][i]=0;
    }
}
/* add obs data --------------------------------------------------------------*/
static int addobsdata(obs_t *obs, const obsd_t *data)
{
    obsd_t *obs_data;
    
    if (obs->nmax<=obs->n) {
        if (obs->nmax<=0) obs->nmax=NINCOBS; else obs->nmax*=2;
        if (!(obs_data=(obsd_t *)realloc(obs->data,sizeof(obsd_t)*obs->nmax))) {
            trace(1,"addobsdata: memalloc error n=%dx%d\n",sizeof(obsd_t),obs->nmax);
            free(obs->data); obs->data=NULL; obs->n=obs->nmax=0;
            return -1;
        }
        obs->data=obs_data;
    }
    obs->data[obs->n++]=*data;
    return 1;
}
/* set signal index ----------------------------------------------------------*/
static void set_index(double ver, int sys, const char *opt,
                      char tobs[MAXOBSTYPE][4], sigind_t *ind)
{
    const char *p,str[8],*optstr="";
    double shift;
    int i,j,k,n;
    
    for (i=n=0;*tobs[i];i++,n++) {
        ind->code[i]=obs2code(tobs[i]+1,ind->frq+i);
        ind->type[i]=(p=strchr(obscodes,tobs[i][0]))?(int)(p-obscodes):0;
        ind->pri[i]=getcodepri(sys,ind->code[i],opt);
        ind->pos[i]=-1;
    }
    /* parse phase shift options */
    switch (sys) {
        case SYS_GPS: optstr="-GL%2s=%lf"; break;
        case SYS_GLO: optstr="-RL%2s=%lf"; break;
        case SYS_GAL: optstr="-EL%2s=%lf"; break;
        case SYS_QZS: optstr="-JL%2s=%lf"; break;
        case SYS_SBS: optstr="-SL%2s=%lf"; break;
        case SYS_CMP: optstr="-CL%2s=%lf"; break;
    }
    for (p=opt;p&&(p=strchr(p,'-'));p++) {
        if (sscanf(p,optstr,str,&shift)<2) continue;
        for (i=0;i<n;i++) {
            if (strcmp(code2obs(ind->code[i],NULL),str)) continue;
            ind->shift[i]=shift;
            trace(2,"phase shift: sys=%2d tobs=%s shift=%.3f\n",sys,
                  tobs[i],shift);
        }
    }
    /* assign index for highest priority code */
    for (i=0;i<NFREQ;i++) {
        for (j=0,k=-1;j<n;j++) {
            if (ind->frq[j]==i+1&&ind->pri[j]&&(k<0||ind->pri[j]>ind->pri[k])) {
                k=j;
            }
        }
        if (k<0) continue;
        
        for (j=0;j<n;j++) {
            if (ind->code[j]==ind->code[k]) ind->pos[j]=i;
        }
    }
    /* assign index of extended obs data */
    for (i=0;i<NEXOBS;i++) {
        for (j=0;j<n;j++) {
            if (ind->code[j]&&ind->pri[j]&&ind->pos[j]<0) break;
        }
        if (j>=n) break;
        
        for (k=0;k<n;k++) {
            if (ind->code[k]==ind->code[j]) ind->pos[k]=NFREQ+i;
        }
    }
    for (i=0;i<n;i++) {
        if (!ind->code[i]||!ind->pri[i]||ind->pos[i]>=0) continue;
        trace(3,"reject obs type: sys=%2d, obs=%s\n",sys,tobs[i]);
    }
    ind->n=n;
    
#if 0 /* for debug */
    for (i=0;i<n;i++) {
        trace(2,"set_index: sys=%2d,tobs=%s code=%2d pri=%2d frq=%d pos=%d shift=%5.2f\n",
              sys,tobs[i],ind->code[i],ind->pri[i],ind->frq[i],ind->pos[i],
              ind->shift[i]);
    }
#endif
}
/* read rinex obs data body --------------------------------------------------*/

/* read rinex obs ------------------------------------------------------------*/

/* decode ephemeris ----------------------------------------------------------*/
static int decode_eph(double ver, int sat, gtime_t toc, const double *data,
                      eph_t *eph)
{
    eph_t eph0={0};
    int sys;
    
    trace(3,"decode_eph: ver=%.2f sat=%2d\n",ver,sat);
    
    sys=satsys(sat,NULL);
    
    if (!(sys&(SYS_GPS|SYS_GAL|SYS_QZS|SYS_CMP))) {
        trace(2,"ephemeris error: invalid satellite sat=%2d\n",sat);
        return 0;
    }
    *eph=eph0;
    
    eph->sat=sat;
    eph->toc=toc;
    
    eph->f0=data[0];
    eph->f1=data[1];
    eph->f2=data[2];
    
    eph->A=SQR(data[10]); eph->e=data[ 8]; eph->i0  =data[15]; eph->OMG0=data[13];
    eph->omg =data[17]; eph->M0 =data[ 6]; eph->deln=data[ 5]; eph->OMGd=data[18];
    eph->idot=data[19]; eph->crc=data[16]; eph->crs =data[ 4]; eph->cuc =data[ 7];
    eph->cus =data[ 9]; eph->cic=data[12]; eph->cis =data[14];
    
    if (sys==SYS_GPS||sys==SYS_QZS) {
        eph->iode=(int)data[ 3];      /* IODE */
        eph->iodc=(int)data[26];      /* IODC */
        eph->toes=     data[11];      /* toe (s) in gps week */
        eph->week=(int)data[21];      /* gps week */
        eph->toe=adjweek(gpst2time(eph->week,data[11]),toc);
        eph->ttr=adjweek(gpst2time(eph->week,data[27]),toc);
        
        eph->code=(int)data[20];      /* GPS: codes on L2 ch */
        eph->svh =(int)data[24];      /* sv health */
        eph->sva=uraindex(data[23]);  /* ura (m->index) */
        eph->flag=(int)data[22];      /* GPS: L2 P data flag */
        
        eph->tgd[0]=   data[25];      /* TGD */
        eph->fit   =   data[28];      /* fit interval */
    }
    else if (sys==SYS_GAL) { /* GAL ver.3 */
        eph->iode=(int)data[ 3];      /* IODnav */
        eph->toes=     data[11];      /* toe (s) in galileo week */
        eph->week=(int)data[21];      /* gal week = gps week */
        eph->toe=adjweek(gpst2time(eph->week,data[11]),toc);
        eph->ttr=adjweek(gpst2time(eph->week,data[27]),toc);
        
        eph->code=(int)data[20];      /* data sources */
                                      /* bit 0 set: I/NAV E1-B */
                                      /* bit 1 set: F/NAV E5a-I */
                                      /* bit 2 set: F/NAV E5b-I */
                                      /* bit 8 set: af0-af2 toc are for E5a.E1 */
                                      /* bit 9 set: af0-af2 toc are for E5b.E1 */
        eph->svh =(int)data[24];      /* sv health */
                                      /* bit     0: E1B DVS */
                                      /* bit   1-2: E1B HS */
                                      /* bit     3: E5a DVS */
                                      /* bit   4-5: E5a HS */
                                      /* bit     6: E5b DVS */
                                      /* bit   7-8: E5b HS */
        eph->sva =uraindex(data[23]); /* ura (m->index) */
        
        eph->tgd[0]=   data[25];      /* BGD E5a/E1 */
        eph->tgd[1]=   data[26];      /* BGD E5b/E1 */
    }
    else if (sys==SYS_CMP) { /* BeiDou v.3.02 */
        eph->toc=bdt2gpst(eph->toc);  /* bdt -> gpst */
        eph->iode=(int)data[ 3];      /* AODE */
        eph->iodc=(int)data[28];      /* AODC */
        eph->toes=     data[11];      /* toe (s) in bdt week */
        eph->week=(int)data[21];      /* bdt week */
        eph->toe=adjweek(bdt2time(eph->week,data[11]),toc);
        eph->ttr=adjweek(bdt2time(eph->week,data[27]),toc);
        
        eph->svh =(int)data[24];      /* satH1 */
        eph->sva=uraindex(data[23]);  /* ura (m->index) */
        
        eph->tgd[0]=   data[25];      /* TGD1 B1/B3 */
        eph->tgd[1]=   data[26];      /* TGD2 B1/B3 */
    }
    if (eph->iode<0||1023<eph->iode) {
        trace(2,"rinex nav invalid: sat=%2d iode=%d\n",sat,eph->iode);
    }
    if (eph->iodc<0||1023<eph->iodc) {
        trace(2,"rinex nav invalid: sat=%s iodc=%d\n",sat,eph->iodc);
    }
    return 1;
}
/* decode glonass ephemeris --------------------------------------------------*/
static int decode_geph(double ver, int sat, gtime_t toc, double *data,
                       geph_t *geph)
{
    geph_t geph0={0};
    gtime_t tof;
    double tow,tod;
    int week,dow;
    
    trace(3,"decode_geph: ver=%.2f sat=%2d\n",ver,sat);
    
    if (satsys(sat,NULL)!=SYS_GLO) {
        trace(2,"glonass ephemeris error: invalid satellite sat=%2d\n",sat);
        return 0;
    }
    *geph=geph0;
    
    geph->sat=sat;
    
    /* toc rounded by 15 min in utc */
    tow=time2gpst(toc,&week);
    toc=gpst2time(week,floor((tow+450.0)/900.0)*900);
    dow=(int)floor(tow/86400.0);
    
    /* time of frame in utc */
    tod=ver<=2.99?data[2]:fmod(data[2],86400.0); /* tod (v.2), tow (v.3) in utc */
    tof=gpst2time(week,tod+dow*86400.0);
    tof=adjday(tof,toc);
    
    geph->toe=utc2gpst(toc);   /* toc (gpst) */
    geph->tof=utc2gpst(tof);   /* tof (gpst) */
    
    /* iode = tb (7bit), tb =index of UTC+3H within current day */
    geph->iode=(int)(fmod(tow+10800.0,86400.0)/900.0+0.5);
    
    geph->taun=-data[0];       /* -taun */
    geph->gamn= data[1];       /* +gamman */
    
    geph->pos[0]=data[3]*1E3; geph->pos[1]=data[7]*1E3; geph->pos[2]=data[11]*1E3;
    geph->vel[0]=data[4]*1E3; geph->vel[1]=data[8]*1E3; geph->vel[2]=data[12]*1E3;
    geph->acc[0]=data[5]*1E3; geph->acc[1]=data[9]*1E3; geph->acc[2]=data[13]*1E3;
    
    geph->svh=(int)data[ 6];
    geph->frq=(int)data[10];
    geph->age=(int)data[14];
    
    /* some receiver output >128 for minus frequency number */
    if (geph->frq>128) geph->frq-=256;
    
    if (geph->frq<MINFREQ_GLO||MAXFREQ_GLO<geph->frq) {
        trace(2,"rinex gnav invalid freq: sat=%2d fn=%d\n",sat,geph->frq);
    }
    return 1;
}
/* decode geo ephemeris ------------------------------------------------------*/
static int decode_seph(double ver, int sat, gtime_t toc, double *data,
                       seph_t *seph)
{
    seph_t seph0={0};
    int week;
    
    trace(3,"decode_seph: ver=%.2f sat=%2d\n",ver,sat);
    
    if (satsys(sat,NULL)!=SYS_SBS) {
        trace(2,"geo ephemeris error: invalid satellite sat=%2d\n",sat);
        return 0;
    }
    *seph=seph0;
    
    seph->sat=sat;
    seph->t0 =toc;
    
    time2gpst(toc,&week);
    seph->tof=adjweek(gpst2time(week,data[2]),toc);
    
    seph->af0=data[0];
    seph->af1=data[1];
    
    seph->pos[0]=data[3]*1E3; seph->pos[1]=data[7]*1E3; seph->pos[2]=data[11]*1E3;
    seph->vel[0]=data[4]*1E3; seph->vel[1]=data[8]*1E3; seph->vel[2]=data[12]*1E3;
    seph->acc[0]=data[5]*1E3; seph->acc[1]=data[9]*1E3; seph->acc[2]=data[13]*1E3;
    
    seph->svh=(int)data[6];
    seph->sva=uraindex(data[10]);
    
    return 1;
}
/* read rinex navigation data body -------------------------------------------*/

/* add ephemeris to navigation data ------------------------------------------*/
static int add_eph(nav_t *nav, const eph_t *eph)
{
    eph_t *nav_eph;
    
    if (nav->nmax<=nav->n) {
        nav->nmax+=1024;
        if (!(nav_eph=(eph_t *)realloc(nav->eph,sizeof(eph_t)*nav->nmax))) {
            trace(1,"decode_eph malloc error: n=%d\n",nav->nmax);
            free(nav->eph); nav->eph=NULL; nav->n=nav->nmax=0;
            return 0;
        }
        nav->eph=nav_eph;
    }
    nav->eph[nav->n++]=*eph;
    return 1;
}
static int add_geph(nav_t *nav, const geph_t *geph)
{
    geph_t *nav_geph;
    
    if (nav->ngmax<=nav->ng) {
        nav->ngmax+=1024;
        if (!(nav_geph=(geph_t *)realloc(nav->geph,sizeof(geph_t)*nav->ngmax))) {
            trace(1,"decode_geph malloc error: n=%d\n",nav->ngmax);
            free(nav->geph); nav->geph=NULL; nav->ng=nav->ngmax=0;
            return 0;
        }
        nav->geph=nav_geph;
    }
    nav->geph[nav->ng++]=*geph;
    return 1;
}
static int add_seph(nav_t *nav, const seph_t *seph)
{
    seph_t *nav_seph;
    
    if (nav->nsmax<=nav->ns) {
        nav->nsmax+=1024;
        if (!(nav_seph=(seph_t *)realloc(nav->seph,sizeof(seph_t)*nav->nsmax))) {
            trace(1,"decode_seph malloc error: n=%d\n",nav->nsmax);
            free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;
            return 0;
        }
        nav->seph=nav_seph;
    }
    nav->seph[nav->ns++]=*seph;
    return 1;
}
/* read rinex nav/gnav/geo nav -----------------------------------------------*/

/* read rinex clock ----------------------------------------------------------*/

/* read rinex file -----------------------------------------------------------*/

/* uncompress and read rinex file --------------------------------------------*/

/* read rinex obs and nav files ------------------------------------------------
* read rinex obs and nav files
* args   : char *file    I      file (wild-card * expanded) ("": stdin)
*          int   rcv     I      receiver number for obs data
*         (gtime_t ts)   I      observation time start (ts.time==0: no limit)
*         (gtime_t te)   I      observation time end   (te.time==0: no limit)
*         (double tint)  I      observation time interval (s) (0:all)
*          char  *opt    I      rinex options (see below,"": no option)
*          obs_t *obs    IO     observation data   (NULL: no input)
*          nav_t *nav    IO     navigation data    (NULL: no input)
*          sta_t *sta    IO     station parameters (NULL: no input)
* return : status (1:ok,0:no data,-1:error)
* notes  : read data are appended to obs and nav struct
*          before calling the function, obs and nav should be initialized.
*          observation data and navigation data are not sorted.
*          navigation data may be duplicated.
*          call sortobs() or uniqnav() to sort data or delete duplicated eph.
*
*          rinex options (separated by spaces) :
*
*            -GLss[=shift]: select GPS signal ss (ss: RINEX 3 code, "1C","2W"...)
*            -RLss[=shift]: select GLO signal ss
*            -ELss[=shift]: select GAL signal ss
*            -JLss[=shift]: select QZS signal ss
*            -CLss[=shift]: select BDS signal ss
*            -SLss[=shift]: select SBS signal ss
*
*            shift: carrier phase shift to be added (cycle)
*
*-----------------------------------------------------------------------------*/


/* compare precise clock -----------------------------------------------------*/
static int cmppclk(const void *p1, const void *p2)
{
    pclk_t *q1=(pclk_t *)p1,*q2=(pclk_t *)p2;
    double tt=timediff(q1->time,q2->time);
    return tt<-1E-9?-1:(tt>1E-9?1:q1->index-q2->index);
}
/* combine precise clock -----------------------------------------------------*/
static void combpclk(nav_t *nav)
{
    pclk_t *nav_pclk;
    int i,j,k;
    
    trace(3,"combpclk: nc=%d\n",nav->nc);
    
    if (nav->nc<=0) return;
    
    qsort(nav->pclk,nav->nc,sizeof(pclk_t),cmppclk);
    
    for (i=0,j=1;j<nav->nc;j++) {
        if (fabs(timediff(nav->pclk[i].time,nav->pclk[j].time))<1E-9) {
            for (k=0;k<MAXSAT;k++) {
                if (nav->pclk[j].clk[k][0]==0.0) continue;
                nav->pclk[i].clk[k][0]=nav->pclk[j].clk[k][0];
                nav->pclk[i].std[k][0]=nav->pclk[j].std[k][0];
            }
        }
        else if (++i<j) nav->pclk[i]=nav->pclk[j];
    }
    nav->nc=i+1;
    
    if (!(nav_pclk=(pclk_t *)realloc(nav->pclk,sizeof(pclk_t)*nav->nc))) {
        free(nav->pclk); nav->pclk=NULL; nav->nc=nav->ncmax=0;
        trace(1,"combpclk malloc error nc=%d\n",nav->nc);
        return;
    }
    nav->pclk=nav_pclk;
    nav->ncmax=nav->nc;
    
    trace(4,"combpclk: nc=%d\n",nav->nc);
}
/* read rinex clock files ------------------------------------------------------
* read rinex clock files
* args   : char *file    I      file (wild-card * expanded)
*          nav_t *nav    IO     navigation data    (NULL: no input)
* return : number of precise clock
*-----------------------------------------------------------------------------*/

/* initialize rinex control ----------------------------------------------------
* initialize rinex control struct and reallocate memory for observation and
* ephemeris buffer in rinex control struct
* args   : rnxctr_t *rnx IO     rinex control struct
* return : status (1:ok,0:memory allocation error)
*-----------------------------------------------------------------------------*/

/* free rinex control ----------------------------------------------------------
* free observation and ephemris buffer in rinex control struct
* args   : rnxctr_t *rnx IO  rinex control struct
* return : none
*-----------------------------------------------------------------------------*/
/* open rinex data -------------------------------------------------------------
* fetch next rinex message and input a messsage from file
* args   : rnxctr_t *rnx IO  rinex control struct
*          FILE  *fp    I    file pointer
* return : status (-2: end of file, 0: no message, 1: input observation data,
*                   2: input navigation data)
*-----------------------------------------------------------------------------*/

/* input rinex control ---------------------------------------------------------
* fetch next rinex message and input a messsage from file
* args   : rnxctr_t *rnx IO  rinex control struct
*          FILE  *fp    I    file pointer
* return : status (-2: end of file, 0: no message, 1: input observation data,
*                   2: input navigation data)
*-----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
* output rinex functions
*-----------------------------------------------------------------------------*/

/* output obs types ver.2 ----------------------------------------------------*/
static void outobstype_ver2(char *buff, const rnxopt_t *opt,int l)
{
    const char label[]="# / TYPES OF OBSERV";
    int i;
    
    trace(3,"outobstype_ver2:\n");
    
    l+=sprintf(buff+l,"%6d",opt->nobs[0]);
    
    for (i=0;i<opt->nobs[0];i++) {
        if (i>0&&i%9==0) l+=sprintf(buff+l,"      ");
        
        l+=sprintf(buff+l,"%6s",opt->tobs[0][i]);
        
        if (i%9==8) l+=sprintf(buff+l,"%-20s\n",label);
    }
    if (opt->nobs[0]==0||i%9>0) {
        l+=sprintf(buff+l,"%*s%-20s\n",(9-i%9)*6,"",label);
    }
}
/* output obs types ver.3 ----------------------------------------------------*/
static void outobstype_ver3(char *buff, const rnxopt_t *opt,int l)
{
    const char label[]="SYS / # / OBS TYPES";
    int i,j;
    
    trace(3,"outobstype_ver3:\n");
    
    for (i=0;navsys[i];i++) {
        if (!(navsys[i]&opt->navsys)||!opt->nobs[i]) continue;
        
        l+=sprintf(buff+l,"%c  %3d",syscodes[i],opt->nobs[i]);
        
        for (j=0;j<opt->nobs[i];j++) {
            if (j>0&&j%13==0) l+=sprintf(buff+l,"      ");
            
            l+=sprintf(buff+l," %3s",opt->tobs[i][j]);
            
            if (j%13==12) l+=sprintf(buff+l,"  %-20s\n",label);
        }
        if (j%13>0) {
            l+=sprintf(buff+l,"%*s  %-20s\n",(13-j%13)*4,"",label);
        }
    }
}
/* output rinex obs header -----------------------------------------------------
* output rinex obd file header
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          nav_t  *nav      I   navigation data
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/
extern int outrnxobsh_s(char *buff, const rnxopt_t *opt, const nav_t *nav) // 已改动
{
    const char *glo_codes[]={"C1C","C1P","C2C","C2P"};
    double ep[6],pos[3]={0},del[3]={0};
    int i,j,k,n,prn[MAXPRNGLO];
    char date[32],*sys,*tsys="GPS";
    int l;
    trace(3,"outrnxobsh:\n");
    
    timestr_rnx(date);
    
    if (opt->rnxver<=2.99) { /* ver.2 */
        sys=opt->navsys==SYS_GPS?"G (GPS)":"M (MIXED)";
    }
    else { /* ver.3 */
        if      (opt->navsys==SYS_GPS) sys="G: GPS";
        else if (opt->navsys==SYS_GLO) sys="R: GLONASS";
        else if (opt->navsys==SYS_GAL) sys="E: Galielo";
        else if (opt->navsys==SYS_QZS) sys="J: QZSS";   /* ver.3.02 */
        else if (opt->navsys==SYS_CMP) sys="C: BeiDou"; /* ver.3.02 */
        else if (opt->navsys==SYS_SBS) sys="S: SBAS Payload";
        else sys="M: Mixed";
    }
    l=sprintf(buff,"%9.2f%-11s%-20s%-20s%-20s\n",opt->rnxver,"","OBSERVATION DATA",
            sys,"RINEX VERSION / TYPE");
    l+=sprintf(buff+l,"%-20.20s%-20.20s%-20.20s%-20s\n",opt->prog,opt->runby,date,
            "PGM / RUN BY / DATE");
    
    for (i=0;i<MAXCOMMENT;i++) {
        if (!*opt->comment[i]) continue;
        l+=sprintf(buff+l,"%-60.60s%-20s\n",opt->comment[i],"COMMENT");
    }
    l+=sprintf(buff+l,"%-60.60s%-20s\n",opt->marker,"MARKER NAME");
    l+=sprintf(buff+l,"%-20.20s%-40.40s%-20s\n",opt->markerno,"","MARKER NUMBER");
    
    if (opt->rnxver>2.99) {
        l+=sprintf(buff+l,"%-20.20s%-40.40s%-20s\n",opt->markertype,"","MARKER TYPE");
    }
    l+=sprintf(buff+l,"%-20.20s%-40.40s%-20s\n",opt->name[0],opt->name[1],
            "OBSERVER / AGENCY");
    l+=sprintf(buff+l,"%-20.20s%-20.20s%-20.20s%-20s\n",opt->rec[0],opt->rec[1],
            opt->rec[2],"REC # / TYPE / VERS");
    l+=sprintf(buff+l,"%-20.20s%-20.20s%-20.20s%-20s\n",opt->ant[0],opt->ant[1],
            opt->ant[2],"ANT # / TYPE");
    
    for (i=0;i<3;i++) if (fabs(opt->apppos[i])<1E8) pos[i]=opt->apppos[i];
    for (i=0;i<3;i++) if (fabs(opt->antdel[i])<1E8) del[i]=opt->antdel[i];
    l+=sprintf(buff+l,"%14.4f%14.4f%14.4f%-18s%-20s\n",pos[0],pos[1],pos[2],"",
            "APPROX POSITION XYZ");
    l+=sprintf(buff+l,"%14.4f%14.4f%14.4f%-18s%-20s\n",del[0],del[1],del[2],"",
            "ANTENNA: DELTA H/E/N");
    
    if (opt->rnxver<=2.99) { /* ver.2 */
        l+=sprintf(buff+l,"%6d%6d%-48s%-20s\n",1,1,"","WAVELENGTH FACT L1/2");
        outobstype_ver2(buff,opt,l);
    }
    else { /* ver.3 */
        outobstype_ver3(buff,opt,l);
    }
    if (opt->tint>0.0) {
        l+=sprintf(buff+l,"%10.3f%50s%-20s\n",opt->tint,"","INTERVAL");
    }
    time2epoch(opt->tstart,ep);
    l+=sprintf(buff+l,"  %04.0f%6.0f%6.0f%6.0f%6.0f%13.7f     %-12s%-20s\n",ep[0],
            ep[1],ep[2],ep[3],ep[4],ep[5],tsys,"TIME OF FIRST OBS");
    
    time2epoch(opt->tend,ep);
    l+=sprintf(buff+l,"  %04.0f%6.0f%6.0f%6.0f%6.0f%13.7f     %-12s%-20s\n",ep[0],
            ep[1],ep[2],ep[3],ep[4],ep[5],tsys,"TIME OF LAST OBS");
    
    if (opt->rnxver>=3.01) { /* ver.3.01 */
        for (i=0;navsys[i];i++) {
            if (!(navsys[i]&opt->navsys)||!opt->nobs[i]) continue;
            l+=sprintf(buff+l,"%c %-58s%-20s\n",syscodes[i],"","SYS / PHASE SHIFT");
        }
    }
    if (opt->rnxver>=3.02) { /* ver.3.02 */
        for (i=n=0;i<MAXPRNGLO;i++) {
            if (nav->glo_fcn[i]>=1) prn[n++]=i+1;
        }
        for (i=j=0;i<(n<=0?1:(n-1)/8+1);i++) {
            if (i==0) l+=sprintf(buff+l,"%3d",n); else l+=sprintf(buff+l,"   ");
            for (k=0;k<8;k++,j++) {
                if (j<n) {
                    l+=sprintf(buff+l," R%02d %2d",prn[j],nav->glo_fcn[prn[j]-1]-8);
                }
                else {
                    l+=sprintf(buff+l," %6s","");
                }
            }
            l+=sprintf(buff+l," %-20s\n","GLONASS SLOT / FRQ #");
        }
    }
    if (opt->rnxver>=3.02) { /* ver.3.02 */
        for (i=0;i<4;i++) l+=sprintf(buff+l," %3s %8.3f",glo_codes[i],0.0);
        l+=sprintf(buff+l,"%8s%-20s\n","","GLONASS COD/PHS/BIS");
    }
    printf("eph_h=%s\n",buff);
    return sprintf(buff+l,"%-60.60s%-20s\n","","END OF HEADER")!=EOF;
}
/* output obs data field -----------------------------------------------------*/
static void outrnxobsf(char *buff, double obs, int lli,int n)
{
    if (obs==0.0||obs<=-1E9||obs>=1E9) n+=sprintf(buff+n,"              ");
    else n+=sprintf(buff+n,"%14.3f",obs);
    if (lli<=0) n+=sprintf(buff+n,"  "); else n+=sprintf(buff+n,"%1.1d ",lli);
}
/* search obs data index -----------------------------------------------------*/
static int obsindex(double ver, int sys, const unsigned char *code,
                    const char *tobs, const char *mask)
{
    char *id;
    int i;
    
    for (i=0;i<NFREQ+NEXOBS;i++) {
        
        /* signal mask */
        if (mask[code[i]-1]=='0') continue;
        
        if (ver<=2.99) { /* ver.2 */
            if (!strcmp(tobs,"C1")&&(sys==SYS_GPS||sys==SYS_GLO||sys==SYS_QZS||
                sys==SYS_SBS||sys==SYS_CMP)) {
                if (code[i]==CODE_L1C) return i;
            }
            else if (!strcmp(tobs,"P1")) {
                if (code[i]==CODE_L1P||code[i]==CODE_L1W||code[i]==CODE_L1Y||
                    code[i]==CODE_L1N) return i;
            }
            else if (!strcmp(tobs,"C2")&&(sys==SYS_GPS||sys==SYS_QZS)) {
                if (code[i]==CODE_L2S||code[i]==CODE_L2L||code[i]==CODE_L2X)
                    return i;
            }
            else if (!strcmp(tobs,"C2")&&sys==SYS_GLO) {
                if (code[i]==CODE_L2C) return i;
            }
            else if (!strcmp(tobs,"P2")) {
                if (code[i]==CODE_L2P||code[i]==CODE_L2W||code[i]==CODE_L2Y||
                    code[i]==CODE_L2N||code[i]==CODE_L2D) return i;
            }
            else if (ver>=2.12&&tobs[1]=='A') { /* L1C/A */
                if (code[i]==CODE_L1C) return i;
            }
            else if (ver>=2.12&&tobs[1]=='B') { /* L1C */
                if (code[i]==CODE_L1S||code[i]==CODE_L1L||code[i]==CODE_L1X)
                    return i;
            }
            else if (ver>=2.12&&tobs[1]=='C') { /* L2C */
                if (code[i]==CODE_L2S||code[i]==CODE_L2L||code[i]==CODE_L2X)
                    return i;
            }
            else if (ver>=2.12&&tobs[1]=='D'&&sys==SYS_GLO) { /* GLO L2C/A */
                if (code[i]==CODE_L2C) return i;
            }
            else {
                id=code2obs(code[i],NULL);
                if (id[0]==tobs[1]) return i;
            }
        }
        else { /* ver.3 */
            id=code2obs(code[i],NULL);
            if (!strcmp(id,tobs+1)) return i;
        }
    }
    return -1;
}
/* output rinex obs body -------------------------------------------------------
* output rinex obs body
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          obsd_t *obs      I   observation data
*          int    n         I   number of observation data
*          int    flag      I   epoch flag (0:ok,1:power failure,>1:event flag)
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/
extern int outrnxobsb_s(char *buff, const rnxopt_t *opt, const obsd_t *obs, int n,
                      int flag)                     //  已改动
{
    const char *mask;
    double ep[6];
    char sats[MAXOBS][4]={""};
    int i,j,k,m,ns,sys,ind[MAXOBS],s[MAXOBS]={0};
    int l;
    
    trace(3,"outrnxobsb: n=%d\n",n);
    
    time2epoch(obs[0].time,ep);
    
    for (i=ns=0;i<n&&ns<MAXOBS;i++) {
        sys=satsys(obs[i].sat,NULL);
        if (!(sys&opt->navsys)||opt->exsats[obs[i].sat-1]) continue;
        if (!sat2code(obs[i].sat,sats[ns])) continue;
        switch (sys) {
            case SYS_GPS: s[ns]=0; break;
            case SYS_GLO: s[ns]=1; break;
            case SYS_GAL: s[ns]=2; break;
            case SYS_QZS: s[ns]=3; break;
            case SYS_SBS: s[ns]=4; break;
            case SYS_CMP: s[ns]=5; break;
        }
        if (!opt->nobs[opt->rnxver<=2.99?0:s[ns]]) continue;
        ind[ns++]=i;
    }
    if (opt->rnxver<=2.99) { /* ver.2 */
        l=sprintf(buff," %02d %2.0f %2.0f %2.0f %2.0f%11.7f  %d%3d",
                (int)ep[0]%100,ep[1],ep[2],ep[3],ep[4],ep[5],flag,ns);
                
        for (i=0;i<ns;i++) {
            if (i>0&&i%12==0) l+=sprintf(buff+l,"\n%32s","");
            l+=sprintf(buff+l,"%-3s",sats[i]);
        }
    }
    else { /* ver.3 */
        l=sprintf(buff,"> %04.0f %2.0f %2.0f %2.0f %2.0f%11.7f  %d%3d%21s\n",
                ep[0],ep[1],ep[2],ep[3],ep[4],ep[5],flag,ns,"");
    }
    for (i=0;i<ns;i++) {
        sys=satsys(obs[ind[i]].sat,NULL);
        
        if (opt->rnxver<=2.99) { /* ver.2 */
            m=0;
            mask=opt->mask[s[i]];
        }
        else { /* ver.3 */
            l+=sprintf(buff+l,"%-3s",sats[i]);
            m=s[i];
            mask=opt->mask[s[i]];
        }
        for (j=0;j<opt->nobs[m];j++) {
            
            if (opt->rnxver<=2.99) { /* ver.2 */
                if (j%5==0) l+=sprintf(buff+l,"\n");
            }
            /* search obs data index */
            if ((k=obsindex(opt->rnxver,sys,obs[ind[i]].code,opt->tobs[m][j],
                            mask))<0) {
                outrnxobsf(buff,0.0,-1,l);
                continue;
            }
            /* output field */
            switch (opt->tobs[m][j][0]) {
                case 'C':
                case 'P': outrnxobsf(buff,obs[ind[i]].P[k],-1,n); break;
                case 'L': outrnxobsf(buff,obs[ind[i]].L[k],obs[ind[i]].LLI[k],n); break;
                case 'D': outrnxobsf(buff,obs[ind[i]].D[k],-1,n); break;
                case 'S': outrnxobsf(buff,obs[ind[i]].SNR[k]*0.25,-1,n); break;
            }
        }
        if (opt->rnxver>2.99&& (l+=sprintf(buff+l,"\n")==EOF)) return 0;   // 改动 fprintf(fp,"\n")==EOF
    }
    if (opt->rnxver>2.99) return 1;
    printf("obs_b=%s",buff);
    return sprintf(buff+l,"\n")!=EOF;
    
}
/* output nav member by rinex nav format -------------------------------------*/
static void outnavf(char *buff, double value,int n)  // 已改动
{
    double e=fabs(value)<1E-99?0.0:floor(log10(fabs(value))+1.0);
    n+=sprintf(buff+n," %s.%012.0fE%+03.0f",value<0.0?"-":" ",fabs(value)/pow(10.0,e-12.0),e);
}
/* output rinex nav header -----------------------------------------------------
* output rinex nav file header
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          nav_t  nav       I   navigation data (NULL: no input)
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/
extern int outrnxnavh_s(char *buff, const rnxopt_t *opt, const nav_t *nav)  // 已改动
{
    int i;
    char date[64],*sys;
    int l;
    
    trace(3,"outrnxnavh:\n");
    
    timestr_rnx(date);
    
    if (opt->rnxver<=2.99) { /* ver.2 */
        l=sprintf(buff,"%9.2f           %-20s%-20s%-20s\n",opt->rnxver,
                "N: GPS NAV DATA","","RINEX VERSION / TYPE");
    }
    else { /* ver.3 */
        if      (opt->navsys==SYS_GPS) sys="G: GPS";
        else if (opt->navsys==SYS_GLO) sys="R: GLONASS";
        else if (opt->navsys==SYS_GAL) sys="E: Galileo";
        else if (opt->navsys==SYS_QZS) sys="J: QZSS";   /* v.3.02 */
        else if (opt->navsys==SYS_CMP) sys="C: BeiDou"; /* v.3.02 */
        else if (opt->navsys==SYS_SBS) sys="S: SBAS Payload";
        else sys="M: Mixed";
        
        l=sprintf(buff,"%9.2f           %-20s%-20s%-20s\n",opt->rnxver,
                "N: GNSS NAV DATA",sys,"RINEX VERSION / TYPE");
    }
    l+=sprintf(buff+l,"%-20.20s%-20.20s%-20.20s%-20s\n",opt->prog,opt->runby,date,
            "PGM / RUN BY / DATE");
    
    for (i=0;i<MAXCOMMENT;i++) {
        if (!*opt->comment[i]) continue;
        l+=sprintf(buff+l,"%-60.60s%-20s\n",opt->comment[i],"COMMENT");
    }
    if (opt->rnxver<=2.99) { /* ver.2 */
        if (opt->outiono) {
            l+=sprintf(buff+l,"  %12.4E%12.4E%12.4E%12.4E%10s%-20s\n",nav->ion_gps[0],
                    nav->ion_gps[1],nav->ion_gps[2],nav->ion_gps[3],"","ION ALPHA");
            l+=sprintf(buff+l,"  %12.4E%12.4E%12.4E%12.4E%10s%-20s\n",nav->ion_gps[4],
                    nav->ion_gps[5],nav->ion_gps[6],nav->ion_gps[7],"","ION BETA");
        }
        if (opt->outtime) {
            l+=sprintf(buff+l,"   ");
            outnavf(buff,nav->utc_gps[0],l);
            outnavf(buff,nav->utc_gps[1],l);
            l+=sprintf(buff+l,"%9.0f%9.0f %-20s\n",nav->utc_gps[2],nav->utc_gps[3],
                    "DELTA-UTC: A0,A1,T,W");
        }
    }
    else { /* ver.3 */
        if (opt->navsys&SYS_GPS) {
            if (opt->outiono) {
                l+=sprintf(buff+l,"GPSA %12.4E%12.4E%12.4E%12.4E%7s%-20s\n",
                        nav->ion_gps[0],nav->ion_gps[1],nav->ion_gps[2],
                        nav->ion_gps[3],"","IONOSPHERIC CORR");
                l+=sprintf(buff+l,"GPSB %12.4E%12.4E%12.4E%12.4E%7s%-20s\n",
                        nav->ion_gps[4],nav->ion_gps[5],nav->ion_gps[6],
                        nav->ion_gps[7],"","IONOSPHERIC CORR");
            }
        }
        if (opt->navsys&SYS_GAL) {
            if (opt->outiono) {
                l+=sprintf(buff+l,"GAL  %12.4E%12.4E%12.4E%12.4E%7s%-20s\n",
                        nav->ion_gal[0],nav->ion_gal[1],nav->ion_gal[2],0.0,"",
                        "IONOSPHERIC CORR");
            }
        }
        if (opt->navsys&SYS_QZS) {
            if (opt->outiono) {
                l+=sprintf(buff+l,"QZSA %12.4E%12.4E%12.4E%12.4E%7s%-20s\n",
                        nav->ion_qzs[0],nav->ion_qzs[1],nav->ion_qzs[2],
                        nav->ion_qzs[3],"","IONOSPHERIC CORR");
                l+=sprintf(buff+l,"QZSB %12.4E%12.4E%12.4E%12.4E%7s%-20s\n",
                        nav->ion_qzs[4],nav->ion_qzs[5],nav->ion_qzs[6],
                        nav->ion_qzs[7],"","IONOSPHERIC CORR");
            }
        }
        if (opt->navsys&SYS_GPS) {
            if (opt->outtime) {
                l+=sprintf(buff+l,"GPUT %17.10E%16.9E%7.0f%5.0f %-5s %-2s %-20s\n",
                        nav->utc_gps[0],nav->utc_gps[1],nav->utc_gps[2],
                        nav->utc_gps[3],"","","TIME SYSTEM CORR");
            }
        }
        if (opt->navsys&SYS_GAL) {
            if (opt->outtime) {
                l+=sprintf(buff+l,"GAUT %17.10E%16.9E%7.0f%5.0f %-5s %-2s %-20s\n",
                        nav->utc_gal[0],nav->utc_gal[1],nav->utc_gal[2],
                        nav->utc_gal[3],"","","TIME SYSTEM CORR");
            }
        }
        if (opt->navsys&SYS_QZS) { /* ver.3.02 */
            if (opt->outtime) {
                l+=sprintf(buff+l,"QZUT %17.10E%16.9E%7.0f%5.0f %-5s %-2s %-20s\n",
                        nav->utc_qzs[0],nav->utc_qzs[1],nav->utc_qzs[2],
                        nav->utc_qzs[3],"","","TIME SYSTEM CORR");
            }
        }
        if (opt->navsys&SYS_CMP) { /* ver.3.02 */
            if (opt->outtime) {
                l+=sprintf(buff+l,"BDUT %17.10E%16.9E%7.0f%5.0f %-5s %-2s %-20s\n",
                        nav->utc_cmp[0],nav->utc_cmp[1],nav->utc_cmp[2],
                        nav->utc_cmp[3],"","","TIME SYSTEM CORR");
            }
        }
    }
    if (opt->outleaps) {
        l+=sprintf(buff+l,"%6d%54s%-20s\n",nav->leaps,"","LEAP SECONDS");
    }
    //printf("obs_h=%s\n",buff);
    return sprintf(buff+l,"%60s%-20s\n","","END OF HEADER")!=EOF;
}
/* output rinex nav body -------------------------------------------------------
* output rinex nav file body record
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          eph_t  *eph      I   ephemeris
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/
extern int outrnxnavb_s(char *buff, const rnxopt_t *opt, const eph_t *eph)  // 已改动
{
    double ep[6],ttr;
    int week,sys,prn;
    char code[32],*sep;
    int n;
    
    trace(3,"outrnxgnavb: sat=%2d\n",eph->sat);
    
    if (!(sys=satsys(eph->sat,&prn))||!(sys&opt->navsys)) return 0;
    
    time2epoch(eph->toc,ep);
    
    if (opt->rnxver>2.99||sys==SYS_GAL||sys==SYS_CMP) { /* ver.3 or ver.2 GAL */
        if (!sat2code(eph->sat,code)) return 0;
        n=sprintf(buff,"%-3s %04.0f %2.0f %2.0f %2.0f %2.0f %2.0f",code,ep[0],ep[1],
                ep[2],ep[3],ep[4],ep[5]);
        sep="    ";
    }
    else if (sys==SYS_QZS) { /* ver.2 or ver.3.02 QZS */
        if (!sat2code(eph->sat,code)) return 0;
        n=sprintf(buff,"%-3s %02d %2.0f %2.0f %2.0f %2.0f %4.1f",code,
                (int)ep[0]%100,ep[1],ep[2],ep[3],ep[4],ep[5]);
        sep="    ";
    }
    else {
        n=sprintf(buff,"%2d %02d %2.0f %2.0f %2.0f %2.0f %4.1f",prn,
                (int)ep[0]%100,ep[1],ep[2],ep[3],ep[4],ep[5]);
        sep="   ";
    }
    outnavf(buff,eph->f0     ,n);
    outnavf(buff,eph->f1     ,n);
    outnavf(buff,eph->f2     ,n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,eph->iode   ,n); /* GPS/QZS: IODE, GAL: IODnav, BDS: AODE */
    outnavf(buff,eph->crs    ,n);
    outnavf(buff,eph->deln   ,n);
    outnavf(buff,eph->M0     ,n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,eph->cuc    ,n);
    outnavf(buff,eph->e      ,n);
    outnavf(buff,eph->cus    ,n);
    outnavf(buff,sqrt(eph->A),n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,eph->toes   ,n);
    outnavf(buff,eph->cic    ,n);
    outnavf(buff,eph->OMG0   ,n);
    outnavf(buff,eph->cis    ,n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,eph->i0     ,n);
    outnavf(buff,eph->crc    ,n);
    outnavf(buff,eph->omg    ,n);
    outnavf(buff,eph->OMGd   ,n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,eph->idot   ,n);
    outnavf(buff,eph->code   ,n);
    outnavf(buff,eph->week   ,n); /* GPS/QZS: GPS week, GAL: GAL week, BDS: BDT week */
    outnavf(buff,eph->flag   ,n);
    n+=sprintf(buff+n,"\n%s",sep  );
    
    outnavf(buff,uravalue(eph->sva),n);
    outnavf(buff,eph->svh    ,n);
    outnavf(buff,eph->tgd[0] ,n); /* GPS/QZS:TGD, GAL:BGD E5a/E1, BDS: TGD1 B1/B3 */
    if (sys==SYS_GAL||sys==SYS_CMP) {
        outnavf(buff,eph->tgd[1],n); /* GAL:BGD E5b/E1, BDS: TGD2 B2/B3 */
    }
    else {
        outnavf(buff,eph->iodc,n);   /* GPS/QZS:IODC */
    }
    n+=sprintf(buff+n,"\n%s",sep  );
    
    ttr=time2gpst(eph->ttr,&week);
    outnavf(buff,ttr+(week-eph->week)*604800.0,n);
    
    if (sys==SYS_GPS||sys==SYS_QZS) {
        outnavf(buff,eph->fit,n);
    }
    else if (sys==SYS_CMP) {
        outnavf(buff,eph->iodc,n); /* AODC */
    }
    else {
        outnavf(buff,0.0,n); /* spare */
    }
    //printf("eph_buff=%s",buff);
    return sprintf(buff+n,"\n")!=EOF;
    
}
/* output rinex gnav header ----------------------------------------------------
* output rinex gnav (glonass navigation) file header
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          nav_t  nav       I   navigation data (NULL: no input)
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/

/* output rinex beidou nav header ----------------------------------------------
* output rinex beidou nav file header (2.12 extention and 3.02)
* args   : FILE   *fp       I   output file pointer
*          rnxopt_t *opt    I   rinex options
*          nav_t  nav       I   navigation data (NULL: no input)
* return : status (1:ok, 0:output error)
*-----------------------------------------------------------------------------*/

