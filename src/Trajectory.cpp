module;

#include "include.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>

module Trajectory;

import PlayerUtil;
import ColorManager;

auto& tm = TrajectoryManager::get();
// todo: (most to least important/easiest)
// properly copy the CCArray
// implement m_speedObjects stuff
// get PlayLayer using the getter
// hook the functions with the handles and get delta
// investigate m_rotateChannel and m_currentChannel
// investigate variables affected by camera (make struct)
// get this thing working ( finish startTrajectory() )
class $modify(PlayLayer){
    void setupHasCompleted(){
        auto mod = Mod::get();

        PlayLayer::setupHasCompleted();

        tm.useTrajectory = mod->getSettingValue<bool>("enable-trajectory");

        ColorManager::get().loadColors();

        tm.lineThickness = mod->getSettingValue<double>("line-thickness");
        tm.lookaheadLength = mod->getSettingValue<int64_t>("lookahead-length");

        tm.playLayer = this;

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

    void onQuit(){
        tm.cleanup();

        PlayLayer::onQuit();
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
};

class $modify(PauseLayer){
    void goEdit(){
        tm.cleanup();

        PauseLayer::goEdit();
    }
};

class $modify(TrajectoryGameObject, EffectGameObject){
    struct Fields{
        bool m_activated;
        bool m_activatedByPlayer1;
        bool m_activatedByPlayer2;
    };

    void activatedByTrajectory(PlayerObject* pl, int idx){
        auto fields = m_fields.self();

        fields->m_activated = true;
        if(!canMultiActivate(pl->m_isPlatformer)){
            if(idx == 1) fields->m_activatedByPlayer1 = true;
            else fields->m_activatedByPlayer2 = true;
        }
    }
};

// class $modify(PlayerObject){

// };

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
                pl->m_speedObjects,
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
            auto fields = obj->m_fields.self();

            bool skip = false;
            if(obj->m_isTouchTriggered){
                if(!fields->m_activated) break;
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

            auto& fields = static_cast<TrajectoryGameObject*>(obj)->m_fields;
            fields->m_activatedByPlayer1 = true;
            fields->m_activatedByPlayer2 = true;
        }
    }
};


void TrajectoryManager::startTrajectory(){
    setupManager();

    // release
    reloadManager(false);
    setupPlayers(false);
    updateTrajectory(false);

    // click
    reloadManager(true);
    setupPlayers(true);
    updateTrajectory(true);

    resetGJBGL();
}

void TrajectoryManager::updateTrajectory(bool down){
    auto& cm = ColorManager::get();
    auto node = getTrajectoryNode();

    auto trailColor = down ?
        cm.getColor(ColorIdx::Holding) :
        cm.getColor(ColorIdx::Release);

    auto deltaSec = delta * 60.f;

    player1->m_maybeIsColliding = false;
    player2->m_maybeIsColliding = false;

    trajectoryActive = true;
    for(uint64_t i = 0; i < lookaheadLength; i++){
        auto prevPosP1 = player1->getPosition();
        auto prevPosP2 = player2->getPosition();

        auto p1Color = trailColor;
        auto p2Color = trailColor;

        if(down){
            p1TrajectoryPoints.emplace_back(prevPosP1);
            p2TrajectoryPoints.emplace_back(prevPosP2);
        }

        player1->m_totalTime += delta;
        player2->m_totalTime += delta;

        player1->resetTouchedRings(false);
        if(currState.isDuel)
            player2->resetTouchedRings(false);

        player1->resetCollisionLog(false);
        if(currState.isDuel)
            player2->resetCollisionLog(false);

        player1->updateInternalActions(delta);
        if(currState.isDuel) player2->updateInternalActions(delta);

        player1->update(deltaSec);
        playLayer->checkCollisions(player1, deltaSec, false);
        player1->updateSpecial(delta);

        if(currState.isDuel){
            player2->update(deltaSec);
            playLayer->checkCollisions(player2, deltaSec, false);
            player2->updateSpecial(delta);
        }

        player1->updateRotation(deltaSec);
        if(currState.isDuel) player2->updateRotation(deltaSec);

        player1->m_shipRotation = player1->getPosition();
        if(currState.isDuel) player2->m_shipRotation = player2->getPosition();

        if(!down && (p1TrajectoryPoints[i] == prevPosP1))
            p1Color = cm.getColor(ColorIdx::Mixed);

        if(!down && currState.isDuel && (p2TrajectoryPoints[i] == prevPosP2))
            p2Color = cm.getColor(ColorIdx::Mixed);

        node->drawSegment(prevPosP1,
            player1->getPosition(),
            lineThickness,
            p1Color
        );

        if(currState.isDuel)
            node->drawSegment(prevPosP2,
                player2->getPosition(),
                lineThickness,
                p2Color
            );

        if(endTrajectory){
            p1Rotation = player1->getRotation();
            player1->updatePlayerScale();

            if(currState.isDuel){
                p2Rotation = player2->getRotation();
                player2->updatePlayerScale();
            }

            renderDeath();
            break;
        }
    }
    trajectoryActive = false;
}

