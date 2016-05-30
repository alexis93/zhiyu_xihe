
#include "rtklib.h"
#include "iwise_pos_module.h"

rtk_t rtk_temp;   //

static int intnav( nav_t *nav_temp, const nav_t *nav, const clockorbit_t * clockorbit)
{
	const double lam_glo[NFREQ]={CLIGHT/FREQ1_GLO,CLIGHT/FREQ2_GLO};
	int i=0;int j=0;int sys=0;int satnum=0;int prn_num=0;
	printf("intnav start\n");
	nav_temp->nt = 0;
	nav_temp->ntmax = MAXSAT;
	nav_temp->n=nav->n;
	nav_temp->nmax=nav->nmax;
	nav_temp->eph  =(eph_t *)malloc(sizeof(eph_t)* 60);
	nav_temp->alm  =(alm_t *)malloc(sizeof(alm_t)* 60);
	nav_temp->geph =(geph_t *)malloc(sizeof(geph_t)* 60);

	printf("1\n");

	for (i = 0; i<MAXSAT; i++){
		nav_temp->cbias[i][0] = 0; nav_temp->cbias[i][1] = 0; nav_temp->cbias[i][2] = 0;
	}
	nav_temp->n    = MAXSAT;
	nav_temp->na   = MAXSAT;
	nav_temp->ng   = NSATGLO;
	nav_temp->ns=NSATSBS*2;

	//for (i=0;i<NSATSBS*2;i++) raw->nav.seph [i]=seph0;
	for (i=0;i<MAXSAT;i++) for (j=0;j<NFREQ;j++) {
		if (!(sys=satsys(i+1,NULL))) continue;
		nav_temp->lam[i][j]=sys==SYS_GLO?lam_glo[j]:lam_carr[j];
	}
	printf("2\n");
	for(i=0;i<40;i++){
		prn_num=0;
		printf("2.0\n");
//		sys=satsys(nav->eph[i].sat, &prn_num);
//		satnum=satno(sys, nav->eph[i].sat);
		if(nav->eph==NULL){
			printf("ERROR\n");
		}
		//printf("%d\n",nav->eph[i].sat);
		satnum=nav->eph[i].sat;
		if(satnum==0){
			continue;
		}
		//printf("2.1\n");
		nav_temp->eph[i].A=nav->eph[i].A;
		//printf("2.2\n");
		nav_temp->eph[i].M0=nav->eph[i].M0;
		nav_temp->eph[i].OMG0=nav->eph[i].OMG0;
		nav_temp->eph[i].OMGd=nav->eph[i].OMGd;
		nav_temp->eph[i].cic=nav->eph[i].cic;
		nav_temp->eph[i].cis=nav->eph[i].cis;
		nav_temp->eph[i].code=nav->eph[i].code;
		nav_temp->eph[i].crc=nav->eph[i].crc;
		nav_temp->eph[i].cuc=nav->eph[i].cuc;
		nav_temp->eph[i].crs=nav->eph[i].crs;
		nav_temp->eph[i].cus=nav->eph[i].cus;
		nav_temp->eph[i].deln=nav->eph[i].deln;
		nav_temp->eph[i].e=nav->eph[i].e;
		nav_temp->eph[i].f0=nav->eph[i].f0;
		nav_temp->eph[i].f1=nav->eph[i].f1;
		nav_temp->eph[i].f2=nav->eph[i].f2;
		nav_temp->eph[i].fit=nav->eph[i].fit;
		nav_temp->eph[i].flag=nav->eph[i].flag;
		nav_temp->eph[i].i0=nav->eph[i].i0;
		nav_temp->eph[i].idot=nav->eph[i].idot;
		nav_temp->eph[i].iodc=nav->eph[i].iodc;
		nav_temp->eph[i].iode=nav->eph[i].iode;
		nav_temp->eph[i].omg=nav->eph[i].omg;
		nav_temp->eph[i].sat=nav->eph[i].sat;
		nav_temp->eph[i].sva=nav->eph[i].sva;
		nav_temp->eph[i].svh=nav->eph[i].svh;
		nav_temp->eph[i].tgd[0]=nav->eph[i].tgd[0];
		nav_temp->eph[i].tgd[1]=nav->eph[i].tgd[1];
		nav_temp->eph[i].tgd[2]=nav->eph[i].tgd[2];
		nav_temp->eph[i].tgd[3]=nav->eph[i].tgd[3];
		nav_temp->eph[i].toc=nav->eph[i].toc;
		nav_temp->eph[i].toe=nav->eph[i].toe;
		nav_temp->eph[i].toes=nav->eph[i].toes;
		nav_temp->eph[i].ttr=nav->eph[i].ttr;
		nav_temp->eph[i].week=nav->eph[i].week;

		nav_temp->ssr[satnum].deph[0]=clockorbit->Sat[satnum].Orbit.DeltaRadial;
		nav_temp->ssr[satnum].deph[1]=clockorbit->Sat[satnum].Orbit.DeltaAlongTrack;
		nav_temp->ssr[satnum].deph[2]=clockorbit->Sat[satnum].Orbit.DeltaCrossTrack;
		nav_temp->ssr[satnum].ddeph[0]=clockorbit->Sat[satnum].Orbit.DotDeltaRadial;
		nav_temp->ssr[satnum].ddeph[1]=clockorbit->Sat[satnum].Orbit.DotDeltaAlongTrack;
		nav_temp->ssr[satnum].ddeph[2]=clockorbit->Sat[satnum].Orbit.DotDeltaCrossTrack;
		nav_temp->ssr[satnum].dclk[0]=clockorbit->Sat[satnum].Clock.DeltaA0;
		nav_temp->ssr[satnum].dclk[1]=clockorbit->Sat[satnum].Clock.DeltaA1;
		nav_temp->ssr[satnum].dclk[2]=clockorbit->Sat[satnum].Clock.DeltaA2;
		nav_temp->ssr[satnum].t0[0].time=clockorbit->GPSEpochTime;
		nav_temp->ssr[satnum].t0[1].time=clockorbit->GPSEpochTime;
		nav_temp->ssr[satnum].t0[2].time=clockorbit->GPSEpochTime;
		nav_temp->ssr[satnum].t0[3].time=clockorbit->GPSEpochTime;
		nav_temp->ssr[satnum].t0[4].time=clockorbit->GPSEpochTime;
	}
	//printf("3\n");
	nav_temp->leaps=nav->leaps;
	for(i=0;i<MAXSAT;i++){
		for(j=0;j<NFREQ;j++){
			nav_temp->lam[i][j]=nav->lam[i][j];
		}
	}
	//printf("4\n");
	return 1;
}

