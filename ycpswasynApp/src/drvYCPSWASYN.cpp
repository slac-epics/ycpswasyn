// This file is part of 'YCPSW EPICS module'.
// It is subject to the license terms in the LICENSE.txt file found in the
// top-level directory of this distribution and at:
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
// No part of 'YCPSW EPICS module', including this file,
// may be copied, modified, propagated, or distributed except according to
// the terms contained in the LICENSE.txt file.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <getopt.h>
#include <sstream>
#include <boost/array.hpp>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <math.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include <dbAccess.h>
#include <dbStaticLib.h>

#include "drvYCPSWASYN.h"
#include "asynPortDriver.h"
#include <epicsExport.h>

#include <cpsw_api_builder.h>
#include <cpsw_api_user.h>
#include <cpsw_yaml.h>
#include <yaml-cpp/yaml.h>

using std::string;
using std::stringstream;

YCPSWASYN::YCPSWASYN(const char *portName, Path p , const char *recordPrefix, int recordNameLenMax)
	: asynPortDriver(	portName, 
				MAX_SIGNALS, 
				NUM_PARAMS, 
				asynInt32Mask | asynDrvUserMask | asynInt16ArrayMask | asynInt32ArrayMask | asynOctetMask | asynFloat64ArrayMask | asynUInt32DigitalMask ,	// Interface Mask
				asynInt16ArrayMask | asynInt32ArrayMask | asynInt32Mask, 	  		// Interrupt Mask
				ASYN_MULTIDEVICE | ASYN_CANBLOCK, 			// asynFlags
				1, 							// Autoconnect
				0, 							// Default priority
				0),							// Default stack size
	driverName_(DRIVER_NAME),
	p_(p),
	portName_(portName),
	recordPrefix_(recordPrefix),
	recordNameLenMax_(recordNameLenMax),
	nRO(0),
	nRW(0),
	nCMD(0),
	nSTM(0),
	recordCount(0)
{
	//const char *functionName = "YCPSWASYN";

	// Create file names from the register and PV list dumps
	std::string regDumpFileName = DUMP_FILE_PATH + string(recordPrefix_) + "_" + REG_DUMP_FILE_NAME;
	std::string pvDumpFileName = DUMP_FILE_PATH + string(recordPrefix_) + "_" + PV_DUMP_FILE_NAME;

	// Write the list of register
	printf("Dumping register map into %s...\n", regDumpFileName.c_str()); //REG_DUMP_FILE_NAME);
	regDumpFile.open(regDumpFileName.c_str()); //REG_DUMP_FILE_NAME);
	dumpRegisterMap(p);
	regDumpFile.close();
	printf("Done!\n");

	// Open the PV list file
	printf("Generatin database from yaml file and dumping pv list on %s...\n", pvDumpFileName.c_str()); //PV_DUMP_FILE_NAME);
	pvDumpFile.open(pvDumpFileName.c_str()); //PV_DUMP_FILE_NAME);

	// Generate the EPICS databse from the root path
	generateDB(p);

	// Close the PV list file
	pvDumpFile.close();
	printf("Done!\n");
	
	// Print the counters
	printf("nRO = %ld\nnRW = %ld\nnCMD = %ld\nnSTM = %ld\nrecordCount = %ld\n", nRO, nRW, nCMD, nSTM, recordCount);
}

///////////////////////////////////
// + Stream acquisition routines //
///////////////////////////////////
void streamTaskC(ThreadArgs *arglist)
{
    YCPSWASYN *pYCPSWASYN = (YCPSWASYN *)arglist->pPvt;
    pYCPSWASYN->streamTask(arglist->stm, arglist->param16index, arglist->param32index);
}

