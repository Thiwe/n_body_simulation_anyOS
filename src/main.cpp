#include <SFML/Graphics.hpp>

/*
Requirements
Because this template builds SFML from source, Linux users will need to install the required system packages first. 
On Ubuntu or other Debian-based OSes, that can be done with the commands below. A similar process will be required 
for non-Debian Linux distributions like Fedora.

sudo apt update && sudo apt install \
     libxrandr-dev \
     libxcursor-dev \
     libxi-dev \
     libudev-dev \
     libflac-dev \
     libvorbis-dev \
     libgl1-mesa-dev \
     libegl1-mesa-dev \
     libdrm-dev \
     libgbm-dev
*/
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <queue>
#include <thread>
#include <functional>
#define _USE_MATH_DEFINES
#include <math.h>
#include <SFML/Graphics.hpp>

#define log(x, y) std::cout << x << y << std::endl;

// A simple Rectangle class
struct Rectangle {
    float x, y, width, height;

    Rectangle(float _x, float _y, float _w, float _h)
        : x(_x), y(_y), width(_w), height(_h) {}
};

// Forward declaration of Entity
class Entity;

// The Quadtree class
class QuadTree {
public:
    // Maximum objects per node and maximum levels before stopping subdivision.
    static const int MAX_OBJECTS = 4;
    static const int MAX_LEVELS = 12;

   
    QuadTree(int level, const Rectangle& rectangleBounds)
        : level(level), rectangleBounds(rectangleBounds)
    {
        for (int i = 0; i < 4; i++) {
            children[i] = nullptr;
        }
    }

    int level;                      // Current node level (0 is the root)
    std::vector<Entity*> objects;   // Objects stored in this node
    Rectangle rectangleBounds;               // The region of space this node occupies
    QuadTree* children[4];             // Pointers to four subnodes
    
    void clear();
    void split();
    int getIndex(const Entity* entity) const;
    void insert(Entity* entity);
    void retrieve(std::vector<Entity*>& returnObjects, Entity* entity);
    void draw(sf::RenderWindow& window);

    // Destructor: clears the quadtree to free memory.
    ~QuadTree() {
        clear();
    }
};

class Entity {
public:

     Entity(int screenWidth, int screenHeight, float minRadius, float maxRadius, float spawnLimit, float gravity, float spawnPositionX, float spawnPositionY)
        :   screenWidth(screenWidth),
            screenHeight(screenHeight),
            minRadius(minRadius),
            maxRadius(maxRadius),
            spawnLimit(spawnLimit),
            gravity(gravity),
            spawnPositionX(spawnPositionX),
            spawnPositionY(spawnPositionY) 
     {
         radius = getRandRadius();
         pos = sf::Vector2f{ spawnPositionX, spawnPositionY };
         shape = sf::CircleShape(radius);
         shape.setOrigin(sf::Vector2f{ radius, radius });
         shape.setPosition(pos);
         shape.setFillColor(sf::Color::Green);
         shape.setPointCount(20);
         vel.x = 1.f;
         mass = radius;
     }

    float getRandRadius() {
        // Thread-local to avoid contention in multithreaded code
        static thread_local std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> distribution(minRadius, maxRadius);
        return distribution(generator);
    };


public: //should ´be private
    void update(float deltatime);
    void render(sf::RenderWindow& window);
    void borderCollision();
    void entityCollision(Entity* first, Entity* second, float deltaTime, sf::RenderWindow& window);
    void checkIdle(float threshold = 0.1f);

    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::CircleShape shape;
    float radius;
    float mass = 1;
    bool idle = false;  // New flag

    int screenWidth;
    int screenHeight;
    float minRadius;
    float maxRadius;
    float spawnLimit;
    float gravity;
    float spawnPositionX;
    float spawnPositionY;
    bool active = false;
	static float speedMod;

};

static int idleFrameCounter = 0;  // Declare correctly
static int debugThreshold = 10;
static bool tabPressed = false;
static bool pause = false;
static int maxCollsionCheckLevels = 4;
float Entity::speedMod = 1.f;




