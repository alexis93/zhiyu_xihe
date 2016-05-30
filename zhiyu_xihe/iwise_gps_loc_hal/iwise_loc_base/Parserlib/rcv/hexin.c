/*------------------------------------------------------------------------------
* ublox.c : ublox receiver dependent functions
*
*          Copyright (C) 2007-2013 by T.TAKASU, All rights reserved.
*
* reference :
*     [1] ublox-AG, GPS.G3-X-03002-D, ANTARIS Positioning Engine NMEA and UBX
*         Protocol Specification, Version 5.00, 2003
*
* version : $Revision: 1.2 $ $Date: 2008/07/14 00:05:05 $
* history : 2007/10/08 1.0 new
*           2008/06/16 1.1 separate common functions to rcvcmn.c
*           2009/04/01 1.2 add range check of prn number
*           2009/04/10 1.3 refactored
*           2009/09/25 1.4 add function gen_ubx()
*           2010/01/17 1.5 add time tag adjustment option -tadj sec
*           2010/10/31 1.6 fix bug on playback disabled for raw data (2.4.0_p9)
*           2011/05/27 1.7 add almanac decoding
*                          add -EPHALL option
*                          fix problem with ARM compiler
*           2013/02/23 1.8 fix memory access violation problem on arm
*                          change options -tadj to -TADJ, -invcp to -INVCP
*-----------------------------------------------------------------------------*/
#include "../rtklib.h"
#include "../../iwise_loc_log.h"

#define HEXINSYNC1    '#'        /* hexin message sync code 1 */
#define HEXINSYNCS    'S'        /* hexin message sync code S */
#define HEXINSYNCM    'M'        /* hexin message sync code M */
#define HEXINSYNCE    'E'        /* hexin message sync code E */
#define UBXCFG      0x06        /* ubx message cfg-??? */

#define ID_RXMRAW   0x0210      /* ubx message id: raw measurement data */
#define ID_RXMSFRB  0x0211      /* ubx message id: subframe buffer */
#define FU1         1           /* ubx message field types */
#define FU2         2
#define FU4         3
#define FI1         4
#define FI2         5
#define FI4         6
#define FR4         7
#define FR8         8
#define FS32        9

static const char rcsid[]="$Id: ublox.c,v 1.2 2008/07/14 00:05:05 TTAKA Exp $";

/* get fields (little-endian) ------------------------------------------------*/
#define U1(p) (*((unsigned char *)(p)))
#define I1(p) (*((char *)(p)))
static unsigned short U2(unsigned char *p) {unsigned short u; memcpy(&u,p,2); return u;}
static unsigned int   U4(unsigned char *p) {unsigned int   u; memcpy(&u,p,4); return u;}
static float          R4(unsigned char *p) {float          r; memcpy(&r,p,4); return r;}
static double         R8(unsigned char *p) {double         r; memcpy(&r,p,8); return r;}

/* set fields (little-endian) ------------------------------------------------*/
static void setU1(unsigned char *p, unsigned char  u) {*p=u;}
static void setU2(unsigned char *p, unsigned short u) {memcpy(p,&u,2);}
static void setU4(unsigned char *p, unsigned int   u) {memcpy(p,&u,4);}
static void setI1(unsigned char *p, char           i) {*p=(unsigned char)i;}
static void setI2(unsigned char *p, short          i) {memcpy(p,&i,2);}
static void setI4(unsigned char *p, int            i) {memcpy(p,&i,4);}
static void setR4(unsigned char *p, float          r) {memcpy(p,&r,4);}
static void setR8(unsigned char *p, double         r) {memcpy(p,&r,8);}

static int has_obs_used = 0; //判断obs有没有使用过，若已经使用则置其为1, 若没有使用则置为0
static int obs_index = 0; //obs数组下标
static int obsd_cnt = 0;  //obs中储存obsd的个数 
static unsigned int msg_num = 0;	/* 本次观测时刻RAWMSR消息总数 */
static int last_obs_week = 0;     /* 上一次obsd获取的时间 week */
static int last_obs_time = 0;     /* 上一次obsd获取的时间 time */
static int parser_obs_flag = 0;  /* 是否开始解析并填充新obsd的标志，没有开始或重新解析下一个新的obs置为0， 开始解析置为1 */

