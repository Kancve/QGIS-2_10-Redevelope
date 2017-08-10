/***************************************************************************
 qgsscaleexpression.cpp
 ---------------------
 begin                : November 2014
 copyright            : (C) 2014 by Vincent Mora
 email                : vincent dor mora at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsscaleexpression.h"
#include "qgis.h"
#include <QStringList>
#include <qmath.h>

QgsScaleExpression::QgsScaleExpression( const QString& expression )
    : QgsExpression( expression )
    , mType( Unknown )
    , mMinSize( 0 )
    , mMaxSize( 10 )
    , mMinValue( 0 )
    , mMaxValue( 100 )
    , mNullSize( 0 )
{
  init();
}

QgsScaleExpression::QgsScaleExpression( Type type, const QString& baseExpression, double minValue, double maxValue, double minSize, double maxSize, double nullSize )
    : QgsExpression( createExpression( type, baseExpression, minValue, maxValue, minSize, maxSize, nullSize ) )
    , mExpression( baseExpression )
    , mType( type )
    , mMinSize( minSize )
    , mMaxSize( maxSize )
    , mMinValue( minValue )
    , mMaxValue( maxValue )
    , mNullSize( nullSize )
{

}

void QgsScaleExpression::init()
{
  bool ok;
  mType = Unknown;

  if ( !rootNode() )
    return;

  const NodeFunction * f = dynamic_cast<const NodeFunction*>( rootNode() );
  if ( !f )
    return;

  QList<Node*> args = f->args()->list();

  // the scale function may be enclosed in a coalesce(expr, 0) to avoid NULL value
  // to be drawn with the default size
  if ( "coalesce" == Functions()[f->fnIndex()]->name() )
  {
    f = dynamic_cast<const NodeFunction*>( args[0] );
    if ( !f )
      return;
    mNullSize = QgsExpression( args[1]->dump() ).evaluate().toDouble( &ok );
    if ( ! ok )
      return;
    args = f->args()->list();
  }

  if ( "scale_linear" == Functions()[f->fnIndex()]->name() )
  {
    mType = Linear;
  }
  else if ( "scale_exp" == Functions()[f->fnIndex()]->name() )
  {
    const double exp = QgsExpression( args[5]->dump() ).evaluate().toDouble( &ok );
    if ( ! ok )
      return;
    if ( qgsDoubleNear( exp, 0.57, 0.001 ) )
      mType = Flannery;
    else if ( qgsDoubleNear( exp, 0.5, 0.001 ) )
      mType = Area;
    else
    {
      return;
    }
  }
  else
  {
    return;
  }

  bool expOk = true;
  mMinValue = QgsExpression( args[1]->dump() ).evaluate().toDouble( &ok );
  expOk &= ok;
  mMaxValue = QgsExpression( args[2]->dump() ).evaluate().toDouble( &ok );
  expOk &= ok;
  mMinSize = QgsExpression( args[3]->dump() ).evaluate().toDouble( &ok );
  expOk &= ok;
  mMaxSize = QgsExpression( args[4]->dump() ).evaluate().toDouble( &ok );
  expOk &= ok;

  if ( !expOk )
  {
    mType = Unknown;
    return;
  }
  mExpression = args[0]->dump();
}

QString QgsScaleExpression::createExpression( Type type, const QString & baseExpr, double minValue, double maxValue, double minSize, double maxSize, double nullSize )
{
  QString minValueString = QString::number( minValue );
  QString maxValueString = QString::number( maxValue );
  QString minSizeString = QString::number( minSize );
  QString maxSizeString = QString::number( maxSize );
  QString nullSizeString = QString::number( nullSize );

  switch ( type )
  {
    case Linear:
      return QString( "coalesce(scale_linear(%1, %2, %3, %4, %5), %6)" ).arg( baseExpr, minValueString, maxValueString, minSizeString, maxSizeString, nullSizeString );

    case Area:
      return QString( "coalesce(scale_exp(%1, %2, %3, %4, %5, 0.5), %6)" ).arg( baseExpr, minValueString, maxValueString, minSizeString, maxSizeString, nullSizeString );

    case Flannery:
      return QString( "coalesce(scale_exp(%1, %2, %3, %4, %5, 0.57), %6)" ).arg( baseExpr, minValueString, maxValueString, minSizeString, maxSizeString, nullSizeString );

    case Unknown:
      break;
  }
  return QString();
}

double QgsScaleExpression::size( double value ) const
{
  switch ( mType )
  {
    case Linear:
      return mMinSize + ( qBound( mMinValue, value, mMaxValue ) - mMinValue ) * ( mMaxSize - mMinSize ) / ( mMaxValue - mMinValue );

    case Area:
      return mMinSize + qPow( qBound( mMinValue, value, mMaxValue ) - mMinValue, .5 ) * ( mMaxSize - mMinSize ) / qPow( mMaxValue - mMinValue, .5 );

    case Flannery:
      return mMinSize + qPow( qBound( mMinValue, value, mMaxValue ) - mMinValue, .57 ) * ( mMaxSize - mMinSize ) / qPow( mMaxValue - mMinValue, .57 );

    case Unknown:
      break;
  }
  return 0;
}