//Window size, min/max circle radius values, spawn limit, and gravity are given as command line arguments to the program.
int main(int argc, char* argv[]) {
   
    if (argc < 9) {
        std::cout << "Usage: " << argv[0] << " <window_size> <spawn_position> <input_variable>" << std::endl;
        return 1;
    }

    uint16_t screenWidth = std::stoi(argv[1]);
    uint16_t screenHeight = std::stoi(argv[2]);
    float minRadius = std::stof(argv[3]);
    float maxRadius = std::stof(argv[4]);
    float spawnLimit = std::stof(argv[5]);
    float gravity = std::stof(argv[6]);
    float spawnPositionX = std::stof(argv[7]);
    float spawnPositionY = std::stof(argv[8]);

    std::vector<Entity*> entities;
    for (int i = 0; i <= spawnLimit; i++) {
        entities.push_back(new Entity(screenWidth, screenHeight, minRadius, maxRadius, spawnLimit, gravity, spawnPositionX, spawnPositionY));
    }

    float initSpawnIntervall = 0.000000001;
    float spawnIntervall = initSpawnIntervall;
    int indexEntityToSpawn = 1; //start at one since we already spoawned 0


    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode({ screenWidth, screenHeight }), "SFML works!");
    Rectangle rootRect(0, 0, screenWidth, screenHeight);
    QuadTree* quadTree = new QuadTree(0, rootRect);

    while (window.isOpen())
    {
        //restart() returns a time object which ahve asSeconds as member
        
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>() || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                std::cout << indexEntityToSpawn << std::endl;
                window.close();
            }
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
            pause = !pause;
            Entity::speedMod = 0.01f;

        }
        if (!pause) {
            float deltaTime = clock.restart().asSeconds();
            window.clear();
            quadTree->clear();
            

            for (Entity* entity : entities) {
                if (entity->active) {
                    quadTree->insert(entity);
                    entity->update(deltaTime);
                    //entity->checkIdle(200);  // Check if entity is idle
                    // Run border collision 
				    entity->borderCollision();

                    //std::vector<Entity*> candidates;
                    //quadTree->retrieve(candidates, entity);
                    //for (Entity* other : candidates) {
                    //    if (entity != other) {
                    //        // Call your sphere collision function.
                    //        entity->entityCollision(entity, other, deltaTime);
                    //    }
                    //}
                    // Only check collision every 10 frames for idle entities

                    //borderCollisionThreads.emplace_back(&Entity::borderCollision, entity);
                    std::vector<Entity*> candidates;
                    quadTree->retrieve(candidates, entity);
					maxCollsionCheckLevels = 4;
                    for (Entity* other : candidates) {
                        if (entity != other) {
                            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab))
                                tabPressed = !tabPressed;

                            if (tabPressed)
                            {
                                std::array line =
                                {
                                    sf::Vertex{entity->pos}                                                                                                                                  ,
                                    sf::Vertex{other->pos}
                                };

                                window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
                            }
                            //collisionThreads.emplace_back(&Entity::entityCollision, entity, entity, other, deltaTime);
                            entity->entityCollision(entity, other, deltaTime, window);
                        }
                    }
                   /* if (!entity->idle || idleFrameCounter % 30 == 0) {
                    }*/
                

                    entity->render(window);

                }
            }
            //// Wait for all collision threads to finish
            //for (std::thread& t : collisionThreads) {
            //    if (t.joinable()) t.join();
            //}

            //// Wait for all collision threads to finish
            //for (std::thread& t : collisionThreads) {
            //    if (t.joinable()) t.join();
            //}

            idleFrameCounter++;  // Correctly increment
       

            if (spawnIntervall < 0) {
                spawnIntervall = initSpawnIntervall;
                if (indexEntityToSpawn  < entities.size()) {
                    entities[indexEntityToSpawn]->active = true;
                    indexEntityToSpawn++;
                 
                }
            }
		    debugThreshold -= deltaTime;
            spawnIntervall -= deltaTime;
            /*float fps = (deltaTime  ? 1.0f / deltaTime : 0.f;
		    float lowFPS = 1000.f;
            lowFPS = fps < lowFPS ? fps : lowFPS;
            std::cout << lowFPS << std::endl;*/

        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            quadTree->draw(window);
        }

        window.display();
    }

    return 0;
}

void Entity::borderCollision()
{
    // Right wall collision
    if (pos.x + radius > screenWidth) {
        pos.x = screenWidth - radius;  // Prevent going out of bounds
        vel.x *= -0.8f; // Reverse velocity with damping to simulate energy loss
    }
    // Left wall collision
    else if (pos.x - radius < 0) {
        pos.x = radius;
        vel.x *= -0.8f;
    }

    // Bottom wall collision
    if (pos.y + radius > screenHeight) {
        pos.y = screenHeight - radius;
        vel.y *= -0.8f; // Reverse velocity to simulate bouncing
    }
    // Top wall collision
    else if (pos.y - radius < 0) {
        pos.y = radius;
        vel.y *= -0.8f;
    }
}