/* 二维数组存储分解后的数据 */
#define FIELD_SIZE		32
static const char Fields[32][FIELD_SIZE];

/* 分离数据字段 并填充到二维数组中 */
static int extract_raw_field(unsigned char *buff, char *fields[])
{
	int i,j;
	unsigned char *p = buff;
	char (*fieldp)[FIELD_SIZE] = (char (*)[FIELD_SIZE])fields;

	i = 0;
	for(;;) {
		for(j = 0; *p != ',' && *p != '\n' && *p != '\0'; p++) {
			fieldp[i][j] = *p;
			j++;
		}
		fieldp[i++][j] = '\0';
		if(*p == '\0' || *p == '\n')
			break;

		p++;
	}

	return i;
}

int print_obs(raw_t *raw, int count)
{
	 printf("sat %d\t rcv %d\n", raw->obs.data[count-1].sat, raw->obs.data[count-1].rcv);
	 printf("GPST (%ld  %lf)\n", raw->obs.data[count-1].time.time, raw->obs.data[count-1].time.sec);
	 printf("SNR %\d LLI %d\n", raw->obs.data[count-1].SNR[0], raw->obs.data[count-1].LLI[0]);
	 printf("code %d\n", raw->obs.data[count-1].code[0]);
	 printf("P %lf\t L %lf  D %f\n", raw->obs.data[count-1].P[0], raw->obs.data[count-1].L[0], raw->obs.data[count-1].D[0]);
	 return 0;
}

