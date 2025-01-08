#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcMapaLogico.h
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Mapa logico del terreno
//----------------------------------------------------------------------------------------------------------------

#ifndef _SVC_MAPA_LOGICO_H_
#define _SVC_MAPA_LOGICO_H_

    #include "DefinesMapaLogico.h"
    #include "DefsFisica.h"
    #include "GameServices/RunNode.h"
    #include "Util/HSTR.h"
    #include "math/vector2.h"
    #include "math/vector3.h"
    #include "GameApp/GameLoc.h"
    #include "svcMapaSegmentos.h"
    #include "CommonData/CommonDataSonido.h"
    #include "Sector.h"
    #include "Condiciones.h"

namespace fileSYS { class IFile; }


namespace game 
{
struct SGameLoc;
struct SPathNode;
class CMapaSectores;
class CMapaCasillas;
class svcObjetosMapa;
class svcEstadoJuego;
struct svcDatosArranque;

class svcMapaLogico
{
public:

  enum { INVALID = 0xffffffff     };
  enum { DEFAULT_MAX_EXPAND = 8   };

  enum { PLANO_ALTURAS_MIN = 0, PLANO_ALTURAS_MAX = 1 };

  // Constructors & Destructors
  svcMapaLogico();
  ~svcMapaLogico();

  // Interface de inicializacion
  bool init (fileSYS::IFile * file, UINT headerVersion);
  void done (void);

  UINT findPosTrooper (DWORD allowType, DWORD flag, CVector2 * posLoc = NULL) const;
  void getPosRefuerzosTropas (DWORD allowType, DWORD flag, std::vector<SGameLoc> &pos) const;

  const SPosTrooper * getPosTrooper (UINT idPosTrooper) const     { return &m_aPosTrooper [idPosTrooper];   }
  UINT getPosTroopers (void) const                                { return m_aPosTrooper.size ();           }

  const std::vector <SLocation>& getLocations() const                   { return m_aLocation; }
    
  inline bool world2Cell(float _x, float _z, UINT & cellX_, UINT & cellY_) const;
  inline bool world2Cell(CVector2 const& _pos2, UINT & cellX_, UINT & cellY_) const;
  inline bool world2Cell(CVector3 const& _pos3, UINT & cellX_, UINT & cellY_) const;
  inline bool world2Cell(SGameLoc const & _loc, UINT & cellX_, UINT & cellY_) const;
  inline bool cell2World(UINT const& cellX, UINT const& cellY, CVector2& dest) const;
  inline bool getCellCenter(UINT const& cellX, UINT const& cellY, CVector2& dest) const;
  inline bool getCellCenter( const SLogicCell * cell, CVector2 & center ) const;

  inline SLogicCell const * getLogicCell(int _x, int _y) const;
  inline SLogicCell const * getLogicCell(UINT _x, UINT _y) const;
  inline SLogicCell const * getLogicCell(float _x, float _z) const;
  inline SLogicCell const * getLogicCell(CVector3 const& _pos3) const;
  inline SLogicCell const * getLogicCell(CVector2 const& _pos2) const;
  inline SLogicCell const * getLogicCell(SGameLoc const & _loc) const;
  inline void getXYCell (const SLogicCell * cell, UINT &x, UINT &y) const;
  
  inline UINT  getMapWidth () const           {return m_uCellsX;       }
  inline UINT  getMapHeight () const          {return m_uCellsY;       }

  inline float getWidthCell (void) const      {return m_fWidthCell;    }
  inline float getHeightCell (void) const     {return m_fHeightCell;   }
  inline float getInvWidthCell (void) const   {return m_fInvWidthCell; }
  inline float getInvHeightCell (void) const  {return m_fInvHeightCell;}

  inline svcMapaSegmentos * getMapaSegmentos (void) { return m_svcMapaSegmentos.ptr ();  }
  inline CSonidoMapa*       getMapaSonidos   (void) { return m_svcMapaSonidos.ptr ();    }
  
  bool validateCell (UINT & cellX, UINT &cellY, UINT maxExpand = DEFAULT_MAX_EXPAND) const;
  bool validateCell (CVector2& pos, UINT maxExpand = DEFAULT_MAX_EXPAND) const;
  
  // = = = = = =
  // Sectores
  // = = = = = =
  CMapaSectores const* getMapaSectores    () const { return m_mapaSectores; }
  CMapaSectores*       getMapaSectoresMod () { return m_mapaSectores; }
  CMapaCasillas const* getMapaCasillas    () const { return m_mapaCasillas; }
  CMapaCasillas*       getMapaCasillasMod () { return m_mapaCasillas; }
  