// modify_cast maybe
void TrajectoryManager::handleOrb(PlayerObject* pl, RingObject* obj){
    auto objTGO = static_cast<TrajectoryGameObject*>(static_cast<EffectGameObject*>(obj));

    pl->addToTouchedRings(obj);

    if(pl->isFlying() || obj->m_claimTouch || pl->m_isDead) return;

    if(pl->m_ringRelatedSet.contains(obj->m_uniqueID)) return;

    auto orbType = obj->getType();

    bool normalOrb = orbType != GameObjectType::CustomRing &&
        orbType != GameObjectType::TeleportOrb;

    if(!pl->m_stateRingJump2 ||
        pl->m_isDashing ||
        !pl->m_stateJumpBuffered ||
        ((pl->m_touchedRing || !normalOrb) &&
        (pl->m_touchedCustomRing || orbType != GameObjectType::CustomRing) &&
        (pl->m_touchedGravityPortal || orbType != GameObjectType::TeleportOrb)))
        return;

    if(obj->m_isReverse) pl->reversePlayer(obj);

    pl->m_ringJumpRelated = true;
    pl->m_ringRelatedSet.insert(obj->m_uniqueID);
    pl->m_touchingRings->removeObject(obj);

    if(normalOrb) pl->m_padRingRelated = true;

    switch(orbType){
        case GameObjectType::PinkJumpRing:
        case GameObjectType::YellowJumpRing:
        case GameObjectType::RedJumpRing:
        case GameObjectType::GravityRing:
        case GameObjectType::GreenRing: {
            pl->m_maybeIsBoosted = true;
            pl->m_isOnGround = false;
            pl->m_isOnGround2 = false;

            auto jumpVel = pl->m_yStart;

            switch(orbType){
                case GameObjectType::PinkJumpRing:
                    if(pl->m_isShip)      jumpVel *= 0.37f;
                    else if(pl->m_isBird) jumpVel *= 0.42f;
                    else if(pl->m_isBall) jumpVel *= 0.77f;
                    else                  jumpVel *= 0.72f;
                    break;
                case GameObjectType::YellowJumpRing:
                    if(pl->m_isRobot) jumpVel *= 0.9f;
                    break;
                case GameObjectType::RedJumpRing:
                    pl->m_isAccelerating = true;

                    if(pl->m_isShip && pl->m_vehicleSize != 1.f) jumpVel *= 1.4f;
                    else if(pl->m_isBird)
                        jumpVel *= (pl->m_vehicleSize == 1.f) ? 1.02f : 1.36f;
                    else if(pl->m_isBall)   jumpVel *= 1.34f;
                    else if(pl->m_isRobot)  jumpVel *= 1.28f;
                    else if(pl->m_isSpider) jumpVel *= 1.34f;
                    else                    jumpVel *= 1.38f;
                    break;
                case GameObjectType::GravityRing:
                    jumpVel *= 0.8f;
                    break;
                case GameObjectType::GreenRing:
                    if(pl->m_isShip) jumpVel *= 0.7f;
                    break;
            }

            float sizeScale = (pl->m_vehicleSize == 1.f) ? 1.f : 0.8f;

            float flipMod = 1.f;
            if(orbType == GameObjectType::GravityRing ||
                orbType == GameObjectType::GreenRing){
                flipGravity(pl, !pl->m_isUpsideDown);
                flipMod = pl->m_isUpsideDown ? 1.0f : -1.0f;
            }

            pl->setYVelocity(jumpVel * sizeScale * flipMod, NULL);

            if(pl->m_isBall) pl->runBallRotation2();
            else if(!pl->m_isLocked && !pl->m_isDashing){
                pl->m_isRotating = false;
                pl->m_isBallRotating = false;
                pl->m_rotateSpeed = 0;

                pl->runNormalRotation(false, 1.f);
            }

            pl->m_hasEverHitRing = true;
        } break;
        case GameObjectType::DropRing: {
            float velocity = pl->m_isUpsideDown ? 15.f : -15.f;

            if(pl->m_isShip || pl->m_isBird || pl->m_isDart || pl->m_isSwing){
                velocity = (pl->m_isUpsideDown ? 14.f : -14.f);

                if(pl->m_isBird)
                    velocity *= 0.8f;
            }
            else if(pl->m_isSpider)
                velocity *= 1.1f;

            pl->setYVelocity(velocity, NULL);

            if(pl->m_isBall) pl->runBallRotation2();
            else if(!pl->m_isLocked && !pl->m_isDashing){
                pl->m_isRotating = false;
                pl->m_isBallRotating = false;
                pl->m_rotateSpeed = 0;

                pl->runNormalRotation(false, 1.f);
            }

            pl->m_hasEverHitRing = true;
            pl->m_isAccelerating = true;

            if(pl->m_isBall || pl->m_isSwing)
                pl->m_jumpBuffered = false;
        } break;
        case GameObjectType::SpiderOrb: {
            bool targetGravity = pl->m_isSideways ? obj->isFacingLeft() : obj->isFacingDown();

            pl->flipGravity(targetGravity, true);
            pl->spiderTestJump(false);
        } break;
        case GameObjectType::TeleportOrb: {
            pl->m_touchedGravityPortal = true;
        } break;
        case GameObjectType::DashRing: {
            pl->startDashing(static_cast<DashRingObject*>(obj));
        } break;
        case GameObjectType::GravityDashRing: {
            flipGravity(pl, !pl->m_isUpsideDown);
            pl->startDashing(static_cast<DashRingObject*>(obj));
        } break;
        // case GameObjectType::CustomRing: {
        //     pl->m_touchedCustomRing = true;
        //     // more later
        // } break;
    }

    objTGO->activatedByTrajectory(pl, playerIndex(pl));
}

