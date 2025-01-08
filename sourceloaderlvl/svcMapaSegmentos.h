#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcMapaSegmentos.h
// Date: 05/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Gestiona el mapa de segmentos del terreno utilizado para colisiones;
// Cada sector esta formado por un conjunto de segmentos cerrado; el primer punto esta almacenado dos veces; 
// una al comienzo y otra al final. Los sectores no tienen porque ser convexos
//----------------------------------------------------------------------------------------------------------------

#ifndef __SVC_MAPA_SEGMENTOS_H__
#define __SVC_MAPA_SEGMENTOS_H__

    #include "math/Vector2.h"
    #include "math/Vector3.h"
    #include <vector>

namespace fileSYS { class IFile;  }
class CSegment2;

namespace game
{

class svcMapaSegmentos
{
public:

  enum { INVALID = 0xffffffff };

  struct SSector;
  
  struct SLink              // un vinculo a un punto de un sector
  {
    SSector*      m_Sector;
    UINT          m_IndexPoint;          // Indice a punto referenciado dentro de ese sector

    bool isValid (void) const
      {  return ( (m_Sector != NULL && m_IndexPoint != INVALID) ? true : false);  }
    SLink (void) {m_Sector = NULL; m_IndexPoint = INVALID; };
  };
  
  struct SSectorPoint       // un punto dentro de un sector
  {
    CVector2      m_Pos;
    SLink         m_LinkedTo;     // el punto puede tener un vinculo a otro punto de otro segmento
  };

  struct SSegmentInfo
  {
    CVector2    m_Normal;       // normal del segmento
    float       m_Longitud;     // longitud del segmento
    SSegmentInfo() { m_Normal = CVector2::Null; m_Longitud = 0.0f; }
  };
      
  struct SSector
  {
    // Si la forma es cerrada el ultimo punto es el primero; pero se almacena igualmente en la lista
    std::vector <SSectorPoint> m_PointList;                   // Lista ordenada de puntos
    std::vector <SSegmentInfo> m_InfoList;                    // Lista ordenada de info de los segmentos (normales y longitud)
    CVector3 m_Center;                                        // Centro geometrico del sector
    float m_BSRadius;                                         // Radio de la bounding sphere
  
    inline UINT getSegments (void) const        { return m_PointList.size () ? (m_PointList.size ()-1) : 0;     }  
    inline UINT getPoints (void) const          { return m_PointList.size ();  }  
  };
  
  // Inicializacion y destruccion
  bool init (void);
  bool load (fileSYS::IFile * file, UINT headerVersion);
  void done (void);

  // Creacion/Edicion de sectores
  svcMapaSegmentos::SSector * createSector (void);
  void deleteSector (SSector * sector);
  
  // Interface para ...
  
  bool clipSegment (SSector * sector, CSegment2 & segment) const;
  bool clipSegment (CSegment2 & segment) const;
  bool clipSegmentVolume (const CSegment2 & segment1, const CSegment2 & segment2, CSegment2 & clipSeg, bool & bEndSegment, float & t) const;
  
  // retorna un path de puntos que nos sirve para llegar de 'posOrig' a 'posDest' (estos incluidos)
  bool findPath ( const CVector2& posOrig, const CVector2& posDest, std::vector<CVector2>* path  );
  
  // Constructors  & Destructors
  svcMapaSegmentos (void);
  ~svcMapaSegmentos (void);

private:

  // Estructura de utilizada al buscar rutas en el mapa de segmentos.
  // Nos quedamos con los datos que nos importan, que son: la pareja de indices a puntos del segmento, 
  // y el afT de la interseccion
  
  struct sSegmentIntersect {
    int   m_IndexA;   // al establecer los indices procuraremos que A sea siempre menor que B
    int   m_IndexB;   //
    float afT;        // respecto al segmento con el que intersectamos (segment)
  };

  // busca una ruta en el sector especificado
  bool findPath ( SSector * sector, CSegment2 & segment, std::vector<CVector2>* path  );

  std::vector <SSector *> m_SectorList;                   // Lista de sectores formados por segmentos
  bool m_bInit;
  
};

} // end namespace game

#endif
