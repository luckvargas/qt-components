/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsggeometry.h"

QT_BEGIN_NAMESPACE


/*!
    Convenience function which returns attributes to be used for 2D solid
    color drawing.
 */

const QSGGeometry::AttributeSet &QSGGeometry::defaultAttributes_Point2D()
{
    static Attribute data[] = {
        { 0, 2, GL_FLOAT }
    };
    static AttributeSet attrs = { 1, sizeof(float) * 2, data };
    return attrs;
}

/*!
    Convenience function which returns attributes to be used for textured 2D drawing.
 */

const QSGGeometry::AttributeSet &QSGGeometry::defaultAttributes_TexturedPoint2D()
{
    static Attribute data[] = {
        { 0, 2, GL_FLOAT },
        { 1, 2, GL_FLOAT }
    };
    static AttributeSet attrs = { 2, sizeof(float) * 4, data };
    return attrs;
}

/*!
    Convenience function which returns attributes to be used for per vertex colored 2D drawing.
 */

const QSGGeometry::AttributeSet &QSGGeometry::defaultAttributes_ColoredPoint2D()
{
    static Attribute data[] = {
        { 0, 2, GL_FLOAT },
        { 1, 4, GL_UNSIGNED_BYTE }
    };
    static AttributeSet attrs = { 2, 2 * sizeof(float) + 4 * sizeof(char), data };
    return attrs;
}


/*!
    \class QSGGeometry
    \brief The QSGGeometry class provides low-level storage for graphics primitives
    in the QML Scene Graph.

    The QSGGeometry class provides a few convenience attributes and attribute accessors
    by default. The defaultAttributes_Point2D() function returns attributes to be used
    in normal solid color rectangles, while the defaultAttributes_TexturedPoint2D function
    returns attributes to be used for the common pixmap usecase.
 */


/*!
    Constructs a geometry object based on \a attributes.

    The object allocate space for \a vertexCount vertices based on the accumulated
    size in \a attributes and for \a indexCount.

    Geometry objects are constructed with GL_TRIANGLE_STRIP as default drawing mode.

    The attribute structure is assumed to be POD and the geometry object
    assumes this will not go away. There is no memory management involved.
 */

QSGGeometry::QSGGeometry(const QSGGeometry::AttributeSet &attributes,
                         int vertexCount,
                         int indexCount,
                         int indexType)
    : m_drawing_mode(GL_TRIANGLE_STRIP)
    , m_vertex_count(0)
    , m_index_count(0)
    , m_index_type(indexType)
    , m_attributes(attributes)
    , m_data(0)
    , m_index_data_offset(-1)
    , m_owns_data(false)
{
    Q_ASSERT(m_attributes.count > 0);
    Q_ASSERT(m_attributes.stride > 0);

    // Because allocate reads m_vertex_count, m_index_count and m_owns_data, these
    // need to be set before calling allocate...
    allocate(vertexCount, indexCount);
}

QSGGeometry::~QSGGeometry()
{
    if (m_owns_data)
        qFree(m_data);
}

/*!
    \fn int QSGGeometry::vertexCount() const

    Returns the number of vertices in this geometry object.
 */

/*!
    \fn int QSGGeometry::indexCount() const

    Returns the number of indices in this geometry object.
 */



/*!
    \fn void *QSGGeometry::vertexData()

    Returns a pointer to the raw vertex data of this geometry object.

    \sa vertexDataAsPoint2D(), vertexDataAsTexturedPoint2D
 */

/*!
    \fn const void *QSGGeometry::vertexData() const

    Returns a pointer to the raw vertex data of this geometry object.

    \sa vertexDataAsPoint2D(), vertexDataAsTexturedPoint2D
 */

/*!
    Returns a pointer to the raw index data of this geometry object.

    \sa indexDataAsUShort(), indexDataAsUInt()
 */
