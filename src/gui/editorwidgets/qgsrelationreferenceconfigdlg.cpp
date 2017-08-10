/***************************************************************************
    qgsrelationreferenceconfigdlg.cpp
     --------------------------------------
    Date                 : 21.4.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrelationreferenceconfigdlg.h"

#include "qgseditorwidgetfactory.h"
#include "qgsfield.h"
#include "qgsproject.h"
#include "qgsrelationmanager.h"
#include "qgsvectorlayer.h"
#include "qgsexpressionbuilderdialog.h"


QgsRelationReferenceConfigDlg::QgsRelationReferenceConfigDlg( QgsVectorLayer* vl, int fieldIdx, QWidget* parent )
    : QgsEditorConfigWidget( vl, fieldIdx, parent )
    , mReferencedLayer( 0 )
{
  setupUi( this );
  connect( mComboRelation, SIGNAL( currentIndexChanged( int ) ), this, SLOT( relationChanged( int ) ) );

  foreach ( const QgsRelation& relation, vl->referencingRelations( fieldIdx ) )
  {
    mComboRelation->addItem( QString( "%1 (%2)" ).arg( relation.id(), relation.referencedLayerId() ), relation.id() );
    if ( relation.referencedLayer() )
    {
      mExpressionWidget->setField( relation.referencedLayer()->displayExpression() );
    }
  }
}

void QgsRelationReferenceConfigDlg::setConfig( const QMap<QString, QVariant>& config )
{
  if ( config.contains( "AllowNULL" ) )
  {
    mCbxAllowNull->setChecked( config[ "AllowNULL" ].toBool() );
  }

  if ( config.contains( "OrderByValue" ) )
  {
    mCbxOrderByValue->setChecked( config[ "OrderByValue" ].toBool() );
  }

  if ( config.contains( "ShowForm" ) )
  {
    mCbxShowForm->setChecked( config[ "ShowForm" ].toBool() );
  }

  if ( config.contains( "Relation" ) )
  {
    mComboRelation->setCurrentIndex( mComboRelation->findData( config[ "Relation" ].toString() ) );
    relationChanged( mComboRelation->currentIndex() );
  }

  if ( config.contains( "MapIdentification" ) )
  {
    mCbxMapIdentification->setChecked( config[ "MapIdentification"].toBool() );
  }

  if ( config.contains( "ReadOnly" ) )
  {
    mCbxReadOnly->setChecked( config[ "ReadOnly"].toBool() );
  }

  if ( config.contains( "FilterFields" ) )
  {
    mFilterGroupBox->setChecked( true );
    Q_FOREACH ( const QString& fld, config["FilterFields"].toStringList() )
    {
      addFilterField( fld );
    }

    mCbxChainFilters->setChecked( config["ChainFilters"].toBool() );
  }
}

void QgsRelationReferenceConfigDlg::relationChanged( int idx )
{
  QString relName = mComboRelation->itemData( idx ).toString();
  QgsRelation rel = QgsProject::instance()->relationManager()->relation( relName );

  mReferencedLayer = rel.referencedLayer();
  mExpressionWidget->setLayer( mReferencedLayer ); // set even if 0
  if ( mReferencedLayer )
  {
    mExpressionWidget->setField( mReferencedLayer->displayExpression() );
    mCbxMapIdentification->setEnabled( mReferencedLayer->hasGeometryType() );
  }

  loadFields();
}

void QgsRelationReferenceConfigDlg::on_mAddFilterButton_clicked()
{
  Q_FOREACH ( QListWidgetItem* item, mAvailableFieldsList->selectedItems() )
  {
    addFilterField( item );
  }
}

void QgsRelationReferenceConfigDlg::on_mRemoveFilterButton_clicked()
{
  Q_FOREACH ( QListWidgetItem* item , mFilterFieldsList->selectedItems() )
  {
    mFilterFieldsList->takeItem( indexFromListWidgetItem( item ) );
    mAvailableFieldsList->addItem( item );
  }
}

QgsEditorWidgetConfig QgsRelationReferenceConfigDlg::config()
{
  QgsEditorWidgetConfig myConfig;
  myConfig.insert( "AllowNULL", mCbxAllowNull->isChecked() );
  myConfig.insert( "OrderByValue", mCbxOrderByValue->isChecked() );
  myConfig.insert( "ShowForm", mCbxShowForm->isChecked() );
  myConfig.insert( "MapIdentification", mCbxMapIdentification->isEnabled() && mCbxMapIdentification->isChecked() );
  myConfig.insert( "ReadOnly", mCbxReadOnly->isChecked() );
  myConfig.insert( "Relation", mComboRelation->itemData( mComboRelation->currentIndex() ) );

  if ( mFilterGroupBox->isChecked() )
  {
    QStringList filterFields;
    for ( int i = 0; i < mFilterFieldsList->count(); i++ )
    {
      filterFields << mFilterFieldsList->item( i )->data( Qt::UserRole ).toString();
    }
    myConfig.insert( "FilterFields", filterFields );

    myConfig.insert( "ChainFilters", mCbxChainFilters->isChecked() );
  }

  if ( mReferencedLayer )
  {
    mReferencedLayer->setDisplayExpression( mExpressionWidget->currentField() );
  }

  return myConfig;
}

void QgsRelationReferenceConfigDlg::loadFields()
{
  mAvailableFieldsList->clear();
  mFilterFieldsList->clear();

  if ( mReferencedLayer )
  {
    QgsVectorLayer* l = mReferencedLayer;
    const QgsFields& flds = l->pendingFields();
    for ( int i = 0; i < flds.count(); i++ )
    {
      mAvailableFieldsList->addItem( l->attributeAlias( i ).isEmpty() ? flds.at( i ).name() : l->attributeAlias( i ) );
      mAvailableFieldsList->item( mAvailableFieldsList->count() - 1 )->setData( Qt::UserRole, flds.at( i ).name() );
    }
  }
}

void QgsRelationReferenceConfigDlg::addFilterField( const QString& field )
{
  for ( int i = 0; i < mAvailableFieldsList->count(); i++ )
  {
    if ( mAvailableFieldsList->item( i )->data( Qt::UserRole ).toString() == field )
    {
      addFilterField( mAvailableFieldsList->item( i ) );
      break;
    }
  }
}

void QgsRelationReferenceConfigDlg::addFilterField( QListWidgetItem* item )
{
  mAvailableFieldsList->takeItem( indexFromListWidgetItem( item ) );
  mFilterFieldsList->addItem( item );
}

int QgsRelationReferenceConfigDlg::indexFromListWidgetItem( QListWidgetItem* item )
{
  QListWidget* lw = item->listWidget();

  for ( int i = 0; i < lw->count(); i++ )
  {
    if ( lw->item( i ) == item )
      return i;
  }

  return -1;
}
