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


#include "LongTimeAction.hpp"
#include "World.hpp"
#include "Player.hpp"
#include "MilTimer.hpp"
#include "Character.hpp"

#include "script/LuaItemScript.hpp"
#include "script/LuaTileScript.hpp"
#include "script/LuaMonsterScript.hpp"
#include "script/LuaNPCScript.hpp"
#include "script/LuaMagicScript.hpp"

LongTimeAction::LongTimeAction(Player *player, World *world) : _owner(player) , _world(world), _actionrunning(false) {
    _script.reset();
    _timetowaitTimer = NULL;
    _redoaniTimer = NULL;
    _redosoundTimer = NULL;
    _ani = 0;
    _sound = 0;
    _targetId = 0;
}

void LongTimeAction::setLastAction(boost::shared_ptr<LuaScript> script, SouTar srce, SouTar trgt,unsigned short int counter, unsigned short int param, ActionType at) {
    _script = script;
    _source = srce;
    _target = trgt;
    _counter = counter;
    _param = param;
    _at = at;

    if (trgt.character != NULL) {
        _targetId = _target.character->id;
    }

    if (srce.character != NULL) {
        _sourceId = _source.character->id;
        _sourceCharType = _source.character->character;
    }
}

bool LongTimeAction::checkAction() {
    if (_actionrunning) {
        //check if timetowaittimer is initialized and if we hit the next time
        if (_timetowaitTimer && _timetowaitTimer->Next()) {
            successAction();
            return true;
        }

        if ((_redoaniTimer) && (_redoaniTimer->Next()) && (_ani != 0)) {
            _world->makeGFXForAllPlayersInRange(_owner->pos.x, _owner->pos.y, _owner->pos.z, MAXVIEW, _ani);
        }

        if ((_redosoundTimer) && (_redosoundTimer->Next()) && (_sound != 0)) {
            _world->makeSoundForAllPlayersInRange(_owner->pos.x, _owner->pos.y, _owner->pos.z, MAXVIEW, _sound);
        }
    }

    return false;
}

void LongTimeAction::startLongTimeAction(unsigned short int timetowait, unsigned short int ani, unsigned short int redoani, unsigned short int sound, unsigned short int redosound) {
    _actionrunning = true;
    _ani = ani;
    _sound = sound;

    //delete old timer and initialize new ones if necsessary
    if (_timetowaitTimer) {
        delete _timetowaitTimer;
    }

    _timetowaitTimer = new MilTimer(timetowait * 100);

    if (_redoaniTimer) {
        delete _redoaniTimer;
        _redoaniTimer = NULL;
    }

    if (_ani != 0 && redoani != 0) {
        _redoaniTimer = new MilTimer(redoani * 100);
    }

    if (_redosoundTimer) {
        delete _redosoundTimer;
        _redosoundTimer = NULL;
    }

    if (_sound != 0 && redosound != 0) {
        _redosoundTimer = new MilTimer(redosound * 100);
    }

    if (_sound != 0) {
        _world->makeSoundForAllPlayersInRange(_owner->pos.x, _owner->pos.y, _owner->pos.z, MAXVIEW, _sound);
    }

    if (_ani != 0) {
        _world->makeGFXForAllPlayersInRange(_owner->pos.x, _owner->pos.y, _owner->pos.z, MAXVIEW, _ani);
    }

}

