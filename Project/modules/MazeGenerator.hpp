
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <time.h>
#define MAZE_SIZE 17  // MAX 800
#define MAZE_HEIGHT 2 // in blocks (for 3D maze)
#define KEYS_NUMBER 3
#define WALL_LIGHTS_NUMBER 6
#define MIN_PATH_BLOCKS MAZE_SIZE *MAZE_SIZE / 3
#define LIGHT_SQUARE_SIZE 20 // in % wrt the block size
#define TRIVIAL_NODE_PROBABILITY 50
#define MIN_DEPTH_FOR_A_TRIVIAL_NODE 4
#define MIN_TTL 5
#define UNFAIR_PREVIOUS_DIRECTION true          // If true, the first direction chosen is probably to be different from the previous one
#define IGNORE_NULL_TIME_TO_LIVE_PROBABILITY 10 // If the time to live is 0, the node is ignored with this probability (allows to have less dead ends)

#define USE_TTL_JUST_ON_FIRST_DIRECTION_ANALIZED false // If true, the time to live is used only on the first direction analyzed in a node
// False: less dense but more complex

#define WALL_LIGHT_FREQUENCY 4

struct MazePoint
{
  int r;
  int c;
};

enum Direction
{
  NONE = 0,
  DOWN = 1,
  RIGHT = 2,
  LEFT = 4,
  UP = 8
};

enum MazeWay
{
  WALL = 0,
  PATH = 1,
  START = 2,
  END = 3,
  PATH_ANALYZED = 4 // For the longest path finding
};

struct Light
{
  MazePoint point;
  Direction direction;
};

struct Key 
{
    MazePoint point;
    bool isTaken;
};

class Maze
{
public:

  Maze(){
    mazeKeys = new std::vector<Key>();
  }

  std::vector<std::vector<MazeWay>> getMazeMap()
  {
    return _mazeMap;
  }

  void generateMaze()
  {
    // Repeat until we have placed the correct number of lights or a minimum
    // number of path cells is reached
    int generation = 0;
    while (mazeLights.size() != WALL_LIGHTS_NUMBER || mazeFreePlaces.size() < MIN_PATH_BLOCKS)
    {
      std::cout << "Generation " << ++generation << std::endl;
      resetMazeMap();
      resetLights();
      resetKeys();
      // if start is in th middle there will be more branches
      startPoint.r = MAZE_SIZE / 2; // (rand() % (MAZE_SIZE - 2)) + 1;
      startPoint.c = MAZE_SIZE / 2; // (rand() % (MAZE_SIZE - 2)) + 1;
      _mazeMap[startPoint.r][startPoint.c] = MazeWay::START;
      mineMaze(startPoint.r, startPoint.c, NONE, 0, -1);
      _mazeMap[endPoint.r][endPoint.c] = MazeWay::END;
      scanMaze(startPoint.r, startPoint.c, 0, WALL_LIGHT_FREQUENCY + 1);
      resetAnalyzedPath();
      std::cout << "\tPlaced " << mazeLights.size() << " lights" << std::endl;
      fixLightsNumber();
      placeKeys();
    }
    std::cout << "Generation completed in " << generation << " iterations" << std::endl;
    generationComplete = true;
  }

  bool isMazeGenerated()
  {
    return generationComplete;
  }

  std::vector<Light> getMazeLights()
  {
    return mazeLights;
  }

  std::vector<Key>* getMazeKeys()
  {
    return mazeKeys;
  }

  MazePoint getStartPoint()
  {
    return startPoint;
  }

  MazePoint getEndPoint()
  {
    return endPoint;
  }

  int get3DHeight()
  {
    return MAZE_HEIGHT;
  }

  void setKeyAsTaken(Key key)
  {
    bool found = false;
    int i = 0;
    while (!found && i < mazeKeys->size())
    {
      if (mazeKeys->at(i).point.r == key.point.r && mazeKeys->at(i).point.c == key.point.c)
      {
        mazeKeys->at(i).isTaken = true;
        found = true;
      }
      i++;
    }
  }

  int getNumberOfRemainingKeys()
  {
    int notTakenKeys = 0;
    for (Key key : *mazeKeys)
    {
      if (!key.isTaken)
      {
        notTakenKeys++;
      }
    }
    return notTakenKeys;
  }

private:
  int blockSize;
  std::vector<std::vector<MazeWay>> _mazeMap;
  int lunghezzaMax = 0;
  MazePoint endPoint;
  MazePoint startPoint;
  int generationHistory[MAZE_SIZE][MAZE_SIZE]; // bit (LSB) 1 = down, bit 2 = right, bit 3 = left, bit 4 = up

  bool generationComplete = false;

  std::vector<Light> mazeLights;
  std::vector<Key>* mazeKeys;
  std::vector<MazePoint> mazeFreePlaces; // just for construction