void Entity::entityCollision(Entity* first, Entity* second, float deltaTime, sf::RenderWindow& window) {

    //The direction of the collision, or normal vector
    sf::Vector2f collisionNormalVector = first->pos - second->pos;
    float distanceNormalVec = std::sqrt(collisionNormalVector.x * collisionNormalVector.x
                                + collisionNormalVector.y * collisionNormalVector.y);
    
    //The direction of the collision plane, perpendicular to the normal
    sf::Vector2f collisionDirectionVector = sf::Vector2f(-collisionNormalVector.y, collisionNormalVector.x);
    float distanceNormalPlaneVec = std::sqrt(collisionDirectionVector.x * collisionDirectionVector.x
        + collisionDirectionVector.y * collisionDirectionVector.y);

    float overlap = (first->radius + second->radius) - distanceNormalVec;
    

    if (distanceNormalVec < first->radius + second->radius) {
        // **Push apart**
        if (distanceNormalVec == 0)
            distanceNormalVec += 0.0001;

        // Normalize collision direction
        sf::Vector2f collisionNormalized = collisionNormalVector / distanceNormalVec;
        sf::Vector2f collisionDirecetionNormalized = collisionDirectionVector / distanceNormalPlaneVec;
        
        // Calculate dot product of velocity and normal for each entity
        float dotProductFirstParallel = first->vel.x * collisionNormalized.x + first->vel.y * collisionNormalized.y;
        float dotProductFirstOrtho = first->vel.x * collisionDirecetionNormalized.x + first->vel.y * collisionDirecetionNormalized.y;

        float dotProductSecondParallel = second->vel.x * collisionNormalized.x + second->vel.y * collisionNormalized.y;
        float dotProductSecondOrtho = second->vel.x * collisionDirecetionNormalized.x + second->vel.y * collisionDirecetionNormalized.y;
        
        sf::Vector2f firstVelParallel = dotProductFirstParallel * collisionNormalized;
        sf::Vector2f secondVelParallel = dotProductSecondParallel * collisionNormalized;

        sf::Vector2f firstVelOrtho = dotProductFirstOrtho * collisionDirecetionNormalized;
        sf::Vector2f secondVelOrtho = dotProductSecondOrtho * collisionDirecetionNormalized;

        //the orthogonal component is not affected by the collision. apply physics to the parallel components:
        float distanceFirst = std::sqrt(firstVelParallel.x * firstVelParallel.x
            + firstVelParallel.y * firstVelParallel.y);
        float distanceSecond = std::sqrt(secondVelParallel.x * secondVelParallel.x
            + secondVelParallel.y * secondVelParallel.y);

        // Update velocities - project velocities onto the normal and reflect
        float commonVel = 2 * (first->mass * distanceFirst + second->mass * distanceSecond) / (first->mass + second->mass);


        float v1LengthAfterCollision = commonVel - distanceFirst;
        float v2LengthAfterCollision = commonVel - distanceSecond;

        //in case of spawned which can lead to division with 0
        distanceFirst += 0.0000001;
        distanceSecond += 0.0000001;
        firstVelParallel *= (v1LengthAfterCollision / distanceFirst);
        secondVelParallel *= (v2LengthAfterCollision / distanceSecond);

        first->vel = firstVelParallel + firstVelOrtho;
        second->vel = secondVelParallel + secondVelOrtho;

    
    }

}


void Entity::update(float deltatime)
{
    
    //vel.x = vel.x * deltatime;
    vel.y += gravity * deltatime;

    pos.x += vel.x * speedMod;
    pos.y += vel.y * speedMod;

    

}

void Entity::render(sf::RenderWindow& window)
{
    //wrapAround(sf::Vector2u(1200, 900)); // call wrap function after updating position
    shape.setPosition(pos);
    window.draw(shape);

}

void Entity::checkIdle(float threshold) {
	//std::cout << vel.x << " " << vel.y << std::endl;
    if (std::abs(vel.x) < threshold && std::abs(vel.y) < threshold) {
        idle = true;
    }
    else {
        idle = false;
    }
}

void QuadTree::clear()
{
    objects.clear();
    for (int i = 0; i < 4; i++) {
        if (children[i] != nullptr) {
            children[i]->clear();
            delete children[i];
            children[i] = nullptr;
        }
    }
}

