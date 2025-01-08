//----------------------------------------------------------------------------------------------------------------
// File: svcModeloCombate.h
// Date: 07/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Almacena todos los datos del modelo de combate de tropas; en si es un contenedor de datos
//----------------------------------------------------------------------------------------------------------------

  #include "__PCH_GameModes.h"
  #include "svcModeloCombate.h"
  #include "ModeloBatalla/TipoDeFormacion.h"
  #include "file/FileSystem.h"
  #include "file/IFile.h"
  #include "ModeloBatalla/TropaBase.h"
  #include "ModeloBatalla/Construccion.h"
  #include "ModeloBatalla/UtilsTropas.h"
  #include "Visualization/AnimationSet.h"
  #include "gameApp/randomLogica.h"
  #include "ModeloBatalla/TropaAI.h"
  #include "ModeloBatalla/TropaState_Melee.h"
  #include "ModeloBatalla/TropaState_Apostar.h"
  #include "ModeloBatalla/TropaState_Cuadro.h"
  #include "ModeloBatalla/svcMapaFisico.h"
  #include "defsEstadoJuegoGestion.h"         // para conocer SBatallon
  #include "baseApp/baseApp.h"
  #include "Util/StatLoggerBattle.h"
  #include "GameApp/GameEnvironment.h"
  #include "ModeloBatalla/UtilsTropas.h"
  #include "ModeloBatalla/svcGestorCombate.h"
  #include "Util/PointInterpolate.h"

#ifdef _INFODEBUG
  #include <sstream>
#endif

  #include "dbg/SafeMem.h"          // Memoria Segura: Debe ser el ultimo include!

namespace game {