  bool            hayLineaMovimiento     (const TPosMundo & src, const TPosMundo & dest, UINT maskPasabl, TPosMundo * const max = NULL, CSector const** destSector = NULL) const;
  CSector const*  calcSectorOwnerFromPos (const TPosMundo & src, const TPosMundo & dest, TPosMundo* max = NULL) const;
  bool            hayLineaVision     (const TPosMundo & src, const TPosMundo & dest, TPosMundo * const max = NULL) const;
  bool            hayLineaVision     (const TPosMundo & src, const TPosMundo & dest, const std::vector <UINT> & ignoreSectorList, TPosMundo * const max = NULL) const;

  // = = = = = =
  // Areas
  // = = = = = =
  UINT areasCount() const { return UINT(m_aArea.size()); }
  const SLogicArea &getArea(UINT uIndex) const { return m_aArea[uIndex]; }
  SLogicArea &getArea(UINT uIndex) { return m_aArea[uIndex]; }
  SLogicArea *findArea(util::HSTR hNombreArea);

  // = = = = = =
  // Otros
  // = = = = = =
  void loadCondiciones( fileSYS::IFile* file, std::vector<sCondicion>& condiciones, bool defensor );
  void loadDuracionEscenario( fileSYS::IFile* file );
  std::vector<sCondicion>& getCondiciones(const eTipoBando bando);
  std::vector<sCondicion>& getCondicionesAta() { return m_CondicionesAta; }
  std::vector<sCondicion>& getCondicionesDef() { return m_CondicionesDef; }
  const std::vector<sCondicion>& getCondicionesAta() const { return m_CondicionesAta; }
  const std::vector<sCondicion>& getCondicionesDef() const { return m_CondicionesDef; }

  float getDuracionEscenarion() const          { return m_DuracionEscenario; }      // en minutos.
  
  float getPlanoAlturas( UINT index ) const    { assert( index < 2 ); return m_PlanoAlturas[index]; }

  UINT getDifficulty() const;

private:

  SPathNode * loadPath (fileSYS::IFile * file, UINT posTrooperId);
  CVector2 getPosTropa (UINT idPos, UINT fila, UINT columna) const;


private:
  UINT                      m_uID;                       // Identificador de mapa logico
  UINT                      m_uCellsX;                   // Celdas horizontales
  UINT                      m_uCellsY;                   // Celdas verticales
  UINT                      m_uTotalCells;               // Numero de celdas totales
  float                     m_fWidthCell;
  float                     m_fHeightCell;
  float                     m_fInvWidthCell;
  float                     m_fInvHeightCell;
  CVector3                  m_v3Origin;
  CVector2                  m_v2Size;
  util::HSTR                m_hName;                     // Nombre del mapa logico
 
  std::vector <SLogicCell>  m_aCellList;                 // Lista de celdas del mapa logico
  std::vector <SPosTrooper> m_aPosTrooper;               // Lista de posiciones de tropa
  std::vector <SLocation>   m_aLocation;                 // Lista de posiciones de control
  std::vector <SLogicZone>  m_aZone;                     // Lista de zonas logicas
  std::vector <SLogicArea>  m_aArea;                     // Lista de areas logicas
  
  CRunNode::EXPORT_SVC <svcMapaSegmentos> m_svcMapaSegmentos;
  CRunNode::EXPORT_SVC <CSonidoMapa>      m_svcMapaSonidos;

  CRunNode::IMPORT_SVC<svcObjetosMapa>          m_svcObjetosMapa;
  CRunNode::IMPORT_SVC<svcEstadoJuego>          m_svcEstadoJuego;
  CRunNode::IMPORT_SVC<svcDatosArranque>        m_svcDatosArranque;
  
  std::vector<sCondicion>                       m_CondicionesAta;     // condiciones de victoria
  std::vector<sCondicion>                       m_CondicionesDef;     // ...
  float                                         m_DuracionEscenario;  // duracion en minutos
  float                                         m_TiempoCuentaAtras;
  
  CMapaSectores* m_mapaSectores;
  CMapaCasillas* m_mapaCasillas;

  float m_PlanoAlturas[2];