bool LongTimeAction::actionDisturbed(Character *disturber) {
    bool disturbed = false;
    checkSource();
    checkTarget();

    if (_actionrunning) {
        if (_script) {
            if ((_at == AT_USE)) {
                //a itemscript
                if (_source.Type == LUA_ITEM) {
                    boost::shared_ptr<LuaItemScript>itScript = boost::dynamic_pointer_cast<LuaItemScript>(_script);
                    disturbed = itScript->actionDisturbed(_owner, disturber);
                }
                //a tilescript
                else if (_source.Type == LUA_FIELD) {
                    boost::shared_ptr<LuaTileScript>tiScript = boost::dynamic_pointer_cast<LuaTileScript>(_script);
                    disturbed = tiScript->actionDisturbed(_owner, disturber);

                }
                //a character
                else if (_source.Type == LUA_CHARACTER) {
                    if (_sourceCharType == Character::monster) {
                        boost::shared_ptr<LuaMonsterScript>monScript = boost::dynamic_pointer_cast<LuaMonsterScript>(_script);
                        disturbed = monScript->actionDisturbed(_owner, disturber);
                    }
                    //a npc
                    else if (_sourceCharType == Character::npc) {
                        boost::shared_ptr<LuaNPCScript>npcScript = boost::dynamic_pointer_cast<LuaNPCScript>(_script);
                        disturbed = npcScript->actionDisturbed(_owner, disturber);
                    }

                }
            } else if ((_at == AT_MAGIC)) {
                boost::shared_ptr<LuaMagicScript>mgScript = boost::dynamic_pointer_cast<LuaMagicScript>(_script);
                disturbed = mgScript->actionDisturbed(_owner, disturber);
            }

            if (disturbed) {
                abortAction();
                return true;
            } else {
                return false;
            }
        } else {
            _actionrunning = false;
        }
    }

    return false;
}