/* 填充obsd*/
static int fill_obsd(raw_t *raw, char *field[], int index)
{
    unsigned int time, svid, week; /* 周内秒(ms)，卫星号, 周计数值 */
	double psr, adr, dopp;	/* 伪距观测值(m)， 累积载波相位(cycle), 多普勒频率(HZ)*/
	unsigned int cn0, locktime;	/* 载噪比(dB-HZ), 持续跟踪时间(ms) */

	char (*fieldp)[FIELD_SIZE] = (char (*)[FIELD_SIZE])field;
	
    week = atoi(fieldp[3]); /*当前消息的week*/               
	time = atoi(fieldp[4]); /*当前消息的tow（周秒）*/
	svid = atoi(fieldp[5]); /* 当前卫星号*/
	psr = atof(fieldp[8]); /* 伪距观测值 */
	adr = atof(fieldp[9]); /* 载波相位 */
	dopp = atof(fieldp[10]); /* 多普勒频率 */
    //dopp = strtof(fieldp[10], NULL);
	cn0 = atoi(fieldp[11]); /* 噪声比 */
	locktime = atoi(fieldp[12]);

    raw->obs.data[index].L[0] = adr;
    raw->obs.data[index].P[0] = psr;
    raw->obs.data[index].D[0] = (float)dopp;
    raw->obs.data[index].SNR[0] = cn0 * 4 ; //修改cn0 * 4
    raw->obs.data[index].sat = svid;
    //raw->obs.data[obs_index].LLI[0] = locktime;
    raw->obs.data[index].LLI[0] = 0;
    raw->obs.data[index].rcv = index;
    raw->obs.data[index].code[0] = CODE_L1C;  //根据ublox填充的

    if(svid <= 37) {
        raw->obs.data[index].time = gpst2time(week,time*0.001);/*时间系统是gpst*/
    } else if(svid >= 160) {
		raw->obs.data[index].time = bdt2gpst(bdt2time(week,time*0.001));/*时间系统是bdt*/
	}
    
    return 0;

}
/* 填充obs字段*/
/* 由于和心星通的观测值是多条消息组成的，在填充obsd的时候应该从第一条消息开始填充 ，当填充满后在返回*/
static int fill_obs(raw_t *raw, char *field[], int count)
{
	unsigned int msg_cnt = 0;	/* 本次观测时刻RAWMSR消息编号 */
	unsigned int quality,time, svid, freq, week; /* 观测值质量，周内秒(ms)，卫星号，频点号，周计数值 */
	
    char (*fieldp)[FIELD_SIZE] = (char (*)[FIELD_SIZE])field;
	msg_num = atoi(fieldp[1]); /*消息总数*/
	msg_cnt = atoi(fieldp[2]); /*当前消息编号*/
	week = atoi(fieldp[3]); /*当前消息的week*/               
	time = atoi(fieldp[4]); /*当前消息的tow（周秒）*/
	freq = atoi(fieldp[6]); /* 观测系统0:为gps; 34:为Beidou*/
	quality = atoi(fieldp[7]); /* 卫星观测值的质量 */

    if(parser_obs_flag == 0) /* 是否开始解析并填充新的obs, 并获取当前新的obs的时间*/ 
    {
        if(quality != 0 && freq == 0)    /* 假如观测值有效 同时观测值的gps系统的，quality = 0为观测值质量,*/
        {
            last_obs_week = week; /*保留同一时刻,观测值的week */
            last_obs_time = time; /*保留同一时刻, 观测值的time */
            fill_obsd(raw, field, obs_index);
            has_obs_used = 0;
            obsd_cnt++;
            obs_index++;
            parser_obs_flag = 1;
            if((msg_num == msg_cnt) && (has_obs_used == 0))
            {
                raw->obs.n = obsd_cnt;/* obs实际填充卫星观测值的个数 */
                raw->obs.nmax = msg_num; /* obs应该填充的卫星观测值的个数， 可能因卫星观测值质量差，被过滤掉 */
                parser_obs_flag = 0;
                obs_index = 0;
                obsd_cnt = 0;
                has_obs_used = 1; 
                return 1; /* obs完整获得， 返回1 */
            }
        }
        return 0;
	}

    if(parser_obs_flag == 1)/* 已经开始解析并填充新的obs, 根据时间来填充新的obsd */ 
    {
        if((last_obs_week == week) && (last_obs_time == time)) //判断上一个obsd的时间和当前obsd的时间是否相同
        {
            if(quality != 0 && freq == 0)    /* 假如观测值有效 同时观测值的gps系统的，quality = 0为观测值质量,*/
            {
                fill_obsd(raw, field, obs_index);
                has_obs_used = 0;
                parser_obs_flag = 1;
                obsd_cnt++;
                obs_index++;
                if((msg_num == msg_cnt) && (has_obs_used == 0)) /* 若obs的纤细总数等于消息编号， 且obs没有使用过，则返回当前obs*/
                {
					raw->obs.n = obsd_cnt;/* obs实际填充卫星观测值的个数 */
					raw->obs.nmax = msg_num; /* obs应该填充的卫星观测值的个数， 可能因卫星观测值质量差，被过滤掉 */
                    parser_obs_flag = 0;
                    obs_index = 0;
                    obsd_cnt = 0;
                    has_obs_used = 1;
                    return 1; /* obs完整获得， 返回1 */
                }
            }
        } 
    }
    return 0;
}


static int decode_rawmsr_ex(raw_t *raw)
{
	if(has_obs_used == 0) { /* 若obs没有使用过， 则返回当前obs*/
		raw->obs.n = obsd_cnt;/* obs实际填充卫星观测值的个数 */
        raw->obs.nmax = msg_num; /* obs应该填充的卫星观测值的个数， 可能因卫星观测值质量差，被过滤掉 */
        parser_obs_flag = 0;
		obs_index = 0;
		obsd_cnt = 0;
		has_obs_used = 1; /* 若obs完整获得， 则返回1*/
		return 1;
	}
	return 5; /* 该返回值必须在1-9之间，但是却不能为1.2.3.9等值 */
}

