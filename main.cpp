#include <unordered_map>
#include "hoolib.hpp"
#include "canvas.hpp"

#include <iomanip>
#include <iostream>

enum PICID {
    HANAKUN_01,
    HANAKUN_02,
    HANAKUN_03,
    HANAKUN_04,
    HANAKUN_05,
    HANAKUN_06,
};

class NPuzzle
{
private:
    const int width_, height_;
    std::vector<int> blocks_;

    std::vector<int>::iterator get0Iterator()
    {
        return std::find(HOOLIB_RANGE(blocks_), 0);
    }

    std::vector<int>::const_iterator get0Iterator() const
    {
        return std::find(HOOLIB_RANGE(blocks_), 0);
    }

    int get0Index() const
    {
        return get0Iterator() - blocks_.begin();
    }

    std::pair<int, int> index2xy(int index) const
    {
        return std::make_pair(index % width_, index / width_);
    }

    int xy2index(int x, int y) const
    {
        return y * width_ + x;
    }

    int& at(int x, int y) { return blocks_.at(xy2index(x, y)); }

public:
    NPuzzle(int width, int height)
        : width_(width), height_(height), blocks_(width * height, 0)
    {
        HOOLIB_THROW_UNLESS(
            width >= 1 && height >= 1 && width * height >= 2,
            "not enough size"
        );
        for(int i = 0;i < width * height - 1;i++)   blocks_.at(i) = i + 1;
        while(true){
            int nos = HooLib::shuffle(HOOLIB_RANGE(blocks_));
            nos += HooLib::iter_swap(get0Iterator(), blocks_.end() - 1);
            if(HooLib::odd(nos))    HooLib::swap(blocks_[0], blocks_[1]);

            bool hasProperlyShuffled = false;
            for(int i = 0;i < width * height - 1;i++)
                if(blocks_.at(i) != i)  hasProperlyShuffled = true;
            if(hasProperlyShuffled) break;
        }
    }

    bool isCorrect() const
    {
        for(int i = 0;i < width_ * height_ - 1;i++)
            if(blocks_[i] != i + 1) return false;
        return blocks_[width_ * height_ - 1] == 0;
    }

    int at(int x, int y) const { return blocks_.at(xy2index(x, y)); }
    std::vector<int>& data() { return blocks_; }
    const std::vector<int>& data() const { return blocks_; }

    enum DIRECTION { NORTH, SOUTH, WEST, EAST };
    void move(DIRECTION dir)
    {
        int dx = 0, dy = 0;
        switch(dir)
        {
        case DIRECTION::NORTH:  dy = 1;  break;
        case DIRECTION::SOUTH:  dy = -1; break;
        case DIRECTION::WEST:   dx = 1; break;
        case DIRECTION::EAST:   dx = -1;  break;
        }
        auto xy = index2xy(get0Index());
        int x = xy.first, y = xy.second;
        HOOLIB_THROW_UNLESS(
            HooLib::betweenEq(0, x + dx, width_ - 1) &&
            HooLib::betweenEq(0, y + dy, height_ - 1),
            "invalid direction"
        );
        HooLib::swap(at(x, y), at(x + dx, y + dy));
    }
};