void LongTimeAction::abortAction() {
    checkSource();
    checkTarget();

    if (_actionrunning) {
        if (_script) {
            if ((_at == AT_USE)) {
                //a itemscript
                if (_source.Type == LUA_ITEM) {
                    boost::shared_ptr<LuaItemScript>itScript = boost::dynamic_pointer_cast<LuaItemScript>(_script);

                    if (_target.Type == LUA_ITEM || _target.Type == LUA_NONE) {
                        itScript->UseItem(_owner, _source.item, _target.item, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONABORTED));
                    } else if (_target.Type == LUA_CHARACTER) {
                        itScript->UseItemWithCharacter(_owner, _source.item, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONABORTED));
                    } else if (_target.Type == LUA_FIELD) {
                        itScript->UseItemWithField(_owner, _source.item, _target.pos, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONABORTED));
                    }
                }
                //a tilescript
                else if (_source.Type == LUA_FIELD) {
                    boost::shared_ptr<LuaTileScript>tiScript = boost::dynamic_pointer_cast<LuaTileScript>(_script);

                    if (_target.Type == LUA_NONE) {
                        tiScript->useTile(_owner, _source.pos, _counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                    } else if (_target.Type == LUA_FIELD) {
                        tiScript->useTileWithField(_owner , _source.pos, _target.pos, _counter , static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONABORTED));
                    } else if (_target.Type == LUA_ITEM) {
                        tiScript->useTileWithItem(_owner, _source.pos, _target.item, _counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                    } else if (_target.Type == LUA_CHARACTER) {
                        tiScript->useTileWithCharacter(_owner, _source.pos, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                    }
                }
                //a character
                else if (_source.Type == LUA_CHARACTER) {
                    //a monster
                    if (_sourceCharType == Character::monster) {
                        boost::shared_ptr<LuaMonsterScript>monScript = boost::dynamic_pointer_cast<LuaMonsterScript>(_script);

                        if (_target.Type == LUA_NONE) {
                            monScript->useMonster(_source.character,_owner,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_FIELD) {
                            monScript->useMonsterWithField(_source.character,_owner,_target.pos,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_ITEM) {
                            monScript->useMonsterWithItem(_source.character,_owner,_target.item,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_CHARACTER) {
                            monScript->useMonsterWithCharacter(_source.character,_owner,_target.character,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        }
                    }
                    //a npc
                    else if (_sourceCharType == Character::npc) {
                        boost::shared_ptr<LuaNPCScript>npcScript = boost::dynamic_pointer_cast<LuaNPCScript>(_script);

                        if (_target.Type == LUA_NONE) {
                            npcScript->useNPC(_owner, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_FIELD) {
                            npcScript->useNPCWithField(_owner, _target.pos, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_ITEM) {
                            npcScript->useNPCWithItem(_owner, _target.item, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        } else if (_target.Type == LUA_CHARACTER) {
                            npcScript->useNPCWithCharacter(_owner, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        }
                    }

                }
            } else if ((_at == AT_MAGIC)) {
                boost::shared_ptr<LuaMagicScript>mgScript = boost::dynamic_pointer_cast<LuaMagicScript>(_script);

                if (_target.Type == LUA_NONE) {
                    mgScript->CastMagic(_owner,_counter,_param,static_cast<unsigned char>(LTS_ACTIONABORTED));
                } else if (_target.Type == LUA_FIELD) {
                    mgScript->CastMagicOnField(_owner,_target.pos,_counter,_param,static_cast<unsigned char>(LTS_ACTIONABORTED));
                } else if (_target.Type == LUA_CHARACTER) {
                    mgScript->CastMagicOnCharacter(_owner,_target.character,_counter,_param,static_cast<unsigned char>(LTS_ACTIONABORTED));
                }
                //Todo add ki handling here
                else if (_target.Type == LUA_ITEM) {
                    mgScript->CastMagicOnItem(_owner,_target.item,_counter,_param,static_cast<unsigned char>(LTS_ACTIONABORTED));
                }
            }
        }
    }

    _actionrunning = false;
    _script.reset();
    delete _redoaniTimer;
    _redoaniTimer = NULL;
    delete _redosoundTimer;
    _redosoundTimer = NULL;
    delete _timetowaitTimer;
    _timetowaitTimer = NULL;
    _ani = 0;
    _sound = 0;


}

void LongTimeAction::successAction() {
    checkSource();
    checkTarget();

    if (_actionrunning) {
        _actionrunning = false;

        if (_script) {
            if ((_at == AT_USE)) {
                //a itemscript
                if (_source.Type == LUA_ITEM) {
                    if (_source.item.quality >= 100) {
                        boost::shared_ptr<LuaItemScript>itScript = boost::dynamic_pointer_cast<LuaItemScript>(_script);

                        if (_target.Type == LUA_ITEM || _target.Type == LUA_NONE) {
                            itScript->UseItem(_owner, _source.item, _target.item, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_CHARACTER) {
                            itScript->UseItemWithCharacter(_owner, _source.item, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_FIELD) {
                            itScript->UseItemWithField(_owner, _source.item, _target.pos, _counter, static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        }
                    }
                }
                //a tilescript
                else if (_source.Type == LUA_FIELD) {
                    boost::shared_ptr<LuaTileScript>tiScript = boost::dynamic_pointer_cast<LuaTileScript>(_script);

                    if (_target.Type == LUA_NONE) {
                        tiScript->useTile(_owner, _source.pos, _counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                    } else if (_target.Type == LUA_FIELD) {
                        tiScript->useTileWithField(_owner , _source.pos, _target.pos, _counter , static_cast<TYPE_OF_ITEM_ID>(_param), static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                    } else if (_target.Type == LUA_ITEM) {
                        tiScript->useTileWithItem(_owner, _source.pos, _target.item, _counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                    } else if (_target.Type == LUA_CHARACTER) {
                        tiScript->useTileWithCharacter(_owner, _source.pos, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                    }
                }
                //a character
                else if (_source.Type == LUA_CHARACTER) {
                    //a monster
                    if (_sourceCharType == Character::monster) {
                        boost::shared_ptr<LuaMonsterScript>monScript = boost::dynamic_pointer_cast<LuaMonsterScript>(_script);

                        if (_target.Type == LUA_NONE) {
                            monScript->useMonster(_source.character,_owner,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_FIELD) {
                            monScript->useMonsterWithField(_source.character,_owner,_target.pos,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_ITEM) {
                            monScript->useMonsterWithItem(_source.character,_owner,_target.item,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_CHARACTER) {
                            monScript->useMonsterWithCharacter(_source.character,_owner,_target.character,_counter,static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        }
                    }
                    //a npc
                    else if (_sourceCharType == Character::npc) {
                        boost::shared_ptr<LuaNPCScript>npcScript = boost::dynamic_pointer_cast<LuaNPCScript>(_script);

                        if (_target.Type == LUA_NONE) {
                            npcScript->useNPC(_owner, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_FIELD) {
                            npcScript->useNPCWithField(_owner, _target.pos, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_ITEM) {
                            npcScript->useNPCWithItem(_owner, _target.item, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                        } else if (_target.Type == LUA_CHARACTER) {
                            npcScript->useNPCWithCharacter(_owner, _target.character, _counter, static_cast<TYPE_OF_ITEM_ID>(_param),static_cast<unsigned char>(LTS_ACTIONABORTED));
                        }
                    }
                }
            } else if ((_at == AT_MAGIC)) {
                boost::shared_ptr<LuaMagicScript>mgScript = boost::dynamic_pointer_cast<LuaMagicScript>(_script);

                if (_target.Type == LUA_NONE) {
                    mgScript->CastMagic(_owner,_counter,_param,static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                } else if (_target.Type == LUA_FIELD) {
                    mgScript->CastMagicOnField(_owner,_target.pos,_counter,_param,static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                } else if (_target.Type == LUA_CHARACTER) {
                    mgScript->CastMagicOnCharacter(_owner,_target.character,_counter,_param,static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                    //Todo add ki handling here
                } else if (_target.Type == LUA_ITEM) {
                    mgScript->CastMagicOnItem(_owner,_target.item,_counter,_param,static_cast<unsigned char>(LTS_ACTIONSUCCESSFULL));
                }

            }
        }
    }

    if (!_actionrunning) {
        _script.reset();
        delete _redoaniTimer;
        _redoaniTimer = NULL;
        delete _redosoundTimer;
        _redosoundTimer = NULL;
        delete _timetowaitTimer;
        _timetowaitTimer = NULL;
        _ani = 0;
        _sound = 0;
    }

}

void LongTimeAction::changeSource(Character *cc) {
    _source.Type = LUA_CHARACTER;
    _source.pos = cc->pos;
    _source.character = cc;
    _sourceId = cc->id;
    _sourceCharType = cc->character;
}

void LongTimeAction::changeSource(ScriptItem sI) {
    _source.Type = LUA_ITEM;
    _source.pos = sI.pos;
    _source.item = sI;
    _sourceId = 0;
    _sourceCharType = Character::player;
}

void LongTimeAction::changeSource(position pos) {
    _source.Type = LUA_FIELD;
    _source.pos = pos;
    _sourceId = 0;
    _sourceCharType = Character::player;
}

void LongTimeAction::changeSource() {
    _source.Type = LUA_NONE;
    _sourceId = 0;
    _sourceCharType = Character::player;
}

void LongTimeAction::changeTarget(Character *cc) {
    _target.Type = LUA_CHARACTER;
    _target.pos = cc->pos;
    _target.character = cc;
    _targetId = cc->id;
}

void LongTimeAction::changeTarget(ScriptItem sI) {
    _target.Type = LUA_ITEM;
    _target.pos = sI.pos;
    _target.item = sI;
    _targetId = 0;
}

void LongTimeAction::checkTarget() {
    if (_targetId == 0) {
        _source.character=NULL;
        return;
    } else {
        if (_targetId < MONSTER_BASE) {
            //player
            if (World::get()->Players.findID(_targetId) == NULL) {
                _target.character=NULL;
            }
        } else if (_targetId >= MONSTER_BASE && _targetId < NPC_BASE) {
            //monster
            if (World::get()->Monsters.findID(_targetId) == NULL) {
                _target.character=NULL;
            }
        } else {
            if (World::get()->Npc.findID(_targetId) == NULL) {
                _target.character=NULL;
            }
        }

    }
}

void LongTimeAction::checkSource() {
    if (_sourceId == 0) {
        _source.character=NULL;
        _sourceCharType=0;
        return;
    } else {
        if (_sourceId < MONSTER_BASE) {
            //player
            if (World::get()->Players.findID(_sourceId) == NULL) {
                _source.character=NULL;
                _sourceCharType=0;
            }
        } else if (_sourceId >= MONSTER_BASE && _sourceId < NPC_BASE) {
            //monster
            if (World::get()->Monsters.findID(_sourceId) == NULL) {
                _source.character=NULL;
                _sourceCharType=0;
            }
        } else {
            if (World::get()->Npc.findID(_sourceId) != NULL) {
                _source.character=NULL;
                _sourceCharType=0;
            }
        }

    }

}

void LongTimeAction::changeTarget(position pos) {
    _target.Type = LUA_FIELD;
    _target.pos = pos;
    _targetId = 0;
}

void LongTimeAction::changeTarget() {
    _target.Type = LUA_NONE;
}

