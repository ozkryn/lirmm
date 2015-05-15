/*****************************************************************************
 *
 *  $Id: main.cpp,v 960cc1bb6b4a 2012/10/24 16:21:52 fp $
 *
 *  Copyright (C) 2006-2009  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  The license mentioned above concerns the source code only. Using the
 *  EtherCAT technology and brand is only permitted in compliance with the
 *  industrial property and similar rights of Beckhoff Automation GmbH.
 *
 ****************************************************************************/

#include <getopt.h>
#include <libgen.h> // basename()
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
using namespace std;

#include "CommandAlias.h"
#include "CommandConfig.h"
//#include "CommandCStruct.h"
#include "CommandData.h"
#include "CommandDebug.h"
#include "CommandDomains.h"
#include "CommandDownload.h"
//#ifdef EC_EOE
//# include "CommandEoe.h"
//#endif
//#include "CommandFoeRead.h"
//#include "CommandFoeWrite.h"
//#include "CommandGraph.h"
#include "CommandMaster.h"
#include "CommandPdos.h"
//#include "CommandRegRead.h"
//#include "CommandRegWrite.h"
#include "CommandRescan.h"
#include "CommandSdos.h"
//#include "CommandSiiRead.h"
//#include "CommandSiiWrite.h"
#include "CommandSlaves.h"
//#include "CommandSoeRead.h"
//#include "CommandSoeWrite.h"
#include "CommandStates.h"
#include "CommandUpload.h"
//#include "CommandVersion.h"
//#include "CommandXml.h"

#include "MasterDevice.h"
#include <unistd.h>
#include <string>


/*****************************************************************************/

typedef list<Command *> CommandList;
CommandList commandList;

string binaryBaseName;
string commandName;
Command::StringVector commandArgs;

// option variables
string masters = "-"; // all masters
string positions = "-"; // all positions
string aliases = "-"; // all aliases
string domains = "-"; // all domains
string dataTypeStr;
Command::Verbosity verbosity = Command::Normal;
bool force = true;
bool helpRequested = false;
string outputFile;
string skin;
/*****************************************************************************/


int sendUploadCommand(string, string, string, string);
int sendDownloadCommand(string, string, string, string, string);
void initEPOS3();



string usage()
{
    stringstream str;
    CommandList::const_iterator ci;
    size_t maxWidth = 0;

    for (ci = commandList.begin(); ci != commandList.end(); ci++) {
        if ((*ci)->getName().length() > maxWidth) {
            maxWidth = (*ci)->getName().length();
        }
    }

    str << "Usage: " << binaryBaseName << " <COMMAND> [OPTIONS] [ARGUMENTS]"
        << endl << endl
        << "Commands (can be abbreviated):" << endl;

    str << left;
    for (ci = commandList.begin(); ci != commandList.end(); ci++) {
        str << "  " << setw(maxWidth) << (*ci)->getName()
            << "  " << (*ci)->getBriefDescription() << endl;
    }

    str << endl
        << "Global options:" << endl
        << "  --master  -m <master>  Comma separated list of masters" << endl
        << "                         to select, ranges are allowed." << endl
        << "                         Examples: '1,3', '5-7,9', '-3'." << endl
        << "                         Default: '-' (all)."
        << endl
        << "  --force   -f           Force a command." << endl
        << "  --quiet   -q           Output less information." << endl
        << "  --verbose -v           Output more information." << endl
        << "  --help    -h           Show this help." << endl
        << endl
        << Command::numericInfo()
        << endl
        << "Call '" << binaryBaseName
        << " <COMMAND> --help' for command-specific help." << endl
        << endl
        << "Send bug reports to " << PACKAGE_BUGREPORT << "." << endl;

    return str.str();
}

/*****************************************************************************/

