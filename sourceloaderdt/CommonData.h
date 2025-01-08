#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: CommonData.h
// Date: 07/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// En este fichero estan todas las estructuras de datos, enumeraciones, clases, .... 
// que son comunes al Napoleon y a otra aplicacion como el editor; exportador
//----------------------------------------------------------------------------------------------------------------

#ifndef __COMMONDATA_H__
#define __COMMONDATA_H__

    #include "util/assert_pro.h"
    #include "math/Vector2.h"
    #include "util/HSTR.h"
    #include "util/serialize.h"
    #include "X3DEngine/Material.h"
    #include "Util/SalvableItem.h"
#include "File/IFile.h"
    
enum eAnimation;

namespace game 
{
enum eTipoRiqueza;
enum eClaseTropa;
enum eTipoTropa;
enum eTipoBarco;
enum eRangoTropa;
enum eTipoTripulacion;
enum eTipoConstruccion;
enum eTipoEdificio;
enum eTipoArea;
enum eTipoBando;
enum eMaterialPais;

// Constante que indica la carpeta de grabado del juego
#define SAVE_FOLDER L"Imperial Glory Savegames"

// Constante que indica el máximo número de caracteres que puede tener un nombre del juego
#define MAX_NAMESIZE 64

// Estructura referente a los diferentes tipos de Riquezas de un pais en cuanto a valores netos.
#pragma pack (push, 4)
struct sRiquezas : SALVABLE, MAKE_SERIALIZABLE
{
	INCLUDE_INTERFACE;
	
	// Vbles asociadas a los recursos existentes
	int iDinero;          // Dinero
	int iMatPrimas;       // Materias primas 
	int iComida;		      // Comida
	int iPoblacion;       // Poblacion
	int iCiencia;         // Ciencia
  int iValorComercial;  // Valor comercial

	sRiquezas (void);
  sRiquezas (int _iDinero, int _iMatPrimas, int _iComida, int _iPoblacion, int _iCiencia, int _iValorComercial);
  sRiquezas (const sRiquezas& _riquezas);
  
  bool operator==(const sRiquezas& _riquezas) const;
  bool operator<=(const sRiquezas& _riquezas) const;
  bool operator>=(const sRiquezas& _riquezas) const;
  bool operator<(const sRiquezas& _riquezas)  const;
  bool operator>(const sRiquezas& _riquezas)  const;

  sRiquezas operator+(const sRiquezas& _riqueza) const;
  sRiquezas operator-(const sRiquezas& _riqueza) const;
  sRiquezas operator*(float _fFactor) const;

  void operator+=(const sRiquezas& _riquezas);
  void operator-=(const sRiquezas& _riquezas);
  void operator*=(const sRiquezas& _riquezas);
  
  void operator*= (UINT _factor);
  sRiquezas operator/ (UINT _divisor);

  void operator*=(float _factor);

  void operator=(const sRiquezas& _riquezas);

  bool tieneRiqueza       (eTipoRiqueza _tipo) const;
  bool tieneAlgunaRiqueza (void) const;

  bool save (fileSYS::IFile* _file) const;
  bool load (fileSYS::IFile* _file);

  void reset (void);

  void acotaPorMinimo (eTipoRiqueza _riqueza, int iValorMinimo = 0);
};
#pragma pack (pop)

// Estructura referente a los diferentes tipos de Riquezas de un pais en cuanto a porcentajes.
#pragma pack (push, 4)

enum eTipoPorcentajeRiquezas
{
  TPR_NULL = -1,
  TPR_DINERO = 0,
  TPR_MATPRIMAS,
  TPR_COMIDA,
  TPR_POBLACION,
  TPR_CIENCIA,

  TPR_COUNT
};

struct sPorcentajeRiquezas : SALVABLE, MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;

  // Porcentajes asociados a las riquezas existentes
  union
  {
    struct  
    {
      float fDinero;     // Dinero
      float fMatPrimas;  // Materias primas 
      float fComida;		 // Comida
      float fPoblacion;  // Poblacion
      float fCiencia;    // Ciencia
    };
    float vRiqueza[TPR_COUNT];
  };
  
