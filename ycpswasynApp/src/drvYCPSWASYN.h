// This file is part of 'YCPSW EPICS module'.
// It is subject to the license terms in the LICENSE.txt file found in the
// top-level directory of this distribution and at:
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
// No part of 'YCPSW EPICS module', including this file,
// may be copied, modified, propagated, or distributed except according to
// the terms contained in the LICENSE.txt file.

#include <stdio.h>
#include <string.h>
#include <boost/array.hpp>
#include "asynPortDriver.h"

#include <cpsw_api_builder.h>
#include <cpsw_api_user.h>
#include <cpsw_yaml.h>
#include <yaml-cpp/yaml.h>

#define DRIVER_NAME		"YCPSWASYN_v1"

#define CARRIER_KEY		"CarrierCore"
#define	CARRIER_SUBS		"C"
#define BAY0_KEY		"Bay0"
#define	BAY0_SUBS		"B0"
#define BAY1_KEY		"Bay1"
#define	BAY1_SUBS		"B1"
#define APP_KEY			"App"
#define	APP_SUBS		"A"
#define STREAM_KEY		"Stream"
#define DB_NAME_PREFIX_LENGTH_MAX	10
#define DB_NAME_LENGTH_MAX			37
#define DB_DESC_LENGTH_MAX			29
#define DB_MBBX_NELEM_MAX			16

char const *mbbxValParam[]
{
	"ZRVL",
	"ONVL",
	"TWVL",
	"THVL",
	"FRVL",
	"FVVL",
	"SXVL",
	"SVVL",
	"EIVL",
	"NIVL",
	"TEVL",
	"ELVL",
	"TVVL",
	"TTVL",
	"FTVL",
	"FFVL"
};

char const *mbbxNameParams[]
{
	"ZRST",
	"ONST",
	"TWST",
	"THST",
	"FRST",
	"FVST",
	"SXST",
	"SVST",
	"EIST",
	"NIST",
	"TEST",
	"ELST",
	"TVST",
	"TTST",
	"FTST",
	"FFST"
};

enum deviceTypeList 
{
	DEV_REG_RO,
	DEV_REG_RW,
	DEV_CMD,
	DEV_STM,
	SIZE
};

typedef struct
{
	void 	*pPvt;
	Stream 	stm;
	int 	param16index;
	int	param32index;
} ThreadArgs;

#define MAX_SIGNALS		((int)deviceTypeList(SIZE) + 1)
#define	NUM_PARAMS		5000
#define NUM_CMD			500
//#define NUM_STREAMS		10
#define STREAM_MAX_SIZE 	200UL*1024ULL*1024ULL


class YCPSWASYN : public asynPortDriver {
	public:
		YCPSWASYN(const char *portName, Path p, const char *recordPrefix, int recordNameLenMax);
		
		// Methods that we override from asynPortDriver
		virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
		virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
		virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
		virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements);
		virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
		virtual asynStatus writeOctet (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
		virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
		virtual asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements);
		virtual asynStatus writeUInt32Digital (asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
		virtual asynStatus readUInt32Digital (asynUser *pasynUser, epicsUInt32 *value, epicsUInt32 mask);

		virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);

		virtual void report(FILE *fp, int details);
		
		// New Methods for this class
		void streamTask(Stream stm, int param16index, int param32index);
		static int YCPSWASYNInit(const char *yaml_doc, Path *p, const char *ipAddr);

		
	protected:
		int 		pRwIndex;
		int 		pRoIndex;
		int 		pCmdIndex;
		//int 		pStmIndex;
		ScalVal		rw[NUM_PARAMS];
		ScalVal_RO 	ro[NUM_PARAMS];
		Command		cmd[NUM_CMD];
		//Stream		stm[NUM_STREAMS];
			
	private:
		const char *driverName_;
		Path p_;
		const char *portName_;
		const char *recordPrefix_;
		const int 	recordNameLenMax_;

		static void printChildrenPath(Path p);
		static void printChildren(Hub h);
		static std::string generatePrefix(Path p);
		static std::string trimPath(Path p, size_t pos);

		virtual void generateDB(Path p);
};

void streamTask(ThreadArgs *arglist);