/////////////////////////////////////////////////////////////////////////////////
// void YCPSWASYN::streamTask(Stream stm, int param16index, int param32index); //
//                                                                             //
// - Stream handling function                                                  //
/////////////////////////////////////////////////////////////////////////////////
void YCPSWASYN::streamTask(Stream stm, int param16index, int param32index)
{
	uint64_t got = 0;
	size_t nWords16, nWords32, nBytes;
	int nFrame;
	uint8_t *buf = new uint8_t[STREAM_MAX_SIZE];

	if (!stm)
	{
		printf("Error on stream handler\n");
		return;
	}

// + Generate fake data //
	/*
	epicsInt32 *fakeData = (epicsInt32 *)calloc(n, sizeof(epicsInt32));

	for (int i = 0 ; i < n ; i++)
	{
		*(fakeData+i) = (epicsInt32)(10000+1000*sin(2*3.14*i*(pIndex+1)/n));
	}

	while(1)
	{
		std::copy(fakeData, fakeData+n, buffer);
		doCallbacksInt32Array(buffer, n, pIndex, DEV_STM); 	
	  	epicsThreadSleep(1);
	}
	*/
// - Generate fake data //

	try 
	{
    	while(1)
        {
        	got = stm->read( buf, STREAM_MAX_SIZE, CTimeout(-1));
        	if( !got )
        	{
        	}
        	else
        	{
        		nBytes = (got - 9); // header = 8 bytes, footer = 1 byte, data = 32bit words.
        		nWords16 = nBytes / 2;
        		nWords32 = nWords16 / 2; 
		       
		        nFrame = (buf[1]<<4) | (buf[0] >> 4);

		        printf("got = %zu bytes (%zu 32-bit words, %zu 16-bit words). Fame # %d\n", nBytes, nWords32, nWords16, nFrame);
	
				doCallbacksInt16Array((epicsInt16*)(buf+8), nWords16, param16index, DEV_STM); 	
				doCallbacksInt32Array((epicsInt32*)(buf+8), nWords32, param32index, DEV_STM); 	

        		memset(buf, 0, STREAM_MAX_SIZE*sizeof(uint8_t));

            }
        }
	} catch( IntrError e )
	{

	}

	return;
}

///////////////////////////////////
// - Stream acquisition routines //
///////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// int YCPSWASYN::YCPSWASYNInit(const char *yaml_doc, Path *p, const char *ipAddr) //
// - Initialization routine                                                        //
//                                                                                 //
/////////////////////////////////////////////////////////////////////////////////////
int YCPSWASYN::YCPSWASYNInit(const char *yaml_doc, Path *p, const char *ipAddr)
{
	unsigned char buf[sizeof(struct in6_addr)];

	// Read YAML file
	YAML::Node doc =  YAML::LoadFile( yaml_doc );

	// Check if the IP address was especify. Otherwise use the one defined on the YAML file
	if (inet_pton(AF_INET, ipAddr, buf))
	{
		printf("Using IP address: %s\n", ipAddr);
		doc["ipAddr"] = ipAddr;
	}
	else
		printf("Using IP address from YAML file\n");

	// Create an NetIODev from the YAML file definition
	NetIODev  root = doc.as<NetIODev>();

	// Create a Path on the root
	*p = IPath::create( root );

	return 0;
}