void getOptions(int argc, char **argv)
{
    int c, argCount;
    stringstream str;

    static struct option longOptions[] = {
        //name,         has_arg,           flag, val
        {"master",      required_argument, NULL, 'm'},
        {"alias",       required_argument, NULL, 'a'},
        {"position",    required_argument, NULL, 'p'},
        {"domain",      required_argument, NULL, 'd'},
        {"type",        required_argument, NULL, 't'},
        {"output-file", required_argument, NULL, 'o'},
        {"skin",        required_argument, NULL, 's'},
        {"force",       no_argument,       NULL, 'f'},
        {"quiet",       no_argument,       NULL, 'q'},
        {"verbose",     no_argument,       NULL, 'v'},
        {"help",        no_argument,       NULL, 'h'},
        {}
    };

    do {
        c = getopt_long(argc, argv, "m:a:p:d:t:o:s:fqvh", longOptions, NULL);

        switch (c) {
            case 'm':
                masters = optarg;
                break;

            case 'a':
                aliases = optarg;
                break;

            case 'p':
                positions = optarg;
                break;

            case 'd':
                domains = optarg;
                break;

            case 't':
                dataTypeStr = optarg;
                break;

            case 'o':
                outputFile = optarg;
                break;

            case 's':
                skin = optarg;
                break;

            case 'f':
                force = true;
                break;

            case 'q':
                verbosity = Command::Quiet;
                break;

            case 'v':
                verbosity = Command::Verbose;
                break;

            case 'h':
                helpRequested = true;
                break;

            case '?':
                cerr << endl << usage();
                exit(1);

            default:
                break;
        }
    }
    while (c != -1);

    argCount = argc - optind;

    if (!argCount) {
        if (helpRequested) {
            cout << usage();
            exit(0);
        } else {
            cerr << "Please specify a command!" << endl
                << endl << usage();
            exit(1);
        }
    }

    commandName = argv[optind];
    
    cout<< "optind :" <<optind<<endl;
    cout<< "commandName :" <<commandName<<endl; 
    
    while (++optind < argc)
        commandArgs.push_back(string(argv[optind]));
}

/****************************************************************************/

list<Command *> getMatchingCommands(const string &cmdStr)
{
    CommandList::iterator ci;
    list<Command *> res;

    // find matching commands from beginning of the string
    for (ci = commandList.begin(); ci != commandList.end(); ci++) {
        if ((*ci)->matchesSubstr(cmdStr)) {
            res.push_back(*ci);
        }
    }

    if (!res.size()) { // nothing found
        // find /any/ matching commands
        for (ci = commandList.begin(); ci != commandList.end(); ci++) {
            if ((*ci)->matchesAbbrev(cmdStr)) {
                res.push_back(*ci);
            }
        }
    }

    return res;
}

