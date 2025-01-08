//----------------------------------------------------------------------------------------------------------------
// File: svcEscenario.cpp
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Implementacion del escenario del modelo de batalla
//----------------------------------------------------------------------------------------------------------------

    #include "__PCH_ModeloBatalla.h"
    #include "svcEscenario.h"
    
    #include "CommonData/LVLFormat.h"
    #include "file/IFile.h"
    #include "file/Chunk.h"
		#include "baseApp/baseApp.h"
    #include "GameApp/GameLoc.h"
    #include "Visualization/Visualization.h"
    #include "Visualization/Environment3D.h"
    #include "Visualization/WeatherManager.h"
    
    #include "GameServices/GestorAudio.h"
    #include "GameServices/AudioMusicManager.h"
    #include "GameServices/Camera.h"
    #include "GameServices/AudioMapPlayer.h"

    #include "X3DEngine/Rendering.h"

    #include "util/StringFuncs.h" 
    
    #include "UtilsTropas.h"
    #include "Construccion.h"
    #include "UtilsCriaturas.h"
    #include "svcGestorCriaturas.h"
    #include "Visualization/EffectManager.h"
    #include "Visualization/Visualization.h"
    #include "Visualization/GrxEffect.h"
    #include "GameInterface/BarProgress.h"
   
    #include "dbg/SafeMem.h"