  void resetAnalyzedPath()
  {
    // Set back the analyzed path to path after the longest path is found
    for (int a = 0; a < MAZE_SIZE; a++)
      for (int b = 0; b < MAZE_SIZE; b++)
        if (_mazeMap[a][b] == PATH_ANALYZED)
          _mazeMap[a][b] = PATH;
  }

  void resetLights()
  {
    mazeLights.clear();
  }

  void resetKeys()
  {
    mazeKeys->clear();
  }

  void placeKeys()
  {
    // Place the keys in the maze
    int keysPlaced = 0;
    while (keysPlaced < KEYS_NUMBER)
    {
      int randPos = rand() % mazeFreePlaces.size();
      MazePoint keyPoint = mazeFreePlaces.at(randPos);
      if (_mazeMap[keyPoint.r][keyPoint.c] == MazeWay::PATH) // No on wall, start or end
      {
        mazeKeys->push_back({keyPoint, false});
        std::cout << "\tKey placed at " << keyPoint.r << ", " << keyPoint.c << std::endl;
        keysPlaced++;
      }
    }
  }

  void fixLightsNumber()
  {
    // Make the lights number be the maximum allowed
    if (mazeLights.size() > WALL_LIGHTS_NUMBER)
    {
      std::cout << "\tRemoving " << mazeLights.size() - WALL_LIGHTS_NUMBER << " lights to have the correct number" << std::endl;
      while (mazeLights.size() > WALL_LIGHTS_NUMBER)
      {
        int randPos = rand() % mazeLights.size();
        mazeLights.erase(mazeLights.begin() + randPos);
      }
    }
    else if (mazeLights.size() < WALL_LIGHTS_NUMBER)
    {
      std::cout << "\tAdding " << WALL_LIGHTS_NUMBER - mazeLights.size() << " lights to have the correct number" << std::endl;

      // Remove places with lights from the list of free maze places
      std::vector<MazePoint> darkPath;
      for (MazePoint point : mazeFreePlaces)
      {
        if (!hasCellALight(point.r, point.c))
        {
          darkPath.push_back(point);
        }
      }

      while (mazeLights.size() < WALL_LIGHTS_NUMBER && darkPath.size() > 0)
      {
        int randPos = rand() % darkPath.size();
        MazePoint newLightPoint = darkPath.at(randPos);
        darkPath.erase(darkPath.begin() + randPos);
        // Get a wall around this point for the light
        if (newLightPoint.r + 1 < MAZE_SIZE - 1 && _mazeMap[newLightPoint.r + 1][newLightPoint.c] == MazeWay::WALL)
        {
          mazeLights.push_back({{newLightPoint.r, newLightPoint.c}, DOWN});
        }
        else if (newLightPoint.r - 1 >= 0 && _mazeMap[newLightPoint.r - 1][newLightPoint.c] == MazeWay::WALL)
        {
          mazeLights.push_back({{newLightPoint.r, newLightPoint.c}, UP});
        }
        else if (newLightPoint.c + 1 < MAZE_SIZE - 1 && _mazeMap[newLightPoint.r][newLightPoint.c + 1] == MazeWay::WALL)
        {
          mazeLights.push_back({{newLightPoint.r, newLightPoint.c}, RIGHT});
        }
        else if (newLightPoint.c - 1 >= 0 && _mazeMap[newLightPoint.r][newLightPoint.c - 1] == MazeWay::WALL)
        {
          mazeLights.push_back({{newLightPoint.r, newLightPoint.c}, LEFT});
        } // else in this point I don't have a wall (don't place it back in darkPath since we don't need it there)
      }
      if (mazeLights.size() < WALL_LIGHTS_NUMBER)
      {
        std::cout << "\tNot enough walls to place the correct number of lights" << std::endl;
      }
    }
  }

  bool hasCellALight(int r, int c)
  {
    // Returns true if there's a light in that cell
    for (Light light : mazeLights)
    {
      if (light.point.r == r && light.point.c == c)
      {
        return true;
      }
    }
    return false;
  }

  void resetMazeMap()
  {
    _mazeMap.resize(MAZE_SIZE);
    for (int i = 0; i < MAZE_SIZE; i++)
    {
      _mazeMap[i].resize(MAZE_SIZE);
    }
    for (int a = 0; a < MAZE_SIZE; a++)
      for (int b = 0; b < MAZE_SIZE; b++)
      {
        _mazeMap[a][b] = MazeWay::WALL;
        generationHistory[a][b] = 0;
      }
    lunghezzaMax = 0;
    startPoint = {0, 0};
    endPoint = {0, 0};
    mazeLights.clear();
    mazeFreePlaces.clear();
  }

