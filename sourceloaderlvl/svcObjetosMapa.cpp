//------------------------------------------------------------------------------------------------------------------
// svcObjetosMapa.cpp
// Date: 03/2003
//------------------------------------------------------------------------------------------------------------------

    #include "__PCH_ModeloBatalla.h"
    #include "svcObjetosMapa.h"
    #include "gameServices/svcGestorVolumenes.h"
    #include "GameModes/svcOpcionesJuego.h"
    #include "ObjetoBase.h"
    #include "Construccion.h"
    #include "Water.h"
    #include "Ice.h"
    #include "file/IFile.h"
    #include "X3DEngine/ModelNode.h"
    #include "X3DEngine/Import.h"
    #include "Visualization/ModelManager.h"
    #include "UtilsTropas.h"
    
    
    #include "dbg/SafeMem.h"

namespace game {

//------------------------------------------------------------------------------------------------------------------
// Constructores y destructores
//------------------------------------------------------------------------------------------------------------------

svcObjetosMapa::svcObjetosMapa (void)
{
  m_bInit = false;
}
svcObjetosMapa::~svcObjetosMapa (void)
{
}

//------------------------------------------------------------------------------------------------------------------
// Inicializa los objetos del mapa; cargando la libreria de objetos : Aqui no se cargan las instancias del 
// mapa; sino la libreria de objetos unicos; de donde instanciaremos objetos
//------------------------------------------------------------------------------------------------------------------

bool svcObjetosMapa::init (fileSYS::IFile * file, const std::string & path, UINT version)
{
  assert (m_bInit == false);
  assert (file);
  
  LOG (("*ModeloBatalla: (INFO) : Cargando libreria de objetos del mapa desde el path %s\n", path.c_str ()));
  
  utilsTropa::set_ObjetosMapa (this);    // Permite acceso desde utilsTropas
  
  // Cargamos la libreria de objetos
  
  UINT count = file->read <UINT> ();
  m_ClassObj.reserve (count);
  
  for (UINT i = 0; i < count; i++)
  {
    SClassObj * pObj = new SClassObj;
    m_ClassObj.push_back (pObj);
    
    file->read (&pObj->m_Type);                           // Tipo de objeto
		std::string name = fileSYS::importString (file);
		
    // TODO: Tener en cuenta q no es necesario cargar estos modelos; ya q realmente luego no se instancian estos;
    // si no los q hay dentro del manager
    
    //-------------<<<
    // Se cambia el contexto de importación para que no se active el alpha-blend si se ha especificado tal en las opciones
    // (Solo se aplica a los objetos ARBOL, pero como no es seguro que tales objetos vengan con ese flag de
    // tipo activado, se aplica a los que no son CONSTRUCCIONES ni AGUA)
    bool bUsarAlphaBlend = m_svcOpcionesJuego->getOpcionesVideo()->getAlphaBlend();
    if(bUsarAlphaBlend || pObj->m_Type == TO_CONSTRUCCION || pObj->m_Type == TO_AGUA)
    {
      pObj->m_Node = CModelManager::ref ().createAnimModel (util::HSTR (path+name));
    }
    else
    {
      e3d::IImporter::SContext context = e3d::Importer->getContext ();
      context.NoAlphaBlend = true;
      e3d::Importer->setContext (context);

      pObj->m_Node = CModelManager::ref ().createAnimModel (util::HSTR (path+name));

      context.NoAlphaBlend = false;
      e3d::Importer->setContext (context);
    }
    //------------->>>
    
    if ( pObj->m_Type == TO_CONSTRUCCION)
    {
      // Cargamos modulos
      UINT countModulos = file->read <UINT> ();
      pObj->m_Modulo.reserve (countModulos);

      for (UINT j = 0; j < countModulos; j++)
      {
        SClassObj::TModulo * pModulo = new SClassObj::TModulo;
        pObj->m_Modulo.push_back (pModulo);

        // Importamos animaciones de destruccion del modulo
        
        UINT countAnis = file->read <UINT> ();
        pModulo->m_AniDestroyName.reserve (countAnis);
        
        for (UINT j = 0; j < countAnis; j++)
        {        
          std::string name = fileSYS::importString (file);
          pModulo->m_AniDestroyName.push_back (util::HSTR (path+name));
        }
        
        // Cargamos los volumenes del modulo
        
        bool bHasVolume = file->read <bool> ();
        
        if (bHasVolume)
        { 
          if ( version >= 150 ) {   // nueva versión, varios volumenes por modulo
          
            UINT numVolumes = file->read<UINT>();
            
            for (UINT iVols = 0; iVols < numVolumes; ++iVols) {
              readVolume(file, version, *pModulo);
            }
          
          } else {    // version antigua, un solo volumen por módulo
            
            readVolume(file, version, *pModulo);
          }
        }
      }
    }
    else if ( pObj->m_Type == TO_AGUA)
    {
    }
    else if ( pObj->m_Type == TO_ARBOL)
    {
    }
    file->read <UINT> ();     // Reservado para uso futuro
    file->read <UINT> ();     // Reservado para uso futuro
  }
  
  LOG (("*ModeloBatalla: (INFO) : Liberia de objetos cargada : Objetos totales = %d\n", m_ClassObj.size ()));
    
  m_bInit = true;
  
  return m_bInit;
}

//------------------------------------------------------------------------------------------------------------------
// readVolume: lee los datos de un volumen de un modulo
//------------------------------------------------------------------------------------------------------------------

void svcObjetosMapa::readVolume(fileSYS::IFile * file, UINT version, SClassObj::TModulo& modulo)
{
  SArbitraryVolume* newVol = new SArbitraryVolume;
  bool usePrevious = false;
  
  if ( version >= 150 ) {
    file->read ( &usePrevious );
  }
  
  // usamos el volumen anterior
  if ( usePrevious ) {
  
    assert( modulo.m_Volumenes.size() > 0 && "NO SE PUEDE CARGAR VOLUMEN PREVIO." );
    
    *newVol = *modulo.m_Volumenes[ modulo.m_Volumenes.size() - 1 ];
    modulo.m_Volumenes.push_back( newVol );
    
    return;
  }
  
  std::string volFileName = fileSYS::importString (file);
  file->read ( &newVol->m_BSphere );
  file->read ( &newVol->m_BBox );

  newVol->m_BVolume = new SGeoVolume;
  assert (newVol->m_BVolume);
  file->read (&newVol->m_BVolume->m_Vertices);
  file->read (&newVol->m_BVolume->m_Indices);
  newVol->m_BVolume->m_Triangles = newVol->m_BVolume->m_Indices / 3;

  newVol->m_BVolume->m_VertexList = new CVector3 [newVol->m_BVolume->m_Vertices];
  newVol->m_BVolume->m_IndexList = new WORD [newVol->m_BVolume->m_Indices];

  file->readArray (&newVol->m_BVolume->m_VertexList [0], newVol->m_BVolume->m_Vertices);
  file->readArray (&newVol->m_BVolume->m_IndexList [0], newVol->m_BVolume->m_Indices);
  
  modulo.m_Volumenes.push_back(newVol);
}

//------------------------------------------------------------------------------------------------------------------
// Carga todos los objetos del mapa y los va creando y registrando; los objetos de aqui son las instancias del mapa
//------------------------------------------------------------------------------------------------------------------

bool svcObjetosMapa::load (fileSYS::IFile * file, UINT version)
{
  // Cargamos los objetos del terreno
  
  LOG (("*ModeloBatalla: (INFO) : Creando instancias de objetos del mapa\n"));
  
  UINT count = file->read <UINT> ();
  for (UINT i = 0; i < count; i++)
  {
    CObjetoBase * pObj = NULL;
    
    eTipoObjeto type = file->read <eTipoObjeto> ();
      
    switch (type)
    {
    case TO_DECORADO:
      pObj = new CObjetoBase;
      break;
    case TO_CONSTRUCCION:
      pObj = new CConstruccion;
      break;
    case TO_AGUA:
      pObj = new CWater;
      break;
    case TO_HIELO:
      pObj = new CIce;
      break;
    case TO_ARBOL:
      pObj = new CObjetoBase;
      break;
    default:
      assert (0);
      continue;
    };

    // Cargamos el objeto desde disco
    pObj->getDatosObjeto ().m_Type = type;                // Necesario asignarlo antes de cargar el objeto
    pObj->getDatosObjeto ().m_ObjID = countObjects ();    // Necesario asignarlo antes de cargar el objeto
    if (! pObj->load (file, version) )
    {
      delete pObj;
      continue;
    }
    // Insertamos el objeto en la lista
    m_Object.push_back (pObj);
  }
  
  LOG (("*ModeloBatalla: (INFO) : Instancias de objetos en mapa creadas : Instancias totales = %d\n", countObjects ()));
  
  return true;
}

//------------------------------------------------------------------------------------------------------------------
// Destruye todos los objetos de mapa
//------------------------------------------------------------------------------------------------------------------

void svcObjetosMapa::done (void)
{
  // Destruimos las instancias
  
  UINT count = m_Object.size ();
  for (UINT i = 0; i < count; i++)
  {
    if (m_Object [i])
    {
      m_Object [i]->done ();
      delete m_Object [i];
      m_Object [i] = NULL;
    }
  }
  m_Object.clear ();
  
  // Destruimos la libreria
  
  count = m_ClassObj.size ();
  for (UINT i = 0; i < count; i++)
  {
    if (m_ClassObj [i])
    {
      delete m_ClassObj [i];
      m_ClassObj [i] = NULL;
    }
  }
  m_ClassObj.clear ();
  m_bInit = false;
  
  utilsTropa::set_ObjetosMapa (NULL);    // Permite acceso desde utilsTropas
  
}


// =============
// SERIALIZACIÓN
// =============
INIT_SERIALIZATION (svcObjetosMapa, SS_LOGIC)

  // Nota: Esto es una serializacion de "trapi" ke te cagas.
  //       Vamos a recorrer la lista de objetos que YA EXISTE
  //       y vamos a serializar cada elemento ya que 
  //       se serializan selectivamente los datos que queremos,
  //       sólo los que queremos. El resto ya se ha Cargado.
  SERIALIZE      (m_svcGestorVolumenes);
  SERIALIZE_SKIP (m_ClassObj);
  SERIALIZE_SKIP (m_bInit);
  SERIALIZE_SKIP (m_Object);

  for (std::vector<CObjetoBase*>::iterator i = m_Object.begin (); i != m_Object.end (); i++)
  {
    CObjetoBase*  obj = *i;
    if ((*i)->isType (TO_CONSTRUCCION))
    {
      CConstruccion* construccion = reinterpret_cast<CConstruccion*>(*i);
      SERIALIZE (*construccion);
    }
  }

END_SERIALIZATION (svcObjetosMapa, SS_LOGIC)

}   // end namespace game










