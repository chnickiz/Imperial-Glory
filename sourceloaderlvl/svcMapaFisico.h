#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcMapaFisico.h
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Implementacion del mapa fisico
//----------------------------------------------------------------------------------------------------------------

#ifndef _SVC_MAPA_FISICO_H_
#define _SVC_MAPA_FISICO_H_

    #include "math/Vector2.h"
    #include "X3DEngine/Intersect.h"
    #include "X3DEngine/Terrain.h"
    #include "Util/HSTR.h"
    #include "GameServices/RunNode.h"

namespace e3d { class ITerrain; class IMapHeight;    }

namespace game
{
class ICamera;

class svcMapaFisico
{
public:
  
  bool init (fileSYS::IFile * file, const std::string & pathModels);
  void done (void);
    
  // Intersecciones
  bool intersect (const CRay &ray) const;  
  inline const CIntersect & getIntersect (void) const   { return m_Intersect; }
  inline CIntersect & getIntersect (void)               { return m_Intersect; }
  
  // Puntos de mundo
  bool calcPuntoEnMundo (unsigned int x, unsigned int y, CVector3& dest) const;
  
  // Obtencion de alturas a partir de un punto 2d en coordenadas de mundo
  inline float getY (float x, float z) const
    { return (m_Terrain->getY (x, z));  }
  inline float getY (CVector2 const& pos) const
    { return (m_Terrain->getY (pos.x, pos.y));  }    

  bool hayLineaVision (const CVector3 & src, const CVector3 & dest, CVector3 * maxPos = NULL) const;
  
  void adaptarPolyATerreno( const std::vector<CVector2>& polySrc, std::vector<CVector3>& polyDest, float extraY = 0.0f );
  void adaptarPolyATerreno( const std::vector<CVector3>& polySrc, std::vector<CVector3>& polyDest, float extraY = 0.0f );
    
  inline e3d::ITerrain * getTerrain (void) const        { return m_Terrain;   }
  
  // Constructors  & Destructors
  svcMapaFisico (void);
  ~svcMapaFisico (void);

private:

  //void loadObjects (fileSYS::IFile * file, const std::string & path);
  
private:
  
  e3d::ITerrain *         m_Terrain;                      // Puntero al terreno final (no editable)
  e3d::IMapHeight *       m_HeightMap;                    // Mapa de alturas del terreno
  mutable CIntersect      m_Intersect;                    // Objeto para computo de intersecciones  
  
  CRunNode::IMPORT_SVC<ICamera>  m_svcCamera;
  
};

} // end namespace game

#endif