void dump(const NPuzzle& puzzle)
{
    for(int y = 0;y < 4;y++){
        for(int x = 0;x < 4;x++)
            std::cout << std::setw(2) << puzzle.at(x, y) << " ";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

class PicPuzzle
{
private:
    const int nWidth_, nHeight_;
    NPuzzle puzzle_;
    int picWidth_, picHeight_;
    std::vector<sf::Sprite> picSprites_;

    int at(int x, int y) const { return puzzle_.at(x, y); }

public:
    PicPuzzle(PICID picId, int nWidth, int nHeight)
        : nWidth_(nWidth), nHeight_(nHeight), puzzle_(nWidth, nHeight), picSprites_(nWidth * nHeight - 1)
    {
        const auto& texture = TextureManager::getInstance().get(picId);
        auto size = texture.getSize();
        picWidth_ = HooLib::divd(size.x, nWidth);
        picHeight_ = HooLib::divd(size.y, nHeight);
        for(int i = 0;i < nWidth * nHeight - 1;i++){
            picSprites_[i].setTexture(texture);
            picSprites_[i].setTextureRect(
                sf::IntRect(picWidth_ * (i % nWidth), picHeight_ * (i / nWidth), picWidth_, picHeight_)
            );
        }
    }

    bool hasFinished() const { return puzzle_.isCorrect(); }

    bool move(NPuzzle::DIRECTION dir)
    {
        try{
            puzzle_.move(dir);
        }
        catch(...){}
        return puzzle_.isCorrect();
    }

    void draw(const sf::Vector2f& offset, sf::RenderTarget& window)
    {
        for(int y = 0;y < nHeight_;y++){
            for(int x = 0;x < nWidth_;x++){
                int i = at(x, y);
                auto pos = offset + sf::Vector2f(x * picWidth_, y * picHeight_);
                if(i == 0){ // empty
                    auto rect = sf::RectangleShape(sf::Vector2f(picWidth_, picHeight_));
                    rect.setPosition(pos);
                    rect.setFillColor(sf::Color::Black);
                    window.draw(rect);
                }
                else{
                    auto& sprite = picSprites_.at(i - 1);
                    sprite.setPosition(pos);
                    window.draw(sprite);
                }

                // frame
                const static int FRAME_THICKNESS = 10;
                auto frame = sf::RectangleShape(sf::Vector2f(picWidth_ - FRAME_THICKNESS, picHeight_ - FRAME_THICKNESS));
                frame.setFillColor(sf::Color::Transparent);
                frame.setOutlineThickness(FRAME_THICKNESS);
                frame.setOutlineColor(sf::Color::Black);
                frame.setPosition(offset + sf::Vector2f(x * picWidth_ + FRAME_THICKNESS / 2, y * picHeight_ + FRAME_THICKNESS / 2));
                window.draw(frame);
            }
        }
    }
};

class Keyboard
{
    const static std::unordered_map<sf::Keyboard::Key, NPuzzle::DIRECTION> key2dir;

    bool hasPressed_;
    std::optional<NPuzzle::DIRECTION> prevDir_;

public:
    Keyboard()
        : hasPressed_(false)
    {}

    void update()
    {
        hasPressed_ = false;
        auto it = std::find_if(HOOLIB_RANGE(key2dir), [](auto& elm) {
            return sf::Keyboard::isKeyPressed(elm.first);
        });
        if(it == key2dir.end()) prevDir_ = std::nullopt;
        else{
            if(!prevDir_)    hasPressed_ = true;
            prevDir_ = it->second;
        }
    }

    std::optional<NPuzzle::DIRECTION> getPressedDir() const
    {
        return hasPressed_ ? prevDir_ : std::nullopt;
    }
};

const std::unordered_map<sf::Keyboard::Key, NPuzzle::DIRECTION> Keyboard::key2dir = {
    {sf::Keyboard::Left, NPuzzle::DIRECTION::WEST},
    {sf::Keyboard::Right, NPuzzle::DIRECTION::EAST},
    {sf::Keyboard::Up, NPuzzle::DIRECTION::NORTH},
    {sf::Keyboard::Down, NPuzzle::DIRECTION::SOUTH}
};


class Scene;
using ScenePtr = std::shared_ptr<Scene>;
class Scene : public std::enable_shared_from_this<Scene>
{
public:
    Scene(){}
    virtual ~Scene(){}

    virtual ScenePtr process(sf::RenderTarget& window, const Keyboard& keyboard) = 0;
};

class Wiping : public Scene
{
private:
    std::array<ScenePtr, 2> scenes_;
    bool hasInitialized_;
    std::array<sf::RenderTexture, 2> screens_;
    double speed_, level_;
    sf::Clock clock_;

public:
    Wiping(const ScenePtr& prev, const ScenePtr& next, double speed = 0.25, double level = 0)
        : scenes_({prev, next}), hasInitialized_(false), speed_(speed), level_(level)
    {}

    ScenePtr process(sf::RenderTarget& window, const Keyboard& keyboard) override
    {
        auto size = window.getSize();
        if(!hasInitialized_){
            HOOLIB_THROW_UNLESS(screens_[0].create(size.x, size.y), "can't make screen to wipe");
            HOOLIB_THROW_UNLESS(screens_[1].create(size.x, size.y), "can't make screen to wipe");
        }

        for(int i = 0;i < 2;i++){
            auto& screen = screens_[i];
            screen.clear();
            scenes_[i]->process(screen, keyboard);
            screen.display();
            int y = (1 - i) * level_ * size.y,
                height = (1 - i) * (1 - level_) * size.y + i * level_ * size.y;
            sf::Sprite sprite;
            sprite.setTexture(screen.getTexture());
            sprite.setTextureRect(sf::IntRect(0, y, size.x, height));
            sprite.setPosition(sf::Vector2f(0, y));
            window.draw(sprite);
        }

        level_ += clock_.restart().asSeconds() * speed_;
        return level_ < 1 ? shared_from_this() : scenes_[1];
    }
};

class Finish : public Scene
{
    enum { INTERVAL = 500 };
private:
    int spriteIndex_;
    std::vector<sf::Sprite> picSprites_;
    sf::Clock clock_;

public:
    Finish()
        : spriteIndex_(0), picSprites_(6)
    {
        picSprites_[0].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_01));
        picSprites_[1].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_02));
        picSprites_[2].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_03));
        picSprites_[3].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_04));
        picSprites_[4].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_05));
        picSprites_[5].setTexture(TextureManager::getInstance().get(PICID::HANAKUN_06));
    }

    ScenePtr process(sf::RenderTarget& window, const Keyboard& keyboard) override
    {
        if(clock_.getElapsedTime().asMilliseconds() > INTERVAL){
            spriteIndex_ = (spriteIndex_ + 1) % 6;
            clock_.restart();
        }
        window.clear(sf::Color::White);
        window.draw(picSprites_[spriteIndex_]);
        return shared_from_this();
    }
};

