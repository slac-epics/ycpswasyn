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

long nDevices, nRO, nRW, nCMD, nSTM;

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
	recordNameLenMax_(recordNameLenMax)
{
	//const char *functionName = "YCPSWASYN";

	nDevices = 0;
	nRO = 0;
	nRW =0;
	nCMD = 0;
	nSTM = 0;

	generateDB(p);
	
	printf("nDevices = %ld\nnRO = %ld\nnRW = %ld\nnCMD = %ld\nnSTM = %ld\n", nDevices, nRO, nRW, nCMD, nSTM);
}

///////////////////////////////////
// + Stream acquisition routines //
///////////////////////////////////
void streamTask(ThreadArgs *arglist)
{
    YCPSWASYN *pYCPSWASYN = (YCPSWASYN *)arglist->pPvt;
    pYCPSWASYN->streamTask(arglist->stm, arglist->param16index, arglist->param32index);
}

void YCPSWASYN::streamTask(Stream stm, int param16index, int param32index)
{
	//int i;
	uint64_t got = 0;
	size_t nWords16, nWords32, nBytes;
	int nFrame;
	uint8_t *buf = new uint8_t[STREAM_MAX_SIZE];
	//size_t n = STREAM_MAX_SIZE;
	//epicsInt16 *buffer16 = (epicsInt16 *)calloc(n, sizeof(epicsInt16));
	//epicsInt32 *buffer32 = (epicsInt32 *)calloc(n, sizeof(epicsInt32));
	//epicsInt16 *pData16;
	//epicsInt32 *pData32;

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

		        //pData16 = (epicsInt16*)(buf+8);
		        //std::copy(pData16, pData16 + nWords16 - 1, buffer16);

		        //pData32 = (epicsInt32*)(buf+8);
		        //std::copy(pData32, pData32 + nWords32 - 1, buffer32);
        		
			//doCallbacksInt16Array(buffer16, nWords16, param16index, DEV_STM); 	
			//doCallbacksInt32Array(buffer32, nWords32, param32index, DEV_STM); 	
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

int YCPSWASYN::YCPSWASYNInit(const char *yaml_doc, Path *p, const char *ipAddr)
{
	unsigned char buf[sizeof(struct in6_addr)];

	YAML::Node doc =  YAML::LoadFile( yaml_doc );

	if (inet_pton(AF_INET, ipAddr, buf))
	{
		printf("Using IP address: %s\n", ipAddr);
		doc["ipAddr"] = ipAddr;
	}
	else
		printf("Using IP address from YAML file\n");

	NetIODev  root = doc.as<NetIODev>();

	*p = IPath::create( root );

	return 0;
}

void YCPSWASYN::printChildrenPath(Path p)
{
	Path p_aux = p->clone();
	Child c_aux = p_aux->tail();
	Hub h_aux;

	if (c_aux)
		h_aux = c_aux->isHub();
	else
		h_aux = p->origin();

	
	Children 	c = h_aux->getChildren();
	int 		n = c->size();
	
	for (int i = 0 ; i < n ; i++)
	{
		Hub h2 = (*c)[i]->isHub();
		if (h2)
		{
			Path p2 = p->findByName((*c)[i]->getName());
			YCPSWASYN::printChildrenPath(p2);
		}
		else
		{
			printf("%s/%s\n", p->toString().c_str(), (*c)[i]->getName());			
		}
	}

	return;
}

void YCPSWASYN::printChildren(Hub h)
{
	Children 	c = h->getChildren();
	int 		n = c->size();

	for (int i = 0 ; i < n ; i++, nDevices++)
	{
		Hub h2 = (*c)[i]->isHub();
		if (h2)
		{
			YCPSWASYN::printChildren(h2);
		}
		else
		{
			printf("%s\n", (*c)[i]->getName());

		}
	}
	return;
}

void YCPSWASYN::generateDB(Path p)
{
	Path p_aux = p->clone();
	Child c_aux = p_aux->tail();
	Hub h_aux;

	if (c_aux)
		h_aux = c_aux->isHub();
	else
		h_aux = p->origin();

	Children 		c = h_aux->getChildren();
	int 			n = c->size();
	stringstream 	db_params, param_name;
	string 			rec_name;

	for (int i = 0 ; i < n ; i++)
	{
		try 
		{
			Hub 	h2 = (*c)[i]->isHub();
			int 	m = (*c)[i]->getNelms();
			Path 	p2;

			if (h2)
			{
				
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

				nDevices++;

				size_t found_key = string((*c)[i]->getName()).find(STREAM_KEY);

				try 
				{
					if ((found_key != std::string::npos) && (isdigit(((*c)[i]->getName())[found_key+strlen(STREAM_KEY)])))
					{
						Stream stm_aux;
						int p16StmIndex, p32StmIndex;

						try
						{
							stm_aux = IStream::create(p2);
						}
						catch (CPSWError &e)
						{
						}

						if (stm_aux)
						{	
							printf("Stream [%s] found on path: %s\n", (*c)[i]->getName(), p->toString().c_str());
						 	
							// Create PVs for 16-bit stream data
							param_name.str("");
						 	param_name << string((*c)[i]->getName()).substr(0, 10);
						 	param_name << "_STM16_" << nSTM;

						 	rec_name.clear();
						 	rec_name = YCPSWASYN::generatePrefix(p);
						 	rec_name += (*c)[i]->getName();
						 	rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4);
						 	rec_name += ":16" ;

							db_params.str("");
							db_params << "PORT=" << portName_;
							db_params << ",ADDR=" << DEV_STM;
							db_params << ",P=" << recordPrefix_;
							db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
							db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";
							//db_params << ",N=10000";

							createParam(DEV_STM, param_name.str().c_str(), asynParamInt16Array, &p16StmIndex);
							dbLoadRecords("../../db/waveform_stream16.template", db_params.str().c_str());

							// Create PVs for 32-bit stream data
							param_name.str("");
							param_name << string((*c)[i]->getName()).substr(0, 10);
							param_name << "_STM32_" << nSTM;

							rec_name.clear();
							rec_name = YCPSWASYN::generatePrefix(p);
							rec_name += (*c)[i]->getName();
							rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4);
							rec_name += ":32" ;

							db_params.str("");
							db_params << "PORT=" << portName_;
							db_params << ",ADDR=" << DEV_STM;
							db_params << ",P=" << recordPrefix_;
							db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
							db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";

							createParam(DEV_STM, param_name.str().c_str(), asynParamInt32Array, &p32StmIndex);	
							dbLoadRecords("../../db/waveform_stream32.template", db_params.str().c_str());

							// Crteate Acquisition Thread 
							asynStatus status;
							ThreadArgs arglist;
							arglist.pPvt = this;
							arglist.stm = stm_aux;
							arglist.param16index = p16StmIndex;
							arglist.param32index = p32StmIndex;
						
							status = (asynStatus)(epicsThreadCreate("Stream", epicsThreadPriorityLow, 
    						        epicsThreadGetStackSize(epicsThreadStackMedium), (EPICSTHREADFUNC)::streamTask, &arglist) == NULL);
    						 
    						if (status) 
    						{
    						    printf("epicsThreadCreate failure\n");
    						    return;
    						}
    						else
    							printf("epicsThreadCreate successfully\n");

							nSTM++;
						}
					}
					else
					{
						ScalVal		rw_aux;
						ScalVal_RO 	ro_aux;
						Command		cmd_aux;

						try 
						{
							ro_aux = IScalVal_RO::create(p2);
							rw_aux = IScalVal::create(p2);		
						}
						catch (CPSWError &e)
						{
						}
						
						try 
						{
							cmd_aux = ICommand::create(p2);							
						}
						catch (CPSWError &e)
						{
						}

						if (rw_aux)
						{
							long nBits      = rw_aux->getSizeBits();
							Enum isEnum	= rw_aux->getEnum();
							int nElements = rw_aux->getNelms();

							param_name.str("");
							param_name <<  string((*c)[i]->getName()).substr(0, 10);
							param_name << "_RW_" << nRW;

							rec_name.clear();
							rec_name = YCPSWASYN::generatePrefix(p);
							rec_name += (*c)[i]->getName();
							rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4);
							rec_name += ":St";

							db_params.str("");
							db_params << "PORT=" << portName_;
							db_params << ",ADDR=" << DEV_REG_RW;
							db_params << ",P=" << recordPrefix_;
							db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
							db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";

							if (isEnum)
							{
								int nValues = isEnum->getNelms();
								int mBits = log2(nValues);

								if (nElements == 1)
								{
									if (nValues > DB_MBBX_NELEM_MAX)
									{
										printf("%s has %d elements, mmbbo record only supports %d. Loaded an ao record instead\n", (*c)[i]->getName(), nValues, DB_MBBX_NELEM_MAX);
										createParam(DEV_REG_RW, param_name.str().c_str(), asynParamUInt32Digital, &pRwIndex);
										dbLoadRecords("../../db/ao.template", db_params.str().c_str());
										rw[pRwIndex] = rw_aux;
										nRW++;
									}
									else
									{
										db_params << ",MASK=" << ((1 << nBits) - 1);
										db_params << ",NOBT=" << nBits;

										IEnum::iterator it;
										int k;

										for (it = isEnum->begin(), k = 0 ; k < DB_MBBX_NELEM_MAX ; k++)
										{
											if (it != isEnum->end())
											{
												db_params << "," << mbbxNameParams[k] << "=" << (*it).first->c_str();
												db_params << "," << mbbxValParam[k] << "=" << (*it).second;
												++it;
											}
											else
											{
												db_params << "," << mbbxNameParams[k] << "=";
												db_params << "," << mbbxValParam[k] << "=";
											}
										}

										createParam(DEV_REG_RW, param_name.str().c_str(), asynParamUInt32Digital, &pRwIndex);
										dbLoadRecords("../../db/mbbo.template", db_params.str().c_str());
										rw[pRwIndex] = rw_aux;
										nRW++;
									}
	
								}
								else
								{
									if (nValues > DB_MBBX_NELEM_MAX)
									{
										printf("%s has %d elements, mmbbo record only supports %d. Loaded a waveform record instead\n", (*c)[i]->getName(), nValues, DB_MBBX_NELEM_MAX);
										db_params << ",N=" << nElements;
										if (nBits <= 8)
										{
											createParam(DEV_REG_RW, param_name.str().c_str(), asynParamOctet, &pRwIndex);
											dbLoadRecords("../../db/waveform_8_out.template", db_params.str().c_str());
										}
										else
										{
											createParam(DEV_REG_RW, param_name.str().c_str(), asynParamInt32Array, &pRwIndex);
											dbLoadRecords("../../db/waveform_out.template", db_params.str().c_str());
										}
										rw[pRwIndex] = rw_aux;
										nRW++;
									}
									else
									{
										stringstream index_aux;
										string c_name;

                                        for (int j = 0 ; j < nElements ; j++)
                                        {	
                                        	index_aux.str("");
                                        	index_aux << j;
											
                                        	c_name.clear();
                                        	c_name = (*c)[i]->getName() + string("[") + index_aux.str() + string("]");
                                        	Path c_path = p->findByName(c_name.c_str());
											ScalVal c_rw = IScalVal::create(c_path);
										
											param_name.str("");
                                        	param_name << string((*c)[i]->getName()).substr(0, 10);
                                        	param_name << "_RW_" << nRW;
											
                                        	rec_name.clear();
                                        	rec_name = YCPSWASYN::generatePrefix(p);
                                        	rec_name += (*c)[i]->getName();
                                        	rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4 - 2);
                                        	rec_name += index_aux.str();
                                        	rec_name += ":St";
                                       		db_params.str("");
                                       		db_params << "PORT=" << portName_;
                                       		db_params << ",ADDR=" << DEV_REG_RO;
                                       		db_params << ",P=" << recordPrefix_;
                                       		db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
                                       		db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";
											db_params << ",MASK=" << ((1 << nBits) - 1);
                                        	db_params << ",NOBT=" << nBits;

                                        	IEnum::iterator it;
                                        	int k;

                                        	for (it = isEnum->begin(), k = 0 ; k < DB_MBBX_NELEM_MAX ; k++)
                                        	{
                                        	    if (it != isEnum->end())
                                        	    {
													db_params << "," << mbbxNameParams[k] << "=" << (*it).first->c_str();
                                        	        db_params << "," << mbbxValParam[k] << "=" << (*it).second;
                                        	        ++it;
                                        	    }
                                        	    else
                                        	    {
                                        	        db_params << "," << mbbxNameParams[k] << "=";
                                        	        db_params << "," << mbbxValParam[k] << "=";
                                        	    }
	
                                        	}
											
											createParam(DEV_REG_RW, param_name.str().c_str(), asynParamUInt32Digital, &pRwIndex);
                                        	dbLoadRecords("../../db/mbbo.template", db_params.str().c_str());
											rw[pRwIndex] = c_rw;
											nRW++;
										}
									}
								}
							}
							else
							{
								if (nElements == 1)
								{
	
									createParam(DEV_REG_RW, param_name.str().c_str(), asynParamInt32, &pRwIndex);	
									dbLoadRecords("../../db/ao.template", db_params.str().c_str());
								}
								else 
								{						
									db_params << ",N=" << nElements;
									if (nBits <= 8)
									{
										createParam(DEV_REG_RW, param_name.str().c_str(), asynParamOctet, &pRwIndex);	
										dbLoadRecords("../../db/waveform_8_out.template", db_params.str().c_str());
									}
									else
									{
										createParam(DEV_REG_RW, param_name.str().c_str(), asynParamInt32Array, &pRwIndex);	
										dbLoadRecords("../../db/waveform_out.template", db_params.str().c_str());
									}
								}
								rw[pRwIndex] = rw_aux;
								nRW++;
							}
						}
						
						if (ro_aux)
						{

							long nBits	= ro_aux->getSizeBits();
							Enum isEnum	= ro_aux->getEnum();
							int nElements = ro_aux->getNelms();

							param_name.str("");
							param_name << string((*c)[i]->getName()).substr(0, 10);
							param_name << "_RO_" << nRO;
														
							rec_name.clear();
							rec_name = YCPSWASYN::generatePrefix(p);
							rec_name += (*c)[i]->getName();
							rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4);
							rec_name += ":Rd";
							db_params.str("");
							db_params << "PORT=" << portName_;
							db_params << ",ADDR=" << DEV_REG_RO;
							db_params << ",P=" << recordPrefix_;
							db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
							db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";

							if (isEnum)
							{
								int nValues = isEnum->getNelms();
								int mBits = log2(nValues);

								if (nElements == 1)
								{
									if (nValues > DB_MBBX_NELEM_MAX)
									{
										printf("%s has %d elements, mmbbi record only supports %d. Loaded an ai record instead\n", (*c)[i]->getName(), nValues, DB_MBBX_NELEM_MAX);
										createParam(DEV_REG_RO, param_name.str().c_str(), asynParamUInt32Digital, &pRoIndex);
										dbLoadRecords("../../db/ai.template", db_params.str().c_str());
										ro[pRoIndex] = ro_aux;
										nRO++;
									}
									else
									{
										db_params << ",MASK=" << ((1 << nBits) - 1);
										db_params << ",NOBT=" << nBits;

										IEnum::iterator it;
										int k;


										for (it = isEnum->begin(), k = 0 ; k < DB_MBBX_NELEM_MAX ; k++)
										{
											if (it != isEnum->end())
											{
												db_params << "," << mbbxNameParams[k] << "=" << (*it).first->c_str();
												db_params << "," << mbbxValParam[k] << "=" << (*it).second;
												++it;
											}
											else
											{
												db_params << "," << mbbxNameParams[k] << "=";
												db_params << "," << mbbxValParam[k] << "=";
											}
										}

										createParam(DEV_REG_RO, param_name.str().c_str(), asynParamUInt32Digital, &pRoIndex);
										dbLoadRecords("../../db/mbbi.template", db_params.str().c_str());
										ro[pRoIndex] = ro_aux;
										nRO++;
									}
	
								}
								else
								{
									if (nValues > DB_MBBX_NELEM_MAX)
									{
										printf("%s has %d elements, mmbbi record only supports %d. Loaded a waveform record instead\n", (*c)[i]->getName(), nValues, DB_MBBX_NELEM_MAX);
										db_params << ",N=" << nElements;
										if (nBits <= 8)
										{
											createParam(DEV_REG_RO, param_name.str().c_str(), asynParamOctet, &pRoIndex);
											dbLoadRecords("../../db/waveform_8_in.template", db_params.str().c_str());
										}
										else
										{
											createParam(DEV_REG_RO, param_name.str().c_str(), asynParamInt32Array, &pRoIndex);
											dbLoadRecords("../../db/waveform_in.template", db_params.str().c_str());
										}
										ro[pRoIndex] = ro_aux;
										nRO++;
									}
									else
									{
										stringstream index_aux;
										string c_name;

                                        for (int j = 0 ; j < nElements ; j++)
                                        {	
                                        	index_aux.str("");
                                        	index_aux << j;
											
                                        	c_name.clear();
                                        	c_name = (*c)[i]->getName() + string("[") + index_aux.str() + string("]");
                                        	Path c_path = p->findByName(c_name.c_str());
											ScalVal_RO c_ro = IScalVal_RO::create(c_path);
										
											param_name.str("");
                                        	param_name << string((*c)[i]->getName()).substr(0, 10);
                                        	param_name << "_RO_" << nRO;
											
                                        	rec_name.clear();
                                        	rec_name = YCPSWASYN::generatePrefix(p);
                                        	rec_name += (*c)[i]->getName();
                                        	rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4 - 2);
                                        	rec_name += index_aux.str();
                                        	rec_name += ":Rd";
                                       		db_params.str("");
                                       		db_params << "PORT=" << portName_;
                                       		db_params << ",ADDR=" << DEV_REG_RO;
                                       		db_params << ",P=" << recordPrefix_;
                                       		db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
                                       		db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";
											db_params << ",MASK=" << ((1 << nBits) - 1);
                                        	db_params << ",NOBT=" << nBits;

                                        	IEnum::iterator it;
                                        	int k;

                                        	for (it = isEnum->begin(), k = 0 ; k < DB_MBBX_NELEM_MAX ; k++)
                                        	{
                                        	    if (it != isEnum->end())
                                        	    {
													db_params << "," << mbbxNameParams[k] << "=" << (*it).first->c_str();
                                        	        db_params << "," << mbbxValParam[k] << "=" << (*it).second;
                                        	        ++it;
                                        	    }
                                        	    else
                                        	    {
                                        	        db_params << "," << mbbxNameParams[k] << "=";
                                        	        db_params << "," << mbbxValParam[k] << "=";
                                        	    }
	
                                        	}
											
											createParam(DEV_REG_RO, param_name.str().c_str(), asynParamUInt32Digital, &pRoIndex);
                                        	dbLoadRecords("../../db/mbbi.template", db_params.str().c_str());
											ro[pRoIndex] = c_ro;
											nRO++;
										}
									}
								}
							}
							else
							{							
								if (nElements == 1)
								{
									createParam(DEV_REG_RO, param_name.str().c_str(), asynParamInt32, &pRoIndex);	
									dbLoadRecords("../../db/ai.template", db_params.str().c_str());
								}
								else 
								{
									db_params << ",N=" << nElements;
									if (nBits <= 8)
									{
										createParam(DEV_REG_RO, param_name.str().c_str(), asynParamOctet, &pRoIndex);
										dbLoadRecords("../../db/waveform_8_in.template", db_params.str().c_str());
									}
									else
									{
										createParam(DEV_REG_RO, param_name.str().c_str(), asynParamInt32Array, &pRoIndex);
										dbLoadRecords("../../db/waveform_in.template", db_params.str().c_str());
									}
								}

								ro[pRoIndex] = ro_aux;
								nRO++;							
							}
						}

						if (cmd_aux)
						{
							param_name.str("");
							param_name << string((*c)[i]->getName()).substr(0, 10);
							param_name << "_CMD_" << nCMD;

							rec_name.clear();
							rec_name = YCPSWASYN::generatePrefix(p);
							rec_name += (*c)[i]->getName();
							rec_name = rec_name.substr(0, recordNameLenMax_ - strlen(recordPrefix_) - 4);
							rec_name += ":Ex";

							db_params.str("");
							db_params << "PORT=" << portName_;
							db_params << ",ADDR=" << DEV_CMD;
							db_params << ",P=" << recordPrefix_;
							db_params << ",R=" << rec_name << ",PARAM=" << param_name.str();
							db_params << ",DESC=\"" << string((*c)[i]->getDescription()).substr(0, DB_DESC_LENGTH_MAX) << "\"";

							createParam(DEV_CMD, param_name.str().c_str(), asynParamInt32, &pCmdIndex);	
							dbLoadRecords("../../db/ao.template", db_params.str().c_str());
							
							cmd[pCmdIndex]= cmd_aux;
							nCMD++;
						}
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

string YCPSWASYN::generatePrefix(Path p)
{
	string p_str = p->toString();
	std::size_t found; 

	if ((found = p_str.find(BAY0_KEY)) != std::string::npos)
		return (std::string(BAY0_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	if ((found = p_str.find(BAY1_KEY)) != std::string::npos)
		return (std::string(BAY1_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	if ((found = p_str.find(CARRIER_KEY)) != std::string::npos)
		return (std::string(CARRIER_SUBS) + ":" + YCPSWASYN::trimPath(p, found));	


	if ((found = p_str.find(APP_KEY)) != std::string::npos)
		return (std::string(APP_SUBS) + ":" + YCPSWASYN::trimPath(p, found));

	return std::string();
}

string YCPSWASYN::trimPath(Path p, size_t pos)
{
	string p_str = p->toString();
	string c_str, pre_str, aux_str;
	std::size_t found_bracket, found_slash;

	if ((found_slash = p_str.find("/", pos)) == std::string::npos)
		return std::string();

	p_str = p_str.substr(found_slash);

	while ( (found_slash = p_str.find_last_of("/")) != std::string::npos)
	{


		c_str = p_str.substr(found_slash+1);
		p_str = p_str.substr(0, found_slash);
		aux_str = c_str.substr(0,3);

		if ((found_bracket = c_str.find('[')) != std::string::npos)
			aux_str += c_str.substr(found_bracket+1, c_str.length() - found_bracket - 2);

		pre_str = aux_str + ":" + pre_str;

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
