//----------------------------------------------------------------------------------------------------------------
// File: svcMapaFisico.cpp
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Implementacion del mapa fisico
//----------------------------------------------------------------------------------------------------------------

    #include "__PCH_ModeloBatalla.h"

    #include "svcMapaFisico.h"
    #include "GameServices/Camera.h"

    #include "Visualization/World.h"
    #include "X3DEngine/Terrain.h"
    #include "X3DEngine/world.h"
    #include "X3DEngine/engine3d.h"       // TODO: Probando carga de objetos!
    #include "X3DEngine/Import.h"         // TODO: Probando carga de objetos!
    #include "X3DEngine/Layer.h"          // TODO: Probando carga de objetos!
    #include "X3DEngine/ModelNode.h"
    #include "X3DEngine/MapHeight.h"
    
    #include "Util/scriptBlock.h"
    #include "file/IFile.h"
    #include "file/Chunk.h"
    #include "CommonData/LVLFormat.h"
    #include "math/Matrix4x4.h"
    #include "utilsTropas.h"
    
    #include "dbg/SafeMem.h"                // Memoria Segura: Debe ser el ultimo include!

#define MARGEN_MAPA 100.f

namespace game
{

//****************************************************************************
// Constructors & Destructors
//****************************************************************************

svcMapaFisico::svcMapaFisico (void)
{
  m_Terrain = NULL;
  m_HeightMap = NULL;
}

svcMapaFisico::~svcMapaFisico (void)
{
  done ();
}

//------------------------------------------------------------------------------------------------------------------
// Inicializa el mapa fisico a partir de un fichero
// El path de texturas debe acabar en '\' o '//'
//------------------------------------------------------------------------------------------------------------------

bool svcMapaFisico::init (fileSYS::IFile * file, const std::string & pathModels)
{
  assert (file);
  
  LOG (("*ModeloBatalla: (INFO) : Cargando informacion fisica... \n"));
  utilsTropa::set_MapaFisico (this);    // Permite acceso desde utilsTropas
  
  fileSYS::CChunkReader chunk (file);
  LVLFMT::eID id;
  
  bool ret = true;    // retorno

  do
  {
    id = (LVLFMT::eID) chunk.begin ();

    switch (id)
    {
    case LVLFMT::ID_TERRAIN:        // Terreno
    {
      CVector3 delta(MARGEN_MAPA, 0.f, MARGEN_MAPA);
      
      m_Terrain = new e3d::ITerrain;
      m_Terrain->load (file, pathModels);

      CVector3 origen = m_Terrain->getOrigin();

      m_svcCamera->setLimits (CAlignBox ( origen + delta, origen+CVector3 (m_Terrain->getSize ().x, 0, m_Terrain->getSize ().y)-delta));
      m_svcCamera->usarLimites (true);
      
      break;
    }
    case LVLFMT::ID_HEIGHTMAP:      // Mapa de alturas
    {
      m_HeightMap = new e3d::IMapHeight;
      m_HeightMap->load (file);
      m_Terrain->setMapHeight (m_HeightMap);
      break;
    }
    default:
      break;
    }
    chunk.end ();
    
  } while (id != LVLFMT::ID_ENDPHYSIC && ret != false);

  // Ahora asignamos el terreno al mundo
  world::World->setTerrain (ret ? m_Terrain : NULL);
  
  LOG (("*ModeloBatalla: (INFO) : Información física cargada \n"));
  
  return ret;
}

//------------------------------------------------------------------------------------------------------------------
// Libera el mapa fisico
//------------------------------------------------------------------------------------------------------------------

void svcMapaFisico::done (void)
{
  world::World->setTerrain (NULL);
  
  if (m_Terrain)
  {
    m_Terrain->release ();
    m_Terrain = NULL;
  }
  if (m_HeightMap)
  {
    m_HeightMap->release ();
    m_HeightMap = NULL;
  }
  utilsTropa::set_MapaFisico (NULL);    // Deshabilita acceso desde utilsTropas
}

//------------------------------------------------------------------------------------------------------------------
// Comprueba si un rayo intersecta en el terreno
//------------------------------------------------------------------------------------------------------------------
  
bool svcMapaFisico::intersect (const CRay &ray) const
{
  assert (m_Terrain);
  return m_Intersect.intersect (m_Terrain->getOctTree (), m_Terrain->getMesh (), ray);
}




//-------------------------------------------------------------------------------------
// Nombre      : svcMapaFisico::calcPuntoEnMundo
// Parametros  : unsigned int x
//             : unsigned int y
//             : CVector3& dest
// Retorno     : bool 
// Descripcion : 
//-------------------------------------------------------------------------------------
bool svcMapaFisico::calcPuntoEnMundo (unsigned int x, unsigned int y, CVector3& dest) const
{
  bool bCalculado = false;
  CRay ray;
  
  ray = m_svcCamera->getRay (static_cast<float>(x), static_cast<float>(y));
  if (intersect (ray))
  {
    CIntersect const& intersection = getIntersect();
    
    CIntersect::SDesc const& theIntersection = intersection.getIntersect ();
    dest = theIntersection.getPoint ();
    bCalculado = true;
  }

  return bCalculado;
}

//-------------------------------------------------------------------------------------
// Nombre      : svcMapaFisico::hayLineaVision
// Parametros  : CVector2 const& src
//             : CVector2 const& dest
//             : CVector2 const* maxPos
// Retorno     : bool
// Descripcion : Comprueba si hay linea de vision de un punto a otro; teniendo en cuenta
//             : una altura como referencia (se supone la altura del observador)
//-------------------------------------------------------------------------------------
bool svcMapaFisico::hayLineaVision (const CVector3 & src, const CVector3 & dest, CVector3 * maxPos) const
{
  // Convertimos al espacio del mapa de alturas
  
  CVector3 srcMap (src);
  CVector3 destMap (dest);
  
  m_Terrain->worldToCell (srcMap.x, srcMap.z);
  m_Terrain->worldToCell (destMap.x, destMap.z);
  
  // Clipeamos las celdas; teniendo en cuenta que hay q dejar una celda de mas para la interpolación dentro
  // del mapa de alturas
  m_Terrain->clipCellToMapaAlturas (srcMap.x, srcMap.z);
  m_Terrain->clipCellToMapaAlturas (destMap.x, destMap.z);
    
  bool bRet = m_HeightMap->hayLineaVision (srcMap, destMap, maxPos);
  
  // Convertimos la posicion de colision al espacio de mundo
  if (maxPos)
  {
    maxPos->x *= m_Terrain->getWidthCell ();
    maxPos->z *= m_Terrain->getHeightCell ();
  }
   
  return bRet;
}

//-------------------------------------------------------------------------------------
// Nombre      : svcMapaFisico::adaptarPolyATerreno
//
// Funcion que coge un poly y lo segmenta y lo adapta al terreno
// Esta función es de Leo, originalmente estaba en UI_GameBattleMain, pero como la nece-
// sitábamos en más sitios, la meto aqui.
//-------------------------------------------------------------------------------------
void svcMapaFisico::adaptarPolyATerreno( const std::vector<CVector2>& polySrc, std::vector<CVector3>& polyDest, float extraY )
{
  std::vector<CVector2> ptsNew;
  
  // cortar puntos para hacer secciones cortas adaptadas al terreno.
  for (int p=0; p<(int)polySrc.size(); p++)
  {
    const CVector2 &p1(polySrc[p]);
    const CVector2 &p2(polySrc[(p+1)%polySrc.size()]);
    const CVector2 dir((p2-p1).normalize());
    float fLen=(p2-p1).length();
    ptsNew.push_back(p1);
    for (float f=1000.f; f<=fLen-1000.f; f+=1000.f)
      ptsNew.push_back(p1+dir*f);
  }

  polyDest.resize(ptsNew.size());
  for (p=0; p<(int)polyDest.size(); p++) 
  {
    polyDest[p]=math::conv2Vec3D(ptsNew[p], utilsTropa::getMaxY( ptsNew[p] ) + extraY );
  }
}

void svcMapaFisico::adaptarPolyATerreno( const std::vector<CVector3>& polySrc, std::vector<CVector3>& polyDest, float extraY )
{
  std::vector<CVector2> polySrc2D;
  
  // convertimos polySrc a polySrc2D :|
  UINT count = polySrc.size ();
  polySrc2D.reserve (count);
  
  for (UINT i = 0; i < count; i++)
    polySrc2D.push_back (math::conv2Vec2D(polySrc [i]));
    
  adaptarPolyATerreno( polySrc2D, polyDest, extraY );
}


} // end namespace game
