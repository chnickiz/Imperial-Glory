#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: CommonDataSonido.cpp
// Date: 02/2004
// Ricard
//----------------------------------------------------------------------------------------------------------------
// Estructura que maneja los datos de sonido referentes a un mapa / level
//----------------------------------------------------------------------------------------------------------------

#include "Common/defs.h"
#include "CommonData/CommonDataSonido.h"
#include "Util/scriptBlock.h"
#include "X3DEngine/engine3d.h"
#include "X3DEngine/StateManager.h" 
#include "X3DEngine/Primitives.h"
#include "file/IFile.h"
#include "interfaceBase/interfaceBase.h"
#include "GameServices/GestorAudio.h"
#include "GameServices/AudioMapPlayer.h"

#include "dbg/SafeMem.h"                  // Memoria Segura: Debe ser el ultimo include!

namespace game
{

  //----------------------------------------------------------------------------------------------------------------
  // Constructor
  //----------------------------------------------------------------------------------------------------------------
  CSonidoMapa::CSonidoMapa() {
    m_bInit = false;
    m_sMusica.id = NULL;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Destructor
  //----------------------------------------------------------------------------------------------------------------
  CSonidoMapa::~CSonidoMapa() {
    done();
  }

  //----------------------------------------------------------------------------------------------------------------
  // Seguimos el standar de init / done
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::init() 
  { 
    if(! m_bInit )
    {
      clearIdSonidos();
      m_bInit = true;
    }
    return true; 
  }

  //----------------------------------------------------------------------------------------------------------------
  // No parece necesario liberar memoria, std lo hace por nosotros
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::done() 
  { 
    if( m_bInit )
    {
      clear();
      clearIdSonidos(false);;
      m_bInit = false;
    }
    return true; 
  }

  //----------------------------------------------------------------------------------------------------------------
  // Limpiamos el contenido de la estructura
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::clear() 
  { 
    resetMusica();
    clearSonidosAmbiente();
    clearSonidosPosicional();

    return true; 
  }

  //----------------------------------------------------------------------------------------------------------------
  // Limpiamos el contenido de la estructura
  //----------------------------------------------------------------------------------------------------------------
  void CSonidoMapa::resetMusica()
  {
    assert(m_vSonidosId.size());

    m_sMusica.id = m_vSonidosId[0];
  }

  //----------------------------------------------------------------------------------------------------------------
  // Libera toda la memoria de la lista de sonidos ambiente
  //----------------------------------------------------------------------------------------------------------------
  void CSonidoMapa::clearSonidosAmbiente()
  {
    std::vector<sSonidoAmbiente*>::iterator 	i;
    for ( i = m_vSonidosAmbiente.begin(); i != m_vSonidosAmbiente.end(); ++i )
      delete *i;
    m_vSonidosAmbiente.clear();
  }

  //----------------------------------------------------------------------------------------------------------------
  // Libera toda la memoria de la lista de sonidos posicionales
  //----------------------------------------------------------------------------------------------------------------
  void CSonidoMapa::clearSonidosPosicional()
  {
    std::vector<sSonidoPosicional*>::iterator 	ii;
    for ( ii = m_vSonidosPosicional.begin(); ii != m_vSonidosPosicional.end(); ++ii )
      delete *ii;
    m_vSonidosPosicional.clear();
  }

  //----------------------------------------------------------------------------------------------------------------
  // Crea por nosotros un sonido ambiente nuevo
  //----------------------------------------------------------------------------------------------------------------
  sSonidoAmbiente* CSonidoMapa::createSonidoAmbiente()
  {
    sSonidoAmbiente* sonido = new sSonidoAmbiente;

    if(! sonido)
      return(NULL);

    sonido->id = m_vSonidosId[0];
    sonido->uVolumen = 100;
 
    m_vSonidosAmbiente.push_back(sonido);

    return(sonido);
  }

  //----------------------------------------------------------------------------------------------------------------
  // Borra un sonido ambiente dado un puntero
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::deleteSonidoAmbiente(sSonidoAmbiente* sonido)
  {
    assert(sonido);

    std::vector<sSonidoAmbiente*>::iterator 	i;

    for ( i = m_vSonidosAmbiente.begin(); i != m_vSonidosAmbiente.end(); ++i )
    {
      if(*i == sonido)
      {
        delete *i;
        m_vSonidosAmbiente.erase(i);
        return true;
      }
    }
    return false;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Cuenta los sonidos Ambiente VALIDOS (no los vacios)
  //----------------------------------------------------------------------------------------------------------------
  UINT CSonidoMapa::countSonidosAmbiente() const
  {
    UINT total = 0;

    std::vector<game::sSonidoAmbiente*>::const_iterator 	i;

    for ( i = m_vSonidosAmbiente.begin(); i != m_vSonidosAmbiente.end(); ++i )
    {
      if(isValid(*i))
        total++;
    }
    return total;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Nos dice si un sonido ambiente es valido o no
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::isValid(const sSonidoAmbiente* sonido) const
  {
    assert(sonido);

    if(! sonido || ! sonido->id)
      return false;
    
    if( m_vSonidosId.size() < 1 )
      return false;

    if( sonido->id == m_vSonidosId[0] )
      return false;

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Crea por nosotros un sonido posicional nuevo
  //----------------------------------------------------------------------------------------------------------------
  sSonidoPosicional* CSonidoMapa::createSonidoPosicional(float minRad, float maxRad, CVector3 Pos)
  {
    sSonidoPosicional* sonido = new sSonidoPosicional;

    if(! sonido)
      return(NULL);

    sonido->id = m_vSonidosId[0];
    sonido->uVolumen = 100;
    sonido->v3Pos = Pos;
    sonido->minRadius = minRad;
    sonido->maxRadius = maxRad;

    m_vSonidosPosicional.push_back(sonido);

    return(sonido);
  }

  //----------------------------------------------------------------------------------------------------------------
  // Borra un sonido posicional dado un puntero
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::deleteSonidoPosicional(sSonidoPosicional* sonido)
  {
    assert(sonido);

    std::vector<game::sSonidoPosicional*>::iterator 	i;

    for ( i = m_vSonidosPosicional.begin(); i != m_vSonidosPosicional.end(); ++i )
    {
      if(*i == sonido)
      {
        delete *i;
        m_vSonidosPosicional.erase(i);
        return true;
      }
    }
    return false;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Cuenta los sonidos Posicionales VALIDOS (no los vacios)
  //----------------------------------------------------------------------------------------------------------------
  UINT CSonidoMapa::countSonidosPosicionales() const
  {
    UINT total=0;

    std::vector<sSonidoPosicional*>::const_iterator 	i;

    for ( i = m_vSonidosPosicional.begin(); i != m_vSonidosPosicional.end(); ++i )
    {
      if(isValid(*i))
        total++;
    }
    return total;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Nos dice si un sonido posicional es valido o no
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::isValid(const sSonidoPosicional* sonido) const
  {
    assert(sonido);

    if(! sonido || ! sonido->id)
      return false;

    if( m_vSonidosId.size() < 1 )
      return false;

    if( sonido->id == m_vSonidosId[0] )
      return false;

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Inicia la carga de las descripcionoes de sonido y luego desde un fichero
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::startFromFile(fileSYS::IFile* file, UINT version, bool export)
  {
    init();

    // Cargamos la tabla de sonidos disponible ------------------------------------------
    LOG (("*ModeloBatalla: (INFO) : Cargando tabla de sonidos ...\n"));

    util::scriptBlock   sbMusica, sbEnviroment;

    // TODO: mirar de donde sacar esta info
    bool bRet = sbMusica.init("Game/GameModes/Batalla/AudioScripts/AudioMusica.sb");
    bRet &= sbEnviroment.init("Game/GameModes/Batalla/AudioScripts/AudioEnvironment.sb");

    loadIdSonidos(sbMusica);
    loadIdSonidos(sbEnviroment);
    LOG (("*ModeloBatalla: (INFO) : Tabla de sonidos cargada: %i sonidos \n", countIdSonidos()));

    // Cargamos el mapa de sonidos ------------------------------------------------------
    LOG (("*ModeloBatalla: (INFO) : Cargando mapa de sonidos ...\n"));

    bRet &= load(file, version, export);
    LOG (("*ModeloBatalla: (INFO) : Mapa de sonidos cargado. Musica:'%s' Ambientales:%u Posicionales:%u ...\n", 
      getMusica()->id->IdName.c_str(),
      countSonidosAmbiente(),
      countSonidosPosicionales()));

    return(bRet);
  }
  

  //----------------------------------------------------------------------------------------------------------------
  // Carga datos de sonido de disco
  // IMPORTANTE! antes es importante llenar la lista de ID de sonido
  // Suele ser interesante antes llamar a CSonidoMapa::clear()
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::load(fileSYS::IFile *file, UINT version_format, bool export)
  {
    assert(file);
    assert(m_vSonidosId.size()); // antes es importante llenar la lista de ID de sonido

    UINT        total, i;

    // leemos el IDName del sonido de musica. El fichero esta referenciado en un script .sb
    game::sSonidoId* sonidoId = searchIdSonido(fileSYS::importString (file));
    if( sonidoId )
      m_sMusica.id = sonidoId;
    else
      m_sMusica.id = m_vSonidosId[0];

    // Sonidos Ambiente ----------------------------------
    file->read ( &total );
    for(i=0; i<total; ++i)
    {
      game::sSonidoAmbiente*  sonido = createSonidoAmbiente();
      game::sSonidoId*        sonidoId = searchIdSonido(fileSYS::importString (file));

      if( sonidoId )
        sonido->id = sonidoId;

      file->read ( &sonido->uVolumen );
    }

    // Sonidos Posicionales ------------------------------
    file->read( &total );
    for(i=0; i<total; ++i)
    {
      game::sSonidoPosicional*  sonido = createSonidoPosicional(0.0f, 0.0f, CVector3(0,0,0));
      game::sSonidoId*          sonidoId = searchIdSonido(fileSYS::importString (file));

      if( sonidoId )
        sonido->id = sonidoId;

      file->read ( &sonido->uVolumen );
      file->read ( &sonido->maxRadius );
      file->read ( &sonido->minRadius );
      file->read ( &sonido->v3Pos );
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Carga de un scriptBlock definiciones de ficheros de audio
  // Solo sirve para relacionar un ID con un fichero. Esta blindado para no cargar IDs duplicados
  //----------------------------------------------------------------------------------------------------------------
  bool CSonidoMapa::loadIdSonidos(const util::scriptBlock& script)
  {
    if(! script.hasKey("AUDIO") )
      return false;

    // Recorre los distintos elementos
    UINT                    uNodos = script["AUDIO"].count();
    sSonidoId*              sonido = NULL;

    for ( UINT u=0; u<uNodos; u++ )
    {
      sonido = new sSonidoId;

      sonido->IdName = script["AUDIO"](u)["IDNAME"].asPsz();
      sonido->FileName = script["AUDIO"](u)["PATH"].asPsz();

      if( searchIdSonido(sonido->IdName) )
      {
        // ID duplicado!
        delete sonido;
        sonido = NULL;
      }
      else
      {
        m_vSonidosId.push_back( sonido );
        if ( ! m_sMusica.id ) 
        {
        	m_sMusica.id = sonido;
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Borra de memoria todos los ID de sonidos
  // WARNING: esto puede ser peligroso llamarlo directamente
  //----------------------------------------------------------------------------------------------------------------
  void CSonidoMapa::clearIdSonidos(bool creaSonidoVacio)
  {
    assert(m_vSonidosAmbiente.size()==0);
    assert(m_vSonidosPosicional.size()==0);
    //assert(m_sMusica.id==NULL);

    std::vector<sSonidoId*>::iterator 	i;
    sSonidoId*                          sonido;

    for ( i = m_vSonidosId.begin(); i != m_vSonidosId.end(); ++i )
    {
      sonido = *i;
      if(sonido)
      {
        delete sonido;
        sonido = NULL;
      }
    }

    m_vSonidosId.clear();

    if( creaSonidoVacio )
    {
      sonido = new sSonidoId;
      sonido->IdName = NO_SOUND;
      sonido->FileName = "";

      m_vSonidosId.push_back( sonido );
    }
  }

  //----------------------------------------------------------------------------------------------------------------
  // Devuelve el numero actual de IDs de sonidos cargados
  //----------------------------------------------------------------------------------------------------------------
  UINT CSonidoMapa::countIdSonidos() const
  {
    return m_vSonidosId.size();
  }

  //----------------------------------------------------------------------------------------------------------------
  // Busca un ID de Sonido
  //----------------------------------------------------------------------------------------------------------------
  sSonidoId*  CSonidoMapa::searchIdSonido(const std::string &IdName) const
  {
    std::vector<sSonidoId*>::const_iterator i;
    sSonidoId*                              sonido;

    for ( i = m_vSonidosId.begin(); i != m_vSonidosId.end(); ++i )
    {
      sonido = *i;

      if( sonido->IdName == IdName )
      {
        return sonido;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Busca un ID de Sonido
  //----------------------------------------------------------------------------------------------------------------
  sSonidoId*  CSonidoMapa::searchIdSonido(const UINT id) const
  {
    if(id >= m_vSonidosId.size())
      return NULL;

    return m_vSonidosId[id];
  }

  //----------------------------------------------------------------------------------------------------------------
  // Busca un ID de Sonido
  //----------------------------------------------------------------------------------------------------------------
  int CSonidoMapa::getPosIdSonido(const sSonidoId* sonido) const
  {
    assert(sonido);

    std::vector<sSonidoId*>::const_iterator i;
    int                                     pos = 0;

    for ( i = m_vSonidosId.begin(); i != m_vSonidosId.end(); ++i, ++pos )
    {
      if( sonido == *i )
       return pos;
    }

    return -1;
  }

  //----------------------------------------------------------------------------------------------------------------
  // Para debug y Editor, hace render de los sonidos posicionales
  //----------------------------------------------------------------------------------------------------------------
  void CSonidoMapa::present(bool printLabels) const
  {
    std::vector<sSonidoPosicional*>::const_iterator 	i;

    for ( i = m_vSonidosPosicional.begin(); i != m_vSonidosPosicional.end(); ++i )
    {
      const sSonidoPosicional* sonido = *i;

      // Pintamos las dos esferas
      if ( printLabels ) 
      {
        interfaceBase::printf (0, sonido->v3Pos, CColor(255,255,255), 1.0f, false, "%s", sonido->id->IdName.c_str());	
      }
      
      e3d::primitives::BEGIN_BLOCKSTATE ( e3d::pStateManager->getBlockState (e3d::CStateManager::BS_ZBUFFER) );
      e3d::primitives::sphere (sonido->v3Pos, sonido->minRadius, 20, CColor(240,240,240));
      e3d::primitives::sphere (sonido->v3Pos, sonido->maxRadius, 20, CColor(0,240,0));
      e3d::primitives::END_BLOCKSTATE ();
    }

  }

} // end namespace

