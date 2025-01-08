//__________________________________________________________________________________________
//
// File: Chunk.cpp
// Date: 06/2002
// Ruben Justo Ibañez <rUbO>
//__________________________________________________________________________________________
// 
// Manejo de chunks dentro de ficheros, tanto lectura como escritura
// TODO: Permitir anidad ChunkReader!!
//__________________________________________________________________________________________

        #include "__PCH_File.h"
        #include "Chunk.h"
        #include "IFile.h"

#include "dbg/SafeMem.h"      // Memoria Segura: Debe ser el ultimo include!

namespace fileSYS { 

//__________________________________________________________________________________________
// 
//                      CHUNK READER --> permite anidar los chunks
//__________________________________________________________________________________________

CChunkReader::CChunkReader (IFile * hFile)
{
  assert (hFile);
  m_File = hFile;
  m_Chunk.reserve (1);
}
CChunkReader::~CChunkReader (void)
{
  assert (m_Chunk.size () == 0);
}
//__________________________________________________________________________________________
// 
// Comienza la lectura de un Chunk; cada chunk debe finalizarse con un end
//__________________________________________________________________________________________

DWORD CChunkReader::begin (void)
{
  assert (m_File);
      
  SDesc chunk;
  chunk.m_Start = m_File->tell ();
  m_File->read (&chunk.m_ID, sizeof (DWORD) );      // Lee el identificador
  m_File->read (&chunk.m_Size, sizeof (UINT) );     // Lee el tamano
  m_Chunk.push_back (chunk);                        // Insertamos el chunk dentro de la pila
  return chunk.m_ID;
}

//__________________________________________________________________________________________
// 
// Finaliza la lectura del chunk : salta al final del chunk opcionalmente
//__________________________________________________________________________________________

void CChunkReader::end (bool bSeekEnd)
{
  assert (m_File);
  assert (m_Chunk.size () > 0);
  
  if (bSeekEnd)
  {
    SDesc & chunk = m_Chunk [m_Chunk.size ()-1];
    m_File->seek (fileSYS::BEGIN, chunk.m_Start+chunk.m_Size);
  }
  m_Chunk.pop_back ();    // Extraemos el chunk
}

//__________________________________________________________________________________________
// 
//                      CHUNK WRITER --> permite anidar los chunks
//__________________________________________________________________________________________

CChunkWriter::CChunkWriter (IFile * hFile)
{
  assert (hFile);
  m_File = hFile;
  m_Chunk.reserve (1);
}
CChunkWriter::~CChunkWriter (void)
{
  assert (m_Chunk.size () == 0);
}

//__________________________________________________________________________________________
// 
// Comienza la escritura de un Chunk : Los chunks pueden ser anidados; pero por cada begin
// debe existir un end
//__________________________________________________________________________________________

void CChunkWriter::begin (DWORD ID)
{
  assert (m_File);

  SDesc chunk;
  chunk.m_Start = m_File->tell ();
  chunk.m_ID = ID;
  chunk.m_Size = 0;
  m_File->write (&ID, sizeof (DWORD) );               // Escribe el identificador
  m_File->write (&chunk.m_Size, sizeof (UINT) );      // Escribe el tamano (de momento no se conoce)
  
  m_Chunk.push_back (chunk);                          // Insertamos el chunk en la pila
}

//__________________________________________________________________________________________
// 
// Finaliza la escritura del ultimo chunk iniciado
//__________________________________________________________________________________________

void CChunkWriter::end (void)
{
  assert (m_File);
  assert (m_Chunk.size ()>0);   // Este "end" no pertenece a ningun begin
  
  SDesc & chunk = m_Chunk [m_Chunk.size ()-1];
  
  // Calculamos tamaño del chunk y volvemos al principio de este

  chunk.m_Size = m_File->tell () - chunk.m_Start;
  m_File->seek (fileSYS::BEGIN, chunk.m_Start);
  
  // Escribimos el chunk ya sabiendo el size real

  m_File->write (&chunk.m_ID, sizeof (DWORD) );     // Escribe el identificador
  m_File->write (&chunk.m_Size, sizeof (UINT) );    // Escribe el tamano

  // Volvemos al final
  
  m_File->seek (fileSYS::BEGIN, chunk.m_Start+chunk.m_Size);
  
  // Extraemos el chunk de la pila
  m_Chunk.pop_back ();
}

/*
//__________________________________________________________________________________________
// 
// Comienza la lectura de un Chunk
//__________________________________________________________________________________________

DWORD CChunkReader::begin (IFile *hFile)
{
    assert (hFile);
        
    m_File = hFile;
    
    m_Start = m_File->tell ();
    m_File->read (&m_ID, sizeof (DWORD) );      // Lee el identificador
    m_File->read (&m_Size, sizeof (UINT) );     // Lee el tamano

    return m_ID;
}

//__________________________________________________________________________________________
// 
// Finaliza la lectura del chunk : salta el chunk
//__________________________________________________________________________________________

void CChunkReader::end (void)
{
    assert (m_File);
    m_File->seek (fileSYS::BEGIN, m_Start+m_Size);
}
*/
/*
//__________________________________________________________________________________________
// 
// Comienza la escritura de un Chunk
//__________________________________________________________________________________________

void CChunkWriter::begin (IFile *hFile, DWORD ID)
{
    assert (hFile);

    m_File = hFile;
    m_Start = m_File->tell ();
    m_ID = ID;
    m_Size = 0;
    m_File->write (&m_ID, sizeof (DWORD) ); // Escribe el identificador
    m_File->write (&m_Size, sizeof (UINT) );    // Escribe el tamano
}

//__________________________________________________________________________________________
// 
// Finaliza la escritura del chunk
//__________________________________________________________________________________________

void CChunkWriter::end (void)
{
    assert (m_File);

    // Calculamos tamaño del chunk y volvemos al principio de este

    m_Size = m_File->tell () - m_Start;
    m_File->seek (fileSYS::BEGIN, m_Start);
    
    // Escribimos el chunk ya sabiendo el size real

    m_File->write (&m_ID, sizeof (DWORD) ); // Escribe el identificador
    m_File->write (&m_Size, sizeof (UINT) );    // Escribe el tamano

    // Volvemos al final
    
    m_File->seek (fileSYS::BEGIN, m_Start+m_Size);
}
*/



}   // end namespace fileSYS