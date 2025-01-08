#pragma once
//----------------------------------------------------------------------------------------------------------------
// File: LVLFormat.h
// Date: 1/2003
// Ruben Justo Ibañez <rUbO>
//----------------------------------------------------------------------------------------------------------------
// Formato .LVL utilizado para la descripcion de niveles
//  Un nivel se compone de :
//    - Mapa fisico (Terreno + objetos)
//    - Mapa logico (Terreno logico + logica de objetos
// Historial de versiones
//
//  v.108           12nov2003
//    --> Nuevo sistema de posiciones de tropas con filas y columnas
//  v.133           05feb2004 Ricard
//    --> Nuevos tipos de LOCs
//  v.134           13feb2004 Ricard
//    --> Exportacion de settings de sonido
//  v.140           15 Abril 2004 Agre
//    --> Exportación del grafo de la IA
//  v.150           18 Junio 2004 Rob
//    --> Nuevos datos de construcciones y volumenes
//  v.160           24 Agosto 2004 Agre
//    --> Geometría específica para alturas especiales del escenario, marcada por propiedad de sector
//  v.170           07 Septiembre 2004 Rob :)
//    --> Angulos e info de flancos por módulos. Red neuronal asociada al cursor y sistema de path finding para iconos
//  v.171           15 Septiembre 2004 Rob :)
//    --> Tiempo de espera y nuevo objetivo de mapa (mantener tropas)
//  v.172           25 Octubre 2004 Rob :)
//    --> Mariconadillas de SLocations en el mapa lógico (.Name)
//  v.180           4 Enero 2005 Rob :)
//    --> Soporte para varias posEspera, ubicHumo VERSION CON POSHUMOS EN MALA POSICION
//  v.181           13 Enero 2005 Rob :)
//    --> Corregido el error de las ubicHumo
//  v.185           15 Enero 2005 Rob :)
//    --> Planos de alturas en el mapa lógico
//  v.186           11 Febrero 2005 Rubo :)
//    --> Añadido YBias para las locations
//----------------------------------------------------------------------------------------------------------------

namespace LVLFMT
{
  #pragma pack (push, 4)
  
  enum { VERSION = 186 };
  
  enum eID
  {
      ID_HEADER                   =       0x0EBAC,    // Cabecera (siempre el el primer chunk)
      
      ID_PHYSICMAP                =       0x0A000,    // Mapa fisico
        ID_TERRAIN                =       0x0A100,    // Terreno
        ID_HEIGHTMAP              =       0x0A300,    // Mapa de alturas
        ID_ENDPHYSIC              =       0x0AFFF,    // Fin de mapa fisico
      
      ID_OBJECTLIST               =       0x01000,    // Lista de objetos
      
      ID_LOGIC                    =       0x0B000,    // Informacion logica
        ID_LOGICMAP               =       0x0B100,    // Comienzo del mapa logico
          ID_TERRAINLOGIC         =       0x0B200,    // Informacion logica del celdas del terreno
          ID_POSTROOPER           =       0x0B210,    // Lista de posiciones de tropas
          ID_LOCATIONS            =       0x0B220,    // Lista de locations
          ID_ZONES                =       0x0B230,    // Lista de zonas
          ID_ARBITRARYZONES       =       0x0B240,    // Lista de zonas arbitrarias
          ID_SEGMENTMAP           =       0x0B250,    // Mapa de segmentos (para colisiones)
          ID_SECTORMAP            =       0x0B260,    // Mapa de sectores
          ID_GRAFOIA              =       0x0B270,    // Grafo de la IA
        ID_ENDLOGICMAP            =       0x0B1FF,    // Fin de mapa logico
      ID_ENDLOGIC                 =       0x0BFFF,    // Fin de logica
      
      ID_ENVIRONMENT              =       0x0C000,    // Descripcion del environment

      ID_SOUND                    =       0x0D000,    // Descripcion de los sonidos
      
      ID_CONDICIONES              =       0x0E000,    // Condiciones de victoria de mapa
      
      ID_END                      =       0x000FF,    // Identificador de chunk-final
      ID_FORCEDWORD               =       0x7FFFFFFF
  };
  
  #pragma pack (1)
  struct SHeader
  {
    DWORD Version;              // Version * 100
    BYTE Reserved [124];        // 124 bytes reservados para uso futuro
  };
  
  #pragma pack (pop)

};
