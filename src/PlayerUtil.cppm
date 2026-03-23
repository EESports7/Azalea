module;

#include "include.hpp"
#include "PlayerVars.hpp"

export module PlayerUtil;

export void copyPlayer(PlayerObject* to, PlayerObject* from){
    from->copyAttributes(to);

    to->m_maybeReducedEffects = true;
    to->resetCollisionLog(false);
    to->setPosition(from->getPosition());
    to->setRotation(from->getRotation());

    #define X(var) to->var = from->var;
        PLAYER_VARS
    #undef X
}