/* decode rawmsr 解析和芯星通的原始数据 */
static int decode_rawmsr(raw_t *raw)
{
	int ret = 0;
	int count = 0;
    int i = 0;

	count = extract_raw_field(raw->buff, Fields);
	ret = fill_obs(raw, Fields, count);

	return ret;
}


/* checksum ------------------------------------------------------------------*/
static int checksum(unsigned char *buff, int len)
{
    unsigned char cka=0,ckb=0;
    int i;
    
    for (i=2;i<len-2;i++) {
        cka+=buff[i]; ckb+=cka;
    }
    return cka==buff[len-2]&&ckb==buff[len-1];
}

static void setcs(unsigned char *buff, int len)
{
    unsigned char cka=0,ckb=0;
    int i;
    
    for (i=2;i<len-2;i++) {
        cka+=buff[i]; ckb+=cka;
    }
    buff[len-2]=cka;
    buff[len-1]=ckb;
}


/* save subframe -------------------------------------------------------------*/
//TODO 这个函数需要修改，和芯星通的格式可能与ublox不同，未仔细检查
static int save_subfrm(int sat, raw_t *raw)
{
    unsigned char *p=raw->buff+3,*q;
    int i,j,n,id=(U4(p+5)>>2)&0x7;

    trace(4,"save_subfrm: sat=%2d id=%d\n",sat,id);
    
    if (id<1||5<id) return 0;
    
    q=raw->subfrm[sat-1]+(id-1)*30;
    
    for (i=n=0,p+=1;i<10;i++,p+=4) {
        for (j=23;j>=0;j--) {
            *q=(*q<<1)+((U4(p)>>j)&1); if (++n%8==0) q++;
        }
    }
    return id;
}
/* decode ephemeris ----------------------------------------------------------*/
static int decode_ephem(int sat, raw_t *raw)
{
    eph_t eph={0};
 
    trace(4,"decode_ephem: sat=%2d\n",sat);
    
    if (decode_frame(raw->subfrm[sat-1]   ,&eph,NULL,NULL,NULL,NULL)!=1||
        decode_frame(raw->subfrm[sat-1]+30,&eph,NULL,NULL,NULL,NULL)!=2||
        decode_frame(raw->subfrm[sat-1]+60,&eph,NULL,NULL,NULL,NULL)!=3) return 0;
    
    if (!strstr(raw->opt,"-EPHALL")) {
        if (eph.iode==raw->nav.eph[sat-1].iode) {
        	return 0; /* unchanged */
        }
    }
    eph.sat=sat;
    raw->nav.eph[sat-1]=eph;
    raw->ephsat=sat;
    return 2;
}

/* decode almanac and ion/utc ------------------------------------------------*/
static int decode_alm1(int sat, raw_t *raw)
{
    trace(4,"decode_alm1 : sat=%2d\n",sat);
    decode_frame(raw->subfrm[sat-1]+90,NULL,raw->nav.alm,raw->nav.ion_gps,
                 raw->nav.utc_gps,&raw->nav.leaps);
    return 0;
}

