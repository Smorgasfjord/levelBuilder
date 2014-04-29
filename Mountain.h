//
//  Mountain.h
//  levelBuilder
//
//  Created by Taylor Woods on 4/16/14.
//  Copyright (c) 2014 Taylor Woods. All rights reserved.
//

#ifndef __levelBuilder__Mountain__
#define __levelBuilder__Mountain__

#include <iostream>
#include "GameObject.h"
#include "GLHandles.h"
#include "glm/glm.hpp"

class Mountain : public GameObject
{
   public:
      void step();
      void draw();
      Mountain();
      Mountain(glm::vec3 pos, GLHandles hand, GameModel model);
      static float testLeftDiagonal(glm::vec3 pos);
      static float testRightDiagonal(glm::vec3 pos);
      static float getZ(glm::vec3 pos);
      static float getX(glm::vec3 pos);
      float rotation;
   private:
      void SetModel(glm::vec3 loc, glm::vec3 size, float rotation);
      GameModel mod;
};

#endif /* defined(__levelBuilder__Mountain__) */
