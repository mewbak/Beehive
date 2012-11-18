///////////////////////////////////////////////////
// File:		Primitive.h
// Date:		4th August 2011
// Authors:		Matt Phillips
// Description:	Manually built primitive
///////////////////////////////////////////////////

#pragma once

#include "Vertex.h"
#include "../core/maths/Vector.h"

#include <Ogre/OgreManualObject.h>
#include <string>

namespace ion
{
	namespace renderer
	{
		//Forward declaration
		class Scene;
		class Material;
		class TexCoord;

		class Primitive
		{
		public:
			enum Pattern { Line, Triangle, Quad };
			enum Projection { Proj3D, Proj2D };
			enum QuadAxis { xy, xz, yz };

			Primitive(Scene& scene, Projection projection);
			~Primitive();

			//Begin/end material/vertex/index list
			void Begin(Material* material, Pattern pattern);
			void End();

			//Add components
			void AddVertex(const Vertex& vertex);
			void AddNormal(const Vector3& normal);
			void AddTextureCoord(const TexCoord& coord);
			void AddIndex(unsigned int index);

			//Add built-in primitives (no need to call Begin()/End())
			void AddQuad(Material* material, float width, float height, QuadAxis axis, const Vector3& offset);
			void AddBox(Material* material, const Vector3& halfExtents, const Vector3& offset);
			void AddSphere(Material* material, float radius, int rings, int segments);

			//Shadows
			void SetCastShadows(bool shadows);

			Ogre::ManualObject* GetOgreManualObj();

		private:
			Ogre::ManualObject* mManualPrimitive;
		};
	}
}