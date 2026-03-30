module;

#include "include.hpp"

export module Trajectory;

export import ColorManager;

import boost.unordered;
using namespace boost::unordered;

export {

struct TrajectoryDrawNode : public CCDrawNode {
    static TrajectoryDrawNode* create(){
        auto ret = new TrajectoryDrawNode();
        if(ret->init()){
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

enum class TriggerID { // add camera triggers
    SlowSpeed = 200,
    NormalSpeed = 201,
    FastSpeed = 202,
    FasterSpeed = 203,
    FastestSpeed = 1334,

    Arrow = 2900,
    End = 1931,
    Gravity = 2066,
    Options = 2899,
    PlayerControl = 1932,
    Reverse = 1917,
    Teleport = 3022,
    Timewarp = 1935
};

enum class Activate {
    General,
    Player1,
    Player2
};

struct Hitboxes {
    std::array<CCPoint, 4> outerHitbox, innerHitbox, rotatedHitbox;
};

struct TrajectoryState {
    unordered_flat_map<int, int> channelIndex;
    unordered_flat_set<std::pair<int, int>> activatedObjects, activatedTriggers;
    Ref<CCArray> speedPortals = nullptr;
    bool isGravityUnlinked{};
    bool isDuel{};
};

class $modify(TrajectoryGameObject, EffectGameObject){
    struct Fields{
        bool hasBeenReset{};

        bool m_activated;
        bool m_activatedByPlayer1;
        bool m_activatedByPlayer2;
    };

    void activatedByTrajectory(PlayerObject* pl, int idx);
    void resetTrajectoryState();
    void editTrajectoryState(Activate setting, bool state);
};

struct TrajectoryManager {
    using TrajectoryObjField = TrajectoryGameObject::Fields;

    static auto& get(){
        static TrajectoryManager i;
        return i;
    }

    static CCDrawNode* getTrajectoryNode(){
        static TrajectoryDrawNode* i = nullptr;
        if(!i){
            i = TrajectoryDrawNode::create();
            i->retain();
            i->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        }
        return i;
    }

    static void cleanup(){
        auto& tm = get();

        if(tm.getTrajectoryNode()) tm.getTrajectoryNode()->clear();

        tm.player1 = nullptr;
        tm.player2 = nullptr;

        tm.effectObjects.clear();
    }

    TrajectoryObjField* getFields(TrajectoryGameObject* obj){
        if(effectObjects.contains(obj))
            return effectObjects[obj];

        auto fields = obj->m_fields.self();
        effectObjects.emplace(obj, fields);
        return fields;
    }
    TrajectoryObjField* getFields(EffectGameObject* obj){
        auto ptr = static_cast<TrajectoryGameObject*>(obj);
        if(effectObjects.contains(ptr))
            return effectObjects[ptr];

        auto fields = ptr->m_fields.self();
        effectObjects.emplace(ptr, fields);
        return fields;
    }

    int playerIndex(PlayerObject* pl) const {
        return (pl == player1) ? 1 : 2;
    }

    PlayLayer* playLayer = nullptr;

    PlayerObject *player1 = nullptr, *player2 = nullptr;
    float p1Rotation{}, p2Rotation{};
    std::vector<CCPoint> p1TrajectoryPoints, p2TrajectoryPoints;

    bool useTrajectory{};

    float lineThickness{};
    uint64_t lookaheadLength{};

    bool endTrajectory{};
    bool trajectoryActive{};

    TrajectoryState refState;
    TrajectoryState currState;

    unordered_flat_map<TrajectoryGameObject*, TrajectoryObjField*> effectObjects;

    int startingChannel{};

    bool renderCircleHitbox{};
    float delta = 1/240.f;

    void startTrajectory();
    void updateTrajectory(bool down);

    void handleOrb(PlayerObject* pl, RingObject* obj);
    void handlePortal(PlayerObject* pl, EffectGameObject* obj);
    void handleTrigger(PlayerObject* pl, EffectGameObject* obj);
    void handlePad(PlayerObject* pl, EffectGameObject* obj);

    bool canBeActivated(PlayerObject* pl, EffectGameObject* obj);
    void flipGravity(PlayerObject* pl, bool flip);

    void setupManager();
    void reloadManager(bool move);
    void setupPlayers(bool down);
    void resetGJBGL();

    Hitboxes getHitboxVert(PlayerObject* pl, float angle);
    void renderDeath();

    TrajectoryManager(const TrajectoryManager&) = delete;
    TrajectoryManager& operator=(const TrajectoryManager&) = delete;
    TrajectoryManager(TrajectoryManager&&) = delete;
    TrajectoryManager& operator=(TrajectoryManager&&) = delete;

private:
    TrajectoryManager() = default;
};

constinit auto& tm = TrajectoryManager::get();

}