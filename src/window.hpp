#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "textures_manager.hpp"
#include "game.hpp"

class Renderer
{
    public:
        //Will be called when added to the window
        virtual void loadTextures() = 0;

        virtual void render(sf::RenderWindow &screen, sf::Vector2u tile_size, World &world) = 0;
    protected:
        TexturesManager  textures;
};


class Window : public Module
{
    public:
        Window(unsigned short width, unsigned short height, std::string title);

        void            setTileSize(sf::Vector2u tile_size);
        sf::Vector2u    getTileSize() const;

        void            setZoom(unsigned char zoom);
        void            setDesiredFPS(unsigned char fps);
        void            centerViewAt(sf::View &view, Terrain &terrain,
                                     Vector2f center);

        void            addRenderer(std::unique_ptr<Renderer> renderer);

        void            update(World &world, bool &quit);

    private:
        void    processInput(World &world, bool &quit);

        void    render(World &world, sf::View &view);
        void    render(Terrain &terrain, Terrain::Ground ground);
        void    render(Character &character);
        void    render(Arrow &arrow);

        void    renderHUD(World &world);
        void    renderDeadCharacters(World &world);

        void    loadTextures();
        void    addDeadCharacter(const DeadCharacter &dead);

        sf::Clock           clock;
        sf::RenderWindow    screen;
        sf::Vector2u        tile_size;
        unsigned char       zoom;
        sf::View            view;

        std::list<std::pair<std::shared_ptr<Character>, float>>   dead_characters;
        std::vector<std::unique_ptr<Renderer>>  renderers;
};

#endif