void TrajectoryManager::handlePortal(PlayerObject* pl, EffectGameObject* obj){
    switch(obj->getType()){
        case GameObjectType::CubePortal:
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;

            pl->switchedToMode(GameObjectType::CubePortal);
            pl->modeDidChange();
            break;
        case GameObjectType::ShipPortal:
            pl->switchedToMode(GameObjectType::ShipPortal);
            pl->toggleFlyMode(true, true);
            break;
        case GameObjectType::BallPortal:
            pl->switchedToMode(GameObjectType::BallPortal);
            pl->toggleRollMode(true, true);
            break;
        case GameObjectType::UfoPortal:
            pl->switchedToMode(GameObjectType::UfoPortal);
            pl->toggleBirdMode(true, true);
            break;
        case GameObjectType::WavePortal:
            pl->switchedToMode(GameObjectType::WavePortal);
            pl->toggleDartMode(true, true);
            break;
        case GameObjectType::RobotPortal:
            pl->switchedToMode(GameObjectType::RobotPortal);
            pl->toggleRobotMode(true, true);
            break;
        case GameObjectType::SpiderPortal:
            pl->switchedToMode(GameObjectType::SpiderPortal);
            pl->toggleSpiderMode(true, true);
            break;
        case GameObjectType::SwingPortal:
            pl->switchedToMode(GameObjectType::SwingPortal);
            pl->toggleSwingMode(true, true);
            break;
        case GameObjectType::RegularSizePortal:
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;
            pl->togglePlayerScale(false, true);
            break;
        case GameObjectType::MiniSizePortal:
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;
            pl->togglePlayerScale(true, true);
            break;
        case GameObjectType::NormalGravityPortal: {
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;

            flipGravity(pl, false);
        } break;
        case GameObjectType::InverseGravityPortal: {
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;

            flipGravity(pl, true);
        } break;
        case GameObjectType::GravityTogglePortal:{
            pl->m_lastPortalPos = obj->getPosition();
            pl->m_lastActivatedPortal = obj;

            flipGravity(pl, !pl->m_isUpsideDown);
        } break;
    }

    static_cast<TrajectoryGameObject*>(obj)->activatedByTrajectory(pl, playerIndex(pl));
}

