/*    This file is distributed with 42,                               */
/*    the (mostly harmless) spacecraft dynamics simulation            */
/*    created by Eric Stoneking of NASA Goddard Space Flight Center   */

/*    Copyright 2010 United States Government                         */
/*    as represented by the Administrator                             */
/*    of the National Aeronautics and Space Administration.           */

/*    No copyright is claimed in the United States                    */
/*    under Title 17, U.S. Code.                                      */

/*    All Other Rights Reserved.                                      */


#ifndef __ORBKIT_H__
#define __ORBKIT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iokit.h"
#include "mathkit.h"
#include "timekit.h"

/*
** #ifdef __cplusplus
** namespace Kit {
** #endif
*/

struct LagrangePointType {
   /*~ Internal Variables ~*/
   double PosN[3]; /* Pos wrt N frame of Body 1 (larger grav center), [[m]] */
   double VelN[3]; /* Vel wrt N frame of Body 1 (larger grav center), [[m/sec]] */
   double X0; /* Radial location wrt Body 1, in Synodic Frame, m */
   double Y0; /* Transversed location wrt Body 1, in Synodic Frame, m */
   double R1;
   double R2;
   double Kxx;
   double Kxy;
   double Kyy;
   double Zw1;
   double Zw2;
   double Zs;
   /* Modal frequencies (or time constant), rad/sec */
   double w1;
   double w2;
   double sigma;
   double wz; 
   
   double ca1;
   double sa1;
   double ca2;
   double sa2;
   /* Modal aspect ratios */
   double AR1;
   double AR2; 
};

struct LagrangeSystemType {
   /*~ Internal Variables ~*/
   long Exists;
   char Name[20];
   long Body1;
   long Body2;
   double mu1;
   double mu2;
   double rho;
   double SLR;
   double ecc;
   double inc;
   double RAAN;
   double ArgP;
   double tp;
   double SMA;
   double MeanRate;
   double Period;
   /* Nondimensional distance from m1 to m2, and derivatives */
   double D;
   double Ddot;
   double Ddotdot;  
   /* True anomaly, and derivatives */
   double th;
   double thdot;
   double thdotdot; 
   struct LagrangePointType LP[5];
   double CLH[3][3];
   double CLN[3][3];
};

/* Chebyshev coefficients.  Used for DE430 planetary ephemerides. */
struct Cheb3DType {
   /* Coefficients only valid for JD1 <= JulDay < JD2 */
   double JD1;
   double JD2;
   long N; /* Order <= 20 */
   double Coef[3][20];
};

struct OrbitType {
   /*~ Parameters ~*/

   long  Tag;   /* Orb[Tag].Tag = Tag */
   long  Exists;
   double Epoch;  /* Sec since J2000 epoch at which orbit elements are referenced */
   long  Regime;  /* ZERO, FLIGHT, CENTRAL (Two-body) or THREE_BODY */
   long  PolyhedronGravityEnabled;
   long  World;
   long Region;
   
   /* For Three-Body Orbit Description */
   long  Sys;  /* e.g. SUNEARTH, EARTHMOON, SUNJUPITER */
   long  LP;   /* Lagrange Point [0-4] */
   long  Body1;
   long  Body2;
   double mu1;
   double mu2;
   long LagDOF;
   /* Modal parameters, m */
   double Ax;
   double Bx;
   double Cx;
   double Dx;
   double Ay;
   double By;
   double Cy;
   double Dy;
   double Az;
   double Bz;  

   /* For Central Orbit Description */
   double mu;
   double SMA;  /* Semi-major axis [[m]] */
   double ecc;  /* Eccentricity */
   double inc;  /* Inclination, [[rad]] */
   double RAAN; /* Right Ascension of Ascending Node, [[rad]] */
   double ArgP; /* Argument of Periapsis, [[rad]] */
   double tp; /* Time of Periapsis Passage, [[sec]] since J2000 epoch */
   double alpha; /* 1/SMA.  Better behaved than SMA when e near 1.0 [[1/m]] */
   double SLR;     /* Semilatus rectum.  Always well behaved [[m]] */
   double rmin;  /* Periapsis radius.  Always well behaved [[m]] */
   double Period;
   double MeanMotion;
   char FileName[20];
   long J2DriftEnabled;
   double MuPlusJ2; /* Effective mu, adjusted for average J2 effect */
   /* Initial values */
   double RAAN0;
   double ArgP0; 
   double RAANdot; /* Due to average J2 effect, rad/sec */
   double ArgPdot; /* Due to average J2 effect, rad/sec */
   double J2Fr0; /* Orbit-average radial acceleration due to J2 */
   double J2Fh1; /* Coefficient for orbit-rate normal acceleration due to J2 */ 
   FILE *SplineFile;

   /*~ Internal Variables ~*/
   /* Linearized three-body motion about LP (X0,Y0), m and m/sec */
   double x;
   double y;
   double z;
   double xdot;
   double ydot;
   double zdot;  
   
