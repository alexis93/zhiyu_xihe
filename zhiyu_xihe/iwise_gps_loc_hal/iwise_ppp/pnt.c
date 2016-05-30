#include "rtklib.h"
//#include "gps_common.h"
#define SQR(x)      ((x)*(x))

#define NX          (4+3)       /* # of estimated parameters */  //modified by yuan;

#define MAXITR      10          /* max number of iteration for point pos */
#define ERR_ION     5.0         /* ionospheric delay std (m) */
#define ERR_TROP    3.0         /* tropspheric delay std (m) */
#define ERR_SAAS    0.3         /* saastamoinen model error std (m) */
#define ERR_BRDCI   0.5         /* broadcast iono model error factor */
#define ERR_CBIAS   0.3         /* code bias error std (m) */
#define REL_HUMI    0.7         /* relative humidity for saastamoinen model */
const double chisqr[100] = {      /* chi-sqr(n) (alpha=0.001) */
	10.8, 13.8, 16.3, 18.5, 20.5, 22.5, 24.3, 26.1, 27.9, 29.6,
	31.3, 32.9, 34.5, 36.1, 37.7, 39.3, 40.8, 42.3, 43.8, 45.3,
	46.8, 48.3, 49.7, 51.2, 52.6, 54.1, 55.5, 56.9, 58.3, 59.7,
	61.1, 62.5, 63.9, 65.2, 66.6, 68.0, 69.3, 70.7, 72.1, 73.4,
	74.7, 76.0, 77.3, 78.6, 80.0, 81.3, 82.6, 84.0, 85.4, 86.7,
	88.0, 89.3, 90.6, 91.9, 93.3, 94.7, 96.0, 97.4, 98.7, 100,
	101, 102, 103, 104, 105, 107, 108, 109, 110, 112,
	113, 114, 115, 116, 118, 119, 120, 122, 123, 125,
	126, 127, 128, 129, 131, 132, 133, 134, 135, 137,
	138, 139, 140, 142, 143, 144, 145, 147, 148, 149
};
/* pseudorange measurement error variance ------------------------------------*/
static double varerr(const prcopt_t *opt, double el, int sys)
{
	double fact, varr;
	fact = sys == SYS_GLO ? EFACT_GLO : (sys == SYS_SBS ? EFACT_SBS : EFACT_GPS);
	varr = SQR(opt->err[0])*(SQR(opt->err[1]) + SQR(opt->err[2]) / sin(el));
	if (opt->ionoopt == IONOOPT_IFLC) varr *= SQR(3.0); /* iono-free */
	return SQR(fact)*varr;
}
/* test SNR mask ---------------------------------------------------------------
* test SNR mask
* args   : int    base      I   rover or base-station (0:rover,1:base station)
*          int    freq      I   frequency (0:L1,1:L2,2:L3,...)
*          double el        I   elevation angle (rad)
*          double snr       I   C/N0 (dBHz)
*          snrmask_t *mask  I   SNR mask
* return : status (1:masked,0:unmasked)
*-----------------------------------------------------------------------------*/
extern int testsnr(int base, int freq, double el, double snr,
	const snrmask_t *mask)
{
	double minsnr, a;
	int i;

	if (!mask->ena[base] || freq < 0 || freq >= NFREQ) return 0;

	a = (el*R2D + 5.0) / 10.0;
	i = (int)floor(a); a -= i;
	if (i<1) minsnr = mask->mask[freq][0];
	else if (i>8) minsnr = mask->mask[freq][8];
	else minsnr = (1.0 - a)*mask->mask[freq][i - 1] + a*mask->mask[freq][i];

	return snr < minsnr;
}
/* get tgd parameter (m) -----------------------------------------------------*/
static double gettgd(int sat, const nav_t *nav)
{
	int i;
	for (i = 0; i < nav->n; i++) {
		if (nav->eph[i].sat != sat) continue;
		return CLIGHT*nav->eph[i].tgd[0];
	}
	return 0.0;
}
/* psendorange with code bias correction -------------------------------------*/
static double prange(const obsd_t *obs, const nav_t *nav, const double *azel,
                     int iter, const prcopt_t *opt, double *var)
{
    const double *lam=nav->lam[obs->sat-1];
    double PC,P1,P2,P1_P2,P1_C1,P2_C2,gamma;
    int i=0,j=1,sys;
    
    *var=0.0;
    
    if (!(sys=satsys(obs->sat,NULL))) return 0.0;
    
    /* L1-L2 for GPS/GLO/QZS, L1-L5 for GAL/SBS */
    if (NFREQ>=3&&(sys&(SYS_GAL|SYS_SBS))) j=2;
    
    if (NFREQ<2||lam[i]==0.0||lam[j]==0.0) return 0.0;
    
    /* test snr mask */
    if (iter>0) {
        if (testsnr(0,i,azel[1],obs->SNR[i]*0.25,&opt->snrmask)) {
            
            return 0.0;
        }
        if (opt->ionoopt==IONOOPT_IFLC) {
            if (testsnr(0,j,azel[1],obs->SNR[j]*0.25,&opt->snrmask)) return 0.0;
        }
    }
    gamma=SQR(lam[j])/SQR(lam[i]); /* f1^2/f2^2 */
    P1=obs->P[i];
    P2=obs->P[j];
    P1_P2=nav->cbias[obs->sat-1][0];
    P1_C1=nav->cbias[obs->sat-1][1];
    P2_C2=nav->cbias[obs->sat-1][2];
    
    /* if no P1-P2 DCB, use TGD instead */
    if (P1_P2==0.0&&(sys&(SYS_GPS|SYS_GAL|SYS_QZS))) {
        P1_P2=(1.0-gamma)*gettgd(obs->sat,nav);
    }
    if (opt->ionoopt==IONOOPT_IFLC) { /* dual-frequency */
        
        if (P1==0.0||P2==0.0) return 0.0;
        if (obs->code[i]==CODE_L1C) P1+=P1_C1; /* C1->P1 */
        if (obs->code[j]==CODE_L2C) P2+=P2_C2; /* C2->P2 */
        
        /* iono-free combination */
        PC=(gamma*P1-P2)/(gamma-1.0);
    }
    else { /* single-frequency */
        
        if (P1==0.0) return 0.0;
        if (obs->code[i]==CODE_L1C) P1+=P1_C1; /* C1->P1 */
        PC=P1-P1_P2/(1.0-gamma);
    }
    if (opt->sateph==EPHOPT_SBAS) PC-=P1_C1; /* sbas clock based C1 */
    
    *var=SQR(ERR_CBIAS);
    
    return PC;
}
/* tropospheric correction -----------------------------------------------------
* compute tropospheric correction
* args   : gtime_t time     I   time
*          nav_t  *nav      I   navigation data
*          double *pos      I   receiver position {lat,lon,h} (rad|m)
*          double *azel     I   azimuth/elevation angle {az,el} (rad)
*          int    tropopt   I   tropospheric correction option (TROPOPT_???)
*          double *trp      O   tropospheric delay (m)
*          double *var      O   tropospheric delay variance (m^2)
* return : status(1:ok,0:error)
*-----------------------------------------------------------------------------*/
extern int tropcorr(gtime_t time, const nav_t *nav, const double *pos,
	const double *azel, int tropopt, double *trp, double *var)
{
	/* saastamoinen model */
	if (tropopt == TROPOPT_SAAS || tropopt == TROPOPT_EST || tropopt == TROPOPT_ESTG) {
		*trp = tropmodel(time, pos, azel, REL_HUMI);
		*var = SQR(ERR_SAAS / (sin(azel[1]) + 0.1));
		return 1;
	}
	/* sbas troposphere model */
	if (tropopt == TROPOPT_SBAS) {
		*trp = sbstropcorr(time, pos, azel, var);
		return 1;
	}
	/* no correction */
	*trp = 0.0;
	*var = tropopt == TROPOPT_OFF ? SQR(ERR_TROP) : 0.0;
	return 1;
}
/* pseudorange residuals -----------------------------------------------------*/
static int rescode(int iter, const obsd_t *obs, int n, const double *rs,
                   const double *dts, const double *vare, const int *svh,
                   const nav_t *nav, const double *x, prcopt_t *opt,
                   double *v, double *H, double *var, double *azel, int *vsat,
				   double *resp, int *ns, rtk_t *rtk)
{
    double r,dion,dtrp,vmeas,vion,vtrp,rr[3],pos[3],dtr,e[3],P;
	int i, j, nv = 0, sys, mask[4] = { 0 }; double azl = 0;
    
    for (i=0;i<3;i++) rr[i]=x[i]; dtr=x[3];
    
    ecef2pos(rr,pos);
    
    for (i=*ns=0;i<n&&i<MAXOBS;i++) {
        vsat[i]=0; azel[i*2]=azel[1+i*2]=resp[i]=0.0;

		if (obs[i].sat>32){   //added by yuan;
			continue;
		}
		//���������ز�����α��Ͳ�����������ǵķ���;
		if ((fabs(obs[i].L[0]) < 0.0001) || (fabs(obs[i].P[0]) < 0.0001)){
			continue;
		}
        if (!(sys=satsys(obs[i].sat,NULL))) continue;
        
        /* reject duplicated observation data */
        if (i<n-1&&i<MAXOBS-1&&obs[i].sat==obs[i+1].sat) {
            
            i++;
            continue;
        }
        /* geometric distance/azimuth/elevation angle */
        if ((r=geodist(rs+i*6,rr,e))<=0.0||
            satazel(pos,e,azel+i*2)<opt->elmin) continue;
		azl = satazel(pos, e, azel + i * 2);
        /* psudorange with code bias correction */
        if ((P=prange(obs+i,nav,azel+i*2,iter,opt,&vmeas))==0.0) continue;
        
        /* excluded satellite? */
        if (satexclude(obs[i].sat,svh[i],opt)) continue;
        
        /* ionospheric corrections */
        if (!ionocorr(obs[i].time,nav,obs[i].sat,pos,azel+i*2,
                      iter>0?opt->ionoopt:IONOOPT_BRDC,&dion,&vion)) continue;
        
        /* tropospheric corrections */
        if (!tropcorr(obs[i].time,nav,pos,azel+i*2,
                      iter>0?opt->tropopt:TROPOPT_SAAS,&dtrp,&vtrp)) {
            continue;
        }
        /* pseudorange residual */
        v[nv]=P-(r+dtr-CLIGHT*dts[i*2]+dion+dtrp);

        /* design matrix */
        for (j=0;j<NX;j++) H[j+nv*NX]=j<3?-e[j]:(j==3?1.0:0.0);
        
        /* time system and receiver bias offset correction */
        if      (sys==SYS_GLO) {v[nv]-=x[4]; H[4+nv*NX]=1.0; mask[1]=1;}
        else if (sys==SYS_GAL) {v[nv]-=x[5]; H[5+nv*NX]=1.0; mask[2]=1;}
        else if (sys==SYS_CMP) {v[nv]-=x[6]; H[6+nv*NX]=1.0; mask[3]=1;}
        else mask[0]=1;
        
        vsat[i]=1; resp[i]=v[nv]; (*ns)++;
        
		rtk->sat_[nv] = obs[i].sat;  //added by yuan;
		/* error variance */
		if ((rtk->counter > 5) && (obs[i].mark_ < 4)){
			var[nv++] = varerr(opt, azel[1 + i * 2], sys) + (4 - obs[i].mark_)*2 + vare[i] + vmeas + vion + vtrp;  //varm:α���ز�����;vare:������������;vart:����������;
			//var[nv - 1] = var[nv - 1] * (4-obs[i].mark_);    //�������ʧ���������������Ŵ�;
		}
		else{
			var[nv++] = varerr(opt, azel[1 + i * 2], sys) + vare[i] + vmeas + vion + vtrp;  //varm:α���ز�����;vare:������������;vart:����������;
		}
        
        
    }
    /* constraint to avoid rank-deficient */
    for (i=0;i<4;i++) {
        if (mask[i]) continue;
        v[nv]=0.0;
        for (j=0;j<NX;j++) H[j+nv*NX]=j==i+3?1.0:0.0;
        var[nv++]=0.01;
    }
    return nv;
}
#define SQRT(x)     ((x)<0.0?0.0:sqrt(x))