void *QSGGeometry::indexData()
{
    return m_index_data_offset < 0
            ? 0
            : ((char *) m_data + m_index_data_offset);
}

/*!
    Returns a pointer to the raw index data of this geometry object.

    \sa indexDataAsUShort(), indexDataAsUInt()
 */
const void *QSGGeometry::indexData() const
{
    return m_index_data_offset < 0
            ? 0
            : ((char *) m_data + m_index_data_offset);
}

/*!
    Sets the drawing mode to be used for this geometry.

    The default value is GL_TRIANGLE_STRIP.
 */
void QSGGeometry::setDrawingMode(GLenum mode)
{
    m_drawing_mode = mode;
}

/*!
    \fn int QSGGeometry::drawingMode() const

    Returns the drawing mode of this geometry.

    The default value is GL_TRIANGLE_STRIP.
 */

/*!
    \fn int QSGGeometry::indexType() const

    Returns the primitive type used for indices in this
    geometry object.
 */


/*!
    Resizes the vertex and index data of this geometry object to fit \a vertexCount
    vertices and \a indexCount indices.

    Vertex and index data will be invalidated after this call and the caller must
 */
void QSGGeometry::allocate(int vertexCount, int indexCount)
{
    if (vertexCount == m_vertex_count && indexCount == m_index_count)
        return;

    m_vertex_count = vertexCount;
    m_index_count = indexCount;

    bool canUsePrealloc = m_index_count <= 0;
    int vertexByteSize = m_attributes.stride * m_vertex_count;

    if (m_owns_data)
        qFree(m_data);

    if (canUsePrealloc && vertexByteSize <= (int) sizeof(m_prealloc)) {
        m_data = (void *) &m_prealloc[0];
        m_index_data_offset = -1;
        m_owns_data = false;
    } else {
        Q_ASSERT(m_index_type == GL_UNSIGNED_INT || m_index_type == GL_UNSIGNED_SHORT);
        int indexByteSize = indexCount * (m_index_type == GL_UNSIGNED_SHORT ? sizeof(quint16) : sizeof(quint32));
        m_data = (void *) qMalloc(vertexByteSize + indexByteSize);
        m_index_data_offset = vertexByteSize;
        m_owns_data = true;
    }

}

/*!
    Updates the geometry \a g with the coordinates in \a rect.

    The function assumes the geometry object contains a single triangle strip
    of QSGGeometry::Point2D vertices
 */
void QSGGeometry::updateRectGeometry(QSGGeometry *g, const QRectF &rect)
{
    Point2D *v = g->vertexDataAsPoint2D();
    v[0].x = rect.left();
    v[0].y = rect.top();

    v[1].x = rect.right();
    v[1].y = rect.top();

    v[2].x = rect.left();
    v[2].y = rect.bottom();

    v[3].x = rect.right();
    v[3].y = rect.bottom();
}

/*!
    Updates the geometry \a g with the coordinates in \a rect and texture
    coordinates from \a textureRect.

    \a textureRect should be in normalized coordinates.

    \a g is assumed to be a triangle strip of four vertices of type
    QSGGeometry::TexturedPoint2D.
 */
void QSGGeometry::updateTexturedRectGeometry(QSGGeometry *g, const QRectF &rect, const QRectF &textureRect)
{
    TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();
    v[0].x = rect.left();
    v[0].y = rect.top();
    v[0].tx = textureRect.left();
    v[0].ty = textureRect.top();

    v[1].x = rect.right();
    v[1].y = rect.top();
    v[1].tx = textureRect.right();
    v[1].ty = textureRect.top();

    v[2].x = rect.left();
    v[2].y = rect.bottom();
    v[2].tx = textureRect.left();
    v[2].ty = textureRect.bottom();

    v[3].x = rect.right();
    v[3].y = rect.bottom();
    v[3].tx = textureRect.right();
    v[3].ty = textureRect.bottom();
}

QT_END_NAMESPACE
