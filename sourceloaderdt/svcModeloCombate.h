#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcModeloCombate.h
// Date: 07/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Almacena todos los datos del modelo de combate de tropas; en si es un contenedor de datos
//----------------------------------------------------------------------------------------------------------------

#ifndef __SVC_MODELO_COMBATE_H__
#define __SVC_MODELO_COMBATE_H__

    #include "common/RefCounter.h"
    #include "GameServices/RunNode.h"
    #include "math/Vector2.h"
    #include "CommonData/CommonData.h"            // Estructuras comunes entre juego y editor
    #include "CommonData/CommonDataEnums.h"       // Estructuras comunes entre juego y editor
    #include "GameServices/DbgApp.h"
    #include "Util/PointInterpolate.h"

namespace fileSYS { class IFile;    }

namespace utilsTropa {

  struct SOclusor;
}
namespace Stat
{
  class CLogEntry;
}

namespace game {

enum eTipoDeFormacion;
enum eFlanco;
enum eRange;
class svcGestorTropas;
class CTropaBase;
struct SBatallon;

// Estructura q describe los daños producidos a una tropa
struct TResultadoAtaque
{
  eTipoAtaque m_TipoAtaque;             // Tipo de ataque q produjo los daños
  eFlanco     m_Flanco;                 // Flanco por el q se produjo el ataque
  eRange      m_Rango;                  // Rango que se computo
  int         m_Danos;                  // Daños calculados
  bool        m_bOclusion;              // Ocluyo con algun obstaculo? NOTA: No se usa
  UINT        m_NumOclusoresAmigos;
  
};



class svcModeloCombate : MAKE_SERIALIZABLE
{
public:
  INCLUDE_INTERFACE_HIERARCHY_FATHER(svcModeloCombate);
  
  // Inicizalicion/Destruccion
  bool init (const std::string & filename);
  void done (void);
  bool reload (void);
  
  // Funciones para computo referido al modelo de combata
  TResultadoAtaque computarResultadoAtaque (CTropaBase * pTropaAta, // Tropa que ataca
                                            CTropaBase * pTropaDef, // Tropa que recibe
                                            eTipoAtaque tipoAtaque, // Tipo de ataque
                                            int unidadesAta,        // Unidades que cuentan como atacando
                                            std::vector<utilsTropa::SOclusor>* pOclusores, // Lista de oclusores. Puede ser 0
                                            const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                            Stat::CLogEntry* pLogEntry) const; // Log generado con el calculo. Puede ser 0

  bool puedeRomperCuadro( CTropaBase* tropaEnCuadro, CTropaBase* tropaCargando );
  int calcChargeDamage( CTropaBase* tropa, CTropaBase* tropaCargando, Stat::CLogEntry* pLogEntry );
  
  // Funciones para obtencion de informacion
  const TDescTropa & getDescTropa (eTipoTropa tipoTropa, eRangoTropa rango) const;
  UINT getTipoTropaIndex (eTipoTropa tipoTropa, eClaseTropa * pClaseTropa = NULL) const;
  const TModifTerreno & getModifTerreno (eTiposDeTerreno tterreno) const;
  float getCargaDistMin(eClaseTropa claseTropa, eRangoTropa rango);
  float getCargaDistMax(eClaseTropa claseTropa, eRangoTropa rango);
  
  UINT  getArtDamage( CTropaBase* tropa, float distDisparo );  
  UINT  getArtDamage( eTipoTropa tipoTropa, eRangoTropa rangoTropa, UINT rangoDisparo ) const;
  float getArtPrecVsEdif( CTropaBase* tropa );
  float getArtPrecVsEdif( eTipoTropa tipoTropa, eRangoTropa rango );
  
  // Descriptores de daño y los modificadores por formación
  const TDescDanTropa &       getDescDan (eClaseTropa idxClaseTropa, int soldados, eRangoTropa rango) const;
  const TModifFormacion &     getModifForm (eTipoDeFormacion form, eClaseTropa claseTropa, eRangoTropa rango) const;
  const TInfoDanArtilleria &  getInfoDanArtilleria(eTipoTropa tipo, eRangoTropa rango) const;
  