/////////////////////////////////////////////////////
// void YCPSWASYN::dumpRegisterMap(const Path& p); //
//                                                 //
// - Write list of register to file                //
/////////////////////////////////////////////////////
void YCPSWASYN::dumpRegisterMap(const Path& p)
{

  	Path p_aux = p->clone();
	Child c_aux = p_aux->tail();
	Hub h_aux;

	// Check if the child is null which mean where are the origin
	if (c_aux)
		h_aux = c_aux->isHub();
	else
		h_aux = p->origin();

	
	Children 	c = h_aux->getChildren();
	int 		n = c->size();
	
	// Recursively look for hubs.
	for (int i = 0 ; i < n ; i++)
	{
		Hub h2 = (*c)[i]->isHub();
		if (h2)
		{
			Path p2 = p->findByName((*c)[i]->getName());
			YCPSWASYN::dumpRegisterMap(p2);
		}
		else
		{
			// When a leave is found, write it path and name to the output file
			regDumpFile << p->toString() << '/' << (*c)[i]->getName() << std::endl;			
		}
	}

	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// int YCPSWASYN::LoadRecord(int regType, const recordParams& rp, const string& dbParams); //
//                                                                                         //
// - Load a EPICS record with the provided infomation                                      //
/////////////////////////////////////////////////////////////////////////////////////////////
//template <typename T>
int YCPSWASYN::LoadRecord(int regType, const recordParams& rp, const string& dbParams)
{
	int paramIndex;
	stringstream dbParamsLocal;

	// Create list of paramater to pass to the  dbLoadRecords function
	dbParamsLocal.str("");
	dbParamsLocal << "PORT=" << portName_;
	dbParamsLocal << ",ADDR=" << regType;
	dbParamsLocal << ",P=" << recordPrefix_;
	dbParamsLocal << ",R=" << rp.recName;
	dbParamsLocal << ",PARAM=" << rp.paramName;
	//dbParamsLocal << ",DESC=\"" << rp.recDesc.substr(0, DB_DESC_LENGTH_MAX) << "\"";
	dbParamsLocal << ",DESC=" << rp.recDesc;
	dbParamsLocal << dbParams;
	
	// Create the asyn paramater
	createParam(regType, rp.paramName.c_str(), rp.paramType, &paramIndex);	//
	
	// Cretae the record
	dbLoadRecords(rp.recTemplate.c_str(), dbParamsLocal.str().c_str());

	// Write the record name to the PV list file
	pvDumpFile << recordPrefix_ << ':' << rp.recName << std::endl;

	// Incrfement the umber of created records
	++recordCount;

	// Return the parameter index
	return paramIndex;
}

//////////////////////////////////////
// + template <typename T>          //
//   int getRegType(const T& reg);  //
//                                  //
// - Get the register type          //
//////////////////////////////////////
template <>
int YCPSWASYN::getRegType(const ScalVal_RO& reg)
{
	return DEV_REG_RO;
}

template <>
int YCPSWASYN::getRegType(const ScalVal& reg)
{
	return DEV_REG_RW;
}

template <>
int YCPSWASYN::getRegType(const Command& reg)
{
	return DEV_CMD;
}

template <>
int YCPSWASYN::getRegType(const Stream& reg)
{
	return DEV_STM;
}
//////////////////////////////////////
// - template <typename T>          //
//   int getRegType(const T& reg);  //
//////////////////////////////////////


/////////////////////////////////////////////////////////////////
// + template <typename T>                                     //
//   void pushParameter(const T& reg, const int& parmaIndex);  //
//                                                             //
// - Push the register pointer to its respective list based    //
//   on the parameter index                                    //
/////////////////////////////////////////////////////////////////
template <>
void YCPSWASYN::pushParameter(const ScalVal_RO& reg, const int& paramIndex)
{
	ro[paramIndex] = reg;
	nRO++;
}

template <>
void YCPSWASYN::pushParameter(const ScalVal& reg, const int& paramIndex)
{
	rw[paramIndex] = reg;
	nRW++;
}

template <>
void YCPSWASYN::pushParameter(const Command& reg, const int& paramIndex)
{
	cmd[paramIndex] = reg;
	nCMD++;
}

template <>
void YCPSWASYN::pushParameter(const Stream& reg, const int& paramIndex)
{
	nSTM++;
}
/////////////////////////////////////////////////////////////////
// - template <typename T>                                     //
//   void pushParameter(const T& reg, const int& parmaIndex);  //
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// std::string YCPSWASYN::extractMbbxDbParams(const Enum& isEnum); //
//  - Extract record parameters related to MBBx records            //
/////////////////////////////////////////////////////////////////////
std::string YCPSWASYN::extractMbbxDbParams(const Enum& isEnum)
{

	int nValues = isEnum->getNelms();
	int mBits = log2(nValues);
	stringstream dbParamsLocal;

	dbParamsLocal.str("");
	dbParamsLocal << ",MASK=" << ((1 << mBits) - 1);
	dbParamsLocal << ",NOBT=" << mBits;

	IEnum::iterator it;
	int k;
	for (it = isEnum->begin(), k = 0 ; k < DB_MBBX_NELEM_MAX ; k++)
	{
		if (it != isEnum->end())
		{
			dbParamsLocal << "," << mbbxNameParams[k] << "=" << (*it).first->c_str();
			dbParamsLocal << "," << mbbxValParam[k] << "=" << (*it).second;
			++it;
		}
		else
		{
			dbParamsLocal << "," << mbbxNameParams[k] << "=";
			dbParamsLocal << "," << mbbxValParam[k] << "=";
		}
	}

	return dbParamsLocal.str();
}

///////////////////////////////////////////////////////
// + template <typename T>                           //
//   int CreateRecord(const T& reg);                 //
//                                                   //
// - Create a record from a register pointer         //
///////////////////////////////////////////////////////
template <typename T>
void YCPSWASYN::CreateRecord(const T& reg)
{
	string dbParams;
	stringstream pName;
	int paramIndex;

	// Get the path to the register
	Path p = reg->getPath();

	// Get the child at the tail
	Child c = p->tail();

	// Het the register type
	int regType = getRegType(reg);

	// Get the register information
	long nBits = reg->getSizeBits();
	Enum isEnum	= reg->getEnum();
	int nElements = reg->getNelms();

	// Create the argument list used when loading the record
	recordParams trp;
	// + record name
	trp.recName = YCPSWASYN::generatePrefix(p) + c->getName();
	trp.recName = trp.recName.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4) + recordSufix[regType];
	// + parameter name
	pName.str("");
    pName << string(c->getName()).substr(0, 10) << recordCount;
	trp.paramName = pName.str();
	// + record description field
	trp.recDesc = string("\"") + string(c->getDescription()).substr(0, DB_DESC_LENGTH_MAX) + string("\"");

	// Look trough the register properties and create the appropiate record type
	if ((!isEnum) || (isEnum->getNelms() > DB_MBBX_NELEM_MAX))
	{
		if (nElements == 1)
		{
			// Create ax record
			trp.paramType = asynParamInt32;
			trp.recTemplate = templateList[regType][0];
			paramIndex = LoadRecord(regType, trp, dbParams);
			pushParameter(reg, paramIndex);
		}
		else
		{
			// Create waveform record
			stringstream dbParamsLocal;
			dbParamsLocal.str("");
			dbParamsLocal << ",N=" << reg->getNelms();
			dbParams += dbParamsLocal.str();

			if (nBits == 8)
			{
				// Create waveform_8_x record
				trp.paramType = asynParamOctet;
                trp.recTemplate = templateList[regType][3];

			}
			else
			{
				// Create waveform_x record
				trp.paramType = asynParamInt32Array;
                trp.recTemplate = templateList[regType][2];

			}
			
			paramIndex = LoadRecord(regType, trp, dbParams);
			pushParameter(reg, paramIndex);

		}
	}
	else
	{
		if (nElements == 1)
		{
			// Create mbbx record
			dbParams += extractMbbxDbParams(isEnum);

			trp.paramType = asynParamUInt32Digital;
			trp.recTemplate = templateList[regType][1];
				
			paramIndex = LoadRecord(regType, trp, dbParams);
			pushParameter(reg, paramIndex);

		}
		else
		{
			// Create array of mbbx records
			stringstream index_aux;
			string c_name;			
			Path pClone = p->clone();
			pClone->up();

			for (int j = 0 ; j < nElements ; j++)
			{
				index_aux.str("");
				index_aux << j;

				c_name.clear();
				c_name = c->getName() + string("[") + index_aux.str() + string("]");
				Path c_path = pClone->findByName(c_name.c_str());
				T c_reg = IScalVal::create(c_path);

				trp.recName = YCPSWASYN::generatePrefix(p) + c->getName();
        		trp.recName = trp.recName.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 5) + index_aux.str() + recordSufix[regType];

        		pName.str("");
    			pName << string(c->getName()).substr(0, 10) << recordCount;
				trp.paramName = pName.str();

				Enum isEnumLocal = c_reg->getEnum();

				dbParams += extractMbbxDbParams(isEnumLocal);

				trp.paramType = asynParamUInt32Digital;
				trp.recTemplate = templateList[regType][1];
 
				paramIndex = LoadRecord(regType, trp, dbParams);                   
				pushParameter(c_reg, paramIndex);
			}

		}
	}
}
///////////////////////////////////////////////////////
// - template <typename T>                           //
//   int CreateRecord(const T& reg);                 //
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// + template <typename T>                           //
//   int CreateRecord(const T& reg, const Path& p);  //
//                                                   //
//  - Create a record from a register pointer and    //
//     its path. This is for Command and Stream      //
//     which don't support the getPath() function    // 
///////////////////////////////////////////////////////
template <>
void YCPSWASYN::CreateRecord(const Command& reg, const Path& p_)
{
	//Path p = reg->getPath();	// NOTE: Command/Stream don't support getPath() as ScalVal does. 
								// Workaround: using overloaded function with argument  Path p.
	Path p = p_->clone();
	Child c = p->tail();

	recordParams trp;
	string dbParams;
	int regType;
	int paramIndex;
	stringstream pName;

	trp.recDesc = string(c->getDescription());

	regType = getRegType(reg);

	trp.recName = YCPSWASYN::generatePrefix(p) + c->getName();
	trp.recName = trp.recName.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4) + recordSufix[regType];

	pName.str("");
    pName << string(c->getName()).substr(0, 10) << recordCount;
	trp.paramName = pName.str();

	trp.paramType = asynParamInt32;
	trp.recTemplate = templateList[regType][0];

	paramIndex = LoadRecord(regType, trp, dbParams);
	pushParameter(reg, paramIndex);

}