  sPorcentajeRiquezas  (void) { reset(); }
  
  void reset           (void);

  bool save (fileSYS::IFile* _file) const;
  bool load (fileSYS::IFile* _file);
};
#pragma pack (pop)

// Estructura para salvar a fichero los datos de una tropa; no es una estructura de juego
// IMPORTANTE: No modificar esta estructura; ya q es una estructura q se salva a disco desde el editor
#pragma pack (push, 4)
struct TDescTropa : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  
  UINT m_uValTropa;              // Valia de la tropa
  UINT m_uUnidades;              // Numero de unidades de la tropa
  int  m_VidaTotal;              // Vida total de la tropa = Numero de unidades totales * VIDA_UNIDAD (100)
  int  m_VidaActual;             // Vida actual de la tropa = Numero de unidades vivias * VIDA_UNIDAD (100)
  
  UINT m_uResDis;                // Resistencia (%) al ataque a distancia
  UINT m_uResCar;                // Resistencia (%) a la carga
  UINT m_uResCue;                // Resistencia (%) al ataque cuerpo a cuerpo
  
  UINT m_uTieRec;                // Tiempo de recarga
  
  float m_fRngDet;               // Rango de deteccion de la tropa
  union {
    struct {
      float           m_fRngDis1;                 // Rango de ataque a distancia 1
      float           m_fRngDis2;                 // Rango de ataque a distancia 2
      float           m_fRngDis3;                 // Rango de ataque a distancia 3
    };
    float m_fRngDis [3];                          // Rangos de ataque a distancia
  };
  
  union {
    struct {      
      UINT m_uPrecDis1;              // Precision de disparo (%) en rango 1
      UINT m_uPrecDis2;              // Precision de disparo (%) en rango 2
      UINT m_uPrecDis3;              // Precision de disparo (%) en rango 3
    };
    UINT m_uPrecDis [3];             // Precision de disparo (%) en cada rango
  };
  UINT m_uPrecCue;               // Precision del ataque cuerpo a cuerpo
  UINT m_uPrecCar;               // Precision de cargas
  
  float m_fVelNor;               // Velocidad de desplazamiento normal
  float m_fVelForColumna;        // Vel de form col. Mas abajo colocamos el resto de las formaciones
  float m_fVelCor;               // Velocidad de correr
  float m_fVelCar;               // Velocidad de cargar
  
  CVector2 m_SepNormal;         // Separacion normal de las unidades
  CVector2 m_SepExpandida;      // Separacion expandida de las unidades
  
  UINT    m_unused;                                 // (m_uPasabilidad) Grado de pasabilidad
  UINT    m_uFlagsAccionesDisponiblesIniciales;     // Flags de iniciales acciones disponibles por la tropa
  UINT    m_uFlagsAccionesDisponibles;              // Flags de máximas acciones disponibles por la tropa

  // Nuevas variables. No cambiar el orden. Tienen que mantenerse así para que siga habiendo compatibilidad
  
  UINT m_uModDanDis;             // Mod. daños
  UINT m_uModDanCar;             // Mod. daños
  UINT m_uModDanCue;             // Mod. daños
  
  float m_fVelForLinea;          // Velocidad de formacion
  float m_fVelForCuadro;         // Velocidad de formacion
  
  float m_fGastoResistPorSegundo;       // ptos de resist gastados en un segundo
  float m_fGananciaResistPorSegundo;    // ptos de resist ganados en un segundo
  float m_fDuracionFatiga;              // segundos
  float m_fPenalizacionMelee;           // porcentaje
    
  
  float m_Placeholder[7];       // reservamos floats para que podamos añadir variables en un futuro
  
  TDescTropa (void);
};
#pragma pack (pop)