void TrajectoryManager::handleTrigger(PlayerObject* pl, EffectGameObject* obj){
    switch(static_cast<TriggerID>(obj->m_objectID)){
        case TriggerID::SlowSpeed:
            pl->m_playerSpeed = 0.7f;
            pl->m_yStart = 10.620032f;
            pl->m_gravity = 0.940199f;
            pl->m_speedMultiplier = 5.980002f;
            break;
        case TriggerID::NormalSpeed:
            pl->m_playerSpeed = 0.9f;
            pl->m_yStart = 11.1800318f;
            pl->m_gravity = 0.958199024f;
            pl->m_speedMultiplier = 5.77000189f;
            break;
        case TriggerID::FastSpeed:
            pl->m_playerSpeed = 1.1f;
            pl->m_yStart = 11.420032f;
            pl->m_gravity = 0.957199f;
            pl->m_speedMultiplier = 5.870002f;
            break;
        case TriggerID::FasterSpeed:
            pl->m_playerSpeed = 1.3f;
            pl->m_yStart = 11.230032f;
            pl->m_gravity = 0.961199f;
            pl->m_speedMultiplier = 6.000002f;
            break;
        case TriggerID::FastestSpeed:
            pl->m_playerSpeed = 1.6f;
            pl->m_yStart = 11.230032f;
            pl->m_gravity = 0.961199f;
            pl->m_speedMultiplier = 6.000002f;
            break;
        case TriggerID::Reverse:
            player1->reversePlayer(nullptr);
            if(currState.isDuel) player2->reversePlayer(nullptr);
            break;
        case TriggerID::Gravity:
            if(obj->m_followCPP){
                if(playerIndex(pl) == 1) player1->m_gravityMod = obj->m_gravityValue;
                else if(playerIndex(pl) == 1) player1->m_gravityMod = obj->m_gravityValue;
                break;
            }
            if(!obj->m_targetPlayer2)
                player1->m_gravityMod = obj->m_gravityValue;
            if(!obj->m_targetPlayer1)
                player2->m_gravityMod = obj->m_gravityValue;
            break;
        case TriggerID::PlayerControl: {
            bool p1Target = obj->m_targetPlayer1 || !obj->m_targetPlayer2;
            bool p2Target = obj->m_targetPlayer2;
            auto tgr = static_cast<PlayerControlGameObject*>(obj);

            auto applyTrigger = [&](PlayerObject* player) {
                if(tgr->m_stopJump) player->releaseButton(PlayerButton::Jump);

                if(playLayer->m_isPlatformer && tgr->m_stopMove){
                    player->releaseButton(static_cast<PlayerButton>(5));
                    player->releaseButton(PlayerButton::Left);
                    player->releaseButton(PlayerButton::Right);
                }

                if(tgr->m_stopRotation) player->stopRotation(true, 0);
                if(tgr->m_stopSlide) player->handlePlayerCommand(543);
            };

            if(p1Target) applyTrigger(player1);
            if(p2Target) applyTrigger(player2);
        } break;
        case TriggerID::Teleport:
            break; // todo: later
        case TriggerID::Options: {
            auto isOn = [](GameOptionsSetting s){
                return s == GameOptionsSetting::On;
            };
            auto isSet = [](GameOptionsSetting s){
                return s == GameOptionsSetting::On || s == GameOptionsSetting::Off;
            };
            auto tgr = static_cast<GameOptionsTrigger*>(obj);

            if(isSet(tgr->m_boostSlide)){
                bool status = isOn(tgr->m_boostSlide);
                player1->m_decreaseBoostSlide = status;
                player2->m_decreaseBoostSlide = status;
            }

            if(isSet(tgr->m_unlinkDualGravity))
                currState.isGravityUnlinked = isOn(tgr->m_unlinkDualGravity);

            if(isSet(tgr->m_disableP1Controls)){
                if(isOn(tgr->m_disableP1Controls))
                    player1->disablePlayerControls();
                else
                    player1->enablePlayerControls();
            }

            if(isSet(tgr->m_disableP2Controls)){
                if(isOn(tgr->m_disableP2Controls))
                    player2->disablePlayerControls();
                else
                    player2->enablePlayerControls();
            }
        } break;
        case TriggerID::End:
            endTrajectory = true;
            break;
        case TriggerID::Arrow: {
            auto tgr = static_cast<RotateGameplayGameObject*>(obj);

            if(tgr->m_changeChannel)
                playLayer->m_gameState.m_currentChannel = tgr->m_targetChannelID;

            if(!tgr->m_channelOnly){
                auto changeDirection = [&](PlayerObject* player) {
                    player->rotateGameplay(
                        tgr->m_moveDirection,
                        tgr->m_groundDirection,
                        tgr->m_editVelocity,
                        tgr->m_velocityModX,
                        tgr->m_velocityModY,
                        tgr->m_overrideVelocity,
                        tgr->m_dontSlide
                    );
                };

                changeDirection(player1);
                if(currState.isDuel) changeDirection(player2);
            }
        } break;
    }
}