template <>
void YCPSWASYN::CreateRecord(const Stream& reg, const Path& p_)
{

	//Path p = reg->getPath();	// NOTE: Command/Stream don't support getPath() as ScalVal does. 
								// Workaround: using overloaded function with argument  Path p.
	Path p = p_->clone();
	Child c = p->tail();

	recordParams trp;
	string dbParams;
	int regType;
	int p16StmIndex, p32stmIndex;
	stringstream pName;

	trp.recDesc = string(c->getDescription());

	regType = getRegType(reg);

	// Create PVs for 32-bit stream data
	trp.recName = YCPSWASYN::generatePrefix(p) + c->getName();
	trp.recName = trp.recName.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4) + ":32";

	pName.str("");
    pName << string(c->getName()).substr(0, 10) << recordCount;
	trp.paramName = pName.str();

	trp.paramType = asynParamInt32;
	trp.recTemplate = templateList[regType][0];

	p32stmIndex = LoadRecord(regType, trp, dbParams);
	

	// Create PVs for 16-bit stream data
	dbParams.clear();

	trp.recName = YCPSWASYN::generatePrefix(p) + c->getName();
	trp.recName = trp.recName.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4) + ":16";

	pName.str("");
    pName << string(c->getName()).substr(0, 10) << recordCount;
	trp.paramName = pName.str();

	trp.paramType = asynParamInt32;
	trp.recTemplate = templateList[regType][1];

	p16StmIndex = LoadRecord(regType, trp, dbParams);


	// Crteate Acquisition Thread 
	asynStatus status;
	ThreadArgs arglist;
	arglist.pPvt = this;
	arglist.stm = reg;
	arglist.param16index = p16StmIndex;
	arglist.param32index = p32stmIndex;
	status = (asynStatus)(epicsThreadCreate("Stream", epicsThreadPriorityLow, 
	        epicsThreadGetStackSize(epicsThreadStackMedium), (EPICSTHREADFUNC)::streamTaskC, &arglist) == NULL);
	 
	if (status) 
	{
	    printf("epicsThreadCreate failure for stream %s\n", trp.recName.c_str());
	    return;
	}
	else
		printf("epicsThreadCreate successfully for stream %s\n", trp.recName.c_str());

	nSTM++;

}
///////////////////////////////////////////////////////
// - template <typename T>                           //
//   int CreateRecord(const T& reg, const Path& p);  //
///////////////////////////////////////////////////////

