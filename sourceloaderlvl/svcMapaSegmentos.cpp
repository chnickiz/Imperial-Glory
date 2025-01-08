//----------------------------------------------------------------------------------------------------------------
// File: svcMapaSegmentos.cpp
// Date: 05/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Gestiona el mapa de segmentos del terreno utilizado para colisiones;
// Cada sector esta formado por un conjunto de segmentos cerrado; el primer punto esta almacenado dos veces; 
// una al comienzo y otra al final. Los sectores no tienen porque ser convexos
//----------------------------------------------------------------------------------------------------------------

    #include "__PCH_ModeloBatalla.h"
    #include "svcMapaSegmentos.h"
    #include "file/IFile.h"
    #include "math/cmath.h"
    #include "math/Segment2.h"
    #include "dbg/SafeMem.h"
        
namespace game {

//#define __DRAWDEBUG__

//------------------------------------------------------------------------------------------------------------------
// Constructores y destructores
//------------------------------------------------------------------------------------------------------------------

svcMapaSegmentos::svcMapaSegmentos (void)
{
  m_bInit = false;
}
svcMapaSegmentos::~svcMapaSegmentos (void)
{
  assert (m_bInit == false);
}

//------------------------------------------------------------------------------------------------------------------
// Inicializa los objetos del mapa; cargando la libreria de objetos
//------------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::init (void)
{
  assert (m_bInit == false);
  m_bInit = true;
  return m_bInit;
}

//------------------------------------------------------------------------------------------------------------------
// Carga todos los objetos del mapa y los va creando y registrando
//------------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::load (fileSYS::IFile * file, UINT headerVersion)
{
  assert (file);

  LOG (("*ModeloBatalla: (INFO) : Cargando mapa de segmentos ...\n"));
  
  // Leemos la lista de sectores
  {
    SSectorPoint point;
    CVector3 pointPos;
    CVector2 normal;
    float longitud;
    DWORD diffuse;
    bool bClosed;
    
    UINT count = file->read <UINT> ();
    for (UINT i = 0; i < count; i++)
    {
      SSector * pSector = createSector ();
      file->read <DWORD>    ( &diffuse );
      file->read <CVector3> ( &pSector->m_Center );
      if ( headerVersion >= 103 ) {
        file->read <float> ( &pSector->m_BSRadius );
      }
      file->read <bool>     ( &bClosed );
      // Leemos la lista de indices a los puntos
      UINT points = file->read <UINT> ();
      pSector->m_PointList.reserve (points);
      for (UINT j = 0; j < points; j++ )
      {        
        file->read ( &pointPos );
        point.m_Pos = math::vector2fromXZ (pointPos);
        pSector->m_PointList.push_back (point);
      }
      UINT infos = file->read <UINT> ();
      pSector->m_InfoList.reserve (infos);
      for (UINT j = 0; j < infos; j++ )
      {        
        file->read ( &normal );
        
        SSegmentInfo newInfo;
        newInfo.m_Normal = normal;
        
        if ( headerVersion >= 103 ) {
          
          file->read ( &longitud );
          newInfo.m_Longitud = longitud;
        }
        
        pSector->m_InfoList.push_back (newInfo);
      }
    }
  }
  
  LOG (("*ModeloBatalla: (INFO) : Mapa de segmentos cargado : Total Sectores = %d\n", m_SectorList.size ()));
  

  return true;
}

//------------------------------------------------------------------------------------------------------------------
// Destruye todos los objetos de mapa
//------------------------------------------------------------------------------------------------------------------

void svcMapaSegmentos::done (void)
{
  assert (m_bInit == true);
  
  if (m_bInit)
  {
    while (!m_SectorList.empty ())
      deleteSector (*m_SectorList.begin ());
    m_SectorList.clear ();
  
    m_bInit = false;
  }
}

//----------------------------------------------------------------------------------------------------------------
// Crea un sector
//----------------------------------------------------------------------------------------------------------------

svcMapaSegmentos::SSector * svcMapaSegmentos::createSector (void)
{
  SSector * sector = new SSector;
  m_SectorList.push_back (sector);
  
  return sector;
}

//----------------------------------------------------------------------------------------------------------------
// Destruye un sector
//----------------------------------------------------------------------------------------------------------------

void svcMapaSegmentos::deleteSector (SSector * sector)
{
  sector->m_PointList.clear ();
  
  // Destruimos fisicamente el sector
  UINT count = m_SectorList.size ();
  for (UINT i = 0; i < count; i++)
  {
    if (m_SectorList [i] == sector)
    {      
      delete m_SectorList [i];
      m_SectorList [i] = NULL;
      m_SectorList.erase (m_SectorList.begin ()+i);
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------
// A partir de dos segmentos; vamos generando los segmentos intermedios con un espaciado concreto; y vamos clipeando
// cada uno de los segmentos contra el mapa; si un segmento clipea; inmediatamente retornamos asignando clipSeg como
// el segmento resultante;
// La funcion nunca clipea el segmento1 (inicial) en cambio si el segmento2 (final)
// retorno: true  -> se encontro interseccion
//          false -> no hay interseccion
// bEndSegment: Es asignada cuando el segmento que intersectaba el mapa era el segmento final
// t : es el factor de 0 a 1 donde se produjo la interseccion
//----------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::clipSegmentVolume (const CSegment2 & segment1, const CSegment2 & segment2, CSegment2 & clipSeg, bool & bEndSegment, float & t) const
{
const float spacing = 150.0f;
const float invSpacing = 1.0f / spacing;
const float factorEpsilonSpacing = 0.2f;    // Un 20% del spacing
const float minDistanceSeg = spacing;
const float minSqrDistanceSeg = minDistanceSeg*minDistanceSeg;

  bEndSegment = false;    // Indicamos q no es el segmento final el q hemos clipeado
  bool bRet = false;
  
  float fSeg0, fSeg1;
  if (segment1.getSqrDistance (segment2, &fSeg0, &fSeg1) > minSqrDistanceSeg)
  {
    // Calculamos el centro de cada segmento
    
    CVector2 center1 = segment1.origin () + (segment1.direction () * 0.5f);
    CVector2 center2 = segment2.origin () + (segment2.direction () * 0.5f);
    
    // Calculamos la longitud del vector de recorrido
    
    CVector2 vpath = center2 - center1;
    float len = vpath.length ();
    
    // Calculamos el numero de pasos a testear segun la constante de espaciado
    
    // TODO: Hacer bien todo esto de los steps!!
    float fSteps = len * invSpacing;
    int steps = int (fSteps);     
    float delta = 1.0f / fSteps;    // Observar q calculamos la relacion con el valor flotante no entero!!
    float f = delta;
    
    float intPart;
    float fractional = modff (fSteps, &intPart);
    
    // Si la parte fraccionaria de casilla de mas que cogemos es muy pequeña; quitamos un paso que es sustituido
    // por el ultimo segmento; de manera que aumentamos las posibilidades de marcar el flag bEndSegment; 
    // ademas de esta manera tenemos en cuenta q si la division es exacta (fractional = 0) no se procese dos veces
    // el ultimo segmento; una en el bucle y otra al finalizar
    
    if (fractional <= factorEpsilonSpacing)
      steps --;
    
    // Calculamos origen y direccion de los segmentos que van de un origen a otro y de un destino a otro
    
    CVector2 o1 = segment1.origin ();
    CVector2 o2 = segment1.origin () + segment1.direction ();
    CVector2 dir1 = segment2.origin () - o1;
    CVector2 dir2 = (segment2.origin ()+segment2.direction()) - (segment1.origin ()+segment1.direction());
    
    for (int i = 0; i < steps; i++, f += delta)   // Bucle con signo necesario
    {
      // Calculamos el segmento a clipear
      CVector2 p1 = o1 + dir1 * f;
      CVector2 p2 = o2 + dir2 * f;
      
      CSegment2 seg (p1, p2-p1);
      
      // Comprobamos clipping con el segmento calculado
      if ( clipSegment (seg) )
      {
        t = f;         // Factor de (0-1) entre los 2 segmentos
        clipSeg = seg;
        bRet = true;
        return bRet;
      }
    }
  }
  
  // Clipeamos concretamente el segmento final

  CSegment2 seg (segment2);
  if ( clipSegment (seg) )
  {
    bEndSegment = true;    // Indicamos q es el segmento final el q hemos clipeado
    clipSeg = seg;
    t = 1.0f;
    return true;
  }
 
  return bRet; 
}

//----------------------------------------------------------------------------------------------------------------
// Clipea un segmento contra un sector concreto
//----------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::clipSegment (SSector * sector, CSegment2 & segment) const
{
bool bRet = false;
int riQuantity;
CVector2 afT;
  
  #ifdef __DRAWDEBUG__
  const float h = 1500.0f;
  e3d::primitives::line3D (math::vector2to3XZ (segment.origin (), h), math::vector2to3XZ (segment.origin () + segment.direction (), h), 0x0000ff00);
  #endif
  
  const std::vector <SSectorPoint> & pointList = sector->m_PointList;
  UINT points = pointList.size ();
  
  //CVector2 segA = segment.origin ();
  //CVector2 segB = segment.origin () + segment.direction ();
  
  for (UINT i = 1; i < points; i++)
  {
    // Construimos el segmento a partir de los 2 puntos
    CSegment2 segWall (pointList [i-1].m_Pos, pointList [i].m_Pos-pointList [i-1].m_Pos);
    
    if ( segment.intersects (segWall, riQuantity, afT) )
    {
      // Calculamos el punto de interseccion a partir de los factores q van de 0 a 1
      
      CVector2 pointI; 
      pointI = segment.origin () + (segment.direction () * afT [0]);
      CVector2 pointB = segment.origin () + segment.direction ();
      
      // Comprobamos que punto debemos clipear; para ello proyectamos el punto a del segmento q estamos
      // clipeando; sobre la normal del segmento con el q clipeamos; y segun la distancia con signo de esta
      // proyeccion; clipeamos un punto u otro
      
      CVector2 center = segment.origin () + (segment.direction () * 0.5f);
      CSegment2 normal (center, sector->m_InfoList [i-1].m_Normal);
      CVector2 diff = segment.origin () - normal.origin ();
      float fT = diff.dot (normal.direction ());
      
      if (fT < 0.0f)          // Clipeamos el punto A
      {
        segment.origin () = pointI;
        segment.direction () = pointB - pointI;
      }
      else                    // Clipeamos el punto B
      {
        segment.direction () = pointI - segment.origin ();
      }
      
      /*
      if (afT [0] < 0.5)      // Clipeamos el punto A
      {
        segment.origin () = pointI;
        segment.direction () = pointB - pointI;
      }
      else                    // Clipeamos el punto B
      {
        segment.direction () = pointI - segment.origin ();
      }
      */
      
      #ifdef __DRAWDEBUG__
      const float h = 2000.0f;
      e3d::primitives::line3D (math::vector2to3XZ (segment.origin (), h), math::vector2to3XZ (segment.origin () + segment.direction (), h), 0xff0000);
      
      #endif      
      
      bRet = true;
    }
  }
  
  return bRet;
}

//----------------------------------------------------------------------------------------------------------------
// Clipea un segmento contra todos los sectores
//----------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::clipSegment (CSegment2 & segment) const
{
bool bRet = false;
  
  UINT count = m_SectorList.size ();
  for (UINT i = 0; i < count; i++)
  {
    if ( clipSegment (m_SectorList [i], segment) )
    {
      bRet = true;
    }
  }
  
  return bRet;
}

//----------------------------------------------------------------------------------------------------------------
// Busca y retorna un path de puntos que nos sirve para llegar de 'posOrig' a 'posDest' (estos incluidos).
// Retornamos true siempre que todo haya salido bien, aun asi el que use esta funcion debe estar preparado para
// cuando esta falle (se retornará false y 'posOrig','posDest' insertados en el path en caso de fallo)
//----------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::findPath ( const CVector2& posOrig, const CVector2& posDest, std::vector<CVector2>* path  )
{
  
  assert( path && "PATH POINTER INVALIDO" );
  
  if ( !path->empty() ) {
    path->clear();
  }

  // construimos el segmento que vamos a utilizar para los tests
  CSegment2 segment( posOrig, posDest - posOrig );
  
  // insertamos el punto de origen como primer nodo en la ruta
  path->push_back( posOrig );

  // recorremos los sectores en busca de intersecciones con nuestro segmento. 
  // Importante: usamos solo el primer sector con el que haya interseccion para generar la ruta.

  bool pathFound = false;
  
  SSector* currSector = 0;
  
  UINT sectorCount = m_SectorList.size ();
  for (UINT iS = 0; iS < sectorCount, pathFound == false; iS++) {
    
    currSector = m_SectorList [iS];
    
    // usando el sector actual intentamos conseguir una ruta
    pathFound = findPath( currSector, segment, path );
  }
  
  assert( pathFound && "RUTA NO ENCONTRADA EN EL MAPA DE SEGMENTOS. ERROR NO ESPERADO. REVISAR PORQUE SE LLEGA A ESTE CASO" );
  
  // insertamos el punto de destino como ultimo nodo en la ruta
  path->push_back( posDest );
  
  return pathFound; 
}

//----------------------------------------------------------------------------------------------------------------
// Busca una ruta en el sector especificado. Añade puntos a la ruta. NO añade el nodo final
//----------------------------------------------------------------------------------------------------------------

bool svcMapaSegmentos::findPath ( SSector * sector, CSegment2 & segment, std::vector<CVector2>* path  )
{
  bool bRet = false;
  int riQuantity = 0;
  CVector2 afT(CVector2::Null);
  
  int indMayorAft = -1;   // indices de la interseccion con el mayor y el menor afT. Correspondientes al vector de intersecciones
  int indMenorAft = -1;   // los obtenemos a medida que vamos realizando los test de interseccion para evitar tener que reparsear el vector en busca del mayor y del menor
  float menorAft = 1.0f;
  float mayorAft = 0.0f;
  
  // vector temporal correspondiente a las intersecciones con segmentos. 
  std::vector<sSegmentIntersect> intersecciones;
  
  const std::vector <SSectorPoint> & pointList = sector->m_PointList;
  UINT points = pointList.size ();
  
  for (UINT i = 1; i < points; i++)
  {
    // Construimos el segmento a partir de los 2 puntos
    CSegment2 segWall (pointList [i-1].m_Pos, pointList [i].m_Pos-pointList [i-1].m_Pos);
    
    // Si hay interseccion nos quedamos con los datos que nos importan
    if ( segment.intersects (segWall, riQuantity, afT) )
    {
      sSegmentIntersect newIntersect;
      
      newIntersect.m_IndexA = math::Min( i-1, i );
      newIntersect.m_IndexB = math::Max( i-1, i );
      newIntersect.afT = afT[0];
      
      // cambiamos el indice del ultimo nodo por el primero ya que estan repetidos en la lista de indices.
      
      if ( newIntersect.m_IndexB == points-1 ) {
        newIntersect.m_IndexB = newIntersect.m_IndexA;      // intercambiamos el orden ya que 0 es el indice mas pequeño
        newIntersect.m_IndexA = 0;
      }
      
      if ( afT[0] <= menorAft ) {
        indMenorAft = intersecciones.size();
        menorAft = afT[0];
      }
      
      if ( afT[0] > mayorAft ) {
        indMayorAft = intersecciones.size();
        mayorAft = afT[0];
      }
      
      intersecciones.push_back( newIntersect );
      
      bRet = true;
    }
  }
  
  if ( !intersecciones.empty() ) {
  
    // tenemos dos rutas, en sentidos diferentes, que recorrer. Elegimos una en base al numero de puntos
    
    int startPoint = intersecciones[ indMenorAft ].m_IndexA;
    int endPoint = intersecciones[ indMayorAft ].m_IndexA;

    int numIndicesCaso1 = abs( endPoint - startPoint );  // numero de nodos que nos encontramos en cada una de las dos rutas posibles
    int numIndicesCaso2 = points - numIndicesCaso1;      // 
    
    int step = 1;     // para el bucle de mas abajo
    
    // elegimos sentido que debemos seguir
    
    if ( startPoint < endPoint && (endPoint - startPoint) > float(points) * 0.5f ) {
    
      step = -1;
    }
    
    // ya tenemos un rango de indices, añadimos las posiciones a la ruta.
    // Al añadirlas realizamos una ultima comprobacion para quitar los puntos sobrantes. 
    // Si desde un punto añadido podemos llegar tranquilamente a nuestro destino sin intersectar, dejamos
    // de añadir
    
    if ( startPoint == endPoint )  {
    
      path->push_back( pointList[startPoint].m_Pos );
    
    } else {
    
      for (int iI = startPoint; iI != endPoint + step; iI += step ) {
        
        // para que sea circular
        if ( step == -1 ) {
          if ( iI < 0 ) {
            iI = points - 2;  // -2 porque el ultimo punto del sector es igual al primero
          } 
        } else {
          if ( iI == points-1 ) {
            iI = 0;
          }
        }
        
        path->push_back( pointList[iI].m_Pos );
        
        // construimos un segmento que va desde la posicion del punto añadido hasta la posicion destino
        CSegment2 testSegment( pointList[iI].m_Pos, ( segment.origin() + segment.direction() ) - pointList[iI].m_Pos );
        
        float oldSqLen = testSegment.direction().squaredLength();
        
        // solo hacemos el test con el sector actual.
        if ( !clipSegment( sector, testSegment ) ) {
          break;
        } else {
          
          float newSqLen = testSegment.direction().squaredLength();
          if ( newSqLen == oldSqLen ) {
            break;
          }
        }
      }    
    }
  }
  
  return bRet;
}


} // end namespace game