#pragma once
#ifndef ANQOU_CANVAS_HPP
#define ANQOU_CANVAS_HPP

#include <array>
#include <memory>
#include <sstream>
#include <SFML/Graphics.hpp>
#include "hoolib.hpp"

using namespace HooLib::Geometry;

inline sf::Vector2f toSfVec(const Vec2d& src)
{
    return sf::Vector2f(src.x, src.y);
}

class SfSegment : public sf::Drawable
{
private:
    std::array<sf::Vertex, 2> vertices_;

public:
    SfSegment(const Segment& src)
        : vertices_({toSfVec(src.from()), toSfVec(src.to())})
    {}

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(vertices_.data(), 2, sf::Lines, states);
    }
};

class SfCircle : public sf::CircleShape
{
public:
    SfCircle(const Circle& circle, const sf::Color& color = sf::Color::White)
        : sf::CircleShape(circle.r)
    {
        setOrigin(circle.r, circle.r);
        setPosition(circle.p.x, circle.p.y);
        //setFillColor(sf::Color::White);
        setFillColor(color);
    }
};

class SfDot : public sf::CircleShape
{
    constexpr static double RADIUS = 3, POINT_COUNT = 6;
public:
    SfDot(const Point& pos)
        : sf::CircleShape(RADIUS)
    {
        setOrigin(RADIUS, RADIUS);
        setPosition(pos.x, pos.y);
        setPointCount(POINT_COUNT);
        setFillColor(sf::Color::White);
    }
};

class DebugPrinter : public sf::Drawable
{
private:
    Point pos_;
    sf::Font font_;
    std::stringstream ss_;

public:
    DebugPrinter(const Point& pos)
        : pos_(pos)
    {
        HOOLIB_THROW_UNLESS(
            font_.loadFromFile("Ricty-Regular.ttf"),
            "Can't load font for debug"
        );
    }

    template<class T> DebugPrinter& operator<<(T t)
    {
        ss_ << t;
        return *this;
    }

    DebugPrinter& operator <<(std::ostream& (*manip)(std::ostream&)) {
        manip(ss_);
        return *this;
    }

    DebugPrinter& operator<<(const Vec2d& v)
    {
        ss_ << "(" << v.x << ", " << v.y << ")";
        return *this;
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        auto src = HooLib::splitStrByChars(ss_.str(), "\n");
        for(int i = 0;i < src.size();i++){
            sf::Text text;
            text.setFont(font_);
            text.setString(src[i]);
            text.setCharacterSize(24);
            text.setColor(sf::Color::White);
            text.setPosition(pos_.x, pos_.y + i * 30);
            target.draw(text, states);
        }
    }
};

class Canvas
{
private:
    std::shared_ptr<sf::RenderWindow> window_;
    std::shared_ptr<sf::View> view_;

public:
    Canvas(const std::string& title)
    {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 8;
        window_ = std::make_shared<sf::RenderWindow>(
            sf::VideoMode(640, 640),
            title,
            sf::Style::Default,
            settings
        );
        window_->setFramerateLimit(60);

        view_ = std::make_shared<sf::View>(
            sf::FloatRect(0, 0, 640, 640)
        );
        window_->setView(*view_);
    }

    template<class Func>
    int run(Func func)
    {
        while (window_->isOpen())
        {
            sf::Event event;
            while (window_->pollEvent(event))
                if (event.type == sf::Event::Closed)
                    window_->close();

            window_->clear(sf::Color::Black);
            int res = func(*window_);
            if(res) return res;
            window_->display();
        }

        return 0;
    }
};

class TextureManager
{
public:
    using ID = int;

private:
    TextureManager(){}
    TextureManager(const TextureManager&){}

private:
    std::unordered_map<ID, sf::Texture> textures_;

public:
    static TextureManager& getInstance()
    {
        static TextureManager instance;
        return instance;
    }

    void add(ID id, const std::string& path)
    {
        sf::Texture texture;
        HOOLIB_THROW_UNLESS(texture.loadFromFile(path), "can't load picture");
        texture.setSmooth(true);
        textures_[id] = texture;
    }

    const sf::Texture& get(ID id)
    {
        auto it = textures_.find(id);
        HOOLIB_THROW_IF(it == textures_.end(), "can't find texture");
        return it->second;
    }
};


#endif