  //·············································································································
  // Funciones para consultar propiedades de las tropas. Si se quiere saber el valor de Ataque, armadura o rango
  // deben usarse estas funciones. NOTA: El parámetro de *tropa es opcional
  //·············································································································
  UINT getValorRangoDet   ( const SBatallon* batallon, CTropaBase* tropa = 0 );        // retorna el rango de detección/disparo en metros (normalmente los valores retornados estaran entre 50 y 200)
  UINT getValorArmadura   ( const SBatallon* batallon, CTropaBase* tropa = 0 );        // retorna el valor de la armadura. De 0 a 100
  UINT getValorAtaqueADist( const SBatallon* batallon, CTropaBase* tropa = 0 );        // retorna el valor del ataque a distancia. De 0 a 100
  UINT getValorAtaqueCAC  ( const SBatallon* batallon, CTropaBase* tropa = 0 );        // retorna el valor del ataque cuerpo a cuerpo. De 0 a 100

  UINT getValorRangoDet   ( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa = 0 );
  UINT getValorArmadura   ( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa = 0 );
  UINT getValorAtaqueADist( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa = 0 );
  UINT getValorAtaqueCAC  ( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa = 0 );
  
  void setInvulnerability(bool b) { m_bInvulnerability = b;    }
  bool getInvulnerability() const { return m_bInvulnerability; } 
  
  // Constructores & Destructores
  svcModeloCombate (void);
  ~svcModeloCombate (void);

  float GetDisparoPerdidoMelee(float fPorcentajeEnemigos) const      { return m_FuncDamageLostMelee.GetVal(fPorcentajeEnemigos); }
  float GetPercentPenalizacionMelee(float fPorcentajeUnidades) const { return m_FuncPercentMelee.GetVal(fPorcentajeUnidades); }

#ifdef _INFODEBUG
  void              setInfoDebug(CInfoDebug *pID) { m_pInfoDebug=pID; }
  const CInfoDebug* getInfoDebug() const          { return m_pInfoDebug; }
#endif

  eTipoTropa getTipoTropa (eClaseTropa claseTropa, UINT index) const;

  // unas funciones para calcular los resultados del ataque (que sino computarResultadoAtaque es muy larga)
  TResultadoAtaque computarResultadoDistancia(CTropaBase * pTropaAta, // Tropa que ataca
                                            CTropaBase * pTropaDef, // Tropa que recibe
                                            int unidadesAta,        // Unidades que cuentan como atacando
                                            std::vector<utilsTropa::SOclusor>* pOclusores, // Lista de oclusores. Puede ser 0
                                            const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                            Stat::CLogEntry* pLogEntry,   // Log generado con el calculo. Puede ser 0
                                            int& danBase,                 // el daño base del ataque
                                            int& modDan,                  // el modificador al daño
                                            int& modRes,                  // el modificador a la resistencia
                                            int& modDanGlobal) const;     // el modificador de daño global
  TResultadoAtaque computarResultadoMelee  (CTropaBase * pTropaAta, // Tropa que ataca
                                            CTropaBase * pTropaDef, // Tropa que recibe
                                            int unidadesAta,        // Unidades que cuentan como atacando
                                            const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                            Stat::CLogEntry* pLogEntry,   // Log generado con el calculo. Puede ser 0
                                            int& danBase,                 // el daño base del ataque
                                            int& modDan,                  // el modificador al daño
                                            int& modRes,                  // el modificador a la resistencia
                                            int& modDanGlobal) const;     // el modificador de daño global
  TResultadoAtaque computarResultadoCarga  (CTropaBase * pTropaAta, // Tropa que ataca
                                            CTropaBase * pTropaDef, // Tropa que recibe
                                            int unidadesAta,        // Unidades que cuentan como atacando
                                            const int *pPorcentajeImpuestoDano,  // Porcentaje por el que se multiplica el daño. Puede ser 0
                                            Stat::CLogEntry* pLogEntry,   // Log generado con el calculo. Puede ser 0
                                            int& danBase,                 // el daño base del ataque
                                            int& modDan,                  // el modificador al daño
                                            int& modRes,                  // el modificador a la resistencia
                                            int& modDanGlobal) const;     // el modificador de daño global

protected:
  
  eClaseTropa getClaseTropa (eTipoTropa tipoTropa) const;
  
  bool loadDatosTropa (fileSYS::IFile * file, UINT rango);
  bool loadDatosDanTropa (fileSYS::IFile * file, UINT rango);
  bool loadModifFormacion (fileSYS::IFile * file, UINT rango);
  bool loadTablaCargasVsCuadro(fileSYS::IFile * file, UINT rango);
  bool loadTablaCargasPropiedades(fileSYS::IFile * file, UINT rango);
  bool loadInfoDanArtilleria(fileSYS::IFile * file, UINT version, UINT rango);
  
  bool loadModifTerreno (void);
  