  bool                      m_bInitd;
  
};

//****************************************************************************
//
//****************************************************************************
inline bool svcMapaLogico::world2Cell(float _x, float _z, UINT & cellX_, UINT & cellY_) const
{
  bool bRet = false;

  int iCX = (int) ((_x - m_v3Origin.x) * m_fInvWidthCell);
  int iCY = (int) ((_z - m_v3Origin.z) * m_fInvHeightCell);

  if(iCX >= 0 && ((UINT)iCX) < m_uCellsX && iCY >= 0 && ((UINT)iCY) < m_uCellsY)
  {
    cellX_ = (UINT)iCX;
    cellY_ = (UINT)iCY;
    bRet = true;
  }

  return bRet;
}

inline bool svcMapaLogico::cell2World (UINT const& cellX, UINT const& cellY, CVector2& dest) const
{
  bool bRet = false;

  if (cellX < m_uCellsX && cellY < m_uCellsY)
  {
    dest.x = cellX * m_fWidthCell;
    dest.y = cellY * m_fHeightCell;
    bRet = true;
  }
  return bRet;
}

inline bool svcMapaLogico::getCellCenter (UINT const& cellX, UINT const& cellY, CVector2& dest) const
{
  bool bRet = false;

  if (cellX < m_uCellsX && cellY < m_uCellsY)
  {
    dest.x = cellX * m_fWidthCell + m_fWidthCell * 0.5f;
    dest.y = cellY * m_fHeightCell + m_fHeightCell * 0.5f;
    bRet = true;
  }
  return bRet;
}



inline bool svcMapaLogico::world2Cell(CVector3 const& _pos3, UINT & cellX_, UINT & cellY_) const
{
  return world2Cell(_pos3.x, _pos3.z, cellX_, cellY_);
}
inline bool svcMapaLogico::world2Cell(CVector2 const& _pos2, UINT & cellX_, UINT & cellY_) const
{
  return world2Cell(_pos2.x, _pos2.y, cellX_, cellY_);
}

inline bool svcMapaLogico::world2Cell(SGameLoc const & _loc, UINT & cellX_, UINT & cellY_) const
{
  return world2Cell(_loc.pos.x, _loc.pos.z, cellX_, cellY_);
}

//****************************************************************************
// Obtenemos el centro de la celda. El valor retornado de center.y es 0.0f
//****************************************************************************
inline bool svcMapaLogico::getCellCenter( const SLogicCell * cell, CVector2 & center ) const
{ 
  if ( cell == 0 ) {
    return false;
  }

  UINT cellX = 0, cellZ = 0;
  getXYCell( cell, cellX, cellZ );
  
  center.x = m_fWidthCell * cellX + (m_fWidthCell * 0.5f);
  center.y = m_fHeightCell * cellZ + (m_fHeightCell * 0.5f);
  
  return true;
}


//****************************************************************************
//
//****************************************************************************
inline SLogicCell const * svcMapaLogico::getLogicCell(UINT _x, UINT _y) const
{
  SLogicCell const * pRet = 0;

  if(_x < m_uCellsX && _y < m_uCellsY)
  {
    pRet = &(m_aCellList[_y * m_uCellsX + _x]);
  }

  return pRet;
}
inline SLogicCell const * svcMapaLogico::getLogicCell(int _x, int _y) const
{
  SLogicCell const * pRet = 0;

  if(_x >= 0 && _y >= 0 && _x < int (m_uCellsX) && _y < int (m_uCellsY))
  {
    pRet = &(m_aCellList[_y * m_uCellsX + _x]);
  }

  return pRet;
}


inline SLogicCell const * svcMapaLogico::getLogicCell(float _x, float _z) const
{
  SLogicCell const * pRet = 0;
  
  UINT uCX = 0;
  UINT uCY = 0;
  if(world2Cell(_x, _z, uCX, uCY))
  {
    pRet = getLogicCell(uCX, uCY);
  }

  return pRet;
}

inline SLogicCell const * svcMapaLogico::getLogicCell(CVector3 const& _pos3) const
{
  SLogicCell const * pRet = 0;

  UINT uCX = 0;
  UINT uCY = 0;
  if(world2Cell(_pos3.x, _pos3.z, uCX, uCY))
  {
    pRet = getLogicCell(uCX, uCY);
  }

  return pRet;
}

inline SLogicCell const * svcMapaLogico::getLogicCell(CVector2 const& _pos2) const
{
  SLogicCell const * pRet = 0;

  UINT uCX = 0;
  UINT uCY = 0;
  if(world2Cell(_pos2.x, _pos2.y, uCX, uCY))
  {
    pRet = getLogicCell(uCX, uCY);
  }

  return pRet;
}

inline SLogicCell const * svcMapaLogico::getLogicCell(SGameLoc const & _loc) const
{
  SLogicCell const * pRet = 0;

  UINT uCX = 0;
  UINT uCY = 0;
  if(world2Cell(_loc.pos.x, _loc.pos.z, uCX, uCY))
  {
    pRet = getLogicCell(uCX, uCY);
  }

  return pRet;
}

inline void svcMapaLogico::getXYCell (const SLogicCell * cell, UINT &x, UINT &y) const
{
  UINT idx = cell - &m_aCellList[0];
  // Calculamos coordenada de celda segun indice
  y = idx / (m_uCellsX);
  x = idx - (y * (m_uCellsX));
}


} // end namespace game

#endif