static int initobs(obsd_t *obs_temp,const obs_tMEP *obs)
{
	int i=0;int j=0;int n=obs->n;
	for (i = 0; i < n; i++){
			obs_temp[i].sat = obs->data[i].sat;
			obs_temp[i].rcv = obs->data[i].rcv;
			obs_temp[i].time.sec = obs->data[i].time.sec;
			obs_temp[i].time.time = obs->data[i].time.time;
			for(j=0;j<NFREQ+NEXOBS;j++){
				obs_temp[i].D[j] = obs->data[i].D[j];
				obs_temp[i].L[j] = obs->data[i].L[j];
				obs_temp[i].P[j] = obs->data[i].P[j];
				obs_temp[i].SNR[j] = obs->data[i].SNR[j];
				obs_temp[i].code[j] = obs->data[i].code[j];
				obs_temp[i].LLI[j] = obs->data[i].LLI[j];
			}

	}
	return 0;
}

static int initrtk(const prcopt_t *opt, const sol_t *sol, const ssat_t *ssat)
{
	//rtk_temp.sol.posf = SOLF_ENU; /* solution format (SOLF_???) *///SOLF_NMEA  SOLF_ENU  SOLF_XYZ
	return 0;
}

static int solcopy(sol_t *sol)
{
	int i=0;
	sol->time    =   rtk_temp.sol.time    ;
	sol->age     =   rtk_temp.sol.age     ;
	sol->nav     =   rtk_temp.sol.nav     ;
	sol->ratio   =   rtk_temp.sol.ratio   ;
	sol->type    =   rtk_temp.sol.type    ;
	sol->stat    =   rtk_temp.sol.stat    ;
	sol->ns      =   rtk_temp.sol.ns      ;

	for(i=0;i<6;i++){
		sol->rr[i]      =  rtk_temp.sol.rr[i]   ;
		sol->qr[i]      =  rtk_temp.sol.qr[i]   ;
		sol->dtr[i]     =  rtk_temp.sol.dtr[i]  ;
		sol->pos[i]     =  rtk_temp.sol.pos[i]  ;
		sol->Bdms[i]    =  rtk_temp.sol.Bdms[i] ;
		sol->Ldms[i]    =  rtk_temp.sol.Ldms[i] ;
		sol->enu[i]     =  rtk_temp.sol.enu[i]  ;
	}
	return 0;
}