////////////////////////////////////////////////
// void YCPSWASYN::generateDB(const Path& p); //
//                                            //
// - Generate the EPICS databse for all the   //
//   registers on the especified path         //
////////////////////////////////////////////////
void YCPSWASYN::generateDB(const Path& p)
{
	Path p_aux = p->clone();
	Child c_aux = p_aux->tail();
	Hub h_aux;

	// Check if the child is null which mean where are the origin
	if (c_aux)
		h_aux = c_aux->isHub();
	else
		h_aux = p->origin();

	Children 		c = h_aux->getChildren();
	int 			n = c->size();
	stringstream 	db_params, param_name;
	string 			rec_name;

	// Recursively look for hubs.
	for (int i = 0 ; i < n ; i++)
	{
		try 
		{
			Hub 	h2 = (*c)[i]->isHub();
			int 	m = (*c)[i]->getNelms();
			Path 	p2;

			if (h2)
			{
				// If this is an arrayof hubs, parse one at a time.
				if (m > 1)
				{
					stringstream c_name;

					for (int j = 0 ; j < m ; j++)
					{
						c_name.str("");
						c_name << (*c)[i]->getName() << "[" << j << "]";
						p2 = p->findByName(c_name.str().c_str());

						YCPSWASYN::generateDB(p2);
					}
				}
				else
				{
					p2 = p->findByName((*c)[i]->getName());
					YCPSWASYN::generateDB(p2);
				}
			}
			else
			{			
				p2 = p->findByName((*c)[i]->getName());

				// Look first for stream interfaces (based only on its name)
				size_t found_key = string((*c)[i]->getName()).find(STREAM_KEY);

				try 
				{
					// If found, try to attached an stream interface and create a record
					if ((found_key != std::string::npos) && (isdigit(((*c)[i]->getName())[found_key+strlen(STREAM_KEY)])))
					{
						Stream stm_aux;

						try
						{
							stm_aux = IStream::create(p2);
						}
						catch (CPSWError &e)
						{
						}

						if (stm_aux)
							YCPSWASYN::CreateRecord(stm_aux, p2);
					}
					// Continue with not-stream registers
					else
					{
						ScalVal		rw_aux;
						ScalVal_RO 	ro_aux;
						Command		cmd_aux;

						// Try to attach a ScalVal_RO and ScalVal interface
						try 
						{
							ro_aux = IScalVal_RO::create(p2);
							rw_aux = IScalVal::create(p2);		
						}
						catch (CPSWError &e)
						{
						}
						
						// Now, try to attach a command interface
						try 
						{
							cmd_aux = ICommand::create(p2);							
						}
						catch (CPSWError &e)
						{
						}

						// Depending on the attached interface, create a record for it
						if (rw_aux)
							YCPSWASYN::CreateRecord(rw_aux);

						if (ro_aux)
							YCPSWASYN::CreateRecord(ro_aux);

						if (cmd_aux)
							YCPSWASYN::CreateRecord(cmd_aux, p2);
					}
				}
				catch (CPSWError &e)
				{
					//std::cerr << "CPSW Error: " << e.getInfo() << "\n";
				}
			}
		}
		catch (CPSWError &e)
		{
			std::cerr << "CPSW Error: " << e.getInfo() << "\n";
		}
	}


	return;
}

