#define MOVE_SPEED 2.0f
#define ROTATE_SPEED glm::radians(120.0f)
#define UNITARY_SCALE 3.0f

// Physic sizes (player pos is in middle of the block so will have PLAYER_PHYSICS_SIZE/2 extension on its sides)
#define PLAYER_PHYSICS_LEFT_LENGTH 0.1f
#define PLAYER_PHYSICS_RIGHT_LENGTH 0.3f // Lantern slightly on right
#define PLAYER_PHYSICS_BACK_LENGTH 0.01f
#define PLAYER_PHYSICS_FRONT_LENGTH 1.0f     // Lantern on front
#define CHUNK_SIZE_MAZE_COLLISION_CHECK 4.0f // To improve performance: the maze blocks (chunk) to check for collision must be in this area around the player on each side
#define DISTANCE_CHECK_PLAYER_KEY 1.5f       // Distance to check if player is close to a key to take it (considering player and keys as points)
#define DISTANCE_CHECK_PLAYER_TELEPORT 1.0f
#define INITIAL_PLAYER_HEIGHT 2.0f
#define INVISIBLE_WALL_Z_DX 43.74
#define INVISIBLE_WALL_Z_SX 4.696
#define INVISIBLE_WALL_X 64.2
#define CENTRE_TEL_Z 23.97
#define DISTANCE_CHECK_PLAYER_CUP 2.0f


struct Rect
{
    float x1, z1, x2, z2, x3, z3, x4, z4;
    // B A
    // C D
};

struct Line
{
    float x1, z1, x2, z2;
};

class Player
{
public:
    Player(float mazeBlockEdgeSize)
    {
        this->position = glm::vec3(0.0f, 0.0f, 0.0f);
        this->rotationAlpha = 0.0f;
        this->rotationBeta = 0.0f;
        this->mazeBlockEdgeSize = mazeBlockEdgeSize;
        this->teleported = false;
        
    }
    void move(float duration, glm::vec3 movement, glm::vec3 rotation, Maze *maze)
    {
        // Compute the new position and rotation and chceck if movement
        // rotationAlpha=glm::radians(-90.0f);
        float newRotationAlpha, newRotationBeta;
        glm::vec3 newPosition;

        newRotationAlpha = rotationAlpha - ROTATE_SPEED * duration * rotation.y;

        ux = glm::rotate(glm::mat4(1.0f), newRotationAlpha, glm::vec3(0, 1, 0)) * glm::vec4(1, 0, 0, 1);
        uz = glm::rotate(glm::mat4(1.0f), newRotationAlpha, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 1, 1);

        newRotationBeta = rotationBeta - ROTATE_SPEED * duration * rotation.x;
        newRotationBeta = newRotationBeta < glm::radians(-90.0f) ? glm::radians(-90.0f) : (newRotationBeta > glm::radians(90.0f) ? glm::radians(90.0f) : newRotationBeta);

        newPosition = position + MOVE_SPEED * movement.x * ux * duration;
        //newPosition = newPosition + MOVE_SPEED * movement.y * glm::vec3(0, 1, 0) * duration; // Uncomment this line to enable vertical movement
        newPosition = newPosition + MOVE_SPEED * movement.z * uz * duration;

        if (checkMazeCollision(position, rotationAlpha, newPosition, newRotationAlpha, maze) && checkBoundary(newPosition) && checkCupCollision(newPosition) )
        {
            rotationAlpha = newRotationAlpha;
            rotationBeta = newRotationBeta;
            position = newPosition;
            
        }

        checkKeyTaken(position, maze);

        teleport(position, maze);
    }

