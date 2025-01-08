//----------------------------------------------------------------------------------------------------------------
// File: svcMapaLogico.h
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Mapa logico del terreno
//----------------------------------------------------------------------------------------------------------------

    #include "__PCH_ModeloBatalla.h"

    #include "svcMapaLogico.h"

    #include "CommonData/LVLFormat.h"
    #include "file/IFile.h"
    #include "file/Chunk.h"
    #include "ModeloBatalla/MapaSectores.h"
    #include "ModeloBatalla/MapaCasillas.h"
    #include "RayTracer.h"
    #include "Math/CMath.h"
    #include "UtilsTropas.h"
    #include "UtilsCriaturas.h"
    #include "AI_Utils.h"
    #include "GameServices/GestorAudio.h"
    #include "GameServices/AudioMapPlayer.h"
    #include "svcObjetosMapa.h"
    #include "Construccion.h"
    #include "GameModes/svcEstadoJuego.h"
    #include "GameModes/svcDatosArranque.h"

    #include "dbg/SafeMem.h"                // Memoria Segura: Debe ser el ultimo include!

namespace game {

//------------------------------------------------------------------------------------------------------------------
// Inicializa el mapa logico a partir de un fichero
//------------------------------------------------------------------------------------------------------------------
svcMapaLogico::svcMapaLogico ()
{
  m_bInitd = false;

  m_uID = m_uCellsX = m_uCellsY = m_uTotalCells = 0;
  m_fWidthCell = m_fHeightCell = m_fInvWidthCell = m_fInvHeightCell = 0.f;
  m_v3Origin.clear();
  m_mapaSectores = NULL;
  m_mapaCasillas = NULL;
  m_DuracionEscenario = 0.0f;
  m_TiempoCuentaAtras = 0.0f;
  m_PlanoAlturas[PLANO_ALTURAS_MIN] = 0.0f;
  m_PlanoAlturas[PLANO_ALTURAS_MAX] = 3000.0f;
}

svcMapaLogico::~svcMapaLogico ()
{
  done();
}

bool svcMapaLogico::init (fileSYS::IFile * _file, UINT headerVersion)
{
  bool bRet = false;    // retorno
  
  if(!m_bInitd)
  {
    utilsTropa::set_MapaLogico (this);        // Permite acceso desde utilsTropas
    utilsCriaturas::set_MapaLogico (this);    // Permite acceso desde utilsCriaturas
        
    if ( !m_svcMapaSegmentos->init () )
      return false;
    
    UINT i, count, sizeArray;
        
    assert (_file);
    if(_file)
    {
      bRet = true;
      LOG (("*ModeloBatalla: (INFO) : Cargando informacion logica... \n"));
      
      fileSYS::CChunkReader chunk (_file);
      LVLFMT::eID id;

      do
      {
        id = (LVLFMT::eID) chunk.begin ();

        switch (id)
        {
          case LVLFMT::ID_TERRAINLOGIC:
          {      
            LOG (("*ModeloBatalla: (INFO) : Cargando mapa de celdas... \n"));
            
            m_hName = fileSYS::importString (_file);
            _file->read <UINT> (&m_uID);
            _file->read <UINT> (&m_uCellsX);
            _file->read <UINT> (&m_uCellsY);
            _file->read <UINT> (&m_uTotalCells);
            _file->read <float> (&m_fWidthCell);
            _file->read <float> (&m_fHeightCell);
            _file->read <CVector3> (&m_v3Origin);
            _file->read <CVector2> (&m_v2Size);
            m_fInvWidthCell = 1.0f / m_fWidthCell;
            m_fInvHeightCell = 1.0f / m_fHeightCell;
                        
            // Leemos la lista de celdas
            
            m_aCellList = std::vector <SLogicCell> (m_uTotalCells);
            for (UINT i = 0; i < m_uTotalCells; i++)
            {              
              _file->read <WORD> (&m_aCellList [i].Type);
              _file->read <BYTE> (&m_aCellList [i].Pasabl);
            }
            LOG (("*ModeloBatalla: (INFO) : Mapa de celdas cargado\n"));
            
            if ( headerVersion >= 185 ) {
            
              _file->read <float> (&m_PlanoAlturas[PLANO_ALTURAS_MIN]);
              _file->read <float> (&m_PlanoAlturas[PLANO_ALTURAS_MAX]);
              
              if ( m_PlanoAlturas[PLANO_ALTURAS_MIN] > m_PlanoAlturas[PLANO_ALTURAS_MAX] ) {
                assert( 0 && "m_PlanoAlturas[PLANO_ALTURAS_MIN] > m_PlanoAlturas[PLANO_ALTURAS_MAX]" );
                m_PlanoAlturas[PLANO_ALTURAS_MIN] = m_PlanoAlturas[PLANO_ALTURAS_MAX] - 100.0f;
              }
              
              LOG (("*ModeloBatalla: (INFO) : Plano de Alturas cargado Min:%.2f Max:%.2f\n", m_PlanoAlturas[PLANO_ALTURAS_MIN], m_PlanoAlturas[PLANO_ALTURAS_MAX] ));
            }
            
            break;
          }

          case LVLFMT::ID_POSTROOPER:
          // Leemos lista de posiciones de tropas
          {
            LOG (("*ModeloBatalla: (INFO) : Cargando posiciones de tropas ...\n"));
            
            SPosTrooper obj;
            _file->read <UINT> (&count);
            m_aPosTrooper.reserve (count);
            for (i = 0; i < count; i++)
            {
              _file->read <UINT> (&obj.TypeID);
              _file->read <CVector3> (&obj.Pos);
              _file->read <float> (&obj.Angle);
              _file->read <DWORD> (&obj.AllowType);
              _file->read <DWORD> (&obj.Flags);
              if (headerVersion >= 108)    // // Version 108
              {
                obj.Name = fileSYS::importString (_file);
                _file->read <UINT> (&obj.Filas);
                _file->read <UINT> (&obj.Columnas);
                _file->read <CVector2> (&obj.Size);
                DWORD unUsed = 0;
                _file->read <DWORD> (&unUsed);  // Reservado para uso futuro
                _file->read <DWORD> (&unUsed);  // Reservado para uso futuro
                _file->read <DWORD> (&unUsed);  // Reservado para uso futuro
                _file->read <DWORD> (&unUsed);  // Reservado para uso futuro
              }
              
              obj.RootNode = NULL;
              obj.nCellsUsed = 0;                                     // Posicion sin utilizar
              m_aPosTrooper.push_back (obj);                          // Insertamos el objeto en la lista
            
              // Leemos la ruta de la tropa si es q la tiene; es importante leerlo despues de haberlo insertado;
              // ya que pasamos el puntero a la funcion q carga la ruta!!
              bool bHasRoot;
              _file->read (&bHasRoot);
              if (bHasRoot)
                m_aPosTrooper [m_aPosTrooper.size ()-1].RootNode = loadPath (_file, m_aPosTrooper.size ()-1);
            }
            LOG (("*ModeloBatalla: (INFO) : Posiciones de tropas cargadas\n"));
            
            break;
          }

          case LVLFMT::ID_LOCATIONS:
          // Leemos lista de locations
          {    
            LOG (("*ModeloBatalla: (INFO) : Cargando lista de locations ...\n"));
            
            SLocation obj;
            _file->read <UINT> (&count);
            m_aLocation.reserve (count);
            for (i = 0; i < count; i++)
            {
              _file->read <UINT> (&obj.Type);
              
              if(headerVersion >= 172)
                obj.hName = fileSYS::importString (_file);
              
              if(LVLFMT::VERSION >= 133)
                _file->read <UINT> (&obj.uId);
              else
                obj.uId = game::TLOC_CRI_NINGUNA;

              _file->read <CVector3> (&obj.Pos);
              
              if (headerVersion >= 150) {
                _file->read <float> (&obj.AngleX);
                _file->read <float> (&obj.AngleY);
              } else {
                obj.AngleX = 0.0f;
                _file->read <float> (&obj.AngleY);
              }
              obj.hParticlesFile = fileSYS::importString (_file);
              
              if ( headerVersion >= 150 ) {
                _file->read <bool>  (&obj.Ciclico);
                _file->read <UINT>  (&obj.FramesFundido);
                _file->read <float> (&obj.Escala);
              }
              if ( headerVersion >= 186 )
              {
                float YBias = 0.0f;
                _file->read <float> (&YBias);
                obj.Pos.y += YBias;
              }
              
              m_aLocation.push_back (obj);                           // Insertamos el objeto en la lista
            }
            LOG (("*ModeloBatalla: (INFO) : Lista de locations cargadas\n"));
            break;
          }

          case LVLFMT::ID_ZONES:
          // Leemos lista de zonas
          {    
            LOG (("*ModeloBatalla: (INFO) : Cargando zonas ...\n"));
            
            SLogicZone obj;
            _file->read <UINT> (&count);
            m_aZone.reserve (count);
            for (i = 0; i < count; i++)
            {
              _file->read <DWORD> (&obj.Color);
              obj.hName = fileSYS::importString (_file);
              _file->read <UINT> (&sizeArray);
              if (sizeArray)
              {
                obj.Cell = std::vector <UINT> (sizeArray);
                _file->readArray ( &obj.Cell [0], sizeArray);
              }
              m_aZone.push_back (obj);
            }
            LOG (("*ModeloBatalla: (INFO) : Zonas cargadas\n"));
            
            break;
          }
          case LVLFMT::ID_SECTORMAP:
          {
            m_mapaSectores = new CMapaSectores;
            m_mapaSectores->init (_file, headerVersion);
            break;
          }
          case LVLFMT::ID_GRAFOIA:
          {
            aiUtils::loadGraphInfo (_file);
            break;
          }
          case LVLFMT::ID_ARBITRARYZONES:
          // Leemos lista de zonas arbitrarias
          {
            if ( headerVersion >= 150 ) {
            
              UINT numAreas = 0;
              _file->read<UINT> ( &numAreas );
              
              for (UINT i = 0; i < numAreas; ++i ) {

                SLogicArea newArea;
                
                newArea.hName = fileSYS::importString( _file );
                _file->read<eTipoArea> ( &newArea.Type );
                _file->read<eTipoBando> ( &newArea.Bando );
                
                // leemos los puntos
                UINT numPuntos = 0;
                _file->read<UINT> ( &numPuntos );
                
                for (UINT j = 0; j < numPuntos; ++j) {
                  CVector2 v1(CVector2::Null);
                  _file->read<CVector2>( &v1 );
                  newArea.Area.push_back( v1 );
                }
                
                m_aArea.push_back( newArea );
              }
            }
            
            break;
          }
          case LVLFMT::ID_SEGMENTMAP: 
          // Leemos el mapa de segmentos utilizado para colisiones
          {    
            bRet &= m_svcMapaSegmentos->load (_file, headerVersion);
            break;
          }

          case LVLFMT::ID_SOUND:
          {
            bRet &= m_svcMapaSonidos->startFromFile(_file, headerVersion, true);
            CGestorAudio::ref().GetMapPlayer()->SetMap(getMapaSonidos());
            break;
          }
            
          case LVLFMT::ID_CONDICIONES:
          {
            // leemos condiciones
            LOG(("*ModeloBatalla: (INFO): ---------------------------------\n"));
            LOG(("*ModeloBatalla: (INFO): ----- CONDICIONES ATACANTES -----\n"));
            loadCondiciones( _file, m_CondicionesAta, false );
            
            LOG(("*ModeloBatalla: (INFO): ----- CONDICIONES DEFENSORES ----\n"));
            loadCondiciones( _file, m_CondicionesDef, true );
            
            loadDuracionEscenario(_file);
            
            if (headerVersion >= 171) { 
              _file->read(&m_TiempoCuentaAtras);
              LOG(("*ModeloBatalla: (INFO) : -> Tiempo Cuenta Atras: %f\n", m_TiempoCuentaAtras ));
            } else {
              m_TiempoCuentaAtras = 1.0f;     // 1 minute by default
            }
            
            LOG(("*ModeloBatalla: (INFO): ---------------------------------\n"));
            
            break;
          }

          default:
            break;
        }
        chunk.end ();
        
      } while (id != LVLFMT::ID_ENDLOGICMAP && bRet != false);
    }
    LOG (("*ModeloBatalla: (INFO) : Información lógica cargada \n"));

    m_bInitd = true;
  }
  
  // Si no hemos cargado un mapa desectores creamos uno con un sector que cubre todo
  if (m_bInitd && !m_mapaSectores)
  {
    LOG (("*Errors: (WARNING) : No existe información lógica; creando mapa lógico por defecto \n"));
    
    float width, height;

    width  = getMapWidth () * getWidthCell ();
    height = getMapHeight () * getHeightCell ();

    m_mapaSectores = new CMapaSectores;
    m_mapaSectores->init (width, height);

    LOG (("*ModeloBatalla: (INFO) : Información lógica por defecto creada\n"));
  }

  // Creamos el mapa de casillas

  m_mapaCasillas = new CMapaCasillas;
  assert( m_mapaCasillas );
  m_mapaCasillas->init( this, 75, 75 );
  LOG (("*ModeloBatalla: (INFO) : Mapa de casillas creado\n"));
  
  return bRet;
}

//------------------------------------------------------------------------------------------------------------------
// Libera el mapa logico
//------------------------------------------------------------------------------------------------------------------

void svcMapaLogico::done (void)
{
  if(m_bInitd)
  {
    m_hName.clear();
    m_aCellList.clear();
    m_aLocation.clear();
    m_aZone.clear();
    m_uID = m_uCellsX = m_uCellsY = m_uTotalCells = 0;
    m_fWidthCell = m_fHeightCell = m_fInvWidthCell = m_fInvHeightCell = 0.f;
    m_v3Origin.clear();
    
    
    // Destruimos las rutas de las posiciones de tropas
    UINT count = m_aPosTrooper.size ();
    for (UINT i = 0; i < count; i++)
    {
      if (m_aPosTrooper [i].RootNode)
      {
        std::vector <SPathNode *> nodeList;
        nodeList.clear ();
        m_aPosTrooper [i].RootNode->generateNodeList (nodeList);
        
        UINT nodes = nodeList.size ();
        for (UINT j = 0; j < nodes; j++)
          delete nodeList [j];
      }
    }
    // finalmente limpiamos la lista de posiciones de tropas
    m_aPosTrooper.clear();
        
    m_svcMapaSegmentos->done ();
    DISPOSE (m_mapaSectores);
    DISPOSE (m_mapaCasillas);
    utilsTropa::set_MapaLogico (NULL);        // Elimina acceso desde utilsTropas
    utilsCriaturas::set_MapaLogico (NULL);    // Elimina acceso desde utilsCriaturas
    m_bInitd = false;
    
    aiUtils::freeGraphInfo ();

    getMapaSonidos()->done();
    
    m_CondicionesAta.clear();
    m_CondicionesDef.clear();

  }
}

//------------------------------------------------------------------------------------------------------------------
// Encuentra una posicion de tropa no utilizada que permita el tipo indicado y contenga los flags marcados;
// Ademas marca esa posicion de tropa como utilizada; el retorno puede ser el identificador de la posicion de tropa
// encontrado; o INVALID si no encontro ninguna
//------------------------------------------------------------------------------------------------------------------

UINT svcMapaLogico::findPosTrooper (DWORD allowType, DWORD flag, CVector2 * posLoc) const
{
  UINT count = m_aPosTrooper.size ();
  for (UINT i = 0; i < count; i++)
  {
    if ( m_aPosTrooper [i].nCellsUsed < (m_aPosTrooper [i].Filas*m_aPosTrooper [i].Columnas) )
    {
      if ( ((m_aPosTrooper [i].AllowType & allowType) == allowType) && ((m_aPosTrooper [i].Flags & flag) == flag) )
      {
        int idxCell = m_aPosTrooper [i].nCellsUsed;
        
        // Calculamos la posicion del loc en función de que fila y columna le asignamos
        
        UINT fila = idxCell / m_aPosTrooper [i].Columnas;
        UINT columna = idxCell % m_aPosTrooper [i].Columnas;
        if (posLoc)
          *posLoc = getPosTropa (i, fila, columna);
        
        m_aPosTrooper [i].nCellsUsed ++;   // Marcamos el loc como utilizado por una tropa nueva
        return (i);
      }
    } 
  }
  return INVALID;
}

//------------------------------------------------------------------------------------------------------------------
// Obtiene una lista de posiciones validas de refuerzos con los parámetros indicados
//------------------------------------------------------------------------------------------------------------------

void svcMapaLogico::getPosRefuerzosTropas (DWORD allowType, DWORD flag, std::vector<SGameLoc> &pos) const
{
  for (int i=0; i<(int)m_aPosTrooper.size(); i++)
  {
    const SPosTrooper &posTroop=m_aPosTrooper[i];
    // para este grid de posiciones calcular el centro 
    if ((posTroop.AllowType&allowType)==allowType && (posTroop.Flags&flag)==flag)
    {
      if (posTroop.Filas*posTroop.Columnas>0)
      {
        CVector2 centro(0.f,0.f);
        for (int f=0; f<(int)posTroop.Filas; f++)
          for (int c=0; c<(int)posTroop.Columnas; c++)
            centro+=getPosTropa(i,f,c);
        centro/=float(posTroop.Filas*posTroop.Columnas);
        SGameLoc gl;
        gl.pos=math::conv2Vec3D (centro);
        gl.ang=posTroop.Angle;
        pos.push_back(gl);
      }
    }
  }
}

//------------------------------------------------------------------------------------------------------------------
// Obtiene un posicion2d a partir de un idPosTropa, fila, columna
//------------------------------------------------------------------------------------------------------------------

CVector2 svcMapaLogico::getPosTropa (UINT idPos, UINT fila, UINT columna) const
{
  const SPosTrooper & pt = m_aPosTrooper [idPos];
  
  CVector2 vHor = CVector2 (pt.Size.x, 0).getRotated (pt.Angle);
  CVector2 vVer = CVector2 (0, pt.Size.y).getRotated (pt.Angle);
  
  CVector2 vHorCell = vHor / static_cast <float>(pt.Columnas);
  CVector2 vVerCell = vVer / static_cast <float>(pt.Filas);
  
  CVector2 topLeft = math::conv2Vec2D (pt.Pos) - (vHor*0.5f) + (vVer*0.5f);    // Nos situamos en el top-left de la caja
  
  topLeft += (vHorCell * float (columna));                  // Top-left de la columna correcta
  topLeft -= (vVerCell * float(fila));                      // Top-Left de la fila y columna correcta
  topLeft += (vHorCell * 0.5f);                             // Nos centramos horizontalmente en la celda
  return topLeft;
}

//------------------------------------------------------------------------------------------------------------------
// Carga la ruta de una posicion de tropa y la vincula
//------------------------------------------------------------------------------------------------------------------

SPathNode * svcMapaLogico::loadPath (fileSYS::IFile * file, UINT posTrooperId)
{
  assert (file && posTrooperId != INVALID);
  
  // Leemos todos los nodos
  UINT count;
  file->read (&count);
  assert (count != 0);
  std::vector <SPathNode *> nodeList;
  
  for (UINT i = 0; i < count; i++)
  {
    SPathNode * node = new SPathNode;
    file->read (&node->m_Pos);
    file->read (&node->m_Angle);
    //node->m_IdPosTrooper = posTrooperId;
    //node->m_Index = i;
    nodeList.push_back (node);
  }
  
  // Ahora leemos las relaciones/enlaces entre los nodos (hijos y padres)

  file->read (&count);
  for (UINT i = 0; i < count; i++)
  {
    // ...padres
    UINT parents, index;
    file->read (&parents);
    nodeList [i]->m_Parent.reserve (parents);
    for (UINT j = 0; j < parents; j++)
    {
      file->read (&index);
      nodeList [i]->m_Parent.push_back (nodeList [index]);
    }
    
    // ...hijos
    UINT children;
    file->read (&children);
    nodeList [i]->m_Child.reserve (children);
    for (UINT j = 0; j < children; j++)
    {      
      file->read (&index);
      nodeList [i]->m_Child.push_back (nodeList [index]);
    }
  }
  
  // Creamos la ruta dentro del mapa logico
  m_aPosTrooper [posTrooperId].RootNode = nodeList [0];
  //m_PathNode.push_back (nodeList [0]);
  return nodeList [0];      // El primer nodo siempre es el rootNode (todos cuelgan de el)
}

//------------------------------------------------------------------------------------------------------------------
// Valida una posicion logica : expandiendo un cuadrado en caso de q no sea validas las casillas
//------------------------------------------------------------------------------------------------------------------

enum { VALID_CELL  = 0  };

bool svcMapaLogico::validateCell (UINT & cellX, UINT &cellY, UINT maxExpand ) const
{
  const SLogicCell * pCell = &m_aCellList [cellY * m_uCellsX + cellX];
  if (pCell->Pasabl == VALID_CELL)
    return true;
    
  // La celda no es buena; entonces ahora vamos comprobando adyacentes
  int lenQuad = 3;
  UINT iteraciones = 1;
  int newCellX;
  int newCellY;
  const SLogicCell * thisCell = NULL;
  
  // Expandimos como maximo maxExpand
  while (iteraciones < maxExpand)
  {
    newCellX = int (cellX) - iteraciones;
    newCellY = int (cellY) - iteraciones;
    
    // Lineas horizontales
    for (int i = 0; i < lenQuad; i++)
    {
      if ( (thisCell = getLogicCell (newCellX, newCellY)) != NULL )
      {
        if (thisCell->Pasabl == VALID_CELL)
        {
          cellX = newCellX;
          cellY = newCellY;
          return true;
        }
      }
      
      if ( (thisCell = getLogicCell (newCellX, (newCellY+(lenQuad-1)))) != NULL )
      {
        if (thisCell->Pasabl == VALID_CELL)
        {
          cellX = newCellX;
          cellY = newCellY+(lenQuad-1);
          return true;
        }
      }
      newCellX ++;
    }

    // Usamos (iteraciones-1) pq nos colocamos una linea mas abajo o arriba; ya q esa celda ya fue procesada
    // en el proceso horizontal
    
    newCellX = int (cellX) - iteraciones;
    newCellY = int (cellY) - (iteraciones-1);
    
    // Lineas verticales
    for (int i = 0; i < (lenQuad-2); i++)
    {
      if ( (thisCell = getLogicCell (newCellX, newCellY)) != NULL )
      {
        if (thisCell->Pasabl == VALID_CELL)
        {
          cellX = newCellX;
          cellY = newCellY;
          return true;
        }
      }
      
      if ( (thisCell = getLogicCell ((newCellX+(lenQuad-1)), newCellY)) != NULL )
      {
        if (thisCell->Pasabl == VALID_CELL)
        {
          cellX = newCellX+(lenQuad-1);
          cellY = newCellY;
          return true;
        }
      }
      newCellY ++;
    }
    
    // Abrimos el quadrado un poco mas
    lenQuad += 2;
    iteraciones++;
  };
  
  return false;
}

//------------------------------------------------------------------------------------------------------------------
// Valida una posicion logica : expandiendo un cuadrado en caso de q no sea validas las casillas
//------------------------------------------------------------------------------------------------------------------

bool svcMapaLogico::validateCell( CVector2& pos, UINT maxExpand ) const
{
  UINT cellX = 0, cellZ = 0;
  world2Cell( pos, cellX, cellZ);
  
  UINT oldCellX = cellX, oldCellZ = cellZ;
  if ( !validateCell( cellX, cellZ ) ) {
    return false;
  }

  // aqui tenemos que tener cuidado y no 'clampear' la posicion si la celda no varía
 
  if ( oldCellX != cellX || oldCellZ != cellZ ) {
    getCellCenter( cellX, cellZ, pos );
  }
  
  return true;
}

//------------------------------------------------------------------------------------------------------------------
// Genera una lista de nodos sin repeticiones; ademas a cada nodo le asigna un indice "m_Index"
//------------------------------------------------------------------------------------------------------------------

void SPathNode::generateNodeList (std::vector <SPathNode *> & nodeList)
{
  // Comprobamos si este nodo ya esta insertado en la lista
  UINT count = nodeList.size ();
  for (UINT i = 0; i < count; i++)
    if (nodeList [i] == this)
      break;
  // No encontramos el nodo; o sea q lo introducimos en la lista
  if (i >= count)
  {
    //m_Index = nodeList.size (); 
    nodeList.push_back (this);
  }
  
  // Procesamos los hijos
  count = m_Child.size ();
  for (UINT i = 0; i < count; i++)
    m_Child [i]->generateNodeList (nodeList);
}


game::CSector const* svcMapaLogico::calcSectorOwnerFromPos (const TPosMundo & src, const TPosMundo & dest, TPosMundo* max) const
{
  assert (src.sec);
  assert (src.pos.x >= 0.f && src.pos.y >= 0.f);
  CSector const* retVal = NULL;

  CRayTracer rt;
  if (!rt.init (src.pos, src.sec, dest.pos, dest.sec))
  {
    retVal = src.sec;
    if (max)
      *max = src;

    return retVal;
  }

  // trazamos el rayo hasta que no encontremos más sectores
  while (!rt.finished ());

  // Si no hay linea de movimiento, y nos han pasado puntero a posicion de choque, la calculamos
  bool choca = rt.getChoca ();
  if (!choca)
  {
    retVal = rt.getCurrSector();
  }
  else if (max)
  {
    rt.lastIntersection (max->pos);
    max->sec = rt.getLastSector ();
    if (max->sec)
      max->sec->bringNearCenter (max->pos);
  }

  return retVal;
}

//-------------------------------------------------------------------------------------
// Nombre      : svcMapaLogico::hayLineaMovimiento
// Parametros  : CVector2 const& src
//             : CSector const* srcSec
//             : CVector2 const& dest
//             : CSector const* destSec
//             : CVector2 const* maxPos
//             : CSector const** macSec
// Retorno     : void 
// Descripcion : Comprueba si hay linea de paso entre los dos puntos; (para ello utiliza el mapa
//               de pasabilidad de los sectores
// Notas       : El dest.sec solo se utiliza como optimización; quiere decir q no es
//               necesario rellenarlo!
//-------------------------------------------------------------------------------------

bool svcMapaLogico::hayLineaMovimiento (const TPosMundo & src, const TPosMundo & dest, UINT maskPasabl, TPosMundo * const max, CSector const** destSector) const
{
  assert (src.sec);
  assert (src.pos.x >= 0.f && src.pos.y >= 0.f);

  // Early out
  if (src.sec == dest.sec)
    return true;
  
  CRayTracer rt;
  if (!rt.init (src.pos, src.sec, dest.pos, dest.sec))
  {
    if (destSector) *destSector = src.sec;
    if (max)        *max        = src;
    return false;
  }

  while (!rt.finished ())
  {
    bool bChoca = !rt.getCurrSector()->valid (maskPasabl);
    rt.setChoca (bChoca);
  }

  // Si no hay linea de movimiento, y nos han pasado puntero a posicion de choque, la calculamos
  bool choca = rt.getChoca ();
  if (!choca && destSector)
  {
    *destSector = rt.getCurrSector();
  }
  else if (choca && max)
  {
    rt.lastIntersection (max->pos);
    max->sec = rt.getLastSector ();
    if (max->sec)
      max->sec->bringNearCenter (max->pos);
  }

  return (!choca);
}

//-------------------------------------------------------------------------------------
// Nombre      : svcMapaLogico::hayLineaVision
// Parametros  : CVector2 const& src
//             : CSector const* srcSec
//             : CVector2 const& dest
//             : CSector const* destSec
//             : CVector2 const* maxPos
//             : CSector const** macSec
// Retorno     : void 
// Descripcion : Comprueba si hay linea de vision entre los dos puntos; (para ello utiliza el mapa
//               de oclusiones de los sectores
// Notas       : El dest.sec solo se utiliza como optimización; quiere decir q no es
//               necesario rellenarlo!
//-------------------------------------------------------------------------------------
bool svcMapaLogico::hayLineaVision (const TPosMundo & src, const TPosMundo & dest, TPosMundo * const max) const
{
  assert (src.sec);
  assert (src.pos.x >= 0.f && src.pos.y >= 0.f);
  
  bool borde = false;

  CRayTracer rt;
  rt.init (src.pos, src.sec, dest.pos, dest.sec);
  while (!rt.finished ())
  {
    bool bOcluye = rt.getCurrSector()->getOclusion() > rt.getFirstSector()->getOclusion();
    if ( false == borde && true == bOcluye ) {
      // Hemos encontrado la primera elevacion
      bOcluye = false;
      borde = true;
    } else if ( true == borde && false == bOcluye ) {
      // Caso especial, hay un "escalon"
      bOcluye = true;
    }

    if ( false == bOcluye ) {
      if ( rt.getCurrSector()->getOclusion() < rt.getFirstSector()->getOclusion() ) {
        if ( rt.getCurrSector() == rt.getLastSector() ) {
          bOcluye = true;
        }
      }
    }

    rt.setChoca (bOcluye);
  }

  // Si no hay linea de vision, y nos han pasado puntero a posicion de choque, la calculamos
  if (rt.getChoca () && max)
  {
    rt.lastIntersection (max->pos);
    max->sec = !rt.getChoca ()? rt.getCurrSector () : rt.getLastSector ();
    max->sec->bringNearCenter (max->pos);
  }

  return (!rt.getChoca ());
}

//-------------------------------------------------------------------------------------
// Version de la funcion anterior; ignorando un conjunto de sectores: se utiliza para calcular la visibilidad
// desde el interior de los edificios; ya que como el rayo se traza desde el interior del edificio; debemos
// ignorar la oclusión del propio edificio
//-------------------------------------------------------------------------------------
bool svcMapaLogico::hayLineaVision (const TPosMundo & src, const TPosMundo & dest, const std::vector <UINT> & ignoreSectorList, TPosMundo * const max) const
{
  assert (src.sec);
  assert (src.pos.x >= 0.f && src.pos.y >= 0.f);
  
  // Cogemos los iteradores inicial/final de la lista de sectores a ignorar
  std::vector <UINT>::const_iterator itBegin = ignoreSectorList.begin ();
  std::vector <UINT>::const_iterator itEnd = ignoreSectorList.end ();
  
  CRayTracer rt;
  rt.init (src.pos, src.sec, dest.pos, dest.sec);
  while (!rt.finished ())
  {
    bool bOcluye = rt.getCurrSector()->ocluye ();
    // Si ocluye comprobamos si el sector esta dentro de la lista de sectores a ignorar; caso en el cual
    // ignoramos la oclusión
    if (bOcluye)
    {
      UINT sectorID = rt.getCurrSector ()->getId ();
      for (std::vector <UINT>::const_iterator itSec = itBegin; itSec != itEnd; itSec++)
      {        
        if (sectorID == (*itSec))
        {
          bOcluye = false;  // Sector encontrado; lo ignoramos
          break;
        }
      }
    }
    rt.setChoca (bOcluye);
  }

  // Si no hay linea de vision, y nos han pasado puntero a posicion de choque, la calculamos
  if (rt.getChoca () && max)
  {
    rt.lastIntersection (max->pos);
    max->sec = !rt.getChoca ()? rt.getCurrSector () : rt.getLastSector ();
    max->sec->bringNearCenter (max->pos);
  }

  return (!rt.getChoca ());
}

//-------------------------------------------------------------------------------------
// Version de la funcion anterior; ignorando un conjunto de sectores: se utiliza para calcular la visibilidad
// desde el interior de los edificios; ya que como el rayo se traza desde el interior del edificio; debemos
// ignorar la oclusión del propio edificio
//-------------------------------------------------------------------------------------
SLogicArea *svcMapaLogico::findArea(util::HSTR hNombre)
{
  for (int a=0; a<(int)m_aArea.size(); a++)
    if (m_aArea[a].hName==hNombre)
      return &m_aArea[a];
  return 0;
}

void svcMapaLogico::loadCondiciones( fileSYS::IFile* file, std::vector<sCondicion>& condiciones, bool defensor)
{
  assert( file );
  if ( !file ) {
    return;
  }
  
  UINT numCond = 0;
  file->read(&numCond);

  for (UINT i = 0; i < numCond; ++i) {
    
    sCondicion cond;
    
    UINT numSub = 0;
    file->read(&numSub);
    
    for (UINT j = 0; j < numSub; ++j) {
      
      UINT uTipo = 0;
      file->read(&uTipo);
      
      eTipoSubCondicion tipo = (eTipoSubCondicion)uTipo;
      
      sSubCondicion sub; bool bValid = true;
      sub.setTipo( tipo );
      
      if ( tipo == SUBCOND_ANIQUILACION ) {
        
        file->read(&sub.Params.Aniquilacion.m_Porcentaje);
        LOG(("*ModeloBatalla: (INFO) : -> Aniquilación: %d\n", sub.Params.Aniquilacion.m_Porcentaje ));
        
      } else if ( tipo == SUBCOND_DESTRUCCION_EDIF ) {
        
        file->read(&sub.Params.DestruccionEdif.m_Id);
        
        // comprobamos si el id leido es válido. 
        CConstruccion* pCons = (CConstruccion*)m_svcObjetosMapa->getObject( sub.Params.DestruccionEdif.m_Id );
        if ( !pCons ) {
          bValid = false;
          assert( 0 && "EL ID DE LA CONSTRUCCION DE UNA CONDICION DE VICTORIA NO ES VALIDO, LA CONDICION SERA IGNORADA (HAY QUE VOLVER A ASIGNAR LA CONDICION DE VICTORIA EN EL EDITOR)" );
        }
        
        if ( pCons ) {
          LOG(("*ModeloBatalla: (INFO) : -> Destrucción Edif: %s\n", pCons->getDatosConstruccion().m_Name.getPsz() ));
        } else {
          LOG(("*ModeloBatalla: (INFO) : -> Destrucción Edif: ERROR. pCons == 0\n" ));
        }
        
      } else if ( tipo == SUBCOND_PROTECCION_EDIF ) {
        
        file->read(&sub.Params.ProteccionEdif.m_Id);
        
        // comprobamos si el id leido es válido. 
        CConstruccion* pCons = (CConstruccion*)m_svcObjetosMapa->getObject( sub.Params.ProteccionEdif.m_Id );
        if ( !pCons ) {
          bValid = false;
          assert( 0 && "EL ID DE LA CONSTRUCCION DE UNA CONDICION DE VICTORIA NO ES VALIDO, LA CONDICION SERA IGNORADA (HAY QUE VOLVER A ASIGNAR LA CONDICION DE VICTORIA EN EL EDITOR)" );
        }
        
        if ( pCons ) {
          LOG(("*ModeloBatalla: (INFO) : -> Protección Edif: %s\n", pCons->getDatosConstruccion().m_Name.getPsz() ));
        } else {
          LOG(("*ModeloBatalla: (INFO) : -> Protección Edif: ERROR. pCons == 0\n" ));
        }
        
      } else if ( tipo == SUBCOND_PROTECCION_ZONA ) {
        
        std::string str = fileSYS::importString(file);
        strcpy( sub.Params.ProteccionZona.m_Zona, str.c_str() );
        
        LOG(("*ModeloBatalla: (INFO) : -> Protección Zona: %s\n", sub.Params.ProteccionZona.m_Zona ));
        
      } else if ( tipo == SUBCOND_MANTENER_TROPAS ) {
        
        file->read(&sub.Params.MantenerTropas.m_Porcentaje);
        LOG(("*ModeloBatalla: (INFO) : -> Mantener Tropas: %d\n", sub.Params.MantenerTropas.m_Porcentaje ));
        
      } else if ( tipo == SUBCOND_NONE ) {
        
        //...
        
      } else {
        assert( 0 && "TIPO DESCONOCIDO" );
      }
      
      if ( bValid ) {
        cond.m_SubCondiciones.push_back( sub ); 
      }
    }
    
    condiciones.push_back( cond );
  }

  // Si es modo victoria total, nos cargamos las condiciones de objetivos
  if ( (  m_svcEstadoJuego->getInfoEscenarioBatalla().bEnRed && 
          m_svcDatosArranque->m_bVictoriaTotal ) ||
       (  m_svcDatosArranque->m_SPartidaRapida.bSeleccionLocal && 
          m_svcDatosArranque->m_bVictoriaTotal ) ) {
  
    condiciones.clear(); // borramos las leidas del mapa

    sSubCondicion sub; // creamos condicion de aniquilacion total
    sub.setTipo(SUBCOND_ANIQUILACION);
    sub.Params.Aniquilacion.m_Porcentaje=100;

    sCondicion cond;
    cond.m_SubCondiciones.push_back(sub);
    condiciones.push_back(cond);
    
    if ( defensor ) {
      
      sub.setTipo( SUBCOND_MANTENER_TROPAS );
      sub.Params.MantenerTropas.m_Porcentaje = 0;
      
      cond.m_SubCondiciones.clear();
      cond.m_SubCondiciones.push_back(sub);
      condiciones.push_back(cond);
    }
    
  }
}

void svcMapaLogico::loadDuracionEscenario( fileSYS::IFile* file )
{
  assert( file );
  if ( !file ) {
    return;
  }
  file->read( &m_DuracionEscenario );
  LOG(("*ModeloBatalla: (INFO) : -> Duración Escenario: %f\n", m_DuracionEscenario ));
}

//----------------------------------------------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------------------------------------------
std::vector<sCondicion>& svcMapaLogico::getCondiciones(const eTipoBando bando)
{
  switch(bando) {
    default:
      assert(0 && "Bando incorrecto");    
    case TBANDO_DEFENSOR:
      return(m_CondicionesDef);
    case TBANDO_ATACANTE:
      return(m_CondicionesAta);
  }
}

UINT svcMapaLogico::getDifficulty() const 
{ 
  switch( m_svcDatosArranque->m_SConfigCampana.Dif )
  {
  case game::svcDatosArranque::DIF_FACIL      : return 0;	break;
  case game::svcDatosArranque::DIF_MEDIO      : return 1;	break;
  case game::svcDatosArranque::DIF_DIFICIL    : return 2;	break;
  case game::svcDatosArranque::DIF_MUY_DIFICIL: return 3; break;
  default:
    assert( "La dificultad tiene un valor erroneo!" && 0 );
  }
  
  return 0;
}

}   // end namespace game
