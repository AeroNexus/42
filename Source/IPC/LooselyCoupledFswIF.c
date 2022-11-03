/*************************************************************************
*
*   LooselyCoupledFswIF.c
*   Authored by Ken Center, Orbit Logic, Inc.
*
*   Contributed to the GitHub Open Source project /ericstoneking/42
*
*   - Facilitates asynchronous injection of fsw commands
*   - Facilitate delivery of sim states to external UDP peers (servers)
*
*   This is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*************************************************************************/

#include "42.h"
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

// Function Prototypes
long FswCmdInterpreter(char CmdLine[512], double *CmdTime); // External, in 42fsw.c
int ConstructTimeMessage(float simTimeRatio, char* Msg);
int ConstructStateMessage(int Isc, char* Msg);
int ConstructAttModeMessage(int Isc, char* Msg);
int AdcsModeTransitionDetected(int Isc);
int ConstructTimeMessage_verbose(float simTimeRatio, char* Msg);
int ConstructStateMessage_verbose(int Isc, char* Msg);
int ConstructStateMessage_orig(int Isc, char* Msg);

// Constants used in local code execution
#define MAX_SC 300

const int BUF_SIZE = 16384;
const int CMD_SIZE = 256;
const int RT_Port = 6789;
const int Fmt_RT_Port = 5678;

const double GPS_EPOCH_OFFSET = 630763269;
const int TIME_MSG_ID = 1;
const int SC_STATE_MSG_ID = 2;
const int ATT_MODE_MSG_ID = 3;

// Attitude modes
#define ATT_MODE_IDLE 1
#define ATT_MODE_COMMANDED 2
#define ATT_MODE_COAST 3
#define ATT_MODE_CONVERGED 5

const double TIME_SYNC_TOLERANCE = 10.0; // Time can be off this much(seconds)from sync source
const double ATT_CONVERGED_TOL = 0.025;   // Criteria for attitude convergence (radians)
const double RATE_CONVERGED_TOL = 0.0025; // Criteria for attitude rate converngence (radians/sec)