  extern UINT g_idPartida;

const int g_MaxPrecision = 100;
const int g_MinModifDan =  -90;   // Minimo modificador de daño (-90) para q no se pueda anular todo el daño
const int g_MaxModifDan = 1000;
const int g_MinModifRes  = -90;
const int g_MaxModifRes  =  90;   // Maximo modificador a resistencia es 90; ya q un 100% quiere decir que absorve todo el impacto

const float g_ModVel      = 0.25f;    // Modificador unitario de velocidad (0 - 1.0)
const float g_ModRngDet   = 2000.0f;    // Modificador del rango de deteccion en coordenadas absolutas de mundo (cm)

//----------------------------------------------------------------------------------------------------------------
// Constructors & Destructors 
//----------------------------------------------------------------------------------------------------------------

svcModeloCombate::svcModeloCombate (void)
{
#ifdef _INFODEBUG
  m_pInfoDebug=0;
#endif

  m_MaxValArmadura = 0;
  m_MaxValAtaqueADist  = 0;
  m_MaxValAtaqueADistArt = 0;
  m_MaxValAtaqueCAC  = 0;
  m_bInvulnerability = false;
  
}
svcModeloCombate::~svcModeloCombate (void)
{
}

//----------------------------------------------------------------------------------------------------------------
// Inicializa el modelo de combate : cargando los datos necesarios
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::init (const std::string & filename)
{
  assert (!filename.empty ());
  
  m_FileName = filename;
  
  // Abrimos el fichero de datos del modelo de combate
  
  LOG (("Inicializando modelo de combate a partir del fichero %s\n", filename.c_str ()));
  fileSYS::IFile * pFile = fileSYS::open (filename, fileSYS::MODE_READ | fileSYS::MODE_SEQUENTIALACCESS);
  if (!pFile)
  {
    LOG (("*Errors: (ERROR) Abriendo fichero %s\n", filename.c_str ()));
    return false;
  }
  
  UINT version = pFile->read <UINT> ();
  
  UINT numRangos = RANGO_COUNT;
  if ( version < 120 ) {
    numRangos = 1;
  }
  
  for (UINT iRango = 0; iRango < numRangos; ++ iRango) {
  
    // Cargamos los datos de las tropas
  
    loadDatosTropa (pFile, iRango);         // Infanteria
    loadDatosTropa (pFile, iRango);         // Caballeria
    loadDatosTropa (pFile, iRango);         // Artilleria
    
    loadDatosDanTropa (pFile, iRango);      // Infanteria
    loadDatosDanTropa (pFile, iRango);      // Caballeria

    loadModifFormacion (pFile, iRango);     // Infanteria
    loadModifFormacion (pFile, iRango);     // Caballeria
    
    if ( version >= 110 ) {
      loadTablaCargasVsCuadro(pFile, iRango);
      loadTablaCargasVsCuadro(pFile, iRango);
      loadTablaCargasPropiedades(pFile, iRango);
    }
    
    if ( version >= 120 ) {
      loadInfoDanArtilleria(pFile, version, iRango);
    }
  }
  
  // Cargamos modificador de terreno
  // NOTES: De momento no se carga de fichero; sino q se inicializa directamente
  loadModifTerreno ();
  
  // Terminamos todo; cerramos el fichero
  fileSYS::close (pFile);
  
  m_MaxValArmadura = findMaxArmadura();
  m_MaxValAtaqueADist = findMaxAtaqueADist();
  m_MaxValAtaqueADistArt = findMaxAtaqueADistArt();
  m_MaxValAtaqueCAC = findMaxAtaqueCAC();
  
  m_bInvulnerability = false;

  m_FuncPercentMelee.Clear();
  m_FuncPercentMelee.AddPoint(1.0f,  1.0f);
  m_FuncPercentMelee.AddPoint(0.8f,  2.0f);
  m_FuncPercentMelee.AddPoint(0.7f,  3.0f);
  m_FuncPercentMelee.AddPoint(0.6f,  4.0f);
  m_FuncPercentMelee.AddPoint(0.52f, 5.0f);
  m_FuncPercentMelee.AddPoint(0.45f, 6.0f);
  m_FuncPercentMelee.AddPoint(0.3f,  10.0f);
  m_FuncPercentMelee.AddPoint(0.15f, 30.0f);

  m_FuncDamageLostMelee.Clear();
  m_FuncDamageLostMelee.AddPoint(0.1f,  0.1f);
  m_FuncDamageLostMelee.AddPoint(0.25f, 0.5f);
  m_FuncDamageLostMelee.AddPoint(0.4f,  1.0f);
  m_FuncDamageLostMelee.AddPoint(0.6f,  2.0f);
  m_FuncDamageLostMelee.AddPoint(0.7f,  3.0f);
  m_FuncDamageLostMelee.AddPoint(0.8f,  4.0f);
  m_FuncDamageLostMelee.AddPoint(0.9f,  5.0f);
  m_FuncDamageLostMelee.AddPoint(1.0f,  8.0f);

  LOG (("Modelo de combate inicializado\n", filename.c_str ()));
  
  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Recarga todos los datos
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::reload (void)
{
  std::string filename = m_FileName;
  
  m_bInvulnerability = false;
  
  done ();
  return ( init (filename) );
}

//----------------------------------------------------------------------------------------------------------------
// Destruye el modelo de combate
//----------------------------------------------------------------------------------------------------------------

void svcModeloCombate::done (void)
{
  for (UINT i = 0; i < RANGO_COUNT; ++i) {
    m_DescDanTropaInf[i].clear ();
    m_DescDanTropaCab[i].clear ();
  }
}

//----------------------------------------------------------------------------------------------------------------
// Carga los datos de una clase de tropa desde disco
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadDatosTropa (fileSYS::IFile * file, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT claseTropa       = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT structSize       = file->read <UINT> ();
  
  bool bCorrupt = false;
  
  // Cogemos el puntero donde guardar los datos en funcion del tipo de tabla
  TDescTropa * pDescTropa = NULL;
  if (claseTropa == TTT_INFANTERIA)
    pDescTropa = m_DescTropaInf[rango];
  else if (claseTropa == TTT_CABALLERIA)
    pDescTropa = m_DescTropaCab[rango];
  else if (claseTropa == TTT_ARTILLERIA)
    pDescTropa = m_DescTropaArt[rango];
  else
    bCorrupt = true;
  
  if (bCorrupt || structSize != sizeof (TDescTropa))
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Leemos los datos para cada tipo de tropa
  for (UINT f = 0; f < filas; f++)
  {
    file->read (&(pDescTropa [f]));
    
    pDescTropa [f].m_fVelNor =          velsec2veltick (pDescTropa [f].m_fVelNor);
    pDescTropa [f].m_fVelForColumna =   velsec2veltick (pDescTropa [f].m_fVelForColumna);
    pDescTropa [f].m_fVelCor =          velsec2veltick (pDescTropa [f].m_fVelCor);
    pDescTropa [f].m_fVelCar =          velsec2veltick (pDescTropa [f].m_fVelCar);
    
    pDescTropa [f].m_fVelForLinea =     velsec2veltick (pDescTropa [f].m_fVelForLinea);
    pDescTropa [f].m_fVelForCuadro =    velsec2veltick (pDescTropa [f].m_fVelForCuadro);
    
    /* TODO: UINT m_uTieRec;                // Tiempo de recarga  */
  }
  
  return true;
}
//----------------------------------------------------------------------------------------------------------------
// Carga las tablas de daños de una clase de tropa desde disco
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadDatosDanTropa (fileSYS::IFile * file, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT claseTropa       = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT sizeStruct       = file->read <UINT> ();

  // Cogemos el puntero donde guardar los datos en funcion del tipo de tabla
  std::vector <TDescDanTropa> * pDescDanTropa = NULL;
  if (claseTropa == TTT_DAN_INFANTERIA)
    pDescDanTropa = &m_DescDanTropaInf[rango];
  else if (claseTropa == TTT_DAN_CABALLERIA)
    pDescDanTropa = &m_DescDanTropaCab[rango];
  else
  {    
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  if (sizeStruct != sizeof (TDescDanTropa))
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Cargamos los datos
  (*pDescDanTropa) = std::vector <TDescDanTropa> (filas);
  for (UINT f = 0; f < filas; f++)
    file->read (&((*pDescDanTropa) [f]));   // Ya q es un puntero a un vector; usamos (*ptr)[f] para referenciar el elemento f

  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Carga las tablas de daños de una clase de tropa desde disco
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadInfoDanArtilleria (fileSYS::IFile * file, UINT version, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT claseTropa       = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT sizeStruct       = file->read <UINT> ();
    
  if ( claseTropa != TTT_INFO_DAN_ARTILLERIA || sizeStruct != sizeof (TInfoDanArtilleria))
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Cargamos los datos
  m_InfoDanArtilleria[rango] = std::vector <TInfoDanArtilleria> (filas);
  for (UINT f = 0; f < filas; f++)
  {
    file->read (&(m_InfoDanArtilleria[rango][f])); 

    if (version<130)
    {
      m_InfoDanArtilleria[rango][f].m_Cue.m_Min=0;
      m_InfoDanArtilleria[rango][f].m_Cue.m_Max=0;
    }
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Carga los datos de modificadores de formacion desde disco
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadModifFormacion (fileSYS::IFile * file, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT claseTropa       = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT sizeStruct       = file->read <UINT> ();

  // Cogemos el puntero donde guardar los datos en funcion del tipo de tabla
  TModifFormacion * pModif = NULL;
  if (claseTropa == TTT_MODIF_FORM_INFANTERIA)
    pModif = m_ModifFormacionInf[rango];
  else if (claseTropa == TTT_MODIF_FORM_CABALLERIA)
    pModif = m_ModifFormacionCab[rango];
  else
  {    
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  if ((sizeStruct != sizeof (TModifFormacion)) || (filas != MFORM_COUNT))
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Leemos los datos dentro de nuestras tablas
  for (UINT f = 0; f < filas; f++)
    file->read (&pModif [f]);
  
  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Carga las tablas de cargas vs cuadro
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadTablaCargasVsCuadro (fileSYS::IFile * file, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT claseTropa       = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT sizeStruct       = file->read <UINT> ();

  // Cogemos el puntero donde guardar los datos en funcion del tipo de tabla
  TCargasVsCuadro * pTablaCargas = NULL;
  if (claseTropa == TTT_CARGAS_CUADRO_INF)
    pTablaCargas = m_CargasVsCuadroInf[rango];
  else if (claseTropa == TTT_CARGAS_CUADRO_CAB)
    pTablaCargas = m_CargasVsCuadroCab[rango];
  else
  {    
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  if ( (sizeStruct != sizeof ( TCargasVsCuadro )) || 
       (filas != 1) )
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Leemos los datos dentro de nuestras tablas
  for (UINT f = 0; f < filas; f++)
    file->read (&pTablaCargas [f]);
  
  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Carga las tablas de propiedades de cargas
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadTablaCargasPropiedades (fileSYS::IFile * file, UINT rango)
{
  assert (file);
  
  // Cargamos nuestros datos desde disco
  
  UINT tipotabla        = file->read <UINT> ();
  UINT filas            = file->read <UINT> ();
  UINT sizeStruct       = file->read <UINT> ();

  if ( tipotabla != TTT_CARGAS_PROPIEDADES ) {
  
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  if ((sizeStruct != sizeof (TCargasPropiedades)) || (filas != NUM_TROPAS_CARGAN))
  {
    assert(0 && "Fichero corrupto");
    return false;
  }
  
  // Leemos los datos dentro de nuestras tablas
  for (UINT f = 0; f < filas; f++)
    file->read (&m_CargasPropiedades [rango][f]);
  
  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Inicializa la lista de modificadores de terreno
//----------------------------------------------------------------------------------------------------------------

bool svcModeloCombate::loadModifTerreno (void)
{
  UINT idTT;
  
  idTT = tterreno2idx (TT_TERRENO_ARIDO);        // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)  
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_ZONA_VERDE);            // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_CAMINO);               // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_NIEVE_NO_PROFUNDA);      // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_TERRENO_HELADO);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_BARRIZAL);
  {
    m_ModifTerreno [idTT].m_Vel [0] = -g_ModVel;       // (+-) Vel Infanteria (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = -g_ModVel;       // (+-) Vel Caballeria (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0.f; // -(2*g_ModVel);   // (+-) Vel Artilleria (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_ANDARDIFICIL;
  }
  idTT = tterreno2idx (TT_NIEVE_PROFUNDA);
  {
    m_ModifTerreno [idTT].m_Vel [0] = - g_ModVel;  // (+-) Vel Infanteria (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_ANDARDIFICIL;
  }
  idTT = tterreno2idx (TT_TERRENO_CULTIVADO);              // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_TERRENO_ELEVADO);
  {
    //////////////////////////////////////////////////////////
    // OJO: SE SUPONE QUE YA NO SE UTILIZA ESTE TT
    //      17/I/2005
    //////////////////////////////////////////////////////////
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = g_ModRngDet;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = g_ModRngDet;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = g_ModRngDet;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_TRIGALES);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_RIO_POCO_PROFUNDO);
  {
    m_ModifTerreno [idTT].m_Vel [0] = -g_ModVel;       // (+-) Vel Infanteria (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = -g_ModVel;       // (+-) Vel Caballeria (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_ATAQUE_A_DISTANCIA | F_CORRER | F_LINEA | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_ATAQUE_A_DISTANCIA | F_CORRER | F_LINEA | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_ATAQUE_A_DISTANCIA | F_CORRER | F_LINEA | F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_VADEARRIO;
  }
  idTT = tterreno2idx (TT_RIO_PROFUNDO);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_VADEARRIO;
  }
  idTT = tterreno2idx (TT_BARRANCO);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_PANTANO);
  {
    m_ModifTerreno [idTT].m_Vel [0] = - g_ModVel;  // (+-) Vel Infanteria (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_ANDARDIFICIL;
  }
  idTT = tterreno2idx (TT_CIENAGA);
  {
    m_ModifTerreno [idTT].m_Vel [0] = - g_ModVel;  // (+-) Vel Infanteria (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_CORRER | F_CARGA;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
    m_ModifTerreno [idTT].m_AniAndar = ANI_ANDARDIFICIL;
  }
  idTT = tterreno2idx (TT_HIERBAS_ALTAS);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_BOSQUE);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_INTERIOR);
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = 0;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = 0;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  idTT = tterreno2idx (TT_PASO_ESTRECHO);     // --> NORMAL
  {
    m_ModifTerreno [idTT].m_Vel [0] = 0;  // (+-) Vel Infanteria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [1] = 0;  // (+-) Vel Caballeria  (%unitario)
    m_ModifTerreno [idTT].m_Vel [2] = 0;  // (+-) Vel Artilleria  (%unitario)
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [0] = F_LINEA | F_CUADRO;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [1] = F_LINEA | F_CUADRO;
    m_ModifTerreno [idTT].m_AccionesNoDisponibles [2] = F_LINEA | F_CUADRO;
    m_ModifTerreno [idTT].m_RngDet [0] = 0;  // (+-) Rango de detección Infanteria
    m_ModifTerreno [idTT].m_RngDet [1] = 0;  // (+-) Rango de detección Caballeria
    m_ModifTerreno [idTT].m_RngDet [2] = 0;  // (+-) Rango de detección Artilleria
    m_ModifTerreno [idTT].m_ResDis = 0;      // (+-) Resistencia ataque a distancia
  }
  
  return true;
}

//----------------------------------------------------------------------------------------------------------------
// Convierte del tipo de terreno a indice
//----------------------------------------------------------------------------------------------------------------

UINT svcModeloCombate::tterreno2idx (eTiposDeTerreno tipo) const
{
  assert (TT_COUNT == 20 && "Faltan tipos de terreno");
  // TODO: Optimizar todo esto; deberia existira una estructura que para cada tipo de terreno
  // conoce tanto el indice como las maskara!!!!
  
  switch (tipo)
  {
  case TT_TERRENO_ARIDO:
    return 0;
  case TT_ZONA_VERDE:
    return 1;
  case TT_CAMINO:
    return 2;
  case TT_NIEVE_NO_PROFUNDA:
    return 3;
  case TT_TERRENO_HELADO:
    return 4;
  case TT_BARRIZAL:
    return 5;
  case TT_NIEVE_PROFUNDA:
    return 6;
  case TT_TERRENO_CULTIVADO:
    return 7;
  case TT_TERRENO_ELEVADO:
    return 8;
  case TT_TRIGALES:
    return 9;
  case TT_RIO_POCO_PROFUNDO:
    return 10;
  case TT_RIO_PROFUNDO:
    return 11;
  case TT_BARRANCO:
    return 12;
  case TT_PANTANO:
    return 13;
  case TT_CIENAGA:
    return 14;
  case TT_HIERBAS_ALTAS:
    return 15;
  case TT_BOSQUE:
    return 16;
  case TT_INTERIOR:
    return 17;
  case TT_PASO_ESTRECHO:
    return 18;
  case TT_OBSTACULO:
    return 19;
  default:
    assert (0 && "TERRENO INVALIDO!");
  };
  
  return 0;
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el modificador de terreno a partir del tipo de terreno y clase de tropa
//----------------------------------------------------------------------------------------------------------------

const TModifTerreno & svcModeloCombate::getModifTerreno (eTiposDeTerreno tterreno) const
{
  UINT idx = tterreno2idx (tterreno);
  return m_ModifTerreno [idx];
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el modificador de formacion a partir de la formacion
//----------------------------------------------------------------------------------------------------------------

const TModifFormacion & svcModeloCombate::getModifForm (eTipoDeFormacion form, eClaseTropa claseTropa, eRangoTropa rango) const
{
  eModifFormacion mform = getModifForm (form);
  
  if (claseTropa == CT_INFANTERIA)
    return m_ModifFormacionInf [rango][mform];
  else if (claseTropa == CT_CABALLERIA)
    return m_ModifFormacionCab [rango][mform];
  else if (claseTropa == CT_ARTILLERIA)
    return TModifFormacion::s_Neutro;

  assert (0 && "No modificador para esta clase de tropa");
  return TModifFormacion::s_Neutro;
}

eModifFormacion svcModeloCombate::getModifForm (eTipoDeFormacion form) const
{
  // TODO: esta enumeracion es casi identica al modificador; tal vez podriamos usar directamente el valor!
  switch (form)
  {
  case FORM_NULL:
    return MFORM_NULL;
  case FORM_LINEA_SIMPLE:
    return MFORM_LINEA_SIMPLE;
  case FORM_LINEA_DOBLE:
    return MFORM_LINEA_DOBLE;
  case FORM_LINEA_TRIPLE:
    return MFORM_LINEA_TRIPLE;
  case FORM_COLUMNA:
    return MFORM_COLUMNA;
  case FORM_CUADRO:
    return MFORM_CUADRO;
  case FORM_CARGA:
    return MFORM_CARGA;
  case FORM_MELEE:
    return MFORM_MELEE;
  case FORM_HUIDA:
    return MFORM_HUIDA;
  case FORM_APOSTADO:
    return MFORM_APOSTADO;
  default:
    assert (0 && "Formacion no implementada!");
    return MFORM_NULL;
  };
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el modificador adecuado en funcion del numero de soldados
//----------------------------------------------------------------------------------------------------------------

const TDescDanTropa & svcModeloCombate::getDescDan (eClaseTropa idxClaseTropa, int soldados, eRangoTropa rango) const
{
  assert(idxClaseTropa!=CT_ARTILLERIA);

  // Seleccionamos la tabla adecuado segun la clase de tropa
  
  const std::vector <TDescDanTropa> * pDanTropa = NULL;
  if (idxClaseTropa == CT_INFANTERIA)
    pDanTropa = &m_DescDanTropaInf[rango];
  else if (idxClaseTropa == CT_CABALLERIA)
    pDanTropa = &m_DescDanTropaCab[rango];

  assert (pDanTropa);
  // Buscamos la entrada segun el numero de soldados : La tabla esta ordenada de mayor a menor; cada entrada
  // en la tabla representa la definicion de daños si el numero de soldados es >= que el de la entrada

  static TDescDanTropa retVal;
  UINT count = pDanTropa->size ();
  UINT i = 0;

  while(i<count && soldados<(*pDanTropa)[i].m_Soldados)
  {
    ++i;
  }

  retVal = (*pDanTropa) [i];
  
  if(i>=count)
  {
    // No encontramos ninguna entrada para nuestro numero de soldados : retornamos la ultima entrada
    assert (0 && "Warning: Numero de soldados no encontrado en tabla!");
    return (*pDanTropa) [pDanTropa->size ()-1];
  }

  if(i<=0 || retVal.m_Soldados==soldados)
  {
    // Estamos en el primer segmento o tenemos exactamente el número de soldados de este segmento
    // no hay nada q interpolar
    return retVal;
  }

  // hay que interpolar entre el segmento elegido y el anterior
  TDescDanTropa prevDesc = (*pDanTropa)[i-1];
  float fProp = (float)(soldados-retVal.m_Soldados)/(float)(prevDesc.m_Soldados-retVal.m_Soldados);
  retVal.m_Car.m_Min = (int)(fProp*prevDesc.m_Car.m_Min + (1.0f - fProp)*retVal.m_Car.m_Min);
  retVal.m_Car.m_Max = (int)(fProp*prevDesc.m_Car.m_Max + (1.0f - fProp)*retVal.m_Car.m_Max);
  retVal.m_Cue.m_Min = (int)(fProp*prevDesc.m_Cue.m_Min + (1.0f - fProp)*retVal.m_Cue.m_Min);
  retVal.m_Cue.m_Max = (int)(fProp*prevDesc.m_Cue.m_Max + (1.0f - fProp)*retVal.m_Cue.m_Max);
  retVal.m_Soldados = soldados;

  return retVal;
}

//----------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------
#ifdef _INFODEBUG
static void InfoDebugOclusores(CInfoDebug* m_pInfoDebug, CTropaBase* pTropaAta, CTropaBase* pTropaDef, std::vector<CTropaBase*>& oclusores, bool bDevolverOclusores)
{
 // if (oclusores.size()>0)
 {
   assert(m_pInfoDebug!=0);

   // si hay oclusión, dibujar la caja de disparo, y las cajas de tropas que ocluyen:

   {
     CVector2 pt1, pt2;
     pt1=pTropaAta->getCenter();
     pt2=pTropaDef->getCenter();
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado,
       CLine(math::conv2Vec3D(pt1,utilsTropa::svcMapaFisico->getY(pt1)+100.f),
             math::conv2Vec3D(pt2-pt1,utilsTropa::svcMapaFisico->getY(pt2)-utilsTropa::svcMapaFisico->getY(pt1))), // +100.f)),
                 bDevolverOclusores? CColor(0,255,0):CColor(0,0,255), 30);
     CVector2 p1,p2,p3;
     CVector2 d(pt2-pt1);
     d.normalize();
     CVector2 p(d);
     p.rotate(math::deg2rad(90.f));
     p1=pt2-p*50.f;
     p2=pt2+d*150.f;
     p3=pt2+p*50.f;
     CVector3 pp1,pp2,pp3;
     pp1=math::conv2Vec3D(p1,utilsTropa::svcMapaFisico->getY(pt2)+100.f);
     pp2=math::conv2Vec3D(p2,utilsTropa::svcMapaFisico->getY(pt2)+100.f);
     pp3=math::conv2Vec3D(p3,utilsTropa::svcMapaFisico->getY(pt2)+100.f);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp1,pp2-pp1), bDevolverOclusores? CColor(0,255,0):CColor(0,0,255), 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp2,pp3-pp2), bDevolverOclusores? CColor(0,255,0):CColor(0,0,255), 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp3,pp1-pp3), bDevolverOclusores? CColor(0,255,0):CColor(0,0,255), 30);
   }

   for (int o=0; o<(int)oclusores.size(); o++)
   {
     CTropaBase *pTropa=oclusores[o];
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado,
         CLine(pTropaAta->getPos3D(),pTropa->getPos3D()-pTropaAta->getPos3D()),
         CColor(255,0,0), 30);
   }

   oclusores.insert(oclusores.begin(), pTropaAta);
   for (int o=0; o<(int)oclusores.size(); o++)
   {
     CTropaBase *pTropa=oclusores[o];
     CVector2 ptT=pTropa->getPos();
     CVector2 orT(0.f,1.f);
     orT.rotate(pTropa->getAngle());
     CVector2 prT(orT);
     prT.rotate(math::deg2rad(90.f));
     float ancho, largo;
     ancho=pTropa->getDatosTropa()->m_FormacionAct.GetAncho();
     largo=pTropa->getDatosTropa()->m_FormacionAct.GetLargo();
     CColor color(o==0? CColor(0,255,0):CColor(255,0,0));
     CVector2 p1,p2,p3,p4;
     p1=ptT+prT*ancho*.5f;
     p2=ptT+prT*ancho*.5f-orT*largo;
     p3=ptT-prT*ancho*.5f-orT*largo;
     p4=ptT-prT*ancho*.5f;
     CVector3 pp1,pp2,pp3,pp4;
     pp1=math::conv2Vec3D(p1, utilsTropa::svcMapaFisico->getY(p1)+100.f);
     pp2=math::conv2Vec3D(p2, utilsTropa::svcMapaFisico->getY(p2)+100.f);
     pp3=math::conv2Vec3D(p3, utilsTropa::svcMapaFisico->getY(p3)+100.f);
     pp4=math::conv2Vec3D(p4, utilsTropa::svcMapaFisico->getY(p4)+100.f);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp1,pp2-pp1), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp2,pp3-pp2), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp3,pp4-pp3), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp4,pp1-pp4), color, 30);
   }
 }
}
#endif