extern void dops(int ns, const double *azel, double elmin, double *dop)
{
	double H[4 * MAXSAT], Q[16], cosel, sinel;
	int i, n;

	for (i = 0; i < 4; i++) dop[i] = 0.0;
	for (i = n = 0; i < ns&&i < MAXSAT; i++) {
		if (azel[1 + i * 2] < elmin || azel[1 + i * 2] <= 0.0) continue;
		cosel = cos(azel[1 + i * 2]);
		sinel = sin(azel[1 + i * 2]);
		H[4 * n] = cosel*sin(azel[i * 2]);
		H[1 + 4 * n] = cosel*cos(azel[i * 2]);
		H[2 + 4 * n] = sinel;
		H[3 + 4 * n++] = 1.0;
	}
	if (n < 4) return;

	matmul("NT", 4, 4, n, 1.0, H, H, 0.0, Q);
	if (!matinv(Q, 4)) {
		dop[0] = SQRT(Q[0] + Q[5] + Q[10] + Q[15]); /* GDOP */
		dop[1] = SQRT(Q[0] + Q[5] + Q[10]);       /* PDOP */
		dop[2] = SQRT(Q[0] + Q[5]);             /* HDOP */
		dop[3] = SQRT(Q[10]);                 /* VDOP */
	}
}
/* validate solution ---------------------------------------------------------*/
static int valsol(const double *azel, const int *vsat, int n,
	const prcopt_t *opt, const double *v, int nv, int nx,
	char *msg)
{
	double azels[MAXOBS * 2], dop[4], vv;
	int i, ns;

	//trace(3, "valsol  : n=%d nv=%d\n", n, nv);

	/* chi-square validation of residuals */
	vv = dot(v, v, nv);
	if (nv > nx&&vv > chisqr[nv - nx - 1]) {
		//sprintf(msg, "chi-square error nv=%d vv=%.1f cs=%.1f", nv, vv, chisqr[nv - nx - 1]);
		return 0;
	}
	/* large gdop check */
	for (i = ns = 0; i<n; i++) {
		if (!vsat[i]) continue;
		azels[ns * 2] = azel[i * 2];
		azels[1 + ns * 2] = azel[1 + i * 2];
		ns++;
	}
	dops(ns, azels, opt->elmin, dop);
	if (dop[0] <= 0.0 || dop[0]>opt->maxgdop) {
		//sprintf(msg, "gdop error nv=%d gdop=%.1f", nv, dop[0]);
		return 0;
	}
	return 1;
}
/* estimate receiver position ------------------------------------------------*/
static int estpos(const obsd_t *obs, int n, const double *rs, const double *dts,
                  const double *vare, const int *svh, const nav_t *nav,
                  prcopt_t *opt, sol_t *sol, double *azel, int *vsat,
				  double *resp, char *msg, rtk_t *rtk)
{
    double x[NX]={0},dx[NX],Q[NX*NX],sig;
    int i,j,k,info,stat,nv,ns;
	double rms = 0;   //added by yuan;
	double RMS = 0;
	int nv_ = 0; double temp = 0; int kk = 0;
    double v[(MAXOBS_+4)*1];double H[(MAXOBS_+4)*135];double var[MAXOBS_+4*1];
    for (i=0;i<3;i++) x[i]=sol->rr[i];
	for (i = 0; i < MAXITR; i++) {
        /* pseudorange residuals */
        nv=rescode(i,obs,n,rs,dts,vare,svh,nav,x,opt,v,H,var,azel,vsat,resp,
                   &ns,rtk);
        if (nv<4) {
            //sprintf(msg,"lack of valid sats ns=%d",nv);
            break;
        }
		//added by yuan, ̽��ϴ�ֲ���н�׼ȷ�������������;
		if ( (rtk->counter>=1) && (i>=1) ){
        for (j = 0; j < nv;j++){
			if (fabs(v[j]) != 0.0){
				rms += v[j];
				nv_++;
			}
		}
		rms = rms / nv_;
		for (j = 0; j < nv_; j++){
			RMS += (v[j] - rms)*(v[j] - rms);
		}
		RMS = sqrt(RMS/nv_);   //�в�ľ�����;
		if (RMS>2.5){
			temp = fabs(v[0] - rms);
			for (j = 1; j < nv_;j++){
				if (temp <= fabs(v[j] - rms)){
					temp = fabs(v[j] - rms);
					kk   = j;
				} 
				else{
					continue;
				}
			}
		    opt->exsats[rtk->sat_[kk] - 1] = 1;
		}
		rms = 0; RMS = 0; nv_ = 0; temp = 0; kk = 0;
		//nv = nv - 1;   //����һ������;
		}
		if (nv < NX) {
			//sprintf(msg, "lack of valid sats ns=%d", nv);
			break;
		}

        /* weight by variance */
		for (j = 0; j < nv; j++) {
			sig = sqrt(var[j]);
			v[j] /= sig;
			for (k = 0; k < NX; k++) H[k + j*NX] /= sig;
		}
        /* least square estimation */
        if ((info=lsq(H,v,NX,nv,dx,Q))) {
            //sprintf(msg,"lsq error info=%d",info);
            break;
        }
        for (j=0;j<NX;j++) 
			x[j]+=dx[j];
        if (norm(dx,NX)<1E-4) {
            sol->type=0;
            sol->time=timeadd(obs[0].time,-x[3]/CLIGHT);
            sol->dtr[0]=x[3]/CLIGHT; /* receiver clock bias (s) */
            sol->dtr[1]=x[4]/CLIGHT; /* glo-gps time offset (s) */
            sol->dtr[2]=x[5]/CLIGHT; /* gal-gps time offset (s) */
            sol->dtr[3]=x[6]/CLIGHT; /* bds-gps time offset (s) */
            for (j=0;j<6;j++) sol->rr[j]=j<3?x[j]:0.0;
            for (j=0;j<3;j++) sol->qr[j]=(float)Q[j+j*NX];
            sol->qr[3]=(float)Q[1];    /* cov xy */
            sol->qr[4]=(float)Q[2+NX]; /* cov yz */
            sol->qr[5]=(float)Q[2];    /* cov zx */
            sol->ns=(unsigned char)ns;
            sol->age=sol->ratio=0.0;
            
            /* validate solution */
			if ((stat = valsol(azel, vsat, n, opt, v, nv, NX, msg))) {
				sol->stat = opt->sateph == EPHOPT_SBAS ? SOLQ_SBAS : SOLQ_SINGLE;
			}
            
            return stat;
        }
    }
    
    
    return 0;
}
//SPP, added by yuan;
extern int pntpos(rtk_t *rtk,obsd_t *obs, int n, nav_t *nav,double *azel,char *msg)
{
    prcopt_t opt_=rtk->opt;
    int i,stat,vsat[MAXOBS]={0},svh[MAXOBS];
	double rs[6 * MAXOBS_]; double dts[2 * MAXOBS_]; double var[1 * MAXOBS_];
	double azel_[2 * MAXOBS_]; double resp[1 * MAXOBS_];
    
    rtk->sol.stat=SOLQ_NONE;
    if (n<=0) { return 0;}
	rtk->sol.time = obs[0].time; msg[0] = '\0';
    opt_.sateph =EPHOPT_BRDC;    //���ù㲥������modified by yuan
    opt_.ionoopt=IONOOPT_BRDC;
    opt_.tropopt=TROPOPT_SAAS;
    /* satellite positons, velocities and clocks */
	satposs(rtk->sol.time, obs, n, nav, opt_.sateph, rs, dts, var, svh);
    
    /* estimate receiver position with pseudorange */
	stat = estpos(obs, n, rs, dts, var, svh, nav, &opt_, &rtk->sol, azel_, vsat, resp, msg, rtk);
    
    if (azel) {
        for (i=0;i<n*2;i++) azel[i]=azel_[i];
    }
    if (rtk->ssat) {
        for (i=0;i<MAXSAT;i++) {
			rtk->ssat[i].vs = 0;
			rtk->ssat[i].azel[0] = rtk->ssat[i].azel[1] = 0.0;
			rtk->ssat[i].resp[0] = rtk->ssat[i].resc[0] = 0.0;
			rtk->ssat[i].snr[0] = 0;
        }
        for (i=0;i<n;i++) {
			rtk->ssat[obs[i].sat - 1].azel[0] = azel_[i * 2];
			rtk->ssat[obs[i].sat - 1].azel[1] = azel_[1 + i * 2];
			rtk->ssat[obs[i].sat - 1].snr[0] = obs[i].SNR[0];
            if (!vsat[i]) continue;
			rtk->ssat[obs[i].sat - 1].vs = 1;
			rtk->ssat[obs[i].sat - 1].resp[0] = resp[i];
        }
    }
    return stat;
}