   /* For Central Orbit Description */
   double anom; /* True Anomaly, rad */
   double PosN[3]; /* Position, [[m]], expressed in N [~=~] */
   double VelN[3]; /* Velocity, [[m/sec]], expressed in N [~=~] */
   double CLN[3][3]; /* For ZERO, L = N.  For FLIGHT, L = ENU.  For CENTRAL, L = LVLH.  For THREE_BODY, L = XYZ */
   double wln[3]; /* Expressed in N */
   /* Fit spline to data file */
   double NodeAbsTime[4];
   double NodePos[4][3];
   double NodeVel[4][3];
   /* Chebyshev Coefficients */
   long Ncheb;
   struct Cheb3DType *Cheb;
};

/*~ Prototypes ~*/
struct OrbitType *CloneOrbit(struct OrbitType *OldOrb, long *Norb,
   long Iorb);
double TrueAnomaly(double mu, double p, double e, double t);
double atanh(double x);
double TimeSincePeriapsis(double mu, double p, double e, double th);
void RV02RV(double mu, double xr0[3], double xv0[3], double anom,
            double xr[3], double xv[3]);
void Eph2RV(double mu, double p, double e, double i, double RAAN,
            double ArgP,double dt, double r[3], double v[3], double *anom);
void  RV2Eph(double time, double mu, double xr[3], double xv[3],
             double *SMA, double *e, double *i, double *RAAN,
             double *ArgP, double *th, double *tp,
             double *SLR, double *alpha, double *rmin,
             double *MeanMotion, double *Period);
void TLE2Eph(const char Line1[80], const char Line2[80], double JD,
   double mu, double *SMA, double *e, double *i, double *RAAN,
   double *ArgP, double *th, double *tp, double *SLR,
   double *alpha, double *rmin, double *Period, double *MeanMotion);
long LoadTleFromFile(const char *Path, const char *TleFileName, const char *TleLabel,
                      double JD, double mu, struct OrbitType *O);
double RV2RVp(double mu, double r[3], double v[3], double rp[3], double vp[3]);
void PlanetEphemerides(long i, double JD, double mu,
       double *SMA, double *ecc, double *inc, double *RAAN, double *omg,
       double *tp, double *anom, double *p, double *alpha, double *rmin,
       double *MeanMotion, double *Period);
void LunaPosition(double JD, double r[3]);
void LunaInertialFrame(double JulDay, double CNJ[3][3]);
double LunaPriMerAng(double JulDay);
void FindCLN(double r[3], double v[3], double CLN[3][3], double wln[3]);
void FindENU(double PosN[3],double WorldW, double CLN[3][3], double wln[3]);
void FindLagPtParms(struct LagrangeSystemType *LS);
void FindLagPtPosVel(double SecSinceJ2000, struct LagrangeSystemType *S,
   long Ilp, double PosN[3], double VelN[3], double CLN[3][3]);
void LagModes2RV(double SecSinceJ2000, struct LagrangeSystemType *LS,
                 struct OrbitType *O, double r[3], double v[3]);
void RV2LagModes(double SecSinceJ2000, struct LagrangeSystemType *LS,
                 struct OrbitType *O);
void R2StableLagMode(double SecSinceJ2000, struct LagrangeSystemType *LS,
                 struct OrbitType *O);
void XYZ2LagModes(double TimeSinceEpoch, struct LagrangeSystemType *LS,
                 struct OrbitType *O);
void AmpPhase2LagModes(double TimeSinceEpoch, double AmpXY1,
   double PhiXY1, double SenseXY1, double AmpXY2, double PhiXY2,
   double SenseXY2, double AmpZ, double PhiZ,
   struct LagrangeSystemType *S, struct OrbitType *O);
void TDRSPosVel(double PriMerAng,double TIME,
                double ptn[10][3], double vtn[10][3]);
void TETE2J2000(double JD,double CTJ[3][3]);
double RadiusOfInfluence(double mu1, double mu2, double r);
void RelRV2EHRV(double OrbRadius, double OrbRate, double OrbCLN[3][3],
             double Rrel[3], double Vrel[3], double re[3], double ve[3]);
void EHRV2RelRV(double OrbRadius, double OrbRate, double OrbCLN[3][3],
   double re[3], double ve[3], double Rrel[3], double Vrel[3]);
void EHRV2EHModes(double r[3], double v[3], double n, double nt,
                  double *A, double *Bc, double *Bs, double *C,
                  double *Dc, double *Ds);
void EHModes2EHRV(double A, double Bc, double Bs, double C,
                  double Dc, double Ds, double n, double nt,
                  double r[3], double v[3]);
double LambertTOF(double mu, double amin, double lambda, double x);
void LambertProblem(double t0, double mu, double xr1[3], double xr2[3],
   double TOF,double TransferType,double *SLR, double *e, double *inc,
   double *RAAN, double *ArgP, double *tp);
double RendezvousCostFunction(double *InVec, double *AuxVec);
void PlanTwoImpulseRendezvous(double mu, double r1e[3], double v1e[3],
   double r2e[3], double v2e[3], double *t1, double *t2,
   double DV1[3], double DV2[3]);
void FindLightLagOffsets(double AbsTime, struct OrbitType *Observer,
   struct OrbitType *Target, double PastPos[3], double FuturePos[3]);
void FindJ2DriftParms(double mu, double J2, double Rw, struct OrbitType *O);

/*
** #ifdef __cplusplus
** }
** #endif
*/

#endif /* __ORBKIT_H__ */