//----------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------
#ifdef _INFODEBUG
static void InfoDebugOclusores2(CInfoDebug* m_pInfoDebug, CTropaBase* pTropaAta, CTropaBase* pTropaDef, const std::vector<utilsTropa::SOclusor>& oclusores, bool bAtacante)
{
 // pinta oclusores de la tropa atacada:
 //

 // oclusores.insert(oclusores.begin(), pTropaAta);
 for (int o=0; o<(int)oclusores.size(); o++)
 {
   CTropaBase *pTropa=oclusores[o].m_pTropa;
   if (pTropa!=pTropaDef)
   {
     CVector2 ptT=pTropa->getPos();
     CVector2 orT(0.f,1.f);
     orT.rotate(pTropa->getAngle());
     CVector2 prT(orT);
     prT.rotate(math::deg2rad(90.f));
     float ancho, largo;
     ancho=pTropa->getDatosTropa()->m_FormacionAct.GetAncho();
     largo=pTropa->getDatosTropa()->m_FormacionAct.GetLargo();
     CColor color(0,0,255); // o==0? CColor(0,255,0):CColor(255,0,0));
     CVector2 p1,p2,p3,p4;
     p1=ptT+prT*ancho*.5f;
     p2=ptT+prT*ancho*.5f-orT*largo;
     p3=ptT-prT*ancho*.5f-orT*largo;
     p4=ptT-prT*ancho*.5f;
     CVector3 pp1,pp2,pp3,pp4;
     pp1=math::conv2Vec3D(p1, utilsTropa::svcMapaFisico->getY(p1)+100.f);
     pp2=math::conv2Vec3D(p2, utilsTropa::svcMapaFisico->getY(p2)+100.f);
     pp3=math::conv2Vec3D(p3, utilsTropa::svcMapaFisico->getY(p3)+100.f);
     pp4=math::conv2Vec3D(p4, utilsTropa::svcMapaFisico->getY(p4)+100.f);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp1,pp2-pp1), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp2,pp3-pp2), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp3,pp4-pp3), color, 30);
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado, CLine(pp4,pp1-pp4), color, 30);

     CVector2 pt1, pt2;
     pt1=pTropaAta->getCenter();
     pt2=pTropa->getCenter();
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addLine(CInfoDebug::LT_FuegoCruzado,
       CLine(math::conv2Vec3D(pt1,utilsTropa::svcMapaFisico->getY(pt1)+100.f),
             math::conv2Vec3D(pt2-pt1,utilsTropa::svcMapaFisico->getY(pt2)-utilsTropa::svcMapaFisico->getY(pt1))), // +100.f)),
                 CColor(0,0,255), 30);

     CVector3 pt(math::conv2Vec3D((pt1+pt2)/2.f, utilsTropa::svcMapaFisico->getY((pt1+pt2)/2.f)+(bAtacante? 50.f:150.f)));
     std::stringstream ss;
     ss<<oclusores[o].m_Porc<<'%';
     (const_cast<CInfoDebug *>(m_pInfoDebug))
       ->addText(CInfoDebug::LT_FuegoCruzado, pt, ss.str(), CColor(0,0,128), 30);

   }
 }

 // porcentaje recibido por la tropa atacada:
 //
 {
   CVector2 pt1, pt2;
   pt1=pTropaAta->getCenter();
   pt2=pTropaDef->getCenter();
   CVector3 pt(math::conv2Vec3D((pt1+pt2)/2.f, utilsTropa::svcMapaFisico->getY((pt1+pt2)/2.f)+(bAtacante? 50.f:150.f)));
   std::stringstream ss;
   if (oclusores.size()>0 && oclusores.back().m_pTropa==pTropaDef)
     ss<<oclusores.back().m_Porc<<'%';
   else
     ss<<"0%";
   (const_cast<CInfoDebug *>(m_pInfoDebug))
     ->addText(CInfoDebug::LT_FuegoCruzado, pt, ss.str(), CColor(0,128,0), 30);
 }
}
#endif


//----------------------------------------------------------------------------------------------------------------
// Computa los daños a partir de un tipo de ataque y las tropas que atacan y reciben el ataque respectivamente
// TODO: 
//   -  Tal vez esta funcion deberia comprobar las oclusiones de construcciones; para determinar si debe aplicar
//      resistencia; tener en cuenta q cuando se aplique absorcion de impacto; no debe ser el mismo porcentaje q 
//      cuando se esta apostado; para evitar q estar detras de una construccion sea igual q estar apostado en ella
//   -  Si esta funcion ademas va a producir daño a los oclusores; tal vez deberia ser configurable o 
//      deberia llamarse de otra forma
//   - Debe calcular los rangos de distancia; teniendo en cuenta tropas apostadas en modulos
//   - Ocluir con el terreno (montañas y demas)
//
//----------------------------------------------------------------------------------------------------------------

TResultadoAtaque svcModeloCombate::computarResultadoAtaque (CTropaBase * pTropaAta, 
                                                            CTropaBase * pTropaDef, 
                                                            eTipoAtaque tipoAtaque, 
                                                            int unidadesAta, 
                                                            std::vector<utilsTropa::SOclusor> *pOclusores/*=0*/, 
                                                            const int *pPorcentajeImpuestoDano/*=0*/, 
                                                            Stat::CLogEntry* pLogEntry) const
{
  TResultadoAtaque result;

  int danBase = 0;
  int modDan = 0;
  int modRes = 0;
  int modDanGlobal = 100;

  // --> Ataque a distancia:
  if (tipoAtaque == TA_DISTANCIA)
  {
    result = computarResultadoDistancia(pTropaAta, pTropaDef, unidadesAta, pOclusores, pPorcentajeImpuestoDano, pLogEntry, danBase, modDan, modRes, modDanGlobal);
  }
  // --> Carga
  else if (tipoAtaque == TA_CARGA)
  {
    result = computarResultadoCarga(pTropaAta, pTropaDef, unidadesAta, pPorcentajeImpuestoDano, pLogEntry, danBase, modDan, modRes, modDanGlobal);
  }
  // --> Cuerpo a Cuerpo
  else if (tipoAtaque == TA_CUERPO)
  {
    result = computarResultadoMelee(pTropaAta, pTropaDef, unidadesAta, pPorcentajeImpuestoDano, pLogEntry, danBase, modDan, modRes, modDanGlobal);
  }

  // Aplicamos modificadores calculados (truncando a valores correctos)
  if (modDan > g_MaxModifDan)
    modDan = g_MaxModifDan;
  else if (modDan < g_MinModifDan)
    modDan = g_MinModifDan;
  
  if (modRes > g_MaxModifRes)
    modRes = g_MaxModifRes;
  else if (modRes < g_MinModifRes)
    modRes = g_MinModifRes;
  
  if (pPorcentajeImpuestoDano!=0)
    modDanGlobal = modDanGlobal * *pPorcentajeImpuestoDano / 100;

#ifdef _INFODEBUG
      // dibujar porcentaje global aplicado al enemigo (incluyendo oclusiones amigas):
      //
      {
        svcDatosTropa const *datosAta = pTropaAta->getDatosTropa();
        CVector3 pt(math::conv2Vec3D(pTropaDef->getCenter(), utilsTropa::svcMapaFisico->getY(pTropaDef->getCenter())+(datosAta->m_TipoBando==TBANDO_ATACANTE? 50.f:150.f)));
        std::stringstream ss;
        ss<<modDanGlobal<<'%';
        (const_cast<CInfoDebug *>(m_pInfoDebug))
          ->addText(CInfoDebug::LT_FuegoCruzado, pt, ss.str(), pOclusores!=0? CColor(0,128,0):CColor(0,0,128), 30);
      }
#endif

  danBase += (modDan * danBase) / 100;
  danBase -= (modRes * danBase) / 100;

  // aplicamos modificador global de porcentaje de daños:
  assert(modDanGlobal>=0 && modDanGlobal<=100);
  danBase=danBase*modDanGlobal/100;

  // Truncamos el valor de daños de 0 al valor del numero de unidades de nuestra tropa; para evitar que matemos
  // mas unidades de las q somos
  
  if (danBase <= 0)
    danBase = 0;
  else if (danBase > (unidadesAta*VIDA_UNIDAD))
    danBase = unidadesAta*VIDA_UNIDAD;


#if STAT_LOGGING
  // Aunque calculé el daño antes, lo piso ahora para asegurarme de que esta bien
  if (pLogEntry && pLogEntry->IsOk())
  {
    switch (pLogEntry->GetTable())
    {
    case Stat::TABLE_MELEE:
      pLogEntry->Set(Stat::MELEE_DAMAGE, danBase);
      break;
    case Stat::TABLE_CARGA:
      pLogEntry->Set(Stat::CARGA_DAMAGE, danBase);
      break;
    case Stat::TABLE_DISPARO:
      pLogEntry->Set(Stat::DISPARO_DAMAGE, danBase);
      break;
    }
  }
#endif

  // Rellenamos los daños finales calculados dentro de la estructura de salida
  result.m_Danos = danBase;
  
  return result;
}

//----------------------------------------------------------------------------------------------------------------
// Determina si se puede romper una formacion de cuadro. Usamos la tabla y los modificadores correspondientes
//----------------------------------------------------------------------------------------------------------------
bool svcModeloCombate::puedeRomperCuadro( CTropaBase* tropaEnCuadro, CTropaBase* tropaCargando )
{
  // determinamos si la tropa que hace la carga es inf. o cab.
  TCargasVsCuadro* tablaVsCuadro = 0;
  float multNumUnidadesAtaqueDist = 0.0f;
  float multNumUnidadesCarga = 0.0f;
  
  eRangoTropa rangoCargando = tropaCargando->getDatosTropa()->m_Rango;
  eRangoTropa rangoEnCuadro = tropaEnCuadro->getDatosTropa()->m_Rango;
  
  if ( tropaCargando->getDatosTropa()->m_ClaseTropa == CT_INFANTERIA ) {
    tablaVsCuadro = m_CargasVsCuadroInf[ rangoCargando ];
    multNumUnidadesAtaqueDist = m_CargasPropiedades[ rangoCargando ][CT_INFANTERIA].m_MultNumUnidadesReq_AtaqueDist;
    multNumUnidadesCarga = m_CargasPropiedades[ rangoCargando ][CT_INFANTERIA].m_MultNumUnidadesReq_Carga;
  } else {
    tablaVsCuadro = m_CargasVsCuadroCab[ rangoCargando ];
    multNumUnidadesAtaqueDist = m_CargasPropiedades[ rangoCargando ][CT_CABALLERIA].m_MultNumUnidadesReq_AtaqueDist;
    multNumUnidadesCarga = m_CargasPropiedades[ rangoCargando ][CT_CABALLERIA].m_MultNumUnidadesReq_Carga;
  }
  
  // obtenemos multiplicador
  float multUnidDef = 0.0f;
  
  if ( rangoEnCuadro == RANGO_NOVATO ) {
    multUnidDef = tablaVsCuadro->m_MultVsNovata;
  } else if ( rangoEnCuadro == RANGO_MEDIO ) {
    multUnidDef = tablaVsCuadro->m_MultVsMedia;
  } else if ( rangoEnCuadro == RANGO_EXPERIMENTADO ) {
    multUnidDef = tablaVsCuadro->m_MultVsExperimentada;
  } else if ( rangoEnCuadro == RANGO_VETERANO ) {
    multUnidDef = tablaVsCuadro->m_MultVsVeterana;
  } else if ( rangoEnCuadro == RANGO_ELITE ) {
    multUnidDef = tablaVsCuadro->m_MultVsElite;
  }
  
  // hacemos el cálculo para saber si el numero de unidades atacantes puede romper la formacion de cuadro
  // un multiplicador negativo lo consideramos como 'fracaso de carga'. Un multiplicador == 0, lo consideramos
  // 'carga efectiva'
  if ( multUnidDef < 0.0f ) {
    return false;
  }
  
  if ( multUnidDef == 0.0f ) {
    return true;
  }
  
  UINT numUnidadesNecesarias = UINT( float(tropaEnCuadro->getDatosTropa()->m_Unidades.size()) * multUnidDef);
  
  // si la tropa esta atacando a distancia, o ya ha sido victima de una carga, hay que aplicar los multiplicadores
  // correspondientes
  if ( tropaEnCuadro->getState() == TS_ATACANDO_A_DISTANCIA ) {
    numUnidadesNecesarias = UINT( float(numUnidadesNecesarias) * multNumUnidadesAtaqueDist);
  }
  
  if ( tropaCargando->getDatosTropa()->m_Unidades.size() >= numUnidadesNecesarias ) {
    return true;
  }
  
  // si el cuadro está a "medio hacer" devolvemos true
  if(tropaEnCuadro->getState() != TS_CUADRO)
  {
    assert(!"El estado de la tropa no es el cuadro!!!");
    return true;
  }
  TropaState_Cuadro const* pCuadro = (TropaState_Cuadro const*)tropaEnCuadro->getDatosTropa()->m_pTropaAI->getDatosAITropa()->m_State;
  if(!pCuadro->cuadroPreparado())
    return true;

  return false;
}

