/**                                                                       
 * @file                                                                  
 * $Revision$
 * $Date$
 * $Id$
 * 
 *   Unless noted otherwise, the portions of Isis written by the USGS are 
 *   public domain. See individual third-party library and package descriptions 
 *   for intellectual property information, user agreements, and related  
 *   information.                                                         
 *                                                                        
 *   Although Isis has been used by the USGS, no warranty, expressed or   
 *   implied, is made by the USGS as to the accuracy and functioning of such 
 *   software and related material nor shall the fact of distribution     
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.                                        
 *                                                                        
 *   For additional information, launch                                   
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html                
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.                                    
 */

#include "BulletDskShape.h"

#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "NaifDskApi.h"

#include <QMutexLocker>
#include <QTime>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "NaifDskPlateModel.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * Default empty constructor.
   */
  BulletDskShape::BulletDskShape() :  m_mesh() { }


  /**
   * Construct a BulletDskShape from a DSK file.
   * 
   * @param dskfile The DSK file to load into a Bullet target shape.
   */
  BulletDskShape::BulletDskShape(const QString &dskfile) : m_mesh()  {
    loadFromDsk(dskfile);
    setMaximumDistance();
  }


  /**
   * Desctructor
   */
  BulletDskShape::~BulletDskShape() { }


  /**
   * Return the number of triangles in the shape
   * 
   * @return @b int The number of triangles. If nothing has been loaded, then 0 is returned.
   */
  int BulletDskShape::getNumTriangles() const {
    if (m_mesh) {
      return ( m_mesh->getIndexedMeshArray()[0].m_numTriangles );
    }

    return 0;
  }


  /** 
   * Return the number of verticies in the shape
   * 
   * @return @b int The number of verticies. If nothing has been loaded, then 0 is returned.
   */
  int BulletDskShape::getNumVertices() const {
    if (m_mesh) {
      return ( m_mesh->getIndexedMeshArray()[0].m_numVertices );
    }

    return 0;
  }


  /**
  * @brief Return normal for a given triangle index
  *  
  * This method is particularly useful to return the normal of a triangle plate 
  * in a mesh-based target body.  
  * 
  * @author 2017-03-28 Kris Becker 
  *  
  * @param indexId The index of the triangle in the mesh.
  * 
  * @return @b btVector3 The local normal for the triangle.
  */
  btVector3 BulletDskShape::getNormal(const int indexId) const {
    btMatrix3x3 triangle = getTriangle(indexId);
    btVector3 edge1 = triangle.getRow(1) - triangle.getRow(0);
    btVector3 edge2 = triangle.getRow(2) - triangle.getRow(0);
    return ( edge1.cross( edge2 ) );
  }


  /**
   * Get the vertices of a triangle in the mesh.
   * 
   * @param index The index of the triangle in the mesh.
   * 
   * @return @b btMatrix3x3 Matrix with each row containing the coordinate of a
   *                        vertex. The vertices are ordered counter-clockwise
   *                        around the surface normal of the triangle.
   */
  btMatrix3x3 BulletDskShape::getTriangle(const int index) const {
    btAssert ( index >= 0 );
    btAssert ( index < getNumTriangles() );

     // Set up pointers to triangle indexes
    const btIndexedMesh &v_mesh = m_mesh->getIndexedMeshArray()[0];

    const int *t_index = static_cast<int32_t *> ((void *) v_mesh.m_triangleIndexBase);
    int p_index = 3 * index;
    int vndx0 = t_index[p_index];
    int vndx1 = t_index[p_index+1];
    int vndx2 = t_index[p_index+2];

    const btScalar *t_vertex = static_cast<const btScalar *> ((void *) v_mesh.m_vertexBase);

    btMatrix3x3 triangle(t_vertex[vndx0+0], t_vertex[vndx0+1], t_vertex[vndx0+2], 
                         t_vertex[vndx1+0], t_vertex[vndx1+1], t_vertex[vndx1+2],
                         t_vertex[vndx2+0], t_vertex[vndx2+1], t_vertex[vndx2+2]);
    return ( triangle );
  }