/****************************************************************************/
/*
int main2(int argc, char **argvTemp)
{
	
	char **argv =0;
	argv = argvTemp;
	
		cout<<"main 2"<<endl;
		cout<<"argv[0]:"<<argv[0]<<endl;
		cout<<"arc: "<<argc<<endl;
		//cout << *(argv+0) << endl;			
		
		for(int z=0;z<argc;z++){
			cout << argv[z] << endl;			
		}
		
	
		
		list<Command *> matchingCommands;
		list<Command *>::const_iterator ci;
		Command *cmd;

		binaryBaseName = basename(argv[0]);

		commandList.push_back(new CommandAlias());
		commandList.push_back(new CommandConfig());
		commandList.push_back(new CommandCStruct());
		commandList.push_back(new CommandData());
		commandList.push_back(new CommandDebug());
		commandList.push_back(new CommandDomains());
		commandList.push_back(new CommandDownload());
	#ifdef EC_EOE
		commandList.push_back(new CommandEoe());
	#endif
		commandList.push_back(new CommandFoeRead());
		commandList.push_back(new CommandFoeWrite());
		commandList.push_back(new CommandGraph());
		commandList.push_back(new CommandMaster());
		commandList.push_back(new CommandPdos());
		commandList.push_back(new CommandRegRead());
		commandList.push_back(new CommandRegWrite());
		commandList.push_back(new CommandRescan());
		commandList.push_back(new CommandSdos());
		commandList.push_back(new CommandSiiRead());
		commandList.push_back(new CommandSiiWrite());
		commandList.push_back(new CommandSlaves());
		commandList.push_back(new CommandSoeRead());
		commandList.push_back(new CommandSoeWrite());
		commandList.push_back(new CommandStates());
		commandList.push_back(new CommandUpload());
		commandList.push_back(new CommandVersion());
		commandList.push_back(new CommandXml());

		getOptions(argc, argv);
		
		int retval = 0;
		
		matchingCommands = getMatchingCommands(commandName);
		
		cout <<"matchingCommands.size(): " <<matchingCommands.size()<<endl;
		
		if (matchingCommands.size()) {
			if (matchingCommands.size() == 1) {
				cmd = matchingCommands.front();
				if (!helpRequested) {
					try {
						cmd->setMasters(masters);
						cout<<"cmd->setMasters(masters)"<< masters<<endl;
						cmd->setVerbosity(verbosity);
						cout<<"cmd->setVerbosity(verbosity)"<< verbosity<<endl;
						cmd->setAliases(aliases);
						cout<<"cmd->setAliases(aliases)"<< aliases<<endl;
						cmd->setPositions(positions);
						cout<<"cmd->setPositions(positions)"<< positions<<endl;
						cmd->setDomains(domains);
						cout<<"cmd->setDomains(domains)"<< domains<<endl;
						cmd->setDataType(dataTypeStr);
						cout<<"cmd->setDataType(dataTypeStr)"<< dataTypeStr<<endl;
						cmd->setOutputFile(outputFile);
						cout<<"cmd->setOutputFile(outputFile)"<< outputFile<<endl;
						cmd->setSkin(skin);
						cout<<"cmd->setSkin(skin)"<< skin<<endl;
						cmd->setForce(force);
						cout<<"cmd->setForce(force)"<< force<<endl;
						cmd->execute(commandArgs);
						cout<<"commandArgs.size()"<< commandArgs.size()<<endl;
						cout<<"cmd->execute(commandArgs)"<< commandArgs[0]<<endl;
						cout<<"cmd->execute(commandArgs)"<< commandArgs[1]<<endl;
					} catch (InvalidUsageException &e) {
						cerr << e.what() << endl << endl;
						cerr << cmd->helpString(binaryBaseName);
						retval = 1;
					} catch (CommandException &e) {
						cerr << e.what() << endl;
						retval = 1;
					} catch (MasterDeviceException &e) {
						cerr << e.what() << endl;
						retval = 1;
					}
				} else {
					cout << cmd->helpString(binaryBaseName);
				}
			} else {
				cerr << "Ambiguous command abbreviation! Matching:" << endl;
				for (ci = matchingCommands.begin();
						ci != matchingCommands.end();
						ci++) {
					cerr << (*ci)->getName() << endl;
				}
				cerr << endl << usage();
				retval = 1;
			}
		} else {
			cerr << "Unknown command " << commandName << "!" << endl
				<< endl << usage();
			retval = 1;
		}
		return retval;

}*/
/****************************************************************************/
int main()
{
	int retval = 0;
	int direction = 0;	
	int profileVelocity=0;
	bool returnWhile=false;
	stringstream str;
	string option;
	char temp[11];
	string pos;
	string newVel;
	
	int targerPosition=50000;
	int newtargerPosition=0;
	
	cout<<"main"<<endl;	
	
	cout<<"Press 1 to move to left or 2 to the rigth: "<< option <<endl;
	cout<<"Otion entered: "<< option <<endl;
		
	//Initialization
	
	initEPOS3();
	
	//sendDownloadCommand("0", "0x6060", "0", "uint8","0x03");
	
	//sendDownloadCommand("0", "0x607A", "0", "uint32","50000");
	
	cout<<"Size of int: "<<sizeof(int)<<endl;	
	
	cout<<"*******JAYMI**********************"<<sizeof(int)<<endl;	
	cout<<"Profile Velocity (rpm/s):"<<endl;
	cin>>profileVelocity;	
	cout<<"Target position:"<<endl;
	cin>>targerPosition;
	while(1)
	{
		cout<<"Waiting commands"<<endl;	
		returnWhile=false;
		cin>>direction;
		if (direction ==1)
		{
							
				newtargerPosition = 0xffffffff - targerPosition + 1;
				cout<<"targerPosition: "<<targerPosition<<endl;
		}
		else if(direction==2)
		{
			newtargerPosition = targerPosition;
			cout<<"else if1 "<<endl;
		}
		else if(direction==0)
		{
			break;
			return 0;			
		}
		else if(direction==3)
		{
			cout<<"Profile Velocity (rpm/s):"<<endl;
			cin>>profileVelocity;	
			cout<<"Target position:"<<endl;
			cin>>targerPosition;
		}
		else
		{
			cout<<"Only 1, 2 or 0 options are permitted"<<endl;
			returnWhile=true;
			
		}
		if (!returnWhile)
		{		
			
		cout<<"before sprintf: "<<endl;			
		sprintf(temp,"%x",newtargerPosition);	
		pos = "0x" + string(temp);
		cout<<"pos: "<<pos<<endl;	
		
		
		sprintf(temp,"%x",profileVelocity);	
		newVel = "0x" + string(temp);
		cout<<"newVel: "<<newVel<<endl;
		
		sendDownloadCommand("0", "0x607A", "0", "uint32",pos);
		sendDownloadCommand("0", "0x6081", "0", "uint32",newVel);										
		sendDownloadCommand("0", "0x6040", "0", "uint16","0x006f");
		sendUploadCommand("0", "0x6041", "0", "uint16");
		sendDownloadCommand("0", "0x6040", "0", "uint16","0x007f");	
		sendUploadCommand("0", "0x6041", "0", "uint16");		
		}
		
	}
	return retval;
}

