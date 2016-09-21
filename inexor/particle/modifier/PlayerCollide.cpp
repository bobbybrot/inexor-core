/*
 * PlayerCollide.cpp
 *
 *  Created on: 28.01.2015
 *      Author: aschaeffer
 */

#include "PlayerCollide.hpp"

namespace inexor {
namespace entity {
namespace particle {

    PlayerCollide::PlayerCollide() : EntityFunction(MODIFIER_PLAYER_COLLIDE_FUNCTION)
    {
    }

    PlayerCollide::~PlayerCollide()
    {
    }

    void PlayerCollide::Execute(TimeStep time_step, std::shared_ptr<EntityInstance> modifier, std::shared_ptr<EntityInstance> particle)
    {
        float elasticity = 10.0f;
        physent d;
        d.type = ENT_BOUNCE;
        d.state = CS_ALIVE;
        d.collidetype = COLLIDE_ELLIPSE_PRECISE;
        d.radius = d.xradius = d.yradius = d.eyeheight = d.aboveeye = 12.0f;

        // elasticity = (*modifier)["elasticity"];
        d.o = vec((*particle)[POS]->vec3Val);
        d.vel = vec((*particle)[VELOCITY]->vec3Val);
        // suppress collision check for mapmodels
        if (plcollide(&d, vec((*particle)[VELOCITY]->vec3Val)))
        {
            game::bounced(&d, collidewall);
            float c = collidewall.dot(d.vel);
            float m = d.vel.magnitude();
            if (m == 0.0f) return;
            float k = 1.0f + (1.0f - elasticity) * c / m;
            d.vel.mul(k);
            d.vel.sub(vec(collidewall).mul(elasticity * 2.0f * c));
            (*particle)[VELOCITY]->vec3Val = vec(d.vel);
        }
    }

}
}
}
