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

#ifndef CONCOMMANDTESTER_H
#define CONCOMMANDTESTER_H

#include <cmath>
#include "Misc/temp_basicstring.h"

#include "Misc/temp_basiclist.h"
#include "Hooks/ConCommandHookListener.h"
#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Misc/temp_singleton.h"

typedef struct PlayerConCommandS
{
	basic_string cmd;
	bool isSpamIgnored;
	float time;

	PlayerConCommandS() : cmd()
	{
		isSpamIgnored = false;
		time = 0.0;
	};
	PlayerConCommandS(const basic_string& command, bool ignore_spam, float cmd_time) : cmd()
	{
		cmd = command;
		isSpamIgnored = ignore_spam;
		time = cmd_time;
	};
	PlayerConCommandS(const PlayerConCommandS& other) : cmd()
	{
		cmd = other.cmd;
		isSpamIgnored = other.isSpamIgnored;
		time = other.time;
	};

	bool operator== (const PlayerConCommandS& other) const
	{
		if(cmd != other.cmd) return false;
		if(time != other.time) return false;
		return true;
	};
} PlayerConCommandT;

typedef struct LastPlayerCommandsS
{
	CUtlVector<PlayerConCommandT> commands;

	LastPlayerCommandsS() : commands() {};
	LastPlayerCommandsS(const LastPlayerCommandsS& other) : commands()
	{
		commands = commands;
	};

	// Remove commands that are older than 1 second from now
	void Clean()
	{
		const float now = Plat_FloatTime();
		while(!commands.IsEmpty())
		{
			if(fabs(now) - fabs(commands[0].time) >= 1.0) commands.Remove(0);
			else break;
		}
	}

	void AddCmd(const basic_string& command, bool ignore_spam)
	{
		Clean();

		commands.AddToTail(PlayerConCommandS(command, ignore_spam, Plat_FloatTime()));
	};

	size_t GetSpamCmdCount()
	{
		size_t ret = 0;
		size_t id = 0;
		size_t const max = commands.Size();

		for(PlayerConCommandT const * current = &commands[id]; id < max; current = &commands[++id])
		{
			if(!current->isSpamIgnored)	++ret;
		}
		return ret;
	};
} LastPlayerCommandsT;

typedef struct CommandInfoS
{
	basic_string command_name;
	bool ignore;
	bool ban;

	CommandInfoS() : command_name()
	{
		ignore = ban = false;
	};
	CommandInfoS(const basic_string& name, bool b1, bool b2) : command_name()
	{
		command_name = name;
		ignore = b1;
		ban = b2;
	};
	CommandInfoS(const CommandInfoS& other) : command_name()
	{
		command_name = other.command_name;
		ignore = other.ignore;
		ban = other.ban;
	};
	bool operator== (const CommandInfoS& other) const
	{
		if(command_name == other.command_name) return true;
		return false;
	};
} CommandInfoT;

typedef CUtlVector<CommandInfoT> CommandListT;

class ConCommandTester :
	public BaseSystem,
	public PlayerDataStructHandler<LastPlayerCommandsT>,
	public ConCommandHookListener,
	public Singleton<ConCommandTester>
{
	typedef Singleton<ConCommandTester> singleton_class;
	typedef PlayerDataStructHandler<LastPlayerCommandsT> playerdatahandler_class;

public:
	ConCommandTester();
	~ConCommandTester();

	/* Called by the plugin */
	bool TestPlayerCommand(NczPlayer* player, const basic_string& command);

private:

	void Init();
	void Load();
	void Unload();

	void AddCommandInfo(const basic_string& name, const bool ignore = false, const bool ban = false);
	void RemoveCommandInfo(const basic_string& name);

	void AddPlayerCommand(NczPlayer* player, const basic_string& command);

	static bool HookSayCallback(NczPlayer* const player, const void* const command, const SourceSdk::CCommand & args);
	static bool HookEntCallback(NczPlayer* const player, const void* const command, const SourceSdk::CCommand & args);

	bool ConCommandCallback(NczPlayer* player, void* cmd, const SourceSdk::CCommand & args);

private:
	CommandListT m_commands_list;
};

class Detection_CmdFlood : public LogDetection<LastPlayerCommandsT>
{
	typedef LogDetection<LastPlayerCommandsT> hClass;
public:
	Detection_CmdFlood() : hClass() {};
	virtual ~Detection_CmdFlood(){};

	virtual basic_string GetDataDump();
	virtual basic_string GetDetectionLogMessage()
	{
		return "ConCommand Flood";
	};
};

class Detection_CmdViolation : public Detection_CmdFlood // Use the same GetDataDump
{
public:
	Detection_CmdViolation() : Detection_CmdFlood() {};
	~Detection_CmdViolation(){};

	virtual basic_string GetDetectionLogMessage()
	{
		return "ConCommand Violation";
	};
};

#endif