extern int pnt_pos_sol(const obs_tMEP *obs, const nav_t *nav, const prcopt_t *opt, const clockorbit_t * clockorbit,
	sol_t *sol, ssat_t *ssat, char *msg)
{
	int i = 0; int j = 0;
	nav_t nav_temp;
	obsd_t obs_temp[40];  //40 sats;
	char msg_temp[128] = "";
	//ssat_t ssat_ppp;
	int n = obs->n;
	printf("start\n");
	if(nav->eph==NULL){
		printf("ERROR\n");
		return -1;
	}
	//satlite navigation data , ssr data :copy
	intnav( &nav_temp, nav,clockorbit);
	printf("intnav\n");
	/* update carrier wave length */
	for (i = 0; i < MAXSAT; i++) for (j = 0; j < NFREQ; j++) {
		nav_temp.lam[i][j] = satwavelen(i + 1, j, nav);
	}
	//obs data copy
	if(obs->n<4){
		printf("obs <4\n");
		return 0;
	}
	initobs(obs_temp,obs);
	printf("initobs\n");
	//rtk  copy
	initrtk(opt, sol, ssat);
	printf("initrtk\n");
	//PPP resolution
	if (!ppp_pos_sol(obs_temp, n, &nav_temp, msg_temp, &rtk_temp)) {
		free(nav_temp.eph );
		free(nav_temp.alm );
		free(nav_temp.geph );
		nav_temp.eph  =NULL;
		nav_temp.alm  =NULL;
		nav_temp.geph =NULL;
		return 1;
	}
	printf("ppp_pos_sol\n");
	//soltion copy
	printf("%f  %f  %f\n",rtk_temp.sol.rr[0],rtk_temp.sol.rr[1],rtk_temp.sol.rr[2]);
	solcopy(sol);
	printf("solcopy\n");
	free(nav_temp.eph );
	free(nav_temp.alm );
	free(nav_temp.geph );
	nav_temp.eph  =NULL;
	nav_temp.alm  =NULL;
	nav_temp.geph =NULL;
	return 0;
}

extern int initopt(prcopt_t *prcopt_t)
{
	prcopt_t->sateph = EPHOPT_SSRAPC;   //EPHOPT_BRDC   EPHOPT_SSRAPC  EPHOPT_PREC
	prcopt_t->elmin = 10.0*D2R;     //
	prcopt_t->posopt[0] = 1;   //
	prcopt_t->niter = 5;   //
	prcopt_t->mode = PMODE_PPP_KINEMA;//PMODE_PPP_KINEMA PMODE_KINEMA PMODE_PPP_KINEMA PMODE_PPP_STATIC        /* positioning mode: single */
	prcopt_t->modear = ARMODE_PPPAR_ILS;    //ARMODE_PPPAR  ARMODE_PPPAR_ILS
	prcopt_t->soltype = 0;   /* solution type (0:forward,1:backward,2:combined) */
	prcopt_t->navsys = SYS_ALL/*SYS_GPS*/;      /* navigation system */
	prcopt_t->nf = 1;         /* number of frequencies (1:L1,2:L1+L2,3:L1+L2+L5) */
	prcopt_t->refpos = 1;
	prcopt_t->ionoopt = IONOOPT_EST;	//	IONOOPT_EST IONOOPT_BRDC
	prcopt_t->thresar[0] = 1.0;
	prcopt_t->tropopt = TROPOPT_EST;  /* troposphere option: Saastamoinen model */  //���������;
	prcopt_t->thresar[4] = 1.0;
	prcopt_t->dynamics = 1;   /* dynamics model (0:none,1:velociy,2:accel) *//* number of pos solution */

	prcopt_t->posopt[2] = 1;   //
	return 0;
}

extern int pnt_pos_init(const char* param)
{
	sol_t sol0={{0}};
	ambc_t ambc0={{{0}}};
	ssat_t ssat0={0};
	int i; int j;

	rtk_temp.sol=sol0;
	rtk_temp.opt = prcopt_default;
	initopt(&rtk_temp.opt);

	for (i=0;i<6;i++) rtk_temp.rb[i]=0.0;

	rtk_temp.nx=pppnx(&rtk_temp.opt);
	rtk_temp.na=0;
	rtk_temp.tt=0.0;
	for (i = 0; i < maxrows; i++) {
		rtk_temp.x[i]  = 0;
		rtk_temp.xa[i] = 0;
		for (j = 0; j < maxrows; j++){
			rtk_temp.P[i+j*maxrows]    = 0;
			rtk_temp.Pa[i + j*maxrows] = 0;
		}
	}
	rtk_temp.nfix=rtk_temp.neb=0;
	for (i=0;i<MAXSAT;i++) {
		rtk_temp.ambc[i]=ambc0;
		rtk_temp.ssat[i]=ssat0;
		for (j = 0; j < NFREQ; j++){
			rtk_temp.Ps[i][j] = 0;
			rtk_temp.Lp[i][j] = 0;
			rtk_temp.n[i][j]  = 0;
		}

	}
	for (i=0;i<MAXERRMSG;i++) rtk_temp.errbuf[i]=0;

	rtk_temp.previous = 0;  //added by yuan;
	rtk_temp.counter  = 0;
	rtk_temp.spp      = 0;
	for (i = 0; i < MAXSAT; i++){
		rtk_temp.sat_[i] = 0;
	}
	rtk_temp.detect   = 0;  //钟跳标记;
	for (i = 0; i < 20;i++){
		rtk_temp.vv[i] = 0;
	}
	rtk_temp.temp_ = 0;   //接收机钟跳;

	return 0;
}

extern int pnt_pos_destroy()
{

	return 1;
}