///////////////////////////////////////////////////////////
// std::string YCPSWASYN::generatePrefix(const Path& p); //
//                                                       //
// - Create the record name prefix from its path         //
///////////////////////////////////////////////////////////
std::string YCPSWASYN::generatePrefix(const Path& p)
{
	string p_str = p->toString();
	std::size_t found; 

	// Look for the defined keys and substitute it with its corresponding string
	// plus a trim version of the path
	if ((found = p_str.find(BAY0_KEY)) != std::string::npos)
		return (std::string(BAY0_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	if ((found = p_str.find(BAY1_KEY)) != std::string::npos)
		return (std::string(BAY1_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	if ((found = p_str.find(CARRIER_KEY)) != std::string::npos)
		return (std::string(CARRIER_SUBS) + ":" + YCPSWASYN::trimPath(p, found));	

	if ((found = p_str.find(APP_KEY)) != std::string::npos)
		return (std::string(APP_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	// return an empty string if not key is found
	return std::string();
}

/////////////////////////////////////////////////////////////////
// std::string YCPSWASYN::trimPath(const Path& p, size_t pos); //
//                                                             //
// - Trim the path for the generation of the record name       //
/////////////////////////////////////////////////////////////////
std::string YCPSWASYN::trimPath(const Path& p, size_t pos)
{
	string p_str = p->toString();
	string c_str, pre_str, aux_str;
	std::size_t found_bracket, found_slash;

	// Look for the fisrt / on the path. 
	// If not found return an empty string
	if ((found_slash = p_str.find("/", pos)) == std::string::npos)
		return std::string();

	// Remove everything to the left of /
	p_str = p_str.substr(found_slash+1);

	// Look for all / on the path
	while ( (found_slash = p_str.find("/")) != std::string::npos)
	{
		c_str = p_str.substr(0, found_slash);
		p_str = p_str.substr(found_slash + 1);

		// Trim the device name
		aux_str = c_str.substr(0,DB_NAME_PATH_TRIM_SIZE);
		if ((found_bracket = c_str.find('[')) != std::string::npos)
			aux_str += c_str.substr(found_bracket+1, c_str.length() - found_bracket - 2);

		pre_str += aux_str + ":";

	}
	return pre_str;
}

/////////////////////////////////////////////
// + Methods overrided from asynPortDriver //
/////////////////////////////////////////////
asynStatus YCPSWASYN::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *name;

	this->getAddress(pasynUser, &addr);
     
	static const char *functionName = "writeInt32";
     
	this->getAddress(pasynUser, &addr);

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RW)
				rw[function]->setVal((uint32_t*)&value, 1);
			else if (addr == DEV_CMD)
				cmd[function]->execute();
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s parameter %s set to %d\n", \
						driverName_, functionName, function, this->portName, name, value);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR setting parameter %s to %d\n", \
						driverName_, functionName, function, this->portName, name, value);				
		}

	}
   	else
		status = asynPortDriver::writeInt32(pasynUser, value);
    
	callParamCallbacks(addr);
 
	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	uint32_t u32;
	const char *name;

	this->getAddress(pasynUser, &addr);
     
	static const char *functionName = "readInt32";

	if (!getParamName(addr, function, &name))
	{

		try
		{
			if (addr == DEV_REG_RO)
			{
				try
				{
					ro[function]->getVal(&u32, 1);
				}
				catch (CPSWError &e)
				{
					status = -1;
					asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
				}
			}
			else if (addr == DEV_REG_RW)
			{
				try
				{
					rw[function]->getVal(&u32, 1);
				}
				catch (CPSWError &e)
				{
					status = -1;
					asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
				}
			}
			else if (addr == DEV_CMD)
			{
				u32 = 0;
				status = 0;
			}
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			*value = (epicsInt32)u32;
			setIntegerParam(addr, function, (int)u32);

			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s read %d from register %s\n", \
						driverName_, functionName, function, this->portName, *value, name);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR reading register %s\n", \
								driverName_, functionName, function, this->portName, name);
		}
	}
	else
		status = asynPortDriver::readInt32(pasynUser, value);
     
	callParamCallbacks(addr);
     
  	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	size_t n;
	const char *name;
	static const char *functionName = "writeInt32Array";
	IndexRange range(0, nElements-1);

	this->getAddress(pasynUser, &addr);

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RW)
				n = rw[function]->setVal((uint32_t*)value, nElements, &range);
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s set new content on parameter %s. Requested = %zu, written = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements, n);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR setting parameter %s. Requested = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements);				

		}

	}
	else

		status = asynPortDriver::writeInt32Array(pasynUser, value, nElements);

	return (status==0) ? asynSuccess : asynError;

}

