/*
	Copyright 2012 - Le Padellec Sylvain

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>

#include "JumpTester.h"

#include "Preprocessors.h"
#include "Systems/Logger.h"

/*
	Test each player to see if they use any script to help BunnyHop.

	Some players jumps just-in-time without using any script.
	We have to make the difference by using statistics.
*/

JumpTester::JumpTester() :
	BaseSystem("JumpTester"),
	OnGroundHookListener(),
	PlayerRunCommandHookListener(),
	playerdata_class(),
	singleton_class()
{
}

JumpTester::~JumpTester()
{
	Unload();
}

void JumpTester::Init()
{
	InitDataStruct();
}

void JumpTester::Load()
{
	OnGroundHookListener::RegisterOnGroundHookListener(this);
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, 3);
}

void JumpTester::Unload()
{
	OnGroundHookListener::RemoveOnGroundHookListener(this);
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);

	PLAYERS_LOOP_RUNTIME
	{
		ResetPlayerDataStruct(ph->playerClass);
	}
	END_PLAYERS_LOOP
}

int GetGameTickCount()
{
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		return static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
	else
	{
		return static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
}

void JumpTester::m_hGroundEntityStateChangedCallback(NczPlayer* player, bool new_isOnGround)
{
	JumpInfoT* playerData = GetPlayerDataStruct(player);

	if(new_isOnGround)
	{
		playerData->onGroundHolder.onGround_Tick = GetGameTickCount();
		playerData->isOnGround = true;
		SystemVerbose1(Helpers::format("Player %s touched the ground.", player->GetName()));
		if(playerData->jumpCmdHolder.outsideJumpCmdCount > 10) // Il serait plus judicieux d'utiliser le RMS
		{
			Detection_BunnyHopScript pDetection = Detection_BunnyHopScript();
			pDetection.PrepareDetectionData(playerData);
			pDetection.PrepareDetectionLog(player, this);
			pDetection.Log();
			player->Kick("You have to turn off your BunnyHop Script to play on this server.");
		}
		else if(playerData->jumpCmdHolder.outsideJumpCmdCount == 0 && playerData->perfectBhopsCount > 5)
		{
			Detection_BunnyHopProgram pDetection = Detection_BunnyHopProgram();
			pDetection.PrepareDetectionData(playerData);
			pDetection.PrepareDetectionLog(player, this);
			pDetection.Log();

			player->Ban("[NoCheatZ 4] You have been banned for using BunnyHop on this server.");
		}
		playerData->jumpCmdHolder.outsideJumpCmdCount = 0;
	}
	else
	{
		playerData->onGroundHolder.notOnGround_Tick = GetGameTickCount();
		++playerData->onGroundHolder.jumpCount;
		playerData->isOnGround = false;
		SystemVerbose1(Helpers::format("Player %s leaved the ground.", player->GetName()));
	}
}

PlayerRunCommandRet JumpTester::PlayerRunCommandCallback(NczPlayer* player, SourceSdk::CUserCmd* pCmd, const SourceSdk::CUserCmd& old_cmd)
{
	PlayerRunCommandRet drop_cmd = CONTINUE;

	JumpInfoT* playerData = GetPlayerDataStruct(player);

	if((playerData->jumpCmdHolder.lastJumpCmdState == false) && (pCmd->buttons & IN_JUMP))
	{
		playerData->jumpCmdHolder.JumpDown_Tick = GetGameTickCount();
		if(playerData->isOnGround)
		{
			int diff = abs(playerData->jumpCmdHolder.JumpDown_Tick - playerData->onGroundHolder.onGround_Tick);
			if(diff < 10)
			{
				++playerData->total_bhopCount;
#				ifdef DEBUG
					printf("Player %s : total_bhopCount = %d\n", player->GetName(), playerData->total_bhopCount);
#				endif
				if(diff < 3 && diff > 0)
				{
					++playerData->goodBhopsCount;
#					ifdef DEBUG
						printf("Player %s : goodBhopsCount = %d\n", player->GetName(), playerData->goodBhopsCount);
#					endif
					drop_cmd = INERT;
				}
				if(diff == 0)
				{
					++playerData->perfectBhopsCount;
#					ifdef DEBUG
						printf("Player %s : perfectBhopsCount = %d\n", player->GetName(), playerData->perfectBhopsCount);
#					endif
					drop_cmd = INERT;
				}
			}

			SystemVerbose1(Helpers::format("Player %s pushed the jump button.", player->GetName()));
		}
		else
		{
			++playerData->jumpCmdHolder.outsideJumpCmdCount;
			SystemVerbose1(Helpers::format("Player %s pushed the jump button while flying.", player->GetName()));
		}
		playerData->jumpCmdHolder.lastJumpCmdState = true;
	}
	else if((playerData->jumpCmdHolder.lastJumpCmdState == true) && ((pCmd->buttons & IN_JUMP) == 0))
	{
		playerData->jumpCmdHolder.lastJumpCmdState = false;
		playerData->jumpCmdHolder.JumpUp_Tick = GetGameTickCount();
		SystemVerbose1(Helpers::format("Player %s released the jump button.", player->GetName()));
	}
	return drop_cmd;
}

const char * ConvertButton(bool v)
{
	if(v) return "Button Down";
	else return "Button Up";
}

basic_string Detection_BunnyHopScript::GetDataDump()
{
	return Helpers::format( ":::: BunnyHopInfoT {\n"
							":::::::: OnGroundHolderT {\n"
							":::::::::::: On Ground At (Tick #) : %d,\n"
							":::::::::::: Leave Ground At (Tick #) : %d,\n"
							":::::::::::: Jump Count : %d\n"
							":::::::: },\n"
							":::::::: JumpCmdHolderT {\n"
							":::::::::::: Last Jump Command : %s,\n"
							":::::::::::: Jump Button Down At (Tick #) : %d,\n"
							":::::::::::: Jump Button Up At (Tick #) : %d,\n"
							":::::::::::: Jump Commands Done While Flying : %d\n"
							":::::::: },\n"
							":::::::: Total Bunny Hop Count : %d,\n"
							":::::::: Good Bunny Hop Count : %d,\n"
							":::::::: Perfect Bunny Hop Ratio : %d %%,\n"
							":::::::: Perfect Bunny Hop Count : %d\n"
							":::: }",
							GetDataStruct()->onGroundHolder.onGround_Tick, GetDataStruct()->onGroundHolder.notOnGround_Tick, GetDataStruct()->onGroundHolder.jumpCount,
							ConvertButton(GetDataStruct()->jumpCmdHolder.lastJumpCmdState), GetDataStruct()->jumpCmdHolder.JumpDown_Tick, GetDataStruct()->jumpCmdHolder.JumpUp_Tick, GetDataStruct()->jumpCmdHolder.outsideJumpCmdCount,
							GetDataStruct()->total_bhopCount,
							GetDataStruct()->goodBhopsCount,
							GetDataStruct()->perfectBhopsPercent,
							GetDataStruct()->perfectBhopsCount);
}