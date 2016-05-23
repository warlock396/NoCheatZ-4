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

#ifndef SPAMCHANGENAMETESTER_H
#define SPAMCHANGENAMETESTER_H

#include "Preprocessors.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/BaseSystem.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

struct ChangeNameInfo
{
	float next_namechange_test;
	size_t namechange_count;

	ChangeNameInfo()
	{
		next_namechange_test = Plat_FloatTime() + 5.0f;
		namechange_count = 0;
	};
	ChangeNameInfo(const ChangeNameInfo& other)
	{
		next_namechange_test = other.next_namechange_test;
		namechange_count = other.namechange_count;
	};
};

class SpamChangeNameTester :
	public PlayerDataStructHandler<ChangeNameInfo>,
	public SourceSdk::IGameEventListener002,
	public BaseSystem,
	public OnTickListener,
	public Singleton<SpamChangeNameTester>
{
	typedef Singleton<SpamChangeNameTester> singleton_class;
	typedef PlayerDataStructHandler<ChangeNameInfo> playerdata_class;

public:
	SpamChangeNameTester();
	~SpamChangeNameTester();

	void Init();
	void Load();
	void Unload();

	void FireGameEvent(SourceSdk::IGameEvent* ev);
	void ClientConnect( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );

	void ProcessOnTick();
	void ProcessPlayerTestOnTick(NczPlayer* player){};
};

#endif