/****************************************************************************/
void initEPOS3()
{
	cout<<"Initializing ESPO3"<<endl;	
	
	sendDownloadCommand("0", "0x6040", "0", "uint16","0x00ff");
	sendUploadCommand("0", "0x6041", "0", "uint16");		
	sendDownloadCommand("0", "0x6040", "0", "uint16","0x0006");
	sendUploadCommand("0", "0x6041", "0", "uint16");
	sendDownloadCommand("0", "0x6040", "0", "uint16","0x0007");
	sendUploadCommand("0", "0x6041", "0", "uint16");
	sendDownloadCommand("0", "0x6040", "0", "uint16","0x000f");
	sendUploadCommand("0", "0x6041", "0", "uint16");
}

/****************************************************************************/

int sendUploadCommand(string positionStr, string regStr, string indexStr, string dataTypeStr)
{
	int retval = 0;
	commandArgs.clear();
	
	commandName = "upload";
	commandArgs.push_back(regStr);
	commandArgs.push_back(indexStr);
	
	cout<<"sending Upload command..."<<endl;

			
		//list<Command *> matchingCommands;
		CommandUpload *cmd;

		/*commandList.push_back(new CommandAlias());				
		commandList.push_back(new CommandDomains());
		commandList.push_back(new CommandMaster());
		commandList.push_back(new CommandRescan());
		commandList.push_back(new CommandSlaves());
		commandList.push_back(new CommandStates());
		
		commandList.push_back(new CommandDownload());
		commandList.push_back(new CommandUpload());*/
		
		cout<<"ckeck1"<< positions<<endl;						
		
						
		/*matchingCommands = getMatchingCommands(commandName);
		cmd = matchingCommands.front();*/
		cmd = new CommandUpload() ;	
					try {
						cout<<"ckeck2:"<< positions<<endl;	
						cmd->setMasters(masters);
						cout<<"cmd->setMasters(masters):"<< masters<<endl;
						cmd->setVerbosity(verbosity);
						cout<<"cmd->setVerbosity(verbosity):"<< verbosity<<endl;
						cmd->setAliases(aliases);
						cout<<"cmd->setAliases(aliases):"<< aliases<<endl;
						cout<<"ckeck3"<<endl;	
						cmd->setPositions(positions);
						cout<<"cmd->setPositions(positions):"<< positions<<endl;
						cmd->setDomains(domains);
						cout<<"cmd->setDomains(domains):"<< domains<<endl;
						cmd->setDataType(dataTypeStr);
						cout<<"cmd->setDataType(dataTypeStr):"<< dataTypeStr<<endl;
						cmd->setOutputFile(outputFile);
						cout<<"cmd->setOutputFile(outputFile):"<< outputFile<<endl;
						cmd->setSkin(skin);
						cout<<"cmd->setSkin(skin):"<< skin<<endl;
						force=true;
						cmd->setForce(force);
						cout<<"cmd->setForce(force):"<< force<<endl;																	
						cmd->execute(commandArgs);
						cout<<"commandArgs.size():"<< commandArgs.size()<<endl;
						cout<<"cmd->execute(commandArgs):"<< commandArgs[0]<<endl;
						cout<<"cmd->execute(commandArgs):"<< commandArgs[1]<<endl;
						cout<<"cmd->getRegvalue():"<< (cmd->getRegValue())<<endl;
					} catch (InvalidUsageException &e) {
						cerr << e.what() << endl << endl;
						retval = 1;
					}					
					
		return retval;
}
/****************************************************************************/

