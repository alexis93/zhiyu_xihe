/*-----------------------------------------------------------------
 *	clock_orbit_rtcm.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 
 *-------------------------------------------------------------------*/
#ifndef RTCM3_CLOCK_ORBIT_RTCM_H
#define RTCM3_CLOCK_ORBIT_RTCM_H

/* Programheader

        Name:           clock_orbit_rtcm.h
        Project:        RTCM3
        Version:        $Id: clock_orbit_rtcm.h,v 1.11 2009/05/14 13:24:13 stoecker Exp $
        Authors:        Dirk St枚cker
        Description:    state space approach for RTCM3
*/

#include <string.h>

enum COR_CONSTANTS {
  CLOCKORBIT_BUFFERSIZE=2048,
  CLOCKORBIT_NUMGPS=32,
  CLOCKORBIT_NUMGLONASS=24,
  CLOCKORBIT_NUMBIAS=10
};
enum SatelliteReferenceDatum { DATUM_ITRF=0, DATUM_LOCAL=1 };
enum SatelliteReferencePoint { POINT_IONOFREE=0, POINT_CENTER=1 };

//TODO lzg
/* enum definitions ----------------------------------------------------*/
enum ClockOrbitType { COTYPE_GPSORBIT=4050, COTYPE_GPSCLOCK=4051,
     COTYPE_GLONASSORBIT=4053, COTYPE_GLONASSCLOCK=4054,
     COTYPE_GPSCOMBINED=4056, COTYPE_GLONASSCOMBINED=4057,
     COTYPE_AUTO=0 };
enum BiasType { BTYPE_GPS=4052, BTYPE_GLONASS=4055, BTYPE_AUTO = 0 };



enum CodeType {
  CODETYPEGPS_L1_CA          = 0,
  CODETYPEGPS_L1_P           = 1,
  CODETYPEGPS_L1_Z           = 2,
  CODETYPEGLONASS_L1_CA      = 0,
  CODETYPEGLONASS_L1_P       = 1,
  CODETYPEGLONASS_L2_CA      = 2,
  CODETYPEGLONASS_L2_P       = 3,
};

enum GCOB_RETURN {
  /* all well */
  GCOBR_MESSAGEFOLLOWS = 1,
  GCOBR_OK = 0,
  /* unknown data, a warning */
  GCOBR_UNKNOWNTYPE = -1,
  GCOBR_UNKNOWNDATA = -2,
  GCOBR_CRCMISMATCH = -3,
  /* failed to do the work */
  GCOBR_NOCLOCKORBITPARAMETER = -10,
  GCOBR_NOBIASPARAMETER = -11,
  /* data mismatch - data in storage does not match new data */
  GCOBR_TIMEMISMATCH = -20,
  GCOBR_DATAMISMATCH = -21,
  /* not enough data - can decode the block completely */
  GCOBR_SHORTBUFFER = -30,
  GCOBR_MISSINGBITS = -31,
  GCOBR_MESSAGEEXCEEDSBUFFER = -32
};

/* type definitions ----------------------------------------------------------*/



/* GLONASS data is stored with offset CLOCKORBIT_NUMGPS in the data structures.
So first GLONASS satellite is at xxx->Sat[CLOCKORBIT_NUMGPS], first
GPS satellite is xxx->Sat[0]. */

struct Bias
{
  int GPSEpochTime;                 /* 0 .. 604799 s */
  int GLONASSEpochTime;             /* 0 .. 86399 s (86400 for leap second) */
  int NumberOfGPSSat;               /* 0 .. 32 */
  int NumberOfGLONASSSat;           /* 0 .. 24 */
  struct BiasSat
  {
    int ID; /* GPS or GLONASS */
    int NumberOfCodeBiases;

    struct CodeBias
    {
      enum CodeType Type;
      float         Bias;           /* m */
    } Biases[CLOCKORBIT_NUMBIAS];
  } Sat[CLOCKORBIT_NUMGPS+CLOCKORBIT_NUMGLONASS];

};

struct ClockOrbit
{
  int GPSEpochTime;                 /* 0 .. 604799 s */
  int GLONASSEpochTime;             /* 0 .. 86399 s (86400 for leap second) */
  int NumberOfGPSSat;               /* 0 .. 32 */
  int NumberOfGLONASSSat;           /* 0 .. 24 */
  int ClockDataSupplied;            /* boolean */
  int OrbitDataSupplied;            /* boolean */
  int epochGPS[101];                /* Weber, for latency */
  int epochSize;                    /* Weber, for latency */
  enum SatelliteReferencePoint SatRefPoint;
  enum SatelliteReferenceDatum SatRefDatum;
  struct SatData {
    int ID; /* GPS or GLONASS */
    int IOD; /* GPS or GLONASS */
    struct OrbitPart
    {
      double DeltaRadial;           /* m */
      double DeltaAlongTrack;       /* m */
      double DeltaCrossTrack;       /* m */
      double DotDeltaRadial;        /* m/s */
      double DotDeltaAlongTrack;    /* m/s */
      double DotDeltaCrossTrack;    /* m/s */
      double DotDotDeltaRadial;     /* m/ss */
      double DotDotDeltaAlongTrack; /* m/ss */
      double DotDotDeltaCrossTrack; /* m/ss */
    } Orbit;
    struct ClockPart
    {
      double DeltaA0;               /* m */
      double DeltaA1;               /* m/s */
      double DeltaA2;               /* m/ss */
    } Clock;
  } Sat[CLOCKORBIT_NUMGPS+CLOCKORBIT_NUMGLONASS];
};

//TODO lzg
/* clocorbit functions -------------------------------------------------------*/
/* return size of resulting data or 0 in case of an error */
extern size_t MakeClockOrbit(struct ClockOrbit *co, enum ClockOrbitType type,
       int moremessagesfollow, char *buffer, size_t size);
extern size_t MakeBias(const struct Bias *b, enum BiasType type,
       int moremessagesfollow, char *buffer, size_t size);
/*
 *NOTE: When an error message has been emitted, the output structures may
 * have been modified. Make a copy of the previous variant before calling 
 * the function to have a clean state.buffer should point to a RTCM3 block 
 */
extern enum GCOB_RETURN GetClockOrbitBias(struct ClockOrbit *co, struct Bias *b,
       const char *buffer, size_t size, int *bytesused);

#ifdef __cplusplus
}
#endif
#endif