class Game : public Scene
{
private:
    PicPuzzle puzzle_;

public:
    Game()
        : puzzle_(PICID::HANAKUN_01, 3, 3)
    {}

    ScenePtr process(sf::RenderTarget& window, const Keyboard& keyboard) override
    {
        auto ret = shared_from_this();
        if(!puzzle_.hasFinished()){
            if(auto dir = keyboard.getPressedDir())
                if(puzzle_.move(*dir))
                    ret = std::make_shared<Wiping>(shared_from_this(), std::make_shared<Finish>());
        }
        window.clear(sf::Color::White);
        puzzle_.draw(sf::Vector2f(0, 0), window);
        return ret;
    }
};

int main()
{
    auto& texman = TextureManager::getInstance();
    texman.add(PICID::HANAKUN_01, "img/hk-0.png");
    texman.add(PICID::HANAKUN_02, "img/hk-1.png");
    texman.add(PICID::HANAKUN_03, "img/hk-2.png");
    texman.add(PICID::HANAKUN_04, "img/hk-3.png");
    texman.add(PICID::HANAKUN_05, "img/hk-4.png");
    texman.add(PICID::HANAKUN_06, "img/hk-5.png");
    Keyboard keyboard;
    ScenePtr scene = std::make_shared<Game>();
    Canvas("hanakun n-puzzle").run([&](auto& window) {
        keyboard.update();
        scene = scene->process(window, keyboard);
        return 0;
    });

    return 0;
}