/*
* Splits the node into 4 subnodes
*/
void QuadTree::split()
{
    int subWidth = static_cast<int>(rectangleBounds.width / 2);
    int subHeight = static_cast<int>(rectangleBounds.height / 2);
    int x = static_cast<int>(rectangleBounds.x);
    int y = static_cast<int>(rectangleBounds.y);

    // Create the four subnodes with their respective bounds.
    children[0] = new QuadTree(level + 1, Rectangle(x + subWidth, y, subWidth, subHeight));
    children[1] = new QuadTree(level + 1, Rectangle(x, y, subWidth, subHeight));
    children[2] = new QuadTree(level + 1, Rectangle(x, y + subHeight, subWidth, subHeight));
    children[3] = new QuadTree(level + 1, Rectangle(x + subWidth, y + subHeight, subWidth, subHeight));


}

// Determine which node the object belongs to.
// Returns:
//   0, 1, 2, or 3 if the object fits completely within a subnode,
//   -1 if the object cannot completely fit within a child node.
int QuadTree::getIndex(const Entity* entity) const {
    int index = -1;

    float xMidpoint = rectangleBounds.x + (rectangleBounds.width / 2.0f);  // Horizontal division
    float yMidpoint = rectangleBounds.y + (rectangleBounds.height / 2.0f);  // Vertical division

    float leftEdge = entity->pos.x - entity->radius;
    float rightEdge = entity->pos.x + entity->radius;
    float topEdge = entity->pos.y - entity->radius;
    float bottomEdge = entity->pos.y + entity->radius;

    // Check if completely in top half
    bool inTopHalf = (bottomEdge < yMidpoint);
    // Check if completely in bottom half
    bool inBottomHalf = (topEdge > yMidpoint);
    // Check if completely in left half
    bool inLeftHalf = (rightEdge < xMidpoint);
    // Check if completely in right half
    bool inRightHalf = (leftEdge > xMidpoint);

    // Determine quadrant (0=top-right, 1=top-left, 2=bottom-left, 3=bottom-right)
    if (inTopHalf) {
        if (inRightHalf) return 0;
        if (inLeftHalf) return 1;
    }
    else if (inBottomHalf) {
        if (inLeftHalf) return 2;
        if (inRightHalf) return 3;
    }

    // Circle doesn't fit completely in any quadrant
    return -1;
}


// Insert the object into the quadtree.
// If the node already has child nodes, try to pass the object 
// to the appropriate child.
// Otherwise, store the object here and split if necessary.
void QuadTree::insert(Entity* entity)
{
    if (children[0] != nullptr) {
        int index = getIndex(entity);
        if (index != -1) {
            children[index]->insert(entity);
            return;
        }
    }
    // If we can't fully fit in a subnode, store it here.
    objects.push_back(entity);

    // If the number of objects exceeds the capacity and we haven't reached the maximum level,
    // split the node and redistribute objects.
    if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS) {
        if (children[0] == nullptr) {
            split();
        }

        // Iterate over objects and move those that fit completely into a child node.
        int i = 0;
        while (i < objects.size()) {
            int index = getIndex(objects[i]);
            if (index != -1) {
                Entity* obj = objects[i];
                objects.erase(objects.begin() + i);
                children[index]->insert(obj);
                // Do not increment i since the vector has shifted.
            }
            else {
                i++;
            }
        }
    }
}


void QuadTree::retrieve(std::vector<Entity*>& returnObjects, Entity* entity) {
    if(debugThreshold < 0) {
        debugThreshold = 10;
		//do stuff here
    }
    int index = getIndex(entity);
    if (index != -1 && children[0] != nullptr) {
        children[index]->retrieve(returnObjects, entity);
    }

 //   maxCollsionCheckLevels--;
	//if (maxCollsionCheckLevels > 0) {
	//	returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
	//	
	//}
    // Add objects from the current node.
    returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
 
}

void QuadTree::draw(sf::RenderWindow& window) {
    sf::RectangleShape rect(sf::Vector2f(rectangleBounds.width, rectangleBounds.height));
    rect.setPosition(sf::Vector2f(rectangleBounds.x, rectangleBounds.y));
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(1);
    rect.setOutlineColor(sf::Color(255, 255, 255, 100)); // Semi-transparent white

    window.draw(rect);

    // Recursively draw child nodes
    if (children[0] != nullptr) {
        for (int i = 0; i < 4; i++) {
            children[i]->draw(window);
        }
    }
}