asynStatus YCPSWASYN::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
    const char *name;
	static const char *functionName = "readInt32Array";
	this->getAddress(pasynUser, &addr);
	uint64_t *buffer = new uint64_t[nElements];

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RO)
				ro[function]->getVal(buffer, nElements);
			else if (addr == DEV_REG_RW)
				rw[function]->getVal(buffer, nElements);
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{

			std::copy(buffer, buffer+nElements, value);
			*nIn = nElements;
		
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s got parameter %s, requested = %zu, got = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements, *nIn);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR getting  parameter %s. Requested = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements);
		}
	}
	else
		status = asynPortDriver::readInt32Array(pasynUser, value, nElements, nIn);
  
	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason)
{
        int addr;
        int function = pasynUser->reason;
        int status=0; 
        const char *name;
        static const char *functionName = "readOctet";
        uint8_t *buffer = new uint8_t[maxChars];
        this->getAddress(pasynUser, &addr);

        if (!getParamName(addr, function, &name))
		{
			try
			{
				if (addr == DEV_REG_RO)
					ro[function]->getVal(buffer, maxChars);
				else if (addr == DEV_REG_RW)
					rw[function]->getVal(buffer, maxChars);
				else
					status = -1;
			}
			catch (CPSWError &e)
			{
				status = -1;
				asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
			}
			
			if (status == 0)
			{
				std::copy(buffer, buffer+maxChars, value);
				*nActual = maxChars;
			
				asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
							"%s:%s(%d), port %s maxChars = %zu, nActual = %zu, eomReason %d\n", \
							driverName_, functionName, function, this->portName, maxChars, *nActual, *eomReason);
			}
			else
			{
				asynPrint(pasynUser, ASYN_TRACE_ERROR, \
							"%s:%s(%d), port %s ERROR getting parameter %s. Requested = %zu\n", \
							driverName_, functionName, function, this->portName, name, maxChars);
			}
		}
        else
                status = asynPortDriver::readOctet(pasynUser, value, maxChars, nActual, eomReason);

        return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::writeOctet (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
	    int addr;
        int function = pasynUser->reason;
        int status=0;
        const char *name;
        static const char *functionName = "writeOctet";
        this->getAddress(pasynUser, &addr);
       	IndexRange range(0, maxChars-1);
        
        if (!getParamName(addr, function, &name))
		{
			if (addr == DEV_REG_RW)
			{
				try
				{
					*nActual = (size_t)rw[function]->setVal((uint8_t*)value, maxChars, &range);
				}
				catch (CPSWError &e)
				{
					status = -1;
					asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
				}

				if (*nActual <= 0)
						status = -1;
			}
			else
				status = -1;

			if (status == 0)
			{
				asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
							"%s:%s(%d), port %s set new content on parameter %s. Requested = %zu, written = %zu\n", \
							driverName_, functionName, function, this->portName, name, maxChars, *nActual);
			}
			else
			{
				asynPrint(pasynUser, ASYN_TRACE_ERROR, \
							"%s:%s(%d), port %s ERROR setting parameter %s. Requested = %zu\n", \
							driverName_, functionName, function, this->portName, name, maxChars);
			}
		}
		else
			status = writeOctet (pasynUser, value, maxChars, nActual);

		return (status==0) ? asynSuccess : asynError;
}


asynStatus YCPSWASYN::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	uint64_t *buffer = new uint64_t[nElements];
    this->getAddress(pasynUser, &addr);
	const char *name;
     
	static const char *functionName = "readFloat64Array";

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RO)
				ro[function]->getVal(buffer, nElements);
			else if (addr == DEV_REG_RW)
				rw[function]->getVal(buffer, nElements);
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			std::copy(buffer, buffer+nElements, value);
			*nIn = nElements;
			
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s get parameter %s. Requested = %zu, got = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements, *nIn);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR getting parameter %s. Requested = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements);
		}
	}
	else
		status = asynPortDriver::readFloat64Array(pasynUser, value, nElements, nIn);
     
	callParamCallbacks();
     
  	return (status==0) ? asynSuccess : asynError;

}