// Esctructura q describe los daños q aplica una tropa en funcion de un numero de soldados concretos y 
// segun la precision
// IMPORTANTE: No modificar esta estructura; ya q es una estructura q se salva a disco desde el editor

#pragma pack (push, 4)
struct TDescDanTropa : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  
  struct TRango : MAKE_SERIALIZABLE
  {
    INCLUDE_INTERFACE;
    int m_Min;
    int m_Max;
  };
  
  int m_Soldados;                   // Maximo de soldados q producen los siguientes rangos de daño
  
  union {
    struct {
      TRango m_DisRng1;                 // Rango de daño en ataque a distancia rango 1
      TRango m_DisRng2;                 // Rango de daño en ataque a distancia rango 2
      TRango m_DisRng3;                 // Rango de daño en ataque a distancia rango 3
    };
    TRango m_DisRng [3];
  };
  TRango m_Cue;                     // Rango de daño en ataque cuerpo a cuerpo
  TRango m_Car;
  
  float m_Placeholder[16];          // reservamos floats para que podamos añadir variables en un futuro
  
  TDescDanTropa (void);
};
#pragma pack (pop)

// Daño de las balas de artilleria

#pragma pack (push, 4)
struct TInfoDanArtilleria : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  
  struct TRango : MAKE_SERIALIZABLE
  {
    INCLUDE_INTERFACE;
    int m_Min;
    int m_Max;
  };

  UINT      m_DamageRng1;         // daño
  UINT      m_DamageRng2;         // daño
  UINT      m_DamageRng3;         // daño
  
  float     m_PrecisionVsEdif;    // de 0 a 1 -> 1.0 = muy preciso

  TRango m_Cue;                     // Rango de daño en ataque cuerpo a cuerpo

  float m_Placeholder[14];          // reservamos floats para que podamos añadir variables en un futuro
  
  TInfoDanArtilleria (void);
};
#pragma pack (pop)

// Esctructura q describe los modificadores aplicados al combate en funcion de la formacion
// tanto del atacante (daños) como del defensor (Resistencia)
// IMPORTANTE: No modificar esta estructura; ya q es una estructura q se salva a disco desde el editor

#pragma pack (push, 4)
struct TModifFormacion : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  int m_ResDis;                 // Modificador (%) de Resistencia al ataque a distancia
  int m_ResCar;                 // Modificador (%) de Resistencia a la carga
  int m_ResCue;                 // Modificador (%) de Resistencia al ataque cuerpo a cuerpo
  
  int m_DanDis;                 // Modificador de daño al ataque a distancia
  int m_DanCar;                 // Modificador de daño a la carga
  int m_DanCue;                 // Modificador de daño al ataque cuerpo a cuerpo
  
  int m_Vel;                    // Modificador de velocidad
  
  float m_Placeholder[16];      // reservamos floats para que podamos añadir variables en un futuro
  
  static TModifFormacion s_Neutro;
  TModifFormacion ()
  {
    m_ResDis = m_ResCar = m_ResCue = 0;
    m_DanDis = m_DanCar = m_DanCue = 0;  
    m_Vel = 0;
  }
};
#pragma pack (pop)

#pragma pack (push, 4)
struct TModifTerreno : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  static TModifTerreno s_Neutro;
  
  float       m_Vel [3];                        // Modificador de velocidad unitario ( -1, +1) (+-) Inf/Cab/Art
  DWORD       m_AccionesNoDisponibles [3];      // Acciones no disponibles (maskara de flags) Inf/Cab/Art
  float       m_RngDet [3];                     // Rango de deteccion cm absolutos (+-) Inf/Cab/Art
  int         m_ResDis;                         // Modificador de Resistencia (%) al ataque a distancia
  eAnimation  m_AniAndar;                       // Animación utilizada para caminar en este terreno
  
  // Constructor
  TModifTerreno (void);
  
};
#pragma pack (pop)