void TrajectoryManager::handlePad(PlayerObject* pl, EffectGameObject* obj){
    pl->m_lastPortalPos = obj->getPosition();
    pl->m_lastActivatedPortal = obj;

    auto padType = obj->getType();

    float bumpMod = playLayer->getBumpMod(pl, std::to_underlying(padType));

    if(obj->m_isReverse) pl->reversePlayer(obj);

    if(pl->m_isPlatformer || !pl->m_fixRobotJump) pl->m_touchedPad = true;

    switch(padType){
        case GameObjectType::PinkJumpPad:
        case GameObjectType::YellowJumpPad:
        case GameObjectType::RedJumpPad: {
            pl->propellPlayer(bumpMod, true, std::to_underlying(padType));

            if(padType == GameObjectType::RedJumpPad){
                pl->m_isAccelerating = true;
                pl->m_lastGroundedPos = ccp(0.f, 0.f);
            } else pl->m_isAccelerating = false;
        } break;
        case GameObjectType::GravityPad: {
            bool targetGravity = pl->m_isSideways ? obj->isFacingLeft() : obj->isFacingDown();
            pl->propellPlayer(0.8f, true, std::to_underlying(padType));
            flipGravity(pl, targetGravity);
            pl->m_isAccelerating = false;
        } break;
        case GameObjectType::SpiderPad: {
            bool targetGravity = pl->m_isSideways ? obj->isFacingLeft() : obj->isFacingDown();

            pl->flipGravity(targetGravity, true);
            pl->spiderTestJump(false);
        } break;
    }

    static_cast<TrajectoryGameObject*>(obj)->activatedByTrajectory(pl, playerIndex(pl));
}

bool TrajectoryManager::canBeActivated(PlayerObject* pl, EffectGameObject* obj){
    bool wasTouching = currState.activatedObjects.contains({obj->m_uniqueID, playerIndex(pl)});
    bool multiActivate = obj->canMultiActivate(pl->m_isPlatformer);

    currState.activatedObjects.emplace(obj->m_uniqueID, playerIndex(pl));

    if(multiActivate) return !wasTouching;

    auto& fields = static_cast<TrajectoryGameObject*>(obj)->m_fields;
    return (playerIndex(pl) != 1) ?
        fields->m_activatedByPlayer1 :
        fields->m_activatedByPlayer2;
}

void TrajectoryManager::flipGravity(PlayerObject* pl, bool flip){
    if(pl->m_isUpsideDown == flip) return;

    pl->flipGravity(flip, true);

    if(currState.isGravityUnlinked || !currState.isDuel ||
        playLayer->m_levelSettings->m_twoPlayerMode) return;

    bool sameMode =
        player1->m_isShip   == player2->m_isShip   &&
        player1->m_isBall   == player2->m_isBall   &&
        player1->m_isBird   == player2->m_isBird   &&
        player1->m_isSpider == player2->m_isSpider &&
        player1->m_isRobot  == player2->m_isRobot  &&
        player1->m_isSwing  == player2->m_isSwing;
    if(!sameMode) return;

    auto other = (playerIndex(pl) == 1) ? player2 : player1;
    other->flipGravity(!flip, true);
}

void TrajectoryManager::setupManager(){
    auto& gs = playLayer->m_gameState;
    auto em = playLayer->m_effectManager;

    refState.isDuel = playLayer->m_player2 != nullptr;
    refState.isGravityUnlinked = gs.m_unkBool31;

    for(auto key : gs.m_activatedObjectIDs | std::views::keys)
        refState.activatedObjects.insert(key);

    refState.activatedTriggers.insert(em->m_unkMap498.begin(), em->m_unkMap498.end());

    refState.speedPortals = CCArray::createWithArray(playLayer->m_speedObjects);

    startingChannel = gs.m_currentChannel;
    refState.channelIndex.insert(gs.m_spawnChannelRelated0.begin(), gs.m_spawnChannelRelated0.end());
}

void TrajectoryManager::reloadManager(bool move){
    if(move) currState = std::move(refState);
    else     currState = refState;
}

