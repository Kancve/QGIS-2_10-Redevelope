/***************************************************************************
     testqgssnappingutils.cpp
     --------------------------------------
    Date                 : November 2014
    Copyright            : (C) 2014 by Martin Dobias
    Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtTest/QtTest>
#include <QObject>
#include <QString>

#include "qgsapplication.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsgeometry.h"
#include "qgsmaplayerregistry.h"
#include "qgssnappingutils.h"


struct FilterExcludePoint : public QgsPointLocator::MatchFilter
{
  FilterExcludePoint( const QgsPoint& p ) : mPoint( p ) {}

  bool acceptMatch( const QgsPointLocator::Match& match ) { return match.point() != mPoint; }

  QgsPoint mPoint;
};


class TestQgsSnappingUtils : public QObject
{
    Q_OBJECT
  public:
    TestQgsSnappingUtils()
        : mVL( 0 )
    {}

  private:
    QgsVectorLayer* mVL;
  private slots:

    void initTestCase()
    {
      QgsApplication::init();
      QgsApplication::initQgis();
      // Will make sure the settings dir with the style file for color ramp is created
      QgsApplication::createDB();
      QgsApplication::showSettings();

      // vector layer with a triangle:
      // (0,1) +---+ (1,1)
      //        \  |
      //         \ |
      //          \|
      //           + (1,0)
      mVL = new QgsVectorLayer( "Polygon", "x", "memory" );
      QgsFeature ff( 0 );
      QgsPolygon polygon;
      QgsPolyline polyline;
      polyline << QgsPoint( 0, 1 ) << QgsPoint( 1, 0 ) << QgsPoint( 1, 1 ) << QgsPoint( 0, 1 );
      polygon << polyline;
      ff.setGeometry( QgsGeometry::fromPolygon( polygon ) );
      QgsFeatureList flist;
      flist << ff;
      mVL->dataProvider()->addFeatures( flist );

      QgsMapLayerRegistry::instance()->addMapLayer( mVL );
    }

    void cleanupTestCase()
    {
      QgsApplication::exitQgis();
    }

    void testSnapModeCurrent()
    {
      QgsMapSettings mapSettings;
      mapSettings.setOutputSize( QSize( 100, 100 ) );
      mapSettings.setExtent( QgsRectangle( 0, 0, 1, 1 ) );
      QVERIFY( mapSettings.hasValidSettings() );

      QgsSnappingUtils u;
      u.setMapSettings( mapSettings );
      u.setCurrentLayer( mVL );

      // first try with no snapping enabled
      u.setDefaultSettings( 0, 10, QgsTolerance::Pixels );

      QgsPointLocator::Match m0 = u.snapToMap( QPoint( 100, 100 ) );
      QVERIFY( !m0.isValid() );
      QVERIFY( !m0.hasVertex() );

      // now enable snapping
      u.setDefaultSettings( QgsPointLocator::Vertex | QgsPointLocator::Edge, 10, QgsTolerance::Pixels );

      QgsPointLocator::Match m = u.snapToMap( QPoint( 100, 100 ) );
      QVERIFY( m.isValid() );
      QVERIFY( m.hasVertex() );
      QCOMPARE( m.point(), QgsPoint( 1, 0 ) );

      QgsPointLocator::Match m2 = u.snapToMap( QPoint( 0, 100 ) );
      QVERIFY( !m2.isValid() );
      QVERIFY( !m2.hasVertex() );

      // do not consider edges in the following test - on 32-bit platforms
      // result was an edge match very close to (1,0) instead of being exactly (1,0)
      u.setDefaultSettings( QgsPointLocator::Vertex, 10, QgsTolerance::Pixels );

      // test with filtering
      FilterExcludePoint myFilter( QgsPoint( 1, 0 ) );
      QgsPointLocator::Match m3 = u.snapToMap( QPoint( 100, 100 ), &myFilter );
      QVERIFY( !m3.isValid() );
    }

    void testSnapModeAll()
    {
      QgsMapSettings mapSettings;
      mapSettings.setOutputSize( QSize( 100, 100 ) );
      mapSettings.setExtent( QgsRectangle( 0, 0, 1, 1 ) );
      QVERIFY( mapSettings.hasValidSettings() );

      QgsSnappingUtils u;
      u.setMapSettings( mapSettings );
      u.setSnapToMapMode( QgsSnappingUtils::SnapAllLayers );

      // right now there are no layers in map settings - snapping will fail

      QgsPointLocator::Match m = u.snapToMap( QPoint( 100, 100 ) );
      QVERIFY( !m.isValid() );

      // now check with our layer
      mapSettings.setLayers( QStringList() << mVL->id() );
      u.setMapSettings( mapSettings );

      QgsPointLocator::Match m2 = u.snapToMap( QPoint( 100, 100 ) );
      QVERIFY( m2.isValid() );
      QCOMPARE( m2.point(), QgsPoint( 1, 0 ) );
    }

    void testSnapModeAdvanced()
    {
      QgsMapSettings mapSettings;
      mapSettings.setOutputSize( QSize( 100, 100 ) );
      mapSettings.setExtent( QgsRectangle( 0, 0, 1, 1 ) );
      QVERIFY( mapSettings.hasValidSettings() );

      QgsSnappingUtils u;
      u.setMapSettings( mapSettings );
      u.setSnapToMapMode( QgsSnappingUtils::SnapAdvanced );
      QList<QgsSnappingUtils::LayerConfig> layers;
      layers << QgsSnappingUtils::LayerConfig( mVL, QgsPointLocator::Vertex, 10, QgsTolerance::Pixels );
      u.setLayers( layers );

      QgsPointLocator::Match m = u.snapToMap( QPoint( 100, 100 ) );
      QVERIFY( m.isValid() );
      QVERIFY( m.hasVertex() );
      QCOMPARE( m.point(), QgsPoint( 1, 0 ) );

      // test with filtering
      FilterExcludePoint myFilter( QgsPoint( 1, 0 ) );
      QgsPointLocator::Match m2 = u.snapToMap( QPoint( 100, 100 ), &myFilter );
      QVERIFY( !m2.isValid() );
    }

    void testSnapOnIntersection()
    {
      // testing with a layer with two crossing linestrings
      // (0,1)  x  x (1,1)
      //         \/
      //         /\    .
      // (0,0)  x  x (1,0)
      QgsVectorLayer* vl = new QgsVectorLayer( "LineString", "x", "memory" );
      QgsPolyline polyline1, polyline2;
      polyline1 << QgsPoint( 0, 0 ) << QgsPoint( 1, 1 );
      polyline2 << QgsPoint( 1, 0 ) << QgsPoint( 0, 1 );
      QgsFeature f1;
      f1.setGeometry( QgsGeometry::fromPolyline( polyline1 ) );
      QgsFeature f2;
      f2.setGeometry( QgsGeometry::fromPolyline( polyline2 ) );
      QgsFeatureList flist;
      flist << f1 << f2;
      vl->dataProvider()->addFeatures( flist );

      QVERIFY( vl->dataProvider()->featureCount() == 2 );

      QgsMapSettings mapSettings;
      mapSettings.setOutputSize( QSize( 100, 100 ) );
      mapSettings.setExtent( QgsRectangle( 0, 0, 1, 1 ) );
      QVERIFY( mapSettings.hasValidSettings() );

      QgsSnappingUtils u;
      u.setMapSettings( mapSettings );
      u.setSnapToMapMode( QgsSnappingUtils::SnapAdvanced );
      QList<QgsSnappingUtils::LayerConfig> layers;
      layers << QgsSnappingUtils::LayerConfig( vl, QgsPointLocator::Vertex, 0.1, QgsTolerance::MapUnits );
      u.setLayers( layers );

      // no snapping on intersections by default - should find nothing
      QgsPointLocator::Match m = u.snapToMap( QgsPoint( 0.45, 0.5 ) );
      QVERIFY( !m.isValid() );

      u.setSnapOnIntersections( true );

      QgsPointLocator::Match m2 = u.snapToMap( QgsPoint( 0.45, 0.5 ) );
      QVERIFY( m2.isValid() );
      QCOMPARE( m2.type(), QgsPointLocator::Vertex );
      QCOMPARE( m2.point(), QgsPoint( 0.5, 0.5 ) );

      delete vl;
    }
};

QTEST_MAIN( TestQgsSnappingUtils )

#include "testqgssnappingutils.moc"