struct TCargasVsCuadro : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  
  // multiplica al numero de unidades defensoras (en cuadro)
  float m_MultVsNovata;
  float m_MultVsMedia;
  float m_MultVsExperimentada;
  float m_MultVsVeterana;
  float m_MultVsElite;
  
  TCargasVsCuadro() 
  {
    m_MultVsNovata = m_MultVsMedia = m_MultVsExperimentada = m_MultVsVeterana = m_MultVsElite = 0.0f;
  }
};

// Propiedades varias de la carga
struct TCargasPropiedades : MAKE_SERIALIZABLE
{
  INCLUDE_INTERFACE;
  
  float m_DistMin;
  float m_DistMax;
  float m_MultNumUnidadesReq_AtaqueDist;    // multiplicador del número de unidades requeridas para romper un cuadro que ataca a distancia :)
  float m_MultNumUnidadesReq_Carga;         // multiplicador del número de unidades requeridas para romper un cuadro que ya se encuentra recibiendo una carga :)
  float m_Placeholder[16];      // reservamos floats para que podamos añadir variables en un futuro
  
  TCargasPropiedades() 
  {
    m_DistMin = m_DistMax = 0.0f;
    m_MultNumUnidadesReq_AtaqueDist = 0.0f;
    m_MultNumUnidadesReq_Carga = 0.0f;
  }
};

typedef std::vector<UINT> TRutaProvincias;

//----------------------------------------------------------------------------------------------------------------
// Conversiones de datos ...
//----------------------------------------------------------------------------------------------------------------

eClaseTropa str2claseTropa (const util::HSTR& hTipo);
util::HSTR claseTropa2str (eClaseTropa ctropa);

eTipoTropa str2tipoTropa (const util::HSTR & _hTipo);
util::HSTR tipoTropa2str (eTipoTropa ttropa);
const char* tipoTropa2psz(eTipoTropa ttropa);

eTipoBarco str2tipoBarco (const util::HSTR& _hTipo);
util::HSTR tipoBarco2str (eTipoBarco tbarco); 

eTipoConstruccion str2tipoConstruccion (const util::HSTR& _hTipo);
util::HSTR        tipoConstruccion2str (eTipoConstruccion _tconstruccion);

eTipoEdificio     str2tipoEdificio(const util::HSTR& _hTipo);
util::HSTR        tipoEdificio2str(eTipoEdificio tipo);

eTipoArea     str2tipoArea(const util::HSTR& _hTipo);
util::HSTR    tipoArea2str(eTipoArea tipo);

eTipoBando    str2tipoBando(const util::HSTR& _hTipo);
util::HSTR    tipoBando2str(eTipoBando tipo);

eRangoTropa str2rangoTropa  (const util::HSTR& _hTipo);
util::HSTR  rangoTropa2str  (eRangoTropa _rtropa);

eTipoTripulacion str2tipoTripulacion (const util::HSTR& _hTipo);
util::HSTR       tipoTripulacion2str (eTipoTripulacion _ttripulacion);

e3d::eMtlMode materialPais2MtlMode (eMaterialPais _material);
eMaterialPais mtlMode2MaterialPais (e3d::eMtlMode _material);

util::HSTR    materialPais2str     (eMaterialPais _material);
eMaterialPais str2MaterialPais     (const util::HSTR& _hMaterial);


//----------------------------------------------------------------------------------------------------------------
// Efectos
//----------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
// Enumeracion indicando los tipos de efectos
// NOTES: (IMPORTANTE!!) Añadir solo efectos al final de la lista! ya que estos ID's se guardan en fichero!
//        aunque sean de modelos diferentes!!
// NOTES: Es importante q el efecto NULO sea el 0!
//
// IMPORTANTE: No olvidar modificar en el 'CommonData.cpp' el array g_DeclEfectos
//-------------------------------------------------------------------------------------------
enum eEffect
{
  EFFECT_NULL       = 0,
  
  EFFECT_IMPACTO_CANON_EDIFICIO,                  // Impacto de las balas de cañon sobre los edificios
  EFFECT_IMPACTO_MORTERO_EDIFICIO,                // Impacto de las balas de cañon sobre los edificios
  EFFECT_IMPACTO_OBUS_EDIFICIO,                   // Impacto de las balas de cañon sobre los edificios