  Direction randomDirection(int history)
  {
    // Return a direction (random) not in the history
    int directionIndex = rand() % 4;
    int tests = 0;
    while (history & (1 << directionIndex) && tests < 4)
    {
      directionIndex = (directionIndex + 1) % 4;
      tests += 1;
    }
    if (tests == 4 && history & (1 << directionIndex))
    {
      return Direction::NONE;
    }
    return (Direction)(1 << directionIndex);
  }

  bool mineMaze(int r, int c, Direction previousDirection, int currentDepth, int timeToLive)
  {
    // std::cout << "\tMining maze at " << r << ", " << c << " with TTL: " << timeToLive << std::endl;
    //  Trivial node: if from here the path can't be longer than a specific length (TTL).
    bool isTrivialNode = rand() % 100 < TRIVIAL_NODE_PROBABILITY && timeToLive < 0 && currentDepth > MIN_DEPTH_FOR_A_TRIVIAL_NODE;
    if (isTrivialNode)
    {
      timeToLive = std::max(MAZE_SIZE - currentDepth, MIN_TTL);
      // std::cout << "\tTrivial node found at " << r << ", " << c << " with TTL: " << timeToLive << std::endl;
    }
    if (timeToLive == 0)
    { // Lower than 0: no limit, above 0: limit for a next node, equal to 0: no more nodes from here
      bool ignoreNullTTL = rand() % 100 < IGNORE_NULL_TIME_TO_LIVE_PROBABILITY;
      if (!ignoreNullTTL)
      {
        return 0;
      }
      // std::cout << "\tIgnoring node at " << r << ", " << c << " with TTL: " << timeToLive << std::endl;
    }

    currentDepth++;

    int rOld = r;
    int cOld = c;
    _mazeMap[r][c] != MazeWay::START ? _mazeMap[r][c] = MazeWay::PATH : _mazeMap[r][c] = MazeWay::START;
    mazeFreePlaces.push_back({r, c});
    int changedDirections = 0;
    bool mazeWasChanged;
    bool isFirstDirectionAnalized = true;
    while (generationHistory[r][c] != 15)
    {
      Direction direction;
      if (generationHistory[r][c] == MazeWay::WALL && UNFAIR_PREVIOUS_DIRECTION)
      {
        // At first, ask for a direction that is not the previous one
        direction = randomDirection(generationHistory[r][c] | previousDirection);
      }
      else
      {
        // After the first direction, ask for a random direction
        direction = randomDirection(generationHistory[r][c]);
      }
      generationHistory[r][c] = generationHistory[r][c] | direction;
      mazeWasChanged = 0;
      switch (direction)
      {
      case DOWN:
        if (_mazeMap[r + 1][c] == MazeWay::WALL && _mazeMap[r + 1][c + 1] == MazeWay::WALL &&
            _mazeMap[r + 1][c - 1] == MazeWay::WALL)
          if (r + 2 < MAZE_SIZE - 1 && _mazeMap[r + 2][c] == MazeWay::WALL && _mazeMap[r + 2][c + 1] == MazeWay::WALL &&
              _mazeMap[r + 2][c - 1] == MazeWay::WALL)
          {
            r = r + 1;
            mazeWasChanged = 1;
          }
        break;
      case RIGHT:
        if (_mazeMap[r][c + 1] == MazeWay::WALL && _mazeMap[r + 1][c + 1] == MazeWay::WALL &&
            _mazeMap[r - 1][c + 1] == MazeWay::WALL)
          if (c + 2 < MAZE_SIZE - 1 && _mazeMap[r][c + 2] == MazeWay::WALL && _mazeMap[r + 1][c + 2] == MazeWay::WALL &&
              _mazeMap[r - 1][c + 2] == MazeWay::WALL)
          {
            c = c + 1;
            mazeWasChanged = 1;
          }
        break;
      case LEFT:
        if (_mazeMap[r][c - 1] == MazeWay::WALL && _mazeMap[r + 1][c - 1] == MazeWay::WALL &&
            _mazeMap[r - 1][c - 1] == MazeWay::WALL)
          if (c - 2 >= 0 && _mazeMap[r][c - 2] == MazeWay::WALL && _mazeMap[r + 1][c - 2] == MazeWay::WALL &&
              _mazeMap[r - 1][c - 2] == MazeWay::WALL)
          {
            c = c - 1;
            mazeWasChanged = 1;
          }
        break;
      case UP:
        if (_mazeMap[r - 1][c] == MazeWay::WALL && _mazeMap[r - 1][c + 1] == MazeWay::WALL &&
            _mazeMap[r - 1][c - 1] == MazeWay::WALL)
          if (r - 2 >= 0 && _mazeMap[r - 2][c] == MazeWay::WALL && _mazeMap[r - 2][c + 1] == MazeWay::WALL &&
              _mazeMap[r - 2][c - 1] == MazeWay::WALL)
          {
            r = r - 1;
            mazeWasChanged = 1;
          }
        break;
      case NONE:
        return 0; // No directions left
      }
      if (mazeWasChanged == 1)
      {
        if (USE_TTL_JUST_ON_FIRST_DIRECTION_ANALIZED)
        {
          if (isFirstDirectionAnalized)
            mineMaze(r, c, direction, currentDepth, timeToLive - 1);
          else
            mineMaze(r, c, direction, currentDepth, -1);
        }
        else
        {
          mineMaze(r, c, direction, currentDepth, timeToLive - 1);
        }
        r = rOld;
        c = cOld;
        changedDirections++;
      }
      isFirstDirectionAnalized = false;
    }
    return 0;
  }