  eModifFormacion getModifForm (eTipoDeFormacion form) const;
  eTipoInfanteria getTipoTropaInf (eTipoTropa tipoTropa) const;
  eTipoCaballeria getTipoTropaCab (eTipoTropa tipoTropa) const;
  eTipoArtilleria getTipoTropaArt (eTipoTropa tipoTropa) const;
  
  UINT tterreno2idx (eTiposDeTerreno tipo) const;
  
  // estas funciones buscan entre todas las tablas, los valores máximos

  UINT findMaxArmadura();
  UINT findMaxArmadura( TDescTropa* descTropa, int arraySizeX, int arraySizeY );
  
  UINT findMaxAtaqueADist();
  UINT findMaxAtaqueADist( const std::vector<TDescDanTropa> descDan[RANGO_COUNT], eClaseTropa claseTropa );

  UINT findMaxAtaqueADistArt();
  UINT findMaxAtaqueADistArt( const std::vector<TInfoDanArtilleria> descDan[RANGO_COUNT] );

  UINT findMaxAtaqueCAC();
  UINT findMaxAtaqueCAC( const std::vector<TDescDanTropa> descDan[RANGO_COUNT], eClaseTropa claseTropa );

  UINT getValorPromedioArmadura( UINT resDis, UINT resCue, UINT resCar );
  UINT getValorPromedioAtaqueADist( const TDescDanTropa& descDan, float prec1, float prec2, float prec3, float modDan, eClaseTropa claseTropa, eRangoTropa rangoTropa );           // prec de 0.0 a 1.0f
  UINT getValorPromedioAtaqueADistArt( const TInfoDanArtilleria& descDan, float prec1, float prec2, float prec3  );   // prec de 0.0 a 1.0f
  UINT getValorPromedioAtaqueCAC( const TDescDanTropa& descDan, float prec, float modDan, eClaseTropa claseTropa, eRangoTropa rangoTropa );

  UINT getValorAtaqueADistArt( const SBatallon* batallon, CTropaBase* tropa = 0 );  // retorna el valor del ataque a distancia para art. De 0 a 100
  UINT getValorAtaqueADistArt( const TDescTropa& descTropa, eTipoTropa tipoTropa, eRangoTropa rango, CTropaBase* tropa = 0 );  // 


private:

  std::string m_FileName;

  // Descriptores de datos de tropa para cada tipo
  TDescTropa      m_DescTropaInf[RANGO_COUNT][TI_COUNT];
  TDescTropa      m_DescTropaCab[RANGO_COUNT][TC_COUNT];
  TDescTropa      m_DescTropaArt[RANGO_COUNT][TA_COUNT];
  
  // Descriptores de daños para cada tipo de tropa; cada entrada dentro del vector es la descripcion
  // de daños para un rango de soldados; las entradas estan ordenadas en ordenadas de mayor a menor
  std::vector<TDescDanTropa>  m_DescDanTropaInf[RANGO_COUNT];
  std::vector<TDescDanTropa>  m_DescDanTropaCab[RANGO_COUNT];
  
  // Modificadores por formacion
  // TODO: Podriamos meterlo en un array bidiminsional!!
  TModifFormacion   m_ModifFormacionInf [RANGO_COUNT][MFORM_COUNT];
  TModifFormacion   m_ModifFormacionCab [RANGO_COUNT][MFORM_COUNT];

  // Modificadores por tipo de terreno
  TModifTerreno m_ModifTerreno [TT_COUNT];

  // Tablas de datos de cargas
  TCargasVsCuadro     m_CargasVsCuadroInf [RANGO_COUNT][RANGO_COUNT];
  TCargasVsCuadro     m_CargasVsCuadroCab [RANGO_COUNT][RANGO_COUNT];
  TCargasPropiedades  m_CargasPropiedades [RANGO_COUNT][NUM_TROPAS_CARGAN];

  // Daños de artillería
  std::vector<TInfoDanArtilleria>  m_InfoDanArtilleria[RANGO_COUNT];
  
  bool m_bInvulnerability;
  
  // Valores almacenados
  UINT m_MaxValArmadura;
  UINT m_MaxValAtaqueADist;
  UINT m_MaxValAtaqueADistArt;
  UINT m_MaxValAtaqueCAC;

  // Funcion para el porcentaje de daño de disparo perdido por disparar en melee
  CPointInterpolate<float> m_FuncDamageLostMelee;

  // Funcion para la penalizacion cuando se aglomeran tropas en melee
  CPointInterpolate<float> m_FuncPercentMelee;
 
#ifdef _INFODEBUG
  CInfoDebug *m_pInfoDebug;
#endif

};

} // end namespace game
#endif