void TrajectoryManager::setupPlayers(bool down){
    auto p1 = playLayer->m_player1;
    auto p2 = playLayer->m_player2;

    copyPlayer(player1, p1);
    if(currState.isDuel) copyPlayer(player2, p2);

    // only do action if it wasn't done
    if(p1->m_holdingButtons[std::to_underlying(PlayerButton::Jump)] != down)
        down ? player1->pushButton(PlayerButton::Jump) : player1->releaseButton(PlayerButton::Jump);

    if(currState.isDuel)
        if(p2->m_holdingButtons[std::to_underlying(PlayerButton::Jump)] != down)
            down ? player1->pushButton(PlayerButton::Jump) : player1->releaseButton(PlayerButton::Jump);
}

void TrajectoryManager::resetGJBGL(){
    auto& gs = playLayer->m_gameState;
    auto em = playLayer->m_effectManager;

    gs.m_currentChannel = startingChannel;
}

Hitboxes TrajectoryManager::getHitboxVert(PlayerObject* pl, float angle){
    auto outerRect = pl->getObjectRect();
    std::array outerHitbox{
        ccp(outerRect.getMinX() /*+ lineThickness*/, outerRect.getMaxY() /*- lineThickness*/),
        ccp(outerRect.getMaxX() /*- lineThickness*/, outerRect.getMaxY() /*- lineThickness*/),
        ccp(outerRect.getMaxX() /*- lineThickness*/, outerRect.getMinY() /*+ lineThickness*/),
        ccp(outerRect.getMinX() /*+ lineThickness*/, outerRect.getMinY() /*+ lineThickness*/)
    };

    auto innerRect = pl->getObjectRect(0.3, 0.3);
    std::array innerHitbox{
        ccp(innerRect.getMinX() /*+ lineThickness*/, innerRect.getMaxY() /*- lineThickness*/),
        ccp(innerRect.getMaxX() /*- lineThickness*/, innerRect.getMaxY() /*- lineThickness*/),
        ccp(innerRect.getMaxX() /*- lineThickness*/, innerRect.getMinY() /*+ lineThickness*/),
        ccp(innerRect.getMinX() /*+ lineThickness*/, innerRect.getMinY() /*+ lineThickness*/)
    };

    auto rotatedHitbox = outerHitbox;
    auto center = ccp(
        (outerRect.getMinX() + outerRect.getMaxX()) / 2.f,
        (outerRect.getMinY() + outerRect.getMaxY()) / 2.f
    );

    angle = CC_DEGREES_TO_RADIANS(angle * -1.f);
    for(auto& vertex : rotatedHitbox){
        auto pnt = vertex - center;
        float cosine = cos(angle);
        float sine = sin(angle);

        vertex.x = center.x + (pnt.x * cosine) - (pnt.y * sine);
        vertex.y = center.y + (pnt.x * sine) + (pnt.y * cosine);
    }

    return {outerHitbox, innerHitbox, rotatedHitbox};
}

void TrajectoryManager::renderDeath(){
    auto& cm = ColorManager::get();
    auto node = getTrajectoryNode();

    auto verticesP1 = getHitboxVert(player1, p1Rotation);
    node->drawPolygon(verticesP1.rotatedHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::RotatedHitbox),
        BorderAlignment::Inside
    );
    node->drawCircle(
        player1->getPosition(),
        (player1->getObjectRect().size.height/2.f) - lineThickness,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::CircleHitbox),
        25
    );
    node->drawPolygon(verticesP1.outerHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::OuterHitbox),
        BorderAlignment::Inside
    );
    node->drawPolygon(verticesP1.innerHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::InnerHitbox),
        BorderAlignment::Inside
    );

    if(!currState.isDuel) return;

    auto verticesP2 = getHitboxVert(player2, p2Rotation);
    node->drawPolygon(verticesP2.rotatedHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::RotatedHitbox),
        BorderAlignment::Inside
    );
    node->drawCircle(
        player2->getPosition(),
        (player2->getObjectRect().size.height/2.f) - lineThickness,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::CircleHitbox),
        25
    );
    node->drawPolygon(verticesP2.outerHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::OuterHitbox),
        BorderAlignment::Inside
    );
    node->drawPolygon(verticesP2.innerHitbox.data(),
        4,
        ccc4f(0,0,0,0),
        lineThickness,
        cm.getColor(ColorIdx::InnerHitbox),
        BorderAlignment::Inside
    );
}