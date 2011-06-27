//  illarionserver - server for the game Illarion
//  Copyright 2011 Illarion e.V.
//
//  This file is part of illarionserver.
//
//  illarionserver is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  illarionserver is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with illarionserver.  If not, see <http://www.gnu.org/licenses/>.


#include "Player.hpp"
#include "World.hpp"
#include "Character.hpp"
#include "Logger.hpp"
#include <boost/shared_ptr.hpp>
#include "PlayerManager.hpp"
#include "MonitoringClients.hpp"
#include "netinterface/NetInterface.hpp"
#include "netinterface/protocol/BBIWIServerCommands.hpp"
#include "netinterface/protocol/BBIWIClientCommands.hpp"


CMonitoringClients::CMonitoringClients(CWorld * world) : _world(world)
{
}

CMonitoringClients::~CMonitoringClients()
{
	client_list.clear();
}

void CMonitoringClients::clientConnect( CPlayer * player )
{
#ifdef _MONITORINGCLIENTS_DEBUG
	std::cout<<"a new client connects ( "<< player->name<<" )"<<std::endl;
#endif
    CLogger::writeMessage("bbiwi","New Client connects: " + player->name + " active clients online: " + CLogger::toString( client_list.size() ));
	//create new Monitoring Client
	client_list.push_back(player); /*<add a new client to the list*/
	//setup the keepalive
    time( &(player->lastkeepalive) );
	//Send all player infos to the new connected client
	ccharactervector < CPlayer* >::iterator pIterator;
	
	for ( pIterator = _world->Players.begin(); pIterator != _world->Players.end(); ++pIterator)
	{
        boost::shared_ptr<CBasicServerCommand>cmd( new CBBPlayerTC( (*pIterator)->id, (*pIterator)->name, (*pIterator)->pos.x, (*pIterator)->pos.y, (*pIterator)->pos.z ) );
        player->Connection->addCommand( cmd );
        cmd.reset( new CBBSendAttribTC( (*pIterator)->id, "hitpoints", (*pIterator)->increaseAttrib("hitpoints",0) ) );
        player->Connection->addCommand( cmd );
        cmd.reset( new CBBSendAttribTC( (*pIterator)->id, "mana", (*pIterator)->increaseAttrib("mana",0) ) );
        player->Connection->addCommand( cmd );
        cmd.reset( new CBBSendAttribTC( (*pIterator)->id, "foodlevel", (*pIterator)->increaseAttrib("foodlevel",0) ) );
        player->Connection->addCommand( cmd );
	}
	
#ifdef _MONITORINGCLIENTS_DEBUG
	std::cout<<"connection sucessfully ( "<< player->name<<" )"<<std::endl;
#endif	
}

void CMonitoringClients::sendCommand( boost::shared_ptr<CBasicServerCommand> command )
{
	std::list<CPlayer*>::iterator iterator;
	//Loop through all onl clients
	for ( iterator = client_list.begin(); iterator != client_list.end(); ++iterator )
	{
		//Send this command to all players
		 (*iterator)->Connection->addCommand( command );
	}

}


void CMonitoringClients::CheckClients()
{
	std::list<CPlayer*>::iterator iterator;
    
	if ( !client_list.empty() )
	{
        for ( iterator = client_list.begin(); iterator != client_list.end(); ++iterator )
		{
            time_t thetime;
            time( &thetime );
            //sendCommand( new CSendMessageTS("CheckClients begin " + CLogger::toString(thetime),0));
            if ( (*iterator)->Connection->online )
            {
                /**
                * get the current time
                */
                time_t tempkeepalive;
                time( &tempkeepalive );
                int temptime;
                temptime = tempkeepalive - (*iterator)->lastkeepalive;
                //check if we have a timeout
                if ( (temptime >= 0) && (temptime < 20 ))
                {     
                    (*iterator)->workoutCommands();
                }
                else
                {
                    //timeout so we have to disconnect
                    CLogger::writeMessage("bbiwi","Client Timed out: " + (*iterator)->name);
                    (*iterator)->Connection->closeConnection();
                }
            }
            else
            {
                std::cout<<(*iterator)->name<<" connection inactive!"<<std::endl;
                CPlayerManager::get()->getLogOutPlayers().non_block_push_back( (*iterator) );
                iterator = client_list.erase( iterator );
                iterator--;	
                std::cout<<"added him to lostplayers!"<<std::endl;
            }
            time( &thetime );
            //sendCommand( new CSendMessageTS("CheckClients end " + CLogger::toString(thetime),0));
		
        }
        
	}
}