//----------------------------------------------------------------------------------------------------------------
// calcChargeDamage: retorna cantidad de unidades que moriran por la carga de una tropa sobre otra.
//----------------------------------------------------------------------------------------------------------------
int svcModeloCombate::calcChargeDamage( CTropaBase* tropa, CTropaBase* tropaCargando, Stat::CLogEntry* pLogEntry )
{
  TResultadoAtaque res = computarResultadoAtaque( tropaCargando, tropa, TA_CARGA, tropaCargando->getDatosTropa()->m_Unidades.size(), 0, 0, pLogEntry);
  int dano = int(res.m_Danos / game::VIDA_UNIDAD);

#if STAT_LOGGING
  if (pLogEntry && pLogEntry->IsOk())
  {
    assert(pLogEntry->GetTable() == Stat::TABLE_CARGA);
    pLogEntry->Set(Stat::CARGA_DAMAGE, dano * game::VIDA_UNIDAD);
  }
#endif

  return dano;
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

float svcModeloCombate::getCargaDistMin(eClaseTropa claseTropa, eRangoTropa rango)
{
  assert( claseTropa != CT_ARTILLERIA );
  return m_CargasPropiedades[rango][claseTropa].m_DistMin;
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

float svcModeloCombate::getCargaDistMax(eClaseTropa claseTropa, eRangoTropa rango)
{  
  assert( claseTropa != CT_ARTILLERIA );
  return m_CargasPropiedades[rango][claseTropa].m_DistMax;
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el tipo de tropa de infanteria a partir del tipo de tropa
//----------------------------------------------------------------------------------------------------------------

eTipoInfanteria svcModeloCombate::getTipoTropaInf (eTipoTropa tipoTropa) const
{
  // TODO: Eliminar cuando decidamos q hacer con los mandos!!
  if ( (tipoTropa&TTROPA_MANDO) != 0 )
    return TI_FUSILEROS;
  
  assert (tipoTropa & TTROPA_INFANTERIA);
  
  switch (tipoTropa)
  {
  case TTROPA_INF_MILICIA:
    return TI_MILICIA;
  case TTROPA_INF_FUSILEROS:
    return TI_FUSILEROS;
  case TTROPA_INF_TIRADORES:
    return TI_TIRADORES;
  case TTROPA_INF_CAZADORES:
    return TI_CAZADORES;
  case TTROPA_INF_GRANADEROS:
    return TI_GRANADEROS;
  case TTROPA_INF_ELITE:
    return TI_ELITE;
  default:    
    assert (0 && "Tipo infanteria desconocido");
    return TI_FUSILEROS;
  };
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el tipo de tropa de infanteria a partir del tipo de tropa
//----------------------------------------------------------------------------------------------------------------

eTipoCaballeria svcModeloCombate::getTipoTropaCab (eTipoTropa tipoTropa) const
{
  assert (tipoTropa & TTROPA_CABALLERIA);
  
  switch (tipoTropa)
  {
  case TTROPA_CAB_HUSARES:
    return TC_HUSARES;
  case TTROPA_CAB_LANCEROS:
    return TC_LANCEROS;
  case TTROPA_CAB_DRAGONES:
    return TC_DRAGONES;
  case TTROPA_CAB_CORACEROS:
    return TC_CORACEROS;
  case TTROPA_CAM_TIRADORES:
    return TC_CAM_TIRADORES;
  case TTROPA_CAM_LANCEROS:
    return TC_CAM_LANCEROS;
  case TTROPA_CAB_ELITE:
    return TC_ELITE;
  default:    
    assert (0 && "Tipo caballeria desconocido");
    return TC_HUSARES;
  };
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene el tipo de tropa de infanteria a partir del tipo de tropa
//----------------------------------------------------------------------------------------------------------------

eTipoArtilleria svcModeloCombate::getTipoTropaArt (eTipoTropa tipoTropa) const
{
  assert (tipoTropa & TTROPA_ARTILLERIA);
  
  switch (tipoTropa)
  {
  case TTROPA_ART_MORTERO:
    return TA_MORTERO;
  case TTROPA_ART_CANON:
    return TA_CANON;
  case TTROPA_ART_OBUS:
    return TA_OBUS;
  case TTROPA_ART_CANON_CAB:
    return TA_CANON_CAB;
  case TTROPA_ART_OBUS_CAB:
    return TA_OBUS_CAB;
  case TTROPA_ART_CANON_CAMP:
    return TA_CANON_CAMP;
  default:    
    assert (0 && "Tipo artilleria desconocido");
    return TA_MORTERO;
  };
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene la clase de una tropa (INF, CAB, ART) a partir de un tipo de tropa concreto (FUSILERO, CANON, OBUS, ...)
//----------------------------------------------------------------------------------------------------------------

eClaseTropa svcModeloCombate::getClaseTropa (eTipoTropa tipoTropa) const
{
  // TODO: Eliminar cuando decidamos q hacer con los mandos!!
  if ( (tipoTropa&TTROPA_MANDO) != 0 )
    return CT_INFANTERIA;
  
  if (tipoTropa & TTROPA_INFANTERIA)
    return CT_INFANTERIA;
  else if (tipoTropa & TTROPA_CABALLERIA)
    return CT_CABALLERIA;
  else if (tipoTropa & TTROPA_ARTILLERIA)
    return CT_ARTILLERIA;
    
  assert (0 && "Clase de tropa no reconocida");
  return CT_INFANTERIA;
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene un descriptor de tropa a partir de un tipo
//----------------------------------------------------------------------------------------------------------------

const TDescTropa & svcModeloCombate::getDescTropa (eTipoTropa tipoTropa, eRangoTropa rango) const
{
  eClaseTropa claseTropa;
  UINT index = getTipoTropaIndex (tipoTropa, &claseTropa);
  
  if (claseTropa == CT_INFANTERIA)
    return (m_DescTropaInf [rango][index]);
  else if (claseTropa == CT_CABALLERIA)
    return (m_DescTropaCab [rango][index]);
  else if (claseTropa == CT_ARTILLERIA)
    return (m_DescTropaArt [rango][index]);
  
  assert (0 && "Clase de tropa no reconocida");
  return m_DescTropaInf [rango][0];
}

//----------------------------------------------------------------------------------------------------------------
// Obtiene un indice de tipo de tropa a partir de un tipo de tropa y opcionalmente rellena la clase de tropa
//----------------------------------------------------------------------------------------------------------------

UINT svcModeloCombate::getTipoTropaIndex (eTipoTropa tipoTropa, eClaseTropa * pClaseTropa) const
{
  eClaseTropa claseTropa = getClaseTropa (tipoTropa);
  if (pClaseTropa)
    *pClaseTropa = claseTropa;
  
  if (claseTropa == CT_INFANTERIA)
  {
    eTipoInfanteria index = getTipoTropaInf (tipoTropa);
    return (index);
  }
  else if (claseTropa == CT_CABALLERIA)
  {
    eTipoCaballeria index = getTipoTropaCab (tipoTropa);
    return (index);
  }
  else if (claseTropa == CT_ARTILLERIA)
  {
    eTipoArtilleria index = getTipoTropaArt (tipoTropa);
    return (index);
  }
  
  assert (0 && "Clase de tropa no reconocida");
  return 0;
}

//----------------------------------------------------------------------------------------------------------------
// Nombre:      getTipoTropa
// Parámetros:  la clase de tropa que es y dentro de esa clase el tipo de tropa que es (pasado como un UINT)
// Descripción: devuelve una variable del tipo eTipoTropa con el tipo de tropa que se ha pasado
//----------------------------------------------------------------------------------------------------------------

eTipoTropa svcModeloCombate::getTipoTropa (eClaseTropa claseTropa, UINT index) const
{
  eTipoTropa ret;

  switch( claseTropa ) 
  {
  	case CT_INFANTERIA:
      switch((eTipoInfanteria)index) {
      case TI_MILICIA:
        ret = TTROPA_INF_MILICIA;
        break;
      case TI_FUSILEROS:
        ret = TTROPA_INF_FUSILEROS;
        break;
      case TI_TIRADORES:
        ret = TTROPA_INF_TIRADORES;
        break;
      case TI_CAZADORES:
        ret = TTROPA_INF_CAZADORES;
        break;
      case TI_GRANADEROS:
        ret = TTROPA_INF_GRANADEROS;
        break;
      case TI_ELITE:
        ret = TTROPA_INF_ELITE;
        break;
      default:
        ret = TTROPA_INVALIDO;
        break;
       }
      break;
    case CT_CABALLERIA:
      switch((eTipoCaballeria)index) {
      case TC_HUSARES:
        ret = TTROPA_CAB_HUSARES;
        break;
      case TC_LANCEROS:
        ret = TTROPA_CAB_LANCEROS;
        break;
      case TC_DRAGONES:
        ret = TTROPA_CAB_DRAGONES;
        break;
      case TC_CORACEROS:
        ret = TTROPA_CAB_CORACEROS;
        break;
      case TC_CAM_TIRADORES:
        ret = TTROPA_CAM_TIRADORES;
        break;
      case TC_CAM_LANCEROS:
        ret = TTROPA_CAM_LANCEROS;
        break;
      case TC_ELITE:
        ret = TTROPA_CAB_ELITE;
        break;
      default:
        ret = TTROPA_INVALIDO;
        break;
      }
      break;
    case CT_ARTILLERIA:
      switch((eTipoArtilleria)index) {
      case TA_MORTERO:
        ret = TTROPA_ART_MORTERO;
        break;
      case TA_CANON:
        ret = TTROPA_ART_CANON;
        break;
      case TA_OBUS:
        ret = TTROPA_ART_OBUS;
        break;
      case TA_CANON_CAB:
        ret = TTROPA_ART_CANON_CAB;
        break;
      case TA_OBUS_CAB:
        ret = TTROPA_ART_OBUS_CAB;
        break;
      case TA_CANON_CAMP:
        ret = TTROPA_ART_CANON_CAMP;
        break;
      default:
        ret = TTROPA_INVALIDO;
        break;
      }
      break;
  	default:
  		assert(false && "Clase de tropa no válida");
      ret = TTROPA_INVALIDO;
  }

  return ret;
}

//----------------------------------------------------------------------------------------------------------------
// getArtDamage
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getArtDamage( CTropaBase* tropa, float distDisparo )
{
  eTipoTropa  tipo  = tropa->getDatosTropa()->m_TipoTropa;
  eRangoTropa rango = tropa->getDatosTropa()->m_Rango;

  // determinamos en que rango estamos
  UINT rangoDisparo;

  if (distDisparo <= tropa->getDatosTropa()->m_fRngDis1)
  {
    rangoDisparo = 1;
  }
  else if (distDisparo <= tropa->getDatosTropa()->m_fRngDis2) 
  {
    rangoDisparo = 2;
  }
  else
  {
    rangoDisparo = 3;
  }

  return getArtDamage(tipo, rango, rangoDisparo);
}


//----------------------------------------------------------------------------------------------------------------
// Devuelve el daño de un tipo de artillería dado a cierta distancia.
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getArtDamage( eTipoTropa tipoTropa, eRangoTropa rangoTropa, UINT rangoDisparo ) const
{
  rq(rangoDisparo >= 1 && rangoDisparo <= 3);

  UINT dano = 0;

  eTipoArtilleria tipoArt = getTipoTropaArt(tipoTropa);

  switch (rangoDisparo)
  {
  case 1: dano = m_InfoDanArtilleria[ rangoTropa ][ tipoArt ].m_DamageRng1; break;
  case 2: dano = m_InfoDanArtilleria[ rangoTropa ][ tipoArt ].m_DamageRng2; break;
  case 3: dano = m_InfoDanArtilleria[ rangoTropa ][ tipoArt ].m_DamageRng3; break;
  }

  return dano;
}


//----------------------------------------------------------------------------------------------------------------
// getArtPrecVsEdif
//----------------------------------------------------------------------------------------------------------------
float svcModeloCombate::getArtPrecVsEdif( eTipoTropa tipoTropa, eRangoTropa rango )
{
  eTipoArtilleria tipoArt = getTipoTropaArt( tipoTropa );
  
  return m_InfoDanArtilleria[ rango ][ tipoArt ].m_PrecisionVsEdif;
}

float svcModeloCombate::getArtPrecVsEdif( CTropaBase* tropa )
{
  return getArtPrecVsEdif( tropa->getDatosTropa()->m_TipoTropa, tropa->getDatosTropa()->m_Rango );
}


//----------------------------------------------------------------------------------------------------------------
// getValorRangoDet
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorRangoDet( const SBatallon* batallon, CTropaBase* tropa )
{
  return getValorRangoDet( batallon->Atributos, batallon->TipoTropa, batallon->rango, tropa );
}

UINT svcModeloCombate::getValorRangoDet( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa )
{
  // OJO!!! Usamos el rango de dis3, ya q es mas claro para el jugador...
  //        el rango de disparo no corresponde con el de detección!

  UINT rangoDet = 0;
  
  if ( descTropa.m_uFlagsAccionesDisponibles & F_ATAQUE_A_DISTANCIA || getClaseTropa(tipoTropa) == CT_ARTILLERIA ) {
  
    rangoDet = UINT( descTropa.m_fRngDis3 / 100.0f );      // rango en cm. lo pasamos a metros
  }
  
  return rangoDet;
}

//----------------------------------------------------------------------------------------------------------------
// getValorArmadura
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorArmadura( const SBatallon* batallon, CTropaBase* tropa )
{
  return getValorArmadura( batallon->Atributos, batallon->TipoTropa, batallon->rango, tropa );
}

UINT svcModeloCombate::getValorArmadura( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa )
{
  // tenemos que recorrer tooodas las tablas de las tropas y buscar el valor máximo que representará nuestro 100% 
  // en base a eso retornamos el valor adecuado.
  
  UINT maxProm = m_MaxValArmadura;

  UINT thisProm = getValorPromedioArmadura( descTropa.m_uResDis, 
                                            descTropa.m_uResCar, 
                                            descTropa.m_uResCue );
  UINT val = UINT( thisProm * ( 100.0f / maxProm ) );
  
  return val;
}

//----------------------------------------------------------------------------------------------------------------
// getValorAtaqueADist
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorAtaqueADist( const SBatallon* batallon, CTropaBase* tropa )
{
  return getValorAtaqueADist( batallon->Atributos, batallon->TipoTropa, batallon->rango,  tropa );
}

UINT svcModeloCombate::getValorAtaqueADist( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa )
{
  // tenemos que recorrer tooodas las tablas de las tropas y buscar el valor máximo que representará nuestro 100% 
  // en base a eso retornamos el valor adecuado
  
  UINT val = 0;
  
  if ( descTropa.m_uFlagsAccionesDisponibles & F_ATAQUE_A_DISTANCIA ) {
  
    if ( getClaseTropa( tipoTropa ) == CT_ARTILLERIA ) {
      return getValorAtaqueADistArt( descTropa, tipoTropa, rango, tropa );
    }
  
    UINT maxProm = m_MaxValAtaqueADist;
    UINT numUnidades = tropa ? tropa->getDatosTropa()->m_Unidades.size() : descTropa.m_uUnidades;
    
    if ( numUnidades == 0 ) {
      return val;
    }
    
    const TDescDanTropa& descDan = getDescDan( getClaseTropa( tipoTropa ), numUnidades, rango );
    
    UINT thisProm = getValorPromedioAtaqueADist( descDan, (float)descTropa.m_uPrecDis1  / 100.0f, 
                                                          (float)descTropa.m_uPrecDis2  / 100.0f, 
                                                          (float)descTropa.m_uPrecDis3  / 100.0f, 
                                                          (float)descTropa.m_uModDanDis / 100.0f,
                                                          getClaseTropa( tipoTropa ),
                                                          rango );
    
    val = UINT( math::round( thisProm * ( 100.0f / maxProm ) ) );
    
    if ( val == 0 && thisProm > 0 ) {
      val = 1;
    }
  }
  
  return val;
}


//----------------------------------------------------------------------------------------------------------------
// getValorAtaqueCAC
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorAtaqueCAC( const SBatallon* batallon, CTropaBase* tropa )
{
  return getValorAtaqueCAC( batallon->Atributos, batallon->TipoTropa, batallon->rango, tropa );
}

UINT svcModeloCombate::getValorAtaqueCAC( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa )
{
  // tenemos que recorrer tooodas las tablas de las tropas y buscar el valor máximo que representará nuestro 100% 
  // en base a eso retornamos el valor adecuado

  UINT val = 0;
  
  if ( descTropa.m_uFlagsAccionesDisponibles & F_ATAQUE_CUERPO_A_CUERPO && 
       getClaseTropa(tipoTropa ) != CT_ARTILLERIA ) {
  
    UINT maxProm = m_MaxValAtaqueCAC;
    UINT numUnidades = tropa ? tropa->getDatosTropa()->m_Unidades.size() : descTropa.m_uUnidades;
    
    if ( numUnidades == 0 ) {
      return val;
    }
    
    const TDescDanTropa& descDan = getDescDan( getClaseTropa( tipoTropa ), numUnidades, rango );
    
    UINT thisProm = getValorPromedioAtaqueCAC( descDan, (float)descTropa.m_uPrecCue / 100.0f, 
                                                        (float)descTropa.m_uModDanCue / 100.0f, 
                                                        getClaseTropa( tipoTropa ),
                                                        rango );
    
    val = UINT( math::round( thisProm * ( 100.0f / maxProm ) ) );
    
    if ( val == 0 && thisProm > 0 ) {
      val = 1;
    }
  }
  
  return val;
}

//----------------------------------------------------------------------------------------------------------------
// getValorAtaqueADistArt
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorAtaqueADistArt( const SBatallon* batallon, CTropaBase* tropa )
{
  return getValorAtaqueADistArt( batallon->Atributos, batallon->TipoTropa, batallon->rango, tropa );
}

UINT svcModeloCombate::getValorAtaqueADistArt( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa )
{
  UINT val = 0;
  
  UINT maxProm = m_MaxValAtaqueADistArt;
  
  const TInfoDanArtilleria& descDan = getInfoDanArtilleria( tipoTropa, rango );
  
  UINT thisProm = getValorPromedioAtaqueADistArt( descDan, (float)descTropa.m_uPrecDis1 / 100.0f, 
                                                           (float)descTropa.m_uPrecDis2 / 100.0f, 
                                                           (float)descTropa.m_uPrecDis3 / 100.0f );
    
  val = UINT( thisProm * ( 100.0f / maxProm ) );

  return val;
}

//----------------------------------------------------------------------------------------------------------------
// Nombre       : computarResultadoDistancia
// Parámetros   : CTropaBase * pTropaAta, // Tropa que ataca
//                CTropaBase * pTropaDef, // Tropa que recibe
//                int unidadesAta,        // Unidades que cuentan como atacando
//                std::vector<utilsTropa::SOclusor>* pOclusores, // Lista de oclusores. Puede ser 0
//                const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
//                Stat::CLogEntry* pLogEntry    // Log generado con el calculo. Puede ser 0
// Descripción  : computa el resultado del combate de distancia (más bien del ataque de distancia) representado
//              por los datos pasados.
//----------------------------------------------------------------------------------------------------------------
TResultadoAtaque svcModeloCombate::computarResultadoDistancia(CTropaBase * pTropaAta, // Tropa que ataca
                                                              CTropaBase * pTropaDef, // Tropa que recibe
                                                              int unidadesAta,        // Unidades que cuentan como atacando
                                                              std::vector<utilsTropa::SOclusor>* pOclusores, // Lista de oclusores. Puede ser 0
                                                              const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                                              Stat::CLogEntry* pLogEntry,
                                                              int& danBase,                 // el daño base del ataque
                                                              int& modDan,                  // el modificador al daño
                                                              int& modRes,                  // el modificador a la resistencia
                                                              int& modDanGlobal) const    // Log generado con el calculo. Puede ser 0
{
  TResultadoAtaque result;

  // Obtenemos algunos datos antes de todo
  svcDatosTropa * datosAta = pTropaAta->getDatosTropa ();
  svcDatosTropa * datosDef = pTropaDef->getDatosTropa ();
  eClaseTropa claseTropaAta = datosAta->m_ClaseTropa;
  eClaseTropa claseTropaDef = datosDef->m_ClaseTropa;
  eTipoTropa tipoTropaAta = datosAta->m_TipoTropa;
  eTipoTropa tipoTropaDef = datosDef->m_TipoTropa;
  eRangoTropa rangoTropaAta = datosAta->m_Rango;
  eRangoTropa rangoTropaDef = datosDef->m_Rango;

  result.m_Rango = RNG_OUT;
  result.m_TipoAtaque = TA_DISTANCIA;
  result.m_Flanco = FLANCO_DELANTERO;
  result.m_bOclusion = false;
  result.m_Danos = 0;
  result.m_NumOclusoresAmigos = 0;

  // TODO: Determinamos si existen oclusores q amortiguen el impacto;
  // TODO: ... Notificar daños a la construccion --> Construccion visualice impactos de balas y tal vez trazadas de balas

  // Calculamos el rango de ataque a distancia teniendo en cuenta si la tropa esta apostada
  float fSqDist = utilsTropa::sqDistancia(pTropaAta, pTropaDef);
  float  fTRango = 0.0f;
  utilsTropa::calcRangosDisparo(pTropaAta, fSqDist, result.m_Rango, fTRango);

  if (result.m_Rango != RNG_OUT)
  {
    UINT precision = datosAta->m_uPrecDis [result.m_Rango];
    assert (precision >= 0 && precision <= g_MaxPrecision);

    // Calculamos el daño base segun el numero de soldados y la precision de la tropa; para ello lanzamos un numero
    // aleatorio; si este es <= q la precision de la tropa se aplica el maximo daño; en caso contrario restamos
    // un punto (100pv) por cada 10% excedido en la precision; sin pasar del minimo

    // ------- Cut here
    // *** UNIDADES ATACANTES ***
    //unidadesAta = datosAta->m_Unidades.size();
    // ------ End cut here

    // si el número de unidades es 0, el resto de cosas tb son 0
    if(unidadesAta == 0)
    {
      danBase = 0;
      modDan = 0;
      modRes = 0;
      modDanGlobal = 0;
      return result;
    }

    const TDescDanTropa & descDan = getDescDan (claseTropaAta, unidadesAta, rangoTropaAta);


    // Vemos si calculamos el daño por encima de los escalones o por debajo
#if DAMAGE_POR_DEBAJO
    int iMaxDamageRng1 = descDan.m_DisRng[result.m_Rango].m_Max;
    int iMinDamageRng1 = descDan.m_DisRng[result.m_Rango].m_Min;      
    int iMaxDamageRng2 = 0;
    int iMinDamageRng2 = 0;
    if (result.m_Rango != RNG_DIS3)
    {
      iMaxDamageRng2 = descDan.m_DisRng[result.m_Rango+1].m_Max;
      iMinDamageRng2 = descDan.m_DisRng[result.m_Rango+1].m_Min;
    }
#else
    int iMaxDamageRng2 = descDan.m_DisRng[result.m_Rango].m_Max;
    int iMinDamageRng2 = descDan.m_DisRng[result.m_Rango].m_Min;
    int iMaxDamageRng1 = iMaxDamageRng2;
    int iMinDamageRng1 = iMinDamageRng2;
    if (result.m_Rango != RNG_DIS1)
    {
      iMaxDamageRng1 = descDan.m_DisRng[result.m_Rango-1].m_Max;
      iMinDamageRng1 = descDan.m_DisRng[result.m_Rango-1].m_Min;
    }
#endif

    assert(iMaxDamageRng1 >= iMinDamageRng1);
    assert(iMaxDamageRng2 >= iMinDamageRng2);

    assert(iMaxDamageRng1 >= iMaxDamageRng2);
    assert(iMinDamageRng1 >= iMinDamageRng2);

    float fMaxDamage = static_cast<float>(iMaxDamageRng1 - iMaxDamageRng2) * (1.0f - fTRango) + static_cast<float>(iMaxDamageRng2);
    float fMinDamage = static_cast<float>(iMinDamageRng1 - iMinDamageRng2) * (1.0f - fTRango) + static_cast<float>(iMinDamageRng2);

    // **** PRECISION ****
    UINT precRnd = RANDOM_LOGICA_INT (0, g_MaxPrecision);
    if (precRnd > precision)
    {
      //                          errorPrec     (precRnd - precision)
      // factorErrorPrecPerOne = ------------ = ---------------------
      //                         maxErrorPrec     (100 - precision)
      //
      // UnidadesPerdidas = factorErrorPrecPerOne * rangoUnidades = factorError... * (MaxUnits-MinUnits)

      float fFalloPerOne = static_cast <float>(precRnd-precision) / static_cast <float>(g_MaxPrecision - precision);
      int falloUnidades = static_cast<int>((fMaxDamage - fMinDamage) * fFalloPerOne);
      danBase = static_cast<int>(fMaxDamage) - falloUnidades;
      if (danBase < static_cast<int>(fMinDamage))
        danBase = static_cast<int>(fMinDamage);
    }
    else
    {
      // Si saco menos de la precision, causo el maximo daño
      danBase = static_cast<int>(fMaxDamage);
    }

    int iDanRef = danBase;

    // Calculamos modificadores...
    modDan += datosAta->m_uModDanDis;     // Cogemos % de daño de  la tropa atacante
    modRes += datosDef->m_uResDis;        // Cogemos % la resistencia de la tropa defensora


    // Calculamos modificadores por formacion

    const TModifFormacion & modifFormAta = getModifForm (datosAta->m_EstadoFormacion, claseTropaAta, rangoTropaAta);
    const TModifFormacion & modifFormDef = getModifForm (datosDef->m_EstadoFormacion, claseTropaDef, rangoTropaDef);
    modDan += modifFormAta.m_DanDis;
    modRes += modifFormDef.m_ResDis;

    // Aplicamos escudo del edificio si la tropa esta apostada
    TModulo * pModulo = utilsTropa::edificioOcupado (pTropaDef);
    int iDefensaModulo = 0;

    if (pModulo)
    {
      // si el defensor esta apostado, aplicar escudo del módulo, teniendo en cuenta 
      // los flancos de defensa que proporciona la construcción:

      // obtener flanco defensor de la tropa atacante 
      // (usando como referencia la orientación del módulo):
      result.m_Flanco = utilsTropa::calcularFlancoApostado(pTropaDef, pTropaAta);
      const game::TModulo * pMod = utilsTropa::edificioOcupado (pTropaDef);
      TropaState_Apostar *pTSA=0;
      if (pTropaDef->estaApostada() && pTropaDef->getState()==TS_APOSTAR &&
        datosDef->m_pTropaAI!=0 && datosDef->m_pTropaAI->getDatosAITropa()!=0 &&
        datosDef->m_pTropaAI->getDatosAITropa()->m_State!=0 &&
        datosDef->m_pTropaAI->getDatosAITropa()->m_State->getStateID()==TS_APOSTAR)
      {
        pTSA=(TropaState_Apostar *)datosDef->m_pTropaAI->getDatosAITropa()->m_State;
      }
      if (pTSA!=0 && pTSA->getUbicSet()>=0 && pTSA->getUbicSet()<MAX_UBICSET)
      {
        DWORD modFlags=pMod->m_InfoFlancos[pTSA->getUbicSet()];
        // si los flancos cubren a la tropa, aplicamos el escudo del edificio:
        if ( (result.m_Flanco!=FLANCO_DELANTERO || (modFlags&F_FLANCO_DELANTERO_LIBRE)==0) && 
          (result.m_Flanco!=FLANCO_TRASERO   || (modFlags&F_FLANCO_TRASERO_LIBRE)==0) && 
          (result.m_Flanco!=FLANCO_IZQUIERDO || (modFlags&F_FLANCO_IZQUIERDO_LIBRE)==0) && 
          (result.m_Flanco!=FLANCO_DERECHO   || (modFlags&F_FLANCO_DERECHO_LIBRE)==0) )
        {
          // Calculamos los daños reales; teniendo en cuenta el escudo del edificio
          iDefensaModulo = ( (danBase * (pModulo->m_Escudo / 2)) / 100);
        }
      }

      danBase = danBase - iDefensaModulo;
    }




    // Calculamos modificador según el flanco de ataque : Si atacamos por un flanco; la resistencia de la 
    // tropa atacada se ve reducida a la mitad; si el ataque es por la retaguardia; la resistencia es 
    // ignorada. Si la tropa a la q atacamos esta en formación de cuadro o apostada no computamos los flancos
    int iModResPerdidoFlanco = 0;
    if ( !(datosDef->m_EstadoFormacion & (FORM_CUADRO | FORM_APOSTADO)) )
    {
      result.m_Flanco = utilsTropa::calcularFlanco (pTropaDef, pTropaAta);
      if (result.m_Flanco == FLANCO_DERECHO || result.m_Flanco == FLANCO_IZQUIERDO)
      {
        if(modRes>0)
        {
          iModResPerdidoFlanco = modRes / 2;
          modRes = modRes / 2;
        }
        // si ya es menor que 0 no hacemos nada
      }
      else if (result.m_Flanco == FLANCO_TRASERO)
      {
        if(modRes>0)
        {
          iModResPerdidoFlanco = modRes;
          modRes = 0;
        }
        // si ya es menor que 0 no hacemos nada
      }
    }


    // si el enemigo esta en Melee (no contra nosotros), los daños se reducen en un 70%, y el
    // amigo contra el que lucha no recibe daños:
    int iModGlobalPerdidoMelee = 0;
    if (datosDef->m_pTropaAI->getDatosAITropa()->m_State!=0)
    {
      bool bMelee=false;
      float fPorcentajeEnemigos = 0.0f;

      if (datosDef->m_pTropaAI->getDatosAITropa()->m_State->getStateID()==TS_MELEE)
      {
        TropaState_Melee *pTSM=(TropaState_Melee *)datosDef->m_pTropaAI->getDatosAITropa()->m_State;
        if (pTSM->luchando())
        {
          bMelee=pTSM->getTargetID()!=pTropaAta->getDatosTropa()->m_uIDTropa;

          SMeleeCombat* pCombat = pTSM->getCombat();
          int nUnidadesMias = pCombat->m_UnidadesAtacantes.size();
          int nUnidadesOtro = pCombat->m_UnidadesDefensores.size();
          if (pTropaAta->getDatosTropa()->m_TipoBando != TBANDO_ATACANTE)
            std::swap(nUnidadesMias, nUnidadesOtro);

          fPorcentajeEnemigos = (float) nUnidadesOtro / (float) nUnidadesMias;
        }
      }
      if (!bMelee)
      {
        if (datosDef->m_pTropaAI->getDatosAITropa()->m_State->getStateID()==TS_APOSTAR)
        {
          TropaState_Apostar *pTSA=(TropaState_Apostar *)datosDef->m_pTropaAI->getDatosAITropa()->m_State;
          bMelee=pTSA->enMelee();
          fPorcentajeEnemigos = 1.0f;
        }
      }
      if (bMelee)
      {
        float fMult = GetDisparoPerdidoMelee(fPorcentajeEnemigos);

        int iNuevoModDanGlobal = (int)((float)modDanGlobal*fMult);
        iModGlobalPerdidoMelee = modDanGlobal - iNuevoModDanGlobal;
        modDanGlobal=iNuevoModDanGlobal;
      }
    }



    // comprobamos oclusores amigos:
    // (solamente tenerlos en "cuenca" si no estamos más altos que el enemigo
    //  --diseño habla de pendiente-cuestarriba, que es lo que calcularemos)
    int iModGlobalPerdidoOclusoresAmigos = 0;

    {
      float tangente=atan2(datosDef->getHeight()-datosAta->getHeight(),(pTropaDef->getPos()-pTropaAta->getPos()).length());
      if (math::rad2deg(tangente)>=-15.f)
      {
        int iNuevoModDanGlobal = modDanGlobal;
        std::vector<CTropaBase *> oclusores;
        bool bPuedeHerir=utilsTropa::tropaPuedeHerirA(pTropaAta, pTropaDef, &oclusores);
        if (!bPuedeHerir) // no causamos daño:
          iNuevoModDanGlobal=0;
        else if (oclusores.size()==2) // 2 amigos entre enemigo y nosotros reducen 75% el daño sobre el enemigo
          iNuevoModDanGlobal=modDanGlobal*25/100;
        else if (oclusores.size()==1) // 1 amigo entre enemigo y nosotros reduce 50% el daño sobre el enemigo
          iNuevoModDanGlobal=modDanGlobal*50/100;
        else if (oclusores.size()!=0)
        {
          assert(oclusores.size()>2);
          int porc=100-min(100,(oclusores.size()-2)*100/10);
          porc=max(1,25*porc/100);
          iNuevoModDanGlobal=modDanGlobal*porc/100;
        }

        iModGlobalPerdidoOclusoresAmigos = modDanGlobal-iNuevoModDanGlobal;
        modDanGlobal = iNuevoModDanGlobal;

        result.m_NumOclusoresAmigos = oclusores.size();

#ifdef _INFODEBUG
        InfoDebugOclusores(m_pInfoDebug, pTropaAta, pTropaDef, oclusores, pOclusores!=0);
#endif
      }
    }

    // comprobamos oclusores enemigos:
    int iModGlobalPerdidoOclusoresEnemigos = 0;
    int nOclusoresEnemigos = 0;
    if (pOclusores!=0)
    {
      std::vector<utilsTropa::SOclusor> oclusores;
      int iNuevoModDanGlobal = modDanGlobal;

      utilsTropa::getOclusores(pTropaAta, pTropaDef, oclusores, datosDef->m_TipoBando);
      assert(oclusores.size()>0); // al menos debe estar el enemigo (o su oclusor completo)
      // el enemigo pHasta de existir nos lo devuelven colocado en oclusores.back() )
      if (oclusores.size()>0 && oclusores.back().m_pTropa!=pTropaDef) // no hacemos daño al enemigo, sino a otros:
      {
        iNuevoModDanGlobal=0;
      }
      else
      {
        if_assert(oclusores.size()>0)
        {
          iNuevoModDanGlobal=modDanGlobal*oclusores.back().m_Porc/100; // le causamos el porcentaje dado
          // oclusores.pop_back();
        }
      }

      iModGlobalPerdidoOclusoresEnemigos = modDanGlobal-iNuevoModDanGlobal;
      nOclusoresEnemigos = oclusores.size();
      modDanGlobal = iNuevoModDanGlobal;

      if (pOclusores!=0 && oclusores.size()>0)
      {
        pOclusores->resize(oclusores.size());
        std::copy(oclusores.begin(),oclusores.end(),pOclusores->begin());
        // no devolvemos la propia tropa atacada:
        if (pOclusores->size()>0 && pOclusores->back().m_pTropa==pTropaDef)
          pOclusores->pop_back();
      }

#ifdef _INFODEBUG
      InfoDebugOclusores2(m_pInfoDebug, pTropaAta, pTropaDef, oclusores, datosAta->m_TipoBando==TBANDO_ATACANTE);
#endif
    } // pOclusores


#if STAT_LOGGING
    if (pLogEntry)
    {
      int iPorcentaje = pPorcentajeImpuestoDano ? (*pPorcentajeImpuestoDano) : 100;
      iDanRef = danBase * iPorcentaje/100;
      (*pLogEntry) = Stat::g_StatLoggerBattle.NewEntry(Stat::TABLE_DISPARO);
      pLogEntry->Set(Stat::DISPARO_PARTIDA_ID,           g_idPartida);
      pLogEntry->Set(Stat::DISPARO_TROPA_ID,             pTropaAta->getDatosTropa()->m_uIDTropa & 0xFF);
      pLogEntry->Set(Stat::DISPARO_FORM_TROPA,           tipoFormacionToStr(datosAta->m_EstadoFormacion));
      pLogEntry->Set(Stat::DISPARO_UNIDADES_TROPA,       pTropaAta->getDatosTropa()->m_Unidades.size());
      pLogEntry->Set(Stat::DISPARO_ENEMIGO_ID,           pTropaDef->getDatosTropa()->m_uIDTropa & 0xFF);
      pLogEntry->Set(Stat::DISPARO_FORM_ENEMIGO,         tipoFormacionToStr(datosDef->m_EstadoFormacion));
      pLogEntry->Set(Stat::DISPARO_UNIDADES_ENEMIGO,     pTropaDef->getDatosTropa()->m_Unidades.size());
      pLogEntry->Set(Stat::DISPARO_TICK,                 game::g_currTick);
      pLogEntry->Set(Stat::DISPARO_DISTANCIA,            sqrtf(fSqDist));
      pLogEntry->Set(Stat::DISPARO_UNIDADES_DISPARAN,    unidadesAta);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_BASE_MIN,      (int) fMinDamage);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_BASE_MAX,      (int) fMaxDamage);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_BASE,          danBase+iDefensaModulo);
      pLogEntry->Set(Stat::DISPARO_EDIFICIO_ENEMIGO,     pModulo ? pModulo->m_Name.getPsz() : "");
      pLogEntry->Set(Stat::DISPARO_DEFENSA_EDIFICIO,     iDefensaModulo);
      pLogEntry->Set(Stat::DISPARO_PORCENTAJE,           iPorcentaje);

      pLogEntry->Set(Stat::DISPARO_MOD_DAMAGE,           datosAta->m_uModDanDis*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_MOD_FORM_TROPA,       modifFormAta.m_DanDis*iDanRef/100);

      iDanRef += iDanRef*modDan/100;

      pLogEntry->Set(Stat::DISPARO_MOD_DEFENSA,          datosDef->m_uResDis*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_MOD_FORM_ENEMIGO,     modifFormDef.m_ResDis*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_EXTRA_FLANCO,  iModResPerdidoFlanco*iDanRef/100);

      iDanRef -= iDanRef*modRes/100;

      pLogEntry->Set(Stat::DISPARO_DAMAGE_PERDIDO_MELEE,              iModGlobalPerdidoMelee*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_PERDIDO_OCLUSORES_AMIGOS,   iModGlobalPerdidoOclusoresAmigos*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_NUM_OCLUSORES_AMIGOS,              result.m_NumOclusoresAmigos);
      pLogEntry->Set(Stat::DISPARO_DAMAGE_PERDIDO_OCLUSORES_ENEMIGOS, iModGlobalPerdidoOclusoresEnemigos*iDanRef/100);
      pLogEntry->Set(Stat::DISPARO_NUM_OCLUSORES_ENEMIGOS,            nOclusoresEnemigos);

      iDanRef = iDanRef*modDanGlobal/100;
      pLogEntry->Set(Stat::DISPARO_DAMAGE, iDanRef);
    }
#endif
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------
// Nombre       : computarResultadoCarga
// Parámetros   : CTropaBase * pTropaAta, // Tropa que ataca
//                CTropaBase * pTropaDef, // Tropa que recibe
//                int unidadesAta,        // Unidades que cuentan como atacando
//                const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
//                Stat::CLogEntry* pLogEntry    // Log generado con el calculo. Puede ser 0
// Descripción  : computa el resultado del combate de distancia (más bien del ataque de distancia) representado
//              por los datos pasados.
//----------------------------------------------------------------------------------------------------------------
TResultadoAtaque svcModeloCombate::computarResultadoCarga    (CTropaBase * pTropaAta, // Tropa que ataca
                                                              CTropaBase * pTropaDef, // Tropa que recibe
                                                              int unidadesAta,        // Unidades que cuentan como atacando
                                                              const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                                              Stat::CLogEntry* pLogEntry,
                                                              int& danBase,                 // el daño base del ataque
                                                              int& modDan,                  // el modificador al daño
                                                              int& modRes,                  // el modificador a la resistencia
                                                              int& modDanGlobal) const    // Log generado con el calculo. Puede ser 0
{
  TResultadoAtaque result;

  // Obtenemos algunos datos antes de todo
  svcDatosTropa * datosAta = pTropaAta->getDatosTropa ();
  svcDatosTropa * datosDef = pTropaDef->getDatosTropa ();
  eClaseTropa claseTropaAta = datosAta->m_ClaseTropa;
  eClaseTropa claseTropaDef = datosDef->m_ClaseTropa;
  eTipoTropa tipoTropaAta = datosAta->m_TipoTropa;
  eTipoTropa tipoTropaDef = datosDef->m_TipoTropa;
  eRangoTropa rangoTropaAta = datosAta->m_Rango;
  eRangoTropa rangoTropaDef = datosDef->m_Rango;

  // Inicializamos la estructura a devolver
  result.m_Rango = RNG_OUT;
  result.m_TipoAtaque = TA_CARGA;
  result.m_Flanco = FLANCO_DELANTERO;
  result.m_bOclusion = false;
  result.m_Danos = 0;
  result.m_NumOclusoresAmigos = 0;

  assert ( (!pTropaAta->estaApostada ()) && (!pTropaDef->estaApostada ()) );

  // Alterado solo respecto al computo de ataque a distancia: 
  //    - Eliminado rango.
  //    - la precision de carga
  //    - Coger rango de daños de carga
  //    - Coger resistencia de carga
  //    - Aplicar modificador de formacion de carga

  UINT precision = datosAta->m_uPrecCar;

  // Calculamos el daño base segun el numero de soldados y la precision de la tropa; para ello lanzamos un numero
  // aleatorio; si este es <= q la precision de la tropa se aplica el maximo daño; en caso contrario restamos
  // un punto (100pv) por cada 10% excedido en la precision; sin pasar del minimo

  // si el número de unidades es 0, el resto de cosas tb son 0
  if(unidadesAta == 0)
  {
    danBase = 0;
    modDan = 0;
    modRes = 0;
    modDanGlobal = 0;
    return result;
  }

  const TDescDanTropa & descDan = getDescDan (claseTropaAta, unidadesAta, rangoTropaAta);
  danBase = descDan.m_Car.m_Max;
  UINT precRnd = RANDOM_LOGICA_INT (0, g_MaxPrecision);
  if (precRnd > precision)
  {
    //                          errorPrec     (precRnd - precision)
    // factorErrorPrecPerOne = ------------ = ---------------------
    //                         maxErrorPrec     (100 - precision)
    //
    // UnidadesPerdidas = factorErrorPrecPerOne * rangoUnidades = factorError... * (MaxUnits-MinUnits)

    float fFalloPerOne = static_cast <float>(precRnd-precision) / static_cast <float>(g_MaxPrecision - precision);
    int falloUnidades = static_cast <int> ( (descDan.m_Car.m_Max-descDan.m_Car.m_Min)*fFalloPerOne );
    danBase -= falloUnidades;
    if (danBase < descDan.m_Car.m_Min)
      danBase = descDan.m_Car.m_Min;
  }
  // Calculamos modificadores...
  modDan += datosAta->m_uModDanCar;     // Cogemos % de daño de  la tropa atacante
  modRes += datosDef->m_uResCar;        // Cogemos la resistencia de la tropa defensora

  // Calculamos modificadores por formacion
  const TModifFormacion & modifFormAta = getModifForm (datosAta->m_EstadoFormacion, claseTropaAta, rangoTropaAta);
  const TModifFormacion & modifFormDef = getModifForm (datosDef->m_EstadoFormacion, claseTropaDef, rangoTropaDef);
  modDan += modifFormAta.m_DanCar;
  modRes += modifFormDef.m_ResCar;

  // Calculamos modificador según el flanco de ataque : Si atacamos por un flanco; la resistencia de la 
  // tropa atacada se ve reducida a la mitad; si el ataque es por la retaguardia; la resistencia es 
  // ignorada. Si la tropa a la q atacamos esta en formación de cuadro o apostada no computamos los flancos
  int iModResPerdidoFlanco = 0;
  if ( !(datosDef->m_EstadoFormacion & (FORM_CUADRO | FORM_APOSTADO)) )
  {
    result.m_Flanco = utilsTropa::calcularFlanco (pTropaDef, pTropaAta);
    if (result.m_Flanco == FLANCO_DERECHO || result.m_Flanco == FLANCO_IZQUIERDO)
    {
      iModResPerdidoFlanco = modRes / 2;
      modRes = modRes / 2;
    }
    else if (result.m_Flanco == FLANCO_TRASERO)
    {
      iModResPerdidoFlanco = modRes-g_MinModifRes;
      modRes = g_MinModifRes;
    }
  }

#if STAT_LOGGING
  if (pLogEntry)
  {
    assert(!pPorcentajeImpuestoDano);
    (*pLogEntry) = Stat::g_StatLoggerBattle.NewEntry(Stat::TABLE_CARGA);
    pLogEntry->Set(Stat::CARGA_PARTIDA_ID,           g_idPartida);
    pLogEntry->Set(Stat::CARGA_TROPA_ID,             pTropaAta->getDatosTropa()->m_uIDTropa & 0xFF);
    pLogEntry->Set(Stat::CARGA_FORM_TROPA,           tipoFormacionToStr(datosAta->m_EstadoFormacion));
    pLogEntry->Set(Stat::CARGA_UNIDADES_TROPA,       pTropaAta->getDatosTropa()->m_Unidades.size());
    pLogEntry->Set(Stat::CARGA_ENEMIGO_ID,           pTropaDef->getDatosTropa()->m_uIDTropa & 0xFF);
    pLogEntry->Set(Stat::CARGA_FORM_ENEMIGO,         tipoFormacionToStr(datosDef->m_EstadoFormacion));
    pLogEntry->Set(Stat::CARGA_UNIDADES_ENEMIGO,     pTropaDef->getDatosTropa()->m_Unidades.size());
    pLogEntry->Set(Stat::CARGA_TICK,                 game::g_currTick);

    pLogEntry->Set(Stat::CARGA_DAMAGE_BASE_MIN,      descDan.m_Car.m_Min);
    pLogEntry->Set(Stat::CARGA_DAMAGE_BASE_MAX,      descDan.m_Car.m_Max);

    int iDanRef = danBase;
    pLogEntry->Set(Stat::CARGA_DAMAGE_BASE,          iDanRef);
    pLogEntry->Set(Stat::CARGA_MOD_DAMAGE,           datosAta->m_uModDanCar*iDanRef/100);
    pLogEntry->Set(Stat::CARGA_MOD_FORM_TROPA,       modifFormAta.m_DanCar*iDanRef/100);

    iDanRef += iDanRef*modDan/100;
    pLogEntry->Set(Stat::CARGA_MOD_DEFENSA,          datosDef->m_uResCar*iDanRef/100);
    pLogEntry->Set(Stat::CARGA_MOD_FORM_ENEMIGO,     modifFormDef.m_ResCar*iDanRef/100);
    pLogEntry->Set(Stat::CARGA_DAMAGE_EXTRA_FLANCO,  iModResPerdidoFlanco*iDanRef/100);

    iDanRef -= iDanRef*modRes/100;
    pLogEntry->Set(Stat::CARGA_DAMAGE, iDanRef);
  }
#endif

  return result;
}

//----------------------------------------------------------------------------------------------------------------
// Nombre       : computarResultadoDistancia
// Parámetros   : CTropaBase * pTropaAta, // Tropa que ataca
//                CTropaBase * pTropaDef, // Tropa que recibe
//                int unidadesAta,        // Unidades que cuentan como atacando
//                const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
//                Stat::CLogEntry* pLogEntry    // Log generado con el calculo. Puede ser 0
// Descripción  : computa el resultado del combate de distancia (más bien del ataque de distancia) representado
//              por los datos pasados.
//----------------------------------------------------------------------------------------------------------------
TResultadoAtaque svcModeloCombate::computarResultadoMelee    (CTropaBase * pTropaAta, // Tropa que ataca
                                                              CTropaBase * pTropaDef, // Tropa que recibe
                                                              int unidadesAta,        // Unidades que cuentan como atacando
                                                              const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                                              Stat::CLogEntry* pLogEntry,
                                                              int& danBase,                 // el daño base del ataque
                                                              int& modDan,                  // el modificador al daño
                                                              int& modRes,                  // el modificador a la resistencia
                                                              int& modDanGlobal) const    // Log generado con el calculo. Puede ser 0
{
  TResultadoAtaque result;

  // Obtenemos algunos datos antes de todo
  svcDatosTropa * datosAta = pTropaAta->getDatosTropa ();
  svcDatosTropa * datosDef = pTropaDef->getDatosTropa ();
  eClaseTropa claseTropaAta = datosAta->m_ClaseTropa;
  eClaseTropa claseTropaDef = datosDef->m_ClaseTropa;
  eTipoTropa tipoTropaAta = datosAta->m_TipoTropa;
  eTipoTropa tipoTropaDef = datosDef->m_TipoTropa;
  eRangoTropa rangoTropaAta = datosAta->m_Rango;
  eRangoTropa rangoTropaDef = datosDef->m_Rango;

  // Inicializamos la estructura a devolver
  result.m_Rango = RNG_OUT;
  result.m_TipoAtaque = TA_CUERPO;
  result.m_Flanco = FLANCO_DELANTERO;
  result.m_bOclusion = false;
  result.m_Danos = 0;
  result.m_NumOclusoresAmigos = 0;

  // Alterado solo respecto al computo de ataque a distancia: 
  //    - Eliminado rango.
  //    - la precision de cuerpo a cuerpo
  //    - Coger rango de daños de cuerpo a cuerpo
  //    - Coger resistencia de cuerpo a cuerpo
  //    - Aplicar modificador de formacion de cuerpo a cuerpo
  //    - No se tienen en cuenta los flancos de ataque

  UINT precision = datosAta->m_uPrecCue;

  // Calculamos el daño base segun el numero de soldados y la precision de la tropa; para ello lanzamos un numero
  // aleatorio; si este es <= q la precision de la tropa se aplica el maximo daño; en caso contrario restamos
  // un punto (100pv) por cada 10% excedido en la precision; sin pasar del minimo

  // si el número de unidades es 0, el resto de cosas tb son 0
  if(unidadesAta == 0)
  {
    danBase = 0;
    modDan = 0;
    modRes = 0;
    modDanGlobal = 0;
    return result;
  }

  int cueMin, cueMax;

  if (claseTropaAta==CT_ARTILLERIA)
  {
    cueMin=getInfoDanArtilleria(tipoTropaAta, rangoTropaAta).m_Cue.m_Min;
    cueMax=getInfoDanArtilleria(tipoTropaAta, rangoTropaAta).m_Cue.m_Max;
  }
  else
  {
    const TDescDanTropa & descDan = getDescDan (claseTropaAta, unidadesAta, rangoTropaAta);
    cueMin=descDan.m_Cue.m_Min;
    cueMax=descDan.m_Cue.m_Max;
  }

  danBase = cueMax; // descDan.m_Cue.m_Max;
  UINT precRnd = RANDOM_LOGICA_INT (0, g_MaxPrecision);
  if (precRnd > precision)
  {
    //                          errorPrec     (precRnd - precision)
    // factorErrorPrecPerOne = ------------ = ---------------------
    //                         maxErrorPrec     (100 - precision)
    //
    // UnidadesPerdidas = factorErrorPrecPerOne * rangoUnidades = factorError... * (MaxUnits-MinUnits)

    float fFalloPerOne = static_cast <float>(precRnd-precision) / static_cast <float>(g_MaxPrecision - precision);
    int falloUnidades = static_cast <int> ( (cueMax /*descDan.m_Cue.m_Max*/ - cueMin /*descDan.m_Cue.m_Min*/)*fFalloPerOne );
    danBase -= falloUnidades;
    if (danBase < cueMin) // descDan.m_Cue.m_Min)
      danBase = cueMin; // descDan.m_Cue.m_Min;
  }
  // Calculamos modificadores...
  modDan += datosAta->m_uModDanCue;     // Cogemos % de daño de  la tropa atacante
  modRes += datosDef->m_uResCue;        // Cogemos la resistencia de la tropa defensora

  // Calculamos modificadores por formacion
  const TModifFormacion & modifFormAta = getModifForm (datosAta->m_EstadoFormacion, claseTropaAta, rangoTropaAta);
  const TModifFormacion & modifFormDef = getModifForm (datosDef->m_EstadoFormacion, claseTropaDef, rangoTropaDef);
  modDan += modifFormAta.m_DanCue;
  modRes += modifFormDef.m_ResCue;

#if STAT_LOGGING
  if (pLogEntry)
  {
    assert(!pPorcentajeImpuestoDano);
    (*pLogEntry) = Stat::g_StatLoggerBattle.NewEntry(Stat::TABLE_MELEE);
    pLogEntry->Set(Stat::MELEE_PARTIDA_ID,           g_idPartida);
    pLogEntry->Set(Stat::MELEE_TROPA_ID,             pTropaAta->getDatosTropa()->m_uIDTropa & 0xFF);
    pLogEntry->Set(Stat::MELEE_FORM_TROPA,           tipoFormacionToStr(datosAta->m_EstadoFormacion));
    pLogEntry->Set(Stat::MELEE_UNIDADES_TROPA,       pTropaAta->getDatosTropa()->m_Unidades.size());
    pLogEntry->Set(Stat::MELEE_ENEMIGO_ID,           pTropaDef->getDatosTropa()->m_uIDTropa & 0xFF);
    pLogEntry->Set(Stat::MELEE_FORM_ENEMIGO,         tipoFormacionToStr(datosDef->m_EstadoFormacion));
    pLogEntry->Set(Stat::MELEE_UNIDADES_ENEMIGO,     pTropaDef->getDatosTropa()->m_Unidades.size());
    pLogEntry->Set(Stat::MELEE_TICK,                 game::g_currTick);

    pLogEntry->Set(Stat::MELEE_DAMAGE_BASE_MIN,      cueMin);
    pLogEntry->Set(Stat::MELEE_DAMAGE_BASE_MAX,      cueMax);

    int iDanRef = danBase;
    pLogEntry->Set(Stat::MELEE_DAMAGE_BASE,          iDanRef);
    pLogEntry->Set(Stat::MELEE_MOD_DAMAGE,           datosAta->m_uModDanCue*iDanRef/100);
    pLogEntry->Set(Stat::MELEE_MOD_FORM_TROPA,       modifFormAta.m_DanCue*iDanRef/100);

    iDanRef += iDanRef*modDan/100;
    pLogEntry->Set(Stat::MELEE_MOD_DEFENSA,          datosDef->m_uResCue*iDanRef/100);
    pLogEntry->Set(Stat::MELEE_MOD_FORM_ENEMIGO,     modifFormDef.m_ResCue*iDanRef/100);

    iDanRef -= iDanRef*modRes/100;
    pLogEntry->Set(Stat::MELEE_DAMAGE, iDanRef); // Lo voy a pisar mas abajo...
  }
#endif
  // TODO: C/C contra edificios!!

  result.m_Danos = danBase;

  return result;
}


//----------------------------------------------------------------------------------------------------------------
// findMaxArmadura
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxArmadura()
{
  // recorremos cada una de las tablas en busca del valor max
  UINT maxVal = 0, maxValInf = 0, maxValCab = 0, maxValArt = 0;
  
  maxVal = maxValInf = findMaxArmadura( *m_DescTropaInf, RANGO_COUNT, TI_COUNT );
  maxValCab = findMaxArmadura( *m_DescTropaCab, RANGO_COUNT, TC_COUNT );
  maxValArt = findMaxArmadura( *m_DescTropaArt, RANGO_COUNT, TA_COUNT );
  
  if ( maxValCab > maxVal ) {
    maxVal = maxValCab;
  }
  
  if ( maxValArt > maxVal ) {
    maxVal = maxValArt;
  }
  
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxArmadura - Extended!
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxArmadura( TDescTropa* descTropa, int arraySizeX, int arraySizeY )
{
  UINT maxVal = 0;
  
  for (UINT i = 0; i < RANGO_COUNT; ++i) {
    for (UINT j = 0; j < TI_COUNT; ++j) {
      
      descTropa[ (i * arraySizeY) + j ];
      TDescTropa& desc = descTropa[ (i * arraySizeY) + j ];
      
      UINT thisVal = getValorPromedioArmadura( desc.m_uResDis, desc.m_uResCue, desc.m_uResCar );
      if ( thisVal > maxVal ) {
        maxVal = thisVal;
      }
    } 
  }
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxAtaqueADist
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueADist()
{
  // recorremos cada una de las tablas en busca del valor max
  UINT maxVal = 0, maxValInf = 0, maxValCab = 0;
  
  maxVal = maxValInf = findMaxAtaqueADist( m_DescDanTropaInf, CT_INFANTERIA );
  maxValCab = findMaxAtaqueADist( m_DescDanTropaCab, CT_CABALLERIA );
  
  if ( maxValCab > maxVal ) {
    maxVal = maxValCab;
  }
  
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxAtaqueADist - Extended!
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueADist( const std::vector<TDescDanTropa> descDan[RANGO_COUNT], eClaseTropa claseTropa )
{
  UINT maxVal = 0;
  
  for (UINT i = 0; i < RANGO_COUNT; ++i) {
    for (UINT j = 0; j < descDan[i].size(); ++j) {
    
      const TDescDanTropa& desc = descDan[i][j];
      UINT thisVal = getValorPromedioAtaqueADist( desc, 1.0f, 1.0f, 1.0f, 1.0f, claseTropa, (eRangoTropa)i );
      if ( thisVal > maxVal ) {
        maxVal = thisVal;
      }
    } 
  }
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxAtaqueCAC
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueCAC()
{
  // recorremos cada una de las tablas en busca del valor max
  UINT maxVal = 0, maxValInf = 0, maxValCab = 0;
  
  maxVal = maxValInf = findMaxAtaqueCAC( m_DescDanTropaInf, CT_INFANTERIA );
  maxValCab = findMaxAtaqueCAC( m_DescDanTropaCab, CT_CABALLERIA );
  
  if ( maxValCab > maxVal ) {
    maxVal = maxValCab;
  }
  
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxAtaqueCAC - Extended!
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueCAC( const std::vector<TDescDanTropa> descDan[RANGO_COUNT], eClaseTropa claseTropa )
{
  UINT maxVal = 0;
  
  for (UINT i = 0; i < RANGO_COUNT; ++i) {
    for (UINT j = 0; j < descDan[i].size(); ++j) {
    
      const TDescDanTropa& desc = descDan[i][j];
      UINT thisVal = getValorPromedioAtaqueCAC( desc, 1.0f, 1.0f, claseTropa, (eRangoTropa)i );
      if ( thisVal > maxVal ) {
        maxVal = thisVal;
      }
    } 
  }
  return maxVal;
}

//---------------------------------------------------------------------------------------------------------------
// findMaxAtaqueADistArt
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueADistArt()
{
  // recorremos cada una de las tablas en busca del valor max
  UINT maxVal = 0;
  
  maxVal = findMaxAtaqueADistArt( m_InfoDanArtilleria );
  
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// findMaxAtaqueADist - Extended!
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::findMaxAtaqueADistArt( const std::vector<TInfoDanArtilleria> descDan[RANGO_COUNT] )
{
  UINT maxVal = 0;
  
  for (UINT i = 0; i < RANGO_COUNT; ++i) {
    for (UINT j = 0; j < descDan[i].size(); ++j) {
    
      const TInfoDanArtilleria& desc = descDan[i][j];
      
      UINT thisVal = getValorPromedioAtaqueADistArt( desc, 1.0f, 1.0f, 1.0f );
      if ( thisVal > maxVal ) {
        maxVal = thisVal;
      }
    } 
  }
  return maxVal;
}

//----------------------------------------------------------------------------------------------------------------
// getValorPromedioArmadura. Probamos suma de valores...podríamos sacar el promedio real
//----------------------------------------------------------------------------------------------------------------
UINT svcModeloCombate::getValorPromedioArmadura( UINT resDis, UINT resCue, UINT resCar )
{
  UINT prom = resDis + resCue + resCar;
  return prom;
}

UINT svcModeloCombate::getValorPromedioAtaqueADist( const TDescDanTropa& descDan, float prec1, float prec2, float prec3, 
                                                    float modDan, eClaseTropa claseTropa, eRangoTropa rangoTropa )
{
  int promDan = (int)( ( descDan.m_DisRng1.m_Max + descDan.m_DisRng2.m_Max + descDan.m_DisRng3.m_Max ) / 3 );
  float promPre = float( ( prec1 + prec2 + prec3 ) / 3 );
  
  // Nuevo: tenemos en cuenta tambien estos modif. para el cálculo
  TModifFormacion modFormLinea = getModifForm( FORM_LINEA_SIMPLE, claseTropa, rangoTropa );
  
  promDan += modFormLinea.m_DanDis;
  
  if ( promPre == 0.0f && promDan > 0 ) {
    promPre = 0.1f;     // para que no den valores de 0 cuando la precision es baja
  }
  
  // para prec 1.0f, dan == max
  int prom = int( float(promDan) * promPre + ( float(promDan) * modDan ) );

  if ( prom < 0 ) {
    prom = 0;
  }
  
  return prom;
}

UINT svcModeloCombate::getValorPromedioAtaqueADistArt( const TInfoDanArtilleria& descDan, float prec1, float prec2, float prec3 )
{
  UINT promDan = UINT( ( descDan.m_DamageRng1 + descDan.m_DamageRng2 + descDan.m_DamageRng3 ) / 3 );
  float promPre = float( ( prec1 + prec2 + prec3 ) / 3 );
  
  if ( promPre == 0.0f && promDan > 0 ) {
    promPre = 0.1f;     // para que no den valores de 0 cuando la precision es baja
  }
  
  // para prec 1.0f, dan == max
  UINT prom = UINT( float(promDan) * promPre );

  return prom;
}

UINT svcModeloCombate::getValorPromedioAtaqueCAC(const TDescDanTropa& descDan, float prec, float modDan, 
                                                  eClaseTropa claseTropa, eRangoTropa rangoTropa)
{
  int promDan = descDan.m_Cue.m_Max;
  
  // Nuevo: tenemos en cuenta tambien estos modif. para el cálculo
  TModifFormacion modFormLinea = getModifForm( FORM_LINEA_SIMPLE, claseTropa, rangoTropa );
  TModifFormacion modFormMelee = getModifForm( FORM_MELEE, claseTropa, rangoTropa );
  
  promDan += modFormLinea.m_DanCue;
  promDan += modFormMelee.m_DanCue;
  
  if ( prec == 0.0f && promDan > 0 ) {
    prec = 0.1f;     // para que no den valores de 0 cuando la precision es baja
  }
  
  // para prec 1.0f, dan == max
  int prom = int( float(promDan) * prec + (float(promDan) * modDan) );

  if ( prom < 0 ) {     // Cuidadin con esto. Puede no interesarnos siempre, dependerá del uso q hagamos de esta función
    prom = 0;
  }

  return prom;
}

const TInfoDanArtilleria &  svcModeloCombate::getInfoDanArtilleria(eTipoTropa tipo, eRangoTropa rango) const 
{ 
  eTipoArtilleria tipoArtilleria = getTipoTropaArt( tipo );
  return m_InfoDanArtilleria[rango][tipoArtilleria];
}


INIT_SERIALIZATION_HIERARCHY_FATHER (svcModeloCombate, SS_LOGIC)
  SERIALIZE (m_FileName);
  SERIALIZE (m_DescTropaInf);
  SERIALIZE (m_DescTropaCab);
  SERIALIZE (m_DescTropaArt);
  
  SERIALIZE (m_DescDanTropaInf);
  SERIALIZE (m_DescDanTropaCab);
  
  SERIALIZE (m_ModifFormacionInf);
  SERIALIZE (m_ModifFormacionCab);
  SERIALIZE (m_ModifTerreno);
  
  SERIALIZE (m_CargasVsCuadroInf);
  SERIALIZE (m_CargasVsCuadroCab);
  SERIALIZE (m_CargasPropiedades);
  
  SERIALIZE (m_InfoDanArtilleria);
  SERIALIZE (m_MaxValArmadura);
  SERIALIZE (m_MaxValAtaqueADist);
  SERIALIZE (m_MaxValAtaqueADistArt);
  SERIALIZE (m_MaxValAtaqueCAC);
  
  SERIALIZE (m_bInvulnerability);
  
#ifdef _INFODEBUG
  SERIALIZE_SKIP(m_pInfoDebug);
  if (LOADING_OK)
    m_pInfoDebug=0;
#endif

END_SERIALIZATION_HIERARCHY_FATHER (svcModeloCombate, SS_LOGIC)

} // end namespace game
