#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#include <boost/array.hpp>
#include <cpsw_api_builder.h>
#include <cpsw_api_user.h>
#include <cpsw_yaml.h>
#include <yaml-cpp/yaml.h>

#include "yaml2edm.h"
#include "edmTemplates.h"

using std::string;
using std::stringstream;


long nDevices, nRO, nRW, nCMD, nSTM;

FILE *edmFile[MAX_NUM_FILES];
int fileIndex = 0;
int menuLevel = 0;

//int *y_pos_status = (int*)calloc(MAX_NUM_FILES, sizeof(int));
//int *y_pos_config = (int*)calloc(MAX_NUM_FILES, sizeof(int));
int y_pos_status[MAX_NUM_FILES];
int y_pos_config[MAX_NUM_FILES];
int y_pos_menu;

FILE *menuFile;

void generateEDM(Path p)
{
	Path p_aux = p->clone();
	Child c_aux = p_aux->tail();
	Hub h_aux;
	string file_name;
	stringstream c_name;
	string 	rec_name;



	if (c_aux)
		h_aux = c_aux->isHub();
	else
		h_aux = p->origin();

	Children 		c = h_aux->getChildren();
	int 			n = c->size();

	for (int i = 0 ; i < n ; i++)
	{
		try 
		{
			Hub 	h2 = (*c)[i]->isHub();
			int 	m = (*c)[i]->getNelms();
			Path 	p2;

			if (h2)
			{
				menuLevel++;


				if (m > 1)
				{
					for (int j = 0 ; j < m ; j++)
					{
						c_name.str("");
						c_name << fileIndex << "_" << (*c)[i]->getName() << "_" << j;

						file_name.clear();
						file_name = c_name.str() + string(".edl");

						insertObject(menuFile, RELATEDDISPLAY_OBJ, file_name.c_str(), MENU_X0 + menuLevel*MENU_X_STEP, y_pos_menu);
						y_pos_menu += MENU_Y_STEP; 

						file_name = string("edm/") + file_name;

						fileIndex++;
						edmFile[fileIndex] = fopen(file_name.c_str(), "w");
						
						insertHeader(edmFile[fileIndex], WINDOWS_W, WINDOWS_H);
						insertObject(edmFile[fileIndex], TITLE_OBJ, "Status", 25, WINDOWS_Y0);
						insertObject(edmFile[fileIndex], TITLE_OBJ, "Configuration", 435, WINDOWS_Y0);
						y_pos_status[fileIndex] = WINDOWS_Y0 + WINDOWS_Y_STEP;
						y_pos_config[fileIndex] = WINDOWS_Y0 + WINDOWS_Y_STEP;

						c_name.str("");
						c_name << (*c)[i]->getName() << "[" << j << "]";
						p2 = p->findByName(c_name.str().c_str());
						generateEDM(p2);
					}
				}
				else
				{
					c_name.str("");
					c_name << fileIndex << "_" << (*c)[i]->getName();

					file_name.clear();
					file_name = c_name.str() + string(".edl");

					insertObject(menuFile, RELATEDDISPLAY_OBJ, file_name.c_str(), MENU_X0 + menuLevel*MENU_X_STEP, y_pos_menu);
					y_pos_menu += MENU_Y_STEP; 

					file_name = string("edm/") + file_name;

					fileIndex++;
					edmFile[fileIndex] = fopen(file_name.c_str(), "w");

					insertHeader(edmFile[fileIndex], WINDOWS_W, WINDOWS_H);
					insertObject(edmFile[fileIndex], TITLE_OBJ, "Status", 25, WINDOWS_Y0);
					insertObject(edmFile[fileIndex], TITLE_OBJ, "Configuration", 435, WINDOWS_Y0);
					y_pos_status[fileIndex] = WINDOWS_Y0 + WINDOWS_Y_STEP;
					y_pos_config[fileIndex] = WINDOWS_Y0 + WINDOWS_Y_STEP;

					p2 = p->findByName((*c)[i]->getName());
					generateEDM(p2);
				}

				fclose(edmFile[fileIndex]);
				fileIndex--;
				menuLevel--;
			}
			else
			{


				p2 = p->findByName((*c)[i]->getName());

				nDevices++;

				try 
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

							if(edmFile[fileIndex])
							{
						
								rec_name.clear();
								rec_name = "$(P):" + generatePrefix(p) + (*c)[i]->getName() + ":St";					

								insertObject(edmFile[fileIndex], LABEL_OBJ, (*c)[i]->getName(), WINDOWS_X2, y_pos_config[fileIndex]);
								insertObject(edmFile[fileIndex], TEXTENTRY_OBJ, rec_name.c_str(), WINDOWS_X3, y_pos_config[fileIndex]);
								rec_name.replace(rec_name.end()-3, rec_name.end(), ":Rd");
								insertObject(edmFile[fileIndex], TEXTUPDATE_OBJ, rec_name.c_str(), WINDOWS_X4, y_pos_config[fileIndex]);
								//*y_pos_config += 50; 
								y_pos_config[fileIndex] += WINDOWS_Y_STEP;
							}
							nRW++;


						}
						
						else if (ro_aux)
						{
							if(edmFile[fileIndex])
							{																											
								rec_name.clear();
								rec_name = "$(P):" + generatePrefix(p) + (*c)[i]->getName() + ":Rd";

								insertObject(edmFile[fileIndex], LABEL_OBJ, (*c)[i]->getName(), WINDOWS_X0, y_pos_status[fileIndex]);
								insertObject(edmFile[fileIndex], TEXTUPDATE_OBJ, rec_name.c_str(), WINDOWS_X1, y_pos_status[fileIndex]);
								//*y_pos_status += 50; 
								y_pos_status[fileIndex] += WINDOWS_Y_STEP;
						
							}
							nRO++;							
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
	//y_pos_status--;

	return;
}

string generatePrefix(Path p)
{
	string p_str = p->toString();
	std::size_t found; 

	if ((found = p_str.find(BAY0_KEY)) != std::string::npos)
		return (std::string(BAY0_SUBS) + ":" + trimPath(p, found));


	if ((found = p_str.find(BAY1_KEY)) != std::string::npos)
		return (std::string(BAY1_SUBS) + ":" + trimPath(p, found));

	if ((found = p_str.find(CARRIER_KEY)) != std::string::npos)
		return (std::string(CARRIER_SUBS) + ":" + trimPath(p, found));	


	if ((found = p_str.find(APP_KEY)) != std::string::npos)
		return (std::string(APP_SUBS) + ":" + trimPath(p, found));

	return std::string();
}

string trimPath(Path p, size_t pos)
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
			aux_str += "_" + c_str.substr(found_bracket+1, c_str.length() - found_bracket - 2);

		pre_str = aux_str + ":" + pre_str;

	}

	return pre_str;
}

void replaceKey(string *str, const char *key, const char *value)
{
	size_t key_found;

	while ((key_found = (*str).find(key)) != string::npos)
		(*str).replace(key_found, strlen(key), value); 

	//size_t key_found = (*str).find(key);
	//(*str).replace(key_found, strlen(key), value); 

	return;
}

int insertObject(FILE *f, int obj_type, const char *name, int x, int y)
{
	string template_aux;
	char buf[5];

	switch (obj_type)
	{
		case  TITLE_OBJ:
			template_aux.assign(title_template);
			break;
		case  LABEL_OBJ:
			template_aux.assign(label_template);
			break;
		case TEXTUPDATE_OBJ:
			template_aux.assign(textUpdate_template);
			break;
		case TEXTENTRY_OBJ:
			template_aux.assign(textEntry_template);
			break;
		case RELATEDDISPLAY_OBJ:
			template_aux.assign(relatedDisplay_template);
			break;
		default:
			return -1;
	}

	replaceKey(&template_aux, OBJ_KEY, name);

	sprintf(buf,  "%d", x);
	replaceKey(&template_aux, X_KEY, buf);

	sprintf(buf,  "%d", y);
	replaceKey(&template_aux, Y_KEY, buf);

	fprintf(f, "%s", template_aux.c_str());

	return 0;
}

int insertHeader(FILE *f, int w, int h)
{
	char buf[5];
	string template_aux;

	template_aux.assign(header_template);

	sprintf(buf,  "%d", w);
	replaceKey(&template_aux, W_KEY, buf);

	sprintf(buf,  "%d", h);
	replaceKey(&template_aux, H_KEY, buf);

	fprintf(f, "%s", template_aux.c_str());

	return 0;

}

int main(int argc, char *argv[])
{
	const char *yaml_doc = "system.yaml";
	Path p;


	YAML::Node doc =  YAML::LoadFile( yaml_doc );
	NetIODev  root = doc.as<NetIODev>();

	p = IPath::create( root );

	//p = p->findByName("/mmio/AmcCarrierSsrlRtmEth/AmcCarrierCore");
	p = p->findByName("/mmio");

	nDevices = 0;
	nRO = 0;
	nRW =0;

	menuFile = fopen("edm/menu.edl", "w");
	insertHeader(menuFile, MENU_W, MENU_H);
	y_pos_menu = MENU_Y0;

	generateEDM(p);

	fclose(menuFile);

	printf("nDevices = %ld\nnRO = %ld\nnRW = %ld\nnCMD = %ld\nnSTM = %ld\n", nDevices, nRO, nRW, nCMD, nSTM);

	return 0;
}