// Global variables (in source scope)
struct {
	int last;
	int current;
	int count;
} attitudeMode[MAX_SC];

  
/**********************************************************************/
void FieldRealTimeCommands(void) // UDP server handling multiple clients sending fsw commands in 42 format (minus leading timestamp)
/**********************************************************************/
{
	// Two servers running
	// 1) On listening port defined by RT_Port: For receiving commands in the native 42 format (minus the timestamp)
	// 2) On listening port defined by Fmt_RT_Port: For receiving commands in a more structured format (CmdID followed by comma-separated arguments)
	//
    char Msg[BUF_SIZE];
	char CmdMsg[CMD_SIZE];
	static SOCKET RtCmdSock = 0;
	static SOCKET FmtRtCmdSock = 0;
	static int first = TRUE;
	int nBytes = 0;
	double cmdTime;
	long cmdResult = FALSE;
	
	struct sockaddr_in servaddr, cliaddr; 
	socklen_t addrLen;
      	
	if(first)
	{	
		printf("Initializing read socket for RT Cmds...\n");    // Creating socket file descriptor 
		if ( (RtCmdSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
			perror("socket creation failed"); 
			exit(EXIT_FAILURE); 
		} 
		  
		memset(&servaddr, 0, sizeof(servaddr)); 
		memset(&cliaddr, 0, sizeof(cliaddr)); 
		  
		// Filling server information 
		servaddr.sin_family    = AF_INET; // IPv4 
		servaddr.sin_addr.s_addr = INADDR_ANY; 
		servaddr.sin_port = htons(RT_Port); 
		
		// Make the listener non-blocking (using a timeout)
		struct timeval read_timeout;
		read_timeout.tv_sec = 0;
		read_timeout.tv_usec = 10;
		setsockopt(RtCmdSock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
		  
		// Bind the socket with the server address 
		if ( bind(RtCmdSock, (const struct sockaddr *)&servaddr,  
				sizeof(servaddr)) < 0 ) 
		{ 
			perror("bind failed"); 
			exit(EXIT_FAILURE); 
		} 
		printf("-> Opened SOCKET %d...\n",RtCmdSock);
		
		// Open a second listening port - specifically for formatted commands - not in the native 42 format
		
		printf("Initializing read socket for Formatted RT Cmds...\n");    // Creating socket file descriptor 
		if ( (FmtRtCmdSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
			perror("socket creation failed"); 
			exit(EXIT_FAILURE); 
		} 
						  
		// Filling server information 
		servaddr.sin_port = htons(Fmt_RT_Port); 
		
		// Make the listener non-blocking (using a timeout)
		setsockopt(FmtRtCmdSock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
		  
		// Bind the socket with the server address 
		if ( bind(FmtRtCmdSock, (const struct sockaddr *)&servaddr,  
				sizeof(servaddr)) < 0 ) 
		{ 
			perror("bind failed"); 
			exit(EXIT_FAILURE); 
		} 
		printf("-> Opened SOCKET %d...\n",FmtRtCmdSock);
		
		
		first = FALSE;
	}
	else
	{
		//printf("Scanning socket for RT Cmds...\n");	
		if(RtCmdSock>=0) 
		{
			memset(Msg,'\0',BUF_SIZE);
			nBytes = recvfrom(RtCmdSock, (char *)Msg, BUF_SIZE,  
							MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
							&addrLen); 
			//printf("From socket %d: Return value = %d\n",RtCmdSock, nBytes);
			if (nBytes>0)
			{
				printf("%d Bytes read, Cargo: %s\n",nBytes, (char*)&Msg[0]);
				// Take the raw command buffer (without simtime) and prepend the current sim time
				sprintf((char*)&CmdMsg[0], "%f %s",SimTime, (char*)&Msg[0]);
				
				// Forward command to interpreter
				cmdResult = FswCmdInterpreter(&CmdMsg[0],&cmdTime);

				if(cmdResult)
				{
					printf("CMD_OK\n");
				}
				else
				{
					printf("CMD_REJECTED\n");
				}
					
			}
			else
			{
				//printf("NO DATA\n");
			}
			
		}
		
		int ScId = 0;
		int CmdId = 0;
		double Qi = 0;
		double Qj = 0;
		double Qk = 0;
		double Qs = 0;
		double NewSimTime = 0; // GPS Epoch
		#define CMD_ID_SET_LVLH_QUAT  100
		#define CMD_ID_SET_SIM_TIME   101
		
		//printf("Scanning socket for Formatted RT Cmds...\n");	
		if(FmtRtCmdSock>=0) 
		{
			memset(Msg,'\0',BUF_SIZE);
			nBytes = recvfrom(FmtRtCmdSock, (char *)Msg, BUF_SIZE,  
							MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
							&addrLen); 
			//printf("From socket %d: Return value = %d\n",FmtRtCmdSock, nBytes);
			if (nBytes>0)
			{
				printf("%d Bytes read, Cargo: %s\n",nBytes, (char*)&Msg[0]);
				
				// Process the message and reformat for 42
				sscanf((char*)Msg, "%d", &CmdId);
				
				int knownCmd = FALSE;
				switch (CmdId) // Switch on the Command Identifier field, handle each case appropriately
				{
					case CMD_ID_SET_LVLH_QUAT:
						printf("Command -> SET_LVLH_ATT_QUATERNION\n");
						sscanf((char*)Msg, "%d,%d,%lf,%lf,%lf,%lf", &CmdId, &ScId, &Qi, &Qj, &Qk, &Qs);
						if( isnan(Qi) || isnan(Qj) || isnan(Qk) || isnan (Qs))
						{
							printf("NaN values detected... ignoring command.\n");	
							break;							
						}
						sprintf((char*)&CmdMsg[0], "%f SC[%d] qrl = [%4.3lf %4.3lf %4.3lf %4.3lf]",SimTime, ScId, Qi, Qj, Qk, Qs);
						printf("Reformatted as 42 command -> %s\n", (char*)&CmdMsg);
						knownCmd = TRUE; // Command is known and validated
						attitudeMode[ScId].current = ATT_MODE_COMMANDED; // Transition mode for this S/C to COAST
						attitudeMode[ScId].count = 0;
					break;
					case CMD_ID_SET_SIM_TIME:
						printf("Command -> CMD_ID_SET_SIM_TIME\n");
						sscanf((char*)Msg, "%d,%lf", &CmdId, &NewSimTime);
						printf("    Time Value is: %lf\n", NewSimTime);
						// Only sync time if needed (not within tolerance)
						double timeDelta = abs(NewSimTime-SimTime);
						if( timeDelta > TIME_SYNC_TOLERANCE)
						{
							printf("KC Test: Time Delta is %lf seconds, SYNCH...\n",timeDelta);
							// AbsTime0 is epoch of sim start relative to the J2000 epoch
							AbsTime0 = NewSimTime - SimTime - 946684800 - 43200; 
							// Note that 946684800 is the difference (in seconds) between the J2000 epoch (1Jan2000) 
							// and the Unix epoch (1Jan1970)	
							// For some reason, this delta value results in an answer off by exactly 12 hours,
							// So the 43200 is a factor to compensate - Action to work this disccrepency later...
							// FWIW - The difference between UNIX and GPS (6Jan1980)epochs is 315964800 
						}
						else
						{
							printf("KC Test: Time Delta is %lf seconds, not synching...\n",timeDelta);
						}
					break;
					default:
						printf("UNRECOGNIZED COMMAND %d\n", CmdId);
					break;
				}
				
				// Forward command to interpreter
				if (knownCmd) 
				{
					cmdResult = FswCmdInterpreter(&CmdMsg[0],&cmdTime);
					if(cmdResult)
					{
						printf("CMD_OK\n");
					}
					else
					{
						printf("CMD_REJECTED\n");
					}
				}
					
			}
			else
			{
				//printf("NO DATA\n");
			}
			
		}
	}
}

/**********************************************************************/
void SendSimStatesToUdpClients(void) // UDP clients sending simulation state stream for each 42 S/C to remote servers
/**********************************************************************/
{
	char udpBuf[20000];
	static int first = TRUE;	
	static int firstTimeReport = TRUE;	
	int basePort = 7777; // Base port for state message delivery (S/C 1 = 7777, S/C 2 = 7778, etc.)
	int timeSyncPort = 8888; // Dedicated time synchronozation message
	static SOCKET s[MAX_SC]; // Make room for 20 S/C
	static SOCKET ts_s; // Socket to be used to carry 42 sim timing info
	
	int nSC = Nsc; // map global to local variable
	char Msg[20000];
	long MsgLen = 0;
	long LineLen;
	int msgLen;
	char line[512];
	
	static double lastReportedTime = 0;
	double timeInterval = 0;
	clock_t cur_time;
	static clock_t last_time;
	float simRatio;
	
	if(first)
	{	
		// Initializer the structure used to manage attitude mode status
		for(int i=0;i<MAX_SC;i++)
		{
			attitudeMode[i].last = ATT_MODE_IDLE;
			attitudeMode[i].current = ATT_MODE_IDLE;		
			attitudeMode[i].count = 0;
		}
		
		// Create the sockets
		printf("SendSimStatesToUdpClients ->\n");
		printf("... Setting up for %d S/C.\n", nSC);
		
		if(MAX_SC < nSC)
		{
			printf("!WARNING! Socket space is only defined for %d S/C but %d detected! This may cause undefined behavior\n", MAX_SC, nSC);
		}
		
		ts_s =  socket(AF_INET, SOCK_DGRAM, 0);
		printf("Initializing socket %d for 42 time distribution\n", ts_s);
		
		for(int iSC=0;iSC<nSC;iSC++)
		{
			s[iSC] =  socket(AF_INET, SOCK_DGRAM, 0);
			printf("Initializing socket %d for SC %d\n", s[iSC],iSC);		
		}
		first = FALSE;
	}
	else
	{
		double deltaTime = GpsTime-lastReportedTime;
		if(deltaTime >= 1.0) // Report at 1 second interval (or greater)
		{
			// Capture clock time (to measure sim time ratio)
			cur_time = clock();
			if(firstTimeReport)
			{
				simRatio = 1.0;
				firstTimeReport = FALSE;
			}
			else
			{
				timeInterval = (double)(cur_time - last_time)/CLOCKS_PER_SEC; // Note: In Windows Subsystem for Linux (WSL), this seems to not work properly
				simRatio = (float)(deltaTime/timeInterval);
				//printf("%f, %f, %f, %ld, %ld\n", deltaTime, timeInterval, simRatio, cur_time, last_time);
			}
  
			// Form and send the message distributing 42 simulation time info
			msgLen = ConstructTimeMessage(simRatio, (char*)&udpBuf[0]);
			
			// dynamically create the socket
			struct sockaddr_in dest;
			dest.sin_family = AF_INET;
			dest.sin_port = htons((ushort)timeSyncPort);
			dest.sin_addr.s_addr = 0x0100007f; // localhost
			
			// Send the cargo to the receiver (assuming they are listening...)
			sendto(ts_s,udpBuf,msgLen,0,(struct sockaddr *)(&dest),sizeof(dest));
			
			lastReportedTime = GpsTime;
			last_time = cur_time;
		}
		
		for(int iSC=0;iSC<nSC;iSC++)
		{		
			// Create the cargo for the message
			msgLen = ConstructStateMessage(iSC, &udpBuf[0]);
			
			// dynamically create the socket
			struct sockaddr_in dest;
			dest.sin_family = AF_INET;
			dest.sin_port = htons((ushort)(basePort+iSC));
			dest.sin_addr.s_addr = 0x0100007f; // localhost
			
			// Send the cargo to the receiver (hope they are listening...)
			sendto(s[iSC],udpBuf,msgLen,0,(struct sockaddr *)(&dest),sizeof(dest));
			
			// Determine ADCS mode transitions, send messages on transitions
			if (AdcsModeTransitionDetected(iSC))
			{
				// Create the cargo for the message
				msgLen = ConstructAttModeMessage(iSC, &udpBuf[0]);
				
				// Send the cargo to the receiver (hope they are listening...)
				sendto(s[iSC],udpBuf,msgLen,0,(struct sockaddr *)(&dest),sizeof(dest));	
			}
		}	
	}
		
}

/**********************************************************************/
int AdcsModeTransitionDetected(int Isc) // Monitor error rates of controller to determine when ADCS modes change
/**********************************************************************/
{
	int status = FALSE;
	double rateErrMag;
	double angErrMag;
	
	switch (attitudeMode[Isc].current)
	{
		case ATT_MODE_IDLE:
		break;
		case ATT_MODE_COMMANDED:
			attitudeMode[Isc].current = ATT_MODE_COAST; // Transition to coast and indicate the transition
			printf("Attitude Mode Transition for SC %d >> COAST\n", Isc);
			status = TRUE;
		break;
		case ATT_MODE_COAST:
			// Increment counter on mode change
			attitudeMode[Isc].count++;
			
			// Compute rate and angle error magnitudes
			angErrMag = sqrt(SC[Isc].AC.PrototypeCtrl.therr[0]*SC[Isc].AC.PrototypeCtrl.therr[0]
								+SC[Isc].AC.PrototypeCtrl.therr[1]*SC[Isc].AC.PrototypeCtrl.therr[1]
								+SC[Isc].AC.PrototypeCtrl.therr[2]*SC[Isc].AC.PrototypeCtrl.therr[2]);
			rateErrMag = sqrt(SC[Isc].AC.PrototypeCtrl.werr[0]*SC[Isc].AC.PrototypeCtrl.werr[0]
								+SC[Isc].AC.PrototypeCtrl.werr[1]*SC[Isc].AC.PrototypeCtrl.werr[1]
								+SC[Isc].AC.PrototypeCtrl.werr[2]*SC[Isc].AC.PrototypeCtrl.werr[2]);
			// Transition to CONVERGED if tolerance is satisfied
			//printf("%d %f %f\n", attitudeMode[Isc].count, angErrMag, rateErrMag);
			if(attitudeMode[Isc].count>5 && angErrMag<ATT_CONVERGED_TOL && rateErrMag<RATE_CONVERGED_TOL)
			{
				attitudeMode[Isc].current = ATT_MODE_CONVERGED;
				printf("Attitude Mode Transition for SC %d >> CONVERGED\n", Isc);
				status = TRUE;
			}
		break;
		case ATT_MODE_CONVERGED:
		break;
		default:
		break;
	}
	
	return status;
}

/**********************************************************************/
int ConstructTimeMessage(float simTimeRatio, char* Msg) // Construct the message buffer to deliver 42 time info
/**********************************************************************/
{
	long MsgLen = 0;
	long LineLen;
	char line[512];
	
	// Time  
	sprintf(line,"%d,%18.12le,%f",
	TIME_MSG_ID, GpsTime+GPS_EPOCH_OFFSET, simTimeRatio);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
			
	return MsgLen;
}

/**********************************************************************/
int ConstructStateMessage(int Isc, char* Msg) // Construct the message buffer for a particular SC ID, return the length of the buffer
/**********************************************************************/
{
	int i,Iw,Iorb;
	long MsgLen = 0;
	long LineLen;
	char line[512];
	int EchoEnabled = FALSE; // Overrides global variable
	
	// Message format:
	// MsgID, ScID, GpsEpochSeconds, PosJ2000[3], VelJ2000[3], AttQ_J2000[4], Wbody[3]
	sprintf(line,"%d,%d,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le,%18.12le",
		SC_STATE_MSG_ID,
		Isc,
		GpsTime+GPS_EPOCH_OFFSET,
		Orb[Isc].PosN[0],
		Orb[Isc].PosN[1],
		Orb[Isc].PosN[2],
		Orb[Isc].VelN[0],
		Orb[Isc].VelN[1],
		Orb[Isc].VelN[2],
		SC[Isc].B[0].qn[0],
		SC[Isc].B[0].qn[1],
		SC[Isc].B[0].qn[2],
		SC[Isc].B[0].qn[3],
		SC[Isc].B[0].wn[0],
		SC[Isc].B[0].wn[1],
		SC[Isc].B[0].wn[2]
	);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
			
	return MsgLen;
}


/**********************************************************************/
int ConstructAttModeMessage(int Isc, char* Msg) // Construct the message buffer for a particular SC ID, return the length of the buffer
/**********************************************************************/
{
	int i,Iw,Iorb;
	long MsgLen = 0;
	long LineLen;
	char line[512];
	
	// Message format:
	// MsgID, ScID, GpsEpochSeconds, Mode
	sprintf(line,"%d,%d,%.2lf,%d",
		ATT_MODE_MSG_ID,
		Isc,
		GpsTime+GPS_EPOCH_OFFSET,
		attitudeMode[Isc].current
	);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
			
	return MsgLen;
}

// Functions below are legacy, but left as examples of other means of extracting data from the 42
// Simulation structure and formating buffers for use in a variety of protocols

/**********************************************************************/
int ConstructTimeMessage_verbose(float simTimeRatio, char* Msg) // Construct the message buffer to deliver 42 time info
/**********************************************************************/
{
	long MsgLen = 0;
	long LineLen;
	char line[512];
	
	// Time  
	sprintf(line,"GpsEpochSeconds: %lf, SimTimeRatio: %f\n",
	GpsTime+GPS_EPOCH_OFFSET, simTimeRatio);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
}

/**********************************************************************/
int ConstructStateMessage_verbose(int Isc, char* Msg) // Construct the message buffer for a particular SC ID, return the length of the buffer
/**********************************************************************/
{
	int i,Iw,Iorb;
	long MsgLen = 0;
	long LineLen;
	char line[512];
	int EchoEnabled = FALSE; // Overrides global variable
	
	// Time  
	sprintf(line,"GPS_SECS %.1lf, TIME %ld-%03ld-%02ld:%02ld:%04.1lf\n",
	GpsTime+GPS_EPOCH_OFFSET,Year,doy,Hour,Minute,Second);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
	  
	if (SC[Isc].Exists) {
		// Position
		Iorb = Isc;
            sprintf(line,"Orb[%ld].PosN (m, J2000) =  %.2f %.2f %.2f\n",
               Iorb,
               Orb[Iorb].PosN[0],
               Orb[Iorb].PosN[1],
               Orb[Iorb].PosN[2]);
            if (EchoEnabled) printf("%s",line);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;

		// Velocity
            sprintf(line,"Orb[%ld].VelN (m/s, J2000) =  %.2f %.2f %.2f\n",
               Iorb,
               Orb[Iorb].VelN[0],
               Orb[Iorb].VelN[1],
               Orb[Iorb].VelN[2]);
            if (EchoEnabled) printf("%s",line);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;

		// Attitude and Attitude Rates
		   for(i=0;i<SC[Isc].Nb;i++) {

		   sprintf(line,"SC[%ld].B[%ld].qn (body->J2000) =  %.4f %.4f %.4f %.4f\n",
			  Isc,i,
			  SC[Isc].B[i].qn[0],
			  SC[Isc].B[i].qn[1],
			  SC[Isc].B[i].qn[2],
			  SC[Isc].B[i].qn[3]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;
		   
		   sprintf(line,"SC[%ld].B[%ld].wn (rad/s) =  %.4f %.4f %.4f\n",
			  Isc,i,
			  SC[Isc].B[i].wn[0],
			  SC[Isc].B[i].wn[1],
			  SC[Isc].B[i].wn[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;
		   
		}

	
	}
		
	return MsgLen;
}

/**********************************************************************/
int ConstructStateMessage_orig(int Isc, char* Msg) // Construct the message buffer for a particular SC ID, return the length of the buffer
/**********************************************************************/
{
	int i,Iw,Iorb;
	long MsgLen = 0;
	long LineLen;
	char line[512];
	int EchoEnabled = FALSE; // Overrides global variable
	
	// Time  
	sprintf(line,"GPS_SECS %lf, TIME %ld-%03ld-%02ld:%02ld:%012.9lf\n",
	GpsTime+GPS_EPOCH_OFFSET,Year,doy,Hour,Minute,Second);
	LineLen = strlen(line);
	memcpy(&Msg[MsgLen],line,LineLen);
	MsgLen += LineLen;
	  
	if (SC[Isc].Exists) {
		// Position
		// sprintf(line,"SC[%ld].PosR =  %18.12le %18.12le %18.12le\n",
		   // Isc,
		   // SC[Isc].PosR[0],
		   // SC[Isc].PosR[1],
		   // SC[Isc].PosR[2]);
		// if (EchoEnabled) printf("%s",line);
		// LineLen = strlen(line);
		// memcpy(&Msg[MsgLen],line,LineLen);
		// MsgLen += LineLen;

		// Velocity
		// sprintf(line,"SC[%ld].VelR =  %18.12le %18.12le %18.12le\n",
		   // Isc,
		   // SC[Isc].VelR[0],
		   // SC[Isc].VelR[1],
		   // SC[Isc].VelR[2]);
		// if (EchoEnabled) printf("%s",line);
		// LineLen = strlen(line);
		// memcpy(&Msg[MsgLen],line,LineLen);
		// MsgLen += LineLen;
		
		// Attitude and Attitude Rates (I think)
		   for(i=0;i<SC[Isc].Nb;i++) {
		   sprintf(line,"SC[%ld].B[%ld].wn =  %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].B[i].wn[0],
			  SC[Isc].B[i].wn[1],
			  SC[Isc].B[i].wn[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].B[%ld].qn =  %18.12le %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].B[i].qn[0],
			  SC[Isc].B[i].qn[1],
			  SC[Isc].B[i].qn[2],
			  SC[Isc].B[i].qn[3]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;
		   
		}
		
		// GPS receiver
		for(i=0;i<SC[Isc].AC.Ngps;i++) {
		   sprintf(line,"SC[%ld].AC.GPS[%ld].Valid =  %ld\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Valid);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Rollover =  %ld\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Rollover);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Week =  %ld\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Week);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Sec =  %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Sec);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].PosN =  %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].PosN[0],
			  SC[Isc].AC.GPS[i].PosN[1],
			  SC[Isc].AC.GPS[i].PosN[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].VelN =  %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].VelN[0],
			  SC[Isc].AC.GPS[i].VelN[1],
			  SC[Isc].AC.GPS[i].VelN[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].PosW =  %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].PosW[0],
			  SC[Isc].AC.GPS[i].PosW[1],
			  SC[Isc].AC.GPS[i].PosW[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].VelW =  %18.12le %18.12le %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].VelW[0],
			  SC[Isc].AC.GPS[i].VelW[1],
			  SC[Isc].AC.GPS[i].VelW[2]);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Lng =  %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Lng);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Lat =  %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Lat);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;

		   sprintf(line,"SC[%ld].AC.GPS[%ld].Alt =  %18.12le\n",
			  Isc,i,
			  SC[Isc].AC.GPS[i].Alt);
		   if (EchoEnabled) printf("%s",line);
		   LineLen = strlen(line);
		   memcpy(&Msg[MsgLen],line,LineLen);
		   MsgLen += LineLen;
		   
		}
		
		
		// Worlds
      // for(Iw=1;Iw<NWORLD;Iw++) {
         // if (World[Iw].Exists) {
            // sprintf(line,"World[%ld].PosH =  %18.12le %18.12le %18.12le\n",
               // Iw,
               // World[Iw].PosH[0],
               // World[Iw].PosH[1],
               // World[Iw].PosH[2]);
            // if (EchoEnabled) printf("%s",line);
            // LineLen = strlen(line);
            // memcpy(&Msg[MsgLen],line,LineLen);
            // MsgLen += LineLen;

            // sprintf(line,"World[%ld].eph.PosN =  %18.12le %18.12le %18.12le\n",
               // Iw,
               // World[Iw].eph.PosN[0],
               // World[Iw].eph.PosN[1],
               // World[Iw].eph.PosN[2]);
            // if (EchoEnabled) printf("%s",line);
            // LineLen = strlen(line);
            // memcpy(&Msg[MsgLen],line,LineLen);
            // MsgLen += LineLen;

            // sprintf(line,"World[%ld].eph.VelN =  %18.12le %18.12le %18.12le\n",
               // Iw,
               // World[Iw].eph.VelN[0],
               // World[Iw].eph.VelN[1],
               // World[Iw].eph.VelN[2]);
            // if (EchoEnabled) printf("%s",line);
            // LineLen = strlen(line);
            // memcpy(&Msg[MsgLen],line,LineLen);
            // MsgLen += LineLen;

         // }
      // }

	  // Orbits
      for(Iorb=0;Iorb<Norb;Iorb++) { //KC - Orbits are for the celestial bodies defined (I guess). [0] is the one that matches Earth
         if (Orb[Iorb].Exists) {
            sprintf(line,"Orb[%ld].PosN =  %18.12le %18.12le %18.12le\n",
               Iorb,
               Orb[Iorb].PosN[0],
               Orb[Iorb].PosN[1],
               Orb[Iorb].PosN[2]);
            if (EchoEnabled) printf("%s",line);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;

            sprintf(line,"Orb[%ld].VelN =  %18.12le %18.12le %18.12le\n",
               Iorb,
               Orb[Iorb].VelN[0],
               Orb[Iorb].VelN[1],
               Orb[Iorb].VelN[2]);
            if (EchoEnabled) printf("%s",line);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;

         }
      }
	
	}
		
	return MsgLen;
}