  bool scanMaze(int r, int c, int lunghezza, int stepsSinceLastLight)
  {
    /**
     * Scan the maze:
     * - find the longest path and set the end point
     * - while walking, place the lights with the correct frequency
     */
    Direction scanDirection;
    int changedDirections = 0;
    int scanDirectionHistory = 0;
    bool mazeWasChanged;
    int rOld = r;
    int cOld = c;
    if (_mazeMap[r][c] != MazeWay::WALL)
    {
      lunghezza++;
      if (lunghezza > lunghezzaMax)
      {
        lunghezzaMax = lunghezza;
        endPoint = {r, c};
      }

      // Check if light placement is needed
      // std::cout << "\tR,C: " << r << "-" << c << " has light back of " << stepsSinceLastLight << std::endl;
      if (stepsSinceLastLight > WALL_LIGHT_FREQUENCY)
      {
        // Need to check if there's a wall or not in that direction: if not place, otherwise try
        // to place on another wall.
        // If no walls (4 ways from here), skip the placement
        bool canPlace = false;
        int lightPlacementDirectionHistory = 0;
        Direction lightPlacementDirection;
        lightPlacementDirection = randomDirection(lightPlacementDirectionHistory);
        while (!canPlace && lightPlacementDirection != NONE)
        {
          switch (lightPlacementDirection)
          {
          case DOWN:
            if (r + 1 < MAZE_SIZE - 1 && _mazeMap[r + 1][c] == MazeWay::WALL)
            {
              canPlace = true;
            }
            break;
          case RIGHT:
            if (c + 1 < MAZE_SIZE - 1 && _mazeMap[r][c + 1] == MazeWay::WALL)
            {
              canPlace = true;
            }
            break;
          case LEFT:
            if (c - 1 >= 0 && _mazeMap[r][c - 1] == MazeWay::WALL)
            {
              canPlace = true;
            }
            break;
          case UP:
            if (r - 1 >= 0 && _mazeMap[r - 1][c] == MazeWay::WALL)
            {
              canPlace = true;
            }
            break;
          case NONE:
            break;
          }
          if (canPlace)
          {
            mazeLights.push_back({{r, c}, lightPlacementDirection});
            stepsSinceLastLight = 0;
          }
          lightPlacementDirectionHistory = lightPlacementDirectionHistory | lightPlacementDirection;
          lightPlacementDirection = randomDirection(lightPlacementDirectionHistory);
        }
      }

      while (changedDirections < 4)
      {
        scanDirection = randomDirection(scanDirectionHistory);
        mazeWasChanged = 0;
        if (_mazeMap[r][c] != MazeWay::START)
          _mazeMap[r][c] = PATH_ANALYZED; // Set as already analyzed to avoid infinite loops
        switch (scanDirection)
        {
        case DOWN:
          if (r + 1 < MAZE_SIZE - 1 && _mazeMap[r + 1][c] == 1)
          {
            r++;
            mazeWasChanged = 1;
          }
          break;
        case RIGHT:
          if (c + 1 < MAZE_SIZE - 1 && _mazeMap[r][c + 1] == 1)
          {
            c++;
            mazeWasChanged = 1;
          }
          break;
        case LEFT:
          if (c - 1 >= 0 && _mazeMap[r][c - 1] == 1)
          {
            c--;
            mazeWasChanged = 1;
          }
          break;
        case UP:
          if (r - 1 >= 0 && _mazeMap[r - 1][c] == 1)
          {
            r--;
            mazeWasChanged = 1;
          }
          break;
        case NONE: // No directions left
          return 0;
        }
        if (mazeWasChanged == 1)
        {
          scanMaze(r, c, lunghezza, stepsSinceLastLight + 1);
          r = rOld;
          c = cOld;
        }
        scanDirectionHistory = scanDirectionHistory | scanDirection;
        changedDirections++;
      }
      lunghezza--;
      return 0;
    }
    return 0;
  }
};