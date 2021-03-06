#include "physic.hpp"

#include "event_manager.hpp"

TileCollision::TileCollision(Character &character, const Vector2f &tile_position,
                             Collisions::Axis axis, float penetration) :
    axis(axis), character(&character), tile_position(tile_position), penetration(penetration)
{
}

bool operator<(const TileCollision &c1, const TileCollision &c2)
{
    return (c1.penetration < c2.penetration);
}

CharactersCollision::CharactersCollision(Character &c1, Character &c2, Collisions::Axis axis) :
    c1(&c1), c2(&c2), axis(axis)
{

}

Physic::Physic() :
    gravity(Vector2f(0.f, 9.81f)), max_resolutions(3), update_step(0.016f), remaining_time(0.f)
{
}

void Physic::setMaxResolutions(unsigned char max)
{
    if (max <= 0)
        max = 1;

    max_resolutions = max;
}

unsigned char Physic::getMaxResolutions() const
{
    return (max_resolutions);
}

void Physic::setGravity(const Vector2f &gravity)
{
    this->gravity = gravity;
}

Vector2f Physic::getGravity() const
{
    return (gravity);
}

void Physic::setUpdateStep(float step)
{
    update_step = step <= 0.f ? 0.016f : step;
}

float Physic::getUpdateStep() const
{
    return (update_step);
}

void Physic::update(World &world, bool &quit)
{
    remaining_time += world.delta_time;
    while (remaining_time >= update_step)
    {
        move(world, update_step);
        resolveCollisions(world);

        remaining_time -= update_step;
    }

    clearAllAccumulators(world);
}

void Physic::update(RigidBody &body, float delta_time) const
{
    body.position += (body.velocity + body.temp_velocity) * delta_time +
                     body.acceleration * delta_time * delta_time * 0.5;
    //adding acceleration to position to have a better movement over irregulate frames
    body.velocity += (body.acceleration + gravity * body.gravity_scale) * delta_time;
}

void Physic::move(World &world, float delta_time) const
{
    for (auto &character : world.characters)
        update(character->body, update_step);

    for (auto &arrow : world.arrows)
    {
        if (arrow->body.velocity.squareLength() > 0.f)
        {
            arrow->previous_velocity = arrow->body.velocity;
            update(arrow->body, update_step);
        }
    }
}

void Physic::resolveCollisions(World &world) const
{
    for (auto &character : world.characters)
        resolve(generateCollisions(world.terrain, *character));

    for (auto &arrow : world.arrows)
        resolveCollisions(world, *arrow);

    generateCollisions(world.characters);
}

void Physic::clearAllAccumulators(World &world) const
{
    for (auto &character : world.characters)
        character->body.clearAccumulators();
}

void Physic::generateCollisions(std::list<std::shared_ptr<Character>> &characters) const
{
    for (auto it = characters.begin(); it != characters.end(); it++)
    {
        auto it2 = it;
        for (it2++; it2 != characters.end(); it2++)
        {
            Vector2f penetration = Collisions::getPenetration((*it)->body, (*it2)->body);
            if (penetration.x != 0.f && penetration.y != 0.f)
            {
                Collisions::resolveWithDynamic((*it)->body, (*it2)->body);

                EventManager<CharactersCollision>::fire(CharactersCollision(*(*it), *(*it2),
                    (abs(penetration.x) < abs(penetration.y) ? Collisions::Axis::X : Collisions::Axis::Y)));
            }
        }
    }
}

std::priority_queue<TileCollision> Physic::generateCollisions(const Terrain &terrain, Character &character) const
{
    std::priority_queue<TileCollision> collisions;

    RigidBody tile;
    tile.size = Vector2f(1.f, 1.f);

    Vector2f start = character.body.position;
    Vector2f end = character.body.position + character.body.size;

    for (int y = start.y; y <= end.y; ++y)
    {
        for (int x = start.x; x <= end.x; ++x)
        {
            if (terrain.isInside(x, y) &&
                terrain.get(Terrain::Ground::Fore, x, y).isSolid())
            {
                tile.position = Vector2f(x, y);

                Vector2f penetration(Collisions::getPenetrationOnXAxis(character.body, tile),
                                     Collisions::getPenetrationOnYAxis(character.body, tile));

                if (penetration.x < 0.f && penetration.y < 0.f)
                {
                    collisions.push(TileCollision(character,
                                                  tile.position,
                                                  penetration.x > penetration.y ?
                                                      Collisions::Axis::X : Collisions::Axis::Y,
                                                  std::abs(penetration.x * penetration.y)));
                }
            }
        }
    }
    return (collisions);
}

void Physic::resolve(std::priority_queue<TileCollision> collisions) const
{
    unsigned char nb_resolutions = 0;

    while (!collisions.empty() && nb_resolutions < max_resolutions)
    {
        resolve(collisions.top());
        collisions.pop();
        ++nb_resolutions;
    }
}

void Physic::resolve(const TileCollision &collision) const
{
    RigidBody tile;
    tile.size = Vector2f(1.f, 1.f);
    tile.position = collision.tile_position;

    if (!collision.character || !Collisions::collide(collision.character->body, tile))
        return;

    Collisions::resolveWithStatic(collision.character->body, tile);

    EventManager<TileCollision>::fire(collision);
}

void Physic::resolveCollisions(World &world, Arrow &arrow) const
{
    for (auto &character : world.characters)
        if (Collisions::collide(arrow.body, character->body))
            arrow.hit(*character);

    RigidBody tile;
    tile.size = Vector2f(1.f, 1.f);

    for (int x = arrow.body.position.x; x <= arrow.body.position.x + arrow.body.size.x; ++x)
    {
        for (int y = arrow.body.position.y; y <= arrow.body.position.y + arrow.body.size.y; ++y)
        {
            tile.position.x = x;
            tile.position.y = y;
            if (world.terrain.isInside(x, y) && world.terrain.get(Terrain::Ground::Fore, x, y).isSolid() &&
                Collisions::collide(arrow.body, tile))
            {
                Collisions::resolveWithStatic(arrow.body, tile);
                arrow.body.velocity.clear();
            }
        }
    }
}