    bool checkCupCollision(glm::vec3 newPosition){

        if (glm::distance(glm::vec2(newPosition.x, newPosition.z), glm::vec2((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 10.0f + (float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f,CENTRE_TEL_Z )) <= DISTANCE_CHECK_PLAYER_CUP){
            return false;
        }


        return true;
    }

    bool checkBoundary(glm::vec3 newPosition){


        if((newPosition.z > INVISIBLE_WALL_Z_DX || newPosition.z < INVISIBLE_WALL_Z_SX || newPosition.x > INVISIBLE_WALL_X) && this->teleported)
            return false;

        return true;
    }

    void teleport(glm::vec3 position,Maze* maze){

        if (glm::distance(glm::vec2(maze->getEndPoint().c * UNITARY_SCALE, maze->getEndPoint().r * UNITARY_SCALE), glm::vec2(position.x, position.z)) <= DISTANCE_CHECK_PLAYER_TELEPORT){

            for (Key key : *maze->getMazeKeys()){
                if(!key.isTaken)
                    return;
            }
            
            setPosition( glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 6.0f + (float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f,  INITIAL_PLAYER_HEIGHT, CENTRE_TEL_Z));
            setRotation(glm::vec2(glm::radians(-90.0f), glm::radians(5.0f)));
            this->teleported = true;

            }
        return;

    }

    void setPosition(glm::vec3 position)
    {
        this->position = position;
    }
    void setRotation(glm::vec2 rotation)
    {
        this->rotationAlpha = rotation.x;
        this->rotationBeta = rotation.y;
    }
    glm::vec3 getPosition()
    {
        return position;
    }
    glm::vec2 getRotation()
    {
        return glm::vec2(rotationAlpha, rotationBeta);
    }

    bool isTeleported(){
        return this->teleported;
    }

private:
    float rotationAlpha, rotationBeta;
    glm::vec3 position;
    glm::vec3 ux, uz;
    float mazeBlockEdgeSize;
    bool teleported;

    // glm::vec2 getClosestMazeCoordinate() {
    //     return glm::vec2(
    //         round(position.x / mazeBlockEdgeSize),
    //         round(position.z / mazeBlockEdgeSize)
    //     );
    // }

    bool checkMazeCollision(glm::vec3 oldPosition, float oldRotationAlpha, glm::vec3 newPosition, float newRotationAlpha, Maze *maze)
    {
        // Don't collide if on another height
        if (newPosition.y < 0.0f || newPosition.y > mazeBlockEdgeSize * maze->get3DHeight())
            return true;
        // Return true if player doesn't collide with maze
        Rect oldPlayerRect = getPlayerRect(oldPosition, oldRotationAlpha);
        Rect newPlayerRect = getPlayerRect(newPosition, newRotationAlpha);
        // Define the lines that describe how each vertex of the player rectangle moves
        Line line1 = {oldPlayerRect.x1, oldPlayerRect.z1, newPlayerRect.x1, newPlayerRect.z1};
        Line line2 = {oldPlayerRect.x2, oldPlayerRect.z2, newPlayerRect.x2, newPlayerRect.z2};
        Line line3 = {oldPlayerRect.x3, oldPlayerRect.z3, newPlayerRect.x3, newPlayerRect.z3};
        Line line4 = {oldPlayerRect.x4, oldPlayerRect.z4, newPlayerRect.x4, newPlayerRect.z4};
        // Check if any of the lines intersects with any maze block
        std::vector<MazePoint> mazeBlocksToCheck = getMazeBlocksToCheck(maze);
        // Each of these blocks has 4 lines I have to check
        for (MazePoint mazeBlock : mazeBlocksToCheck)
        {
            Rect mazeBlockRect = getMazeBlockRect(mazeBlock);
            Line mazeBlockLine1 = {mazeBlockRect.x1, mazeBlockRect.z1, mazeBlockRect.x2, mazeBlockRect.z2};
            Line mazeBlockLine2 = {mazeBlockRect.x2, mazeBlockRect.z2, mazeBlockRect.x3, mazeBlockRect.z3};
            Line mazeBlockLine3 = {mazeBlockRect.x3, mazeBlockRect.z3, mazeBlockRect.x4, mazeBlockRect.z4};
            Line mazeBlockLine4 = {mazeBlockRect.x4, mazeBlockRect.z4, mazeBlockRect.x1, mazeBlockRect.z1};
            if (checkLinesIntersection(line1, mazeBlockLine1) || checkLinesIntersection(line1, mazeBlockLine2) || checkLinesIntersection(line1, mazeBlockLine3) || checkLinesIntersection(line1, mazeBlockLine4) ||
                checkLinesIntersection(line2, mazeBlockLine1) || checkLinesIntersection(line2, mazeBlockLine2) || checkLinesIntersection(line2, mazeBlockLine3) || checkLinesIntersection(line2, mazeBlockLine4) ||
                checkLinesIntersection(line3, mazeBlockLine1) || checkLinesIntersection(line3, mazeBlockLine2) || checkLinesIntersection(line3, mazeBlockLine3) || checkLinesIntersection(line3, mazeBlockLine4) ||
                checkLinesIntersection(line4, mazeBlockLine1) || checkLinesIntersection(line4, mazeBlockLine2) || checkLinesIntersection(line4, mazeBlockLine3) || checkLinesIntersection(line4, mazeBlockLine4))
            {
                return false;
            }
        }
        return true;
    }

    std::vector<MazePoint> getMazeBlocksToCheck(Maze *maze)
    {
        // Return the maze blocks to check for collision around the player
        // Find the maze rows and columns in the player zone
        float minRow = glm::max(glm::floor((position.z - CHUNK_SIZE_MAZE_COLLISION_CHECK) / mazeBlockEdgeSize), 0.0f);
        float maxRow = glm::min(int(glm::ceil((position.z + CHUNK_SIZE_MAZE_COLLISION_CHECK) / mazeBlockEdgeSize)), MAZE_SIZE - 1);
        float minCol = glm::max(glm::floor((position.x - CHUNK_SIZE_MAZE_COLLISION_CHECK) / mazeBlockEdgeSize), 0.0f);
        float maxCol = glm::min(int(glm::ceil((position.x + CHUNK_SIZE_MAZE_COLLISION_CHECK) / mazeBlockEdgeSize)), MAZE_SIZE - 1);
        std::vector<MazePoint> mazeBlocksToCheck;
        for (int i = minRow; i <= maxRow; i++)
            for (int j = minCol; j <= maxCol; j++)
            {
                if (maze->getMazeMap()[i][j] == MazeWay::WALL)
                    mazeBlocksToCheck.push_back({i, j});
            }
        return mazeBlocksToCheck;
    }

    Rect getPlayerRect(glm::vec3 position, float rotationAlpha)
    {
        return Rect{
            // top right, top left, bottom left, bottom right
            position.x + PLAYER_PHYSICS_RIGHT_LENGTH * cos(rotationAlpha) - PLAYER_PHYSICS_FRONT_LENGTH * sin(rotationAlpha),
            position.z - PLAYER_PHYSICS_RIGHT_LENGTH * sin(rotationAlpha) - PLAYER_PHYSICS_FRONT_LENGTH * cos(rotationAlpha),
            position.x - PLAYER_PHYSICS_LEFT_LENGTH * cos(rotationAlpha) - PLAYER_PHYSICS_FRONT_LENGTH * sin(rotationAlpha),
            position.z + PLAYER_PHYSICS_LEFT_LENGTH * sin(rotationAlpha) - PLAYER_PHYSICS_FRONT_LENGTH * cos(rotationAlpha),
            position.x - PLAYER_PHYSICS_LEFT_LENGTH * cos(rotationAlpha) + PLAYER_PHYSICS_BACK_LENGTH * sin(rotationAlpha),
            position.z + PLAYER_PHYSICS_LEFT_LENGTH * sin(rotationAlpha) + PLAYER_PHYSICS_BACK_LENGTH * cos(rotationAlpha),
            position.x + PLAYER_PHYSICS_RIGHT_LENGTH * cos(rotationAlpha) + PLAYER_PHYSICS_BACK_LENGTH * sin(rotationAlpha),
            position.z - PLAYER_PHYSICS_RIGHT_LENGTH * sin(rotationAlpha) + PLAYER_PHYSICS_BACK_LENGTH * cos(rotationAlpha)};
    }

    Rect getMazeBlockRect(MazePoint mazeBlock)
    {
        // Not related to player but was easier
        return Rect{
            mazeBlock.c * mazeBlockEdgeSize - mazeBlockEdgeSize / 2.0f, mazeBlock.r * mazeBlockEdgeSize - mazeBlockEdgeSize / 2.0f,
            mazeBlock.c * mazeBlockEdgeSize + mazeBlockEdgeSize / 2.0f, mazeBlock.r * mazeBlockEdgeSize - mazeBlockEdgeSize / 2.0f,
            mazeBlock.c * mazeBlockEdgeSize + mazeBlockEdgeSize / 2.0f, mazeBlock.r * mazeBlockEdgeSize + mazeBlockEdgeSize / 2.0f,
            mazeBlock.c * mazeBlockEdgeSize - mazeBlockEdgeSize / 2.0f, mazeBlock.r * mazeBlockEdgeSize + mazeBlockEdgeSize / 2.0f};
    }

    bool checkLinesIntersection(Line line1, Line line2)
    {
        // Skip check if line is null
        if ((line1.x1 - line1.x2 == 0 && line1.z1 - line1.z2 == 0) || (line2.x1 - line2.x2 == 0 && line2.z1 - line2.z2 == 0))
            return false;
        // Check if two lines intersect
        float x1 = line1.x1, z1 = line1.z1, x2 = line1.x2, z2 = line1.z2;
        float x3 = line2.x1, z3 = line2.z1, x4 = line2.x2, z4 = line2.z2;
        float denominator = (z4 - z3) * (x2 - x1) - (x4 - x3) * (z2 - z1); // If 0 lines are parallel
        if (denominator == 0)
        {
            return false;
        }
        float ua = ((x4 - x3) * (z1 - z3) - (z4 - z3) * (x1 - x3)) / denominator;
        float ub = ((x2 - x1) * (z1 - z3) - (z2 - z1) * (x1 - x3)) / denominator;
        // Since we have normalized lines, the intersection point is in the range [0, 1]. If not, the lines
        // may intersect but not in the range of the segment
        return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
    }

    bool checkKeyTaken(glm::vec3 position, Maze *maze)
    {
        glm::vec3 keyPos;
        for (Key key : *maze->getMazeKeys())
        {    
            if (!key.isTaken)
            {
                keyPos = glm::vec3(key.point.c * UNITARY_SCALE, 0.4, key.point.r * UNITARY_SCALE);
                if (glm::distance(glm::vec2(keyPos.x, keyPos.z), glm::vec2(position.x, position.z)) <= DISTANCE_CHECK_PLAYER_KEY)
                    maze->setKeyAsTaken(key);
            }
        }
        return true;
    }
};