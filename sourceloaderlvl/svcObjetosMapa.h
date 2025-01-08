#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: svcObjetosMapa.h
// Date: 03/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Gestiona todos los objetos del mapa
//----------------------------------------------------------------------------------------------------------------

#ifndef __SVC_OBJETOS_MAPA_H__
#define __SVC_OBJETOS_MAPA_H__

    #include "GameServices/RunNode.h"    
    #include "math/Vector3.h"
    #include "util/Hstr.h"
    #include <vector>
    #include "ClassObject.h"

namespace e3d { class IAnimModelNode; class IAnimation;     }
namespace fileSYS { class IFile;  }

class IGrxObject;

namespace game
{
class CObjetoBase;
class svcGestorVolumenes;
class svcOpcionesJuego;

enum { INVALID_OBJECT = 0xffffffff   };

// Tipo de Objeto de mapa:
// IMPORTANTE: No cambiar el orden de estas enumeraciones; ya q se cargan desde el .LVL!!!
enum eTipoObjeto
{
  TO_DECORADO,
  TO_CONSTRUCCION,
  TO_ARBOL,
  TO_AGUA,
  TO_HIELO,
  TO_FORCEDWORD = 0x7fffffff
};

class svcObjetosMapa : MAKE_SERIALIZABLE
{
public:
  
  // Inicializacion y destruccion
  bool init (fileSYS::IFile * file, const std::string & path, UINT version);
  bool load (fileSYS::IFile * file, UINT version);
  void done (void);
  
  UINT countObjects (void) const                        { return m_Object.size ();      }
  const SClassObj * getClassObj (UINT id) const         { assert (id < m_ClassObj.size ()); return id < m_ClassObj.size ()? m_ClassObj[id]: NULL; }
  CObjetoBase * getObject (UINT id) const               { assert (id < m_Object.size ());   return id < m_Object.size   ()? m_Object[id]  : NULL; }
    
  // Constructors  & Destructors
  svcObjetosMapa (void);
  ~svcObjetosMapa (void);

  // Serialziación
  INCLUDE_INTERFACE;

  // Gets de variables miembro
  svcGestorVolumenes * getGestorVolumenes() { return m_svcGestorVolumenes.ptr(); }

private:

  void readVolume(fileSYS::IFile * file, UINT version, SClassObj::TModulo& modulo);      // carga los datos de un volumen de un modulo

  CRunNode::IMPORT_SVC<svcGestorVolumenes> m_svcGestorVolumenes;
  CRunNode::IMPORT_SVC<svcOpcionesJuego>   m_svcOpcionesJuego;
  
  std::vector <CObjetoBase *> m_Object;                 // Lista de objetos (instancias)
  std::vector <SClassObj *> m_ClassObj;                 // Lista de objetos de la libreria
  bool m_bInit;
  
};

} // end namespace game

#endif