  EFFECT_DESTRUCCION_EDIFICIO,
  EFFECT_HUMO_EDIFICIO,
  
  EFFECT_HUMO_CORRER,                             // humo creado cuando corre una tropa

  EFFECT_HUMO_MELEE_APOSTADO,                     // humo creado cuando dos unidades se pegan apostadas

  EFFECT_HUMO_ANDAR_INFANTERIA,                   // humo creado al mover infanteria andando
  EFFECT_HUMO_ANDAR_INFANTERIA_DESIERTO,          // humo creado al mover infanteria por terreno desierto andando
  EFFECT_HUMO_ANDAR_INFANTERIA_MEDITERRANEO,      // humo creado al mover infanteria por terreno mediterraneo andando
  EFFECT_HUMO_ANDAR_INFANTERIA_NEVADO,            // humo creado al mover infanteria por terreno nevado andando

  EFFECT_HUMO_MOVER_INFANTERIA,                   // humo creado al mover infanteria
  EFFECT_HUMO_MOVER_INFANTERIA_DESIERTO,          // humo creado al mover infanteria por terreno desierto
  EFFECT_HUMO_MOVER_INFANTERIA_MEDITERRANEO,      // humo creado al mover infanteria por terreno mediterraneo
  EFFECT_HUMO_MOVER_INFANTERIA_NEVADO,            // humo creado al mover infanteria por terreno nevado

  EFFECT_HUMO_MOVER_CABALLERIA,                   // humo creado al mover caballeria
  EFFECT_HUMO_MOVER_ARTILLERIA,                   // humo creado al mover artilleria
  
  // Disparos de la artilleria
  EFFECT_DISPARO_CANON1,                  // Efecto para cuando disparan los cañones (variante 1)
  EFFECT_DISPARO_CANON2,                  // Efecto para cuando disparan los cañones (variante 2)
  EFFECT_DISPARO_CANON3,                  // Efecto para cuando disparan los cañones (variante 3)
  EFFECT_DISPARO_MORTERO,                 // Efecto para cuando disparan los morteros
  
  // Disparos de la infanteria
  EFFECT_DISPARO_INF_DEPIE1,              // Efecto para cuando dispara la infanteria de pie (variante 1/3)
  EFFECT_DISPARO_INF_DEPIE2,              // Efecto para cuando dispara la infanteria de pie (variante 2/3)
  EFFECT_DISPARO_INF_DEPIE3,              // Efecto para cuando dispara la infanteria de pie (variante 3/3)
  EFFECT_DISPARO_INF_RODILLA1,            // Efecto para cuando dispara la infanteria a rodilla (variante 1/3)
  EFFECT_DISPARO_INF_RODILLA2,            // Efecto para cuando dispara la infanteria a rodilla (variante 2/3)
  EFFECT_DISPARO_INF_RODILLA3,            // Efecto para cuando dispara la infanteria a rodilla (variante 3/3)

  EFFECT_DISPARO_CAB1,                    // Efecto para cab
  EFFECT_DISPARO_CAB2,                    // Efecto para cab
  EFFECT_DISPARO_CAB3,                    // Efecto para cab
  
  // Impactos en el suelo por la artilleria

  EFFECT_IMPACTO_CANON_SUELOA,            // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_CANON_SUELOB,            // Impacto de las balas de cañon sobre el terreno (2/3)
  EFFECT_IMPACTO_CANON_SUELOC,            // Impacto de las balas de cañon sobre el terreno (3/3)

  EFFECT_IMPACTO_CANON_CAMP_SUELOA,       // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_CANON_CAMP_SUELOB,       // Impacto de las balas de cañon sobre el terreno (2/3)
  EFFECT_IMPACTO_CANON_CAMP_SUELOC,       // Impacto de las balas de cañon sobre el terreno (3/3)