asynStatus YCPSWASYN::writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	size_t n;
    this->getAddress(pasynUser, &addr);
	const char *name;
	IndexRange range(0, nElements-1);

     
	static const char *functionName = "writeFloat64Array";

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RW)
				n = rw[function]->setVal((uint32_t*)value, nElements, &range);
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s set new content on parameter %s. Requested = %zu, written = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements, n);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR setting parameter %s. Requested = %zu\n", \
						driverName_, functionName, function, this->portName, name, nElements);
		}
	}
	else
		status = asynPortDriver::writeFloat64Array(pasynUser, value, nElements);
     
	callParamCallbacks();
     
  	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::writeUInt32Digital (asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *name;
	epicsUInt32 val;

	this->getAddress(pasynUser, &addr);
     
	static const char *functionName = "writeUInt32Digital";
     
	this->getAddress(pasynUser, &addr);

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RW)
			{
				val &= ~mask;
				val |= value;
				rw[function]->setVal((uint32_t*)&val, 1);
			}
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s parameter %s set to %d\n", \
						driverName_, functionName, function, this->portName, name, value);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR setting parameter %s to %d\n", \
						driverName_, functionName, function, this->portName, name, value);				
		}

	}
   	else
		status = asynPortDriver::writeUInt32Digital(pasynUser, value, mask);
    
	callParamCallbacks(addr);
 
	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::readUInt32Digital (asynUser *pasynUser, epicsUInt32 *value, epicsUInt32 mask)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	uint32_t u32;
	const char *name;

	this->getAddress(pasynUser, &addr);
     
	static const char *functionName = "readUInt32Digital";

	if (!getParamName(addr, function, &name))
	{
		try
		{
			if (addr == DEV_REG_RO)
			{
				try
				{
					ro[function]->getVal(&u32, 1);
				}
				catch (CPSWError &e)
				{
					status = -1;
					asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
				}
			}
			else if (addr == DEV_REG_RW)
			{
				try
				{
					rw[function]->getVal(&u32, 1);
				}
				catch (CPSWError &e)
				{
					status = -1;
					asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
				}
			}
			else
				status = -1;
		}
		catch (CPSWError &e)
		{
			status = -1;
			asynPrint(pasynUser, ASYN_TRACE_ERROR, "CPSW Error (during %s, parameter: %s): %s\n", functionName, name, e.getInfo().c_str());
		}

		if (status == 0)
		{
			u32 &= mask;
			*value = (epicsInt32)u32;
			setUIntDigitalParam(addr, function, (epicsUInt32)u32, mask);

			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
						"%s:%s(%d), port %s read %d from register %s\n", \
						driverName_, functionName, function, this->portName, *value, name);
		}
		else
		{
			asynPrint(pasynUser, ASYN_TRACE_ERROR, \
						"%s:%s(%d), port %s ERROR reading register %s\n", \
								driverName_, functionName, function, this->portName, name);
		}
	}
	else
		status = asynPortDriver::readUInt32Digital(pasynUser, value, mask);
     
	callParamCallbacks(addr);
     
  	return (status==0) ? asynSuccess : asynError;
}

asynStatus YCPSWASYN::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
	int addr;
	int status = 0;
	int function = pasynUser->reason;
    this->getAddress(pasynUser, &addr);
	const char *name;
	static const char *functionName = "getBounds";

	if (!getParamName(addr, function, &name))
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, \
					"%s:%s(%d), port %s getRegister %s\n", \
					driverName_, functionName, function, this->portName, name);
	
	status = asynPortDriver::getBounds(pasynUser, low, high);
	
	return (status==0) ? asynSuccess : asynError;

}


void YCPSWASYN::report(FILE *fp, int details)
{
	fprintf(fp, "  Port: %s\n", this->portName);
	asynPortDriver::report(fp, details);
}

/////////////////////////////////////////////
// - Methods overrided from asynPortDriver //
/////////////////////////////////////////////

extern "C" int YCPSWASYNConfig(const char *portName, const char *yaml_doc, const char *ipAddr, const char *recordPrefix, int recordNameLenMax)
{
	int status; 
	Path p;

	if (recordNameLenMax <= (strlen(recordPrefix) + 4))
	{
		printf("ERROR! Record name length (%d) must be greater lenght of prefix (%zu) + 4\n\n", recordNameLenMax, strlen(recordPrefix));
		return asynError;
	}


	status = YCPSWASYN::YCPSWASYNInit(yaml_doc, &p, ipAddr);
  
	YCPSWASYN *pYCPSWASYN = new YCPSWASYN(portName, p, recordPrefix, recordNameLenMax);
	pYCPSWASYN = NULL;
    
	return (status==0) ? asynSuccess : asynError;
}

static const iocshArg confArg0 =	{ "portName",			iocshArgString};
static const iocshArg confArg1 =	{ "yamlDoc",			iocshArgString};
static const iocshArg confArg2 =	{ "ipAddr",				iocshArgString};
static const iocshArg confArg3 =	{ "recordPrefix",		iocshArgString};
static const iocshArg confArg4 =	{ "recordNameLenMax",	iocshArgInt};

static const iocshArg * const confArgs[] = {
	&confArg0,
	&confArg1,
	&confArg2,
	&confArg3,
	&confArg4
};
                                            
static const iocshFuncDef configFuncDef = {"YCPSWASYNConfig",5,confArgs};

static void configCallFunc(const iocshArgBuf *args)
{
	YCPSWASYNConfig(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].ival);
}

void drvYCPSWASYNRegister(void)
{
	iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
	epicsExportRegistrar(drvYCPSWASYNRegister);
}
