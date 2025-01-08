#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcEscenario.h
// Date: 12/2002
//----------------------------------------------------------------------------------------------------------------
// Implementacion del escenario del modelo de batalla
//----------------------------------------------------------------------------------------------------------------

#ifndef __SVC_ESCENARIO_H__
#define __SVC_ESCENARIO_H__

    #include "GameServices/RunNode.h"
    #include "svcMapaFisico.h"
    #include "svcMapaLogico.h"
    #include "svcObjetosMapa.h"
    #include "CommonData/CommonDataSonido.h"
    #include "X3DEngine/ParticleSystem.h"
    #include "Condiciones.h"

class IGrxEffect;
    
namespace game
{
struct SGameLoc;
class svcGestorCriaturas;
class svcEstadoJuego;
class ICamera;

class svcEscenario : MAKE_SERIALIZABLE
{
public:

  INCLUDE_INTERFACE;

  // Inicialización / Destrucción
  bool init (const util::HSTR & fileDefEscenarios);
  void done (void);
  
  // carga un escenario completo (fisico y logico) a partir de un nombre
  bool cargaEscenario (util::HSTR const & _hNombreEscenario, bool fromSerialization = false);
  // descarga de memoria el escenario
  void liberaEscenario (void );

  void doneAudio (void);

  void playMapSound(bool fromSerialization = false);

  // Encuentra una posicion de inicio para un tipo de tropa
  UINT findPosInicioTropa (eTipoBando tipoBando, eTipoTropa _ttropa, SGameLoc * pGameLoc_);

  void getPosRefuerzosTropas (eTipoBando tipoBando, eTipoTropa _ttropa, std::vector<SGameLoc> &pos);

  CSonidoMapa*                   getMapaSonidos()                     { return  m_svcMapaLogico->getMapaSonidos(); }
  const std::vector<sCondicion>& getCondiciones(const eTipoBando bando) { return m_svcMapaLogico->getCondiciones(bando); }
  const std::vector<sCondicion>& getCondicionesAta() const            { return m_svcMapaLogico->getCondicionesAta(); }
  const std::vector<sCondicion>& getCondicionesDef()const             { return m_svcMapaLogico->getCondicionesDef(); }
  float getDuracionEscenarion() const                                 { return m_svcMapaLogico->getDuracionEscenarion(); }      // en minutos.

  const util::HSTR getPathEscenario() const { return m_hPath; }

  // Constructor & Destructor
  svcEscenario (void);
  ~svcEscenario (void);

private:
  
  util::HSTR findNombreEscenario (const util::HSTR & _hNombreEscenario) const;
  void postCargaEscenario (bool bFromSerialization);
  void processLocations (void);
  bool creaCriatura (const SLocation &loc);
  bool createGrxEffect (const SLocation &loc);
  void loadAudio (const util::scriptBlock& sb );
     
private:

  CRunNode::EXPORT_SVC<svcMapaFisico>           m_svcMapaFisico;
  CRunNode::EXPORT_SVC<svcObjetosMapa>          m_svcObjetosMapa;     // Nota: 
  CRunNode::EXPORT_SVC<svcMapaLogico>           m_svcMapaLogico;      // svcObjetosMapa tiene que estar exportado antes! 
  
  CRunNode::IMPORT_SVC<ICamera>                 m_svcCamera;
  CRunNode::IMPORT_SVC<svcGestorCriaturas>      m_svcGestorCriaturas;  
  CRunNode::IMPORT_SVC<svcEstadoJuego>          m_svcEstadoJuego;  

  util::HSTR m_hEscenariosDefsFile;             // Fichero de definiciones de escenarios
  util::HSTR m_hEscenarioCargado;               // Escenario cargado
  util::HSTR m_hPath;                           // Path del escenario

  std::vector<IGrxEffect*>                      m_Effects;

  bool m_bInit;                                 // Clase inicializada?
  
};

} // end namespace game


#endif