  EFFECT_IMPACTO_MORTERO_SUELOA,          // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_MORTERO_SUELOB,          // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_MORTERO_SUELOC,          // Impacto de las balas de cañon sobre el terreno (1/3)

  EFFECT_IMPACTO_OBUS_SUELOA,             // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_OBUS_SUELOB,             // Impacto de las balas de cañon sobre el terreno (1/3)
  EFFECT_IMPACTO_OBUS_SUELOC,             // Impacto de las balas de cañon sobre el terreno (1/3)
  
  EFFECT_TRAIL_CANON,                     // Trail de las balan de cañon

  EFFECT_ROMPIENTE,                       // Efecto de rompiente con el mar en la proa del barco
  
  EFFECT_IMPACTO_METRALLA,                // Efecto de impacto de la metralla en el barco
  EFFECT_IMPACTO_FUEGO,                   // Efecto de impacto de la metralla en el barco
  EFFECT_IMPACTO_BOMBA,                   // Efecto de impacto de la metralla en el barco

  EFFECT_IMPACTO_AGUA,                    // Efecto de impacto en el agua
  EFFECT_IMPACTO_AGUA_MODELOS,            // Efecto de impacto en el agua (modelos v3d)

  EFFECT_HUNDIMIENTO,                     // Efecto de hundimiento
  EFFECT_FUEGO_CUBIERTA,                  // Efecto de fuego en la cubierta como resultado del daño recibido
  EFFECT_DISPARO_SALVA,                   // Efecto de disparo de un cañon

  EFFECT_ABORDAJE1,                       // Efecto de humo en cubierta
  EFFECT_ABORDAJE2,                       // Efecto de humo en cubierta
  EFFECT_ABORDAJE_HUMO,                   // Efecto de humaredas aleatorias
  EFFECT_ABORDAJE_DISPAROS,               // Efecto de fogonazos aleatorios

  EFFECT_DISPARO_METRALLA,                // efecto de disparo de un cañon con municion de metralla
  EFFECT_DISPARO_FUEGO,                   // efecto de disparo de un cañon con municion de metralla
  
  // ----------------------------DESTRUCCION CAPITALES ---------------------------------------------------------
  EFFECT_DESTRUCCION_BRANDENBURGO,        // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LONDRES01,           // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LONDRES02,           // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LONDRES03,           // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_AUSTRIA,             // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_PARIS,               // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LETONIA01,           // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LETONIA02,           // Efecto de destruccion : Capital
  EFFECT_DESTRUCCION_LETONIA03,           // Efecto de destruccion : Capital
  // ------------------------------------------------------------------------------------------------------------

  EFFECT_COUNT,                           // Contador de efectos
  EFFECT_FORCEDWORD = 0x7fffffff,
};

// Flags de un efecto
enum eEffectFlags
{
  EF_NONE           =    0,
  EF_PRELOADED      =  0x1,         // El efecto será precargado en memoria
  EF_STATIC         =  0x2,         // El efecto no se descargará de memoria una vez creado
  
  EF_FORCEDWORD     = 0x7fffffff,   // Enumeración de 32 bits
};

// Modelos de juego definidos (los utilizan los efectos para determinar q se debe cargar)
enum eGameModel
{
  GM_NULL,                              // Invalido!
  GM_MENU,                              // Modelo de menús
  GM_MANAGEMENT,                        // Modelo de gestión
  GM_BATTLE,                            // Modelo de batalla
  GM_NAVAL,                             // Modelo naval
};

// Declaración de un efecto
struct TDeclEffect
{
  std::string name;                     // Nombre del efecto : Debe existir dentro del script
  eGameModel  model;                    // Modelo de juego
  DWORD       flags;                    // Flags del efecto (eFlagsEffect)
};

// Declaración de la lista de efectos soportados y sus características 
// NOTES: (No especificamos tamaño del array ya que hay un STATIC_ASSERT controlando que encaje
extern TDeclEffect g_DeclEfectos [];

};   // end namespace game
#endif
