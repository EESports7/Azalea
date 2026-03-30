#include "include.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

import Trajectory;

class $modify(PlayLayer){
    void setupHasCompleted(){
        auto mod = Mod::get();

        PlayLayer::setupHasCompleted();

        tm.useTrajectory = mod->getSettingValue<bool>("enable-trajectory");

        ColorManager::get().loadColors();

        tm.lineThickness = mod->getSettingValue<double>("line-thickness");
        tm.lookaheadLength = mod->getSettingValue<int64_t>("lookahead-length");

        #ifndef GEODE_IS_ANDROID
            tm.p1TrajectoryPoints.reserve(tm.lookaheadLength);
            tm.p2TrajectoryPoints.reserve(tm.lookaheadLength);
        #endif

        tm.playLayer = get();

        tm.renderCircleHitbox = m_levelSettings->m_fixRadiusCollision;

        tm.player1 = PlayerObject::create(1, 1, this, this, true);
        tm.player2 = PlayerObject::create(1, 1, this, this, true);

        tm.player1->retain();
        tm.player2->retain();

        tm.player1->setPosition({0, 105});
        tm.player2->setPosition({0, 105});

        tm.player1->setVisible(false);
        tm.player2->setVisible(false);

        m_objectLayer->addChild(tm.player1);
        m_objectLayer->addChild(tm.player2);

        m_objectLayer->addChild(TrajectoryManager::getTrajectoryNode(), INT_MAX);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) override {
        if((tm.trajectoryActive ||
           player == tm.player1 ||
           player == tm.player2) &&
           object != m_anticheatSpike){
            tm.endTrajectory = true;
            return;
           }

        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit(){
        tm.cleanup();

        PlayLayer::onQuit();
    }
};

class $modify(PauseLayer){
    void goEdit(){
        tm.cleanup();

        PauseLayer::goEdit();
    }
};

class $modify(GJBaseGameLayer){
    void update(float dt) override {
        if(m_started && m_resumeTimer <= 0 && tm.useTrajectory)
            tm.startTrajectory();

        GJBaseGameLayer::update(dt);
    }

    void checkSpawnObjects(){
        if(!tm.trajectoryActive)
            return GJBaseGameLayer::checkSpawnObjects();

        auto pl = tm.playLayer;

        CCPoint playerPos;
        if(tm.player1->m_isPlatformer){
            playerPos = LevelTools::posForTime(
                static_cast<float>(pl->m_gameState.m_levelTime),
                tm.currState.speedPortals,
                static_cast<int>(pl->m_levelSettings->m_startSpeed),
                true,
                pl->m_gameState.m_rotateChannel // should be fine
            );
        }else{
            playerPos = tm.player1->getPosition();
        }

        while(true){
            auto* triggers = static_cast<CCArray*>(
                pl->m_spawnObjects->objectForKey(
                    pl->m_gameState.m_currentChannel
                )
            );

            if(!triggers) triggers = CCArray::create();

            auto& idx = tm.currState.channelIndex[pl->m_gameState.m_currentChannel];
            auto reverse = pl->m_gameState.m_spawnChannelRelated1[pl->m_gameState.m_currentChannel];

            if(idx >= triggers->count()) break;

            auto obj = static_cast<TrajectoryGameObject*>(triggers->objectAtIndex(idx));

            bool skip = false;
            if(obj->m_isTouchTriggered){
                if(!tm.getFields(obj)->m_activated) break;
                skip = true;
            }else{
                auto objPos = obj->getPosition();

                if(tm.player1->m_isPlatformer){
                    if(playerPos.x < objPos.x) break;
                }else if(tm.player1->m_isSideways){
                    if(reverse){
                        if(objPos.y < playerPos.y) break;
                    }else{
                        if(playerPos.y < objPos.y) break;
                    }
                }else{
                    if(reverse){
                        if(objPos.x < playerPos.x) break;
                    }else{
                        if(playerPos.x < objPos.x) break;
                    }
                }
            }

            if(!obj->m_isGroupDisabled && !skip){
                tm.handleTrigger(tm.player1, obj);
            }

            idx++;
        }
    }

    void playerTouchedTrigger(PlayerObject* pl, EffectGameObject* obj){
        if(!tm.trajectoryActive)
            return GJBaseGameLayer::playerTouchedTrigger(pl, obj);

        if(!obj->m_isTouchTriggered) return;

        std::pair key{obj->m_uniqueID, tm.playerIndex(pl)};
        if(!tm.currState.activatedObjects.contains(key)){
            if(obj->m_isMultiTriggered)
                tm.currState.activatedTriggers.erase(key);

            tm.currState.activatedObjects.emplace(key);
        }else if(obj->m_isMultiTriggered)
            tm.currState.activatedTriggers.erase(key);

        if(!tm.currState.activatedTriggers.contains(key)){
            tm.currState.activatedTriggers.emplace(key);

            tm.handleTrigger(pl, obj);

            auto tgObj = static_cast<TrajectoryGameObject*>(obj);
            tgObj->editTrajectoryState(Activate::Player1, true);
            tgObj->editTrajectoryState(Activate::Player2, true);
        }
    }

    void collisionCheckObjects(PlayerObject* object, gd::vector<GameObject*>* objects, int objectCount, float dt){

    }
};