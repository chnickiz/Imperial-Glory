//__________________________________________________________________________________________
//
// File: Chunk.h
// Date: 06/2002
// Ruben Justo Ibañez <rUbO>
//__________________________________________________________________________________________
// 
// Manejo de chunks dentro de ficheros, tanto lectura como escritura
//__________________________________________________________________________________________

#ifndef __CHUNK_H__
#define __CHUNK_H__

    
namespace fileSYS { 

class IFile;

class CChunkReader
{
private:

  struct SDesc
  {
    DWORD   m_ID;                       // Identificador de tipo de chunk
    UINT    m_Start;                    // Offset dentro del fichero donde comienza el Chunk
    UINT    m_Size;                     // Tamaño que ocupa el chunk (incluida la definicion del chunk)
  };
  
  IFile *             m_File;           // Fichero utilizado
  std::vector <SDesc> m_Chunk;          // Descriptor de chunk
  
public:

  CChunkReader (IFile * hFile);
  ~CChunkReader (void);
  
  // Recuperacion de datos

  inline DWORD getID (void) const         { return m_Chunk [m_Chunk.size ()-1].m_ID;      }
  inline UINT getStart (void) const       { return m_Chunk [m_Chunk.size ()-1].m_Start;   }
  inline UINT getSize (void) const        { return m_Chunk [m_Chunk.size ()-1].m_Size;    }

  // Interface
  
  DWORD begin (void);
  void end (bool bSeekEnd = true);
};

class CChunkWriter
{
private:

  struct SDesc
  {
    DWORD   m_ID;                       // Identificador de tipo de chunk
    UINT    m_Start;                    // Offset dentro del fichero donde comienza el Chunk
    UINT    m_Size;                     // Tamaño que ocupa el chunk (incluida la definicion del chunk)
  };
  
  IFile *             m_File;           // Fichero utilizado
  std::vector <SDesc> m_Chunk;          // Descriptor de chunk

public:

  CChunkWriter (IFile * hFile);
  ~CChunkWriter (void);
  
  void begin (DWORD ID);
  void end (void);

  inline DWORD getID (void) const         { return m_Chunk [m_Chunk.size ()-1].m_ID;      }
  inline UINT getStart (void) const       { return m_Chunk [m_Chunk.size ()-1].m_Start;   }
};

/*
class CChunkReader
{
private:

    DWORD   m_ID;                       // Identificador de tipo de chunk
    UINT    m_Start;                    // Offset dentro del fichero donde comienza el Chunk
    UINT    m_Size;                     // Tamaño que ocupa el chunk (incluida la definicion del chunk)
    IFile   * m_File;                   // Fichero utilizado

public:

    // Recuperacion de datos

    inline DWORD getID (void) const         { return m_ID;      }
    inline UINT getStart (void) const       { return m_Start;   }
    inline UINT getSize (void) const        { return m_Size;    }

    // Interface
    
    DWORD begin (IFile *hFile);
    void end (void);
};
*/
/*
class CChunkWriter
{
private:

    DWORD   m_ID;                       // Identificador de tipo de chunk
    UINT    m_Start;                    // Offset dentro del fichero donde comienza el Chunk
    UINT    m_Size;                     // Tamaño que ocupa el chunk (incluida la definicion del chunk)
    IFile   * m_File;                   // Fichero utilizado

public:

    void begin (IFile *hFile, DWORD ID);
    void end (void);

    inline DWORD getID (void) const         { return m_ID;      }
    inline UINT getStart (void) const       { return m_Start;   }
};
*/



}   // end namespace

#endif