/* decode almanac ------------------------------------------------------------*/
static int decode_alm2(int sat, raw_t *raw)
{
    trace(4,"decode_alm2 : sat=%2d\n",sat);
    decode_frame(raw->subfrm[sat-1]+120,NULL,raw->nav.alm,NULL,NULL,NULL);
    return  0;
}
/* decode ublox rxm-sfrb: subframe buffer ------------------------------------*/
static int decode_rxmsfrb(raw_t *raw)
{
	
    unsigned int words[10];
    int i,prn,sat,sys,id;
    
    trace(4,"decode_rxmsfrb: len=%d\n",raw->len);
    sat=raw->buff[3];
    //TODO 需要增加卫星号有效性检查
    
    //sys=satsys(sat,&prn);
    if(sat <= 37) {
    	sys = SYS_GPS;
    } else {
    	sys = SYS_NONE;
    }
    
    if (sys==SYS_GPS) {
        id=save_subfrm(sat,raw); //TODO 这里需要修改，要跟据和芯星通的格式修改, ??
        if (id==3) return decode_ephem(sat,raw);
        if (id==4) return decode_alm1 (sat,raw);
        if (id==5) return decode_alm2 (sat,raw);
        return 0;
    }
    
    return 0;
}
/* decode ublox raw message --------------------------------------------------*/
static int decode_hexin(raw_t *raw)
{
    int type=raw->buff[1];
    
    trace(3,"decode_ubx: type=%04x len=%d\n",type,raw->len);
    
    /* checksum */
    /*
    if (!checksum(raw->buff,raw->len)) {
        trace(2,"ubx checksum error: type=%04x len=%d\n",type,raw->len);
        return -1;
    }
    */
    //TODO raw->outtype 不知道有什么作用？
    if (raw->outtype) {
        sprintf(raw->msgtype,"HEXIN 0x%04X (%4d):",type,raw->len);
    }
    switch (type) {
    	case HEXINSYNCE : return decode_rawmsr_ex(raw);
        case HEXINSYNCM : return decode_rawmsr(raw);
        case HEXINSYNCS : return decode_rxmsfrb(raw);

    }
    return 0;
}
/* sync code -----------------------------------------------------------------*/
static uint8_t sync_hexin(unsigned char *buff, unsigned char data)
{
    buff[0]=buff[1]; buff[1]=data;
    if (buff[0]==HEXINSYNC1&&buff[1]==HEXINSYNCS)
    {
    	return HEXINSYNCS;
    }
    else if (buff[0]==HEXINSYNC1&&buff[1]==HEXINSYNCM)
    {
    	return HEXINSYNCM;
    } else if(buff[0]==HEXINSYNC1&&buff[1]==HEXINSYNCE) {
		return HEXINSYNCE;
	}	
    else
    {
    	return 0;
    }

}
/* input ublox raw message from stream -----------------------------------------
* fetch next ublox raw data and input a mesasge from stream
* args   : raw_t *raw   IO     receiver raw data control struct
*          unsigned char data I stream data (1 byte)
* return : status (-1: error message, 0: no message, 1: input observation data,
*                  2: input ephemeris, 3: input sbas message,
*                  9: input ion/utc parameter)
*
* notes  : to specify input options, set raw->opt to the following option
*          strings separated by spaces.
*
*          -EPHALL    : input all ephemerides
*          -INVCP     : invert polarity of carrier-phase
*          -TADJ=tint : adjust time tags to multiples of tint (sec)
*
*-----------------------------------------------------------------------------*/
extern int input_hexin(raw_t *raw, unsigned char data)
{
	uint8_t type;
	trace(5,"input_hexin: data=%02x\n",data);


    /* synchronize frame */
    if (raw->nbyte==0) {
		type=sync_hexin(raw->buff,data);
		if (type==HEXINSYNCS) {
			raw->buff[0]='#';
			raw->buff[1]='S';
			raw->nbyte=2;
        return 0;
        }
       else if (type==HEXINSYNCM) {
    	   	raw->buff[0]='#';
			raw->buff[1]='M';
			raw->nbyte=2;
		return 0;
        }
       else if (type==HEXINSYNCE) {
			raw->buff[0]='#';
			raw->buff[1]='E';
			raw->nbyte=2;
       		return 0;
        }
       else
        {
        	return 0;
        }
    }

   raw->buff[raw->nbyte++]=data;

	/* 获取包的长度 */
    if (raw->nbyte==3) {
        if ((raw->len=raw->buff[2]+3)>MAXRAWLEN) {
            trace(2,"hexin length error: len=%d\n",raw->len);
            raw->nbyte=0;
            return -1;
        }
    }
    if (raw->nbyte<3||raw->nbyte<raw->len) return 0;
    raw->nbyte=0;
    
    /* decode ublox raw message */
    return decode_hexin(raw);
}