/**
 * @brief Load the contents of a NAIF DSK and create a Bullet triangle mesh  
 * 
 * @author 2017-03-28 Kris Becker 
 * 
 * @param dskfile The DSK file to load.
 */
  void BulletDskShape::loadFromDsk(const QString &dskfile) {

    /** NAIF DSK parameter setup   */
    SpiceInt      v_handle;   //!< The DAS file handle of the DSK file.
    SpiceDLADescr v_dladsc;   /**< The DLA descriptor of the DSK segment representing the 
                                   target surface.*/
    SpiceDSKDescr v_dskdsc;   //!< The DSK descriptor.
    SpiceInt      v_plates;   //!< Number of Plates in the model.
    SpiceInt      v_vertices; //!< Number of vertices defining the plate.

    // Sanity check
    FileName dskFile(dskfile);
    if ( !dskFile.fileExists() ) {
      QString mess = "NAIF DSK file [" + dskfile + "] does not exist.";
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    // Open the NAIF Digital Shape Kernel (DSK)
    dasopr_c( dskFile.expanded().toLatin1().data(), &v_handle );
    NaifStatus::CheckErrors();
  
    // Search to the first DLA segment
    SpiceBoolean found;
    dlabfs_c( v_handle, &v_dladsc, &found );
    NaifStatus::CheckErrors();
    if ( !found ) {
      QString mess = "No segments found in DSK file " + dskfile ; 
      throw IException(IException::User, mess, _FILEINFO_);
    }

    dskgd_c( v_handle, &v_dladsc, &v_dskdsc );
    NaifStatus::CheckErrors();

    // Get size/counts
    dskz02_c( v_handle, &v_dladsc, &v_vertices, &v_plates );
    NaifStatus::CheckErrors();

    // Now allocate a new indexed mesh to contain all the DSK data
    btIndexedMesh i_mesh;
    m_mesh.reset( new btTriangleIndexVertexArray());
    m_mesh->addIndexedMesh(i_mesh, PHY_INTEGER);

    // Get internal mesh reference and set parameters appropriately
    btIndexedMesh &v_mesh = m_mesh->getIndexedMeshArray()[0];
    v_mesh.m_vertexType = PHY_DOUBLE;

    // Set and allocate data for triangle indexes
    v_mesh.m_numTriangles = v_plates;
    v_mesh.m_triangleIndexBase = new unsigned char[v_plates * 3 * sizeof(int)];
    v_mesh.m_triangleIndexStride = (sizeof(int) * 3);

    // Set and allocate vertex data
    v_mesh.m_numVertices = v_vertices;
    v_mesh.m_vertexBase = new unsigned char[v_vertices * 3 * sizeof(double)];
    v_mesh.m_vertexStride = (sizeof(double) * 3);

    SpiceInt n;
    (void) dskv02_c(v_handle, &v_dladsc, 1, v_vertices, &n, 
                    ( SpiceDouble(*)[3] ) (v_mesh.m_vertexBase));
    NaifStatus::CheckErrors();

    // Read the indexes from the DSK
    (void) dskp02_c(v_handle, &v_dladsc, 1, v_plates, &n, 
                    ( SpiceInt(*)[3] ) (v_mesh.m_triangleIndexBase));
    NaifStatus::CheckErrors();

    // Ok, close the DSK...
    dascls_c(v_handle);

    // Got to reset the vertex indexes to 0-based
    int *pindex = static_cast<int *> ((void *) v_mesh.m_triangleIndexBase);
    int nverts = v_plates * 3;
    for (int i = 0 ; i < nverts ; i++) {
      pindex[i] -= 1;
      btAssert ( pindex[i] >= 0 );
      btAssert ( pindex[i] < v_vertices );
    }

    bool useQuantizedAabbCompression = true;
    // bool useQuantizedAabbCompression = false;
    btBvhTriangleMeshShape *v_triShape = new btBvhTriangleMeshShape(m_mesh.data(), 
                                                                    useQuantizedAabbCompression);
    v_triShape->setUserPointer(this);
    btCollisionObject *vbody = new btCollisionObject();
    vbody->setCollisionShape(v_triShape);
    setTargetBody(vbody);

    return;

  }

}  // namespace Isis