int sendDownloadCommand(string positionStr, string regStr, string indexStr, string dataTypeStr, string valuesStr)
{
	int retval = 0;
	commandArgs.clear();
	
	commandName = "download";
	commandArgs.push_back(regStr);
	commandArgs.push_back(indexStr);
	commandArgs.push_back(valuesStr);
	
	cout<<"sending download command..."<<endl;
		
		list<Command *> matchingCommands;
		Command *cmd;	

		commandList.push_back(new CommandAlias());				
		commandList.push_back(new CommandDomains());
		commandList.push_back(new CommandMaster());
		commandList.push_back(new CommandRescan());
		commandList.push_back(new CommandSlaves());
		commandList.push_back(new CommandStates());
		
		commandList.push_back(new CommandDownload());
		commandList.push_back(new CommandUpload());
		
		cout<<"ckeck1"<< positions<<endl;						
								
		matchingCommands = getMatchingCommands(commandName);
		cmd = matchingCommands.front();
					try {
						cout<<"ckeck2"<< positions<<endl;	
						cmd->setMasters(masters);
						cout<<"cmd->setMasters(masters)"<< masters<<endl;
						cmd->setVerbosity(verbosity);
						cout<<"cmd->setVerbosity(verbosity)"<< verbosity<<endl;
						cmd->setAliases(aliases);
						cout<<"cmd->setAliases(aliases)"<< aliases<<endl;
						cout<<"ckeck3"<<endl;	
						cmd->setPositions(positions);
						cout<<"cmd->setPositions(positions)"<< positions<<endl;
						cmd->setDomains(domains);
						cout<<"cmd->setDomains(domains)"<< domains<<endl;
						cmd->setDataType(dataTypeStr);
						cout<<"cmd->setDataType(dataTypeStr)"<< dataTypeStr<<endl;
						cmd->setOutputFile(outputFile);
						cout<<"cmd->setOutputFile(outputFile)"<< outputFile<<endl;
						cmd->setSkin(skin);
						cout<<"cmd->setSkin(skin)"<< skin<<endl;
						force=true;
						cmd->setForce(force);
						cout<<"cmd->setForce(force)"<< force<<endl;																	
						cmd->execute(commandArgs);
						cout<<"commandArgs.size()"<< commandArgs.size()<<endl;
						cout<<"cmd->execute(commandArgs)"<< commandArgs[0]<<endl;
						cout<<"cmd->execute(commandArgs)"<< commandArgs[1]<<endl;
						cout<<"cmd->execute(commandArgs)"<< commandArgs[2]<<endl;
					} catch (InvalidUsageException &e) {
						cerr << e.what() << endl << endl;
						retval = 1;
					}
					
					
		return retval;
}
