#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: CommonDataSonido.h
// Date: 02/2004
// Ricard
//----------------------------------------------------------------------------------------------------------------
// Datos de Sonidos relativos al nivel; Se comparten con el editor
// NOTA: Si se modifican datos, actualizar CNapEditor::saveSoundMap() y la CSonidoMapa::load()
//----------------------------------------------------------------------------------------------------------------

#ifndef __COMMONDATASONIDO_H__
#define __COMMONDATASONIDO_H__

#include <string>
#include <vector>
#include "util/assert_pro.h"
#include "math/Vector3.h"
#include "file/FileSystem.h"
#include "Util/Serialize.h"

#define NO_SOUND  "NINGUN SONIDO"

namespace game {

  struct sSonidoId {
    std::string   IdName;
    std::string   FileName;
  };

  struct sMusica {
    const sSonidoId*  id;
  };

  struct sSonidoAmbiente {
    const sSonidoId*  id;
    UINT              uVolumen;
  };

  struct sSonidoPosicional {
    const sSonidoId*  id;
    UINT              uVolumen;
    CVector3          v3Pos;
    float             minRadius;
    float             maxRadius;
  };

  class CSonidoMapa {
  public:
    bool init();
    bool isInit() const                                                                   { return m_bInit;               }
    bool done();
    bool clear();

    // metodos relacionados con la musica ----------------------
    sMusica*            getMusica()                                                       { return &m_sMusica;            }
    const sMusica*      getMusica() const                                                 { return &m_sMusica;            }
    void                resetMusica();
    
    // metodos relacionados con los sonidos ambiente -----------
    sSonidoAmbiente*    createSonidoAmbiente();
    bool                deleteSonidoAmbiente(sSonidoAmbiente* sonido);
    void                clearSonidosAmbiente();
    std::vector<sSonidoAmbiente*>* getSonidosAmbiente()                                   { return &m_vSonidosAmbiente;   }
    const sSonidoAmbiente* getSonidoAmbiente(UINT id) const                               { return m_vSonidosAmbiente[id];}
    UINT                countSonidosAmbiente() const;
    bool                isValid(const sSonidoAmbiente* sonido) const;

    // metodos relacionados con los sonidos posicionales -------
    sSonidoPosicional*  createSonidoPosicional(float minRad, float maxRad, CVector3 Pos);
    bool                deleteSonidoPosicional(sSonidoPosicional* sonido);
    void                clearSonidosPosicional();
    std::vector<sSonidoPosicional*>* getSonidosPosicional()                               { return &m_vSonidosPosicional; }
    const sSonidoPosicional* getSonidoPosicional(UINT id) const                             { return m_vSonidosPosicional[id];}
    UINT                countSonidosPosicionales() const;
    bool                isValid(const sSonidoPosicional* sonido) const;

    // metodos relacionados con la tabla general de sonidos ----
    bool loadIdSonidos(const util::scriptBlock& script);
    void clearIdSonidos(bool creaSonidoVacio = true);
    UINT countIdSonidos() const;
    sSonidoId*  searchIdSonido(const std::string &IdName) const;
    sSonidoId*  searchIdSonido(const UINT id) const;
    int         getPosIdSonido(const sSonidoId* sonido) const;

    // metodos generales del mapa de sonidos -------------------
    bool          startFromFile(fileSYS::IFile* file, UINT version, bool export);
    bool          load(fileSYS::IFile *file, UINT version_format, bool export);
    void          present(bool printLabels=true) const;
    const CSonidoMapa* ptr() const                                                        { return this;                  }

    CSonidoMapa::CSonidoMapa();
    CSonidoMapa::~CSonidoMapa();

  private:
    bool                            m_bInit;
    sMusica                         m_sMusica;
    std::vector<sSonidoAmbiente*>   m_vSonidosAmbiente;
    std::vector<sSonidoPosicional*> m_vSonidosPosicional;
    std::vector<sSonidoId*>         m_vSonidosId;
  };

} // end namespace game

#endif // __COMMONDATASONIDO_H__