namespace game {

//----------------------------------------------------------------------------------------------------------------
// Constructor & Destructor
//----------------------------------------------------------------------------------------------------------------

svcEscenario::svcEscenario (void)
{
}
svcEscenario::~svcEscenario (void)
{
}

//----------------------------------------------------------------------------------------------------------------
// Inicializa el escenario 
//----------------------------------------------------------------------------------------------------------------
bool svcEscenario::init (const util::HSTR & fileDefEscenarios)
{
  m_hEscenariosDefsFile = fileDefEscenarios;
  m_bInit = true;


  return m_bInit;
}

//----------------------------------------------------------------------------------------------------------------
// Desinicializa el escenario
//----------------------------------------------------------------------------------------------------------------

void svcEscenario::done (void)
{
  if (m_bInit)
  {
    liberaEscenario ();
    m_bInit = false;
  }
}

//----------------------------------------------------------------------------------------------------------------
// Cargamos un escenario a partir de su nombre identificativo; ese nombre se 
// busca dentro del fichero de descripcion de escenarios; y ya nos da los 
// parametros del escenario y en nombre del fichero (.LVL)
//----------------------------------------------------------------------------------------------------------------
bool svcEscenario::cargaEscenario (util::HSTR const & _hNombreEscenario, bool fromSerialization)
{
util::HSTR filename;

  assert (m_bInit);

  LOADING_PROGRESS_UPDATE(0);

  LOG (("*ModeloBatalla: (INFO) : Buscando descripcion de escenario %s\n", _hNombreEscenario.getPsz ()));
  
  // Buscamos el escenario en los scripts
  if (!fromSerialization)
    m_hEscenarioCargado = _hNombreEscenario;
  filename = findNombreEscenario (_hNombreEscenario);
  if (filename.isNull ())
    return false;
    
  // Obtenemos la ruta del nivel
  std::string pathLevel;
  strfuncs::ExtractPath (filename.getStr (), &pathLevel);
  m_hPath = pathLevel;
  LOG (("*ModeloBatalla: (INFO) : Cargando mapa de escenario %s\n", filename.getPsz ()));
  
  // Abrimos el fichero
  fileSYS::IFile * file = fileSYS::open (filename.getStr (), fileSYS::MODE_READ);
  if (!file)
  {    
    LOG (("*Errors: (ERROR) : Mapa de escenario no encontrado: %s\n", filename.getPsz ()));
    assert (false && "Mapa de escenario no encontrado");
    return false;
  }

  // Cargamos la cabecera
  
  fileSYS::CChunkReader chunk (file);
  LVLFMT::eID id = (LVLFMT::eID) 0;
  bool bRet = true;    // retorno
  
  id = (LVLFMT::eID) chunk.begin ();
  LVLFMT::SHeader header;
  if (id == LVLFMT::ID_HEADER)
  {
    file->read (&header);
    LOG (("*ModeloBatalla: Version del mapa %.2f\n", float (header.Version)/100.0f));
    if (header.Version < LVLFMT::VERSION)
      LOG (("*Errors: WARNING: Version de mapa obsoleta ultima version = %.2f\n", float (LVLFMT::VERSION)/100.0f));
  }
  else
    bRet = false;
  
  chunk.end ();

  // Cargamos el resto de los chunks hasta que encontremos el chunk final o encontremos un error

  float fCurrProgress = 5.0f;
  float fProgressUpdate = 5.0f; // por poner algo

  while (id != LVLFMT::ID_END && bRet != false)  
  {
    bool bSeekEnd = true;       // Indica que el end salte al final del chunk
    
    id = (LVLFMT::eID) chunk.begin ();

    if(fCurrProgress<85.0f)
    {
      LOADING_PROGRESS_UPDATE(fCurrProgress);
      fCurrProgress += fProgressUpdate;
    }

    switch (id)
    {
    case LVLFMT::ID_LOGIC:
    {
      bSeekEnd = false;   // No saltar el chunk : Vamos a leer dentro
      break;
    }
    case LVLFMT::ID_LOGICMAP:
    {
      bRet &= m_svcMapaLogico->init (file,  header.Version);
      break;
    }
    case LVLFMT::ID_PHYSICMAP:
    {
      bRet &= m_svcMapaFisico->init (file, pathLevel+"models\\");
      break;
    }
    case LVLFMT::ID_OBJECTLIST:
    {
      bRet &= m_svcObjetosMapa->init (file, pathLevel+"models\\", header.Version);
      bRet &= m_svcObjetosMapa->load (file, header.Version);
      break;
    }
    case LVLFMT::ID_ENVIRONMENT:
    {
      env3d::SDesc gameEnv3D;
      file->read ( &gameEnv3D.bSkyBoxActive );
      gameEnv3D.NameSkyBox = pathLevel+"models\\"+fileSYS::importString (file);
      file->read ( &gameEnv3D.bFogActive );
      file->read ( &gameEnv3D.FogColor );
      file->read ( &gameEnv3D.FogStart );
      file->read ( &gameEnv3D.FogEnd );
      file->read ( &gameEnv3D.FogDensity );
      file->read ( &gameEnv3D.BckgrndColor );
      file->read ( &gameEnv3D.ClipZNear );
      file->read ( &gameEnv3D.ClipZFar );
      file->read ( &gameEnv3D.FOV );
      env3d::changeConfig (gameEnv3D);
      break;
    }
    case LVLFMT::ID_SOUND:
    {
      assert(false == getMapaSonidos()->isInit());
      bRet &= getMapaSonidos()->startFromFile(file, header.Version, true);
      CGestorAudio::ref().GetMapPlayer()->SetMap(getMapaSonidos());
      break;
    }
    case LVLFMT::ID_CONDICIONES:
    {
      // leemos condiciones
      LOG(("*ModeloBatalla: (INFO): ---------------------------------\n"));
      LOG(("*ModeloBatalla: (INFO): ----- CONDICIONES ATACANTES -----\n"));
      m_svcMapaLogico->loadCondiciones( file, m_svcMapaLogico->getCondicionesAta(), false );
      
      LOG(("*ModeloBatalla: (INFO): ----- CONDICIONES DEFENSORES ----\n"));
      m_svcMapaLogico->loadCondiciones( file, m_svcMapaLogico->getCondicionesDef(), true );
      
      m_svcMapaLogico->loadDuracionEscenario(file);
      LOG(("*ModeloBatalla: (INFO): ---------------------------------\n"));
      
      break;
    }
    default:
      break;
    }
    chunk.end (bSeekEnd);
    
  };
  
  // Cerramos el fichero
  if (file)
    fileSYS::close (file);

  LOADING_PROGRESS_UPDATE(85);

  // Carga la descripción del clima para ese escenario en el WeatherManager. Busca el script en
  // el directorio del escenario actual, en el fichero 'clima.sb'
  // Se le pasa el servicio del mapa físico para calculart las alturas de las salpicaduras en el terreno
  CWeatherManager::ref().init(pathLevel, m_svcMapaFisico.ptr(), 0);

  // Se llama a visualización para que realice alguna operación tener cargados todos los GRX
  // Se manda a visualización mapear los objetos de escenario al 'octTree' del terreno
  CVisualization::ref().processGrxPostCreate();
  //CVisualization::ref().mapObjectsInOctree();
  
  // Realiza ciertas operaciones de preparación del escenario
  postCargaEscenario (fromSerialization);
  
  LOADING_PROGRESS_UPDATE(100);

  return bRet;
}


  

//------------------------------------------------------------------------------------------------------------------
// Busca un nombre de escenario y devuelve el fichero a cargar
//------------------------------------------------------------------------------------------------------------------

util::HSTR svcEscenario::findNombreEscenario (const util::HSTR & _hNombreEscenario) const
{
  if (m_svcEstadoJuego->isEscenarioBatallaHistorica())
  {
    // buscar el escenario en el script de batallas historicas:
    util::scriptBlock sbBH;
    if (sbBH.init("Game/GameModes/BatallasHistoricas.sb"))
    {
      for (int b=0; b<(int)sbBH.count(); b++)
      {
        util::scriptBlock sb(sbBH(b));
        if (sb.hasKey("ESCENARIO") && sb.hasKey("FICHERO_ESCENARIO"))
        {
          util::HSTR hEscenario=sb["ESCENARIO"].asHStr();
          if (hEscenario==_hNombreEscenario)
          {
            util::HSTR mapa=sb["FICHERO_ESCENARIO"].asHStr();
            return mapa;
          }
        }
      }
    }
  }
  else
  {
    util::scriptBlock sbDefEscenarios;
    if (sbDefEscenarios.init(m_hEscenariosDefsFile))
    {
      UINT num = sbDefEscenarios.count();
      for(UINT u=0; u < num; u++)
      {
        if(sbDefEscenarios(u)["NAME"].asHStr() == _hNombreEscenario)
        {
          util::HSTR filename = sbDefEscenarios(u)["MAPA"].asHStr();
          sbDefEscenarios.done ();
          
          return (filename);
        }
      }
    }
    sbDefEscenarios.done ();
  }

  LOG (("*Errors: (ERROR) : Descripcion de escenario %s no encontrada\n", _hNombreEscenario.getPsz ()));
  assert ( 0 && "Descripcion de escenario no encontrada");
  
  return util::HSTR::Null;
 }
  

//------------------------------------------------------------------------------------------------------------------
// Función llamada una vez finalizada la carga de todo el mapa :
// Se encarga de asignar los sectores de las posiciones, de ordenar los canales de render, ...
//------------------------------------------------------------------------------------------------------------------
void svcEscenario::postCargaEscenario (bool bFromSerialization)
{
  // Ordenamos todos los canales de render; de tal manera q pintemos el agua el primero de todos
  //e3d::IRendering::ref ().sortChannels (e3d::IRendering::CHANNELS_ALPHA, lessChannel);
  
  // Asignamos sectores a construcciones/modulos
  UINT count = m_svcObjetosMapa->countObjects ();
  for (UINT i = 0; i < count; i++)
  {
    CObjetoBase * pObj = m_svcObjetosMapa->getObject (i);
    if (pObj->isType (TO_CONSTRUCCION) )
    {
      CConstruccion * pConst = static_cast<CConstruccion *>(pObj);
      pConst->getDatosConstruccion ().m_Pos.sec = m_svcMapaLogico->getMapaSectores ()->getSectorOwner (pConst->getDatosConstruccion ().m_Pos.pos);
      
      UINT modulos = pConst->getDatosConstruccion ().m_Modulo.size ();
      for (UINT i = 0; i < modulos; i++)
      {
        TModulo * pMod = pConst->getModulo (i);
        pMod->m_Center.sec = m_svcMapaLogico->getMapaSectores ()->getSectorOwner (pMod->m_Center.pos);
      }
    }
  }

  if (!bFromSerialization)
  {
    // Procesa los LOCs
    processLocations();
  }

  // Asignamos el mapa fisico a la camara (de tal manera q pueda adapterse al terreno!)
  assert (CVisualization::ref ().getCamera ());
  CVisualization::ref ().getCamera ()->setMapaFisico (m_svcMapaFisico.ptr ());
  
  // Carga la descripción de la camara para este escenario teniendo en cuenta si somos atacantes o defensores
  // Si estamos cargando desde serializacion; pasamos de la camara; ya q esta serializada
  if (!bFromSerialization)
  {
    util::scriptBlock sbCamera;
    std::string fileCamera = (m_hPath.getStr ())+"Camara.sb";
    if ( sbCamera.init (util::HSTR (fileCamera)))
    {
      UINT idJugLocal = m_svcEstadoJuego->getJugadorBatallaLocal ();

      if (m_svcEstadoJuego->getInfoEscenarioBatalla().bHistorica)
      {
        if (m_svcEstadoJuego->isJugAtacante(idJugLocal))
        {
          assert(sbCamera.hasKey("HISTORICA_ATACANTE") && "Falta la etiqueta HISTORICA_ATACANTE en el fichero de camara");
          m_svcCamera->loadConfig (sbCamera["HISTORICA_ATACANTE"]);
        }
        else if (m_svcEstadoJuego->isJugDefensor(idJugLocal))
        {
          assert(sbCamera.hasKey("HISTORICA_DEFENSOR") && "Falta la etiqueta HISTORICA_DEFENSOR en el fichero de camara");
          m_svcCamera->loadConfig (sbCamera["HISTORICA_DEFENSOR"]);
        }
      }
      else
      {
        if (m_svcEstadoJuego->isJugAtacante(idJugLocal))
        {
          assert(sbCamera.hasKey("ATACANTE") && "Falta la etiqueta ATACANTE en el fichero de camara");
          m_svcCamera->loadConfig (sbCamera ["ATACANTE"]);
        }
        else if (m_svcEstadoJuego->isJugDefensor(idJugLocal))
        {
          assert(sbCamera.hasKey("DEFENSOR") && "Falta la etiqueta DEFENSOR en el fichero de camara");
          m_svcCamera->loadConfig (sbCamera ["DEFENSOR"]);
        }
      }
      sbCamera.done ();
    }
    else
      LOG (("*Errors: (ERROR) : Fichero de camara de mapa %s no encontrado\n", fileCamera.c_str ()));
  }

}

void svcEscenario::playMapSound(bool bFromSerialization)
{
  if( getMapaSonidos()->isInit() )
  {
    if (!bFromSerialization)
    {
      // Cargamos todos los sonidos encontrados en el fichero .LVL referentes a este escenario
      // Enviamos al gestor de audio la informacion de los sonidos a reproducir
      CGestorAudio::ref().GetMapPlayer()->PlayMap();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------
// Procesa las Locs del mapa para crear los elementos necesarios del mapa
//    - Sistemas de particulas
//    - Criaturas
//    - ....
//----------------------------------------------------------------------------------------------------------------
void svcEscenario::processLocations (void)
{
  const std::vector<SLocation>            lLocations = m_svcMapaLogico->getLocations();
  std::vector<SLocation>::const_iterator  i;

  for ( i = lLocations.begin (); i != lLocations.end (); ++i )
  {
    const SLocation&  loc = *i;

    switch (loc.Type)
    {
      case game::TLOC_CRIATURA:
      {
        if(loc.uId < game::TLOC_CRI_NINGUNA) 
          continue;

        if(! creaCriatura(loc) )
          LOG (("*Errors: (ERROR) : Problemas en la creación inicial de criaturas tipo=%u especie=%u \n", loc.Type, loc.uId));
        break;
      }

      case game::TLOC_PARTICULA:
      {
        // creamos el grx de efectos
        if ( !loc.hParticlesFile.isNull() ) {
          if ( createGrxEffect( loc ) ) {
            LOG (("*ModeloBatalla: (INFO) : FX de escenario cargado: %s\n", loc.hParticlesFile.getPsz ()));
          } else {
            LOG (("*ModeloBatalla: (ERROR) : No se puede cargar FX de escenario: %s\n", loc.hParticlesFile.getPsz ()));
          }
        }
      
        break;
      }

      case game::TLOC_REPLEGARSE:
      {
        break;
      }

      case game::TLOC_REFUERZOS:
      {
        break;
      }
    }
  }

}

//----------------------------------------------------------------------------------------------------------------
// Crea un efecto a partir de un Loc
//----------------------------------------------------------------------------------------------------------------
bool svcEscenario::createGrxEffect(const SLocation &loc)
{
  assert( !loc.hParticlesFile.isNull() );
  
  bool bRet = false;
  
  IGrxEffect* effect = (IGrxEffect*)CVisualization::ref().create(GRX_EFFECT);
  
  if ( effect ) 
  {
    effect->initEmpty();
    bRet = effect->addParticleSys (loc.hParticlesFile, loc.Ciclico, float(loc.FramesFundido), IGrxEffect::RESOURCE_MANAGED);
    
    // Asignamos el tiempo del efecto, teniendo en cuenta q si es ciclico asignamos tiempo infinito, y si no lo
    // es, calculamos la duración real del efecto
    float timeEffect = IGrxEffect::INFINITE_TIME;
    if (!loc.Ciclico)
      timeEffect = IGrxEffect::AUTO_TIME;
    effect->setLifeTime (timeEffect);
    
    // Asignamos la matriz de transformación del efecto desde el LOC
    CMatrix4x4 tm, mRotY, mRotX, mSc;
    tm.doTrans( loc.Pos );
    mRotX.doRotX( loc.AngleX );
    mRotY.doRotY( loc.AngleY );
    mSc.doScale( CVector3(loc.Escala, loc.Escala, loc.Escala) );
    
    tm = mRotX * mRotY * mSc * tm;

    effect->setTransform( tm ); 
    
    m_Effects.push_back( effect );
    
  } 
  
  return bRet; 
}

//----------------------------------------------------------------------------------------------------------------
// Crea una criatura a partir de un Loc
//----------------------------------------------------------------------------------------------------------------
bool svcEscenario::creaCriatura(const SLocation &loc)
{

  if(loc.uId >= game::TLOC_CRI_COUNT)
  {
    assert(0 && "Tipo de criatura NO valida");
    return false; 
  }

  UINT idCriatura = m_svcGestorCriaturas->creaCriatura (game::g_CriaturaLoc[loc.uId]);

  if (idCriatura == 0) {
    assert(0 && "No se puede crear la criatura!");
    return false;
  }


  CCriatura* pCriatura = m_svcGestorCriaturas->IDCriatura2Ptr ( idCriatura );

  if (! pCriatura )
  {
    assert(0 && "Puntero a criatura no valido");
    return false;
  }

  utilsCriaturas::teleport(pCriatura, CVector2 (loc.Pos.x, loc.Pos.z), loc.AngleY);
  pCriatura->setStateSegunEspecie();
  
  return true;
}

//------------------------------------------------------------------------------------------------------------------
// libera el escenario actualmente cargado
//------------------------------------------------------------------------------------------------------------------
void svcEscenario::liberaEscenario (void)
{
  // destruimos los grx de efectos
  for (UINT i = 0; i < m_Effects.size(); ++i) {
    CVisualization::ref().release( m_Effects[i] );
  }
  m_Effects.clear();

  // Se limpia el gestor de clima para el escenario actual
  CWeatherManager::ref().done();

  // Restauramos el environment 3d
	env3d::changeConfig (base_app::g_DescEnv3D);

  // Quitamos la referencia al mapa fisico de la camara
  assert (CVisualization::ref ().getCamera ());
  CVisualization::ref ().getCamera ()->setMapaFisico (NULL);
	
  m_svcObjetosMapa->done();
  m_svcMapaFisico->done();
  m_svcMapaLogico->done();
}

//------------------------------------------------------------------------------------------------
// Obtiene una posicion de inicio de tropa para ATACANTE/DEFENSOR y para un
// tipo de tropa concreto; esta posicion es inmediatamente marcada como utilizada
// Devuelve el identificador de la posicion de tropa encontrada; o INVALID si no la encontro
//------------------------------------------------------------------------------------------------
UINT svcEscenario::findPosInicioTropa (eTipoBando tipoBando, eTipoTropa _ttropa, SGameLoc * pGameLoc_)
{
  if (!pGameLoc_)
    return false;
    
  DWORD allowType = _ttropa;
  DWORD flag = FPT_INICIO;
  
  if (tipoBando == TBANDO_DEFENSOR)
    flag |= FPT_DEFENSOR;
  else if (tipoBando == TBANDO_ATACANTE)
    flag |= FPT_ATACANTE;
  else
    assert (0 && "Bando invalido!");
  
  // Buscamos una posicion de tropa que cumpla las condiciones
  
  CVector2 posFinal (0, 0);
  UINT idPosTrooper = m_svcMapaLogico->findPosTrooper (allowType, flag, &posFinal);
  if (idPosTrooper != svcMapaLogico::INVALID)
  {
    const SPosTrooper * pt = m_svcMapaLogico->getPosTrooper (idPosTrooper);
    assert (pt);
    pGameLoc_->pos = math::conv2Vec3D (posFinal);
    pGameLoc_->ang = pt->Angle;
  }
  else
  {
    // Asignamos posicion por defecto diferente a los atacantes q a los defensores; para q no empiecen pegandose
    LOG (("*Errors: (WARNING) : No se encontro posicion inicial para tropa (asignado default)\n"));
    if (tipoBando == TBANDO_DEFENSOR)
      pGameLoc_->pos   = CVector3 (10000.0f, 0.0f, 10000.0f);
    else if (tipoBando == TBANDO_ATACANTE)
      pGameLoc_->pos   = CVector3 (40000.f, 0, 40000.f);
  }
  return idPosTrooper;
}

//----------------------------------------------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------------------------------------------
void svcEscenario::getPosRefuerzosTropas (eTipoBando tipoBando, eTipoTropa _ttropa, std::vector<SGameLoc> &pos)
{
  pos.clear();
  DWORD allowType = _ttropa;
  DWORD flag = FPT_REFUERZOS;
  if (tipoBando == TBANDO_DEFENSOR)
    flag |= FPT_DEFENSOR;
  else if (tipoBando == TBANDO_ATACANTE)
    flag |= FPT_ATACANTE;
  else
    assert (0 && "Bando invalido!");
 
  // preguntamos al mapa lógico
  m_svcMapaLogico->getPosRefuerzosTropas(allowType, flag, pos);
}



//----------------------------------------------------------------------------------------------------------------
// Carga el audio del escenario
//----------------------------------------------------------------------------------------------------------------
void svcEscenario::loadAudio( const util::scriptBlock& sb )
{
  // reproducimos musica
  /*
  if ( sb.hasKey( "MUSIC" ) ) {
    if ( sb["MUSIC"].hasKey( "IDNAME" ) ) {
      CGestorAudio::ref().PlayMusic( sb["MUSIC"]["IDNAME"].asHStr() );
    } else {
      LOG (("*Errors: (ERROR) : Script de audio de mapa incorrecto\n"));
    }
  } else {
    CGestorAudio::ref().StopMusic();
  }
  
  // reproducimos el resto de los sonidos
  if ( sb.hasKey("SOUNDS") ) {
    
    util::scriptBlock sbSounds( sb["SOUNDS"] );
    
    CGestorAudio::ref().PlayFromScript( sbSounds, m_SonidosID );
    sbSounds.done();
  }
  */
}

//****************************************************************************
// doneAudio: done de audio de batalla/mapa.
//****************************************************************************
void svcEscenario::doneAudio()
{
  // Stop de los sonidos ambientales del mapa
  //CGestorAudio::ref().GetMusicManager()->ReleaseMusic();
  
  // Paramos los sonidos del escenario
  if( getMapaSonidos()->isInit() )
  {
    // Paramos el audio
    CGestorAudio::ref().GetMapPlayer()->StopMap();
  }
  
  CGestorAudio::ref().StopAllSounds();    // stop de los sonidos
}


// =============
// SERIALIZATION
// =============
INIT_SERIALIZATION (svcEscenario, SS_LOGIC)

  // Cargamos el escenario (Lógico/fisico/objetosMapa)
  SERIALIZE_SKIP (m_svcMapaFisico);
  SERIALIZE_SKIP (m_svcMapaLogico);
  
  SERIALIZE (m_svcEstadoJuego);  
  SERIALIZE (m_svcCamera);
  SERIALIZE (m_svcGestorCriaturas);

  SERIALIZE (m_hEscenarioCargado);
  SERIALIZE (m_hEscenariosDefsFile);
  SERIALIZE (m_hPath);
  SERIALIZE_SKIP (m_Effects);
  
  INIT_IF_OK (m_bInit);
  if (LOADING && STATE_OK)
    STATE = cargaEscenario (m_hEscenarioCargado, true);

  // Registramos los servicios necesarios en el namespace 'utilsTropa'
  if (LOADING && STATE_OK)
    utilsTropa::set_MapaLogico (m_svcMapaLogico.ptr ());

  // Esto es un poquito guarro!
  XTRA_PARAMS.set_MAPA_SECTORES (const_cast<CMapaSectores*>(m_svcMapaLogico->getMapaSectores ()));
  XTRA_PARAMS.set_MAPA_LOGICO   (m_svcMapaLogico.ptr ());
  XTRA_PARAMS.set_MAPA_FISICO   (m_svcMapaFisico.ptr ());
  SERIALIZE (m_svcObjetosMapa);
  
    
  //SERIALIZE (m_SonidosID);      // lista de sonidos que son lanzados por defecto al iniciar la batalla (dependientes del mapa).

END_SERIALIZATION (svcEscenario, SS_LOGIC)  

}   // end namespace
